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
        fs, tu = {}, ""
        for s in elf.get_section_by_name(".symtab").iter_symbols():
            if s["st_info"]["type"] == "STT_FILE":
                tu = s.name
            if s["st_info"]["type"] == "STT_FUNC" and lo <= (s["st_value"] & ~1) < hi and s.name:
                a = s["st_value"] & ~1
                # static functions are per object: name@object, as dc libmatch names them
                name = match.qualified(s.name, tu + ".o") if s["st_info"]["bind"] == "STB_LOCAL" \
                    and s["st_other"]["visibility"] == "STV_DEFAULT" else s.name  # (hidden globals end up local)
                fs[a] = Func(a, 0, "thumb" if s["st_value"] & 1 else "arm", name, "todo")
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
    from dc.libmatch import aliases_of
    fails = 0
    so, objs = _build([], "base")
    match.ALIASES.clear()  # only the self-test objects' aliases (as libmatch does for archives)
    for o in objs.values():
        match.ALIASES.update({k: tuple(v) for k, v in aliases_of(match.load_object(o)).items()})
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

    # Static functions: each caller resolves to its own object's copy (recorded as a placement).
    twins = {f.name: f.addr for f in _funcs_from_symtab(so) if f.name.startswith("twin@")}
    got = {n: res[n].placements.get("func:twin") for n in ("twin_user_thumb", "twin_user_arm") if n in res}
    want = {"twin_user_thumb": twins.get("twin@sample"), "twin_user_arm": twins.get("twin@sample_arm")}
    if got != want or None in want.values() or not all(res[n].matched for n in got):
        fails += 1
        print(f"FAIL: static twins resolved {got}, want {want}")
    else:
        print("ok   same-named static functions resolve to their own object's copy")

    # Address-comment name adoption: definitions are adopted, macro invocations are not.
    from dc.cli import addr_comment_names
    got = addr_comment_names("/* 0x10 arm: f */\nint *foo(int x)\n{\n"
                             "/* 0x20 val_x, 0x24 x_val */\nCST_VAL_REGISTER_TYPE(x, int)\n")
    if got != [(0x10, "foo")]:
        fails += 1
        print(f"FAIL: address-comment adoption gave {got}")
    else:
        print("ok   address comments adopt definitions, not macro invocations")

    # C++ identifiers map to the object's mangled symbol (never to the bare identifier).
    from dc.cli import symbol_for
    syms = {"_Z16GetVoiceDataFunci", "_ZN9CIvonaTTSC1Ev", "_ZN9CIvonaTTS4initEv", "plain_c", "_ZL4statv"}
    got = [symbol_for(i, syms) for i in ("GetVoiceDataFunc", "init", "plain_c", "missing", "stat")]
    if got != ["_Z16GetVoiceDataFunci", "_ZN9CIvonaTTS4initEv", "plain_c", None, "_ZL4statv"]:
        fails += 1
        print(f"FAIL: symbol_for gave {got}")
    else:
        print("ok   address comments map C++ identifiers to mangled symbols")
    got = [symbol_for(i, {"walk.isra.0", "f.constprop.1.isra.2", "g.part.0", "g.part.1"})
           for i in ("walk", "f", "g", "wal")]
    if got != ["walk.isra.0", "f.constprop.1.isra.2", None, None]:
        fails += 1
        print(f"FAIL: symbol_for clones gave {got}")
    else:
        print("ok   address comments map static functions to their GCC clones")

    # Splitter: entries inside verified functions are fragments and must be dropped.
    from dc.inventory import inside_verified
    inside = inside_verified([Func(0x100, 0x40, "thumb", "big", "lib"), Func(0x140, 8, "thumb", "x")])
    got = [a for a in (0xfe, 0x100, 0x102, 0x13e, 0x140, 0x142) if inside(a)]
    if got != [0x102, 0x13e]:
        fails += 1
        print(f"FAIL: inside_verified gave {[hex(a) for a in got]}")
    else:
        print("ok   splitter drops fragments inside verified functions only")

    # libmatch: a failed proposal reverts its name, so callers verified against the tentative
    # name must be re-verified (fixed point); objects must agree on data placement.
    from dc.libmatch import verify_fixed_point, placement_outliers
    names = {1: "callee", 2: "caller"}
    old = {1: "FUN_1", 2: "FUN_2"}
    ok = lambda a, h: a == 2 and names[1] == "callee"  # callee itself fails; caller needs its name
    got = verify_fixed_point({1: None, 2: None}, ok, lambda a: names.__setitem__(a, old[a]))
    if got:
        fails += 1
        print(f"FAIL: verify_fixed_point kept {sorted(got)}")
    else:
        print("ok   libmatch re-verifies callers after a callee's tentative name is reverted")
    got = placement_outliers({1: {".bss": 0x100}, 2: {".bss": 0x100}, 3: {".bss": 0x200}, 4: {}})
    tie = placement_outliers({1: {".bss": 0x100}, 2: {".bss": 0x200}})
    if got != {3} or tie != {1, 2}:
        fails += 1
        print(f"FAIL: placement_outliers gave {got}, tie {tie}")
    else:
        print("ok   libmatch drops pairings that disagree with their object's data placement")

    # libmatch: a lone row from another library amid one library's code is a mis-attribution.
    from dc.libmatch import island_rows
    E = 4000  # expat block after a 1000-row openssl block; strays inside it
    rows = [(a, "openssl", f"o{a // 40}") for a in range(0, E, 4)]
    rows += [(a, "expat", "xmlparse") for a in range(E, E + 400, 4) if a not in (E + 100, E + 104, E + 200, E + 300)]
    rows += [(E + 100, "openssl", "o5"), (E + 104, "openssl", "o6"),  # two strays from two objects
             (E + 200, "openssl", "o3"),                              # a lone stray
             (E + 300, "curl", "url"), (E + 302, "curl", "url"),      # two rows of one object: kept
             (E + 404, "curl", "url"), (E + 408, "curl", "easy")]
    rows.sort()
    got = island_rows(rows)
    if got != {E + 100, E + 104, E + 200}:
        fails += 1
        print(f"FAIL: island_rows gave {sorted(got)}")
    else:
        print("ok   libmatch drops a lone row of another library inside a library's block")

    # libmatch merged spans may start at a prologue (a function the splitter cut at a jump table).
    from dc.libmatch import _has_prologue

    class _Elf:
        words = {0: 0xe92d4800, 4: 0xe52db004, 8: 0xe51b3008, 12: 0xb580, 16: 0x6803}
        u32 = staticmethod(lambda a: _Elf.words[a])
    fake = type("B", (), {"elf": _Elf})()
    got = [_has_prologue(fake, Func(a, 4, m, "f")) for a, m in
           ((0, "arm"), (4, "arm"), (8, "arm"), (12, "thumb"), (16, "thumb"))]
    if got != [True, True, False, True, False]:
        fails += 1
        print(f"FAIL: _has_prologue gave {got}")
    else:
        print("ok   prologue detection (push / -O0 leaf str fp) for merged library spans")

    # libmatch prefix spans: a row holding two functions is offered up to the inner prologue.
    from dc.libmatch import _prefix_spans

    class _Elf2:
        words = {0x100: 0xe92d4010, 0x104: 0xe1a04000, 0x108: 0xe8bd8010, 0x10c: 0xe92d4070,
                 0x110: 0xe92d0030, 0x114: 0xe12fff1e}  # push {r4,lr} ... pop; push {r4-r6,lr}; push {r4,r5}
        u32 = staticmethod(lambda a: _Elf2.words[a])
    fake2 = type("B", (), {"elf": _Elf2, "funcs": [Func(0x100, 0x18, "arm", "FUN_00000100", "todo")]})()
    got = [(m.addr, m.size) for m in _prefix_spans(fake2)]
    if got != [(0x100, 0xc)]:  # only push-with-lr counts as a prologue
        fails += 1
        print(f"FAIL: _prefix_spans gave {got}")
    else:
        print("ok   libmatch offers a row's prefix up to an inner prologue (under-split rows)")

    # Table integrity: rows starting inside another row (or repeating its address) are flagged.
    from dc.libmatch import overlapping_rows
    rows = [Func(0x100, 0x20, "arm", "a"), Func(0x108, 0x18, "arm", "FUN_00000108"),
            Func(0x120, 8, "arm", "b"), Func(0x120, 8, "arm", "FUN_00000120"), Func(0x128, 4, "arm", "c")]
    got = sorted((f.addr, f.name) for f in overlapping_rows(rows))
    if got != [(0x108, "FUN_00000108"), (0x120, "FUN_00000120")]:
        fails += 1
        print(f"FAIL: overlapping_rows gave {got}")
    else:
        print("ok   overlapping or repeated table rows are detected")

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
