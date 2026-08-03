#!/usr/bin/env bash
set -euo pipefail
umask 077
ulimit -c 0

ARCHIVE_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
THREADS=${THREADS:-80}
DIAGNOSE_INVERSE_REGENERATION=${DIAGNOSE_INVERSE_REGENERATION:-0}

case "$THREADS" in
    ''|*[!0-9]*) echo "THREADS must be an integer in [1,96]" >&2; exit 2 ;;
esac
if [ "$THREADS" -lt 1 ] || [ "$THREADS" -gt 96 ]; then
    echo "THREADS must be an integer in [1,96]" >&2
    exit 2
fi
case "$DIAGNOSE_INVERSE_REGENERATION" in
    0|1) ;;
    *) echo "DIAGNOSE_INVERSE_REGENERATION must be 0 or 1" >&2; exit 2 ;;
esac

export OMP_NUM_THREADS=$THREADS
export OMP_DYNAMIC=FALSE
export OMP_PROC_BIND=FALSE
export OMP_PLACES=cores
export OMP_MAX_ACTIVE_LEVELS=1
export MKL_NUM_THREADS=1
export MKL_DYNAMIC=FALSE
export OPENBLAS_NUM_THREADS=1
export BLIS_NUM_THREADS=1
export NUMEXPR_NUM_THREADS=1
export CXX=/usr/bin/g++

# A fresh DAWN shell does not expose MPFR through pkg-config.  Load the exact
# production module only when no MPFR package is currently visible; a visible
# but different version is rejected below rather than silently replaced.
if command -v pkg-config >/dev/null 2>&1 && ! pkg-config --exists mpfr; then
    if ! type module >/dev/null 2>&1; then
        if [ -r /etc/profile.d/modules.sh ]; then
            . /etc/profile.d/modules.sh
        elif [ -r /usr/share/Modules/init/bash ]; then
            . /usr/share/Modules/init/bash
        else
            echo "MPFR is unavailable and the DAWN module command cannot be initialized" >&2
            exit 2
        fi
    fi
    type module >/dev/null 2>&1 || {
        echo "MPFR is unavailable and the DAWN module command is not functional" >&2
        exit 2
    }
    module load rhel9/default-dawn || {
        echo "failed to initialize the required rhel9/default-dawn environment" >&2
        exit 2
    }
    module load mpfr/4.2.1/gcc/jrfmlssl || {
        echo "failed to load required module mpfr/4.2.1/gcc/jrfmlssl" >&2
        exit 2
    }
fi

for command in /usr/bin/g++ bash cmp cp find make mkdir mktemp pkg-config python3 rm sha256sum; do
    command -v "$command" >/dev/null 2>&1 || {
        echo "required command is unavailable: $command" >&2
        exit 2
    }
done
detected_mpfr=$(pkg-config --modversion mpfr 2>/dev/null) || {
    echo "pkg-config cannot find required MPFR 4.2.1" >&2
    exit 2
}
detected_gmp=$(pkg-config --modversion gmp 2>/dev/null) || {
    echo "pkg-config cannot find required GMP 6.3.0" >&2
    exit 2
}
[ "$detected_mpfr" = 4.2.1 ] || {
    echo "MPFR version mismatch: require 4.2.1, found $detected_mpfr" >&2
    exit 2
}
[ "$detected_gmp" = 6.3.0 ] || {
    echo "GMP version mismatch: require 6.3.0, found $detected_gmp" >&2
    exit 2
}
detected_numpy=$(python3 -c 'import numpy; print(numpy.__version__)' 2>/dev/null) || {
    echo "Python cannot import required NumPy 1.23.5" >&2
    exit 2
}
[ "$detected_numpy" = 1.23.5 ] || {
    echo "NumPy version mismatch: require 1.23.5, found $detected_numpy" >&2
    exit 2
}

