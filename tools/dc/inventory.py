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


NORETURN = {"exit", "abort", "_exit", "__stack_chk_fail", "longjmp", "siglongjmp", "__assert2",
            "__cxa_throw", "__cxa_rethrow", "_Unwind_Resume", "__cxa_end_cleanup", "_ZSt9terminatev"}


def _calls_noreturn(b: Binary, it, names) -> bool:
    if it.kind != "insn" or it.mnem not in ("bl", "blx") or it.branch is None:
        return False
    dest, _ = b.elf.follow(it.branch)
    return names.get(dest) in NORETURN


def _split_after_terminators(b: Binary, entries: dict[int, tuple[str, str]]) -> None:
    """Split spans at code that follows an unconditional transfer and is not a branch/switch
    target anywhere in .text (gold merges identical .ARM.exidx rows, so leaf functions go
    missing). Verified (lib/matched) functions are never split. Splitting does not change how
    code decodes, so targets are collected once and only the new tail spans are rescanned."""
    text = b.elf.text
    names = {v & ~1: n for n, (v, s, t) in b.elf.dynsyms.items() if t == "STT_FUNC"}
    names.update(b.elf.plt)
    protected = {f.addr for f in b.funcs if f.status in ("lib", "matched")}
    starts = sorted(entries)
    nxt = {a: (starts[i + 1] if i + 1 < len(starts) else text.end) for i, a in enumerate(starts)}
    spans = {a: disassemble(b.elf.read(a, nxt[a] - a), a, entries[a][0], names.get) for a in starts}
    targets = set()
    for items in spans.values():
        for k, it in enumerate(items):
            if it.branch is not None and it.mnem not in ("bl", "blx"):
                targets.add(it.branch)
            targets.update(it.case_targets)
            if it.kind == "insn" and it.mnem.startswith("add") and it.ops.startswith("pc, pc,"):
                # ARM switch: `addls pc, pc, rN, lsl #2` then `b default` and a table of `b case`
                for nx in items[k + 1:]:
                    if nx.kind != "insn" or nx.mnem != "b":
                        break
                    targets.add(nx.addr)
            if it.kind == "insn" and it.mnem.startswith("ldr") and it.ops.startswith("pc, [pc,"):
                # ARM switch: `ldrls pc, [pc, rN, lsl #2]` then a table of absolute addresses
                for nx in items[k + 1:]:
                    if nx.kind != "word" and not (nx.kind == "insn" and nx.addr in b.elf.dynrelocs):
                        break
                    w = b.elf.u32(nx.addr)
                    if not (text.addr <= w & ~1 < text.end):
                        break
                    targets.add(w & ~1)
    added = 0
    work = [(a, spans[a]) for a in starts if a not in protected]
    while work:
        a, items = work.pop()
        after_term = False
        for k, it in enumerate(items):
            if after_term and it.kind == "insn" and it.raw not in PADDING:
                if it.addr not in targets and it.addr not in entries:
                    entries[it.addr] = (entries[a][0], f"FUN_{it.addr:08x}")
                    added += 1
                    work.append((it.addr, items[k:]))  # same decoding, now its own span
                    break
                after_term = False  # reachable code: keep scanning
            if it.kind == "insn" and (it.terminator or _calls_noreturn(b, it, names)):
                after_term = True
    if added:
        print(f"split {added} functions that follow a return inside an existing span")


def _add_pcrel_code_pointers(b: Binary, entries: dict[int, tuple[str, str]]) -> None:
    """Static functions whose address is taken (callbacks) are materialised PC-relatively:
    `ldr rX, =f-(.LPIC+8); add rX, pc, rX`. Add those targets as entries (bit0 = Thumb)."""
    text = b.elf.text
    pc_bias = {"arm": 8, "thumb": 4}
    added = 0
    for _ in range(3):
        starts = sorted(entries)
        new = {}
        for i, a in enumerate(starts):
            end = starts[i + 1] if i + 1 < len(starts) else text.end
            mode = entries[a][0]
            items = disassemble(b.elf.read(a, end - a), a, mode)
            words = {it.addr: it for it in items if it.kind == "word"}
            for k, it in enumerate(items):
                if it.pool_ref is None or it.pool_ref not in words:
                    continue
                reg = it.insn.reg_name(it.insn.operands[0].reg)
                for nx in items[k + 1:k + 10]:
                    if nx.kind != "insn":
                        continue
                    if nx.mnem == "add" and nx.ops.replace(" ", "") in (f"{reg},pc", f"{reg},pc,{reg}"):
                        w = int.from_bytes(words[it.pool_ref].raw, "little")
                        v = (w + nx.addr + pc_bias[mode]) & 0xFFFFFFFF
                        t = v & ~1
                        if text.addr <= t < text.end and t not in entries and t not in new \
                                and (v & 1 or t % 4 == 0) and t != a:
                            new[t] = ("thumb" if v & 1 else "arm", f"FUN_{t:08x}")
                        break
                    if nx.ops.startswith(reg + ",") and nx.mnem not in ("str", "strh", "strb", "cmp", "tst"):
                        break
        if not new:
            break
        entries.update(new)
        added += len(new)
    if added:
        print(f"added {added} functions referenced through PC-relative code pointers")


def _looks_like_arm_entry(b: Binary, a: int) -> bool:
    """Heuristic for tail-call targets: ARM code that starts with a prologue or directly
    follows an unconditional return/branch."""
    if a & 3 or not (b.elf.text.addr < a < b.elf.text.end):
        return False
    w = b.elf.u32(a)
    if (w & 0xFFFF0000) == 0xE92D0000 or w == 0xE1A0C00D:  # push/stmfd sp!,{..} ; mov ip, sp
        return True
    prev = b.elf.u32(a - 4)
    return prev in (0xE12FFF1E,) or (prev & 0xFFFF8000) == 0xE8BD8000 or (prev & 0xFF000000) == 0xEA000000


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
    for f in b.funcs:  # known entries are kept (they may come from earlier passes or by hand)
        entries[f.addr] = (f.mode, f.name)
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
                tail_call = (mode == "arm" and it.mnem == "b" and it.branch is not None
                             and not (a <= it.branch < end) and _looks_like_arm_entry(b, it.branch))
                if (it.mnem in ("bl", "blx") or tail_call) and it.branch is not None \
                        and text.addr <= it.branch < text.end \
                        and not (it.mnem == "bl" and mode == "arm" and it.branch & 3):
                    if it.branch not in entries and it.branch not in new:
                        tmode = ("arm" if mode == "thumb" else "thumb") if it.mnem == "blx" else mode
                        new[it.branch] = (tmode, f"FUN_{it.branch:08x}")
        if not new:
            break
        added += len(new)
        entries.update(new)
    if added:
        print(f"added {added} call targets missing from the Ghidra export")


    _split_after_terminators(b, entries)
    _add_pcrel_code_pointers(b, entries)

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
            if f.status == "todo" and f.name.startswith("FUN_") and not nm.startswith("FUN_"):
                f.name = nm  # only upgrade auto names; never drop learned/hand-given names
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
