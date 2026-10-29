#!/usr/bin/env python3
"""Generate C for simple tables read from a binary.

  tools/gen_tables.py BINARY KIND ADDR NAME [COUNT] [KIND ADDR NAME [COUNT] ...] > file.c

  strs ADDR NAME COUNT   const char *const NAME[COUNT] (NULL entries kept)
  ints ADDR NAME COUNT   const int NAME[COUNT]
  int  ADDR NAME         const int NAME
"""
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
from dc.project import Binary  # noqa: E402
from gen_lex_table import c_lit  # noqa: E402


def main() -> int:
    b = Binary(sys.argv[1])
    w = lambda a: struct.unpack("<I", b.elf.read(a, 4))[0]
    i32 = lambda a: struct.unpack("<i", b.elf.read(a, 4))[0]
    args = sys.argv[2:]
    print(f"/* generated from {sys.argv[1]} by tools/gen_tables.py */")
    while args:
        kind, addr, name = args[0], int(args[1], 16), args[2]
        n = int(args[3], 0) if kind != "int" else 1
        args = args[4:] if kind != "int" else args[3:]
        if kind == "int":
            print(f"const int {name} = {i32(addr)};  /* 0x{addr:x} */\n")
        elif kind == "ints":
            print(f"const int {name}[{n}] = {{  /* 0x{addr:x} */")
            v = [str(i32(addr + 4 * k)) for k in range(n)]
            for k in range(0, n, 10):
                print("    " + ", ".join(v[k:k + 10]) + ",")
            print("};\n")
        elif kind == "strs":
            print(f"const char *const {name}[{n}] = {{  /* 0x{addr:x} */")
            v = [c_lit(b.elf.cstring(w(addr + 4 * k), 4096).decode("latin1")) if w(addr + 4 * k)
                 else "NULL" for k in range(n)]
            line = "   "
            for x in v:
                if len(line) + len(x) + 2 > 100:
                    print(line)
                    line = "   "
                line += " " + x + ","
            print(line + "\n};\n")
        else:
            raise SystemExit(f"unknown kind {kind}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
