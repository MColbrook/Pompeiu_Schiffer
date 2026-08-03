# Reproduction report

**Audit date:** 2 August 2026  
**Manifest-hardened source-to-audit generation:** **PASS**  
**Static authentication and exact checking of the assembled candidate ZIP:** **PASS**  
**Clean packaged source-to-certificate reproduction:** **PASS** — Slurm job `32612929` completed `0:0`  
**Overall manifest-hardened release gate:** **PASS**

> **Fail-closed current status.**  Slurm job `32612504` regenerated one
> 192-bit and two 256-bit complete audits from the frozen manifest-hardened
> source and frozen inverse.  The resulting 24-file candidate certificate has
> SHA-256
> `95d06a084d737c478118b7b8254836e57df39373da850985360a1a1068d9cae2`.
> Its ZIP structure, both manifests, exact authenticated-audit checker,
> symbolic suite, compiled smoke suite, nested-`SHA256SUMS` adversarial
> regression, and exact closing inequalities have passed.  A clean extraction
> of those exact ZIP bytes ran `THREADS=80 ./reproduce.sh` as job `32612929`
> on `pvc-s-36`.  The job completed with exit code `0:0`; all three regenerated
> audits matched their packaged counterparts byte-for-byte, the exact checker
> printed its scoped `PROVED` conclusion, and the reproduction script printed
> both its frozen-proof-input and clean packaged-reproduction PASS markers.
> This closes the mandated clean-extraction gate for the exact candidate ZIP.
> It is a same-implementation reproduction, not an independent implementation.

## Current manifest-hardened release

### Defect repaired and scope of the repair

An independent review found that the first repaired checker excluded every
regular file whose basename was `SHA256SUMS` when constructing the actual file
set.  Consequently, an unlisted file such as `junk/SHA256SUMS` could evade the
claimed exact-file allowlist.  The authentic first-repair archive contained
exactly its expected 24 members and no such file, so this validation-contract
defect did not alter a proof source, numerical leaf, support term, exact
inequality, or the theorem arithmetic.  It nevertheless prevented that archive
from being described as fully fail-closed.

The checker now excludes only the root-relative path `SHA256SUMS`, and the
compiled smoke suite requires rejection of an unlisted nested file with that
basename.  The MPFR backend also now terminates unless
`mpfr_buildopt_tls_p()` attests a thread-safe TLS build.  Every complete audit
records `provenance.mpfr_buildopt_tls_p=true`, and the exact checker requires
that field.  No numerical constant, exact centre byte, frozen-inverse byte,
support cutoff, norm weight, or radii-polynomial formula was changed.

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

Both 256-bit audits and both 256-bit interval payloads are byte-identical.  All
three interval-payload hashes are also identical to those generated by the
first-repair job `32541169`.

#### Exact structural comparison with the first repaired audits

A recursive parsed-JSON comparison against the audits in the superseded
`4bf19006...` certificate found exactly three changes in each file:

1. `provenance.mpfr_buildopt_tls_p=true` was added;
2. `source_sha256` changed from `36217e3c...` to `2057deab...`; and
3. `provenance.source_manifest_sha256` changed identically.

No other JSON value differs.  In particular, every numerical interval,
support key, witness, complete column, aggregate, and theorem-relevant bound is
unchanged.  This parsed comparison, together with the identical raw interval
payload hashes, distinguishes the validation-contract/TLS hardening from a new
choice of numerical proof data.

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

It was submitted as Slurm job `32612929`.  At the pre-completion report freeze,
the job was `RUNNING` on `pvc-s-36`, having started at 03:12:45 BST on
2 August 2026.  At that snapshot the initial hash, safe extraction, both
manifests, static checker, symbolic test, force-build, smoke suite and
source-size check had passed, while the first full 192-bit certification was
still in progress.  Those partial observations were correctly not counted as
a PASS at the time.

The terminal records now show `COMPLETED` with exit code `0:0`.  Slurm records
a start at 02:12:45 UTC and end at 02:50:55 UTC on 2 August 2026, for an
allocation elapsed time of 38 min 10 s.  The timed command
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

