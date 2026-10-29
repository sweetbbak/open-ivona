#!/usr/bin/env python3
"""Generate C for compiled Flite regexes (cst_regex_defs.h style) read from a binary.

  tools/gen_regex.py BINARY {ptr|rx} ADDR NAME [...] > file.c

  ptr ADDR NAME   a `const cst_regex *const NAME` at ADDR (its regex and program are static)
  rx  ADDR NAME   a `static const cst_regex NAME_rx` at ADDR

The program comment shows the pattern's nodes (Henry Spencer's regexp opcodes).
"""
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
from dc.project import Binary  # noqa: E402

OPS = {0: "END", 1: "BOL", 2: "EOL", 3: "ANY", 4: "ANYOF", 5: "ANYBUT", 6: "BRANCH", 7: "BACK",
       8: "EXACTLY", 9: "NOTHING", 10: "STAR", 11: "PLUS"}


def describe(prog: bytes) -> str:
    out, i = [], 1
    while i + 2 < len(prog):
        op = prog[i]
        i += 3
        name = OPS.get(op) or (f"OPEN{op - 20}" if 20 < op < 30 else f"CLOSE{op - 40}")
        if op in (4, 5, 8):
            j = prog.index(0, i)
            name += "(" + prog[i:j].decode("latin1") + ")"
            i = j + 1
        out.append(name)
    return " ".join(out)


def main() -> int:
    b = Binary(sys.argv[1])
    w = lambda a: struct.unpack("<I", b.elf.read(a, 4))[0]
    args = sys.argv[2:]
    print(f"/* generated from {sys.argv[1]} by tools/gen_regex.py */")
    for k in range(0, len(args), 3):
        kind, addr, name = args[k], int(args[k + 1], 16), args[k + 2]
        rx = w(addr) if kind == "ptr" else addr
        start, anch = b.elf.read(rx, 2)
        must, mlen, size, prog = w(rx + 4), w(rx + 8), w(rx + 12), w(rx + 16)
        assert must == 0, f"{name}: regmust is set"
        data = b.elf.read(prog, size)
        print(f"/* {describe(data)} */")
        print(f"static const unsigned char {name}_rxprog[] = {{  /* 0x{prog:x} */")
        for i in range(0, size, 16):
            print("   " + "".join(f" {x}," for x in data[i:i + 16]))
        print("};")
        print(f"static const cst_regex {name}_rx = {{  /* 0x{rx:x} */")
        print(f"   {start}, {anch}, NULL, {mlen}, {size},\n   (char *){name}_rxprog\n}};")
        if kind == "ptr":
            print(f"const cst_regex * const {name} = &{name}_rx;  /* 0x{addr:x} */")
        print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
