"""dc — matching decompilation harness CLI.

  dc split              build/refresh symbols/<bin>.funcs.tsv (runs Ghidra export if needed)
  dc ls [status...]     list functions (filter by status)
  dc asm FUNC           annotated target disassembly of FUNC (name or hex address)
  dc diff FUNC [-s SRC] compile SRC (or the function's recorded src) and diff against target
  dc tu SRC             compile SRC once and diff every table function it defines (records src)
  dc check              rebuild every TU and verify all `matched` functions still match
  dc status             print progress and regenerate docs/progress.md
  dc rename OLD NEW     rename a function/data symbol in the tables and all sources
  dc data ADDR NAME [SIZE]   name a data address (used to verify GOT/GOTOFF/PC-relative refs)
  dc dump ADDR N FIELDS emit C initializers for N structs (fields: s,p,i,u,h,H,b,B,f)
  dc libmatch           find + verify statically linked NDK library functions (libgcc, gnustl, crt)
  dc selftest           link sample code with gold and verify the matcher end to end

Global option: -b/--bin STEM (default from config/project.yaml).
"""
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys

import yaml

from . import build, match
from .disasm import disassemble
from .project import Binary, Data, STATUSES, path, ROOT

RED, GREEN, YELLOW, DIM, RESET = "\x1b[31m", "\x1b[32m", "\x1b[33m", "\x1b[2m", "\x1b[0m"


def _color(on: bool):
    global RED, GREEN, YELLOW, DIM, RESET
    if not on:
        RED = GREEN = YELLOW = DIM = RESET = ""


def default_bin() -> str:
    with open(path("config", "project.yaml")) as fh:
        return yaml.safe_load(fh)["default"]


def all_bins() -> list[str]:
    with open(path("config", "project.yaml")) as fh:
        return yaml.safe_load(fh)["binaries"]


# ---------------------------------------------------------------------------------------------
def cmd_split(a):
    from .inventory import split
    j = path("build", "ghidra", a.bin + ".funcs.json")
    if a.ghidra or not os.path.exists(j):
        subprocess.run([path("tools", "ghidra", "export.sh"), a.bin], check=True)
    split(a.bin, j)


def cmd_ls(a):
    b = Binary(a.bin)
    for f in b.funcs:
        if a.status and f.status not in a.status:
            continue
        print(f"{f.addr:08x} {f.size:6x} {f.mode:5} {f.status:11} {f.score:>6} {f.name:40} {f.src}")


def cmd_asm(a):
    b = Binary(a.bin)
    f = b.func(a.func)
    m = match.Matcher(b)
    items = disassemble(b.elf.read(f.addr, f.size), f.addr, f.mode, m.call_name)
    notes = m.annotate_target(items, f.mode)
    print(f"; {f.name}  {f.addr:#x}..{f.end:#x}  {f.mode}  status={f.status}")
    for it in items:
        n = notes.get(it.addr, "")
        print(f"{it.addr:8x}:  {it.raw.hex():12}  {it.text:40} {DIM}{('; ' + n) if n else ''}{RESET}")


def _row_text(it, note, width):
    if it is None:
        return "".ljust(width)
    s = f"{it.addr:5x}: {it.text}"
    if note:
        s += f"  ; {note}"
    return s[:width].ljust(width)


def print_result(res: match.Result, verbose: bool = True):
    f = res.func
    if verbose:
        w = 62
        print(f"{'TARGET ' + f.name:{w}} | CANDIDATE")
        for r in res.rows:
            mark = " " if r.ok else "!"
            col = "" if r.ok else RED
            line = f"{col}{_row_text(r.t, r.t_note, w)} {mark} {_row_text(r.c, r.c_note, w).rstrip()}"
            if not r.ok and r.why:
                line += f"   <- {r.why}"
            print(line + RESET)
    for p in res.problems:
        print(f"{RED}problem: {p}{RESET}")
    for s in dict.fromkeys(res.soft):
        print(f"{YELLOW}note: {s}{RESET}")
    pct = 100 * res.score
    if res.matched:
        print(f"{GREEN}MATCH {f.name} ({f.size:#x} bytes){RESET}")
    else:
        print(f"{RED}{pct:.1f}% {f.name}{RESET}")


