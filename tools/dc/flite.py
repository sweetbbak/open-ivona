"""`dc flite`: locate embedded (IVONA-modified) CMU Flite 1.4 code in the engine.

1. build:  compile every Flite 1.4 source file with the engine's calibrated ARM flags and the
           IVONA-adjusted headers in include/flite/ (objects under build/flite/).
2. locate: find where each Flite function lives in the target, by
           a) unique string-literal anchors (a Flite function and a target function that reference
              the same rare strings),
           b) exact masked-byte matches,
           c) source-order propagation: functions of one Flite file are laid out in source order,
              so functions between two located neighbours map to the target functions between them,
           and score every proposal with the relocation-aware matcher (unknown names tolerated).
3. report: docs/notes/flite-locate.md, and high-confidence names are written to the table
           (only over auto FUN_ names; status/src are left alone).
"""
from __future__ import annotations

import difflib
import os
import shlex
import struct
import subprocess
from collections import defaultdict
from dataclasses import dataclass, field

from . import build, match
from .disasm import disassemble
from .project import Binary, path, ROOT

FLITE = os.path.join("external", "flite-1.4", "src")
# platform variants / files that cannot be part of an Android build
SKIP = {"utils/cst_file_palmos.c", "utils/cst_mmap_none.c", "utils/cst_mmap_win32.c",
        "utils/cst_alloc.c",  # signature changed by IVONA; matched by hand
        "audio/au_alsa.c", "audio/au_oss.c", "audio/au_palmos.c", "audio/au_sun.c", "audio/au_wince.c",
        "audio/auclient.c", "audio/auserver.c", "audio/au_command.c", "utils/cst_socket.c"}


def sources() -> list[str]:
    out = []
    for dp, _, fs in os.walk(path(FLITE)):
        for fn in sorted(fs):
            if fn.endswith(".c"):
                rel = os.path.relpath(os.path.join(dp, fn), path(FLITE))
                if rel not in SKIP:
                    out.append(rel)
    return sorted(out)


def obj_for(rel: str) -> str:
    return path("build", "flite", rel + ".o")


def build_all(b: Binary) -> dict[str, str | None]:
    fl = b.cfg["flags"]
    flags = list(fl["common"]) + list(fl["arm"]) + list(fl["c"]) + ["-Iinclude/flite", "-I" + FLITE + "/cg",
                                                                    "-w", "-DCST_NO_SOCKETS"]
    script = []
    for rel in sources():
        o = os.path.relpath(obj_for(rel), ROOT)
        os.makedirs(os.path.dirname(path(o)), exist_ok=True)
        src = os.path.join(FLITE, rel)
        script.append(f"echo '@@@ {rel}'; " + shlex.join(["arm-linux-androideabi-gcc", *flags, "-I" + os.path.dirname(src),
                                                          "-c", src, "-o", o]) + " 2>&1")
    env = dict(os.environ)
    env.setdefault("DC_NDK", b.cfg.get("toolchain", "r10e"))
    r = subprocess.run([path("bin", "agcc"), "sh", "-c", "; ".join(script)], capture_output=True, text=True, env=env)
    res, cur, msgs = {}, None, defaultdict(str)
    for line in r.stdout.splitlines():
        if line.startswith("@@@ "):
            cur = line[4:]
        elif cur:
            msgs[cur] += line + "\n"
    for rel in sources():
        res[rel] = None if os.path.exists(obj_for(rel)) else (msgs[rel] or "failed")
    return res


# ---------------------------------------------------------------------------------------------
@dataclass
class FFunc:
    file: str
    name: str
    order: int
    cand: match.Candidate
    strings: frozenset
    mnems: list = field(default_factory=list)
    callees: list = field(default_factory=list)


def _cand_strings(c: match.Candidate) -> frozenset:
    out = set()
    for r in c.relocs.values():
        sec = c.sections.get(r.sym.section)
        if sec is None or not (sec.flags & match.SHF_STRINGS):
            continue
        off = r.sym.value if r.sym.type != "STT_SECTION" else None
        if off is None:
            continue
        end = sec.data.find(b"\0", off)
        s = sec.data[off:end]
        if len(s) >= 4:
            out.add(s)
    return frozenset(out)


