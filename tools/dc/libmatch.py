"""`dc libmatch`: identify statically linked library code (libgcc, GNU libstdc++/libsupc++, crt)
by matching every function of the NDK's prebuilt archives against the target, then verify each
hit with the full relocation-aware matcher. Verified functions get the library symbol name,
status `lib` and src `lib:<library>:<member>` so `dc check` re-verifies them like source TUs."""
from __future__ import annotations

import os
from collections import defaultdict

from . import match
from .inventory import exidx_starts
from .project import Binary, Func, path, ROOT


def read_ar(fn: str) -> list[tuple[str, bytes]]:
    """Members of a GNU/SysV `ar` archive (handles the // long-name table)."""
    with open(fn, "rb") as fh:
        data = fh.read()
    if data[:8] != b"!<arch>\n":
        return [(os.path.basename(fn), data)]  # plain object file
    out, pos, longnames = [], 8, b""
    while pos + 60 <= len(data):
        hdr = data[pos:pos + 60]
        name = hdr[:16].decode().strip()
        size = int(hdr[48:58].decode().strip())
        body = data[pos + 60:pos + 60 + size]
        pos += 60 + size + (size & 1)
        if name == "//":
            longnames = body
            continue
        if name in ("/", "/SYM64/"):
            continue
        if name.startswith("/") and name[1:].isdigit():
            off = int(name[1:])
            name = longnames[off:longnames.index(b"/\n", off)].decode()
        out.append((name.rstrip("/"), body))
    return out


def _masked(code: bytes, relocs) -> bytes:
    b = bytearray(code)
    for o in relocs:
        b[o:o + 4] = b"\0\0\0\0"
    return bytes(b)