def cmd_diff(a):
    b = Binary(a.bin)
    f = b.func(a.func)
    src = a.src or f.src
    if not src:
        raise SystemExit(f"{f.name} has no source yet; pass -s src/{a.bin}/<file>.c")
    src = os.path.relpath(os.path.abspath(src), ROOT) if os.path.exists(src) else src
    if not os.path.exists(path(src)):
        raise SystemExit(f"{src} does not exist")
    old_src = f.src
    f.src = src
    if a.extra:
        build.EXTRA_FLAGS[:] = a.extra.split()
        a.no_save = True
    err = build.compile_units(b, [src])[src]
    if err is not None:
        f.src = old_src
        raise SystemExit(1)
    res = match.run(b, f, build.obj_path(b, src))
    print_result(res, verbose=not a.quiet)
    if a.no_save:
        return
    f.score = "100" if res.matched else f"{100 * res.score:.1f}"
    if res.matched:
        f.status = "matched"
    elif f.status in ("todo", "matched"):
        f.status = "wip" if f.status == "todo" else "nonmatching"
        if f.status == "nonmatching":
            print(f"{RED}REGRESSION: {f.name} was matched{RESET}")
    b.save()


def cmd_tu(a):
    """Compile one TU and diff every function it defines that is in the table."""
    b = Binary(a.bin)
    src = os.path.relpath(os.path.abspath(a.src), ROOT)
    obj = build.obj_path(b, src)
    mode_used = build.tu_mode(b, src)
    if build.compile_units(b, [src])[src] is not None:
        raise SystemExit(1)
    o = match.load_object(obj)
    names = {x.name for x in o.syms if x.type == "STT_FUNC" and x.defined}
    by_name = {f.name: f for f in b.funcs}
    known = [by_name[n] for n in sorted(names, key=lambda n: by_name[n].addr if n in by_name else 0) if n in by_name]
    missing = sorted(n for n in names if n not in by_name)
    for f in known:
        f.src = src
    if build.tu_mode(b, src) != mode_used:  # mode is inferred from the TU's target functions
        if build.compile_units(b, [src])[src] is not None:
            raise SystemExit(1)
    total = 0
    for f in known:
        res = match.run(b, f, obj)
        if a.verbose or not res.matched:
            print_result(res, verbose=a.verbose)
        else:
            print_result(res, verbose=False)
        f.score = "100" if res.matched else f"{100 * res.score:.1f}"
        if res.matched:
            f.status = "matched"
            total += 1
        elif f.status in ("todo", "matched"):
            f.status = "wip" if f.status == "todo" else "nonmatching"
    if missing:
        print(f"{YELLOW}not in the function table (rename targets first?): {', '.join(missing)}{RESET}")
    print(f"{src}: {total}/{len(known)} functions match")
    b.save()


def cmd_check(a):
    """Rebuild every TU and re-verify every function that has a source (C/C++ or library member).
    Also requires all functions of a TU to agree on where each of its data sections landed."""
    from .libmatch import ensure_member
    libs = project_cfg().get("libraries", {})
    bins = [a.bin] if a.only else all_bins()
    bad = 0
    for stem in bins:
        if not os.path.exists(path("symbols", stem + ".funcs.tsv")):
            continue
        b = Binary(stem)
        srcs = sorted({f.src for f in b.funcs if f.src and not f.src.startswith("lib:")})
        errs = build.compile_units(b, srcs, quiet=True) if srcs else {}
        objs = {}
        for f in b.funcs:
            if f.src.startswith("lib:"):
                _, lib, member = f.src.split(":", 2)
                objs[f.src] = ensure_member(libs, lib, member)
            elif f.src:
                objs[f.src] = build.obj_path(b, f.src)
        counts = {"matched": 0, "lib": 0}
        placements: dict[tuple[str, str], tuple[int, str]] = {}
        for f in b.funcs:
            if not f.src:
                continue
            if errs.get(f.src) is not None:
                if f.status == "matched":
                    print(f"{RED}{stem}: {f.src} failed to compile{RESET}\n{errs[f.src]}")
                    bad += 1
                continue
            res = match.run(b, f, objs[f.src])
            for sec, at in res.placements.items():
                prev = placements.setdefault((f.src, sec), (at, f.name))
                if prev[0] != at:
                    res.problems.append(f"{f.src} {sec} is at {at:#x} here but at {prev[0]:#x} for {prev[1]}")
            if f.status in ("matched", "lib"):
                if res.matched:
                    counts[f.status] += 1
                else:
                    bad += 1
                    print(f"{RED}{stem}: {f.name} ({f.status}) no longer matches ({100 * res.score:.1f}%){RESET}")
                    print_result(res, verbose=a.verbose)
            elif res.matched:
                print(f"{GREEN}{stem}: {f.name} now matches (was {f.status}){RESET}")
                f.status, f.score = "matched", "100"
                counts["matched"] += 1
        b.save()
        print(f"{stem}: verified {counts['matched']} matched + {counts['lib']} library functions")
    if bad:
        print(f"{RED}{bad} problem(s){RESET}")
        sys.exit(1)


