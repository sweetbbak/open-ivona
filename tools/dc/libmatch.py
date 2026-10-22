"""`dc libmatch`: identify statically linked library code (libgcc, GNU libstdc++/libsupc++, crt)
by matching every function of the NDK's prebuilt archives against the target, then verify each
hit with the full relocation-aware matcher. Verified functions get the library symbol name,
status `lib` and src `lib:<library>:<member>` so `dc check` re-verifies them like source TUs."""
from __future__ import annotations

import os
from collections import defaultdict

from . import match
from .project import Binary, path, ROOT


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
    taken = {f.name for f in b.funcs}
    proposal: dict[int, tuple] = {}
    for addr, hs in hits.items():
        names = {h[2] for h in hs}
        if len(names) != 1:
            if verbose:
                print(f"ambiguous {addr:#x}: {sorted(names)}")
            continue
        h = hs[0]
        f = funcs[addr]
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
    for a, h in proposal.items():
        f = funcs[a]
        if a in verified:
            lib, member, sym, cand, obj = verified[a]
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
    print(f"verified {len(verified)} library functions: " + ", ".join(f"{k}={v}" for k, v in sorted(by_lib.items())))


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
