# Authenticated certificate and audit schema

The schema is enforced by `verify_certificate.py`, using exact rational arithmetic for every numeric certificate or audit field that it consumes. This document is descriptive; the checker is fail-closed.

The working tree may use `results/certificate.json`. In `results/validation_result.zip`, the same file is named `certificate.json` at the archive root. The archive-root layout is canonical and is the only layout accepted by the final checker.

## Common encoding and authentication

Decimal quantities are canonical finite decimal strings, never JSON floats and never exponent notation. Examples are `"0"`, `"1.05"`, and `"-0.0025"`; `"-0"`, redundant trailing zeros, NaN, and infinity are rejected. Bounds in audits are two-element arrays `[lo,hi]`; published certificate bounds are objects `{"lo":lo,"hi":hi}`. Integer metadata remains JSON integer data.

Every JSON audit and `parameter_trials.json` is canonical compact JSON: sorted keys, no insignificant whitespace, and one final newline. Duplicate keys are rejected.

The archive contains `SHA256SUMS`, which covers every regular non-symlink file other than itself and no nonexistent file. `source_manifest.txt` canonically hashes exactly these frozen sources:

```text
Makefile
README.md
docs/CERTIFICATE_SCHEMA.md
docs/VALIDATION_SPEC.md
src/certify.py
src/interval_assemble.cpp
src/recurrence_reference.py
tests/smoke.py
tests/symbolic.py
verify_certificate.py
```

Beyond `SHA256SUMS`, a `PROVED` archive has exactly these ten sources plus the thirteen named common/proof artefacts below (23 regular files in total); a `FAILED` archive has the ten sources, nine common artefacts, and `failure_report.md` (20 in total). The checker rejects every extra file, including build products, caches, and import-shadowing modules, even if it is listed and hashed.

`source_sha256` is the hash of `source_manifest.txt`. The exact input is named `data/center_L60_S40_R30.hex`; `input_sha256` authenticates that file and is pinned by the checker. `inverse_sha256` authenticates the shipped `data/approx_inverse.bin`, whose entries are thereafter exact binary64 rationals. Paths must be relative canonical POSIX paths without symlinks or `..`. The checker is deliberately pinned to the exact centre hash and `L=60,S=40,R=J=30,rho=1.05`; the assembler's wider supported-input contract does not make the final checker generic.

All archives also include nonempty `data/smoke_center.hex`, authenticated `data/approx_inverse.bin`, `requirements.txt`, executable `reproduce.sh`, `versions.txt`, and `parameter_trials.json`. The latter contains one to three ordered trials. It starts at `L=60,S=40,R=30,J=30,rho="1.05"`; every nonfinal trial is `FAILED` with a rigorous `failed_bound`. The last trial is `SELECTED` for a proof, or `FAILED` with its failed bound in a failure archive.

## Compact numeric audit

Each heavy run writes one authenticated audit with the following shape. Ellipses below stand for complete maps, not optional sampling.

```json
{
  "precision_bits": 256,
  "input_sha256": "...",
  "source_sha256": "...",
  "inverse_sha256": "...",
  "provenance": {
    "backend": "MPFR",
    "backend_version": "...",
    "rounding": "directed",
    "mpfr_buildopt_tls_p": true,
    "threads": 80,
    "compiler": "...",
    "source_manifest_sha256": "..."
  },
  "support": {
    "finite_columns": 2471,
    "g_boundary_columns": 23941,
    "p_boundary_first": 31,
    "p_boundary_last": 150,
    "far_g_min_D": 684,
    "shape_shift_start": 151
  },
  "y": {
    "finite": ["0", "..."],
    "omitted": ["0", "..."],
    "omitted_witness": {
      "support": {"angular_max": 90, "radial_max": 341, "row_count": 24001},
      "terms": {"h:s": ["0", "..."]},
      "bound": ["0", "..."]
    }
  },
  "z": {
    "finite": {"column": [["0", "..."], ["0", "..."], ["0", "..."]]},
    "g_boundary": {"ell:s": [["0", "..."], ["0", "..."], ["0", "..."]]},
    "p_boundary": {"j": [["0", "..."], ["0", "..."], ["0", "..."]]},
    "far_g": ["0", "..."],
    "far_p": ["0", "..."],
    "far_p_witness": {
      "shift_start": 151,
      "weighted_terms": {"h": ["0", "..."]},
      "bound": ["0", "..."]
    }
  },
  "nonlinear": {
    "formula": "multilinear_global_kappa_1_8_v1",
    "inverse_norm": ["0", "..."],
    "p_norm": ["0", "..."],
    "u_norm": ["0", "..."]
  }
}
```

Every finite, `g`-boundary, and `p`-boundary entry is `[finite_part,tail_part,complete_column]`; the complete interval must enclose the sum of its first two intervals. Key sets must exactly equal the support derived from the frozen cutoffs. The omitted witness covers all 24,001 derived omitted rows. The far-shape witness covers every derived shifted positive mode. The checker recomputes the exact centre norms and the monotone far-`g` formula, and rejects missing or optimistic terms.

