# Reproduction report

**Audit date:** 2 August 2026  
**Manifest-hardened source-to-audit generation:** **PASS**  
**Static authentication and exact checking of the assembled candidate ZIP:** **PASS**  
**Clean packaged source-to-certificate reproduction:** **PASS** — Slurm job `32612929` completed `0:0`  
**Overall manifest-hardened release gate:** **PASS**

## Current manifest-hardened release

### Frozen identities

| Object | SHA-256 | Current status |
|---|---|---|
| manifest-hardened candidate ZIP | `95d06a084d737c478118b7b8254836e57df39373da850985360a1a1068d9cae2` | static and clean packaged reproduction PASS |
| inner `SHA256SUMS` | `854adca1dc6bae2e7e2e977be83c19d3af52c5d8cba87b48d269e521289003f8` | PASS |
| `certificate.json` | `8aae465d483c4f60a92818616dc224f7aa69aa3adfc65dbc03ba944d6ccc2315` | exact checker PASS |
| manifest-hardened source manifest | `2057deab6a10db09c8df250205f6a52807b5cde5ffba899a9bdbac7be3749ed5` | PASS |
| exact hexadecimal centre | `a30353f226a96c88a3d7e8b54b9d29d000c884fadb6cc52b47f176a874d73d29` | unchanged; PASS |
| frozen `approx_inverse.bin` | `aa2de444bcae9e0c9bdf8afc8f5290ef5c363d84caf1c1de2df31fe569f9ea1c` | unchanged; reused without regeneration |
| manifest-hardened 192-bit audit | `3bf1af9b3334f33c7e0e53f3c3377d1330e6dde0a3a335573442d951c89aeb9a` | generated; PASS |
| manifest-hardened 256-bit audit A | `0012f9e23bb636596719e7e7e10193728950ef84bc4f26a8aa1972a49eee3922` | generated; PASS |
| manifest-hardened 256-bit audit B | `0012f9e23bb636596719e7e7e10193728950ef84bc4f26a8aa1972a49eee3922` | generated; byte-identical to A |
| compiled generation-job assembler | `839e714dff2a00f5740c587a97a9bdf983fecab47ea9bab3cffa045cf1e63ce4` | recorded |

The candidate is 50,609,235 bytes.  Structural inspection found exactly 24
unique regular members, no duplicate, absolute or traversal name, no symlink,
and stored executable mode `100755` for `reproduce.sh`.  ZIP CRC checking found
no damaged member.  Its internal manifest lists all 23 nonmanifest files.

### Manifest-hardened audit generation: job 32612504

The submitted wrapper was
`/home/gs852/polished_submission_check/dawn_generate_manifest_hardened_audits.slurm`,
SHA-256
`13df6f53f06d69c20d96d1e65e939c7923196ab8b3d93913d69d156cb399e9bc`.
It copied the frozen tree to a fresh directory under `/tmp`, authenticated the
source manifest, centre and inverse, force-built the production assembler, ran
the symbolic and smoke suites, and generated the three complete audits.  Its
preliminary commands were

```bash
sha256sum --strict -c source_manifest.txt
make -B work/interval_assemble
make symbolic
make smoke
make stats
```

For `label=192,256a,256b` and `bits=192,256,256`, respectively, the production
commands were

```bash
/usr/bin/time -v -o time_assemble_${label}.txt \
  work/interval_assemble data/center_L60_S40_R30.hex \
  intervals_${label}.bin ${bits}

/usr/bin/time -v -o time_certify_${label}.txt \
  python3 src/certify.py \
    --center data/center_L60_S40_R30.hex \
    --intervals intervals_${label}.bin \
    --work work_${label} \
    --output audit_${label}.json \
    --binary work/interval_assemble \
    --inverse data/approx_inverse.bin \
    --reuse-inverse --complete-audit \
    --source-manifest source_manifest.txt
```

The symbolic suite, compiled 192/256-bit smoke suite, smoke nesting and
determinism checks, exact-rational complete-tail oracle, malformed-input tests,
nested-`SHA256SUMS` adversarial test, and complete-audit smoke integration all
passed.  The smoke and production backends reported
`mpfr_buildopt_tls_p=1`.  The six counted C++/Python files contain 2,961
nonblank, noncomment lines against the enforced 3,000-line cap.