The checker's second line still correctly scopes that checker invocation; the
following reproduction-script line records the separate source-to-audit work.
This terminal result closes the mandated clean-extraction gate for the exact
candidate archive.  Because the regeneration uses the same source, formulae,
MPFR/GMP implementation and frozen inverse, it is reproducibility evidence and
not an implementation-diverse validation.

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

## Superseded first-repair history: certificate 4bf19006...

The remainder of this section records the first repaired certificate and its
jobs for provenance only.  That certificate fixed the dimension-alias and
precision-parser defects and its authentic numerical payload was not
invalidated.  It was nevertheless superseded because its exact-file checker
did not reject an unlisted nested file named `SHA256SUMS`.  Historical PASS
statements below apply only to the gates then exercised and are not the current
release verdict.

### First repaired audit generation: job 32541169

The recorded Slurm wrapper was
`/home/gs852/polished_submission_check/dawn_generate_repaired_audits.slurm`.
It copied the repaired working tree to a fresh directory under `/tmp`, checked
the frozen source manifest and proof inputs, force-rebuilt the C++ assembler,
ran the symbolic and smoke suites, and generated one 192-bit and two 256-bit
complete audits.  The job ended with

```text
generated audit metadata and 256-bit byte identity PASS
REPAIRED AUDIT GENERATION PASS
```

Slurm records job `32541169` and its batch step as `COMPLETED`, exit code
`0:0`.  The distinction between audit generation here and the later clean
packaged reproduction remains deliberate.

### Frozen first-repair inputs and generated outputs

| Object | SHA-256 | Result |
|---|---|---|
| repaired source manifest | `36217e3c75ef53876ed81a424f78be3fb077d9bedff64427b163a4a421382743` | PASS; `sha256sum --strict -c source_manifest.txt` |
| exact hexadecimal centre | `a30353f226a96c88a3d7e8b54b9d29d000c884fadb6cc52b47f176a874d73d29` | PASS |
| frozen `approx_inverse.bin` | `aa2de444bcae9e0c9bdf8afc8f5290ef5c363d84caf1c1de2df31fe569f9ea1c` | PASS; reused without regeneration |
| freshly compiled interval assembler | `c32c2e0816716b8c98822df202a2637ad1854147008d97cd17bc98dcb49dccfc` | recorded |
| repaired 192-bit audit | `2e0d76a497a58e37c69027371498a8f3b5c539012f39bf7267553d4a0ed38020` | PASS; generated |
| repaired 256-bit audit A | `981381e11ed962774b0e19f0755241e7ad0a4167cc745893b8f9b00b086dc255` | PASS; generated |
| repaired 256-bit audit B | `981381e11ed962774b0e19f0755241e7ad0a4167cc745893b8f9b00b086dc255` | PASS; generated and byte-identical to A |

The centre is 55,547 bytes.  The shipped inverse is 48,846,752 bytes and was
passed to every certification phase with `--reuse-inverse`.  Its hash was
checked before and after every audit.

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

### Comparison with the immutable predecessor audits

A recursive parsed-JSON comparison found exactly two differing leaves in
each repaired audit: `source_sha256` and
`provenance.source_manifest_sha256`.  Both changed from the predecessor
manifest hash
`4a8d9e6fddc9bafb1c7e1e6ca62acf11f7f529ea466d5ae646c811bd3a8ac9f6`
to the repaired manifest hash
`36217e3c75ef53876ed81a424f78be3fb077d9bedff64427b163a4a421382743`.
All other JSON values were identical.  Replacing only those two old hash
strings in each predecessor file made it byte-identical to its generated
repaired counterpart (`cmp` exit status zero for 192, 256a, and 256b).

This comparison establishes that the validated numerical payload did not
change during the defensive source repair.  Certificate authentication and
clean packaged reproduction were discharged separately as recorded below.

### First-repair gate ledger

