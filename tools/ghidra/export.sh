#!/usr/bin/env bash
# Import + analyze a target .so with Ghidra headless and export the function inventory.
# Usage: tools/ghidra/export.sh <bin-stem>   e.g. libtiver  ->  build/ghidra/<stem>.funcs.json
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
STEM="$1"
GHIDRA="${GHIDRA_HOME:-/opt/ghidra}"
mkdir -p "$ROOT/build/ghidra/proj"
"$GHIDRA/support/analyzeHeadless" "$ROOT/build/ghidra/proj" "$STEM" \
    -import "$ROOT/target/$STEM.so" -overwrite -loader ElfLoader -loader-imagebase 0 \
    -scriptPath "$ROOT/tools/ghidra" -postScript ExportFunctions.java "$ROOT/build/ghidra/$STEM.funcs.json" \
    2>&1 | grep -E 'ERROR|exported|Exception' || true
test -s "$ROOT/build/ghidra/$STEM.funcs.json"
