"""Linear-sweep disassembly of one function with literal-pool and jump-table awareness."""
from __future__ import annotations

import struct
from dataclasses import dataclass

import capstone
from capstone import arm as A

_CS = {}


def cs(mode: str) -> capstone.Cs:
    if mode not in _CS:
        c = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB if mode == "thumb" else capstone.CS_MODE_ARM)
        c.detail = True
        _CS[mode] = c
    return _CS[mode]


@dataclass
class Item:
    addr: int  # absolute (target) or section offset (candidate)
    size: int
    kind: str  # insn | word | data
    raw: bytes
    mnem: str = ""
    ops: str = ""
    insn: object = None
    pool_ref: int | None = None  # for pc-relative loads: address of the literal they read
    branch: int | None = None  # for direct branches: destination address
    case_targets: tuple = ()  # for inline switch tables: destinations
    terminator: bool = False  # unconditional control transfer (no fall-through)

    @property
    def text(self) -> str:
        if self.kind == "insn":
            return f"{self.mnem} {self.ops}".strip()
        if self.kind == "word":
            return f".word 0x{struct.unpack('<I', self.raw)[0]:08x}"
        return ".byte " + ",".join(f"0x{b:02x}" for b in self.raw)


CASE_HELPERS = {
    "__gnu_thumb1_case_sqi": 1, "__gnu_thumb1_case_uqi": 1,
    "__gnu_thumb1_case_shi": 2, "__gnu_thumb1_case_uhi": 2,
    "__gnu_thumb1_case_si": 4,
}


def disassemble(code: bytes, base: int, mode: str, call_name=None) -> list[Item]:
    """Disassemble `code` located at `base`. `call_name(dest)` names branch destinations so that
    inline switch tables after `bl __gnu_thumb1_case_*` can be skipped as data."""
    c = cs(mode)
    items: list[Item] = []
    pool: set[int] = set()
    end = base + len(code)
    pc = base
    step = 2 if mode == "thumb" else 4
    while pc < end:
        off = pc - base
        if pc in pool and pc + 4 <= end:
            items.append(Item(pc, 4, "word", code[off:off + 4]))
            pc += 4
            continue
        insn = next(c.disasm(code[off:off + 4], pc, 1), None)
        if insn is None:
            items.append(Item(pc, step, "data", code[off:off + step]))
            pc += step
            continue
        it = Item(pc, insn.size, "insn", bytes(insn.bytes), insn.mnemonic, insn.op_str, insn)
        _analyze(it, mode)
        if it.pool_ref is not None:
            pool.add(it.pool_ref)
        items.append(it)
        pc += insn.size
        if it.branch is not None and call_name is not None and it.mnem in ("bl", "blx"):
            helper = call_name(it.branch) or ""
            width = CASE_HELPERS.get(helper)
            if width:
                pc = _skip_case_table(code, base, pc, width, helper.endswith(("sqi", "shi")), items)
    return items


def _case_count(items: list[Item]) -> int | None:
    """Number of table entries: GCC guards the dispatch with `cmp rI, #N ; bhi default`."""
    for k in range(len(items) - 2, max(-1, len(items) - 9), -1):
        it = items[k]
        if it.kind != "insn":
            return None
        if it.mnem == "cmp" and it.insn.operands[1].type == A.ARM_OP_IMM:
            nxt = items[k + 1]
            if nxt.mnem in ("bhi", "bls", "bhs", "blo", "bcs", "bcc"):
                return it.insn.operands[1].imm + 1
    return None


def _skip_case_table(code: bytes, base: int, pc: int, width: int, signed: bool, items: list[Item]) -> int:
    """Inline table after `bl __gnu_thumb1_case_*` (libgcc lib1funcs.S). Byte/halfword entries are
    halfword offsets from the table start; `si` entries are byte offsets from the aligned table."""
    off = pc - base
    n = _case_count(items)
    if n is None:
        items[-1].ops += "  ; case table length unknown"
        return pc
    if width == 1:
        ents = list(struct.unpack_from(f"<{n}{'b' if signed else 'B'}", code, off))
        targets = [pc + 2 * e for e in ents]
        size = (n + 1) & ~1
    elif width == 2:
        ents = list(struct.unpack_from(f"<{n}{'h' if signed else 'H'}", code, off))
        targets = [pc + 2 * e for e in ents]
        size = 2 * n
    else:
        off4 = (off + 3) & ~3
        ents = list(struct.unpack_from(f"<{n}i", code, off4))
        targets = [base + off4 + e for e in ents]
        size = (off4 - off) + 4 * n
    size = min(size, len(code) - off)
    items.append(Item(pc, size, "data", code[off:off + size], case_targets=tuple(targets)))
    return pc + size


def _analyze(it: Item, mode: str) -> None:
    insn = it.insn
    ops = insn.operands
    uncond = insn.cc in (A.ARM_CC_AL, A.ARM_CC_INVALID)
    if insn.id in (A.ARM_INS_B, A.ARM_INS_BL, A.ARM_INS_BLX) and ops and ops[0].type == A.ARM_OP_IMM:
        it.branch = ops[0].imm
        it.terminator = uncond and insn.id == A.ARM_INS_B
        return
    if uncond:
        if insn.id == A.ARM_INS_BX:
            it.terminator = True
        elif insn.id in (A.ARM_INS_POP, A.ARM_INS_LDM, A.ARM_INS_LDMIB, A.ARM_INS_LDMDA, A.ARM_INS_LDMDB):
            it.terminator = any(o.type == A.ARM_OP_REG and o.reg == A.ARM_REG_PC for o in ops)
        elif insn.id in (A.ARM_INS_MOV, A.ARM_INS_LDR, A.ARM_INS_ADD) and ops and ops[0].type == A.ARM_OP_REG \
                and ops[0].reg == A.ARM_REG_PC:
            it.terminator = True
    if insn.id == A.ARM_INS_LDR and len(ops) >= 2 and ops[1].type == A.ARM_OP_MEM and ops[1].mem.base == A.ARM_REG_PC \
            and ops[1].mem.index == 0:
        disp = ops[1].mem.disp
        if mode == "thumb":
            it.pool_ref = ((it.addr + 4) & ~3) + disp
        else:
            it.pool_ref = it.addr + 8 + (disp if ops[1].subtracted is False else -disp)
