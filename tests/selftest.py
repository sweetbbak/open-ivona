"""Harness self-test: compile sample TUs, link them with gold into a .so, then require that
(1) every function matches its own linked copy at 100% and (2) a mutated build is rejected.

Run: ./dc selftest
"""
from __future__ import annotations

import os
import shlex
import subprocess

from elftools.elf.elffile import ELFFile

from dc import build, match
from dc.project import Binary, Func, path, ROOT

SRCS = {"tests/selftest/sample.c": "thumb", "tests/selftest/sample_arm.c": "arm"}
OUT = path("build", "selftest")


def _binary(so: str) -> Binary:
    b = Binary("selftest", so_path=so, cfg_stem="libtiver", tables_dir=OUT)
    b.cfg["units"] = {s: {"mode": m} for s, m in SRCS.items()}
    return b


def _build(defs: list[str], tag: str) -> tuple[str, dict[str, str]]:
    b = _binary("/dev/null")
    cmds, objs = [], {}
    for s in SRCS:
        o = os.path.join("build", "selftest", tag, os.path.basename(s) + ".o")
        objs[s] = o
        cc = build.compile_cmd(b, s)
        cc[cc.index("-o") + 1] = o
        cmds.append(shlex.join(cc[:1] + defs + cc[1:]))
    so = os.path.join("build", "selftest", tag, "libsample.so")
    cmds.append(shlex.join(["arm-linux-androideabi-gcc", "-shared", "-fuse-ld=gold", "--sysroot=/opt/sysroot",
                            "-nostdlib", "-Wl,--no-undefined", "-o", so, *objs.values(), "-lc", "-lgcc"]))
    os.makedirs(path("build", "selftest", tag), exist_ok=True)
    r = subprocess.run([path("bin", "agcc"), "sh", "-c", " && ".join(cmds)], capture_output=True, text=True)
    if r.returncode:
        raise SystemExit(r.stdout + r.stderr)
    return path(so), {s: path(o) for s, o in objs.items()}


def _funcs_from_symtab(so: str) -> list[Func]:
    with open(so, "rb") as fh:
        elf = ELFFile(fh)
        text = elf.get_section_by_name(".text")
        lo, hi = text["sh_addr"], text["sh_addr"] + text["sh_size"]
        fs = {}
        for s in elf.get_section_by_name(".symtab").iter_symbols():
            if s["st_info"]["type"] == "STT_FUNC" and lo <= (s["st_value"] & ~1) < hi and s.name:
                a = s["st_value"] & ~1
                fs[a] = Func(a, 0, "thumb" if s["st_value"] & 1 else "arm", s.name, "todo")
    starts = sorted(fs)
    for i, a in enumerate(starts):
        fs[a].size = (starts[i + 1] if i + 1 < len(starts) else hi) - a
    return [fs[a] for a in starts]


def _run(so: str, objs: dict[str, str]) -> dict[str, match.Result]:
    b = _binary(so)
    b.funcs = _funcs_from_symtab(so)
    out = {}
    for f in b.funcs:
        for s, o in objs.items():
            if match.load_candidate(o, f.name) is not None:
                out[f.name] = match.run(b, f, o)
    return out


def main() -> int:
    from dc.cli import print_result
    fails = 0
    so, objs = _build([], "base")
    res = _run(so, objs)
    if len(res) < 9:
        print(f"FAIL: only {len(res)} functions found in the self-test .so")
        fails += 1
    for name, r in sorted(res.items()):
        if not r.matched:
            fails += 1
            print(f"FAIL: {name} does not match its own linked copy")
            print_result(r)
        else:
            print(f"ok   {name} ({r.func.mode}, {r.func.size:#x} bytes){'  soft: ' + '; '.join(r.soft) if r.soft else ''}")

    # Address-comment name adoption: definitions are adopted, macro invocations are not.
    from dc.cli import addr_comment_names
    got = addr_comment_names("/* 0x10 arm: f */\nint *foo(int x)\n{\n"
                             "/* 0x20 val_x, 0x24 x_val */\nCST_VAL_REGISTER_TYPE(x, int)\n")
    if got != [(0x10, "foo")]:
        fails += 1
        print(f"FAIL: address-comment adoption gave {got}")
    else:
        print("ok   address comments adopt definitions, not macro invocations")

    # Splitter: entries inside verified functions are fragments and must be dropped.
    from dc.inventory import inside_verified
    inside = inside_verified([Func(0x100, 0x40, "thumb", "big", "lib"), Func(0x140, 8, "thumb", "x")])
    got = [a for a in (0xfe, 0x100, 0x102, 0x13e, 0x140, 0x142) if inside(a)]
    if got != [0x102, 0x13e]:
        fails += 1
        print(f"FAIL: inside_verified gave {[hex(a) for a in got]}")
    else:
        print("ok   splitter drops fragments inside verified functions only")

    # Mutations: build the *target* differently; candidates from the clean build must fail in
    # exactly the functions whose code or referenced data changed.
    mutations = [
        (["-DMUTATE=1"], ["lookup"]),                        # instruction immediate
        (['-DGREETING="hullo %s"'], ["greet"]),              # string literal contents
        (["-DTABLE_LAST=7"], ["table_get"]),                 # static const table contents
        (["-DSUB_OP=op_add"], ["apply"]),                    # function pointer in rodata
    ]
    for k, (defs, want) in enumerate(mutations):
        so_m, _ = _build(defs, f"mut{k}")
        bad = sorted(n for n, r in _run(so_m, objs).items() if not r.matched)
        if bad != want:
            fails += 1
            print(f"FAIL: {defs}: expected mismatches {want}, got {bad}")
        else:
            print(f"ok   {' '.join(defs)} -> rejected {want} only")
    print("SELFTEST " + ("PASSED" if not fails else f"FAILED ({fails})"))
    return 1 if fails else 0