def _target_strings(m: match.Matcher, f) -> frozenset:
    items = disassemble(m.elf.read(f.addr, f.size), f.addr, f.mode, m.call_name)
    out = set()
    pc_bias = 8 if f.mode == "arm" else 4
    words = {it.addr: it for it in items if it.kind == "word"}
    for k, it in enumerate(items):
        if it.pool_ref is None or it.pool_ref not in words:
            continue
        reg = it.insn.reg_name(it.insn.operands[0].reg)
        for nx in items[k + 1:k + 10]:
            if nx.kind == "insn" and nx.mnem == "add" and nx.ops.replace(" ", "") in (f"{reg},pc,{reg}", f"{reg},pc"):
                v = (struct.unpack("<I", words[it.pool_ref].raw)[0] + nx.addr + pc_bias) & 0xFFFFFFFF
                s = m.elf.cstring(v, 400)
                if s is not None and len(s) >= 4:
                    out.add(s)
                break
    return frozenset(out)


def _mnems(items) -> list[str]:
    return [it.mnem for it in items if it.kind == "insn"]


def _cand_callees(c: match.Candidate) -> list[str]:
    return [r.sym.name for o, r in sorted(c.relocs.items()) if r.type in match.BRANCH_RELOCS]


def _target_callees(m: match.Matcher, f) -> list[str]:
    out = []
    for it in disassemble(m.elf.read(f.addr, f.size), f.addr, f.mode):
        if it.kind == "insn" and it.branch is not None and it.mnem in ("bl", "blx", "b") \
                and not (f.addr <= it.branch < f.end):
            out.append(m.call_name(it.branch) or f"FUN_{it.branch:08x}")
    return out


def _known(seq: list[str], known: set) -> tuple:
    return tuple(n for n in seq if n in known)


def report_path(stem: str) -> str:
    """docs/notes/flite-locate.md for the engine, flite-locate-<stem>.md for other binaries."""
    name = "flite-locate.md" if stem == "libttsivonavoice" else f"flite-locate-{stem}.md"
    return path("docs", "notes", name)


