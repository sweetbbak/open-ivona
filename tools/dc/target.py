"""Model of a linked target ELF (.so): bytes by address, PLT/GOT/dynamic-reloc resolution."""
from __future__ import annotations

import bisect
import struct
from dataclasses import dataclass, field
from functools import cached_property

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

R_ARM_ABS32 = 2
R_ARM_GLOB_DAT = 21
R_ARM_JUMP_SLOT = 22
R_ARM_RELATIVE = 23


@dataclass
class Section:
    name: str
    addr: int
    size: int
    data: bytes | None  # None for NOBITS
    flags: int

    @property
    def end(self) -> int:
        return self.addr + self.size


@dataclass
class DynReloc:
    type: int
    sym: str | None  # symbol name for GLOB_DAT/ABS32/JUMP_SLOT
    symval: int  # resolved value if defined locally (0 if import)


@dataclass
class TargetElf:
    path: str
    sections: list[Section] = field(default_factory=list)
    dynsyms: dict[str, tuple[int, int, str]] = field(default_factory=dict)  # name -> (value, size, type)
    imports: set[str] = field(default_factory=set)
    dynrelocs: dict[int, DynReloc] = field(default_factory=dict)  # address -> reloc
    plt: dict[int, str] = field(default_factory=dict)  # plt entry addr -> import name
    got_org: int = 0  # _GLOBAL_OFFSET_TABLE_ (DT_PLTGOT)

    @classmethod
    def load(cls, path: str) -> "TargetElf":
        t = cls(path)
        with open(path, "rb") as fh:
            elf = ELFFile(fh)
            for s in elf.iter_sections():
                if not s["sh_flags"] & 2:  # SHF_ALLOC
                    continue
                data = None if s["sh_type"] == "SHT_NOBITS" else s.data()
                t.sections.append(Section(s.name, s["sh_addr"], s["sh_size"], data, s["sh_flags"]))
            t.sections.sort(key=lambda s: s.addr)
            dynsym = elf.get_section_by_name(".dynsym")
            for sym in dynsym.iter_symbols():
                if not sym.name:
                    continue
                if sym["st_shndx"] == "SHN_UNDEF":
                    t.imports.add(sym.name)
                else:
                    t.dynsyms[sym.name] = (sym["st_value"], sym["st_size"], sym["st_info"]["type"])
            for dyn in elf.iter_segments():
                if dyn["p_type"] == "PT_DYNAMIC":
                    for tag in dyn.iter_tags():
                        if tag.entry.d_tag == "DT_PLTGOT":
                            t.got_org = tag.entry.d_val
            jump_slots: list[tuple[int, str]] = []
            for rs in elf.iter_sections():
                if not isinstance(rs, RelocationSection):
                    continue
                symtab = elf.get_section(rs["sh_link"])
                for r in rs.iter_relocations():
                    typ = r["r_info_type"]
                    name, val = None, 0
                    if r["r_info_sym"]:
                        sym = symtab.get_symbol(r["r_info_sym"])
                        name = sym.name
                        val = sym["st_value"] if sym["st_shndx"] != "SHN_UNDEF" else 0
                    t.dynrelocs[r["r_offset"]] = DynReloc(typ, name, val)
                    if typ == R_ARM_JUMP_SLOT:
                        jump_slots.append((r["r_offset"], name))
        t._decode_plt(jump_slots)
        return t

    def _decode_plt(self, jump_slots: list[tuple[int, str]]) -> None:
        """Map each PLT entry to its import by decoding the GOT slot it jumps through (gold ARM PLT)."""
        plt = self.section(".plt")
        if plt is None:
            return
        slot_name = dict(jump_slots)
        # gold: 20-byte header, then 12-byte entries: add ip,pc,#X ; add ip,ip,#Y ; ldr pc,[ip,#Z]!
        a = plt.addr + 20
        while a + 12 <= plt.end:
            i0, i1, i2 = struct.unpack_from("<3I", plt.data, a - plt.addr)
            if (i0 & 0xFFFFF000) != 0xE28FC000 or (i1 & 0xFFFFF000) != 0xE28CC000 or (i2 & 0xFFFFF000) != 0xE5BCF000:
                raise ValueError(f"unexpected PLT encoding at {a:#x}")
            slot = a + 8 + _arm_imm(i0 & 0xFFF) + _arm_imm(i1 & 0xFFF) + (i2 & 0xFFF)
            self.plt[a] = slot_name[slot]
            a += 12

    # ---- address-space helpers -------------------------------------------------
    def section(self, name: str) -> Section | None:
        return next((s for s in self.sections if s.name == name), None)

    def section_at(self, addr: int) -> Section | None:
        for s in self.sections:
            if s.addr <= addr < s.end:
                return s
        return None

    def read(self, addr: int, n: int) -> bytes:
        s = self.section_at(addr)
        if s is None or addr + n > s.end:
            raise ValueError(f"read {addr:#x}+{n} outside a section")
        if s.data is None:
            return bytes(n)
        return s.data[addr - s.addr: addr - s.addr + n]

    def u32(self, addr: int) -> int:
        return struct.unpack("<I", self.read(addr, 4))[0]

    def cstring(self, addr: int, limit: int = 4096) -> bytes | None:
        s = self.section_at(addr)
        if s is None or s.data is None:
            return None
        raw = s.data[addr - s.addr: addr - s.addr + limit]
        i = raw.find(b"\0")
        return raw[:i] if i >= 0 else None

    def got_slot_value(self, slot: int) -> tuple[str | None, int | None]:
        """What a GOT slot points to: (import/symbol name, local address or None)."""
        r = self.dynrelocs.get(slot)
        if r is None:
            return None, self.u32(slot)  # statically resolved by gold
        if r.type == R_ARM_RELATIVE:
            return None, self.u32(slot)
        if r.type in (R_ARM_GLOB_DAT, R_ARM_ABS32, R_ARM_JUMP_SLOT):
            return r.sym, (r.symval or None)
        return None, None

    def veneer_dest(self, a: int) -> int | None:
        """Destination of a gold-generated veneer at `a`, or None if `a` is not a veneer.
        Patterns: Thumb `bx pc; nop` (falls into ARM code at a+4); ARM PIC long-branch
        `ldr ip,[pc]; add pc,ip,pc; .word off`; ARM->Thumb PIC `ldr ip,[pc,#4]; add ip,pc,ip;
        bx ip; .word off`; ARM `ldr pc,[pc,#-4]; .word abs`."""
        try:
            w = self.read(a, 16) if self.section_at(a + 15) else self.read(a, 12)
        except ValueError:
            return None
        if w[:4] == b"\x78\x47\xc0\x46":
            return a + 4
        i0, i1, i2 = struct.unpack_from("<3I", w)
        if i0 == 0xE59FC000 and i1 == 0xE08CF00F:
            return (a + 12 + i2) & 0xFFFFFFFF
        if i0 == 0xE59FC004 and i1 == 0xE08FC00C and i2 == 0xE12FFF1C and len(w) == 16:
            return (a + 12 + struct.unpack_from("<I", w, 12)[0]) & 0xFFFFFFFE
        if i0 == 0xE51FF004:
            return i1 & ~1
        return None

    def follow(self, a: int) -> tuple[int, bool]:
        """Follow veneers to the real destination. Returns (address, went_through_veneer)."""
        hops = 0
        while hops < 4:
            d = self.veneer_dest(a)
            if d is None:
                break
            a, hops = d, hops + 1
        return a, hops > 0

    def is_veneer(self, a: int) -> bool:
        return self.veneer_dest(a) is not None

    @cached_property
    def text(self) -> Section:
        return self.section(".text")


def _arm_imm(imm12: int) -> int:
    rot = (imm12 >> 8) * 2
    v = imm12 & 0xFF
    return ((v >> rot) | (v << (32 - rot))) & 0xFFFFFFFF if rot else v


class AddrIndex:
    """Sorted address -> name lookup with containment (for 'sym+off' rendering)."""

    def __init__(self, items: dict[int, tuple[str, int]]):
        self.items = items  # addr -> (name, size)
        self.keys = sorted(items)

    def exact(self, addr: int) -> str | None:
        v = self.items.get(addr)
        return v[0] if v else None

    def containing(self, addr: int) -> tuple[str, int] | None:
        i = bisect.bisect_right(self.keys, addr) - 1
        if i < 0:
            return None
        base = self.keys[i]
        name, size = self.items[base]
        if addr == base or addr < base + max(size, 1):
            return name, addr - base
        return None
