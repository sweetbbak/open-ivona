"""Compare a compiled candidate function (.o section + relocations) against the target binary.

A candidate matches when every byte not covered by a relocation is identical, and every
relocation, once its linker formula is inverted against the target bytes, puts the symbol
the candidate names at the place the target actually uses:
  * imports       -> the call goes (through veneers) to that import's PLT entry / GOT slot
  * named symbols -> the address recorded for that name (dynsym, symbols/*.tsv, FUN_/D_<hex>)
  * static data   -> the candidate's section/object bytes equal the target bytes found there
  * .bss statics  -> placement is recorded and must be consistent across the whole TU
Bytes after the candidate's section end must be inter-function padding.
"""
from __future__ import annotations

import difflib
import struct
from dataclasses import dataclass, field

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

from .disasm import Item, disassemble
from .project import Binary, Func

# ARM ELF relocation types emitted by GCC 4.8 for -fpic code
R_NONE, R_PC24, R_ABS32, R_REL32 = 0, 1, 2, 3
R_THM_CALL = 10
R_GOTOFF32, R_BASE_PREL, R_GOT_BREL = 24, 25, 26
R_PLT32, R_CALL, R_JUMP24, R_THM_JUMP24 = 27, 28, 29, 30
R_TARGET1, R_V4BX = 38, 40
R_GOT_PREL = 96
R_THM_JUMP11, R_THM_JUMP8 = 102, 103
BRANCH_RELOCS = {R_PC24, R_THM_CALL, R_PLT32, R_CALL, R_JUMP24, R_THM_JUMP24, R_THM_JUMP11, R_THM_JUMP8}
WORD_RELOCS = {R_ABS32, R_REL32, R_GOTOFF32, R_BASE_PREL, R_GOT_BREL, R_TARGET1, R_GOT_PREL}
RELOC_NAMES = {R_PC24: "PC24", R_ABS32: "ABS32", R_REL32: "REL32", R_THM_CALL: "THM_CALL", R_GOTOFF32: "GOTOFF",
               R_BASE_PREL: "GOTPC", R_GOT_BREL: "GOT", R_PLT32: "PLT32", R_CALL: "CALL", R_JUMP24: "JUMP24",
               R_THM_JUMP24: "THM_JUMP24", R_TARGET1: "TARGET1", R_GOT_PREL: "GOT_PREL",
               R_THM_JUMP11: "THM_JUMP11", R_THM_JUMP8: "THM_JUMP8"}
SHF_MERGE, SHF_STRINGS = 0x10, 0x20


@dataclass
class CandSym:
    name: str
    section: str
    value: int
    size: int
    bind: str
    type: str
    defined: bool


@dataclass
class CandReloc:
    offset: int
    type: int
    sym: CandSym
    addend: int  # REL: in-place addend (whole word for data relocs, 0 for branches)


@dataclass
class CandSection:
    data: bytes  # b"" for NOBITS
    size: int
    flags: int
    nobits: bool
    relocs: dict[int, "CandReloc"]  # pointers stored in the data


@dataclass
class Candidate:
    obj: str
    name: str
    mode: str
    code: bytes
    relocs: dict[int, CandReloc]
    sections: dict[str, CandSection]
    syms: list[CandSym]


@dataclass
class Row:
    t: Item | None
    c: Item | None
    ok: bool
    t_note: str = ""
    c_note: str = ""
    why: str = ""


@dataclass
class Result:
    func: Func
    rows: list[Row] = field(default_factory=list)
    problems: list[str] = field(default_factory=list)
    soft: list[str] = field(default_factory=list)  # accepted, verified only for TU consistency
    placements: dict[str, int] = field(default_factory=dict)  # candidate section -> target address
    visited: set = field(default_factory=set)  # data already compared (recursion guard)

    @property
    def score(self) -> float:
        if not self.rows:
            return 0.0
        return sum(r.ok for r in self.rows) / len(self.rows)

    @property
    def matched(self) -> bool:
        return bool(self.rows) and not self.problems and all(r.ok for r in self.rows)


# ---------------------------------------------------------------------------------------------
@dataclass
class ObjFile:
    path: str
    syms: list[CandSym]
    sections: dict[str, CandSection]

    @property
    def func_names(self) -> list[str]:
        return [x.name for x in self.syms if x.type == "STT_FUNC" and x.defined]