def libmatch(stem: str, libs: dict[str, str], verbose: bool = False) -> None:
    load_aliases(libs)
    b = Binary(stem)
    _drop_vanished(b, libs)
    by_size = defaultdict(list)  # (mode, span size) -> functions
    for f in b.funcs:
        by_size[(f.mode, f.size)].append(f)
    # The splitter can cut one library function into fragments (Thumb-1 `bl` far jumps, branches
    # inside hand-written assembly). Also offer runs of unverified rows as one span; a verified hit
    # folds the fragments back into a single row.
    merged = _merged_spans(b)
    for spans in merged.values():
        for m in spans:
            by_size[(m.mode, m.size)].append(m)

    # 1. candidates: (lib, member, symbol) -> Candidate ; signature hits by masked bytes
    hits: dict[int, list[tuple[str, str, str, match.Candidate]]] = defaultdict(list)
    n_syms = 0
    for lib, fn in libs.items():
        full = path(fn)
        if not os.path.exists(full):
            print(f"skip {lib}: {fn} missing (run toolchain/fetch_ndk.sh)")
            continue
        for member, obj in read_ar(full):
            parsed = match.load_object(f"{lib}:{member}", obj)
            for raw in parsed.func_names:
                cand = match.candidate_from(parsed, raw)
                if cand is None or len(cand.code) < 4:
                    continue
                sym = table_name(parsed, raw, member)
                n_syms += 1
                sig = _masked(cand.code, [o for o in cand.relocs])
                n = len(cand.code)
                for f in (f for pad in range(0, 9, 2) for f in by_size.get((cand.mode, n + pad), ())):
                    t = b.elf.read(f.addr, n)
                    if _masked(t, cand.relocs) == sig:
                        hits[f.addr].append((lib, member, sym, cand, obj, f))
    print(f"{n_syms} library functions scanned, {len(hits)} target functions with a signature hit")

    # 2. tentatively name every hit (needed so calls between library functions resolve), then
    #    verify with the full matcher; repeat until no more changes.
    funcs = {f.addr: f for f in b.funcs}
    merged_hits = set()
    for a, hs in list(hits.items()):
        pseudo = [h for h in hs if h[5] is not funcs[a]]
        if not pseudo or any(h[5] is funcs[a] for h in hs):
            hits[a] = [h for h in hs if h[5] is funcs[a]]  # the row itself matched: no folding
            continue
        span = min((h[5] for h in pseudo), key=lambda m: m.size)  # smallest span that hit
        hits[a] = [h for h in pseudo if h[5] is span]
        span.name = funcs[a].name
        funcs[a] = span  # compare against the merged span from here on
        merged_hits.add(a)
    taken = {f.name for f in b.funcs}
    proposal: dict[int, tuple] = {}
    for addr, hs in hits.items():
        names = {h[2] for h in hs}
        f = funcs[addr]
        if len(names) != 1:
            # identical code under several names (e.g. destructors): the target's own (exported)
            # name decides; otherwise the hit stays ambiguous
            if f.name in names:
                hs = [h for h in hs if h[2] == f.name]
            else:
                if verbose:
                    print(f"ambiguous {addr:#x}: {sorted(names)}")
                continue
        h = hs[0]
        if f.status in ("matched",) or (h[2] in taken and f.name != h[2]):
            continue
        proposal[addr] = h
    old_names = {a: funcs[a].name for a in proposal}
    for a, h in proposal.items():
        funcs[a].name = h[2]
    b.__dict__.pop("names", None)

    # Learn names of auto-named callees/data from library relocations, then verify strictly.
    learned: dict[str, set[int]] = {}
    for a, h in proposal.items():
        match.Matcher(b, learn=learned).compare(funcs[a], h[3])
    n_learned = _apply_learned(b, learned, taken)
    b.__dict__.pop("names", None)
    print(f"learned {n_learned} names from library relocations")

    def revert(a: int) -> None:
        funcs[a].name = old_names[a]
        b.__dict__.pop("names", None)
    verified = verify_fixed_point(proposal, lambda a, h: match.Matcher(b).compare(funcs[a], h[3]).matched, revert)
    folded = 0
    for a, h in proposal.items():
        f = funcs[a]
        if a in verified:
            lib, member, sym, cand, obj = verified[a][:5]
            if a in merged_hits:  # fold the fragments into the table row at a
                row = next(r for r in b.funcs if r.addr == a)
                inner = [r for r in b.funcs if a < r.addr < a + f.size]
                b.funcs = [r for r in b.funcs if not (a < r.addr < a + f.size)]
                row.size, row.name = f.size, f.name
                f = row
                folded += len(inner)
            f.status, f.score, f.src, f.note = "lib", "100", f"lib:{lib}:{member}", ""
            _save_member(lib, member, obj)
        else:
            f.name = old_names[a]
            if verbose:
                print(f"signature hit {a:#x} {h[2]} failed full verification")
    b.save()
    by_lib = defaultdict(int)
    for a, h in verified.items():
        by_lib[h[0]] += 1
    if folded:
        print(f"folded {folded} split fragments back into verified library functions")
    n_nb = _neighbour_pass(b, libs)
    if n_nb:
        b.save()
        print(f"verified {n_nb} more by object order next to verified neighbours")
    n_bad = _placement_pass(b, libs)
    n_bad += _island_pass(b, libs)
    n_bad += _final_pass(b, libs)
    if n_bad:
        b.save()
        print(f"dropped {n_bad} pairings that no longer verify or disagree on data placement")
    print(f"verified {len(verified)} library functions: " + ", ".join(f"{k}={v}" for k, v in sorted(by_lib.items())))


def table_name(obj, sym: str, member: str) -> str:
    """Table name of a library function: static functions are per object (name@member)."""
    if any(x.name == sym and x.bind == "STB_LOCAL" and x.defined and x.type == "STT_FUNC" for x in obj.syms):
        return match.qualified(sym, member)
    return sym


def _drop_vanished(b: Binary, libs: dict[str, str]) -> None:
    """A rebuilt (vendored) archive can lose members, e.g. after disabling an algorithm: their
    rows were coincidental matches and go back to todo."""
    members: dict[str, set[str]] = {}
    n = 0
    for f in b.funcs:
        if f.status != "lib" or not f.src.startswith("lib:"):
            continue
        _, lib, member = f.src.split(":", 2)
        if lib not in libs or not os.path.exists(path(libs[lib])):
            continue
        if lib not in members:
            members[lib] = {m for m, _ in read_ar(path(libs[lib]))}
        if member not in members[lib]:
            f.status, f.score, f.src, f.note, f.name = "todo", "", "", "", f"FUN_{f.addr:08x}"
            n += 1
    if n:
        b.save()
        print(f"{n} library rows reset: their archive member no longer exists")


def verify_fixed_point(proposal: dict, ok, revert) -> dict:
    """Keep the proposals that verify. A proposal that fails gets its old name back (revert),
    which can break a caller verified while the tentative name was in place, so repeat until
    nothing changes."""
    verified = dict(proposal)
    while True:
        failed = [a for a, h in verified.items() if not ok(a, h)]
        if not failed:
            return verified
        for a in failed:
            del verified[a]
            revert(a)


