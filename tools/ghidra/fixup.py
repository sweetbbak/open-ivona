#!/usr/bin/env python3
"""Post-process Ghidra C for tools/ghidra/decompile.sh: resolve PIC string and data references
to string literals or table names, and FUN_ calls to the table's names.

  tools/ghidra/fixup.py BINARY FILE
"""
import os
import re
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from dc.project import Binary  # noqa: E402


def main() -> int:
    b = Binary(sys.argv[1])
    names = {f.addr: f.name for f in b.funcs}
    names.update({d.addr: d.name for d in b.data})

    def word(a):
        try:
            return struct.unpack("<I", b.elf.read(a, 4))[0]
        except Exception:
            return None

    def lit(a):
        if a in names and not names[a].startswith("D_"):
            return "&" + names[a]
        try:
            s = b.elf.cstring(a, 200)
        except Exception:
            s = None
        if s and all(32 <= c < 127 or c in (9, 10) for c in s):
            t = s.decode().replace("\\", "\\\\").replace("\n", "\\n").replace('"', '\\"')
            return f'"{t}"'
        return f"D_{a:08x}"

    src = open(sys.argv[2]).read()
    # (DAT_x + 0xY): a PC-relative literal, the word at DAT_x plus the PC
    def pcrel(m):
        w = word(int(m.group(1), 16))
        return m.group(0) if w is None else lit((w + int(m.group(2), 16)) & 0xffffffff)
    src = re.sub(r"\(?(?:char \*|int|undefined4 \*)?\)?\(?DAT_([0-9a-f]{8}) \+ 0x([0-9a-f]+)\)?",
                 pcrel, src)
    # v = DAT_x; ... v + 0xY: the same with the word loaded into a variable first
    out, cur = [], {}
    for line in src.splitlines(keepends=True):
        m = re.match(r"\s*(\w+) = DAT_([0-9a-f]{8});", line)
        if m and word(int(m.group(2), 16)) is not None:
            cur[m.group(1)] = word(int(m.group(2), 16))

        def var(mm):
            v = cur.get(mm.group(1))
            if v is None:
                return mm.group(0)
            r = lit((v + int(mm.group(2), 16)) & 0xffffffff)
            return r if not r.startswith("D_") else mm.group(0)
        out.append(re.sub(r"\b(\w+) \+ (0x[0-9a-f]+)\b", var, line))
    src = "".join(out)
    src = re.sub(r"\bFUN_([0-9a-f]{8})\b", lambda m: names.get(int(m.group(1), 16), m.group(0)), src)
    sys.stdout.write(src)
    return 0


if __name__ == "__main__":
    sys.exit(main())
