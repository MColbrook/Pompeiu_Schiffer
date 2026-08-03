#!/usr/bin/env python3
from __future__ import annotations

import json
import runpy
import pathlib
import struct
import subprocess
import tempfile
import sys
from fractions import Fraction as Q

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src"))
from recurrence_reference import assemble_float  # noqa: E402

HEADER = struct.Struct("<8s13Q")
INVERSE_HEADER = struct.Struct("<8s2Q")
FLOAT_REGRESSION_TOL = 1.0e-12
WIDTH_TOL = 1.0e-12


def read_output(path: pathlib.Path, expected_precision: int):
    with path.open("rb") as handle:
        fields = HEADER.unpack(handle.read(HEADER.size))
        magic, version, header_bytes, L, S, R, J, N, precision, backend, rounding, major, minor, patch = fields
        data = np.fromfile(handle, dtype="<f8")
    assert magic == b"POMINT02" and version == 2 and header_bytes == HEADER.size
    assert precision == expected_precision and backend == 1 and rounding == 1
    expected_values = 2 * N + 2 * N * N
    assert data.size == expected_values
    f = data[: 2 * N].reshape(N, 2)
    m = data[2 * N :].reshape(N, N, 2)
    return (L, S, R, J, N), f, m, (major, minor, patch)


def validate_intervals(name: str, intervals: np.ndarray) -> np.ndarray:
    lower = intervals[..., 0]
    upper = intervals[..., 1]
    if not (np.all(np.isfinite(lower)) and np.all(np.isfinite(upper))):
        raise AssertionError(f"{name} contains a non-finite endpoint")
    if not np.all(lower <= upper):
        raise AssertionError(f"{name} contains reversed endpoints")
    midpoint = lower + 0.5 * (upper - lower)
    scaled_width = (upper - lower) / (1.0 + np.abs(midpoint))
    if np.max(scaled_width) > WIDTH_TOL:
        raise AssertionError(f"{name} enclosure is unexpectedly wide")
    return midpoint


def regression_error(midpoint: np.ndarray, reference: np.ndarray) -> float:
    return float(np.max(np.abs(midpoint - reference) / (1.0 + np.abs(reference))))


TAIL_TOL = Q(1, 2**48)


def tail_k_column(ell: int, s: int) -> tuple[Q, Q, Q]:
    degree = 10 * ell + 2 * s
    return (
        Q(1, 4 * degree * (degree + 1)),
        Q(-1, 2 * degree * (degree + 2)),
        Q(1, 4 * (degree + 1) * (degree + 2)),
    )


def multiply_z10(mode: int, vector: list[Q]) -> tuple[int, list[Q]]:
    for _ in range(10):
        out = [Q(0)] * (len(vector) + 1)
        for s, value in enumerate(vector):
            denominator = 2 * s + mode + 1
            out[s] += Q(s + mode + 1, denominator) * value
            if s:
                out[s - 1] += Q(s, denominator) * value
        mode += 1
        vector = out
    return mode, vector


def check_tail_upper(name: str, emitted: str, exact: Q) -> None:
    bound = Q(emitted)
    if exact == 0:
        if bound != 0:
            raise AssertionError(f"{name}: expected exact zero, got {bound}")
    elif not exact <= bound <= exact + TAIL_TOL:
        raise AssertionError(f"{name}: exact={exact}, emitted={bound}")


def keyed_tail(rows: list[list[str]], name: str) -> dict[str, list[str]]:
    result = {row[0]: row[1:] for row in rows}
    if len(result) != len(rows):
        raise AssertionError(f"duplicate {name} key")
    return result


