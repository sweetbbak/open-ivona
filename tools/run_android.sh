#!/usr/bin/env bash
# Runs an ARM Android (bionic) program under qemu-user with the Android 4.4 /system from
# toolchain/fetch_android_sysroot.sh. A user namespace makes the bionic property area at
# toolchain/android-19/dev/__properties__ look root-owned, which bionic requires, and a PID
# namespace keeps thread ids below 65536 (bionic mutexes store 16 bits of the owner tid).
#   tools/run_android.sh [-l LIBDIR]... PROGRAM ARGS...
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SYS="$ROOT/toolchain/android-19"
[ -x "$SYS/system/bin/linker" ] || "$ROOT/toolchain/fetch_android_sysroot.sh"
if [ ! -f "$SYS/dev/__properties__" ]; then
    mkdir -p "$SYS/dev"
    # an empty KitKat property area: bytes_used (the root trie node), serial, magic, version
    python3 -c "import struct,sys; h=struct.pack('<IIII',20,0,0x504f5250,0xfc6ed0ab); sys.stdout.buffer.write(h+bytes(128*1024-len(h)))" > "$SYS/dev/__properties__"
    chmod 644 "$SYS/dev/__properties__"
fi
libs=()
while [ "${1:-}" = "-l" ]; do libs+=("$(cd "$2" && pwd)"); shift 2; done
LDP=$(IFS=:; echo "${libs[*]:-}")
# bionic (4.4) keeps only 16 bits of a thread id in mutexes: a PID namespace keeps tids small
exec unshare -r --pid --fork env LD_LIBRARY_PATH="$LDP" qemu-arm -L "$SYS" "$@"