def placement_outliers(placements: dict[int, dict[str, int]]) -> set[int]:
    """Functions (addr -> {section: target address}) disagreeing with their object's unique
    majority placement of a section; on a tie no placement is accepted."""
    votes: dict[str, dict[int, int]] = defaultdict(lambda: defaultdict(int))
    for pl in placements.values():
        for sec, at in pl.items():
            votes[sec][at] += 1

    def accepted(sec: str, at: int) -> bool:
        v = votes[sec]
        best = max(v.values())
        return v[at] == best and list(v.values()).count(best) == 1
    return {a for a, pl in placements.items() if not all(accepted(s, at) for s, at in pl.items())}


_OBJ_CACHE: dict[str, object] = {}


def _verify_rows(b: Binary, libs: dict[str, str], src: str, fs: list[Func]) -> dict[int, match.Result]:
    """Full verification of the rows that claim library member `src` (parsed once)."""
    _, lib, member = src.split(":", 2)
    if src not in _OBJ_CACHE:
        _OBJ_CACHE[src] = match.load_object(ensure_member(libs, lib, member))
    obj = _OBJ_CACHE[src]
    m = match.Matcher(b)
    out = {}
    for f in fs:
        cand = match.candidate_from(obj, match.base_name(f.name))
        if cand is None:
            r = match.Result(f)
            r.problems.append(f"{f.name} is not defined in {src}")
            out[f.addr] = r
        else:
            out[f.addr] = m.compare(f, cand)
    return out


def _placement_pass(b: Binary, libs: dict[str, str]) -> int:
    """Functions of one object must agree on where its data sections (.bss, .data, ...) landed,
    as `dc check` requires. Identical code from another object can verify on its own; keep the
    placement most of the object's functions agree on and return the others to todo."""
    by_src: dict[str, list[Func]] = defaultdict(list)
    for f in b.funcs:
        if f.status == "lib" and f.src.startswith("lib:") and f.src.split(":")[1] in libs:
            by_src[f.src].append(f)
    n = 0
    for src, fs in by_src.items():
        if len(fs) < 2:
            continue
        _, lib, member = src.split(":", 2)
        res = _verify_rows(b, libs, src, fs)
        bad = placement_outliers({a: r.placements for a, r in res.items()})
        for f in fs:
            if f.addr in bad:
                f.status, f.score, f.src, f.note = "todo", "", "", ""
                f.name = f"FUN_{f.addr:08x}"
                n += 1
    return n


def island_rows(rows: list[tuple[int, str, str]]) -> set[int]:
    """rows: (addr, library, member) of verified library functions in address order. A static
    archive's code is one contiguous block, so identical code from the wrong library (tiny -O0
    setters are byte-identical across OpenSSL and Expat) shows up as a lone row: its member has
    no adjacent row, both nearest verified neighbours come from one other library, and it lies
    inside that library's main range (central 95% of its rows) but outside its own."""
    by_lib: dict[str, list[int]] = defaultdict(list)
    for a, lib, _ in rows:
        by_lib[lib].append(a)

    def main_range(lib: str) -> tuple[int, int]:
        v = by_lib[lib]
        k = len(v) // 40
        return v[k], v[len(v) - 1 - k]
    ranges = {lib: main_range(lib) for lib in by_lib}
    out = set()
    for i in range(1, len(rows) - 1):
        a, lib, member = rows[i]
        prev, nxt = rows[i - 1], rows[i + 1]
        if (prev[1], prev[2]) == (lib, member) or (nxt[1], nxt[2]) == (lib, member):
            continue
        other = prev[1]
        if other == lib or nxt[1] != other:
            continue
        lo, hi = ranges[other]
        own_lo, own_hi = ranges[lib]
        if lo <= a <= hi and not own_lo <= a <= own_hi:
            out.add(a)
    return out


def _island_pass(b: Binary, libs: dict[str, str]) -> int:
    rows = sorted((f.addr, *f.src.split(":")[1:3]) for f in b.funcs
                  if f.status == "lib" and f.src.startswith("lib:"))
    bad = island_rows(rows)
    for f in b.funcs:
        if f.addr in bad and f.src.split(":")[1] in libs:
            f.status, f.score, f.src, f.note, f.name = "todo", "", "", "", f"FUN_{f.addr:08x}"
    return len(bad)


