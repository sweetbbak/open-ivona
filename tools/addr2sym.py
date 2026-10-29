#!/usr/bin/env python3
"""The function (symbol+offset) at an address of an ELF file, from its symbol table.

  tools/addr2sym.py FILE.so OFFSET...     e.g. build/voice/libvoice_en_gb_amy.so 0x1fd54

For crash reports of the runnable builds (tts_run prints module+offset).
"""
import sys

from elftools.elf.elffile import ELFFile


def main() -> int:
    e = ELFFile(open(sys.argv[1], "rb"))
    syms = []
    for name in (".symtab", ".dynsym"):
        t = e.get_section_by_name(name)
        if t:
            syms += [(s["st_value"] & ~1, s["st_size"], s.name) for s in t.iter_symbols()
                     if s["st_info"]["type"] == "STT_FUNC" and s["st_value"]]
    syms.sort()
    for a in sys.argv[2:]:
        x = int(a, 16)
        best = None
        for v, n, name in syms:
            if v <= x:
                best = (v, n, name)
            else:
                break
        if best:
            v, n, name = best
            inside = "" if x < v + n else " (past its end)"
            print(f"{x:#x}: {name}+{x - v:#x}{inside}")
        else:
            print(f"{x:#x}: ?")
    return 0


if __name__ == "__main__":
    sys.exit(main())
