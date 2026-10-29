#!/usr/bin/env python3
"""Cross-binary matching: find functions of one binary that are already decompiled in another.

  tools/xmatch.py [--from libttsivonavoice] [--to libvoice_en_gb_amy] [--apply] [--min 0.9]

The voice library statically contains copies of much of the engine (the embedded Flite, the
vox database, tracks, utilities). For every todo/wip function of --to, candidates are the
functions of --from that have a source and either the same name or the same instruction
sequence (mnemonics, operands masked). Each candidate's source is compiled with --to's flags and
compared by the real matcher, so nothing is recorded on a fingerprint alone.

Without --apply it only reports. With --apply, a MATCH is recorded in --to's table (status,
source, score) and the row is renamed through `dc rename` to the symbol the source defines (for
a placeholder name, FUN_xxx, the sources that refer to it follow). Non-matching candidates scoring
at least --min are listed for follow-up.

Never run `dc -b <to> tu` on a --from source instead: its address comments are --from addresses
and would rename unrelated rows of --to.
"""
from __future__ import annotations

import argparse
import collections
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
from dc import build, match  # noqa: E402
from dc.disasm import disassemble  # noqa: E402
from dc.project import Binary, path, ROOT  # noqa: E402


def signature(b: Binary, f) -> tuple:
    """The function's instruction mnemonics (literal pool words as 'w'): equal for the same
    code compiled into two binaries, whatever the addresses and registers."""
    items = disassemble(b.elf.read(f.addr, f.size), f.addr, f.mode)
    return tuple(it.insn.mnemonic if it.kind == "insn" else "w" for it in items)


def base(name: str) -> str:
    return name.split("@")[0]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--from", dest="src_bin", default="libttsivonavoice")
    ap.add_argument("--to", dest="dst_bin", default="libvoice_en_gb_amy")
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--min", type=float, default=0.9, help="report non-matches scoring at least this")
    args = ap.parse_args()

    src_b, dst_b = Binary(args.src_bin), Binary(args.dst_bin)
    by_sig = collections.defaultdict(list)
    by_name = collections.defaultdict(list)
    for f in src_b.funcs:
        if f.src.startswith("src/") and f.status in ("matched", "wip", "nonmatching"):
            by_name[base(f.name)].append(f)
            if f.size >= 8:
                by_sig[(f.mode, signature(src_b, f))].append(f)

    todo = [f for f in dst_b.funcs if f.status in ("todo", "wip")]
    plans = []  # (dst func, [candidate src funcs])
    for f in todo:
        cands = list(by_name.get(base(f.name), [])) if not f.name.startswith("FUN_") else []
        if f.size >= 8:
            for c in by_sig.get((f.mode, signature(dst_b, f)), []):
                if c not in cands:
                    cands.append(c)
        if cands:
            plans.append((f, cands))
    print(f"{len(todo)} todo/wip functions in {args.dst_bin}; {len(plans)} have candidates")

    # compile each candidate source once, with the destination binary's flags
    srcs = sorted({c.src for _, cs in plans for c in cs})
    errs = build.compile_units(dst_b, srcs, quiet=True)
    names = {f.name for f in dst_b.funcs}
    matched, near, renames = [], [], []
    for f, cands in plans:
        best = None
        for c in cands:
            if errs.get(c.src) is not None:
                continue
            res = match.run(dst_b, f, build.obj_path(dst_b, c.src), base(c.name))
            if best is None or res.score > best[1].score or res.matched:
                best = (c, res)
            if res.matched:
                break
        if best is None:
            continue
        c, res = best
        if res.matched:
            new = base(c.name)
            if new != f.name and new in names:
                # twin functions (byte-identical code, different symbols): the row cannot carry
                # the symbol without clashing, so nothing is recorded
                print(f"  twin: {f.name} has the code of {new}, a name the table already uses")
                continue
            matched.append((f, c))
            if args.apply:
                f.src, f.status, f.score = c.src, "matched", "100"
                # the row must carry the symbol the source defines; dc rename rewrites sources
                # only for placeholder names, so a real name (strrchr) changes in the table only
                if new != f.name:
                    renames.append((f.name, new))
                    names.add(new)
        elif res.score >= args.min:
            near.append((f, c, res.score))
    for f, c in matched:
        print(f"MATCH {f.addr:08x} {f.name:32} = {base(c.name):32} {c.src}")
    for f, c, sc in sorted(near, key=lambda x: -x[2]):
        print(f"near  {f.addr:08x} {f.name:32} ~ {base(c.name):32} {100 * sc:5.1f}% {c.src}")
    print(f"{len(matched)} match, {len(near)} near (>= {100 * args.min:.0f}%)")
    if args.apply and matched:
        dst_b.save()
        for old, new in renames:
            subprocess.run([path("dc"), "-b", args.dst_bin, "rename", old, new], cwd=ROOT,
                           check=True, stdout=subprocess.DEVNULL)
        print(f"recorded {len(matched)} matches, renamed {len(renames)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