def _final_pass(b: Binary, libs: dict[str, str]) -> int:
    """Resetting a row to todo (FUN_ name) can break callers verified against its library name:
    re-verify every library row (with the per-object placement rule) until nothing changes, so
    libmatch never leaves a row `dc check` rejects."""
    n = 0
    while True:
        bad = []
        by_src: dict[str, list[Func]] = defaultdict(list)
        for f in b.funcs:
            if f.status == "lib" and f.src.startswith("lib:") and f.src.split(":")[1] in libs:
                by_src[f.src].append(f)
        b.__dict__.pop("names", None)
        for src, fs in by_src.items():
            res = _verify_rows(b, libs, src, fs)
            out = placement_outliers({a: r.placements for a, r in res.items()})
            bad += [f for f in fs if not res[f.addr].matched or f.addr in out]
        if not bad:
            return n
        for f in bad:
            f.status, f.score, f.src, f.note, f.name = "todo", "", "", "", f"FUN_{f.addr:08x}"
        n += len(bad)


def _apply_learned(b: Binary, learned: dict[str, set[int]], taken: set[str]) -> int:
    """Give auto-named (FUN_/data) target addresses the library names their references imply,
    when a name points at exactly one address."""
    funcs = {f.addr: f for f in b.funcs}
    data_names = {d.name for d in b.data}
    n = 0
    for name, addrs in learned.items():
        if len(addrs) != 1 or name in taken or name in data_names:
            continue
        addr = next(iter(addrs))
        if addr in funcs and funcs[addr].name.startswith("FUN_") and funcs[addr].status == "todo":
            funcs[addr].name = name
            taken.add(name)
            n += 1
        elif addr not in funcs and b.elf.section_at(addr) and not b.elf.section_at(addr).name.startswith(".text"):
            from .project import Data
            b.data.append(Data(addr, 0, name, "named from library relocation"))
            data_names.add(name)
            n += 1
    b.__dict__.pop("names", None)
    return n


def _member_order(obj) -> list[tuple[str, ...]]:
    """Function symbols of an object in link order (section index, then offset); aliases at one
    address (C1/C2, D1/D2 constructors and destructors) form one group."""
    sec_index = {name: i for i, name in enumerate(obj.sections)}
    groups: dict[tuple, list[str]] = {}
    for x in obj.syms:
        if x.type == "STT_FUNC" and x.defined and x.section in sec_index:
            groups.setdefault((sec_index[x.section], x.value & ~1), []).append(table_name(obj, x.name, obj.path))
    return [tuple(groups[k]) for k in sorted(groups)]


def _neighbour_pass(b: Binary, libs: dict[str, str]) -> int:
    """Identical-code library functions (e.g. several `what()` returning different strings) are
    ambiguous by signature. Gold keeps an object's functions together in section order, so the
    unverified rows right after a verified library function are paired with the following
    functions of the same object (skipping ones --gc-sections dropped); every pairing must still
    pass the full matcher."""
    rows = sorted(b.funcs, key=lambda f: f.addr)
    taken = {f.name for f in b.funcs}
    objs: dict[str, tuple] = {}
    n = 0
    for i, p in enumerate(rows):
        if p.status != "lib" or not p.src.startswith("lib:"):
            continue
        _, lib, member = p.src.split(":", 2)
        if lib not in libs:
            continue
        if p.src not in objs:
            obj = match.load_object(ensure_member(libs, lib, member))
            objs[p.src] = (obj, _member_order(obj))
        obj, order = objs[p.src]
        if not any(p.name in g for g in order):
            continue
        for step in (1, -1):
            n += _walk(b, rows, i, step, obj, order, p.src, taken)
    return n


def _walk(b, rows, i, step, obj, order, src, taken) -> int:
    """Pair the unverified rows next to verified row i (direction `step`) with the object's
    neighbouring functions; a few unused ones may be skipped (dropped by --gc-sections)."""
    n = 0
    k = next(n for n, g in enumerate(order) if rows[i].name in g) + step
    j = i + step
    while 0 <= j < len(rows) and rows[j].status == "todo" and 0 <= k < len(order):
        f = rows[j]
        span = range(k, min(k + 4, len(order))) if step > 0 else range(k, max(k - 4, -1), -1)
        for kk in span:
            group = order[kk]
            if f.name in group:
                sym = f.name  # an exported alias name is what other code references
            elif any(g in taken for g in group):
                continue
            else:
                sym = group[0]
            cand = match.candidate_from(obj, match.base_name(sym))
            if cand is None or cand.mode != f.mode or not (0 <= f.size - len(cand.code) <= 8):
                continue
            old = f.name
            f.name = sym
            b.__dict__.pop("names", None)
            learned: dict[str, set[int]] = {}
            if match.Matcher(b, learn=learned).compare(f, cand).matched and learned:
                _apply_learned(b, learned, taken)  # callees/data named by this pairing
            if match.Matcher(b).compare(f, cand).matched:
                f.status, f.score, f.src, f.note = "lib", "100", src, "by object order"
                taken.add(sym)
                n += 1
                k = kk + step
                break
            f.name = old
            b.__dict__.pop("names", None)
        else:
            break
        j += step
    return n


