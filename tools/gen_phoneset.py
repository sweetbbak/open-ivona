#!/usr/bin/env python3
"""Generate C for an IVONA (Flite-style) cst_phoneset read from a binary.

  tools/gen_phoneset.py BINARY ADDR NAME > file.c

A phoneset is {name, featnames, featvals, phonenames, silence, num_phones, fvtable, sorted}:
featnames and phonenames are NULL-terminated string arrays, featvals a NULL-terminated array of
constant string vals, fvtable num_phones rows of featvals indices (each ending in -1) plus one
extra row, a lone 0. Definitions come out in source order; GCC emits them in reverse, which is
the order they have in the binary.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
from dc.project import Binary  # noqa: E402
from gen_lex_table import c_lit, cstr  # noqa: E402


def main() -> int:
    b = Binary(sys.argv[1])
    addr = int(sys.argv[2], 16)
    name = sys.argv[3]
    w = lambda a: struct.unpack("<I", b.elf.read(a, 4))[0]
    i32 = lambda a: struct.unpack("<i", b.elf.read(a, 4))[0]
    pname, featnames, featvals, phonenames, silence, n, fvtable, sorted_ = (
        w(addr + 4 * k) for k in range(8))

    def strings(a):
        out = []
        while w(a):
            out.append(cstr(b, w(a)))
            a += 4
        return out

    feats = strings(featnames)
    phones = strings(phonenames)
    assert len(phones) == n, (len(phones), n)
    vals = []
    a = featvals
    while w(a):
        v = w(a)
        assert w(v) & 0xFFFF == 5, f"val at {v:#x} is not a string"
        vals.append(cstr(b, w(v + 4)))
        a += 4
    rows = []
    for k in range(n):
        r, row = w(fvtable + 4 * k), []
        while i32(r) != -1:
            row.append(i32(r))
            r += 4
        rows.append(row)
    end = w(fvtable + 4 * n)
    assert i32(end) == 0, f"fvtable[{n}] -> {end:#x} is not the 0 row"

    print(f"/* {name}: generated from {sys.argv[1]} 0x{addr:x} by tools/gen_phoneset.py */")
    print(f"static const char *const {name}_featnames[] = {{  /* 0x{featnames:x} */")
    print("    " + ", ".join(c_lit(f) for f in feats) + ", NULL")
    print("};\n")
    print(f"static const char *const {name}_phonenames[] = {{  /* 0x{phonenames:x} */")
    for k in range(0, n, 10):
        print("    " + ", ".join(c_lit(p) for p in phones[k:k + 10]) + ",")
    print("    NULL\n};\n")
    for k, row in enumerate(rows):
        print(f"static const int {name}_fv_{k:03d}[] = {{ "
              + ", ".join(str(x) for x in row + [-1]) + f" }};  /* {phones[k]} */")
    print(f"static const int {name}_fv_end[] = {{ 0 }};\n")
    print(f"static const int *const {name}_fvtable[] = {{  /* 0x{fvtable:x} */")
    for k in range(0, n, 6):
        print("    " + ", ".join(f"{name}_fv_{j:03d}" for j in range(k, min(n, k + 6))) + ",")
    print(f"    {name}_fv_end\n}};\n")
    for k, v in enumerate(vals):
        print(f"DEF_STATIC_CONST_VAL_STRING({name}_fval_{k}, {c_lit(v)});")
    print(f"\nstatic const cst_val *const {name}_featvals[] = {{  /* 0x{featvals:x} */")
    for k in range(0, len(vals), 6):
        print("    " + ", ".join(f"&{name}_fval_{j}" for j in range(k, min(len(vals), k + 6))) + ",")
    print("    NULL\n};\n")
    print(f"/* 0x{addr:x} */")
    print(f"const cst_phoneset {name} = {{")
    print(f"    {c_lit(cstr(b, pname))}, {name}_featnames, {name}_featvals, {name}_phonenames,")
    print(f"    {c_lit(cstr(b, silence))}, {n}, {name}_fvtable, {sorted_}")
    print("};")
    return 0


if __name__ == "__main__":
    sys.exit(main())