def exact_tail_smoke(binary: pathlib.Path) -> None:
    with tempfile.TemporaryDirectory(prefix="pompeiu-exact-tail-") as raw:
        root = pathlib.Path(raw)
        center = root / "center.hex"
        inverse = root / "inverse.bin"
        output = root / "tail.json"
        repeat = root / "tail_repeat.json"
        center.write_text("1 1 1 1\n0x1p+0\n0x0p+0\n0x1p-2\n0x0p+0\n")
        identity = [1.0 if i == j else 0.0 for i in range(4) for j in range(4)]
        inverse.write_bytes(
            INVERSE_HEADER.pack(b"POMINV1", 1, 4) + struct.pack("<16d", *identity)
        )
        for destination in (output, repeat):
            subprocess.run(
                [str(binary), "--verify-tails", str(center), str(inverse),
                 str(destination), "192"],
                check=True,
            )
        if output.read_bytes() != repeat.read_bytes():
            raise AssertionError("tail verification is not deterministic")
        tail = json.loads(output.read_text())
        if tail.get("mpfr_buildopt_tls_p") is not True:
            raise AssertionError("tail verifier did not attest MPFR TLS")
        support = {
            "finite_columns": 4,
            "g_boundary_columns": 24,
            "p_boundary_first": 2,
            "p_boundary_last": 3,
            "far_g_min_D": 26,
            "shape_shift_start": 4,
        }
        if tail["precision_bits"] != 192 or tail["support"] != support:
            raise AssertionError("wrong exact-tail metadata/support")

        a0, b0, c0 = tail_k_column(0, 1)
        shape = [[Q(1, 4) * a0, Q(1, 4) * b0, Q(1, 4) * c0]]
        mode = 0
        for _ in range(4):
            mode, shifted = multiply_z10(mode, shape[-1])
            shape.append(shifted)
        shape_norms = [sum(map(abs, vector)) for vector in shape]
        expected_shape = [Q(1, 32), Q(1, 672), Q(1, 2112),
                          Q(1, 4352), Q(1, 7392)]
        if shape_norms != expected_shape:
            raise AssertionError("exact shape-shift oracle failed")

        check_tail_upper("Y omitted", tail["y_omitted"], Q(1, 192))
        check_tail_upper("p norm", tail["p_norm"], Q(1))
        check_tail_upper("u norm", tail["u_norm"], Q(33, 32))
        check_tail_upper("far g", tail["far_g"], sum(map(abs, tail_k_column(0, 13))))

        finite_expected = [
            tail_k_column(0, 1)[2],
            tail_k_column(1, 1)[2],
            Q(1, 192),
            sum(map(abs, shape[1][2:])),
        ]
        if len(tail["finite_tail"]) != 4:
            raise AssertionError("wrong finite-tail length")
        for i, exact in enumerate(finite_expected):
            check_tail_upper(f"finite tail {i}", tail["finite_tail"][i], exact)

        omitted = keyed_tail(tail["omitted_rows"], "omitted row")
        omitted_keys = (
            {f"{ell}:{s}" for ell in range(2) for s in range(2, 13)}
            | {f"2:{s}" for s in range(3)}
        )
        if set(omitted) != omitted_keys:
            raise AssertionError("wrong omitted-row support")
        for key, value in omitted.items():
            exact = Q(1, 192) if key == "0:2" else Q(0)
            check_tail_upper(f"omitted {key}", value[0], exact)

        g_boundary = keyed_tail(tail["g_boundary"], "g boundary")
        g_keys = (
            {f"{ell}:{s}" for ell in range(2) for s in range(2, 13)}
            | {"2:1", "2:2"}
        )
        if set(g_boundary) != g_keys:
            raise AssertionError("wrong g-boundary support")
        for ell in range(2):
            for s in range(2, 13):
                first, middle, last = tail_k_column(ell, s)
                finite = first if s == 2 else Q(0)
                omitted_part = abs(middle) + last if s == 2 else first + abs(middle) + last
                values = g_boundary[f"{ell}:{s}"]
                check_tail_upper(f"g {ell}:{s} finite", values[0], finite)
                check_tail_upper(f"g {ell}:{s} tail", values[1], omitted_part)
        for s in (1, 2):
            values = g_boundary[f"2:{s}"]
            check_tail_upper(f"g 2:{s} finite", values[0], Q(0))
            check_tail_upper(f"g 2:{s} tail", values[1],
                             sum(map(abs, tail_k_column(2, s))))

        p_boundary = keyed_tail(tail["p_boundary"], "p boundary")
        if set(p_boundary) != {"2", "3"}:
            raise AssertionError("wrong p-boundary support")
        for j in (2, 3):
            check_tail_upper(f"p {j} finite", p_boundary[str(j)][0], Q(0))
            check_tail_upper(f"p {j} tail", p_boundary[str(j)][1], shape_norms[j])

        far_modes = keyed_tail(tail["far_p_modes"], "far-p mode")
        if set(far_modes) != {"2", "3", "4", "5"}:
            raise AssertionError("wrong far-p support")
        for harmonic, values in far_modes.items():
            exact = shape_norms[4] if harmonic == "4" else Q(0)
            check_tail_upper(f"far-p mode {harmonic}", values[0], exact)
        check_tail_upper("far p", tail["far_p"], Q(1, 7392))


