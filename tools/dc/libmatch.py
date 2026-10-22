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
    b = Binary(stem)
    by_size = defaultdict(list)  # (mode, span size) -> functions
    for f in b.funcs:
        by_size[(f.mode, f.size)].append(f)
    # Large Thumb-1 functions use `bl` as a far jump, so the splitter can cut one library
    # function into fragments. Also offer each exidx-delimited run of unverified rows as one span;
    # a verified hit folds the fragments back into a single row.
    merged = _merged_spans(b)
    for m in merged.values():
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
            for sym in parsed.func_names:
                cand = match.candidate_from(parsed, sym)
                if cand is None or len(cand.code) < 4:
                    continue
                n_syms += 1
                sig = _masked(cand.code, [o for o in cand.relocs])
                n = len(cand.code)
                for f in (f for pad in range(0, 9, 2) for f in by_size.get((cand.mode, n + pad), ())):
                    t = b.elf.read(f.addr, n)
                    if _masked(t, cand.relocs) == sig:
                        hits[f.addr].append((lib, member, sym, cand, obj))
    print(f"{n_syms} library functions scanned, {len(hits)} target functions with a signature hit")

    # 2. tentatively name every hit (needed so calls between library functions resolve), then
    #    verify with the full matcher; repeat until no more changes.
    funcs = {f.addr: f for f in b.funcs}
    merged_hits = {a for a, hs in hits.items() if any(h[3] is not None and len(h[3].code) > funcs[a].size
                                                      for h in hs) and a in merged}
    for a in merged_hits:  # compare against the merged span from here on
        funcs[a] = merged[a]
        merged[a].name = next(f.name for f in b.funcs if f.addr == a)
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
    data_names = {d.name for d in b.data}
    n_learned = 0
    for name, addrs in learned.items():
        if len(addrs) != 1 or name in taken or name in data_names:
            continue
        addr = addrs.pop()
        if addr in funcs and funcs[addr].name.startswith("FUN_") and funcs[addr].status == "todo":
            funcs[addr].name = name
            n_learned += 1
        elif addr not in funcs and b.elf.section_at(addr) and not b.elf.section_at(addr).name.startswith(".text"):
            from .project import Data
            b.data.append(Data(addr, 0, name, "named from library relocation"))
            n_learned += 1
    b.__dict__.pop("names", None)
    print(f"learned {n_learned} names from library relocations")

    verified: dict[int, tuple] = {}
    for a, h in proposal.items():
        res = match.Matcher(b).compare(funcs[a], h[3])
        if res.matched:
            verified[a] = h
    folded = 0
    for a, h in proposal.items():
        f = funcs[a]
        if a in verified:
            lib, member, sym, cand, obj = verified[a]
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
    print(f"verified {len(verified)} library functions: " + ", ".join(f"{k}={v}" for k, v in sorted(by_lib.items())))


def _member_order(obj) -> list[tuple[str, ...]]:
    """Function symbols of an object in link order (section index, then offset); aliases at one
    address (C1/C2, D1/D2 constructors and destructors) form one group."""
    sec_index = {name: i for i, name in enumerate(obj.sections)}
    groups: dict[tuple, list[str]] = {}
    for x in obj.syms:
        if x.type == "STT_FUNC" and x.defined and x.section in sec_index:
            groups.setdefault((sec_index[x.section], x.value & ~1), []).append(x.name)
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
            cand = match.candidate_from(obj, sym)
            if cand is None or cand.mode != f.mode or not (0 <= f.size - len(cand.code) <= 8):
                continue
            old = f.name
            f.name = sym
            b.__dict__.pop("names", None)
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


def _merged_spans(b: Binary) -> dict[int, Func]:
    """exidx start -> pseudo-function covering every unverified row up to the next exidx start or
    verified row, when that run has more than one row."""
    ex = sorted(set(exidx_starts(b)))
    rows = sorted(b.funcs, key=lambda f: f.addr)
    idx = {f.addr: i for i, f in enumerate(rows)}
    out = {}
    for k, a in enumerate(ex):
        if a not in idx or rows[idx[a]].status != "todo":
            continue
        limit = ex[k + 1] if k + 1 < len(ex) else None
        i = idx[a]
        j = i + 1
        while j < len(rows) and rows[j].status == "todo" and rows[j].mode == rows[i].mode \
                and (limit is None or rows[j].addr < limit):
            j += 1
        if j - i < 2:
            continue
        end = rows[j - 1].addr + rows[j - 1].size
        out[a] = Func(a, end - a, rows[i].mode, rows[i].name, "todo")
    return out


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