def load_object(obj_path: str, data: bytes | None = None) -> ObjFile:
    """Parse a relocatable object once (symbols, allocated sections and their relocations)."""
    import io
    fh = io.BytesIO(data) if data is not None else open(obj_path, "rb")
    with fh:
        elf = ELFFile(fh)
        syms = []
        symtab = elf.get_section_by_name(".symtab")
        for s in (symtab.iter_symbols() if symtab else ()):
            shndx = s["st_shndx"]
            secname = elf.get_section(shndx).name if isinstance(shndx, int) else ""
            syms.append(CandSym(s.name or secname, secname, s["st_value"], s["st_size"], s["st_info"]["bind"],
                                s["st_info"]["type"], shndx != "SHN_UNDEF"))
        sections: dict[str, CandSection] = {}
        for s in elf.iter_sections():
            if s["sh_flags"] & 2 and s["sh_type"] in ("SHT_PROGBITS", "SHT_NOBITS", "SHT_INIT_ARRAY",
                                                       "SHT_FINI_ARRAY"):
                nob = s["sh_type"] == "SHT_NOBITS"
                sections[s.name] = CandSection(b"" if nob else s.data(), s["sh_size"], s["sh_flags"], nob, {})
        for rs in elf.iter_sections():
            if not isinstance(rs, RelocationSection):
                continue
            sec = sections.get(elf.get_section(rs["sh_info"]).name)
            if sec is None:
                continue
            for r in rs.iter_relocations():
                typ = r["r_info_type"]
                if typ in (R_NONE, R_V4BX):
                    continue
                off = r["r_offset"]
                addend = struct.unpack_from("<i", sec.data, off)[0] if typ in WORD_RELOCS and not sec.nobits else 0
                sec.relocs[off] = CandReloc(off, typ, syms[r["r_info_sym"]], addend)
    return ObjFile(obj_path, syms, sections)


def candidate_from(obj: ObjFile, name: str) -> Candidate | None:
    """Extract function `name`. Alone in its section (-ffunction-sections) -> the whole section,
    which includes the literal pool; otherwise [value, value + st_size) or up to the next function."""
    func_sym = next((x for x in obj.syms if x.name == name and x.type == "STT_FUNC" and x.defined), None)
    if func_sym is None:
        return None
    home = obj.sections[func_sym.section]
    start = func_sym.value & ~1
    others = sorted(x.value & ~1 for x in obj.syms if x.section == func_sym.section and x.defined
                    and x.type == "STT_FUNC" and (x.value & ~1) != start)
    if not others:
        size = home.size - start
    else:
        nxt = [o for o in others if o > start]
        size = func_sym.size or ((nxt[0] if nxt else home.size) - start)
    code = home.data[start:start + size]
    relocs = {o - start: CandReloc(o - start, r.type, r.sym, r.addend)
              for o, r in home.relocs.items() if start <= o < start + size}
    return Candidate(obj.path, name, "thumb" if func_sym.value & 1 else "arm", code, relocs, obj.sections, obj.syms)


def load_candidate(obj_path: str, name: str, data: bytes | None = None) -> Candidate | None:
    return candidate_from(load_object(obj_path, data), name)