def expect_failure(command: list[str], output: pathlib.Path | None = None,
                   needle: str | None = None) -> None:
    if output is not None:
        output.unlink(missing_ok=True)
    completed = subprocess.run(command, text=True, capture_output=True)
    if completed.returncode == 0:
        raise AssertionError(f"malformed input was accepted: {command!r}")
    diagnostic = completed.stdout + completed.stderr
    if needle is not None and needle not in diagnostic:
        raise AssertionError(f"wrong rejection for {command!r}: {diagnostic}")
    if output is not None and output.exists():
        raise AssertionError(f"failed command left an output: {output}")


def manifest_allowlist_smoke() -> None:
    checker = runpy.run_path(str(ROOT / "verify_certificate.py"))
    check_manifest, digest = checker["check_manifest"], checker["digest"]
    with tempfile.TemporaryDirectory(prefix="pompeiu-manifest-") as raw:
        root = pathlib.Path(raw)
        tracked = root / "tracked.txt"
        tracked.write_text("tracked\n", encoding="ascii")
        (root / "SHA256SUMS").write_text(
            f"{digest(tracked)}  tracked.txt\n", encoding="ascii")
        check_manifest(root)
        nested = root / "junk"
        nested.mkdir()
        (nested / "SHA256SUMS").write_text("unlisted\n", encoding="ascii")
        try:
            check_manifest(root)
        except ValueError as exc:
            if "does not cover exactly" not in str(exc): raise
        else:
            raise AssertionError("nested unlisted SHA256SUMS was accepted")