def cmd_status(a):
    lines = ["# Decompilation progress", "", "Generated by `./dc status`. Do not edit.", "",
             "| binary | functions | matched | matched bytes | nonmatching | wip | lib |",
             "|---|---|---|---|---|---|---|"]
    for stem in all_bins():
        if not os.path.exists(path("symbols", stem + ".funcs.tsv")):
            continue
        b = Binary(stem)
        tot = len(b.funcs)
        tot_b = sum(f.size for f in b.funcs)
        cnt = {s: [f for f in b.funcs if f.status == s] for s in STATUSES}
        mb = sum(f.size for f in cnt["matched"])
        lines.append(f"| {stem} | {tot} | {len(cnt['matched'])} ({100 * len(cnt['matched']) / tot:.1f}%) | "
                     f"{mb:#x} / {tot_b:#x} ({100 * mb / tot_b:.2f}%) | {len(cnt['nonmatching'])} | "
                     f"{len(cnt['wip'])} | {len(cnt['lib'])} |")
    text = "\n".join(lines) + "\n"
    os.makedirs(path("docs"), exist_ok=True)
    with open(path("docs", "progress.md"), "w") as fh:
        fh.write(text)
    print(text)


def cmd_rename(a):
    b = Binary(a.bin)
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", a.new):
        raise SystemExit("new name must be a C identifier")
    hit = False
    for f in b.funcs:
        if f.name == a.old:
            f.name, hit = a.new, True
    for d in b.data:
        if d.name == a.old:
            d.name, hit = a.new, True
    if not hit:
        raise SystemExit(f"{a.old} not found in {a.bin} tables")
    if any(f.name == a.new for f in b.funcs if f.name != a.new) or sum(f.name == a.new for f in b.funcs) > 1:
        raise SystemExit(f"{a.new} already used")
    b.save()
    pat = re.compile(rf"\b{re.escape(a.old)}\b")
    for d in ("src", "include"):
        for dp, _, fs in os.walk(path(d)):
            for fn in fs:
                p = os.path.join(dp, fn)
                s = open(p).read()
                s2 = pat.sub(a.new, s)
                if s2 != s:
                    open(p, "w").write(s2)
                    print(f"updated {os.path.relpath(p, ROOT)}")


def cmd_data(a):
    b = Binary(a.bin)
    addr = int(a.addr, 16)
    for d in b.data:
        if d.addr == addr:
            d.name, d.size = a.name, int(a.size, 16) if a.size else d.size
            break
    else:
        b.data.append(Data(addr, int(a.size, 16) if a.size else 0, a.name))
    b.save()


def _c_string(b: bytes) -> str:
    out = []
    for c in b:
        ch = chr(c)
        if ch in '"\\':
            out.append("\\" + ch)
        elif 32 <= c < 127:
            out.append(ch)
        else:
            out.append({10: "\\n", 9: "\\t", 13: "\\r"}.get(c, f"\\{c:03o}"))
    return '"' + "".join(out) + '"'


