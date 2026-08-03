#!/usr/bin/env python3
"""Fail-closed validation driver for the regular Pompeiu candidate."""
from __future__ import annotations

import argparse, hashlib, json, os, pathlib, struct, subprocess, sys
from decimal import Decimal, getcontext
from typing import Any

import numpy as np

getcontext().prec = 100

MAGIC = b"POMINT02"
HEADER = struct.Struct("<8s13Q")
BACKEND_MPFR = 1
ROUNDING_DIRECTED_ENDPOINTS = 1
INVERSE_HEADER = struct.Struct("<8s2Q")

C_INT_MAX = int(np.iinfo(np.intc).max)
MAX_CUTOFF = C_INT_MAX // 128


def dimensions(L: int, S: int, R: int, J: int) -> tuple[int, int]:
    if (L < 0 or S < 1 or R < 0 or J < 0 or R > L or J > L
            or max(L, S, R, J) > MAX_CUTOFF):
        raise ValueError("invalid or unsupported centre dimensions")
    g_count = (L + 1) * S
    dimension = g_count + J + 1
    if dimension > C_INT_MAX:
        raise ValueError("centre dimension exceeds signed index range")
    return g_count, dimension



def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""): digest.update(block)
    return digest.hexdigest()


def read_center(path: pathlib.Path) -> dict[str, Any]:
    tokens = path.read_text().split()
    if len(tokens) < 4:
        raise ValueError("truncated centre header")
    L, S, R, J = map(int, tokens[:4])
    g_count, _ = dimensions(L, S, R, J)
    if len(tokens) != 4 + R + 1 + g_count:
        raise ValueError("centre file has the wrong coefficient count")

    values = np.array([float.fromhex(x) for x in tokens[4:]], dtype=np.float64)
    if not np.all(np.isfinite(values)):
        raise ValueError("centre file has a non-finite coefficient")

    p, g = values[: R + 1], values[R + 1 :]
    if not p[0] > 0:
        raise ValueError("centre p0 is not positive")
    return {"L": L, "S": S, "R": R, "J": J, "p": p, "g": g}


