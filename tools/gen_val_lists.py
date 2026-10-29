#!/usr/bin/env python3
"""Generate C for a Flite-style table of constant string val lists (us_gpos.c) from a binary.

  tools/gen_val_lists.py BINARY ADDR NAME > file.c

The table is a NULL-terminated array of NULL-terminated `const cst_val *` arrays whose vals are
constant strings; each val is emitted once (DEF_STATIC_CONST_VAL_STRING), named by address.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
from dc.project import Binary  # noqa: E402
from gen_lex_table import c_lit, cstr  # noqa: E402


def main() -> int:
    b = Binary(sys.argv[1])
    addr, name = int(sys.argv[2], 16), sys.argv[3]
    w = lambda a: struct.unpack("<I", b.elf.read(a, 4))[0]
    lists, vals = [], {}
    k = 0
    while w(addr + 4 * k):
        l, items = w(addr + 4 * k), []
        j = 0
        while w(l + 4 * j):
            v = w(l + 4 * j)
            assert w(v) & 0xFFFF == 5, f"val {v:#x} is not a string"
            vals.setdefault(v, cstr(b, w(v + 4)))
            items.append(v)
            j += 1
        lists.append((l, items))
        k += 1
    print(f"/* {name}: generated from {sys.argv[1]} 0x{addr:x} by tools/gen_val_lists.py */")
    for v in sorted(vals):
        print(f"DEF_STATIC_CONST_VAL_STRING({name}_v{v:x}, {c_lit(vals[v])});")
    print()
    for i, (l, items) in enumerate(lists):
        print(f"static const cst_val *const {name}_{i}[] = {{  /* 0x{l:x}: {vals[items[0]]} */")
        line = "   "
        for v in items:
            t = f" &{name}_v{v:x},"
            if len(line) + len(t) > 100:
                print(line)
                line = "   "
            line += t
        print(line + " NULL\n};")
    print(f"\n/* 0x{addr:x} */")
    print(f"const cst_val *const *const {name}[] = {{")
    print("    " + ", ".join(f"{name}_{i}" for i in range(len(lists))) + ", NULL\n};")
    return 0


if __name__ == "__main__":
    sys.exit(main())