def cmd_dump(a):
    """Emit C initializer rows for an array of structs. FIELDS: comma-separated codes
    s=char* string, p=pointer (symbol name), i=int32, u=uint32 hex, h=int16, H=uint16, b=int8, B=uint8, f=float."""
    import struct as st
    b = Binary(a.bin)
    e = b.elf
    sizes = {"s": 4, "p": 4, "i": 4, "u": 4, "f": 4, "h": 2, "H": 2, "b": 1, "B": 1}
    codes = a.fields.split(",")
    stride = a.stride or sum(sizes[c] for c in codes)
    base = int(a.addr, 16)
    for k in range(a.count):
        at = base + k * stride
        vals = []
        for c in codes:
            if c in "sp":
                w = e.u32(at)
                r = e.dynrelocs.get(at)
                if r is None and w == 0:
                    vals.append("0")
                elif r is not None and r.type != 23:
                    vals.append(f"&{r.sym}")
                elif c == "s" and e.cstring(w) is not None:
                    vals.append(_c_string(e.cstring(w)))
                else:
                    vals.append(b.names.describe(w & ~1 if e.text.addr <= w < e.text.end else w))
            elif c == "f":
                vals.append(repr(st.unpack("<f", e.read(at, 4))[0]) + "f")
            else:
                fmt = {"i": "<i", "u": "<I", "h": "<h", "H": "<H", "b": "<b", "B": "<B"}[c]
                v = st.unpack(fmt, e.read(at, sizes[c]))[0]
                vals.append(f"{v:#x}" if c in "uHB" else str(v))
            at += sizes[c]
        print(f"    {{ {', '.join(vals)} }}, /* {k} */")


def project_cfg() -> dict:
    with open(path("config", "project.yaml")) as fh:
        return yaml.safe_load(fh)


def cmd_libmatch(a):
    from .libmatch import libmatch
    libmatch(a.bin, project_cfg()["libraries"], a.verbose)


def cmd_selftest(a):
    sys.path.insert(0, path("tests"))
    import selftest
    sys.exit(selftest.main())


def main(argv=None):
    import signal
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    p = argparse.ArgumentParser(prog="dc", description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("-b", "--bin", default=None)
    p.add_argument("--no-color", action="store_true")
    sub = p.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("split"); s.add_argument("--ghidra", action="store_true", help="re-run Ghidra export")
    s.set_defaults(fn=cmd_split)
    s = sub.add_parser("ls"); s.add_argument("status", nargs="*"); s.set_defaults(fn=cmd_ls)
    s = sub.add_parser("asm"); s.add_argument("func"); s.set_defaults(fn=cmd_asm)
    s = sub.add_parser("diff"); s.add_argument("func"); s.add_argument("-s", "--src")
    s.add_argument("-q", "--quiet", action="store_true"); s.add_argument("-n", "--no-save", action="store_true")
    s.add_argument("-X", "--extra", help="extra compiler flags for an experiment (implies -n)")
    s.set_defaults(fn=cmd_diff)
    s = sub.add_parser("tu"); s.add_argument("src"); s.add_argument("-v", "--verbose", action="store_true")
    s.set_defaults(fn=cmd_tu)
    s = sub.add_parser("check"); s.add_argument("--only", action="store_true", help="only the -b binary")
    s.add_argument("-v", "--verbose", action="store_true"); s.set_defaults(fn=cmd_check)
    s = sub.add_parser("status"); s.set_defaults(fn=cmd_status)
    s = sub.add_parser("rename"); s.add_argument("old"); s.add_argument("new"); s.set_defaults(fn=cmd_rename)
    s = sub.add_parser("data"); s.add_argument("addr"); s.add_argument("name"); s.add_argument("size", nargs="?")
    s.set_defaults(fn=cmd_data)
    s = sub.add_parser("selftest"); s.set_defaults(fn=cmd_selftest)
    s = sub.add_parser("libmatch"); s.add_argument("-v", "--verbose", action="store_true")
    s.set_defaults(fn=cmd_libmatch)
    s = sub.add_parser("dump"); s.add_argument("addr"); s.add_argument("count", type=int)
    s.add_argument("fields"); s.add_argument("--stride", type=lambda x: int(x, 0)); s.set_defaults(fn=cmd_dump)
    a = p.parse_args(argv)
    _color(sys.stdout.isatty() and not a.no_color)
    a.bin = a.bin or default_bin()
    a.fn(a)