cd "$ARCHIVE_ROOT"
test -s SHA256SUMS
test -s source_manifest.txt
test -s certificate.json
test -x verify_certificate.py
sha256sum --strict -c SHA256SUMS
sha256sum --strict -c source_manifest.txt

RUN_ROOT=$(mktemp -d /tmp/pompeiu-regular-reproduce.XXXXXX)
case "$RUN_ROOT" in
    /tmp/pompeiu-regular-reproduce.*) ;;
    *) echo "unsafe temporary path: $RUN_ROOT" >&2; exit 2 ;;
esac
cleanup() {
    case "$RUN_ROOT" in
        /tmp/pompeiu-regular-reproduce.*) rm -rf -- "$RUN_ROOT" ;;
    esac
}
trap cleanup EXIT HUP INT TERM

REFERENCE=$RUN_ROOT/reference
TREE=$RUN_ROOT/tree
GENERATED=$RUN_ROOT/generated
mkdir -p "$REFERENCE" "$TREE" "$GENERATED"
cp -a -- "$ARCHIVE_ROOT/." "$REFERENCE/"

cd "$REFERENCE"
sha256sum --strict -c SHA256SUMS
sha256sum --strict -c source_manifest.txt
python3 verify_certificate.py certificate.json

cp -a -- "$REFERENCE/." "$TREE/"
case "$TREE" in
    "$RUN_ROOT"/tree) ;;
    *) echo "unsafe build-tree path: $TREE" >&2; exit 2 ;;
esac
rm -rf -- "$TREE/work" "$TREE/results"
find "$TREE" -type d -name __pycache__ -prune -exec rm -rf -- {} +
find "$TREE" -type f \( -name '*.pyc' -o -name '*.pyo' \) -delete

cd "$TREE"
sha256sum --strict -c source_manifest.txt
make -B work/interval_assemble
make symbolic
make smoke
make stats

BIN=$TREE/work/interval_assemble
[ -f "$BIN" ] && [ ! -L "$BIN" ] && [ -x "$BIN" ] || {
    echo "fresh interval assembler was not produced" >&2; exit 2;
}
CENTER=$TREE/data/center_L60_S40_R30.hex
MANIFEST=$TREE/source_manifest.txt
FROZEN_INVERSE=$TREE/data/approx_inverse.bin
INVERSE=$GENERATED/approx_inverse.bin
cp -- "$FROZEN_INVERSE" "$INVERSE"

run_audit() {
    label=$1
    bits=$2
    run_work=$GENERATED/work_$label
    intervals=$GENERATED/intervals_$label.bin
    audit=$GENERATED/audit_$label.json
    mkdir -p "$run_work"
    "$BIN" "$CENTER" "$intervals" "$bits"
    python3 "$TREE/src/certify.py" \
        --center "$CENTER" \
        --intervals "$intervals" \
        --work "$run_work" \
        --output "$audit" \
        --binary "$BIN" \
        --inverse "$INVERSE" \
        --reuse-inverse \
        --complete-audit \
        --source-manifest "$MANIFEST"
}

# The authenticated inverse is an exact proof input for all three audits.
inverse_selector_diagnostic=not-run
run_audit 192 192
if [ "$DIAGNOSE_INVERSE_REGENERATION" = 1 ]; then
    regenerated=$GENERATED/regenerated_inverse.bin
    if python3 "$TREE/src/certify.py" \
        --center "$CENTER" \
        --intervals "$GENERATED/intervals_192.bin" \
        --work "$GENERATED/regenerated_inverse_work" \
        --output "$GENERATED/regenerated_inverse_incomplete.json" \
        --binary "$BIN" \
        --inverse "$regenerated"; then
        echo "inverse-regeneration diagnostic unexpectedly returned complete" >&2
        exit 2
    else
        status=$?
    fi
    [ "$status" -eq 3 ] || {
        echo "inverse-regeneration diagnostic failed with status $status" >&2
        exit 2
    }
    echo "authenticated frozen inverse:"
    sha256sum -- "$FROZEN_INVERSE"
    echo "non-authoritative regenerated inverse:"
    sha256sum -- "$regenerated"
    if cmp -- "$FROZEN_INVERSE" "$regenerated"; then
        inverse_selector_diagnostic=identical
        echo "non-authoritative inverse-selector diagnostic: byte-identical"
    else
        echo "non-authoritative inverse-selector diagnostic: bytes differ"
        inverse_selector_diagnostic=different
        echo "the authenticated frozen proof input remains unchanged"
    fi
