#!/usr/bin/env python3
"""Generate C for IVONA string rewrite rule tables (include/str_rewrite.h) and string lists read
from a binary.

  tools/gen_rewrite_rules.py BINARY {rules|list} ADDR NAME [{rules|list} ADDR NAME ...] > file.c

Rule rows are {cond, left, match, right, out}; a table ends at a row whose left is NULL. A
context is a NULL-terminated array of positions, each a NULL-terminated set of alternative
strings (a list). Contexts and sets that are not named on the command line become statics named
after their address, emitted once each before their first use.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
from dc.project import Binary  # noqa: E402
from gen_lex_table import c_lit, cstr  # noqa: E402


def main() -> int:
    b = Binary(sys.argv[1])
    w = lambda a: struct.unpack("<I", b.elf.read(a, 4))[0]
    args = sys.argv[2:]
    items = [(args[i], int(args[i + 1], 16), args[i + 2]) for i in range(0, len(args), 3)]

    def strs(a):
        out = []
        while w(a):
            out.append(cstr(b, w(a)))
            a += 4
        return out

    def list_c(name, v, static, addr):
        body = ", ".join(c_lit(s) for s in v) + (", NULL" if v else "NULL")
        return (f"{'static ' if static else ''}const char *const {name}[] = {{ {body} }};"
                f"  /* 0x{addr:x} */")

    names = {addr: name for kind, addr, name in items if kind == "list"}
    emitted = set()
    print(f"/* generated from {sys.argv[1]} by tools/gen_rewrite_rules.py */")
    for kind, addr, name in items:
        if kind == "list":
            if addr not in emitted:
                print(list_c(name, strs(addr), False, addr))
                emitted.add(addr)
            continue
        rows = []
        a = addr
        while w(a + 4):
            rows.append(tuple(w(a + 4 * k) for k in range(5)))
            a += 20
        for row in rows:
            for p in (row[1], row[3]):
                if p in emitted:
                    continue
                sets = []
                q = p
                while w(q):
                    s = w(q)
                    if s not in emitted:
                        names.setdefault(s, f"rw_set_{s:x}")
                        print(list_c(names[s], strs(s), True, s))
                        emitted.add(s)
                    sets.append(names[s])
                    q += 4
                names.setdefault(p, f"rw_ctx_{p:x}")
                body = ", ".join(sets) + (", NULL" if sets else "NULL")
                print(f"static const char *const *const {names[p]}[] = {{ {body} }};"
                      f"  /* 0x{p:x} */")
                emitted.add(p)
        print(f"\n/* 0x{addr:x} */")
        print(f"const str_rewrite_rule {name}[] = {{")
        for cond, left, match, right, out in rows:
            c = c_lit(cstr(b, cond)) if cond else "NULL"
            print(f"    {{ {c}, {names[left]}, {c_lit(cstr(b, match))}, {names[right]}, "
                  f"{c_lit(cstr(b, out))} }},")
        print("    { NULL, NULL, NULL, NULL, NULL }\n};\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
