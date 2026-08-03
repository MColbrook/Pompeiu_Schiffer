# Regular Pompeiu candidate: directed-rounding validator

This compact project performs the final computer-validation stage near the supplied 10-fold analytic candidate. Its target is a noncircular, bounded, simply connected planar domain with real-analytic Jordan boundary whose indicator Fourier transform vanishes on a circle.

The complete computer-assisted proof artefact, including the independently executable verifier, rigorous certificates, source code, and reproduction instructions, is archived at Zenodo: DOI: 10.5281/zenodo.21765287.

No residual or floating-point scout is a proof. The `PROVED` status requires every finite, omitted-row, cross, infinite-tail, nonlinear, radii, and geometry obligation to be enclosed. `verify_certificate.py` authenticates and aggregates the packaged audit leaves and checks the final exact inequalities; it neither regenerates those leaves nor mechanically verifies the analytic prose. `reproduce.sh` separately checks the frozen-source-and-authenticated-inverse-to-audit computation. A completed rigorous bound or lemma failure must be reported as a typed `FAILED` certificate; malformed or unsupported inputs instead terminate nonzero.

## One production path

`src/interval_assemble.cpp` is the single production interval backend. It evaluates the exact recurrence formulation with MPFR lower/upper directed rounding, deterministic OpenMP reductions, and exact zeros. Every entry point fails unless `mpfr_buildopt_tls_p()` attests a thread-safe TLS build, and complete audits record that attestation. The exact input is `data/center_L60_S40_R30.hex`; every hexadecimal binary64 literal is interpreted as its exact dyadic rational. The separately coded NumPy recurrence is a non-rigorous binary64 smoke oracle only.

The driver in `src/certify.py`:

1. reads the MPFR finite residual/Jacobian enclosure;
2. either selects a binary64 LAPACK approximate inverse for an exploratory run or reuses a supplied inverse, after which every entry is treated as exact;
3. invokes the rigorous finite verifier and the recurrence-based finite/tail enumerator;
4. writes a canonical compact numeric audit.

The finite product enclosure uses fixed-order FMA products with an order-independent floating-point error bound, plus the MPFR interval radii. It does not perform a dense interval inverse. No quadrature, collocation, Bessel roots, or moment-map implementation is used.

The validator entry points that accept dimensions or precision share a fail-closed input contract: `L>=0`, `S>=1`, `0<=R,J<=L`, each cutoff is at most `INT_MAX/128`, the derived dimension fits a signed index, and precision is a nonempty ASCII digit string in `[64,4096]`. A centre contains exactly `R+1+(L+1)S` finite exact-binary64 hexadecimal coefficients and has `p0>0`; interval headers contain the derived `N`, and interval and inverse payload sizes are exact. Unsupported dimensions, inconsistent headers, malformed tokens, and incorrectly sized payloads are rejected before proof arithmetic. The final checker is deliberately fixed to the authenticated `L=60,S=40,R=J=30,rho=1.05` instance.

## Build and tests

Requirements are GNU Make, a C++17 compiler with OpenMP, MPFR/GMP discoverable through `pkg-config`, Python 3, and NumPy.

```bash
make
make smoke
make symbolic
make stats
```

`make smoke` runs 192- and 256-bit tiny assemblies, nesting/regression checks, rigorous finite and complete-tail verification, a deterministic repeat, exact-rational tail oracles, malformed-input rejection tests, and an adversarial nested-`SHA256SUMS` allowlist regression. `make symbolic` checks representative positive, zero, and negative angular recurrences with exact rational polynomial arithmetic. These tests validate machinery; neither alone proves the theorem.

## Production and staged reproduction

The low-level production interfaces are:

```text
work/interval_assemble CENTER INTERVALS BITS
work/interval_assemble --verify-finite CENTER INTERVALS INVERSE OUTPUT
work/interval_assemble --verify-tails CENTER INVERSE OUTPUT BITS
python3 src/certify.py --center CENTER --intervals INTERVALS \
  --work RUN_DIR --output AUDIT --inverse INVERSE [--reuse-inverse] \
  --complete-audit --source-manifest SOURCE_MANIFEST
```

The final workflow performs one 192-bit run and two byte-deterministic 256-bit runs with the same shipped exact inverse. These are same-implementation precision/determinism checks, not implementation diversity. A configurable `THREADS` controls OpenMP; BLAS remains single-threaded.

During assembly, the working certificate is `results/certificate.json`. The final zip is staged so that `certificate.json`, `verify_certificate.py`, and `reproduce.sh` are at the archive root. From an extracted archive, the frozen-source-and-inverse-to-audit reproduction and the separately scoped authenticated-leaf check are:

```bash
THREADS=80 ./reproduce.sh
python3 verify_certificate.py certificate.json
```

`reproduce.sh` rebuilds all interval and audit products from the compact exact centre while reusing the shipped, authenticated inverse. Optional inverse regeneration is non-authoritative because BLAS/LAPACK implementations need not choose byte-identical entries. The checked archive records compiler, MPFR/GMP, NumPy, node, thread, and inverse provenance.

## Inputs and audit artefacts

- `docs/VALIDATION_SPEC.md` fixes the theorem, operator, basis, scaling, support decomposition, and analytic inequalities.
- `docs/CERTIFICATE_SCHEMA.md` fixes the authenticated certificate and audit formats.
- `data/center_L60_S40_R30.hex` is the theorem input.
- `data/approx_inverse.bin` is the authenticated binary64 approximate inverse used by every packaged audit.
- `src/recurrence_reference.py` is a separately coded, non-rigorous binary64 smoke oracle.

The six Python/C++ production files are counted exactly once by `make stats`, which fails above 3,000 nonblank, noncomment lines. The cap was transparently raised from 2,500 to accommodate fail-closed hardening and exact regressions; it is anti-sprawl hygiene, not proof evidence. Generated matrices, build trees, caches, and verbose logs are not final artefacts; the authenticated frozen inverse is the deliberate exception because its exact bytes are a proof input.