Slurm records job `32612504` and its batch step as `COMPLETED`, exit code
`0:0`, on `pvc-s-247.data.cluster`.  It ran from 02:25:22 to 03:04:42 BST on
2 August 2026, an allocation time of 39 min 20 s.  The wrapper's measured
generation interval was 37 min 15 s.  The allocation was one exclusive Dawn
node with 96 CPUs and 1,027,200 MB requested memory (Slurm display); the validator used 80
threads.  The node had two 48-core Intel Xeon Platinum 8468 sockets, 1.0 TiB
physical memory, and no swap.  The observed environment was GCC 11.5.0,
Python 3.9.25, NumPy 1.23.5, MPFR 4.2.1 and GMP 6.3.0.  The four GPUs required
by the partition were allocated but unused by the CPU/MPFR validator.

| Phase | Internal assembly time | `/usr/bin/time` wall time | User time | System time | Maximum RSS |
|---|---:|---:|---:|---:|---:|
| 192-bit assembly | 51.579772897 s | 53.50 s | 2,525.59 s | 17.78 s | 6,997,744 KiB |
| 192-bit certification | — | 11 min 18.58 s | 32,242.87 s | 1,179.20 s | 202,449,084 KiB |
| 256-bit A assembly | 52.232413182 s | 53.50 s | 2,596.13 s | 16.51 s | 6,994,104 KiB |
| 256-bit A certification | — | 11 min 27.77 s | 32,781.80 s | 860.59 s | 199,950,196 KiB |
| 256-bit B assembly | 53.012943380 s | 54.95 s | 2,581.10 s | 17.71 s | 6,995,656 KiB |
| 256-bit B certification | — | 11 min 43.80 s | 33,010.71 s | 821.15 s | 199,250,736 KiB |

The six timed phases totalled 37 min 12.10 s, 105,738.20 user seconds and
2,912.94 system seconds.  The largest individual `/usr/bin/time` RSS was
202,449,084 KiB (about 193.07 GiB); Slurm's batch-step maximum was
203,606,608 KiB (about 194.17 GiB).  Every timed phase reported zero swaps and
exit status zero.

The interval-payload identities were

| Precision | SHA-256 |
|---|---|
| 192-bit | `17980dacd8e237c230c979a2a033a5be6a4ea6e3d4a55bc90af5a8241467f3dc` |
| 256-bit A | `5ab2f7297b52811bd78187f074ff9c28950216fdfbbfeba3c5264459915b5439` |
| 256-bit B | `5ab2f7297b52811bd78187f074ff9c28950216fdfbbfeba3c5264459915b5439` |

Both 256-bit audits and both 256-bit interval payloads are byte-identical.

### Static candidate authentication and exact checking

From a fresh, mode-preserving extraction of the exact candidate ZIP, the
following commands passed:

```bash
sha256sum --strict -c SHA256SUMS
sha256sum --strict -c source_manifest.txt
python3 verify_certificate.py certificate.json
python3 tests/symbolic.py
```

The exact checker reconstructed all support sets and closing arithmetic and
printed

```text
Y_upper=159/1000000000000 Z_upper=621/1000
C2_upper=122 C3_upper=3/250
radii_polynomial_upper=-94679749999997/250000000000000000000
derivative_upper=-94688999999991/250000000000000
AUTHENTICATED AUDIT AGGREGATION AND FINAL INEQUALITIES VERIFIED: PROVED
SOURCE-TO-AUDIT REGENERATION NOT PERFORMED BY THIS COMMAND
```

Both closing inequalities are strictly negative over the rationals.  The last
line is essential: this static invocation authenticates and aggregates the
packaged interval leaves but does not regenerate them from source.

### Clean packaged reproduction: job 32612929 — PASS

The wrapper
`/home/gs852/polished_submission_check/dawn_reproduce_manifest_hardened_certificate.slurm`,
SHA-256
`b2c35ae65238622dd720458918b116b92fea24aceed15708dad0554352b852ba`,
pins the exact candidate hash, rejects unsafe ZIP structure, restores stored
regular-file modes, requires exactly 24 files and executable `reproduce.sh`,
and runs

```bash
sha256sum --strict -c SHA256SUMS
sha256sum --strict -c source_manifest.txt
python3 verify_certificate.py certificate.json
python3 tests/symbolic.py
/usr/bin/time -v -o time_reproduce.txt \
  env THREADS=80 ./reproduce.sh
```

The timed command
`env THREADS=80 ./reproduce.sh` took 37 min 55.31 s wall time, 106,592.63 user
seconds and 2,583.82 system seconds.  `/usr/bin/time` recorded a maximum RSS of
201,323,764 KiB, zero swaps and exit status zero; Slurm recorded a batch-step
maximum RSS of 202,884,832 K.