| Check | Outcome |
|---|---|
| repaired source-manifest authentication | PASS |
| frozen centre and inverse authentication | PASS |
| clean force-build from repaired source | PASS |
| exact symbolic tests | PASS |
| compiled MPFR smoke tests | PASS |
| repaired 192-bit complete-audit generation | PASS |
| repaired 256-bit complete-audit generation A | PASS |
| repaired 256-bit complete-audit generation B | PASS |
| repaired 256a versus 256b | PASS; byte-identical |
| repaired audits versus predecessor numerical payloads | PASS; only the two declared source-manifest hash leaves changed |
| assemble and authenticate the repaired certificate archive | PASS; SHA-256 `4bf190061031713e4dbf529e1d8d510689ee9261c4a2d56120c1449e48bba20b` |
| exact checker over the assembled repaired certificate | PASS; both closing inequalities strictly negative over the rationals |
| clean full-node `THREADS=80 ./reproduce.sh` from the packaged repaired archive | PASS; job `32597494`, all three audits byte-identical to the packaged payloads |

Accordingly, every first-repair gate then represented in this ledger was
**PASS**.  That ledger did not exercise the later-discovered
nested-`SHA256SUMS` adversarial case and is not the current release verdict.

### Superseded first-repair packaged-extraction wrapper attempt

Slurm job `32563664` ran on `pvc-s-256` against the exact repaired candidate
archive with SHA-256
`4bf190061031713e4dbf529e1d8d510689ee9261c4a2d56120c1449e48bba20b`.
It ended `FAILED`, exit code `1:0`, after 19 seconds.  The wrapper authenticated
the archive and safely rejected duplicate, absolute, traversal and non-regular
ZIP members, but its initial Python copy extractor did not restore the stored
Unix permission bits under `umask 077`.  The extracted `reproduce.sh` was
therefore not executable.  After both internal checksum
manifests had passed, the exact checker stopped with

```text
CERTIFICATE REJECTED: reproduce.sh is not executable
```

No `time_reproduce.txt` was produced, no production interval assembly or
certification began, and the attempt supplied no numerical evidence.  The
candidate ZIP itself already stored `reproduce.sh` as regular mode `100755`;
this was solely an external extraction-wrapper defect.  The corrected wrapper
copies each prevalidated regular member with exclusive creation and restores
`stat.S_IMODE(item.external_attr >> 16)`.  It was locally checked to preserve
mode `0755` and to pass the unchanged archive's exact checker.  The certificate
archive was not edited or recompressed in response.

### Superseded first-repair clean packaged reproduction: job 32597494

The corrected Slurm wrapper
`/home/gs852/polished_submission_check/dawn_reproduce_packaged_certificate.slurm`
has SHA-256
`78a8727dd6b1dc5f1420f8221883f9b4e18c9d9080bb35cabec4577b0798cc2e`.
It ran against the unchanged repaired certificate archive
`pompeiu_validation_certificate.repaired_candidate.zip`, SHA-256
`4bf190061031713e4dbf529e1d8d510689ee9261c4a2d56120c1449e48bba20b`.
Key identities inside those frozen ZIP bytes are

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

The archive hash was checked again after the complete run.  The job neither
edited nor recompressed the archive, and it did not regenerate or replace the
packaged inverse or any other proof input.

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

The standalone checker correctly disclaims source regeneration; that work was
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

## Immutable predecessor history

The immutable predecessor certificate passed its static authentication and
exact checks, and historical job `32509644` regenerated its three production
audits byte-for-byte.  This evidence is retained for provenance but is not
silently inherited by the current manifest-hardened certificate.

### Immutable predecessor inputs

| Object | SHA-256 | Result |
|---|---|---|
| exact original archive, `pompeiu_regular_validation.zip` | `1f1153b6b00b239146f9ce45ce7d092acdb4abb20399da1a63ef445293125845` | PASS; ZIP integrity passed; file left unchanged |
| immutable predecessor `pompeiu_validation_certificate.zip` | `0a97036e2fdbb296b222c5d80ce4e472f77968b84e1c9c9ef2cbf24e2b0ac474` | PASS |
| exact hexadecimal centre | `a30353f226a96c88a3d7e8b54b9d29d000c884fadb6cc52b47f176a874d73d29` | PASS |
| predecessor source manifest | `4a8d9e6fddc9bafb1c7e1e6ca62acf11f7f529ea466d5ae646c811bd3a8ac9f6` | PASS |
| `certificate.json` | `f2b2d9d05bbc023a00cf7f60da5b2814bf93d7d39e39d3bb5e1b92b3d3c04429` | PASS |

