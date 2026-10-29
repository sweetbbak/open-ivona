#!/usr/bin/env python3
"""Extract a binary's opaque byte tables (config/<bin>.assets.yaml) into C.

  tools/gen_assets.py BINARY [OUT.c]     (default build/gen/<bin>/assets.c)

Each asset becomes `const unsigned char NAME[SIZE]`, read from target/<bin>.so (whose sha256 is
checked when it is loaded). The output is rewritten only when it changes.
"""
import os
import sys

import yaml

sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
from dc.project import Binary, path  # noqa: E402


def literal(data: bytes) -> str:
    out, line = [], []
    prev_octal = False
    for c in data:
        if c in (0x22, 0x5c):
            t = "\\" + chr(c)
        elif 32 <= c < 127 and not (prev_octal and 0x30 <= c <= 0x37) and c != 0x3f:
            t = chr(c)
        else:
            t = "\\%o" % c
        prev_octal = t.startswith("\\") and t[1:].isdigit()
        line.append(t)
        if sum(len(x) for x in line) > 96:
            out.append('    "' + "".join(line) + '"')
            line, prev_octal = [], False
    if line:
        out.append('    "' + "".join(line) + '"')
    return "\n".join(out)


def main() -> int:
    stem = sys.argv[1]
    dst = sys.argv[2] if len(sys.argv) > 2 else path("build", "gen", stem, "assets.c")
    b = Binary(stem)
    with open(path("config", stem + ".assets.yaml")) as fh:
        assets = yaml.safe_load(fh)
    parts = [f"/* generated from target/{stem}.so by tools/gen_assets.py: do not edit */\n"]
    for a in assets:
        data = b.elf.read(a["addr"], a["size"])
        parts.append(f"/* 0x{a['addr']:x} */\nconst unsigned char {a['name']}[{a['size']}] =\n"
                     f"{literal(data)};\n")
    text = "\n".join(parts)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if not os.path.exists(dst) or open(dst).read() != text:
        with open(dst, "w") as fh:
            fh.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