def locate(stem: str, apply: bool, verbose: bool = False, rounds: int = 6) -> None:
    b = Binary(stem)
    # --- Flite side
    ffuncs: list[FFunc] = []
    by_file: dict[str, list[FFunc]] = defaultdict(list)
    for rel in sources():
        o = obj_for(rel)
        if not os.path.exists(o):
            continue
        obj = match.load_object(o)
        names = [s.name for s in obj.syms if s.type == "STT_FUNC" and s.defined]
        for i, n in enumerate(names):
            c = match.candidate_from(obj, n)
            if c is None or not c.code:
                continue
            ff = FFunc(rel, n, i, c, _cand_strings(c), _mnems(disassemble(c.code, 0, c.mode)))
            ff.callees = _cand_callees(c)
            ffuncs.append(ff)
            by_file[rel].append(ff)
    flite_names = {ff.name for ff in ffuncs}

    proposals: dict[str, tuple[int, str, float]] = {}
    learned_total = 0
    for rnd in range(rounds):
        b.__dict__.pop("names", None)
        m = match.Matcher(b)
        tfuncs = [f for f in b.funcs if f.mode == "arm" and f.status != "lib"]
        tby_addr = {f.addr: f for f in tfuncs}
        taddrs = sorted(tby_addr)
        if rnd == 0:
            tstrings = {f.addr: _target_strings(m, f) for f in tfuncs}
            string_owners = defaultdict(set)
            for a, ss in tstrings.items():
                for s_ in ss:
                    string_owners[s_].add(a)
        known = {f.name for f in b.funcs if not f.name.startswith("FUN_")} | set(b.elf.imports)
        tcallees = {f.addr: _known(_target_callees(m, f), known) for f in tfuncs}
        sig_owners = defaultdict(set)
        for a, sig in tcallees.items():
            if len(sig) >= 2:
                sig_owners[sig].add(a)

        def score(ff: FFunc, a: int, learn=None) -> float:
            return match.Matcher(b, learn=learn).compare(tby_addr[a], ff.cand).score

        def propose(ff, a, how):
            if a not in tby_addr:
                return
            sc = score(ff, a)
            old = proposals.get(ff.name)
            if old is None or sc > old[2]:
                proposals[ff.name] = (a, how, sc)

        names = {f.name: f.addr for f in b.funcs}
        for ff in ffuncs:
            if ff.name in names:  # already named (by hand, earlier rounds or exports)
                propose(ff, names[ff.name], "name")
                continue
            votes = defaultdict(int)
            for s_ in ff.strings:
                owners = string_owners.get(s_, ())
                if 0 < len(owners) <= 2:
                    for a in owners:
                        votes[a] += 1
            if votes:
                a, v = max(votes.items(), key=lambda kv: kv[1])
                if list(votes.values()).count(v) == 1:
                    propose(ff, a, f"strings({v})")
            sig = _known(ff.callees, known)
            if len(sig) >= 2 and len(sig_owners.get(sig, ())) == 1:
                propose(ff, next(iter(sig_owners[sig])), f"calls({len(sig)})")

        # order propagation inside each Flite file
        for rel, fl in by_file.items():
            located = [(i, proposals[f.name][0]) for i, f in enumerate(fl)
                       if f.name in proposals and proposals[f.name][2] >= 0.5]
            spans = list(zip(located, located[1:]))
            if located:
                spans.append(((located[-1][0], located[-1][1]), (len(fl), None)))
                spans.insert(0, ((-1, None), (located[0][0], located[0][1])))
            for (i0, a0), (i1, a1) in spans:
                if i1 - i0 <= 1:
                    continue
                between_f = fl[i0 + 1:i1]
                if a0 is not None and a1 is not None:
                    if a1 <= a0:
                        continue
                    between_t = [a for a in taddrs if a0 < a < a1]
                elif a1 is None:
                    between_t = [a for a in taddrs if a > a0][:len(between_f) * 3 + 4]
                else:
                    between_t = [a for a in taddrs if a < a1][-(len(between_f) * 3 + 4):]
                for ff, a in _align(between_f, [tby_addr[x] for x in between_t], b, m):
                    if ff.name not in proposals:
                        sc = score(ff, a)
                        if sc >= (0.3 if (a0 is not None and a1 is not None) else 0.5):
                            proposals[ff.name] = (a, "order", sc)

        # resolve conflicts and adopt names (in memory) for confident proposals; learn callee names
        best_for_addr: dict[int, tuple[str, float]] = {}
        for n, (a, how, sc) in proposals.items():
            if a not in best_for_addr or sc > best_for_addr[a][1]:
                best_for_addr[a] = (n, sc)
        for n in [n for n, p in proposals.items() if best_for_addr[p[0]][0] != n]:
            del proposals[n]
        used = {f.name for f in b.funcs}
        changed = 0
        learned: dict[str, set] = {}
        for n, (a, how, sc) in proposals.items():
            f = tby_addr[a]
            ff = next(x for x in ffuncs if x.name == n)
            size_ok = len(ff.cand.code) <= f.size * 3 and f.size <= len(ff.cand.code) * 3
            if (sc >= 0.6 or (how.startswith("strings") and size_ok)) and f.name.startswith("FUN_") \
                    and n not in used:
                f.name, f.note = n, f"flite {how} {100 * sc:.0f}%"
                used.add(n)
                changed += 1
            if sc >= 0.8:
                ff = next(x for x in ffuncs if x.name == n)
                match.Matcher(b, learn=learned).compare(f, ff.cand)
        for name, addrs in learned.items():
            if len(addrs) == 1 and name in flite_names and name not in used:
                a = next(iter(addrs))
                f = tby_addr.get(a)
                if f is not None and f.name.startswith("FUN_"):
                    f.name, f.note = name, "flite (callee of a located function)"
                    used.add(name)
                    changed += 1
                    learned_total += 1
        print(f"round {rnd + 1}: {len(proposals)} located, {changed} new names")
        if not changed:
            break

    _report(b, by_file, proposals, stem)
    if apply:
        b.save()
        print("names written to the function table")