The archive hash remained
`95d06a084d737c478118b7b8254836e57df39373da850985360a1a1068d9cae2`.
All three regenerated audits matched their packaged counterparts
byte-for-byte; the two 256-bit outputs were byte-identical, their intervals
nested inside the widened 192-bit intervals, and the final exact checker
printed

```text
AUTHENTICATED AUDIT AGGREGATION AND FINAL INEQUALITIES VERIFIED: PROVED
SOURCE-TO-AUDIT REGENERATION NOT PERFORMED BY THIS COMMAND
FROZEN-PROOF-INPUT SOURCE-TO-AUDIT REPRODUCTION PASS  (THREADS=80, INVERSE_SELECTOR_DIAGNOSTIC=not-run)
CLEAN MANIFEST-HARDENED PACKAGED REPRODUCTION PASS
```

### Current gate ledger

| Check | Outcome |
|---|---|
| corrected root-relative exact-file allowlist | PASS |
| adversarial unlisted nested `SHA256SUMS` rejection | PASS |
| MPFR thread-safe TLS fail-closed check and audit attestation | PASS |
| source manifest, centre and inverse authentication | PASS |
| exact symbolic and compiled smoke suites | PASS |
| hardened 192/256/256 complete-audit generation | PASS; job `32612504` |
| hardened 256a versus 256b | PASS; byte-identical |
| candidate ZIP structure and internal manifests | PASS; SHA-256 `95d06a08...` |
| static exact authenticated-audit aggregation | PASS; both closing inequalities strictly negative |
| clean exact-ZIP `THREADS=80 ./reproduce.sh` | **PASS**; job `32612929`, `COMPLETED 0:0` |
| overall manifest-hardened release gate | **PASS** |

### Raw evidence and limitations

The completed generation evidence is retained under
`/home/gs852/polished_submission_check/dawn_hardened_generation_32612504/`.
It includes the actual scheduler batch script, Slurm stdout and stderr,
post-job `sacct` and `scontrol` records, `sstat`, all six `/usr/bin/time`
records, source and payload hashes, generated audits, and
`EVIDENCE_SHA256SUMS`, whose SHA-256 is
`7ee8a45928474e9e7c79043a5c552b0ae3ac2c54bb5a0fa7cfd4e6ddcf983937`.
The completed clean-reproduction evidence is retained under
`/home/gs852/polished_submission_check/dawn_hardened_reproduction_32612929/`,
with scheduler stdout and stderr at
`dawn-hardened-reproduce-32612929.out` and
`dawn-hardened-reproduce-32612929.err`.  The terminal timing record has SHA-256
`e3e2e502b5075b323a0658d36da1e0b1c0e1d5d8ff81b74413d9c923cb2b181f`;
the submitted wrapper has SHA-256
`b2c35ae65238622dd720458918b116b92fea24aceed15708dad0554352b852ba`.

**`DAWN_REPRODUCTION_EVIDENCE.zip` SHA-256: `2d705f31fa63503acba611b1f2c168db8cf7b570dc5b62ee7582f8151e45231d`**

The 192-bit serialisation deliberately widens positive upper bounds by a
factor of 1.01.  Its enclosure is rigorous, but 256-in-192 nesting is therefore
a coarse consistency check rather than a close precision-convergence test.
The 192-bit run, both 256-bit runs and the clean packaged run all use the same
formulae, index maps, production implementation, MPFR/GMP path and frozen
inverse.  They test source binding, deterministic repetition and precision
consistency; they are not a separately written full-scale validator and cannot
exclude a common-implementation error.

The workflow pins and records the principal compiler, MPFR, GMP, Python and
NumPy versions, force-builds the backend, authenticates every proof input, and
records the loaded modules and dynamic libraries.  It is not an adversarially
hermetic build: it still trusts the Dawn host, command resolution, compiler,
dynamic loader and installed runtimes.  The raw records make that trust base
auditable but do not remove it.

### Commands and preliminary checks

The clean copied tree was checked and built with

```bash
sha256sum --strict -c source_manifest.txt
make -B work/interval_assemble
make symbolic
make smoke
make stats
```

The build command emitted by `make` was

```bash
/usr/bin/g++ -O3 -std=c++17 -Wall -Wextra -Wpedantic -fopenmp \
  -frounding-math -fno-fast-math -ffp-contract=off \
  src/interval_assemble.cpp -o work/interval_assemble \
  -fopenmp -lmpfr -lgmp
```

