"""`dc split`: build/refresh the function table of a binary from the Ghidra export, dynsym,
.ARM.exidx and direct-call targets. Existing names/status/src are preserved."""
from __future__ import annotations

import json
import struct

from .disasm import disassemble
from .project import Binary, Func, path


def exidx_starts(b: Binary) -> list[int]:
    s = b.elf.section(".ARM.exidx")
    if s is None:
        return []
    out = []
    for i in range(0, s.size, 8):
        w = struct.unpack_from("<I", s.data, i)[0]
        off = w & 0x7FFFFFFF
        if off & 0x40000000:
            off -= 0x80000000
        out.append((s.addr + i + off) & 0xFFFFFFFF)
    return out


PADDING = {b"\xc0\x46", b"\x00\x00", b"\x00\xbf", b"\x00\x00\xa0\xe1", b"\x00\x00\x00\x00"}


def _split_after_terminators(b: Binary, entries: dict[int, tuple[str, str]]) -> None:
    """Split spans at code that follows an unconditional transfer and is not a branch/switch
    target of the span (gold merges identical .ARM.exidx rows, so leaf functions go missing)."""
    text = b.elf.text
    names = {v & ~1: n for n, (v, s, t) in b.elf.dynsyms.items() if t == "STT_FUNC"}
    names.update(b.elf.plt)
    added = 0
    work = sorted(entries)
    while work:
        starts = sorted(entries)
        nxt = {a: (starts[i + 1] if i + 1 < len(starts) else text.end) for i, a in enumerate(starts)}
        new = []
        for a in work:
            mode = entries[a][0]
            items = disassemble(b.elf.read(a, nxt[a] - a), a, mode, names.get)
            targets = set()
            for it in items:
                if it.branch is not None and it.mnem not in ("bl", "blx"):
                    targets.add(it.branch)
                targets.update(it.case_targets)
            after_term = False
            for it in items:
                if it.kind == "insn" and it.terminator:
                    after_term = True
                    continue
                if not after_term:
                    continue
                if it.kind != "insn" or it.raw in PADDING:
                    continue
                if it.addr not in targets and it.addr not in entries:
                    new.append(it.addr)
                    entries[it.addr] = (mode, f"FUN_{it.addr:08x}")
                break
        added += len(new)
        work = new
    if added:
        print(f"split {added} functions that follow a return inside an existing span")


def guess_mode(b: Binary, a: int) -> str:
    if a & 2:
        return "thumb"
    w = b.elf.u32(a)
    # ARM: unconditional (cond=0b1110) push/stmdb/mov/ldr/sub/... ; Thumb push {..,lr} is 0xb5xx
    if (w & 0xFF00) == 0xB500 or (w >> 28) != 0xE:
        return "thumb"
    return "arm"


def split(stem: str, ghidra_json: str | None = None) -> None:
    b = Binary(stem)
    text = b.elf.text
    ghidra_json = ghidra_json or path("build", "ghidra", stem + ".funcs.json")
    with open(ghidra_json) as fh:
        gfuncs = json.load(fh)

    entries: dict[int, tuple[str, str]] = {}  # addr -> (mode, suggested name)
    for g in gfuncs:
        a = g["entry"]
        if text.addr <= a < text.end:
            nm = g["name"]
            if nm.startswith(("FUN_", "thunk_", "LAB_")) or nm.startswith("_INIT_") or nm.startswith("_FINI_"):
                nm = f"FUN_{a:08x}"
            entries[a] = (g["mode"], nm)

    for name, (val, size, typ) in b.elf.dynsyms.items():
        if typ == "STT_FUNC" and text.addr <= (val & ~1) < text.end:
            a = val & ~1
            mode = "thumb" if val & 1 else "arm"
            old = entries.get(a)
            if old and old[0] != mode:
                print(f"warn: {name} {a:#x}: dynsym says {mode}, ghidra says {old[0]}")
            entries[a] = (mode, name)

    # Code pointers in data (R_ARM_RELATIVE / ABS32 values pointing into .text; bit0 = Thumb).
    ptr_modes: dict[int, str] = {}
    for at, r in b.elf.dynrelocs.items():
        if r.type == 23:  # R_ARM_RELATIVE
            v = b.elf.u32(at)
        elif r.type in (2, 21) and r.symval:  # ABS32 / GLOB_DAT to a local symbol
            v = r.symval
        else:
            continue
        if text.addr <= (v & ~1) < text.end:
            ptr_modes[v & ~1] = "thumb" if v & 1 else "arm"
    for a, mode in ptr_modes.items():
        if a not in entries:
            entries[a] = (mode, f"FUN_{a:08x}")
        elif entries[a][0] != mode:
            print(f"warn: {a:#x}: code pointer says {mode}, export says {entries[a][0]}")

    # .ARM.exidx function starts (mode guessed from the first word if unknown).
    for a in exidx_starts(b):
        if text.addr <= a < text.end and a not in entries:
            entries[a] = (guess_mode(b, a), f"FUN_{a:08x}")

    # Direct call targets found by sweeping known functions: catches functions Ghidra missed.
    added = 0
    for _ in range(4):
        starts = sorted(entries)
        new = {}
        for i, a in enumerate(starts):
            end = starts[i + 1] if i + 1 < len(starts) else text.end
            mode = entries[a][0]
            for it in disassemble(b.elf.read(a, end - a), a, mode):
                if it.mnem in ("bl", "blx") and it.branch is not None and text.addr <= it.branch < text.end \
                        and not (it.mnem == "bl" and mode == "arm" and it.branch & 3):
                    if it.branch not in entries and it.branch not in new:
                        tmode = mode if it.mnem == "bl" else ("arm" if mode == "thumb" else "thumb")
                        new[it.branch] = (tmode, f"FUN_{it.branch:08x}")
        if not new:
            break
        added += len(new)
        entries.update(new)
    if added:
        print(f"added {added} call targets missing from the Ghidra export")


    _split_after_terminators(b, entries)

    # Linker stubs (gold thumb->ARM PLT veneers) and unique names
    seen: dict[str, int] = {}
    for a in sorted(entries):
        mode, nm = entries[a]
        if b.elf.is_veneer(a):
            d, _ = b.elf.follow(a)
            dn = b.elf.plt.get(d) or (entries[d][1] if d in entries else f"{d:08x}")
            nm = f"__{dn}_{mode}_veneer"
        if nm in seen:
            nm = f"{nm}_{a:08x}"
        seen[nm] = a
        entries[a] = (mode, nm)

    old = {f.addr: f for f in b.funcs}
    starts = sorted(entries)
    funcs = []
    for i, a in enumerate(starts):
        end = starts[i + 1] if i + 1 < len(starts) else text.end
        mode, nm = entries[a]
        f = old.get(a)
        if f:
            f.size, f.mode = end - a, mode
            if f.status == "todo":
                f.name = nm
        else:
            f = Func(a, end - a, mode, nm, "todo")
        if b.elf.is_veneer(a) and f.status == "todo":
            f.status, f.note = "lib", "linker veneer"
        funcs.append(f)
    dropped = set(old) - set(entries)
    for a in dropped:
        print(f"warn: previously known function {old[a].name} at {a:#x} no longer an entry (kept)")
        funcs.append(old[a])
    b.funcs = funcs
    b.save()
    n_arm = sum(f.mode == "arm" for f in funcs)
    print(f"{stem}: {len(funcs)} functions ({n_arm} arm, {len(funcs) - n_arm} thumb), "
          f"{sum(f.size for f in funcs):#x} bytes of .text ({text.size:#x})")