def malformed_input_smoke(binary: pathlib.Path, centre: pathlib.Path,
                          intervals: pathlib.Path, inverse: pathlib.Path) -> None:
    with tempfile.TemporaryDirectory(prefix="pompeiu-malformed-") as raw:
        root = pathlib.Path(raw)
        rejected = root / "rejected"
        for token in ("256junk", "256.0", "+256", " 256", "256 ", "63", "4097", ""):
            expect_failure([str(binary), str(centre), str(rejected), token], rejected)
            expect_failure([str(binary), "--verify-tails", str(centre), str(inverse),
                            str(rejected), token], rejected)

        invalid_headers = {
            "r_gt_l": (
                "1 1 2 1\n0x1p+0\n0x0p+0\n0x0p+0\n0x0p+0\n0x0p+0\n"
            ),
            "j_gt_l": "1 1 1 2\n0x1p+0\n0x0p+0\n0x1p-2\n0x0p+0\n",
            "zero_s": "1 0 0 0\n0x1p+0\n",
            "negative_l": "-1 1 0 0\n0x1p+0\n",
            "extreme_l": "2147483647 1 0 0\n0x1p+0\n",
        }
        certify = ROOT / "src" / "certify.py"
        for name, text in invalid_headers.items():
            invalid = root / f"{name}.hex"
            invalid.write_text(text)
            expect_failure([str(binary), str(invalid), str(rejected), "192"], rejected, "dimension")
            expect_failure([str(binary), "--verify-tails", str(invalid), str(inverse),
                            str(rejected), "192"], rejected, "dimension")
            expect_failure([
                sys.executable, str(certify), "--center", str(invalid),
                "--intervals", str(intervals), "--work", str(root / f"work_{name}"),
                "--output", str(rejected),
            ], rejected, "dimension")
            try:
                assemble_float(invalid)
            except ValueError:
                pass
            else:
                raise AssertionError(f"reference parser accepted {name}")

        small = root / "small.hex"
        small.write_text("1 1 1 1\n0x1p+0\n0x0p+0\n0x1p-2\n0x0p+0\n")
        invalid_interval_headers = {
            "r_gt_l": (1, 1, 2, 1, 4),
            "j_gt_l": (1, 1, 1, 2, 5),
            "zero_s": (1, 0, 0, 0, 1),
            "extreme_l": (2147483647, 1, 0, 0, 1),
            "forged_n": (1, 1, 1, 1, 5),
        }
        for name, (L, S, R, J, N) in invalid_interval_headers.items():
            payload = root / f"{name}.bin"
            header = HEADER.pack(
                b"POMINT02", 2, HEADER.size, L, S, R, J, N,
                192, 1, 1, 4, 2, 1,
            )
            payload.write_bytes(header + bytes(8 * (2 * N + 2 * N * N)))
            expect_failure([str(binary), "--verify-finite", str(small), str(payload),
                            str(inverse), str(rejected)], rejected, "dimension")
            expect_failure([
                sys.executable, str(certify), "--center", str(small),
                "--intervals", str(payload), "--work", str(root / f"payload_{name}"),
                "--output", str(rejected),
            ], rejected, "dimension")

        interval_bytes = intervals.read_bytes()
        for name, content in (("truncated", interval_bytes[:-1]),
                              ("oversized", interval_bytes + b"\0")):
            payload = root / f"{name}.bin"
            payload.write_bytes(content)
            expect_failure([str(binary), "--verify-finite", str(centre), str(payload),
                            str(inverse), str(rejected)], rejected)
            expect_failure([
                sys.executable, str(certify), "--center", str(centre),
                "--intervals", str(payload), "--work", str(root / f"size_{name}"),
                "--output", str(rejected),
            ], rejected)

        inverse_bytes = inverse.read_bytes()
        for name, content in (("truncated", inverse_bytes[:-1]),
                              ("oversized", inverse_bytes + b"\0")):
            bad_inverse = root / f"inverse_{name}.bin"
            bad_inverse.write_bytes(content)
            expect_failure([str(binary), "--verify-finite", str(centre), str(intervals),
                            str(bad_inverse), str(rejected)], rejected)
            expect_failure([str(binary), "--verify-tails", str(centre), str(bad_inverse),
                            str(rejected), "192"], rejected)

        base_tokens = centre.read_text().split()
        coefficient_cases = {
            "missing": base_tokens[:-1],
            "extra": base_tokens + ["0x0p+0"],
            "nonfinite": base_tokens[:4] + ["nan"] + base_tokens[5:],
            "p0_zero": base_tokens[:4] + ["0x0p+0"] + base_tokens[5:],
            "p0_negative": base_tokens[:4] + ["-0x1p+0"] + base_tokens[5:],
        }
        for name, coefficients in coefficient_cases.items():
            malformed = root / f"coeff_{name}.hex"
            malformed.write_text("\n".join(coefficients) + "\n")
            expect_failure([str(binary), str(malformed), str(rejected), "192"], rejected)
            expect_failure([str(binary), "--verify-finite", str(malformed), str(intervals),
                            str(inverse), str(rejected)], rejected)
            expect_failure([str(binary), "--verify-tails", str(malformed), str(inverse),
                            str(rejected), "192"], rejected)
            expect_failure([
                sys.executable, str(certify), "--center", str(malformed),
                "--intervals", str(intervals), "--work", str(root / f"coeff_{name}"),
                "--output", str(rejected),
            ], rejected)

            try:
                assemble_float(malformed)
            except ValueError:
                pass
            else:
                raise AssertionError(f"reference parser accepted coefficient case {name}")