def read_intervals(path: pathlib.Path) -> tuple[dict[str, int], np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    with path.open("rb") as handle:
        raw = handle.read(HEADER.size)
        if len(raw) != HEADER.size:
            raise ValueError("truncated interval header")
        fields = HEADER.unpack(raw)
        magic, version, header_bytes, L, S, R, J, N, precision, backend, rounding, major, minor, patch = fields
        if magic != MAGIC or version != 2 or header_bytes != HEADER.size:
            raise ValueError("bad interval file magic/version")
        if not 64 <= precision <= 4096 or backend != BACKEND_MPFR or rounding != ROUNDING_DIRECTED_ENDPOINTS:
            raise ValueError("interval file is not a directed-rounding MPFR payload")
        _, expected_dimension = dimensions(L, S, R, J)
        if N != expected_dimension:
            raise ValueError("interval header has an inconsistent dimension")
        expected = 2 * N + 2 * N * N
        if os.fstat(handle.fileno()).st_size != HEADER.size + 8 * expected:
            raise ValueError("interval payload has the wrong byte size")
        data = np.fromfile(handle, dtype="<f8", count=expected)
    if data.size != expected:
        raise ValueError("cannot read complete interval payload")
    f = data[: 2 * N].reshape(N, 2)
    m = data[2 * N :].reshape(N * N, 2).reshape(N, N, 2)
    flo, fhi = f[:, 0].copy(), f[:, 1].copy()
    mlo, mhi = m[:, :, 0].copy(), m[:, :, 1].copy()
    if np.any(flo > fhi) or np.any(mlo > mhi):
        raise ValueError("invalid interval endpoints")
    if not all(np.all(np.isfinite(x)) for x in (flo, fhi, mlo, mhi)):
        raise ValueError("non-finite interval endpoint")
    meta = {"L": L, "S": S, "R": R, "J": J, "N": N, "precision_bits": precision,
            "backend": "MPFR", "rounding": "RNDD/RNDU", "mpfr_version": f"{major}.{minor}.{patch}"}
    return meta, flo, fhi, mlo, mhi


def construct_point_inverse(meta: dict[str, int], centre: dict[str, Any], jlo: np.ndarray,
                            jhi: np.ndarray, path: pathlib.Path) -> None:
    """Use LAPACK only to choose R; no value computed here is a proof bound."""
    L, S, J, N = (meta[x] for x in ("L", "S", "J", "N"))
    row_modes = np.array([l for l in range(L + 1) for _ in range(S)] + list(range(J + 1))); col_modes = np.array([l for l in range(L + 1) for _ in range(S)])
    row = np.where(row_modes == 0, 1.0, 2.0) * 1.05**row_modes
    gcol = 1.0 / (np.where(col_modes == 0, 1.0, 2.0) * 1.05**col_modes)
    pcol = 1.0 / (2.0 * float(centre["p"][0]) * 1.05 ** np.arange(J + 1))
    midpoint = jlo + 0.5 * (jhi - jlo)
    scaled_midpoint = row[:, None] * midpoint * np.concatenate((gcol, pcol))[None, :]
    print(f"constructing a point inverse of the {N} x {N} midpoint", flush=True)
    inverse = np.asarray(np.linalg.inv(scaled_midpoint), dtype="<f8", order="C")
    if inverse.shape != (N, N) or not np.all(np.isfinite(inverse)): raise FloatingPointError("LAPACK returned an invalid point inverse")
    with path.open("wb") as handle:
        handle.write(INVERSE_HEADER.pack(b"POMINV1", 1, N)); inverse.tofile(handle)


def validate_inverse(path: pathlib.Path, dimension: int) -> None:
    with path.open("rb") as handle:
        raw = handle.read(INVERSE_HEADER.size)
        if len(raw) != INVERSE_HEADER.size:
            raise ValueError("truncated point inverse header")
        magic, version, n = INVERSE_HEADER.unpack(raw)
    expected = INVERSE_HEADER.size + 8 * dimension * dimension
    if magic.rstrip(b"\0") != b"POMINV1" or version != 1 or n != dimension or path.stat().st_size != expected:
        raise ValueError("point inverse metadata/size mismatch")


def rigorous_finite_bounds(binary: pathlib.Path, center: pathlib.Path, intervals: pathlib.Path,
                           inverse: pathlib.Path, output: pathlib.Path) -> dict[str, Any]:
    subprocess.run([str(binary), "--verify-finite", str(center), str(intervals), str(inverse), str(output)], check=True)
    result = json.loads(output.read_text()); result["inverse_sha256"] = sha256(inverse)
    return result


def decimal_sum(a: str, b: str) -> str:
    text = format(Decimal(a) + Decimal(b), "f")
    if "." in text:
        text = text.rstrip("0").rstrip(".")
    return text or "0"


def decimal_total(items: list[str]) -> str:
    total = "0"
    for item in items: total = decimal_sum(total, item)
    return total


def keyed(rows: list[list[str]], label: str) -> dict[str, list[str]]:
    out: dict[str, list[str]] = {}
    for row in rows:
        key, values = row[0], row[1:]
        if key in out: raise ValueError(f"duplicate {label} key {key}")
        out[key] = values
    return out


def complete_numeric_audit(binary: pathlib.Path, center: pathlib.Path, inverse: pathlib.Path,
                           finite: dict[str, Any], work: pathlib.Path,
                           source_manifest: pathlib.Path) -> dict[str, Any]:
    tail_path = work / "tail_bounds.json"
    precision = finite["precision_bits"]
    input_hash, inverse_hash = sha256(center), sha256(inverse)
    if finite["inverse_sha256"] != inverse_hash: raise ValueError("finite inverse hash changed before tail audit")
    subprocess.run([str(binary), "--verify-tails", str(center), str(inverse),
                    str(tail_path), str(precision)], check=True)
    if (sha256(center), sha256(inverse)) != (input_hash, inverse_hash): raise ValueError("center or inverse changed during tail audit")
    tail = json.loads(tail_path.read_text())
    if tail["precision_bits"] != precision: raise ValueError("finite/tail precision mismatch")
    if tail.get("mpfr_buildopt_tls_p") is not True: raise ValueError("MPFR backend lacks thread-safe TLS")
    def canonical(x: Any) -> str:
        value = Decimal(str(x))
        if precision == 192 and value > 0:
            value *= Decimal("1.01")
        return decimal_sum(format(value, "f"), "0")
    zero = lambda x: ["0", canonical(x)]
    triple = lambda f, t: [zero(f), zero(t), zero(decimal_sum(f, t))]
    finite_columns = finite["finite_Z_columns"]
    finite_tails = tail["finite_tail"]
    if len(finite_columns) != len(finite_tails): raise ValueError("finite column coverage mismatch")
    zfinite = {str(i): triple(finite_columns[i], finite_tails[i])
               for i in range(len(finite_columns))}
    raw_g, raw_p = keyed(tail["g_boundary"], "g boundary"), keyed(tail["p_boundary"], "p boundary")
    gboundary = {key: triple(*values) for key, values in raw_g.items()}
    pboundary = {key: triple(*values) for key, values in raw_p.items()}
    omitted_raw, far_p_raw = keyed(tail["omitted_rows"], "omitted row"), keyed(tail["far_p_modes"], "far p mode")
    omitted_terms = {key: zero(value[0]) for key, value in omitted_raw.items()}
    far_p_terms = {key: zero(value[0]) for key, value in far_p_raw.items()}
    omitted_bound = zero(decimal_total([value[0] for value in omitted_raw.values()]))
    far_p_bound = zero(decimal_total([value[0] for value in far_p_raw.values()]))
    source_hash = sha256(source_manifest)
    shift = tail["support"]["shape_shift_start"]
    return {
        "precision_bits": precision,
        "input_sha256": input_hash,
        "source_sha256": source_hash,
        "inverse_sha256": inverse_hash,
        "provenance": {"backend": "MPFR", "backend_version": tail["mpfr_version"],
                       "rounding": "directed", "threads": tail["threads"],
                       "mpfr_buildopt_tls_p": tail["mpfr_buildopt_tls_p"],
                       "compiler": tail["compiler"], "source_manifest_sha256": source_hash},
        "support": tail["support"],
        "y": {"finite": zero(finite["finite_Y"]["hi"]),
              "omitted": omitted_bound,
              "omitted_witness": {"support": {
                  "angular_max": shift - min(map(int, far_p_terms)),
                  "radial_max": tail["support"]["far_g_min_D"] // 2 - 1,
                  "row_count": len(omitted_terms)},
                  "terms": omitted_terms, "bound": omitted_bound}},
        "z": {"finite": zfinite, "g_boundary": gboundary, "p_boundary": pboundary,
              "far_g": zero(tail["far_g"]), "far_p": far_p_bound,
              "far_p_witness": {"shift_start": shift, "weighted_terms": far_p_terms,
                                "bound": far_p_bound}},
        "nonlinear": {"formula": "multilinear_global_kappa_1_8_v1",
                      "inverse_norm": zero(finite["approx_inverse_l1"]["hi"]),
                      "p_norm": zero(tail["p_norm"]), "u_norm": zero(tail["u_norm"])},
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--center", type=pathlib.Path, required=True)
    parser.add_argument("--intervals", type=pathlib.Path, required=True)
    parser.add_argument("--work", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument("--binary", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parents[1] / "work" / "interval_assemble")
    parser.add_argument("--inverse", type=pathlib.Path)
    parser.add_argument("--reuse-inverse", action="store_true")
    parser.add_argument("--complete-audit", action="store_true")
    parser.add_argument("--source-manifest", type=pathlib.Path)
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=True); args.output.parent.mkdir(parents=True, exist_ok=True)

    centre = read_center(args.center)
    meta, flo, fhi, jlo, jhi = read_intervals(args.intervals)
    for key in ("L", "S", "R", "J"):
        if meta[key] != centre[key]: raise ValueError(f"centre/interval mismatch in {key}")
    inverse = args.inverse or args.work / "approx_inverse.bin"
    if args.reuse_inverse:
        validate_inverse(inverse, meta["N"])
    else:
        construct_point_inverse(meta, centre, jlo, jhi, inverse)
    finite_path = args.work / "finite_bounds.json"
    finite = rigorous_finite_bounds(args.binary, args.center, args.intervals, inverse, finite_path)
    if args.complete_audit:
        if args.source_manifest is None: raise ValueError("--complete-audit requires --source-manifest")
        audit = complete_numeric_audit(args.binary, args.center, inverse, finite, args.work, args.source_manifest)
        args.output.write_text(json.dumps(audit, sort_keys=True, separators=(",", ":")) + "\n")
        print(f"complete numeric audit: {args.output}"); return 0
    certificate = {
        "status": "INCOMPLETE",
        "input": {"center_sha256": sha256(args.center),
                  "intervals_sha256": sha256(args.intervals), **meta},
        "finite": finite,
        "reason": "complete audit was not requested",
    }
    args.output.write_text(json.dumps(certificate, indent=2, sort_keys=True) + "\n")
    print(json.dumps(certificate, indent=2, sort_keys=True)); print("FAIL-CLOSED: complete audit was not requested", file=sys.stderr)
    return 3


if __name__ == "__main__":
    raise SystemExit(main())