# ---------------------------------------------------------------------------------------------
class Matcher:
    def __init__(self, b: Binary, learn: dict | None = None):
        self.b = b
        self.elf = b.elf
        self.names = b.names
        # When given, unknown symbol names that the target resolves to an auto-named (FUN_/D_)
        # address are accepted tentatively and recorded here as name -> {address}.
        self.learn = learn

    def call_name(self, addr: int) -> str | None:
        addr, _ = self.elf.follow(addr)
        if addr in self.elf.plt:
            return self.elf.plt[addr]
        return self.names.index.exact(addr)

    def _in_text(self, a: int) -> bool:
        t = self.elf.text
        return t.addr <= (a & ~1) < t.end

    # -- target-side annotation (reading asm without a candidate) ------------------------------
    def annotate_target(self, items: list[Item], mode: str) -> dict[int, str]:
        notes: dict[int, str] = {}
        pc_bias = 4 if mode == "thumb" else 8
        anchors: dict[int, int] = {}  # pool addr -> address of the `add rX, pc` that consumes it
        for k, it in enumerate(items):
            if it.kind != "insn":
                continue
            if it.branch is not None:
                notes[it.addr] = self.names.describe(it.branch)
            if it.pool_ref is not None:
                reg = it.insn.reg_name(it.insn.operands[0].reg)
                for nx in items[k + 1:k + 10]:
                    if nx.kind != "insn":
                        continue
                    if nx.mnem == "add" and "pc" in nx.ops and nx.ops.startswith(reg + ","):
                        anchors[it.pool_ref] = nx.addr
                        break
                    if nx.mnem.startswith("ldr") and nx.ops.endswith(f"[pc, {reg}]"):
                        anchors[it.pool_ref] = nx.addr  # ldr rX, [pc, reg]: load from pc-relative address
                        break
                    if nx.ops.startswith(reg + ",") and nx.mnem not in ("str", "strh", "strb", "cmp", "tst"):
                        break  # register overwritten before any `add reg, pc`
        got = self.elf.section(".got")
        for it in items:
            if it.kind != "word":
                continue
            w = struct.unpack("<I", it.raw)[0]
            if it.addr in self.elf.dynrelocs:
                r = self.elf.dynrelocs[it.addr]
                notes[it.addr] = "&" + (r.sym or self.names.describe(w & ~1 if self._in_text(w) else w))
            elif it.addr in anchors:
                v = (w + anchors[it.addr] + pc_bias) & 0xFFFFFFFF
                if v == self.elf.got_org:
                    notes[it.addr] = "_GLOBAL_OFFSET_TABLE_ (GOT base)"
                elif got and got.addr <= v < got.end:
                    nm, local = self.elf.got_slot_value(v)
                    notes[it.addr] = f"GOT[{nm or self.names.describe(local or 0)}]"
                else:
                    notes[it.addr] = "&" + self.describe_data(v)
            else:
                v = (w + self.elf.got_org) & 0xFFFFFFFF
                sec = self.elf.section_at(v)
                if sec and sec.name not in (".text", ".plt") and 0x100 < w < 0x80000000:
                    notes[it.addr] = "GOT-relative? &" + self.describe_data(v)
                elif w < 0x10000 or w > 0xFFFF0000:
                    notes[it.addr] = f"#{w if w < 0x80000000 else w - (1 << 32)}"
        return notes

    def describe_data(self, v: int) -> str:
        sec = self.elf.section_at(v)
        d = self.names.describe(v)
        if sec and sec.name == ".rodata":
            s = self.elf.cstring(v, 80)
            if s is not None and len(s) >= 2 and all(32 <= c < 127 or c in (9, 10, 13) for c in s):
                return f'{d} "{s.decode("latin1")}"'
        return d

    # -- comparison ----------------------------------------------------------------------------
    def compare(self, f: Func, cand: Candidate) -> Result:
        res = Result(f)
        n = len(cand.code)
        if cand.mode != f.mode:
            res.problems.append(f"mode mismatch: target is {f.mode}, candidate compiled as {cand.mode}")
        tail = self.elf.read(f.addr + n, f.size - n) if n < f.size else b""
        tail_is_code = bool(tail) and not _is_padding(tail)
        if n > f.size:
            res.problems.append(f"candidate is {n:#x} bytes, target function span is only {f.size:#x}")
        if tail_is_code:
            res.problems.append(f"candidate ends at {n:#x} but target continues to {f.size:#x}")
        tlen = f.size if tail_is_code else min(n, f.size)
        t_items = disassemble(self.elf.read(f.addr, tlen), f.addr, f.mode, self.call_name)
        c_items = self._disasm_candidate(cand)
        t_notes = self.annotate_target(t_items, f.mode)

        if [(i.addr - f.addr, i.size) for i in t_items] == [(i.addr, i.size) for i in c_items]:
            for ti, ci in zip(t_items, c_items):
                ok, why, cnote = self._compare_item(f, cand, ti, ci, res)
                res.rows.append(Row(ti, ci, ok, t_notes.get(ti.addr, ""), cnote, why))
        else:
            self._diff_rows(f, cand, t_items, c_items, t_notes, res)
        return res

    def _disasm_candidate(self, cand: Candidate) -> list[Item]:
        """In a REL object a call to an undefined symbol branches to (roughly) itself, so name
        branch destinations by the relocation sitting on each call instruction."""
        first = disassemble(cand.code, 0, cand.mode)
        dest_names = {}
        for it in first:
            r = cand.relocs.get(it.addr)
            if r is not None and it.branch is not None:
                dest_names[it.branch] = r.sym.name
        return disassemble(cand.code, 0, cand.mode, dest_names.get)

    def _compare_item(self, f: Func, cand: Candidate, ti: Item, ci: Item, res: Result) -> tuple[bool, str, str]:
        inner = [r for o, r in cand.relocs.items() if ci.addr <= o < ci.addr + ci.size]
        if not inner:
            return (True, "", "") if ti.raw == ci.raw else (False, "bytes differ", "")
        ok, whys, notes = True, [], []
        for r in inner:
            rok, why, note = self._check_reloc(f, cand, r, res)
            ok &= rok
            notes.append(note)
            if why:
                whys.append(why)
        if any(r.type in BRANCH_RELOCS for r in inner):
            if _branch_kind(ti) != _branch_kind(ci):
                ok = False
                whys.append("different branch instruction")
        elif _mask(ti.raw, ci.addr, inner) != _mask(ci.raw, ci.addr, inner):
            ok = False
            whys.append("bytes differ")
        return ok, "; ".join(whys), ", ".join(notes)

    def _check_reloc(self, f: Func, cand: Candidate, r: CandReloc, res: Result) -> tuple[bool, str, str]:
        """Invert the relocation formula on the target bytes to find where the target put the
        candidate's symbol (S_t), then check that is the right entity."""
        P = f.addr + r.offset
        sym = r.sym
        note = f"{RELOC_NAMES.get(r.type, r.type)} {sym.name}"
        if r.type in BRANCH_RELOCS:
            t = disassemble(self.elf.read(P, 4), P, f.mode)[0]
            if t.branch is None:
                return False, "target has no branch here", note
            return (*self._expect(cand, sym, t.branch, res, call=True), note)
        W = self.elf.u32(P)
        A = r.addend
        if r.type in (R_GOT_PREL, R_GOT_BREL):
            slot = (W - A + P) & 0xFFFFFFFF if r.type == R_GOT_PREL else (W + self.elf.got_org) & 0xFFFFFFFF
            got = self.elf.section(".got")
            if not got or not (got.addr <= slot < got.end):
                return False, f"GOT slot {slot:#x} is outside .got", note
            nm, local = self.elf.got_slot_value(slot)
            if nm is not None and nm == sym.name:
                return True, "", note
            if local is not None:
                return (*self._expect(cand, sym, local, res), note)
            return False, f"target GOT slot holds {nm}", note
        if r.type == R_BASE_PREL:
            if sym.name != "_GLOBAL_OFFSET_TABLE_":
                return False, f"GOTPC against {sym.name}", note
            ok = (self.elf.got_org + A - P) & 0xFFFFFFFF == W
            return ok, "" if ok else "GOT base offset differs", note
        if r.type == R_GOTOFF32:
            return (*self._expect(cand, sym, (W - A + self.elf.got_org) & 0xFFFFFFFF, res), note)
        if r.type == R_REL32:
            return (*self._expect(cand, sym, (W - A + P) & 0xFFFFFFFF, res), note)
        if r.type in (R_ABS32, R_TARGET1):
            dr = self.elf.dynrelocs.get(P)
            if dr is None:
                return False, "target has no dynamic relocation on this absolute word", note
            if dr.type == 23:  # RELATIVE: word holds S+A
                return (*self._expect(cand, sym, (W - A) & 0xFFFFFFFF, res), note)
            ok = dr.sym == sym.name and W == A
            return ok, "" if ok else f"target word is &{dr.sym}+{W:#x}", note
        return False, f"unsupported relocation type {r.type}", note

    def _expect(self, cand: Candidate, sym: CandSym, s_t: int, res: Result, call: bool = False) -> tuple[bool, str]:
        """Is the candidate's `sym` located at target address `s_t`?"""
        if call or self._in_text(s_t):
            s_t, _ = self.elf.follow(s_t)
            if s_t in self.elf.plt:
                imp = self.elf.plt[s_t]
                return (imp == sym.name, "" if imp == sym.name else f"target calls {imp}@plt")
            if self._in_text(s_t):
                s_t &= ~1
        if sym.type == "STT_SECTION":
            return self._expect_section(cand, sym.section, s_t, res)
        want = self.names.addr_of(sym.name)
        if want is not None:
            if want == s_t:
                return True, ""
            return False, f"{sym.name} is {want:#x} but target uses {self.names.describe(s_t)}"
        if sym.defined and sym.bind == "STB_LOCAL" and sym.type != "STT_FUNC":
            return self._expect_local(cand, sym, s_t, res)
        if sym.name in self.elf.imports:
            return False, f"{sym.name} is an import but target uses {self.names.describe(s_t)}"
        if self.learn is not None and _is_auto(self.names.describe(s_t)):
            self.learn.setdefault(sym.name, set()).add(s_t)
            return True, ""
        return False, f"unknown name {sym.name}; target uses {self.names.describe(s_t)}"

    def _place(self, res: Result, section: str, base: int) -> tuple[bool, str]:
        prev = res.placements.setdefault(section, base)
        if prev != base:
            return False, f"{section} placed at both {prev:#x} and {base:#x}"
        want = output_section(section)
        got = self.elf.section_at(base)
        if want and got and got.name != want:
            hint = " (const-ness differs?)" if {want, got.name} & {".data", ".data.rel.ro.local", ".data.rel.ro",
                                                                    ".rodata"} else ""
            return False, f"candidate {section} would link into {want}, target has it in {got.name}{hint}"
        return True, ""

    def _expect_section(self, cand: Candidate, section: str, s_t: int, res: Result) -> tuple[bool, str]:
        """Section-relative reference: s_t is where the whole input section landed."""
        sec = cand.sections.get(section)
        if sec is None:
            return False, f"reference to unknown section {section}"
        if section.startswith(".text"):
            fn = next((s for s in cand.syms if s.section == section and s.type == "STT_FUNC"), None)
            want = self.names.addr_of(fn.name) if fn else None
            return (want == s_t, "" if want == s_t else f"{section} is not at {s_t:#x}")
        ok, why = self._place(res, section, s_t)
        if not ok:
            return ok, why
        if sec.flags & SHF_MERGE:
            res.soft.append(f"{section} (merged) assumed at {s_t:#x}")
            return True, ""
        return self._compare_bytes(cand, sec, 0, sec.size, s_t, section, res)

    def _expect_local(self, cand: Candidate, sym: CandSym, s_t: int, res: Result) -> tuple[bool, str]:
        sec = cand.sections.get(sym.section)
        if sec is None:
            return False, f"{sym.name} in unknown section {sym.section}"
        if sec.flags & SHF_STRINGS:
            end = sec.data.find(b"\0", sym.value)
            cs, ts = sec.data[sym.value:end], self.elf.cstring(s_t)
            return (cs == ts, "" if cs == ts else f"string differs: target {ts!r}, candidate {cs!r}")
        if not sec.flags & SHF_MERGE:
            ok, why = self._place(res, sym.section, s_t - sym.value)
            if not ok:
                return ok, why
        size = sym.size or (sec.size - sym.value)
        return self._compare_bytes(cand, sec, sym.value, size, s_t, sym.name, res)

    def _compare_bytes(self, cand: Candidate, sec: CandSection, off: int, size: int, s_t: int, what: str,
                       res: Result) -> tuple[bool, str]:
        key = (id(sec), off, s_t)
        if key in res.visited:
            return True, ""
        res.visited.add(key)
        tsec = self.elf.section_at(s_t)
        if tsec is None or s_t + size > tsec.end:
            return False, f"{what}: {size:#x} bytes at {s_t:#x} are outside any target section"
        if sec.nobits:
            if tsec.data is not None:
                return False, f"{what} is .bss in the candidate but {s_t:#x} is in {tsec.name}"
            res.soft.append(f"{what} (.bss) placed at {s_t:#x}")
            return True, ""
        cb = bytearray(sec.data[off:off + size])
        tb = bytearray(self.elf.read(s_t, size))
        for ro, r in sec.relocs.items():  # pointers inside the data
            if off <= ro < off + size:
                cb[ro - off:ro - off + 4] = tb[ro - off:ro - off + 4] = b"\0\0\0\0"
                ok, why = self._check_data_pointer(cand, r, s_t + (ro - off), res)
                if not ok:
                    return False, f"{what}+{ro - off:#x}: {why}"
        if cb == tb:
            return True, ""
        return False, f"{what}: candidate data differs from target at {s_t:#x}"

    def _check_data_pointer(self, cand: Candidate, r: CandReloc, at: int, res: Result) -> tuple[bool, str]:
        """A pointer word inside referenced data: the target must hold a dynamic relocation that
        points at the same entity (checked like any other symbol reference)."""
        if r.type not in (R_ABS32, R_TARGET1):
            return False, f"unsupported data relocation type {r.type}"
        dr = self.elf.dynrelocs.get(at)
        if dr is None:
            return False, "target has no dynamic relocation here"
        if dr.type != 23:
            return (dr.sym == r.sym.name, "" if dr.sym == r.sym.name else f"target points to {dr.sym}")
        w = self.elf.u32(at)
        sec = cand.sections.get(r.sym.section)
        if r.sym.type == "STT_SECTION" and sec is not None and sec.flags & SHF_STRINGS:
            # pointer into a merged string section: the linker may share/tail-merge strings,
            # so compare the string itself (S+A is exact for data relocations)
            end = sec.data.find(b"\0", r.addend)
            cs, ts = sec.data[r.addend:end], self.elf.cstring(w)
            return (cs == ts, "" if cs == ts else f"string differs: target {ts!r}, candidate {cs!r}")
        return self._expect(cand, r.sym, (w - r.addend) & 0xFFFFFFFF, res)

    def _diff_rows(self, f, cand, t_items, c_items, t_notes, res):
        def key(it: Item) -> str:
            return it.mnem if it.kind == "insn" else it.kind
        sm = difflib.SequenceMatcher(a=[key(i) for i in t_items], b=[key(i) for i in c_items], autojunk=False)
        for op, a0, a1, b0, b1 in sm.get_opcodes():
            if op == "equal":
                for ti, ci in zip(t_items[a0:a1], c_items[b0:b1]):
                    if ti.addr - f.addr == ci.addr:
                        ok, why, cn = self._compare_item(f, cand, ti, ci, res)
                    else:
                        ok, why, cn = ti.raw == ci.raw, "shifted", ""
                    res.rows.append(Row(ti, ci, ok, t_notes.get(ti.addr, ""), cn, why))
            else:
                ts, cs_ = t_items[a0:a1], c_items[b0:b1]
                for k in range(max(len(ts), len(cs_))):
                    ti = ts[k] if k < len(ts) else None
                    ci = cs_[k] if k < len(cs_) else None
                    res.rows.append(Row(ti, ci, False, t_notes.get(ti.addr, "") if ti else "", "", op))