The final certificate references the canonical root files `audit_mpfr_192.json`, `audit_mpfr_256a.json`, and `audit_mpfr_256b.json` under `runs.mpfr_192`, `runs.mpfr_256a`, and `runs.mpfr_256b`, each by SHA-256; alternative paths are rejected. The two 256-bit files are byte-identical; every 256-bit interval nests inside its corresponding widened 192-bit interval. All runs authenticate the same exact input, frozen sources, MPFR version, thread-safe TLS build attestation, and shipped exact binary64 inverse. These are same-implementation precision and determinism checks, not an independent implementation.

## `PROVED` certificate

A successful certificate has these mandatory fields:

```json
{
  "status": "PROVED",
  "input_path": "data/center_L60_S40_R30.hex",
  "input_sha256": "...",
  "source_sha256": "...",
  "inverse_sha256": "...",
  "backend": {"name": "MPFR", "version": "...", "precision_bits": 256, "rounding": "directed"},
  "parameters": {"L": 60, "S": 40, "R": 30, "J": 30, "rho": "1.05", "dimension": 2471},
  "runs": {
    "mpfr_192": {"path": "...", "sha256": "..."},
    "mpfr_256a": {"path": "...", "sha256": "..."},
    "mpfr_256b": {"path": "...", "sha256": "..."}
  },
  "bounds": {
    "Y": {"lo": "0", "hi": "..."},
    "Z": {"lo": "0", "hi": "..."},
    "C2": {"lo": "0", "hi": "..."},
    "C3": {"lo": "0", "hi": "..."},
    "radius": "...",
    "univalence_sum": {"lo": "0", "hi": "..."},
    "p1_abs_lower": {"lo": "...", "hi": "..."}
  },
  "components": {
    "finite_Y": {"lo": "0", "hi": "..."},
    "omitted_Y": {"lo": "0", "hi": "..."},
    "finite_Z": {"lo": "0", "hi": "..."},
    "finite_to_tail_Z": {"lo": "0", "hi": "..."},
    "g_tail_Z": {"lo": "0", "hi": "..."},
    "p_tail_Z": {"lo": "0", "hi": "..."},
    "support_cutoffs": {
      "finite_columns": 2471,
      "g_boundary_columns": 23941,
      "p_boundary_first": 31,
      "p_boundary_last": 150,
      "far_g_min_D": 684,
      "shape_shift_start": 151
    }
  },
  "checks": {
    "radii_polynomial_upper": "...",
    "derivative_upper": "...",
    "univalence_margin_lower": "...",
    "passed": true
  }
}
```

The checker takes maxima of complete column sums to obtain the global `Z`, sums finite and omitted residual contributions for `Y`, and recomputes `C2`, `C3`, the two radii inequalities, the analytic-collar/univalence bound, and nonvanishing of `p1`. Published bounds must enclose these recomputed values and retain strict margins. A nonempty `proof_report.md` is mandatory. Checker exit zero establishes only authenticated-leaf aggregation and the final exact inequalities: it never executes the assembler or certification driver, does not itself establish that the supplied leaves enclose the recurrence, and does not mechanically prove the analytic prose. `reproduce.sh` separately checks frozen-source-and-authenticated-inverse leaf generation, but remains the same implementation rather than implementation diversity.

## Typed `FAILED` certificate

A rigorous failure archive has the same authenticated common fields (`status`, input path/hash, source hash, parameters, base artefacts, hashes, and trials), plus nonempty `failure_report.md` and:

```json
{
  "status": "FAILED",
  "failure": {
    "kind": "BOUND_NOT_CLOSED",
    "first_obligation": "D",
    "completed_obligations": ["A", "B", "C"],
    "inequality": "Z < 1",
    "test": "upper_lt",
    "threshold": "1",
    "rigorous_bound": ["0", "..."]
  }
}
```

`kind` is `BOUND_NOT_CLOSED` or `FALSE_ANALYTIC_LEMMA`. `first_obligation` is `A` through `F`, and `completed_obligations` is exactly the preceding prefix. The test identifies how the interval relates to the threshold: `upper_lt` records failure to prove an upper bound strictly below it, `lower_gt` failure to prove a lower bound strictly above it, while `interval_above` and `interval_below` rigorously locate the full interval on the named side. A `FAILED` archive is a rigorous diagnostic, never a numerical plausibility claim.

## Remaining archive files

A `PROVED` archive contains `proof_report.md`; a `FAILED` archive contains `failure_report.md`. Both contain the compact frozen source and build-description files, exact centre and smoke centre, authenticated `data/approx_inverse.bin`, `certificate.json`, `verify_certificate.py`, executable `reproduce.sh`, `requirements.txt`, `versions.txt`, `parameter_trials.json`, `source_manifest.txt`, `SHA256SUMS`, and only the compact audit data needed by the stated status. Generated interval matrices, build products, caches, and verbose logs are not shipped.
