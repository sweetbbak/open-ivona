#!/usr/bin/env bash
# Differential test of the decompiled voice library: every utterance of the corpus is spoken
# with the original libvoice_en_gb_amy.so and with the decompiled one (same engine), and the
# WAVs must be byte-identical. Synthesis is deterministic, so any difference is a bug.
#   tools/difftest.sh [CORPUS] [--voice-lib PATH]   (default tests/run/corpus.txt, decomp)
# Output: one PASS/DIFF/FAIL line per utterance, WAVs in build/difftest/; exit 1 unless all pass.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
CORPUS=tests/run/corpus.txt
CANDIDATE=decomp
while [ $# -gt 0 ]; do
    case "$1" in
        --voice-lib) CANDIDATE="$2"; shift 2 ;;
        *) CORPUS="$1"; shift ;;
    esac
done
OUT=build/difftest
rm -rf "$OUT"; mkdir -p "$OUT"
pass=0; total=0
while IFS= read -r line; do
    case "$line" in ''|'#'*) continue ;; esac
    total=$((total + 1))
    n=$(printf '%02d' $total)
    ./speak --voice-lib original "$line" "$OUT/$n-orig.wav" > "$OUT/$n-orig.log" 2>&1 </dev/null
    ro=$?
    ./speak --voice-lib "$CANDIDATE" "$line" "$OUT/$n-cand.wav" > "$OUT/$n-cand.log" 2>&1 </dev/null
    rc=$?
    short=$(printf '%.60s' "$line")
    if [ $ro -ne 0 ]; then
        echo "FAIL $n original failed (see $OUT/$n-orig.log): $short"
    elif [ $rc -ne 0 ]; then
        echo "FAIL $n candidate failed (see $OUT/$n-cand.log): $short"
    elif cmp -s "$OUT/$n-orig.wav" "$OUT/$n-cand.wav"; then
        echo "PASS $n $short"; pass=$((pass + 1))
    else
        echo "DIFF $n ($(stat -c%s "$OUT/$n-orig.wav") vs $(stat -c%s "$OUT/$n-cand.wav") bytes): $short"
    fi
done < "$CORPUS"
echo "$pass/$total byte-identical"
[ $pass -eq $total ]
