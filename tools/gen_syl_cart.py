#!/usr/bin/env python3
"""Generate C for an IVONA syllabification CART (postlex_apply_rules) read from a binary.

  tools/gen_syl_cart.py BINARY ADDR NAME > file.c

The tree is {feats, vals, nodes}: feats are {kind, offset, phone feature name} questions about
the phones around a candidate boundary, vals the feature values asked for, nodes 6-byte
{feat, val, yes, no} records; a node whose yes and no are both 0 is a leaf whose val is the
answer. The tables are sized by walking the tree from node 0.
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
    feats, vals, nodes = w(addr), w(addr + 4), w(addr + 8)
    node = lambda i: struct.unpack("<BBHH", b.elf.read(nodes + 6 * i, 6))
    seen, work = set(), [0]
    nf = nv = 0
    while work:
        i = work.pop()
        if i in seen:
            continue
        seen.add(i)
        f, v, yes, no = node(i)
        if yes or no:
            nf, nv = max(nf, f + 1), max(nv, v + 1)
            work += [yes, no]
    n = max(seen) + 1
    print(f"/* {name}: generated from {sys.argv[1]} 0x{addr:x} by tools/gen_syl_cart.py */")
    print(f"static const syl_cart_feat {name}_feats[{nf}] = {{  /* 0x{feats:x} */")
    for k in range(nf):
        kind, off, s = w(feats + 12 * k), w(feats + 12 * k + 4), w(feats + 12 * k + 8)
        print(f"    {{ {kind}, {off}, {c_lit(cstr(b, s))} }},")
    print("};\n")
    print(f"static const char *const {name}_vals[{nv}] = {{  /* 0x{vals:x} */")
    print("    " + ", ".join(c_lit(cstr(b, w(vals + 4 * k))) for k in range(nv)))
    print("};\n")
    print(f"static const syl_cart_node {name}_nodes[{n}] = {{  /* 0x{nodes:x} */")
    for k in range(0, n, 4):
        print("    " + " ".join("{ %d, %d, %d, %d }," % node(j) for j in range(k, min(n, k + 4))))
    print("};\n")
    print(f"/* 0x{addr:x} */")
    print(f"const syl_cart {name} = {{ {name}_feats, {name}_vals, {name}_nodes }};")
    return 0


if __name__ == "__main__":
    sys.exit(main())
