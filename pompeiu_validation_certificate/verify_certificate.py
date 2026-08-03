#!/usr/bin/env python3
"""Fail-closed authenticated-audit aggregation and final-inequality checker."""
from __future__ import annotations

import argparse, hashlib, json, re
from decimal import Decimal
from fractions import Fraction
from pathlib import Path, PurePosixPath
from typing import Any

DEC = re.compile(r"-?(?:0|[1-9][0-9]*)(?:\.[0-9]*[1-9])?\Z")
HASH = re.compile(r"[0-9a-f]{64}\Z")
EXPECTED_INPUT = "data/center_L60_S40_R30.hex"
EXPECTED_INPUT_SHA256 = "a30353f226a96c88a3d7e8b54b9d29d000c884fadb6cc52b47f176a874d73d29"
SOURCE_FILES = tuple(sorted(("Makefile", "README.md", "docs/CERTIFICATE_SCHEMA.md",
    "docs/VALIDATION_SPEC.md", "src/certify.py", "src/interval_assemble.cpp",
    "src/recurrence_reference.py", "tests/smoke.py", "tests/symbolic.py", "verify_certificate.py")))
COMMON_ARCHIVE_FILES = frozenset(SOURCE_FILES) | frozenset(("certificate.json",
    EXPECTED_INPUT, "data/approx_inverse.bin", "data/smoke_center.hex",
    "parameter_trials.json", "reproduce.sh", "requirements.txt",
    "source_manifest.txt", "versions.txt"))
PROVED_ARCHIVE_FILES = COMMON_ARCHIVE_FILES | frozenset(("audit_mpfr_192.json",
    "audit_mpfr_256a.json", "audit_mpfr_256b.json", "proof_report.md"))
FAILED_ARCHIVE_FILES = COMMON_ARCHIVE_FILES | frozenset(("failure_report.md",))


def bad(message: str) -> None: raise ValueError(message)


def must(condition: bool, message: str) -> None:
    if not condition: bad(message)


def no_duplicates(items: list[tuple[str, Any]]) -> dict[str, Any]:
    out: dict[str, Any] = {}
    for key, value in items:
        if key in out: bad(f"duplicate JSON key {key!r}")
        out[key] = value
    return out


