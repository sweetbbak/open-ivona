"""dc — matching decompilation harness CLI.

  dc split              build/refresh symbols/<bin>.funcs.tsv (runs Ghidra export if needed)
  dc ls [status...]     list functions (filter by status)
  dc asm FUNC           annotated target disassembly of FUNC (name or hex address)
  dc diff FUNC [-s SRC] compile SRC (or the function's recorded src) and diff against target
                        (-s lib:LIB:MEMBER [--sym NAME] diffs against a library object member)
  dc tu SRC             compile SRC once and diff every table function it defines (records src)
  dc learn SRC          name FUN_/D_ callees and data after what SRC references (from good matches)
  dc check              rebuild every TU and verify all `matched` functions still match
  dc status             print progress and regenerate docs/progress.md
  dc rename OLD NEW     rename a function/data symbol in the tables and all sources
  dc data ADDR NAME [SIZE]   name a data address (used to verify GOT/GOTOFF/PC-relative refs)
  dc dump ADDR N FIELDS emit C initializers for N structs (fields: s,p,i,u,h,H,b,B,f)
  dc libmatch           find + verify statically linked NDK library functions (libgcc, gnustl, crt)
  dc vendor build LIB   compile pinned third-party sources (config/vendor.yaml) into build/libobj/LIB.a
  dc flite build|locate|all [--apply]   compile Flite 1.4 and locate it in the engine
  dc flite import hrg/cst_item.c ...      copy Flite sources in with address comments
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
    if src.startswith("lib:"):  # library member: diff only (dc libmatch owns lib rows)
        from .libmatch import ensure_member, load_aliases
        libs = project_cfg().get("libraries", {})
        load_aliases(libs)
        _, lib, member = src.split(":", 2)
        res = match.run(b, f, ensure_member(libs, lib, member), a.sym)
        print_result(res, verbose=not a.quiet)
        return
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


ADDR_COMMENT = re.compile(r"/\*\s*0x([0-9a-fA-F]+)\b[^*]*(?:\*(?!/)[^*]*)*\*/\s*\n"
                          r"(?:[ \t]*\S[^\n]*\n){0,3}?[^\n(;]*?\b([A-Za-z_]\w*)\s*\(")


MACRO_NAME = re.compile(r"[A-Z_][A-Z0-9_]*")


def addr_comment_names(text: str) -> list[tuple[int, str]]:
    """(addr, name) for every `/* 0xADDR ...` comment directly above a function definition.
    ALL_CAPS names are macro invocations (e.g. CST_VAL_REGISTER_TYPE), not definitions: skipped."""
    out = []
    for m in ADDR_COMMENT.finditer(text):
        if not MACRO_NAME.fullmatch(m.group(2)):
            out.append((int(m.group(1), 16), m.group(2)))
    return out


def symbol_for(ident: str, defined: set[str] | None) -> str | None:
    """The object symbol a source identifier defines: itself (C, extern "C") or a C++ mangled
    name whose last name component is it (_Z<len><ident>... or ...<len><ident>[C1/D1...]E...).
    A static function GCC cloned (`ident.isra.0`, `.constprop.N`, `.part.N`) maps to its
    clone, which is the symbol the code lives in. None when ambiguous or absent."""
    if defined is None or ident in defined:
        return ident
    clones = [s for s in defined
              if re.fullmatch(rf"{re.escape(ident)}(?:\.(?:isra|constprop|part)\.\d+)+", s)]
    if clones:
        return clones[0] if len(clones) == 1 else None
    tag = f"{len(ident)}{ident}"
    cands = sorted(s for s in defined if s.startswith("_Z" + tag) or s.startswith("_ZL" + tag) or
                   re.search(rf"{tag}(?:[CD][0-3])?E", s))
    return cands[0] if len(cands) == 1 else None


def adopt_names(b: Binary, src: str, defined: set[str] | None = None) -> None:
    """A `/* 0xADDR ...` comment directly above a function definition is authoritative: the table
    entry at ADDR gets the function's name (so a reset table can always be rebuilt from source).
    `defined` (the compiled object's function symbols) maps C++ identifiers to mangled names.
    Only sources of this binary (src/<bin>/...) carry its addresses: a TU shared with another
    binary (libvoice reusing the engine's Flite files) adopts nothing."""
    if not src.startswith(f"src/{b.stem}/"):
        return
    text = open(path(src)).read()
    by_addr = {f.addr: f for f in b.funcs}
    used = {f.name for f in b.funcs}
    for addr, ident in addr_comment_names(text):
        name = symbol_for(ident, defined)
        if name is None:
            if not by_addr.get(addr) or by_addr[addr].name not in (defined or ()):
                print(f"{YELLOW}{src}: cannot tell which symbol {ident} at {addr:#x} is{RESET}")
            continue
        f = by_addr.get(addr)
        if f is None:
            print(f"{YELLOW}{src}: no function starts at {addr:#x} (for {name}){RESET}")
            continue
        if f.name != name:
            if name in used:
                print(f"{YELLOW}{src}: {name} already names another function; not renaming {f.name}{RESET}")
                continue
            used.discard(f.name)
            used.add(name)
            f.name = name


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
    adopt_names(b, src, names)
    b.save()
    b = Binary(a.bin)
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


def cmd_learn(a):
    """Name auto-named (FUN_/D_) target functions and data after the symbols a TU references,
    using every function of the TU that already scores >= --min. Names must be consistent."""
    from .project import Data
    b = Binary(a.bin)
    src = os.path.relpath(os.path.abspath(a.src), ROOT)
    if build.compile_units(b, [src])[src] is not None:
        raise SystemExit(1)
    obj = build.obj_path(b, src)
    o = match.load_object(obj)
    by_name = {f.name: f for f in b.funcs}
    learned: dict[str, set] = {}
    for n in [x.name for x in o.syms if x.type == "STT_FUNC" and x.defined]:
        f = by_name.get(n)
        c = match.candidate_from(o, n)
        if f is None or c is None:
            continue
        probe: dict[str, set] = {}
        res = match.Matcher(b, learn=probe).compare(f, c)
        if res.score >= a.min:
            for k, v in probe.items():
                learned.setdefault(k, set()).update(v)
    used = {f.name for f in b.funcs} | {d.name for d in b.data}
    funcs = {f.addr: f for f in b.funcs}
    data_addrs = {d.addr for d in b.data}
    n = 0
    for name, addrs in sorted(learned.items()):
        if len(addrs) != 1 or name in used:
            continue
        addr = next(iter(addrs))
        if addr in funcs:
            if funcs[addr].name.startswith("FUN_"):
                print(f"function {addr:#x}: {funcs[addr].name} -> {name}")
                funcs[addr].name = name
                n += 1
        elif addr not in data_addrs and b.elf.section_at(addr) and not b.elf.section_at(addr).name.startswith(".text"):
            print(f"data {addr:#x}: {name}")
            b.data.append(Data(addr, 0, name, f"learned from {src}"))
            n += 1
    b.save()
    print(f"learned {n} names")


def check_placements(placements: dict, src: str, name: str, res) -> None:
    """Data placement must agree per object: a matching function's section placements are
    recorded and checked against the earlier matching functions of the same object. A function
    that does not match may pair its literals with the wrong instructions (a PC-relative
    literal's anchor at another offset), so its placements are neither recorded nor checked."""
    if not res.matched:
        return
    for sec, at in res.placements.items():
        prev = placements.setdefault((src, sec), (at, name))
        if prev[0] != at:
            res.problems.append(f"{src} {sec} is at {at:#x} here but at {prev[0]:#x} for {prev[1]}")


def cmd_check(a):
    """Rebuild every TU and re-verify every function that has a source (C/C++ or library member).
    Also requires all functions of a TU to agree on where each of its data sections landed."""
    from .libmatch import ensure_member, load_aliases
    libs = project_cfg().get("libraries", {})
    load_aliases(libs)
    bins = [a.bin] if a.only else all_bins()
    bad = 0
    for stem in bins:
        if not os.path.exists(path("symbols", stem + ".funcs.tsv")):
            continue
        b = Binary(stem)
        from .libmatch import overlapping_rows
        for f in overlapping_rows(b.funcs):
            print(f"{RED}{stem}: table row {f.name} at {f.addr:#x} overlaps another row{RESET}")
            bad += 1
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
            check_placements(placements, f.src, f.name, res)
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
             "| binary | functions | matched | matched bytes | nonmatching | wip | lib | verified bytes (matched + lib) |",
             "|---|---|---|---|---|---|---|---|"]
    for stem in all_bins():
        if not os.path.exists(path("symbols", stem + ".funcs.tsv")):
            continue
        b = Binary(stem)
        tot = len(b.funcs)
        tot_b = sum(f.size for f in b.funcs)
        cnt = {s: [f for f in b.funcs if f.status == s] for s in STATUSES}
        mb = sum(f.size for f in cnt["matched"])
        vb = mb + sum(f.size for f in cnt["lib"])
        lines.append(f"| {stem} | {tot} | {len(cnt['matched'])} ({100 * len(cnt['matched']) / tot:.1f}%) | "
                     f"{mb:#x} / {tot_b:#x} ({100 * mb / tot_b:.2f}%) | {len(cnt['nonmatching'])} | "
                     f"{len(cnt['wip'])} | {len(cnt['lib'])} | {vb:#x} ({100 * vb / tot_b:.1f}%) |")
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
    # Sources are only rewritten for placeholder names; a real name may also be a macro or a
    # field/local elsewhere (renaming CST_VAL_REGISTER_TYPE once rewrote the macro's users).
    if not (re.fullmatch(r"(FUN|D|DAT)_[0-9a-fA-F]+", a.old) or a.src):
        print(f"{YELLOW}tables updated; sources not rewritten (use --src to rewrite {a.old} in src/ and include/){RESET}")
        return
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


def cmd_vendor(a):
    from .vendor import build
    build(a.bin, a.lib)


def cmd_libmatch(a):
    from .libmatch import libmatch
    from .vendor import recipes
    vend = recipes()
    libs = {k: v for k, v in project_cfg()["libraries"].items()
            if k not in vend or a.bin in vend[k].get("binaries", [])}  # vendored code: listed binaries only
    libmatch(a.bin, libs, a.verbose)


def cmd_flite(a):
    from . import flite
    b = Binary(a.bin)
    if a.action in ("build", "all"):
        res = flite.build_all(b)
        bad = {k: v for k, v in res.items() if v}
        print(f"compiled {len(res) - len(bad)}/{len(res)} Flite files")
        for k, v in bad.items():
            print(f"{YELLOW}{k}: {v.strip().splitlines()[0] if v.strip() else 'failed'}{RESET}")
    if a.action in ("locate", "all"):
        flite.locate(a.bin, apply=a.apply, verbose=a.verbose)
    if a.action == "import":
        for rel in a.files:
            flite.import_file(a.bin, rel)


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
    s.add_argument("--sym", help="symbol in the library member (default: the function name)")
    s.set_defaults(fn=cmd_diff)
    s = sub.add_parser("tu"); s.add_argument("src"); s.add_argument("-v", "--verbose", action="store_true")
    s.set_defaults(fn=cmd_tu)
    s = sub.add_parser("learn"); s.add_argument("src"); s.add_argument("--min", type=float, default=0.8)
    s.set_defaults(fn=cmd_learn)
    s = sub.add_parser("check"); s.add_argument("--only", action="store_true", help="only the -b binary")
    s.add_argument("-v", "--verbose", action="store_true"); s.set_defaults(fn=cmd_check)
    s = sub.add_parser("status"); s.set_defaults(fn=cmd_status)
    s = sub.add_parser("rename"); s.add_argument("old"); s.add_argument("new")
    s.add_argument("--src", action="store_true", help="also rewrite a non-placeholder name in src/ and include/")
    s.set_defaults(fn=cmd_rename)
    s = sub.add_parser("data"); s.add_argument("addr"); s.add_argument("name"); s.add_argument("size", nargs="?")
    s.set_defaults(fn=cmd_data)
    s = sub.add_parser("selftest"); s.set_defaults(fn=cmd_selftest)
    s = sub.add_parser("flite"); s.add_argument("action", choices=["build", "locate", "all", "import"])
    s.add_argument("files", nargs="*", help="Flite source files (relative to src/) for `import`")
    s.add_argument("--apply", action="store_true", help="name located functions in the table")
    s.add_argument("-v", "--verbose", action="store_true"); s.set_defaults(fn=cmd_flite)
    s = sub.add_parser("vendor"); s.add_argument("action", choices=["build"]); s.add_argument("lib")
    s.set_defaults(fn=cmd_vendor)
    s = sub.add_parser("libmatch"); s.add_argument("-v", "--verbose", action="store_true")
    s.set_defaults(fn=cmd_libmatch)
    s = sub.add_parser("dump"); s.add_argument("addr"); s.add_argument("count", type=int)
    s.add_argument("fields"); s.add_argument("--stride", type=lambda x: int(x, 0)); s.set_defaults(fn=cmd_dump)
    a = p.parse_args(argv)
    _color(sys.stdout.isatty() and not a.no_color)
    a.bin = a.bin or default_bin()
    a.fn(a)
