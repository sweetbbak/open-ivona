#!/usr/bin/env bash
# Ghidra's decompilation of functions, with the tables' names and resolved strings, on stdout.
#   tools/ghidra/decompile.sh BINARY ADDR...     e.g. libvoice_en_gb_amy 4b790 366c4
# Needs the project from tools/ghidra/export.sh (build/ghidra/proj); it is opened read-only.
# A hint for writing C, like the retdec output: never a source of truth.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$1"; shift
GHIDRA="${GHIDRA_HOME:-/opt/ghidra}"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
# a copy of the project, so a running Ghidra (or another decompile) does not lock it
cp -r "$ROOT/build/ghidra/proj/$BIN.gpr" "$ROOT/build/ghidra/proj/$BIN.rep" "$tmp/"
"$GHIDRA/support/analyzeHeadless" "$tmp" "$BIN" -process "$BIN.so" -noanalysis -readOnly \
    -scriptPath "$ROOT/tools/ghidra" -postScript Decompile.java "$ROOT/symbols/$BIN." \
    "$tmp/out.c" "${@#0x}" 2>&1 | grep -E "ERROR|Exception" >&2 || true
"$ROOT/.venv/bin/python" "$ROOT/tools/ghidra/fixup.py" "$BIN" "$tmp/out.c"
