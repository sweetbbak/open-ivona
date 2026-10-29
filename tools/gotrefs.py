#!/usr/bin/env python3
"""What a function's GOT-relative literals refer to.

  tools/gotrefs.py BINARY FUNC

PIC code loads the GOT base (`ldr rX, =_GLOBAL_OFFSET_TABLE_-(.+8); add rX, pc, rX`) and then
small negative literals: offsets of GOT slots (`ldr rY, [rX, rZ]`) or of local objects
(GOTOFF, `add`). For each such literal this prints the address it designates and, for a GOT
slot, the symbol the slot is relocated to (or the table name of the address it holds).
"""
import os
import re
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from dc.disasm import disassemble  # noqa: E402
from dc.project import Binary, path  # noqa: E402


def main() -> int:
    b = Binary(sys.argv[1])
    f = b.func(sys.argv[2])
    names = {x.addr: x.name for x in b.funcs}
    names.update({d.addr: d.name for d in b.data})
    rel = {}
    out = subprocess.run(["readelf", "-r", "-W", path("target", b.stem + ".so")],
                         capture_output=True, text=True).stdout
    for line in out.splitlines():
        p = line.split()
        if len(p) >= 3 and re.fullmatch(r"[0-9a-f]{8}", p[0]):
            rel[int(p[0], 16)] = (p[2], p[4] if len(p) > 4 else "")
    items = disassemble(b.elf.read(f.addr, f.size), f.addr, f.mode)
    w = lambda a: struct.unpack("<I", b.elf.read(a, 4))[0]
    # the GOT base: the literal loaded into a register that is then added to pc
    got = None
    loaded = {}
    for it in items:
        if it.kind != "insn":
            continue
        t = f"{it.insn.mnemonic} {it.insn.op_str}"
        m = re.match(r"ldr (\w+), \[pc, #(0x[0-9a-f]+|\d+)\]", t)
        if m:
            loaded[m.group(1)] = it.addr + 8 + int(m.group(2), 0)
        m = re.match(r"add (\w+), pc, (\w+)", t)
        if m and m.group(2) in loaded and got is None:
            cand = (w(loaded[m.group(2)]) + it.addr + 8) & 0xffffffff
            sec = b.elf.section_at(cand)
            if sec is not None and sec.name in (".got", ".got.plt"):
                got = cand
    if got is None:
        print("no GOT base found")
        return 1
    print(f"GOT base {got:#x}")
    for it in items:
        if it.kind == "insn":
            continue
        v = struct.unpack("<i", b.elf.read(it.addr, 4))[0]
        if -0x10000 < v < 0:
            a = (got + v) & 0xffffffff
            sec = b.elf.section_at(a)
            if sec is not None and sec.name == ".got":
                kind, sym = rel.get(a, ("?", ""))
                held = w(a)
                what = sym or names.get(held, f"{held:#x}")
                print(f"  {it.addr:08x} #{v}: GOT[{a:#x}] -> {what}  ({kind})")
            else:
                print(f"  {it.addr:08x} #{v}: {a:#x} {names.get(a, '')} ({sec.name if sec else '?'})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