def _align(fl: list[FFunc], tl: list, b: Binary, m: match.Matcher) -> list[tuple[FFunc, int]]:
    """Order-preserving alignment of Flite functions to target functions by mnemonic similarity."""
    if not fl or not tl:
        return []
    tm = [_mnems(disassemble(b.elf.read(t.addr, t.size), t.addr, t.mode)) for t in tl]
    n, k = len(fl), len(tl)
    sim = [[difflib.SequenceMatcher(None, f.mnems, tm[j], autojunk=False).ratio() for j in range(k)] for f in fl]
    # DP: best total similarity with strictly increasing target index; functions may be unmatched
    NEG = -1.0
    dp = [[0.0] * (k + 1) for _ in range(n + 1)]
    back = [[None] * (k + 1) for _ in range(n + 1)]
    for i in range(1, n + 1):
        for j in range(0, k + 1):
            best, arg = dp[i - 1][j], ("skip_f", j)
            if j > 0:
                if dp[i][j - 1] > best:
                    best, arg = dp[i][j - 1], ("skip_t", j - 1)
                cand = dp[i - 1][j - 1] + (sim[i - 1][j - 1] if sim[i - 1][j - 1] >= 0.45 else NEG)
                if cand > best:
                    best, arg = cand, ("pair", j - 1)
            dp[i][j], back[i][j] = best, arg
    out, i, j = [], n, k
    while i > 0:
        kind, jj = back[i][j] if back[i][j] else ("skip_f", j)
        if kind == "pair":
            out.append((fl[i - 1], tl[jj].addr))
            i, j = i - 1, jj
        elif kind == "skip_t":
            j = jj
        else:
            i -= 1
    return list(reversed(out))


def _report(b: Binary, by_file, final, stem: str) -> None:
    lines = ["# Flite 1.4 locator report", "", "Generated by `./dc flite locate`. Score = relocation-aware "
             "matcher score of the unmodified Flite 1.4 function against the target (names tolerated).", ""]
    tot = found = exact = 0
    for rel in sorted(by_file):
        rows = []
        for ff in by_file[rel]:
            tot += 1
            p = final.get(ff.name)
            if p:
                found += 1
                exact += p[2] == 1.0
                f = b.func(f"{p[0]:x}")
                rows.append(f"| `{ff.name}` | `{p[0]:#x}` | {f.name} | {p[1]} | {100 * p[2]:.0f}% |")
            else:
                rows.append(f"| `{ff.name}` | - | | | |")
        lines += [f"## {rel}", "", "| Flite function | target | table name | how | score |", "|---|---|---|---|---|"]
        lines += rows + [""]
    lines.insert(4, f"Located {found}/{tot} Flite functions ({exact} byte-identical modulo names).\n")
    os.makedirs(path("docs", "notes"), exist_ok=True)
    with open(report_path(stem), "w") as fh:
        fh.write("\n".join(lines) + "\n")
    print(f"located {found}/{tot} Flite functions ({exact} exact); report: {os.path.relpath(report_path(stem), ROOT)}")


def import_file(stem: str, rel: str) -> str:
    """Copy Flite 1.4's `rel` into src/<bin>/flite/ with /* 0xADDR */ comments above every function
    whose name is in the function table, so `dc tu` can verify and record it."""
    import re
    b = Binary(stem)
    addr_of = {f.name: f.addr for f in b.funcs}
    src = open(path(FLITE, rel), encoding="latin-1").read().split("\n")
    out_rel = os.path.join("src", stem, "flite", os.path.basename(rel))
    if os.path.exists(path(out_rel)):
        raise SystemExit(f"{out_rel} exists; edit it instead")
    defn = re.compile(r"^(?:[A-Za-z_][\w \t\*]*?[\s\*])?([A-Za-z_]\w*)\s*\(([^;]*)$")
    out, located = [], 0
    for i, line in enumerate(src):
        m = defn.match(line)
        if m and not line.startswith((" ", "\t", "#", "/*", "*")) and m.group(1) in addr_of \
                and not line.rstrip().endswith(";"):
            # K&R-ish split: return type on the previous line
            if out and out[-1].strip() and not out[-1].rstrip().endswith((";", "}", "*/")) and "(" not in out[-1]:
                prev = out.pop()
                out.append(f"/* {addr_of[m.group(1)]:#x} */")
                out.append(prev)
            else:
                out.append(f"/* {addr_of[m.group(1)]:#x} */")
            located += 1
        out.append(line)
    header = (f"/* Imported from CMU Flite 1.4 src/{rel} by `dc flite import`; functions with an address comment\n"
              f" * are present in the engine (unmodified ones match as-is, others carry IVONA's changes). */\n")
    with open(path(out_rel), "w") as fh:
        fh.write(header + "\n".join(out))
    print(f"{out_rel}: {located} located functions annotated")
    return out_rel