def _is_auto(desc: str) -> bool:
    return desc.startswith(("FUN_", "D_")) and "+" not in desc


def output_section(name: str) -> str | None:
    """Output section the linker (gold, default script) puts an input section into."""
    for prefix, out in ((".text", ".text"), (".rodata", ".rodata"), (".data.rel.ro.local", ".data.rel.ro.local"),
                        (".data.rel.ro", ".data.rel.ro"), (".data", ".data"), (".bss", ".bss"),
                        (".init_array", ".init_array"), (".fini_array", ".fini_array")):
        if name == prefix or name.startswith(prefix + "."):
            return out
    return None


def _branch_kind(it: Item) -> str:
    if it.kind != "insn":
        return it.kind
    if it.mnem in ("bl", "blx"):
        return "call"  # the linker turns BL into BLX for ARM<->Thumb interworking
    return it.mnem


def _mask(raw: bytes, base: int, relocs: list[CandReloc]) -> bytes:
    b = bytearray(raw)
    for r in relocs:
        o = r.offset - base
        b[o:o + 4] = b"\0\0\0\0"
    return bytes(b)


def _is_padding(b: bytes) -> bool:
    return len(b) % 2 == 0 and all(b[i:i + 2] in (b"\xc0\x46", b"\0\0") for i in range(0, len(b), 2))


def run(b: Binary, f: Func, obj: str, name: str | None = None) -> Result:
    cand = load_candidate(obj, name or f.name)
    if cand is None:
        r = Result(f)
        r.problems.append(f"{name or f.name} is not defined in {obj}")
        return r
    return Matcher(b).compare(f, cand)