For labels `192`, `256a`, and `256b`, with bit precisions 192, 256, and 256,
respectively, the production phases were exactly of the form

```bash
/usr/bin/time -v -o time_assemble_${label}.txt \
  work/interval_assemble data/center_L60_S40_R30.hex \
  intervals_${label}.bin ${bits}

/usr/bin/time -v -o time_certify_${label}.txt \
  python3 src/certify.py \
    --center data/center_L60_S40_R30.hex \
    --intervals intervals_${label}.bin \
    --work work_${label} \
    --output audit_${label}.json \
    --binary work/interval_assemble \
    --inverse data/approx_inverse.bin \
    --reuse-inverse --complete-audit \
    --source-manifest source_manifest.txt
```

The log records the following preliminary results:

- exact symbolic recurrence, clamped-inverse, and symmetry-normalisation
  suite: **PASS**;
- compiled MPFR smoke audit at 192 and 256 bits: **PASS**, including
  determinism and 256-in-192 nesting for the smoke instance;
- source-size check: 2,926 nonblank, noncomment lines out of the permitted
  3,000.

These preliminary checks do not replace the final exact checker over the
assembled repaired certificate.

### Machine and numerical environment

| Field | Observed value |
|---|---|
| Slurm job | `32541169` |
| node | `pvc-s-141.data.cluster` |
| allocation | `pvc9`; 1 node; 1 task; 96 CPUs; exclusive; `mem=0`; 4 GPUs required by the partition but unused by the validator |
| validator threads | 80 |
| CPU | Intel Xeon Platinum 8468 |
| topology | 2 sockets, 48 cores/socket, 1 thread/core, 96 CPUs online |
| NUMA | 2 nodes |
| cache | L1d 4.5 MiB; L1i 3 MiB; L2 192 MiB; L3 210 MiB |
| physical memory | 1.0 TiB; 996 GiB initially available |
| swap | none |
| architecture | x86_64, little endian |
| compiler | GCC 11.5.0 20240719 (Red Hat 11.5.0-14) |
| Python | 3.9.25 |
| NumPy | 1.23.5 |
| MPFR | 4.2.1, directed RNDD/RNDU rounding |
| GMP | 6.3.0 |

`OMP_NUM_THREADS=80` was used.  Dynamic OpenMP was disabled; the BLAS-family,
NumExpr, and similar thread counts were fixed to one.  The validator executed
CPU/MPFR code and no GPU kernel.

### Runtime and memory

| Phase | Internal assembly time | `/usr/bin/time` wall time | User time | System time | Maximum RSS |
|---|---:|---:|---:|---:|---:|
| 192-bit assembly | 50.093516468 s | 51.84 s | 2,538.93 s | 20.87 s | 6,988,700 KiB |
| 192-bit certification | — | 11 min 18.55 s | 32,426.53 s | 835.12 s | 200,865,956 KiB |
| 256-bit A assembly | 55.851404488 s | 57.16 s | 2,610.72 s | 18.31 s | 6,992,048 KiB |
| 256-bit A certification | — | 11 min 43.15 s | 33,408.60 s | 871.57 s | 199,641,108 KiB |
| 256-bit B assembly | 51.904007473 s | 54.11 s | 2,628.59 s | 15.77 s | 6,995,960 KiB |
| 256-bit B certification | — | 12 min 24.64 s | 33,343.73 s | 864.38 s | 200,765,580 KiB |

The script recorded start `2026-08-01T12:35:43Z` and finish
`2026-08-01T13:13:55Z`, a 38 min 12 s measured generation interval.  The
complete Slurm allocation lasted 40 min 11 s.  The six timed production
phases totalled 106,957.10 user seconds and 2,626.02 system seconds.  Slurm's
batch-step maximum RSS was 202,123,528 KiB (about 192.76 GiB); the largest
individual `/usr/bin/time` maximum was 200,865,956 KiB (about 191.56 GiB).
Every timed phase reported zero swaps and exit status zero.

The generated interval-payload hashes were

| Precision | SHA-256 |
|---|---|
| 192-bit | `17980dacd8e237c230c979a2a033a5be6a4ea6e3d4a55bc90af5a8241467f3dc` |
| 256-bit A | `5ab2f7297b52811bd78187f074ff9c28950216fdfbbfeba3c5264459915b5439` |
| 256-bit B | `5ab2f7297b52811bd78187f074ff9c28950216fdfbbfeba3c5264459915b5439` |

