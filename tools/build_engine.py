#!/usr/bin/env python3
"""Build a runnable libttsivonavoice.so from the decompiled sources (not the matching build).

Every TU under src/libttsivonavoice is compiled with its matching flags plus -DIVONA_NO_CERT_CHECK
(docs/notes/cert-bypass.md) into build/engine/, then linked with gold against the vendored
libraries (build/libobj/*.a). Undefined symbols are reported, grouped by the function table's
address ranges, so they show what is still missing.

usage: tools/build_engine.py [-D NAME[=VAL]]... [--keep-going]
"""
from __future__ import annotations

import argparse
import collections
import os
import re
import shlex
import subprocess
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
from dc.project import Binary, path, ROOT  # noqa: E402
from dc import build  # noqa: E402

OUT = "build/engine"
VENDORED = ["openssl", "curl", "expat", "pcre", "silk", "cmp"]
SYSLIBS = ["-ldl", "-llog", "-lstdc++", "-lm", "-lc"]
# data-only TUs: no function row credits them, so name them here
EXTRA_SRCS = ["src/libttsivonavoice/charset_tables.c", "src/libttsivonavoice/runnable_stubs.c"]


def obj_for(src: str) -> str:
    return os.path.join(OUT, "obj", src + ".o")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-D", action="append", default=[], help="extra define (e.g. IVONA_CERT_SSML=1)")
    args = ap.parse_args()

    b = Binary("libttsivonavoice")
    # the TUs the function table credits (some live with the voice library; a TU no row names,
    # like the superseded src/libttsivonavoice/flite/cst_phoneset.c, would clash)
    srcs = {f.src for f in b.funcs if f.src.startswith("src/")} | set(EXTRA_SRCS)
    srcs = sorted(s for s in srcs if os.path.exists(path(s)))
    defines = ["-DIVONA_NO_CERT_CHECK"] + ["-D" + d for d in args.D]
    script = []
    for s in srcs:
        o = obj_for(s)
        os.makedirs(os.path.dirname(path(o)), exist_ok=True)
        if os.path.exists(path(o)):
            os.remove(path(o))
        cmd = build.compile_cmd(b, s)
        cmd = cmd[:cmd.index("-o") + 1] + [o]
        cmd[1:1] = defines
        script.append(f"echo '@@@ {s}'; " + shlex.join(cmd) + " 2>&1 | grep -E 'error' ")
    os.makedirs(path(OUT), exist_ok=True)
    so = os.path.join(OUT, "libttsivonavoice.so")
    objs = [obj_for(s) for s in srcs]
    # the vendored archives have no symbol index: link indexed copies
    libs = []
    for l in VENDORED:
        if os.path.exists(path("build/libobj", l + ".a")):
            libs.append(os.path.join(OUT, l + ".a"))
            script.insert(0, f"cp build/libobj/{l}.a {libs[-1]} && arm-linux-androideabi-ranlib {libs[-1]}")
    link = ["arm-linux-androideabi-g++", "-shared", "-fuse-ld=gold", "-o", so,
            "-Wl,--no-undefined", "-Wl,--unresolved-symbols=report-all", "-Wl,--gc-sections",
            "-Wl,--version-script=config/libttsivonavoice.exports",
            "-Wl,-soname,libttsivonavoice.so", *objs, *libs,
            "-L/opt/sysroot/usr/lib", *SYSLIBS]
    script.append("echo '@@@ link'; " + shlex.join(link) + " 2>&1")
    r = subprocess.run([path("bin", "agcc"), "sh", "-c", "; ".join(script)], capture_output=True,
                       text=True, cwd=ROOT)
    out = r.stdout + r.stderr

    failed = [s for s in srcs if not os.path.exists(path(obj_for(s)))]
    for s in failed:
        print(f"compile failed: {s}")
    undef = collections.Counter()
    for m in re.finditer(r"undefined reference to '([^']+)'", out):
        undef[m.group(1)] += 1
    multi = sorted(set(re.findall(r"multiple definition of '([^']+)'", out)))
    by_addr = {f.name: f for f in b.funcs}
    rows = []
    for name in undef:
        f = by_addr.get(name)
        rows.append((f.addr if f else 0, name, f))
    rows.sort()
    print(f"{len(srcs) - len(failed)}/{len(srcs)} TUs compiled; {len(undef)} undefined symbols;"
          f" {len(multi)} multiply defined")
    for addr, name, f in rows:
        if f:
            print(f"  {addr:08x} {f.size:5x} {f.status:11} {name}")
        else:
            print(f"  {'':8} {'':5} {'(data/other)':11} {name}")
    for name in multi:
        print(f"  multiple definition: {name}")
    if os.path.exists(path(so)) and not undef and not multi:
        print(f"linked {so}")
        return 0
    if not undef and not multi and not failed:
        print(out[-3000:])
    return 1


if __name__ == "__main__":
    sys.exit(main())