def _has_prologue(b: Binary, f: Func) -> bool:
    """Starts like a function: ARM `push {..}` / `str fp, [sp, #-4]!` (-O0 leaf), Thumb `push`.
    Splitter fragments (code after a jump table or a far branch) do not."""
    if f.mode == "arm":
        w = b.elf.u32(f.addr)
        return (w & 0xffff0000) == 0xe92d0000 or w == 0xe52db004
    h = b.elf.u32(f.addr) & 0xffff
    return (h & 0xfe00) == 0xb400


def _merged_spans(b: Binary) -> dict[int, list[Func]]:
    """Start address -> pseudo-functions covering 2, 3, ... consecutive unverified rows of one
    mode. Runs start at an exidx start or a named (exported) row and stop before the next one."""
    ex = set(exidx_starts(b))
    rows = sorted(b.funcs, key=lambda f: f.addr)
    out = {}
    for i, f in enumerate(rows):
        if f.status != "todo" or not (f.addr in ex or not f.name.startswith("FUN_") or _has_prologue(b, f)):
            continue
        spans = []
        j = i + 1
        while j < len(rows) and rows[j].status == "todo" and rows[j].mode == f.mode \
                and rows[j].addr not in ex and rows[j].name.startswith("FUN_") and j - i < 400:
            end = rows[j].addr + rows[j].size
            spans.append(Func(f.addr, end - f.addr, f.mode, f.name, "todo"))
            j += 1
        if spans:
            out[f.addr] = spans
    return out


def aliases_of(parsed) -> dict[str, set]:
    """Global symbols of one object that share an address, as name -> other names."""
    at = defaultdict(list)
    for x in parsed.syms:
        if x.defined and x.type in ("STT_FUNC", "STT_OBJECT") and x.bind != "STB_LOCAL":
            at[(x.section, x.value)].append(x.name)
    out = {}
    for names in at.values():
        for nm in names:
            if len(names) > 1:
                out[nm] = {n for n in names if n != nm}
    return out


def load_aliases(libs: dict[str, str]) -> None:
    """Fill match.ALIASES from every archive (cached in build/libobj/aliases.json)."""
    import json
    cache = path("build", "libobj", "aliases.json")
    key = sorted(libs.items())
    if os.path.exists(cache):
        with open(cache) as fh:
            data = json.load(fh)
        if data.get("libs") == key:
            match.ALIASES.update({k: tuple(v) for k, v in data["aliases"].items()})
            return
    out: dict[str, set] = defaultdict(set)
    for lib, fn in libs.items():
        if not os.path.exists(path(fn)):
            continue
        for member, obj in read_ar(path(fn)):
            for nm, als in aliases_of(match.load_object(f"{lib}:{member}", obj)).items():
                out[nm].update(als)
    aliases = {k: sorted(v) for k, v in out.items() if v}
    os.makedirs(os.path.dirname(cache), exist_ok=True)
    with open(cache, "w") as fh:
        json.dump({"libs": key, "aliases": aliases}, fh)
    match.ALIASES.update({k: tuple(v) for k, v in aliases.items()})


def member_path(lib: str, member: str) -> str:
    return path("build", "libobj", lib, member)


def _save_member(lib: str, member: str, obj: bytes) -> None:
    p = member_path(lib, member)
    os.makedirs(os.path.dirname(p), exist_ok=True)
    if not os.path.exists(p):
        with open(p, "wb") as fh:
            fh.write(obj)


def ensure_member(libs: dict[str, str], lib: str, member: str) -> str:
    p = member_path(lib, member)
    if not os.path.exists(p):
        for m, obj in read_ar(path(libs[lib])):
            if m == member:
                _save_member(lib, member, obj)
    return p