def main() -> int:
    manifest_allowlist_smoke()
    binary = ROOT / "work" / "interval_assemble"
    centre = ROOT / "data" / "smoke_center.hex"
    output192 = ROOT / "work" / "smoke_intervals.bin"
    output256 = ROOT / "work" / "smoke_intervals_256.bin"
    subprocess.run([str(binary), str(centre), str(output192), "192"], check=True)
    subprocess.run([str(binary), str(centre), str(output256), "256"], check=True)
    meta, fi, ji, mpfr_version = read_output(output192, 192)
    meta256, fi256, ji256, mpfr_version256 = read_output(output256, 256)
    assert meta256 == meta and mpfr_version256 == mpfr_version
    f, j = assemble_float(centre)
    assert meta[-1] == len(f)
    fm = validate_intervals("192-bit residual", fi)
    jm = validate_intervals("192-bit Jacobian", ji)
    validate_intervals("256-bit residual", fi256)
    validate_intervals("256-bit Jacobian", ji256)
    f_error = regression_error(fm, f)
    j_error = regression_error(jm, j)
    # The separately coded NumPy recurrence is a regression oracle, not a rigorous
    # enclosure: its accumulated binary64 roundoff need not lie inside the much
    # tighter MPFR interval. Compare midpoints at a deliberately loose scale.
    if f_error > FLOAT_REGRESSION_TOL or j_error > FLOAT_REGRESSION_TOL:
        raise AssertionError(
            f"float midpoint regression failed: F={f_error:.3e}, J={j_error:.3e}"
        )
    if not (
        np.all(fi[:, 0] <= fi256[:, 0])
        and np.all(fi256[:, 1] <= fi[:, 1])
        and np.all(ji[:, :, 0] <= ji256[:, :, 0])
        and np.all(ji256[:, :, 1] <= ji[:, :, 1])
    ):
        raise AssertionError("256-bit smoke enclosure is not nested in 192-bit enclosure")
    finite_work = ROOT / "work" / "smoke_finite_test"
    finite_work.mkdir(exist_ok=True)
    inverse = finite_work / "approx_inverse.bin"
    certificates = []
    for precision, intervals, reuse in ((192, output192, False), (256, output256, True)):
        certificate = finite_work / f"certificate_{precision}.json"
        command = [sys.executable, str(ROOT / "src" / "certify.py"),
                   "--center", str(centre), "--intervals", str(intervals),
                   "--work", str(finite_work), "--output", str(certificate),
                   "--inverse", str(inverse)]
        if reuse:
            command.append("--reuse-inverse")
        completed = subprocess.run(command, text=True, capture_output=True)
        if completed.returncode != 3:
            raise AssertionError(f"finite verifier failed ({precision} bits): {completed.stderr}")
        certificates.append(json.loads(certificate.read_text()))
    for key in ("finite_Y", "finite_Z"):
        if float(certificates[1]["finite"][key]["hi"]) > float(certificates[0]["finite"][key]["hi"]):
            raise AssertionError(f"256-bit {key} bound is not nested in the 192-bit bound")
    if certificates[0]["finite"]["inverse_sha256"] != certificates[1]["finite"]["inverse_sha256"]:
        raise AssertionError("finite reruns did not use the same exact binary64 inverse")
    finite_first = finite_work / "finite_bounds.json"
    finite_repeat = finite_work / "finite_bounds_repeat.json"
    subprocess.run([str(binary), "--verify-finite", str(centre), str(output256),
                    str(inverse), str(finite_repeat)], check=True)
    if finite_repeat.read_bytes() != finite_first.read_bytes():
        raise AssertionError("finite verification is not deterministic")
    exact_tail_smoke(binary)
    complete_audit = finite_work / "complete_audit.json"
    subprocess.run([
        sys.executable, str(ROOT / "src" / "certify.py"),
        "--center", str(centre), "--intervals", str(output256),
        "--work", str(finite_work / "complete"), "--output", str(complete_audit),
        "--binary", str(binary), "--inverse", str(inverse), "--reuse-inverse",
        "--complete-audit", "--source-manifest", str(ROOT / "source_manifest.txt"),
    ], check=True)
    complete = json.loads(complete_audit.read_text())
    if (complete["precision_bits"] != 256
            or complete["support"]["finite_columns"] != meta[-1]
            or complete["provenance"].get("mpfr_buildopt_tls_p") is not True):
        raise AssertionError("complete-audit smoke metadata mismatch")
    family_counts = {
        "finite": len(complete["z"]["finite"]),
        "g_boundary": len(complete["z"]["g_boundary"]),
        "p_boundary": len(complete["z"]["p_boundary"]),
        "omitted": len(complete["y"]["omitted_witness"]["terms"]),
        "far_p": len(complete["z"]["far_p_witness"]["weighted_terms"]),
    }
    expected_counts = {"finite": 11, "g_boundary": 81, "p_boundary": 5,
                       "omitted": 84, "far_p": 7}
    if family_counts != expected_counts:
        raise AssertionError(f"complete-audit family coverage mismatch: {family_counts}")
    malformed_input_smoke(binary, centre, output256, inverse)
    widths = ji[:, :, 1] - ji[:, :, 0]
    print(
        "smoke PASS",
        {
            "backend": "MPFR",
            "mpfr_version": mpfr_version,
            "mpfr_buildopt_tls_p": True,
            "precision_bits": [192, 256],
            "N": meta[-1],
            "max_F_width": float(np.max(fi[:, 1] - fi[:, 0])),
            "max_J_width": float(np.max(widths)),
            "float_F_scaled_error": f_error,
            "float_J_scaled_error": j_error,
            "nested_256_in_192": True,
            "finite_Z_192": certificates[0]["finite"]["finite_Z"]["hi"],
            "finite_verifier": "fixed-order FMA with MPFR bounds",
            "deterministic": True,
        },
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