Thus both the complete 256-bit audits and their underlying interval payloads
were byte-identical across the two runs.

| Certificate member | SHA-256 |
|---|---|
| `SHA256SUMS` | `8f580f5075467404a35ff1071de31ccd5faa3b5c766a83e7743ba4ae1d9baf77` |
| `certificate.json` | `2592e8441b1b366308c892133892191562451e46df3432c0b3fefc00bb78e8b2` |
| `source_manifest.txt` | `36217e3c75ef53876ed81a424f78be3fb077d9bedff64427b163a4a421382743` |
| `reproduce.sh` | `ac5906822d0f8ad999b7891ff36ae319bcb0ff9c6c96c148534b0f3551133a5e` |

The wrapper authenticated that hash, rejected unsafe ZIP structure, extracted
exactly 24 regular members while restoring their stored modes, rejected
symlinks and unexpected member counts, and required `reproduce.sh` to be
executable.  In the fresh extraction it then ran

```bash
sha256sum --strict -c SHA256SUMS
sha256sum --strict -c source_manifest.txt
python3 verify_certificate.py certificate.json
python3 tests/symbolic.py
/usr/bin/time -v env THREADS=80 ./reproduce.sh
```

#### Allocation and environment

Slurm records job `32597494` and its batch step as `COMPLETED`, exit code
`0:0`, on `pvc-s-247.data.cluster`.  The allocation ran from
2026-08-01 23:51:57 BST to 2026-08-02 00:32:08 BST, an elapsed 40 min 11 s.
After extraction and preflight, the wrapper recorded start
`2026-08-01T22:53:58Z` and finish `2026-08-01T23:32:07Z`.

| Field | Observed value |
|---|---|
| Slurm job | `32597494` |
| node | `pvc-s-247.data.cluster` |
| allocation | `pvc9`; 1 node; 1 task; 96 CPUs; exclusive; `mem=0`; 4 GPUs required by the partition but unused by the validator |
| validator threads | 80 |
| CPU | Intel Xeon Platinum 8468 |
| topology | 2 sockets, 48 cores/socket, 1 thread/core, 96 CPUs online |
| NUMA | 2 nodes |
| cache | L1d 4.5 MiB; L1i 3 MiB; L2 192 MiB; L3 210 MiB |
| physical memory | 1.0 TiB; 996 GiB initially available |
| swap | none |
| architecture | x86_64, little endian |
| operating system | Rocky Linux 9.8; kernel 5.14.0-570.128.1.el9_6.x86_64 |
| compiler | GCC 11.5.0 20240719 (Red Hat 11.5.0-14) |
| Python | 3.9.25 |
| NumPy | 1.23.5 |
| MPFR | 4.2.1, directed RNDD/RNDU rounding in the validator |
| GMP | 6.3.0 |

OpenMP used 80 validator threads.  The reproduction script disabled dynamic
OpenMP and fixed BLAS-family, NumExpr, and similar thread counts to one.  It
reused the authenticated frozen inverse for every audit; optional inverse
regeneration was not run and was reported as
`INVERSE_SELECTOR_DIAGNOSTIC=not-run`.

#### Runtime, memory, and authenticated logs

| Measurement | Value |
|---|---:|
| complete `reproduce.sh` wall time | 38 min 04.28 s |
| Slurm allocation elapsed time | 40 min 11 s |
| user CPU time | 106,435.17 s |
| system CPU time | 2,663.58 s |
| average CPU utilisation | 4,776% |
| maximum RSS (`/usr/bin/time`) | 200,699,292 KiB (about 191.40 GiB) |
| maximum RSS (Slurm batch step) | 202,013,416 KiB (about 192.66 GiB) |
| swaps | 0 |
| exit status | 0 |

The retained wrapper output, error log, and `/usr/bin/time` record have the
following hashes.

| Record | SHA-256 |
|---|---|
| `dawn-package-reproduce-32597494.out` | `92bebbd0536e6e882726320ec74720225bff1931d5f7595398d9c52b13690a2f` |
| `dawn-package-reproduce-32597494.err` | `f941d2cf71bfda93348ab0bc1318d016608b2e692d49ade82b9fa7aa10e8c6b5` |
| `dawn_packaged_reproduction_32597494/time_reproduce.txt` | `90044e694d07357462784be0b9ad2fedfb2c087133b8c29a9ec8f2f0f28de589` |

#### Closing output and interpretation

The complete output contains the exact terminal messages