The outer archive's top-level checksum manifest is stale.  That known
packaging defect is documented in the supplied audit; it was neither ignored
nor repaired.  Authority was assigned to the separately supplied inner
certificate.  The original 377,291,576-byte archive is not included in the
polished bundle and must be transferred separately by the hash above.

### Predecessor clean static check

The certificate was extracted into
`/home/gs852/pompeiu/certificate_authenticated`.  The commands run there were

```bash
sha256sum -c SHA256SUMS
python3 verify_certificate.py certificate.json
python3 tests/symbolic.py
```

Results:

- all 22 files in the certificate manifest: **PASS**;
- exact checker: **PASS**, terminating with `CERTIFICATE VERIFIED: PROVED`;
- symbolic recurrence, clamped-inverse, trace, and symmetry test: **PASS**.

The exact checker printed

```text
Y_upper=159/1000000000000 Z_upper=621/1000
C2_upper=122 C3_upper=3/250
radii_polynomial_upper=-94679749999997/250000000000000000000
derivative_upper=-94688999999991/250000000000000
```

Both closing inequalities are strictly negative over the rationals.

### Predecessor complete reproduction

The audited command inside the allocation was exactly

```bash
/usr/bin/time -v env THREADS=80 ./reproduce.sh
```

It was launched by
`/home/gs852/pompeiu/reproduce_full_node.slurm` as Slurm job **32509644**.
The batch shape was:

```text
partition=pvc9
account=AIRR-P67-DAWN-GPU
qos=gpu1
nodes=1
tasks=1
cpus-per-task=96
exclusive allocation
mem=0 (all node memory)
THREADS=80
time limit=02:00:00
```

Four Intel GPUs were allocated because this Dawn partition requires them;
the validator is CPU/MPFR code and ran no GPU kernel.

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

The wrapper did not time each subsequent Python complete-certification phase
separately.  The overall exact wall time above is the fresh-run measurement.

### Predecessor rebuilt audit comparison

The fresh output said exactly:

```text
three regenerated audits match the packaged payloads byte-for-byte
CERTIFICATE VERIFIED: PROVED
REPRODUCTION PASS (THREADS=80)
```

Because each generated file equals its packaged counterpart, their hashes are

| Rebuilt audit | SHA-256 | Comparison |
|---|---|---|
| 192-bit | `06ce854e4ff187a9293244d52e4470dfee463ce6a9250596ff438f08e9493ee0` | byte-identical to packaged 192-bit audit |
| 256-bit A | `17aa7fa0b48b14426c7d9e68e48e31853ef85fc54c5358d57d660c149ba912dd` | byte-identical to packaged 256a |
| 256-bit B | `17aa7fa0b48b14426c7d9e68e48e31853ef85fc54c5358d57d660c149ba912dd` | byte-identical to packaged 256b and rebuilt 256a |

The exact checker also validated interval nesting of both 256-bit results in
the separately generated 192-bit enclosures, authenticated support counts and
witnesses, all exact constants, the two radii inequalities, univalence, and
the nonzero first shape coefficient.

### Predecessor pass/fail ledger

| Check | Outcome |
|---|---|
| original archive SHA-256 and ZIP integrity | PASS |
| inner certificate SHA-256 | PASS |
| inner `SHA256SUMS` | PASS |
| frozen source manifest | PASS |
| exact checker | PASS |
| exact symbolic tests | PASS |
| compiled MPFR smoke test | PASS; deterministic and 256-in-192 nesting true |
| MPFR 192-bit source regeneration | PASS |
| MPFR 256-bit source regeneration A | PASS |
| MPFR 256-bit source regeneration B | PASS |
| all three regenerated versus packaged | PASS, byte-for-byte |
| regenerated 256a versus 256b | PASS, byte-identical |
| 256-bit interval nesting in 192-bit intervals | PASS |
| final exact certificate verification | PASS |

No predecessor reproduction discrepancy remains.  This was a reproduction of
the supplied predecessor implementation, not an independent second
implementation; that distinction is preserved deliberately.  The first
repaired certificate's corresponding historical gate was closed by job
`32597494`; the current manifest-hardened certificate's separate gate was closed by job
`32612929` as recorded above.