fi
run_audit 256a 256
run_audit 256b 256
cmp -- "$GENERATED/audit_256a.json" "$GENERATED/audit_256b.json"

python3 - "$REFERENCE" "$GENERATED" "$THREADS" <<'PY'
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1]).resolve()
generated = pathlib.Path(sys.argv[2]).resolve()
requested_threads = int(sys.argv[3])
certificate = json.loads((root / "certificate.json").read_text())
records = []
for key, local in (("mpfr_192", "audit_192.json"),
                   ("mpfr_256a", "audit_256a.json"),
                   ("mpfr_256b", "audit_256b.json")):
    relative = pathlib.PurePosixPath(certificate["runs"][key]["path"])
    if relative.is_absolute() or ".." in relative.parts or relative.as_posix() == ".":
        raise SystemExit(f"unsafe packaged audit path for {key}: {relative}")
    packaged = root.joinpath(*relative.parts).resolve()
    if not packaged.is_relative_to(root) or not packaged.is_file():
        raise SystemExit(f"missing packaged audit for {key}: {relative}")
    rebuilt = generated / local
    packaged_raw, rebuilt_raw = packaged.read_bytes(), rebuilt.read_bytes()
    packaged_json, rebuilt_json = json.loads(packaged_raw), json.loads(rebuilt_raw)
    try:
        packaged_threads = packaged_json["provenance"]["threads"]
        rebuilt_threads = rebuilt_json["provenance"]["threads"]
    except (KeyError, TypeError) as exc:
        raise SystemExit(f"missing thread provenance in {key}: {exc}") from exc
    if type(packaged_threads) is not int:
        raise SystemExit(f"packaged thread provenance is not an integer in {key}")
    if type(rebuilt_threads) is not int or rebuilt_threads != requested_threads:
        raise SystemExit(
            f"rebuilt thread provenance for {key} is {rebuilt_threads!r}, "
            f"requested {requested_threads}"
        )
    records.append((key, packaged_raw, rebuilt_raw, packaged_json, rebuilt_json,
                    packaged_threads))

packaged_thread_counts = {record[5] for record in records}
if len(packaged_thread_counts) != 1:
    raise SystemExit("packaged audits disagree on provenance.threads")
packaged_threads = packaged_thread_counts.pop()
if requested_threads == packaged_threads:
    for key, packaged_raw, rebuilt_raw, _, _, _ in records:
        if rebuilt_raw != packaged_raw:
            raise SystemExit(f"rebuilt audit differs bytewise from packaged {key}")
    print("three regenerated audits match the packaged payloads byte-for-byte")
else:
    for key, _, _, packaged_json, rebuilt_json, _ in records:
        del packaged_json["provenance"]["threads"]
        del rebuilt_json["provenance"]["threads"]
        if rebuilt_json != packaged_json:
            raise SystemExit(
                f"rebuilt audit differs from packaged {key} beyond provenance.threads"
            )
    print("three regenerated audits match after removing only provenance.threads "
          f"(packaged={packaged_threads}, requested={requested_threads})")
PY

# Recheck the same immutable reference snapshot used for every comparison.
cd "$REFERENCE"
python3 verify_certificate.py certificate.json
echo "FROZEN-PROOF-INPUT SOURCE-TO-AUDIT REPRODUCTION PASS "\
     "(THREADS=$THREADS, INVERSE_SELECTOR_DIAGNOSTIC=$inverse_selector_diagnostic)"
