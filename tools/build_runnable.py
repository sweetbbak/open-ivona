#!/usr/bin/env python3
"""Build a runnable library from the decompiled sources (not the matching build).

  tools/build_runnable.py [-b BINARY] [-D NAME[=VAL]]...

BINARY is libttsivonavoice (the engine, default; build/engine/) or libvoice_en_gb_amy (the voice
library; build/voice/). Every TU the binary's function table credits is compiled with its matching
flags plus the target's defines, then linked with gold against the vendored libraries
(build/libobj/*.a), keeping only the original's exports (config/BINARY.exports) with
--gc-sections. Undefined symbols are reported by table address: they are what is still missing.
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

# per binary: output directory, vendored archives, system libraries (the original's NEEDED),
# defines, and the TUs no function row credits (data-only, stubs) that must be linked too
TARGETS = {
    "libttsivonavoice": {
        "out": "build/engine",
        "vendored": ["openssl", "curl", "expat", "pcre", "silk", "cmp"],
        "syslibs": ["-ldl", "-llog", "-lstdc++", "-lm", "-lc"],
        "defines": ["-DIVONA_NO_CERT_CHECK"],
        "extra": ["src/libttsivonavoice/charset_tables.c", "src/libttsivonavoice/runnable_stubs.c",
                  "src/libttsivonavoice/flite/cst_endian.c"],  # cst_endian_loc
    },
    "libvoice_en_gb_amy": {
        "out": "build/voice",
        "vendored": ["pcre", "silk", "cmp"],
        "syslibs": ["-lm", "-ldl", "-llog", "-lc"],
        "defines": [],
        "extra": ["src/libvoice_en_gb_amy/runnable_stubs.c"],
    },
}
OUT = ""
TOOLBIN = "toolchain/ndk-r10e/android-ndk-r10e/toolchains/arm-linux-androideabi-4.8/prebuilt/linux-x86_64/bin"


def weaken_uncredited(b: Binary, srcs: list[str]) -> int:
    """A function some TU defines but the table credits to another TU of this binary (shared
    files duplicate helpers) is made weak in that object, so the credited definition wins.
    Returns how many symbols were weakened."""
    credited = {}
    for f in b.funcs:
        if f.src.startswith("src/"):
            credited[f.name.split("@")[0]] = f.src
    n = 0
    for s in srcs:
        o = path(obj_for(s))
        if not os.path.exists(o):
            continue
        nm = subprocess.run([path(TOOLBIN, "arm-linux-androideabi-nm"), "--defined-only", "-g", o],
                            capture_output=True, text=True).stdout
        weak = [l.split()[2] for l in nm.splitlines()
                if len(l.split()) == 3 and l.split()[1] == "T"
                and credited.get(l.split()[2], s) != s and credited[l.split()[2]] in srcs]
        if weak:
            subprocess.run([path(TOOLBIN, "arm-linux-androideabi-objcopy"),
                            *[f"--weaken-symbol={w}" for w in weak], o], check=True)
            n += len(weak)
    return n


def obj_for(src: str) -> str:
    return os.path.join(OUT, "obj", src + ".o")


def main() -> int:
    global OUT
    ap = argparse.ArgumentParser()
    ap.add_argument("-b", "--bin", default="libttsivonavoice", choices=sorted(TARGETS))
    ap.add_argument("-D", action="append", default=[], help="extra define (e.g. IVONA_CERT_SSML=1)")
    args = ap.parse_args()

    t = TARGETS[args.bin]
    OUT = t["out"]
    b = Binary(args.bin)
    # the TUs the function table credits (some are shared between the binaries; a TU no row names,
    # like the superseded src/libttsivonavoice/flite/cst_phoneset.c, would clash)
    srcs = {f.src for f in b.funcs if f.src.startswith("src/")} | set(t["extra"])
    srcs = sorted(s for s in srcs if os.path.exists(path(s)))
    defines = t["defines"] + ["-D" + d for d in args.D]
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
    so = os.path.join(OUT, args.bin + ".so")
    objs = [obj_for(s) for s in srcs]
    # the vendored archives have no symbol index: link indexed copies
    libs = []
    for l in t["vendored"]:
        if os.path.exists(path("build/libobj", l + ".a")):
            libs.append(os.path.join(OUT, l + ".a"))
            script.insert(0, f"cp build/libobj/{l}.a {libs[-1]} && arm-linux-androideabi-ranlib {libs[-1]}")
    link = ["arm-linux-androideabi-g++", "-shared", "-fuse-ld=gold", "--sysroot=/opt/sysroot", "-o", so,
            "-Wl,--no-undefined", "-Wl,--unresolved-symbols=report-all", "-Wl,--gc-sections",
            f"-Wl,--version-script=config/{args.bin}.exports",
            f"-Wl,-soname,{args.bin}.so", *objs, *libs,
            "-L/opt/sysroot/usr/lib", *t["syslibs"]]
    # compile, settle duplicate definitions by the table, then link
    r = subprocess.run([path("bin", "agcc"), "sh", "-c", "; ".join(script)], capture_output=True,
                       text=True, cwd=ROOT)
    out = r.stdout + r.stderr
    nweak = weaken_uncredited(b, srcs)
    r = subprocess.run([path("bin", "agcc"), "sh", "-c", shlex.join(link) + " 2>&1"],
                       capture_output=True, text=True, cwd=ROOT)
    out += r.stdout + r.stderr

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
          f" {len(multi)} multiply defined; {nweak} duplicate definitions settled by the table")
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