def loads(raw: bytes, name: str) -> Any:
    try:
        return json.loads(raw.decode(), object_pairs_hook=no_duplicates,
            parse_float=lambda x: bad(f"JSON number {x} must be a decimal string in {name}"),
            parse_constant=lambda x: bad(f"nonfinite JSON value {x} in {name}"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        bad(f"invalid JSON in {name}: {exc}")


def num(value: Any, where: str) -> Fraction:
    must(isinstance(value, str) and DEC.fullmatch(value) is not None and value != "-0",
         f"{where} is not a canonical finite decimal string")
    return Fraction(Decimal(value))


def interval(value: Any, where: str, positive: bool = True) -> tuple[Fraction, Fraction]:
    must(isinstance(value, list) and len(value) == 2, f"{where} must be [lo,hi]")
    lo, hi = num(value[0], where + ".lo"), num(value[1], where + ".hi")
    must(lo <= hi and (not positive or lo >= 0), f"invalid interval order/sign at {where}")
    return lo, hi


def bound(value: Any, where: str) -> tuple[Fraction, Fraction]:
    must(isinstance(value, dict) and set(value) == {"lo", "hi"}, f"{where} must contain exactly lo and hi")
    return interval([value["lo"], value["hi"]], where)


def require(value: Any, keys: set[str], where: str) -> dict[str, Any]:
    must(isinstance(value, dict), f"{where} must be an object")
    missing = keys - set(value); must(not missing, f"missing {where} fields: {sorted(missing)}")
    return value


def digest(path: Path) -> str:
    out = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""): out.update(block)
    return out.hexdigest()


def member(root: Path, name: Any, where: str) -> Path:
    must(isinstance(name, str), f"{where} path is not a string")
    pure = PurePosixPath(name)
    must(not pure.is_absolute() and ".." not in pure.parts and bool(pure.parts)
         and name == pure.as_posix(), f"unsafe path at {where}: {name!r}")
    path = root.joinpath(*pure.parts)
    must(path.resolve().is_relative_to(root.resolve()) and not path.is_symlink()
         and path.is_file(), f"missing/non-regular file at {where}: {name!r}")
    return path


def archive_root(certificate: Path) -> Path:
    root = certificate.parent
    if (root / "SHA256SUMS").is_file(): return root
    bad("SHA256SUMS not found beside the root certificate")


def check_manifest(root: Path) -> dict[str, str]:
    listed: dict[str, str] = {}
    for line_no, line in enumerate((root / "SHA256SUMS").read_text().splitlines(), 1):
        match = re.fullmatch(r"([0-9a-f]{64})  (.+)", line)
        must(match is not None, f"malformed SHA256SUMS line {line_no}")
        name = match.group(2)
        must(name != "SHA256SUMS" and name not in listed, f"invalid/duplicate manifest member {name!r}")
        path = member(root, name, f"SHA256SUMS:{line_no}")
        must(digest(path) == match.group(1), f"SHA-256 mismatch for {name}")
        listed[name] = match.group(1)
    actual = {p.relative_to(root).as_posix() for p in root.rglob("*") if p.is_file()
              and not p.is_symlink()
              and p.relative_to(root).as_posix() != "SHA256SUMS"}
    must(not any(p.is_symlink() for p in root.rglob("*")) and set(listed) == actual,
         "SHA256SUMS does not cover exactly every non-symlink archive file")
    return listed


def check_source_manifest(root: Path, claimed: str) -> None:
    path = member(root, "source_manifest.txt", "source manifest")
    must(digest(path) == claimed, "source_sha256 does not authenticate source_manifest.txt")
    lines, canonical = path.read_text(encoding="ascii").splitlines(), []
    must(len(lines) == len(SOURCE_FILES), "source manifest has the wrong file count")
    for expected, line in zip(SOURCE_FILES, lines):
        match = re.fullmatch(r"([0-9a-f]{64})  (.+)", line)
        must(match is not None and match.group(2) == expected, f"source manifest entry must be {expected!r}")
        must(digest(member(root, expected, "source manifest")) == match.group(1),
             f"source hash mismatch for {expected}")
        canonical.append(f"{match.group(1)}  {expected}\n")
    must(path.read_bytes() == "".join(canonical).encode(), "source manifest is not canonical")


def support_sets(L: int, S: int, R: int, J: int) -> tuple[dict[str, int], set[str], set[str], set[str]]:
    n, radial, angular = (L + 1) * S + J + 1, S + 10 * R + 1, L + R
    finite = {str(i) for i in range(n)}
    g = {f"{ell}:{s}" for ell in range(L + 1) for s in range(S + 1, radial + 1)}
    g |= {f"{ell}:{s}" for ell in range(L + 1, angular + 1) for s in range(1, S + 2 + 10 * (angular - ell))}
    p = {str(j) for j in range(J + 1, 2 * L + R + 1)}
    support = {"finite_columns": n, "g_boundary_columns": len(g), "p_boundary_first": J + 1,
        "p_boundary_last": 2 * L + R, "far_g_min_D": 2 * (radial + 1),
        "shape_shift_start": 2 * L + R + 1}
    return support, finite, g, p


def omitted_support(L: int, S: int, R: int, J: int) -> tuple[dict[str, int], set[str], set[str]]:
    radial, angular = S + 10 * R + 1, L + R
    rows = {f"{h}:{s}" for h in range(L + 1) for s in range(S + 1, radial + 1)}
    rows |= {f"{h}:0" for h in range(J + 1, L + 1)}
    rows |= {f"{h}:{s}" for h in range(L + 1, angular + 1) for s in range(S + 2 + 10 * (angular - h))}
    meta = {"angular_max": angular, "radial_max": radial, "row_count": len(rows)}
    return meta, rows, {str(h) for h in range(L + 1, 3 * L + R + 2)}


def witness(value: Any, keys: set[str], support: dict[str, int] | None, where: str,
            terms_name: str = "terms") -> tuple[Fraction, Fraction]:
    needed = {terms_name, "bound"} | ({"support"} if support is not None else set())
    value = require(value, needed, where)
    if support is not None: must(value["support"] == support, f"{where}.support differs from derived support")
    terms = value[terms_name]
    must(isinstance(terms, dict) and set(terms) == keys, f"{where} term coverage mismatch")
    parts = [interval(terms[key], f"{where}.{terms_name}.{key}") for key in keys]
    total = sum((item[0] for item in parts), Fraction()), sum((item[1] for item in parts), Fraction())
    result = interval(value["bound"], where + ".bound")
    must(result[0] <= total[0] and result[1] >= total[1], f"{where}.bound omits terms")
    return result


def family(value: Any, keys: set[str], where: str) -> tuple[Fraction, Fraction, Fraction]:
    got = len(value) if isinstance(value, dict) else "non-object"
    must(isinstance(value, dict) and set(value) == keys, f"{where} key coverage mismatch: got {got}, expected {len(keys)}")
    maxima = [Fraction(0), Fraction(0), Fraction(0)]
    for key in keys:
        entry = value[key]
        must(isinstance(entry, list) and len(entry) == 3, f"{where}.{key} must be [finite,tail,complete]")
        parts = [interval(item, f"{where}.{key}[{i}]") for i, item in enumerate(entry)]
        combined, combined_lo = parts[0][1] + parts[1][1], parts[0][0] + parts[1][0]
        must(parts[2][0] <= combined_lo and parts[2][1] >= combined, f"incomplete column interval at {where}.{key}")
        maxima = [max(maxima[0], parts[0][1]), max(maxima[1], parts[1][1]),
                  max(maxima[2], parts[2][1], combined)]
    return tuple(maxima)  # type: ignore[return-value]


def aggregate(payload: Any, support: dict[str, int], finite: set[str], g: set[str], p: set[str],
              where: str, omitted: tuple[dict[str, int], set[str]] | None = None,
              shifted: set[str] | None = None) -> dict[str, Any]:
    payload = require(payload, {"precision_bits", "input_sha256", "source_sha256", "inverse_sha256",
        "provenance", "support", "y", "z", "nonlinear"}, where)
    must(payload["support"] == support, f"{where}.support differs from derived support")
    ykeys = {"finite", "omitted"} | ({"omitted_witness"} if omitted else set())
    y = require(payload["y"], ykeys, where + ".y")
    yf, yo = interval(y["finite"], where + ".y.finite"), interval(y["omitted"], where + ".y.omitted")
    if omitted:
        proof = witness(y["omitted_witness"], omitted[1], omitted[0], where + ".y.omitted_witness")
        must(yo[0] <= proof[0] and yo[1] >= proof[1], f"{where}.y.omitted omits its witness")
    zkeys = {"finite", "g_boundary", "p_boundary", "far_g", "far_p"} | ({"far_p_witness"} if shifted else set())
    z = require(payload["z"], zkeys, where + ".z")
    ff, ft, fc = family(z["finite"], finite, where + ".z.finite")
    gf, gt, gc = family(z["g_boundary"], g, where + ".z.g_boundary")
    pf, pt, pc = family(z["p_boundary"], p, where + ".z.p_boundary")
    far_g_iv = interval(z["far_g"], where + ".z.far_g")
    far_p_iv = interval(z["far_p"], where + ".z.far_p")
    if shifted:
        seed = require(z["far_p_witness"], {"shift_start", "weighted_terms", "bound"}, where + ".z.far_p_witness")
        must(seed["shift_start"] == support["shape_shift_start"], "wrong far-p shift start")
        proof = witness(seed, shifted, None, where + ".z.far_p_witness", "weighted_terms")
        must(far_p_iv[0] <= proof[0] and far_p_iv[1] >= proof[1], f"{where}.z.far_p omits its witness")
    far_g, far_p = far_g_iv[1], far_p_iv[1]
    nl = require(payload["nonlinear"], {"formula", "inverse_norm", "p_norm", "u_norm"}, where + ".nonlinear")
    must(nl["formula"] == "multilinear_global_kappa_1_8_v1", f"unsupported nonlinear formula in {where}")
    return {"Yf": yf[1], "Yo": yo[1], "Y": yf[1] + yo[1],
            "Zff": ff, "Zft": ft, "Zg": max(gc, far_g), "Zp": max(pc, far_p), "Z": max(fc, gc, pc, far_g, far_p),
            "alpha": interval(nl["inverse_norm"], where + ".nonlinear.inverse_norm")[1],
            "pnorm_iv": interval(nl["p_norm"], where + ".nonlinear.p_norm"),
            "unorm_iv": interval(nl["u_norm"], where + ".nonlinear.u_norm"), "far_g_iv": far_g_iv, "far_p_iv": far_p_iv,
            "gfinite": gf, "gtail": gt, "pfinite": pf, "ptail": pt}


def nested(low: Any, high: Any, where: str) -> None:
    if isinstance(low, list) and len(low) == 2 and all(isinstance(x, str) for x in low):
        a, b = interval(low, where + ".192", False), interval(high, where + ".256", False)
        must(a[0] <= b[0] <= b[1] <= a[1], f"256-bit interval is not nested at {where}")
    elif isinstance(low, dict) and isinstance(high, dict):
        must(set(low) == set(high), f"192/256 key coverage differs at {where}")
        for key in low: nested(low[key], high[key], where + "." + key)
    elif isinstance(low, list) and isinstance(high, list) and len(low) == len(high):
        for i, (a, b) in enumerate(zip(low, high)): nested(a, b, f"{where}[{i}]")
    elif low != high: bad(f"192/256 metadata differs at {where}")


def audit_file(root: Path, ref: Any, where: str) -> tuple[Path, bytes, Any]:
    ref = require(ref, {"path", "sha256"}, where)
    must(set(ref) == {"path", "sha256"}, f"unexpected audit-reference fields at {where}")
    must(isinstance(ref["sha256"], str) and HASH.fullmatch(ref["sha256"]) is not None, f"invalid hash at {where}")
    path = member(root, ref["path"], where)
    raw, actual = path.read_bytes(), digest(path)
    must(actual == ref["sha256"], f"audit hash mismatch at {where}")
    obj = loads(raw, str(path))
    canonical = (json.dumps(obj, sort_keys=True, separators=(",", ":"), ensure_ascii=True, allow_nan=False) + "\n").encode()
    must(raw == canonical, f"noncanonical audit JSON at {where}")
    return path.resolve(), raw, obj


def center(path: Path, params: dict[str, Any]) -> list[Fraction]:
    tokens = path.read_text(encoding="ascii").split()
    try:
        header = [int(x) for x in tokens[:4]]
    except ValueError: bad("invalid exact-center header")
    must(header == [params[k] for k in ("L", "S", "R", "J")],
         "exact-center header differs from parameters")
    expected = params["R"] + 1 + (params["L"] + 1) * params["S"]
    must(len(tokens) == expected + 4, "exact center has the wrong coefficient count")
    values: list[Fraction] = []
    for token in tokens[4:]:
        try:
            value = float.fromhex(token)
        except ValueError:
            bad("invalid hexadecimal binary64 center coefficient")
        must(value.hex().lower() == token.lower() and -float("inf") < value < float("inf"),
             "noncanonical/nonfinite binary64 center coefficient")
        values.append(Fraction(*value.as_integer_ratio()))
    return values


def center_norms(values: list[Fraction], params: dict[str, Any], rho: Fraction) -> tuple[Fraction, Fraction]:
    L, S, R = (params[key] for key in ("L", "S", "R"))
    pnorm, u = sum(abs(values[j]) * rho**j for j in range(R + 1)), {(0, 0): Fraction(1)}
    for ell in range(L + 1):
        for s in range(1, S + 1):
            value, D = values[R + 1 + ell * S + s - 1], 10 * ell + 2 * s
            for row, factor in ((s - 1, Fraction(1, 4 * D * (D + 1))), (s, Fraction(-1, 2 * D * (D + 2))),
                                (s + 1, Fraction(1, 4 * (D + 1) * (D + 2)))):
                u[ell, row] = u.get((ell, row), Fraction()) + value * factor
    unorm = sum((1 if ell == 0 else 2) * rho**ell * abs(value) for (ell, _), value in u.items())
    return pnorm, unorm


def canonical_json(path: Path, where: str) -> Any:
    raw = path.read_bytes(); obj = loads(raw, where)
    encoded = (json.dumps(obj, sort_keys=True, separators=(",", ":"), ensure_ascii=True, allow_nan=False) + "\n").encode()
    must(raw == encoded, f"{where} is not canonical JSON")
    return obj


def check_trials(root: Path, status: str) -> None:
    obj = require(canonical_json(member(root, "parameter_trials.json", "parameter trials"),
        "parameter_trials.json"), {"trials"}, "parameter trials")
    trials = obj["trials"]
    must(isinstance(trials, list) and 1 <= len(trials) <= 3, "must record one to three trials")
    for i, trial in enumerate(trials):
        trial = require(trial, {"parameters", "outcome", "reason"}, f"trial {i}")
        prm = require(trial["parameters"], {"L", "S", "R", "J", "rho"}, f"trial {i}.parameters")
        must(all(type(prm[k]) is int for k in ("L", "S", "R", "J")), f"trial {i} cutoffs are not integers")
        num(prm["rho"], f"trial {i}.rho")
        must(trial["outcome"] in {"FAILED", "SELECTED"} and isinstance(trial["reason"], str)
             and bool(trial["reason"].strip()), f"invalid trial {i} outcome/reason")
        if trial["outcome"] == "FAILED":
            interval(require(trial, {"failed_bound"}, f"trial {i}")["failed_bound"], f"trial {i}.failed_bound", False)
        must(i == len(trials) - 1 or trial["outcome"] == "FAILED", "a changed configuration requires a preceding failed trial")
    first = trials[0]["parameters"]
    must([first[k] for k in ("L", "S", "R", "J")] == [60, 40, 30, 30]
         and num(first["rho"], "first trial rho") == Fraction(21, 20), "wrong initial trial")

    expected = "SELECTED" if status == "PROVED" else "FAILED"
    must(trials[-1]["outcome"] == expected, "final trial outcome disagrees with certificate status")
    final = trials[-1]["parameters"]
    must([final[k] for k in ("L", "S", "R", "J")] == [60, 40, 30, 30]
         and num(final["rho"], "final trial rho") == Fraction(21, 20),
         "final trial parameters disagree with the fixed certificate instance")

def audit_meta(audit: dict[str, Any], bits: int, cert: dict[str, Any]) -> None:
    must(type(audit.get("precision_bits")) is int and audit["precision_bits"] == bits,
         f"audit precision label is not exact integer {bits}")
    inverse = audit.get("inverse_sha256")
    must(isinstance(inverse, str) and HASH.fullmatch(inverse) is not None and inverse == cert["inverse_sha256"],
         "audit inverse hash differs")
    prov = require(audit.get("provenance"), {"backend", "backend_version", "rounding", "threads",
        "mpfr_buildopt_tls_p", "compiler", "source_manifest_sha256"}, f"{bits}-bit provenance")
    must(set(prov) == {"backend", "backend_version", "rounding", "threads", "compiler",
                       "mpfr_buildopt_tls_p", "source_manifest_sha256"}, f"unexpected {bits}-bit provenance fields")
    must(prov["backend"] == "MPFR" and prov["backend_version"] == cert["backend"]["version"]
         and prov["rounding"] == "directed" and prov["mpfr_buildopt_tls_p"] is True, f"invalid {bits}-bit backend provenance")
    must(type(prov["threads"]) is int and 1 <= prov["threads"] <= 96 and isinstance(prov["compiler"], str)
         and bool(prov["compiler"].strip()), f"invalid {bits}-bit threads/compiler provenance")
    must(prov["source_manifest_sha256"] == cert["source_sha256"], f"{bits}-bit source provenance differs")


def encloses(iv: tuple[Fraction, Fraction], value: Fraction, where: str) -> None:
    must(iv[0] <= value <= iv[1], f"{where} does not enclose recomputed value")


def failed(certificate: dict[str, Any]) -> int:
    item = require(certificate.get("failure"), {"kind", "first_obligation", "completed_obligations",
        "inequality", "test", "threshold", "rigorous_bound"}, "failure")
    must(item["kind"] in {"BOUND_NOT_CLOSED", "FALSE_ANALYTIC_LEMMA"}
         and item["first_obligation"] in set("ABCDEF"), "invalid failure kind/obligation")
    must(isinstance(item["inequality"], str) and bool(item["inequality"].strip()), "empty failure inequality")
    order = "ABCDEF"; iv = interval(item["rigorous_bound"], "failure.rigorous_bound", False)
    threshold = num(item["threshold"], "failure.threshold")
    index = order.index(item["first_obligation"])
    must(item["completed_obligations"] == list(order[:index]), "failure obligations are not an A--F prefix")
    tests = {"upper_lt": iv[1] >= threshold, "lower_gt": iv[0] <= threshold,
             "interval_above": iv[0] > threshold, "interval_below": iv[1] < threshold}
    must(item["test"] in tests and tests[item["test"]], "failure interval does not prove non-closure")
    print(f"CERTIFICATE STATUS FAILED ({item['kind']}, obligation {item['first_obligation']})")
    return 2


def verify(path: Path) -> int:
    cert = require(loads(path.read_bytes(), str(path)), {"status", "input_sha256", "input_path",
        "source_sha256", "parameters"}, "certificate")
    root = archive_root(path)
    listed = check_manifest(root)
    must(cert["status"] in {"PROVED", "FAILED"}, "status must be PROVED or FAILED")
    expected_files = PROVED_ARCHIVE_FILES if cert["status"] == "PROVED" else FAILED_ARCHIVE_FILES
    must(set(listed) == expected_files,
         "archive members differ from the exact allowlist for its status")
    for key in ("input_sha256", "source_sha256"):
        must(isinstance(cert[key], str) and HASH.fullmatch(cert[key]) is not None, f"invalid {key}")
    must(cert["input_path"] == EXPECTED_INPUT and cert["input_sha256"] == EXPECTED_INPUT_SHA256,
         "certificate uses the wrong centre")
    check_source_manifest(root, cert["source_sha256"])
    for name in ("data/smoke_center.hex", "data/approx_inverse.bin", "requirements.txt", "reproduce.sh", "versions.txt"):
        must(bool(member(root, name, "required artifact").read_bytes()), f"required artifact {name} is empty")
    must(member(root, "reproduce.sh", "reproducer").stat().st_mode & 0o111, "reproduce.sh is not executable")
    check_trials(root, cert["status"])
    params = require(cert["parameters"], {"L", "S", "R", "J", "rho", "dimension"}, "parameters")
    must(all(type(params[k]) is int for k in ("L", "S", "R", "J", "dimension"))
         and [params[k] for k in ("L", "S", "R", "J", "dimension")] == [60, 40, 30, 30, 2471],
         "checker requires the integer L60/S40/R30/J30 split")
    rho = num(params["rho"], "parameters.rho")
    must(rho == Fraction(21, 20), "checker requires rho=1.05")
    input_path = member(root, EXPECTED_INPUT, "input_path")
    must(digest(input_path) == EXPECTED_INPUT_SHA256, "supplied exact centre hash differs")
    coefficients = center(input_path, params)
    if cert["status"] == "FAILED":
        must(bool(member(root, "failure_report.md", "failure report").read_text().strip()),
             "failure_report.md is empty")
        return failed(cert)
    cert = require(cert, {"backend", "inverse_sha256", "bounds", "components", "checks", "runs"}, "certificate")
    must(bool(member(root, "proof_report.md", "proof report").read_text().strip()), "proof_report.md is empty")
    must(isinstance(cert["inverse_sha256"], str)
         and HASH.fullmatch(cert["inverse_sha256"]) is not None, "invalid inverse_sha256")
    must(digest(member(root, "data/approx_inverse.bin", "frozen inverse"))
         == cert["inverse_sha256"], "frozen inverse hash differs from certificate")
    backend = require(cert["backend"], {"name", "version", "precision_bits", "rounding"}, "backend")
    must(backend["name"] == "MPFR" and type(backend["precision_bits"]) is int
         and backend["precision_bits"] == 256 and backend["rounding"] == "directed"
         and isinstance(backend["version"], str) and bool(backend["version"].strip()),
         "final backend must be directed-rounding MPFR at 256 bits")
    support, finite, g, p = support_sets(60, 40, 30, 30)
    omitted_meta, omitted_rows, shifted = omitted_support(60, 40, 30, 30)
    runs = require(cert["runs"], {"mpfr_192", "mpfr_256a", "mpfr_256b"}, "runs")
    records = [audit_file(root, runs[key], "runs." + key) for key in ("mpfr_192", "mpfr_256a", "mpfr_256b")]
    identities = [(item[0].stat().st_dev, item[0].stat().st_ino) for item in records]
    must(len(set(identities)) == 3, "the three audit runs are not distinct regular files")
    (_, raw192, audit192), (_, raw256a, audit256), (_, raw256b, audit256b) = records
    must(raw256a == raw256b, "256-bit numeric audit payloads are not byte-identical")
    for bits, audit in ((192, audit192), (256, audit256), (256, audit256b)):
        audit_meta(audit, bits, cert)
        must(audit.get("input_sha256") == cert["input_sha256"]
             and audit.get("source_sha256") == cert["source_sha256"],
             f"{bits}-bit audit input/source hashes differ")
    low, high = dict(audit192), dict(audit256)
    low.pop("precision_bits")
    high.pop("precision_bits")
    nested(low, high, "audit")
    args = (support, finite, g, p)
    q192 = aggregate(audit192, *args, "audit192", (omitted_meta, omitted_rows), shifted)
    q256 = aggregate(audit256, *args, "audit256", (omitted_meta, omitted_rows), shifted)
    b, p1 = coefficients[0], coefficients[1]
    must(b > 0, "center p0 is not positive")
    bounds = require(cert["bounds"], {"Y", "Z", "C2", "C3", "radius",
                                      "univalence_sum", "p1_abs_lower"}, "bounds")
    radius = num(bounds["radius"], "bounds.radius")
    must(radius > 0, "radius is not positive")
    exact_pnorm, exact_unorm = center_norms(coefficients, params, rho)
    D = support["far_g_min_D"]
    kappa = Fraction(1, 4 * D * (D + 1)) + Fraction(1, 2 * D * (D + 2)) + Fraction(
        1, 4 * (D + 1) * (D + 2))
    exact_far_g = exact_pnorm * exact_pnorm * kappa
    for name, q in (("192", q192), ("256", q256)):
        must(q["alpha"] >= 1, f"{name}-bit inverse norm omits the tail identity")
        encloses(q["pnorm_iv"], exact_pnorm, f"{name}-bit p norm")
        encloses(q["unorm_iv"], exact_unorm, f"{name}-bit U norm")
        encloses(q["far_g_iv"], exact_far_g, f"{name}-bit far-g formula")
        mixed = q["pnorm_iv"][1] / (16 * b)
        shape_square = q["unorm_iv"][1] / (4 * b * b)
        q["C2"], q["C3"] = q["alpha"] * max(mixed, shape_square), q["alpha"] / (96 * b * b)
        q["map"] = q["Y"] + q["Z"] * radius + q["C2"] * radius**2 + q["C3"] * radius**3 - radius
        q["derivative"] = q["Z"] + 2 * q["C2"] * radius + 3 * q["C3"] * radius**2 - 1
        must(q["map"] < 0 and q["derivative"] < 0, f"{name}-bit radii inequalities do not close")
    denominator = b - radius / (2 * b)
    center_tail = sum(abs(x) for x in coefficients[1:31])
    perturbation = radius / (2 * b * rho)
    must(denominator > 0, "p0 may vanish in the contraction ball")
    univalence = (center_tail + perturbation) / denominator
    p1_lo, p1_hi = abs(p1) - perturbation, abs(p1) + perturbation
    must(rho > 1 and univalence < 1 and p1_lo > 0,
         "analytic-collar, univalence, or non-disk geometry check fails")
    advertised = {key: bound(bounds[key], "bounds." + key) for key in ("Y", "Z", "C2", "C3")}
    for key in advertised:
        encloses(advertised[key], q256[key], "bounds." + key)
    upper = {key: advertised[key][1] for key in advertised}
    cert_map = upper["Y"] + upper["Z"] * radius + upper["C2"] * radius**2 + upper["C3"] * radius**3 - radius
    cert_derivative = upper["Z"] + 2 * upper["C2"] * radius + 3 * upper["C3"] * radius**2 - 1
    must(cert_map < 0 and cert_derivative < 0, "published bounds do not close the radii inequalities")
    ucert, pcert = bound(bounds["univalence_sum"], "bounds.univalence_sum"), bound(
        bounds["p1_abs_lower"], "bounds.p1_abs_lower")
    must(ucert[0] <= 0 <= univalence <= ucert[1] < 1,
         "published univalence interval is not strictly below one")
    must(0 < pcert[0] <= p1_lo <= p1_hi <= pcert[1],
         "published p1 interval does not prove nonvanishing")
    components = require(cert["components"], {"finite_Y", "omitted_Y", "finite_Z",
                                               "finite_to_tail_Z", "g_tail_Z", "p_tail_Z",
                                               "support_cutoffs"}, "components")
    values = {"finite_Y": q256["Yf"], "omitted_Y": q256["Yo"],
              "finite_Z": q256["Zff"], "finite_to_tail_Z": q256["Zft"],
              "g_tail_Z": q256["Zg"], "p_tail_Z": q256["Zp"]}
    for key, value in values.items():
        encloses(bound(components[key], "components." + key), value, "components." + key)
    must(components["support_cutoffs"] == support,
         "components.support_cutoffs differs from derived support")
    checks = require(cert["checks"], {"radii_polynomial_upper", "derivative_upper",
                                      "univalence_margin_lower", "passed"}, "checks")
    map_claim = num(checks["radii_polynomial_upper"], "checks.radii_polynomial_upper")
    derivative_claim = num(checks["derivative_upper"], "checks.derivative_upper")
    margin_claim = num(checks["univalence_margin_lower"], "checks.univalence_margin_lower")
    if not (checks["passed"] is True and cert_map <= map_claim < 0
            and cert_derivative <= derivative_claim < 0
            and 0 < margin_claim <= 1 - univalence):
        bad("claimed checks do not follow from exact recomputation")
    print(f"Y_upper={upper['Y']} Z_upper={upper['Z']}")
    print(f"C2_upper={upper['C2']} C3_upper={upper['C3']}")
    print(f"radii_polynomial_upper={cert_map} derivative_upper={cert_derivative}")
    print(f"univalence_margin_lower={1-univalence} p1_abs_lower={p1_lo}")
    print("AUTHENTICATED AUDIT AGGREGATION AND FINAL INEQUALITIES VERIFIED: PROVED")
    print("SOURCE-TO-AUDIT REGENERATION NOT PERFORMED BY THIS COMMAND")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("certificate", type=Path)
    args = parser.parse_args()
    try:
        return verify(args.certificate.resolve())
    except (OSError, ValueError, KeyError, TypeError) as exc:
        print(f"CERTIFICATE REJECTED: {exc}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
