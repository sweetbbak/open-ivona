#!/usr/bin/env python3
"""Generate C for an IVONA lexicon table (lex_table, phones.c) read from a binary.

  tools/gen_lex_table.py BINARY ADDR NAME > file.c

A lex_table is {n, entries, phone_names}: entries is a sorted array of n pointers to lex_entry
records ({name, pos char, phone indices..., 0xff}), phone_names an array of phone strings. Each
record has its own length, so each gets its own struct type (LEX_ENTRY). The phone table is
emitted up to the highest index the entries use.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
from dc.project import Binary  # noqa: E402


def cstr(b, a):
    s = b.elf.cstring(a, 4096)
    return s.decode("latin1")


def c_lit(s):
    out = []
    for ch in s:
        o = ord(ch)
        if ch in '"\\':
            out.append("\\" + ch)
        elif 32 <= o < 127:
            out.append(ch)
        else:
            out.append(f"\\{o:03o}")
    return '"' + "".join(out) + '"'


def main() -> int:
    b = Binary(sys.argv[1])
    addr = int(sys.argv[2], 16)
    name = sys.argv[3]
    w = lambda a: struct.unpack("<I", b.elf.read(a, 4))[0]
    n, entries, phones = w(addr), w(addr + 4), w(addr + 8)
    recs = []
    maxp = -1
    for i in range(n):
        e = w(entries + 4 * i)
        word = cstr(b, w(e))
        pos = b.elf.read(e + 4, 1)[0]
        ph = []
        p = e + 5
        while True:
            c = b.elf.read(p, 1)[0]
            if c == 0xFF:
                break
            ph.append(c)
            maxp = max(maxp, c)
            p += 1
        recs.append((e, word, pos, ph))
    print(f"/* {name}: generated from {sys.argv[1]} 0x{addr:x} by tools/gen_lex_table.py */")
    print(f"static const char *const {name}_phones[{maxp + 1}] = {{  /* 0x{phones:x} */")
    for i in range(maxp + 1):
        print(f"    {c_lit(cstr(b, w(phones + 4 * i)))},")
    print("};\n")
    for i, (e, word, pos, ph) in enumerate(recs):
        body = ", ".join(str(x) for x in ph + [0xFF])
        posc = c_lit(chr(pos))[1:-1]
        print(f"LEX_ENTRY({name}_{i}, {c_lit(word)}, '{posc}', {body});")
    print(f"\nstatic const lex_entry *const {name}_entries[{n}] = {{  /* 0x{entries:x} */")
    for i in range(n):
        print(f"    (const lex_entry *)&{name}_{i},")
    print("};\n")
    print(f"static const lex_table {name} = {{ {n}, {name}_entries, {name}_phones }};"
          f"  /* 0x{addr:x} */")
    return 0


if __name__ == "__main__":
    sys.exit(main())