```text
three regenerated audits match the packaged payloads byte-for-byte
AUTHENTICATED AUDIT AGGREGATION AND FINAL INEQUALITIES VERIFIED: PROVED
SOURCE-TO-AUDIT REGENERATION NOT PERFORMED BY THIS COMMAND
FROZEN-PROOF-INPUT SOURCE-TO-AUDIT REPRODUCTION PASS  (THREADS=80, INVERSE_SELECTOR_DIAGNOSTIC=not-run)
CLEAN PACKAGED FROZEN-PROOF-INPUT REPRODUCTION PASS
```

The standalone checker correctly disclaimed source regeneration; that work was
performed by the surrounding authenticated `reproduce.sh` run.  Its freshly
generated 192-bit, 256-bit A, and 256-bit B audits matched their three packaged
counterparts byte-for-byte.  The two 256-bit payloads were therefore also
byte-identical to one another.  The final checker reauthenticated the immutable
reference snapshot and obtained the exact bounds

```text
Y_upper=159/1000000000000 Z_upper=621/1000
C2_upper=122 C3_upper=3/250
radii_polynomial_upper=-94679749999997/250000000000000000000
derivative_upper=-94688999999991/250000000000000
```

Both closing inequalities are strictly negative over the rationals.

The certificate's embedded `proof_report.md` and `versions.txt` record the
packaged reproduction as pending at the moment the archive was frozen.  That
chronology is retained deliberately: changing either embedded file after this
run would create a different, unreproduced archive.  The present outer report
records the subsequent successful reproduction of those exact frozen bytes.

This run reproduces the same implementation with the same formulae, compiler
path, MPFR/GMP libraries, and frozen inverse.  The separate 192-bit run,
byte-identical repeated 256-bit runs, interval nesting, exact checker, and clean
packaged reproduction are strong consistency, determinism, and archival
evidence; they are not an independent second implementation and do not exclude
a common-implementation error.

### Machine

| Field | Observed value |
|---|---|
| node | `pvc-s-250.data.cluster` |
| CPU | Intel Xeon Platinum 8468 |
| topology | 2 sockets, 48 cores/socket, 1 thread/core, 96 CPUs online |
| NUMA | 2 nodes |
| cache | L1d 4.5 MiB; L1i 3 MiB; L2 192 MiB; L3 210 MiB |
| physical memory | 1.0 TiB |
| swap | none |
| architecture | x86_64, little endian |

The initial allocation snapshot reported 996 GiB available memory.

### Numerical environment

`reproduce.sh` fail-closed checks and accepted:

| Component | Required and observed |
|---|---|
| MPFR | 4.2.1, explicit RNDD/RNDU directed rounding |
| GMP | 6.3.0 |
| NumPy | 1.23.5 |
| GCC | 11.5.0 20240719 (Red Hat 11.5.0-14), recorded in rebuilt audit provenance |
| OpenMP | 80 validator threads; deterministic reduction contract |
| BLAS-family thread counts | fixed to 1 by the script |

Loaded module records included `rhel9/default-dawn` and
`mpfr/4.2.1/gcc/jrfmlssl`, the latter loading GMP 6.3.0.  The rebuilt audits
record GCC 11.5.0.  The wrapper did not print `python3 --version`; the
packaged provenance records Python 3.9.25, but that patch version was not
independently printed by this rerun and is not silently promoted to a fresh
observation.  The script did enforce NumPy 1.23.5, rebuild the C++ program
from authenticated source, and reproduce every audit byte-for-byte.

### Runtime and memory

| Measurement | Value |
|---|---:|
| start UTC | 2026-08-01 00:07:51 |
| finish UTC | 2026-08-01 00:45:28 |
| elapsed wall time (`/usr/bin/time`) | 37 min 37.10 s |
| Slurm allocation elapsed time | 39 min 24 s |
| user CPU time | 106,481.64 s |
| system CPU time | 2,692.64 s |
| average CPU utilisation | 4,836% |
| maximum RSS (`/usr/bin/time`) | 201,710,892 KiB (about 192.4 GiB) |
| maximum RSS observed live by Slurm `sstat` | 202,961,412 KiB (about 193.6 GiB) |
| swaps | 0 |
| exit status | 0 |

The small difference between the two RSS figures reflects measurement scope;
both are reported rather than silently selecting one.

The three full interval-assembly phases reported:

| Audit | Precision | Assembly time |
|---|---:|---:|
| 192 | 192 bits | 54.527791514 s |
| 256a | 256 bits | 54.631465298 s |
| 256b | 256 bits | 50.808850295 s |
