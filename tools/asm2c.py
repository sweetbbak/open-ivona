#!/usr/bin/env python3
"""Lift an ARM function of a binary to goto-style C over register variables: a functional
stand-in for a function not decompiled yet, never a matching source.

  tools/asm2c.py BINARY FUNC [OUT.c]     (default build/gen/<bin>/lifted_<func>.c)

Used by tools/build_runnable.py for the functions a TARGETS entry lists as "lifted" (big
hand-written rule chains such as en_word_pos). Registers are uint32_t locals and the flags are
the last compare's operands. Calls pass r0..r3 and the outgoing stack words, as many as the
callee takes (ARITY). An address formed pc-relatively, or as an offset from one (constants are
propagated over the CFG), is emitted as a C expression for what the binary holds there: a
string literal, a NULL-terminated string list copied out as wl_<addr>, or a symbol (GOT slots
by their relocation). Words loaded from such an address (ldm, memcpy) are emitted the same way.
Anything the lifter cannot express exactly stops it with an error rather than guessing.
The output is rewritten only when it changes.
"""
import os
import re
import struct
import subprocess
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
from dc.disasm import disassemble  # noqa: E402
from dc.project import Binary, path  # noqa: E402

# argument words each callee takes (item_traverse is variadic: a 0-terminated path)
ARITY = {
    'item_traverse': 7, 'ffeature_string': 2, 'strcmp': 2, 'cst_member_string': 2,
    'cst_regex_match': 2, 'item_parent': 1, 'item_as': 2, 'item_prev': 1, 'item_next': 1,
    'phrase_contains_word': 2, 'item_feat_string': 2, 'item_daughter': 1, 'strstr': 2,
    'strlen': 1, 'phrase_start_token': 2, 'item_last_daughter': 1, 'cst_startswith': 2,
    'cst_endswith': 2, 'en_lex_select_pos': 3, 'val_string': 1, 'item_feat': 2,
    'cst_startswith_any': 2, 'cst_endswith_any': 2, 'phrase_end_token': 2, 'memcpy': 3,
    'cst_strrchr': 2,
}
LIBC = {'strcmp': '(const char *)(a), (const char *)(b)', 'strlen': '(const char *)(a)',
        'strstr': '(const char *)(a), (const char *)(b)',
        'memcpy': '(void *)(a), (const void *)(b), (size_t)(c)'}
CONDS = {'eq': '(fa == fb)', 'ne': '(fa != fb)', 'ls': '(fa <= fb)', 'hi': '(fa > fb)',
         'lo': '(fa < fb)', 'hs': '(fa >= fb)', 'lt': '((int32_t)fa < (int32_t)fb)',
         'ge': '((int32_t)fa >= (int32_t)fb)', 'gt': '((int32_t)fa > (int32_t)fb)',
         'le': '((int32_t)fa <= (int32_t)fb)'}
RN = {'sb': 'r9', 'sl': 'r10', 'fp': 'r11'}
CONDITIONABLE = ('b', 'mov', 'add', 'sub', 'ldr')


class LiftError(Exception):
    pass


def reg(r):
    return RN.get(r, r)


def imm(s):
    return int(s.lstrip('#'), 0) & 0xffffffff


def c_lit(s):
    out = '"'
    for ch in s:
        o = ord(ch)
        if ch in '"\\':
            out += '\\' + ch
        elif 32 <= o < 127:
            out += ch
        else:
            out += f'\\{o:03o}'
    return out + '"'


class Lifter:
    def __init__(self, b: Binary, fname: str):
        self.b = b
        self.f = b.func(fname)
        self.names = {x.addr: x.name for x in b.funcs}
        self.names.update({d.addr: d.name for d in b.data})
        self.names.update({a: n.replace('@plt', '') for a, n in b.elf.plt.items()})
        self.rel = {}
        out = subprocess.run(['readelf', '-r', '-W', path('target', b.stem + '.so')],
                             capture_output=True, text=True, check=True).stdout
        for line in out.splitlines():
            p = line.split()
            if len(p) >= 3 and re.fullmatch(r'[0-9a-f]{8}', p[0]):
                self.rel[int(p[0], 16)] = (p[2], p[4] if len(p) > 4 else '')
        f = self.f
        self.items = {it.addr: it for it in disassemble(b.elf.read(f.addr, f.size), f.addr, f.mode,
                                                        lambda a: self.names.get(a))
                      if it.kind == 'insn'}
        self.lists = {}    # addr -> strings
        self.blocks = {}   # (addr, size) -> initializer
        self.syms = set()  # data symbols referenced
        self.calls = set()

    def w(self, a):
        return struct.unpack('<I', self.b.elf.read(a, 4))[0]

    def readonly(self, a):
        """Whether the word at a is a constant: read-only data, or an anonymous initializer
        template in .data (GCC copies local arrays' initial values from there)."""
        s = self.sec(a) or ''
        if s.startswith(('.rodata', '.data.rel.ro')):
            return True
        return s == '.data' and not any(d.addr <= a < d.addr + max(d.size, 4) for d in self.b.data)

    def sec(self, a):
        s = self.b.elf.section_at(a)
        return s.name if s is not None else None

    # ---- decoding ----
    def ops(self, it):
        return [o.strip() for o in re.split(r',(?![^\[]*\])', it.insn.op_str)]

    def split_cond(self, mn):
        for c in CONDS:
            if mn.endswith(c) and mn[:-2] in CONDITIONABLE:
                return mn[:-2], c
        return mn, None

    def pcrel(self, a, op):
        """The word an `ldr rX, [pc, #off]` at a loads."""
        return self.w(a + 8 + int((op[4:-1].strip(', ') or '0').lstrip('#'), 0))

    def reglist(self, it):
        return [reg(x.strip()) for x in it.insn.op_str.split('{')[1].rstrip('}').split(',')]

    def succs(self, a):
        it = self.items[a]
        mn, c = self.split_cond(it.insn.mnemonic)
        if mn == 'b':
            t = imm(self.ops(it)[0])
            return [t] if c is None else [t, a + 4]
        if mn == 'pop' and 'pc' in it.insn.op_str:
            return []
        return [a + 4]

    # ---- constant propagation: ('i', n) int, ('a', addr) address, ('s', off) stack address,
    # ('g', slot) GOT slot contents, ('w', word) a word read from the binary ----
    def step(self, a, st):
        st = dict(st)
        it = self.items[a]
        mn, c = self.split_cond(it.insn.mnemonic)
        o = self.ops(it)
        d = reg(o[0]) if o else None
        val = None
        if mn == 'ldr' and o[1].startswith('[pc'):
            val = ('i', self.pcrel(a, o[1]))
        elif mn == 'add' and len(o) == 3 and o[1] == 'pc' and st.get(reg(o[2]), ('',))[0] == 'i':
            val = ('a', (st[reg(o[2])][1] + a + 8) & 0xffffffff)
        elif mn == 'add' and len(o) == 3 and o[1] == 'sp' and o[2].startswith('#'):
            val = ('s', imm(o[2]))
        elif mn in ('add', 'sub') and len(o) == 3 and o[2].startswith('#') and reg(o[1]) in st:
            k, v = st[reg(o[1])]
            val = (k, (v + imm(o[2]) if mn == 'add' else v - imm(o[2])) & 0xffffffff)
        elif mn == 'add' and len(o) == 3 and reg(o[1]) in st and reg(o[2]) in st:
            (k1, v1), (k2, v2) = st[reg(o[1])], st[reg(o[2])]
            if {k1, k2} <= {'i', 'a'}:
                val = ('a' if 'a' in (k1, k2) else 'i', (v1 + v2) & 0xffffffff)
        elif mn == 'mov' and o[1].startswith('#'):
            val = ('i', imm(o[1]))
        elif mn == 'mov' and reg(o[1]) in st:
            val = st[reg(o[1])]
        elif mn == 'ldr' and re.match(r'\[(\w+), (\w+)\]$', o[1]):
            m = re.match(r'\[(\w+), (\w+)\]$', o[1])
            x, y = st.get(reg(m.group(1))), st.get(reg(m.group(2)))
            if x and y and {x[0], y[0]} <= {'i', 'a'}:
                slot = (x[1] + y[1]) & 0xffffffff
                if self.sec(slot) == '.got':
                    val = ('g', slot)
        elif mn == 'ldr' and re.match(r'\[(\w+)(?:, #(-?\w+))?\]$', o[1]):
            m = re.match(r'\[(\w+)(?:, #(-?\w+))?\]$', o[1])
            bv = st.get(reg(m.group(1)))
            if bv and bv[0] == 'a' and self.readonly(bv[1]):
                val = ('w', self.w(bv[1] + int(m.group(2) or '0', 0)))
        if mn in ('ldm', 'stm'):
            base = reg(o[0].rstrip('!'))
            rl = self.reglist(it)
            bv = st.get(base)
            if mn == 'stm':
                for i, r in enumerate(rl):
                    if bv and bv[0] == 's':
                        if r in st:
                            st['S%x' % (bv[1] + 4 * i)] = st[r]
                        else:
                            st.pop('S%x' % (bv[1] + 4 * i), None)
            if mn == 'ldm':
                for i, r in enumerate(rl):
                    if bv and bv[0] == 'a':
                        st[r] = ('w', self.w(bv[1] + 4 * i))
                    else:
                        st.pop(r, None)
            if o[0].endswith('!'):
                if bv:
                    st[base] = (bv[0], bv[1] + 4 * len(rl))
                else:
                    st.pop(base, None)
            return st
        if mn == 'str' and re.match(r'\[sp(?:, #(\w+))?\]$', o[1]):
            slot = 'S%x' % int(re.match(r'\[sp(?:, #(\w+))?\]$', o[1]).group(1) or '0', 0)
            if c is None and d in st:
                st[slot] = st[d]
            else:
                st.pop(slot, None)
            return st
        if mn == 'str' and re.match(r'\[(\w+)(?:, #(\w+))?\]$', o[1]):
            m = re.match(r'\[(\w+)(?:, #(\w+))?\]$', o[1])
            bv = st.get(reg(m.group(1)))
            if bv and bv[0] == 's':
                slot = 'S%x' % (bv[1] + int(m.group(2) or '0', 0))
                if c is None and d in st:
                    st[slot] = st[d]
                else:
                    st.pop(slot, None)
            return st
        if mn == 'ldr' and re.match(r'\[sp(?:, #(\w+))?\]$', o[1]):
            slot = 'S%x' % int(re.match(r'\[sp(?:, #(\w+))?\]$', o[1]).group(1) or '0', 0)
            val = st.get(slot)
        if mn == 'bl':
            t = imm(o[0])
            if self.names.get(t) == 'memcpy':
                dst, src, n = st.get('r0'), st.get('r1'), st.get('r2')
                if dst and dst[0] == 's' and src and src[0] == 'a' and n and n[0] == 'i':
                    for i in range(n[1] // 4):
                        st['S%x' % (dst[1] + 4 * i)] = ('w', self.w(src[1] + 4 * i))
            for r in ('r0', 'r1', 'r2', 'r3', 'ip', 'lr'):
                st.pop(r, None)
        elif d and mn not in ('cmp', 'cmn', 'tst', 'b', 'str', 'push', 'pop'):
            if c is not None:
                if st.get(d) != val:
                    st.pop(d, None)
            elif val is None:
                st.pop(d, None)
            else:
                st[d] = val
        return st

    def edge_facts(self, a, s, out):
        """On the 'equal' edge of a compare with a constant + beq/bne, the register is that
        constant (GCC reuses it as one), and so are copies of it made since the compare."""
        mn, c = self.split_cond(self.items[a].insn.mnemonic)
        if mn != 'b' or c not in ('eq', 'ne'):
            return out
        eq_side = imm(self.ops(self.items[a])[0]) if c == 'eq' else a + 4
        if s != eq_side:
            return out
        copies = []
        p = a - 4
        while p in self.items and p > a - 24:
            it = self.items[p]
            po = self.ops(it)
            m = it.insn.mnemonic
            if m in ('cmp', 'subs'):
                break
            if m in ('tst', 'cmn', 'bl') or m.endswith('s') and m not in ('ldrs', 'ldrsb'):
                return out
            if m == 'mov' and not po[1].startswith('#'):
                copies.append((reg(po[0]), reg(po[1])))
            elif po:
                copies.append((reg(po[0]), None))
            p -= 4
        else:
            return out
        prev = self.items.get(p)
        if prev is None:
            return out
        po = self.ops(prev)
        regs, k = [], None
        if prev.insn.mnemonic == 'cmp' and po[1].startswith('#'):
            regs, k = [reg(po[0])], imm(po[1])
        elif prev.insn.mnemonic == 'cmp' and out.get(reg(po[1]), ('',))[0] == 'i':
            regs, k = [reg(po[0])], out[reg(po[1])][1]
        elif prev.insn.mnemonic == 'cmp' and out.get(reg(po[0]), ('',))[0] == 'i':
            regs, k = [reg(po[1])], out[reg(po[0])][1]
        elif prev.insn.mnemonic == 'subs' and po[2] == '#0':
            regs, k = [reg(po[0]), reg(po[1])], 0
        if not regs:
            return out
        # instructions between the compare and the branch, in order
        for dst, src in reversed(copies):
            if dst in regs and src not in regs:
                regs.remove(dst)
            elif src in regs:
                regs.append(dst)
        out = dict(out)
        for r in regs:
            out[r] = ('i', k)
        return out

    def propagate(self):
        f = self.f
        self.IN = {f.addr: {}}
        work = [f.addr]
        while work:
            a = work.pop()
            out = self.step(a, self.IN[a])
            for s in self.succs(a):
                if s not in self.items:
                    raise LiftError(f'{a:x}: branch to {s:x}, outside {f.name} (a fragment row?)')
                o_s = self.edge_facts(a, s, out)
                new = o_s if s not in self.IN else {k: v for k, v in self.IN[s].items()
                                                    if o_s.get(k) == v}
                if s not in self.IN or new != self.IN[s]:
                    self.IN[s] = new
                    work.append(s)
        self.reached = sorted(self.IN)
        self.labels = {f.addr}
        for a in self.reached:
            ss = self.succs(a)
            if ss != [a + 4]:
                self.labels.update(ss)

    # ---- addresses as C ----
    def cstr(self, a):
        return self.b.elf.cstring(a, 4096).decode('latin1')

    def is_list(self, a):
        s = self.sec(a)
        if s is None or s.startswith('.rodata'):
            return False
        n = 0
        while True:
            p = self.w(a + 4 * n)
            if p == 0:
                return n > 0
            ps = self.sec(p)
            if ps is None or not ps.startswith('.rodata'):
                return False
            n += 1

    def sym(self, name):
        self.syms.add(name)
        return name

    def addr_expr(self, a):
        """A C expression for address a, or None."""
        nm = self.names.get(a)
        if nm and (nm.startswith('en_rx_') or nm.startswith('cst_rx_')):
            return self.sym(nm)
        if self.is_list(a):
            if a not in self.lists:
                n, v = 0, []
                while self.w(a + 4 * n):
                    v.append(self.cstr(self.w(a + 4 * n)))
                    n += 1
                self.lists[a] = v
            return f'wl_{a:x}'
        if (self.sec(a) or '').startswith('.rodata'):
            return c_lit(self.cstr(a))
        if nm:
            return self.sym(nm)
        return None

    def word_expr(self, v, a):
        if v == 0:
            return '0'
        e = self.addr_expr(v)
        if e is None:
            raise LiftError(f'{a:x}: loads {v:#x}, which has no C name')
        return f'(uint32_t)({e})'

    def got_expr(self, slot):
        kind, s = self.rel.get(slot, ('?', ''))
        if s:
            return self.sym(s.split('@')[0])
        return self.addr_expr(self.w(slot))

    def operand(self, s):
        if s == 'sp':
            return '(uint32_t)stkb'
        if s.startswith('#'):
            return f'0x{imm(s):x}u'
        return reg(s)

    def check_runtime(self, a, st, r):
        """r is read at run time: it must not hold an address that was left unassigned."""
        v = st.get(r)
        if v and v[0] == 'a' and self.sec(v[1]) != '.got' and self.addr_expr(v[1]) is None:
            raise LiftError(f'{a:x}: {r} holds {v[1]:#x}, which has no C name')

    # ---- emit ----
    def lift_insn(self, a):
        it = self.items[a]
        mn, c = self.split_cond(it.insn.mnemonic)
        o = self.ops(it)
        st = self.IN[a]
        after = self.step(a, st)
        guard = f'if {CONDS[c]} ' if c else ''
        d = reg(o[0]) if o else None
        txt = f'{it.insn.mnemonic} {it.insn.op_str}'
        if mn == 'push' or (mn in ('add', 'sub') and o[0] == 'sp'):
            return f'/* {txt} */'
        if mn == 'pop':
            return 'return (const char *)r0;'
        if mn == 'b':
            return f'{guard}goto L{imm(o[0]):x};'
        if mn == 'bl':
            t = imm(o[0])
            fn = self.names.get(t, f'FUN_{t:08x}')
            n = 1 if fn == self.f.name else ARITY.get(fn)
            if n is None:
                raise LiftError(f'{a:x}: add {fn} to ARITY')
            self.calls.add(fn)
            args = ['r0', 'r1', 'r2', 'r3', 'stk[0]', 'stk[1]', 'stk[2]'][:n]
            for r in args[:4]:
                self.check_runtime(a, st, r)
            if fn == 'memcpy' and st.get('r1', ('',))[0] == 'a' and st.get('r2', ('',))[0] == 'i':
                src, size = st['r1'][1], st['r2'][1]
                self.blocks[src, size] = ', '.join(
                    'NULL' if x == 0 else self.addr_expr(x) or '?'
                    for x in (self.w(src + 4 * i) for i in range(size // 4)))
                if '?' in self.blocks[src, size] or size % 4:
                    raise LiftError(f'{a:x}: memcpy from {src:#x} is not a pointer block')
                args[1] = f'(uint32_t)blk_{src:x}_{size:x}'
            return f'r0 = (uint32_t)C_{fn}({", ".join(args)});'
        if mn == 'cmp':
            return f'fa = {reg(o[0])}; fb = {self.operand(o[1])};'
        if mn == 'cmn':
            return f'fa = {reg(o[0])}; fb = -{self.operand(o[1])};'
        if mn == 'tst':
            return f'fa = {reg(o[0])} & {self.operand(o[1])}; fb = 0;'
        if mn == 'subs':
            return f'fa = {reg(o[1])}; fb = {self.operand(o[2])}; {d} = fa - fb;'
        if c is None and d in after and after[d][0] in ('a', 'g'):
            k, v = after[d]
            if k == 'a' and self.sec(v) == '.got':
                return f'/* {d} = GOT: {txt} */'
            e = self.addr_expr(v) if k == 'a' else self.got_expr(v)
            if e is None:
                return f'/* {d} = {v:#x}, not named: {txt} */'
            return f'{d} = (uint32_t)({e});  /* {txt} */'
        if mn in ('ldm', 'stm'):
            base = reg(o[0].rstrip('!'))
            rl = self.reglist(it)
            parts = []
            for i, r in enumerate(rl):
                if mn == 'ldm' and after.get(r, ('',))[0] == 'w':
                    parts.append(f'{r} = {self.word_expr(after[r][1], a)};')
                elif mn == 'ldm':
                    self.check_runtime(a, st, base)
                    parts.append(f'{r} = *(uint32_t *)({base} + {4 * i});')
                else:
                    self.check_runtime(a, st, base)
                    parts.append(f'*(uint32_t *)({base} + {4 * i}) = {r};')
            if o[0].endswith('!'):
                parts.append(f'{base} += {4 * len(rl)};')
            return ' '.join(parts) + f'  /* {txt} */'
        if mn == 'ldr' and o[1].startswith('[pc'):
            return f'{guard}{d} = 0x{self.pcrel(a, o[1]):x}u;'
        if c is None and mn == 'ldr' and after.get(d, ('',))[0] == 'w':
            return f'{d} = {self.word_expr(after[d][1], a)};  /* {txt} */'
        if c and mn == 'add' and len(o) == 3 and o[1] == 'pc':
            for back in range(4, 20, 4):
                pi = self.items.get(a - back)
                if pi and pi.insn.mnemonic == 'ldr' + c and self.ops(pi)[0] == o[2] \
                        and self.ops(pi)[1].startswith('[pc'):
                    e = self.addr_expr((self.pcrel(a - back, self.ops(pi)[1]) + a + 8) & 0xffffffff)
                    if e is None:
                        break
                    return f'{guard}{d} = (uint32_t)({e});  /* {txt} */'
            raise LiftError(f'{a:x}: cannot resolve {txt}')
        if mn == 'mov':
            self.check_runtime(a, st, reg(o[1]))
            return f'{guard}{d} = {self.operand(o[1])};'
        if mn in ('add', 'sub', 'and', 'orr', 'eor'):
            for x in o[1:]:
                self.check_runtime(a, st, reg(x))
            op = {'add': '+', 'sub': '-', 'and': '&', 'orr': '|', 'eor': '^'}[mn]
            return f'{guard}{d} = {self.operand(o[1])} {op} {self.operand(o[2])};'
        if mn in ('ldr', 'ldrb', 'ldrsb', 'str', 'strb'):
            m = re.match(r'\[(\w+)(?:, (#-?\w+|\w+))?\]$', o[1])
            if m is None:
                raise LiftError(f'{a:x}: addressing mode of {txt}')
            if m.group(1) == 'sp':
                base = 'stkb'
            else:
                self.check_runtime(a, st, reg(m.group(1)))
                base = f'(uint8_t *){reg(m.group(1))}'
            off = m.group(2)
            offe = '' if off is None else (f' + {int(off[1:], 0)}' if off.startswith('#')
                                          else f' + {reg(off)}')
            ty = {'ldr': 'uint32_t', 'str': 'uint32_t', 'ldrb': 'uint8_t', 'strb': 'uint8_t',
                  'ldrsb': 'int8_t'}[mn]
            p = f'*({ty} *)({base}{offe})'
            return f'{guard}{p} = {d};' if mn.startswith('str') else f'{guard}{d} = (uint32_t){p};'
        raise LiftError(f'{a:x}: cannot lift {txt}')

    def lift(self, coverage=False) -> str:
        self.propagate()
        body = []
        labels = sorted(self.labels)
        for a in self.reached:
            if a in self.labels:
                body.append(f'L{a:x}:')
                if coverage:
                    body.append(f'    cov_hits[{labels.index(a)}]++;')
            body.append('    ' + self.lift_insn(a))
        f = self.f
        h = [f'/* {f.name} (0x{f.addr:x}, {f.size:#x} bytes of {self.b.stem}), lifted from the binary',
             ' * by tools/asm2c.py: a functional stand-in, not decompiled source. Generated; do not',
             ' * edit or commit. */',
             '#include <stddef.h>', '#include <stdint.h>', '#include <string.h>', '']
        for s in sorted(self.syms):
            h.append(f'extern const char {s}[];')
        for fn in sorted(self.calls):
            if fn in LIBC:
                params = ', '.join('abc'[:ARITY[fn]])
                h.append(f'#define C_{fn}({params}) {fn}({LIBC[fn]})')
            elif fn == f.name:
                h.append(f'#define C_{fn}(a) {fn}((const void *)(a))')
            else:
                h.append(f'uint32_t {fn}();')
                h.append(f'#define C_{fn} {fn}')
        for a, v in sorted(self.lists.items()):
            h.append(f'static const char *const wl_{a:x}[] = {{ '
                     + ', '.join(c_lit(x) for x in v) + ', NULL };')
        for (a, n), init in sorted(self.blocks.items()):
            h.append(f'static const char *const blk_{a:x}_{n:x}[] = {{ {init} }};')
        if coverage:
            h += [f'static const unsigned cov_addr[{len(labels)}] = {{ '
                  + ', '.join(f'0x{a:x}' for a in labels) + ' };',
                  f'static unsigned long cov_hits[{len(labels)}];',
                  '#include <stdio.h>', '#include <stdlib.h>',
                  'static void cov_dump(void)', '{',
                  '    const char *p = getenv("IVONA_COVERAGE");',
                  '    FILE *f = p ? fopen(p, "a") : NULL;', '    unsigned i;',
                  '    if (!f)', '        return;',
                  f'    for (i = 0; i < {len(labels)}; i++)',
                  '        fprintf(f, "%x %lu\\n", cov_addr[i], cov_hits[i]);',
                  '    fclose(f);', '}',
                  'static void cov_init(void) __attribute__((constructor));',
                  'static void cov_init(void) { atexit(cov_dump); }']
        h += ['', f'const char *{f.name}(const void *arg)', '{',
              '    uint32_t r0 = (uint32_t)arg, r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0, r7 = 0;',
              '    uint32_t r8 = 0, r9 = 0, r10 = 0, r11 = 0, ip = 0, lr = 0, fa = 0, fb = 0;',
              '    uint32_t stk[64] = { 0 };', '    uint8_t *stkb = (uint8_t *)stk;', '']
        return '\n'.join(h + body + ['}', ''])


# ---------------------------------------------------------------------------------------------
# --readable: typed C with nested calls and returns folded into conditions. Still goto-style
# where blocks are shared, but a starting point for a real decompilation, and portable.

RET = {'ffeature_string': 'const char *', 'item_feat_string': 'const char *',
       'val_string': 'const char *', 'strstr': 'const char *', 'cst_strrchr': 'const char *',
       'item_feat': 'const cst_val *', 'strlen': 'size_t', 'memcpy': 'void *'}
for _f in ('item_traverse', 'item_parent', 'item_prev', 'item_next', 'item_as', 'item_daughter',
           'item_last_daughter', 'phrase_start_token', 'phrase_end_token'):
    RET[_f] = 'cst_item *'
for _f in ('strcmp', 'cst_member_string', 'cst_regex_match', 'phrase_contains_word',
           'cst_startswith', 'cst_endswith', 'en_lex_select_pos', 'cst_startswith_any',
           'cst_endswith_any'):
    RET[_f] = 'int'
PREFIX = {'cst_item *': 'it', 'const cst_item *': 'it', 'const char *': 's', 'int': 'n',
          'size_t': 'len', 'const char *const *': 'wl', 'const cst_regex *': 'rx',
          'const cst_val *': 'v', 'void *': 'p'}
INV = {'eq': 'ne', 'ne': 'eq', 'ls': 'hi', 'hi': 'ls', 'lo': 'hs', 'hs': 'lo', 'lt': 'ge',
       'ge': 'lt', 'gt': 'le', 'le': 'gt'}
COND_OP = {'eq': '==', 'ne': '!=', 'ls': '<=', 'hi': '>', 'lo': '<', 'hs': '>=', 'lt': '<',
           'ge': '>=', 'gt': '>', 'le': '<='}


class V:
    """A symbolic value. kind: const (text), var (text), stack (sp offset), call (fn, args),
    load (base, offset; text = 'ptr'/'b'/'sb'), bin (fn = operator, args)."""

    def __init__(self, kind, type_, text=None, fn=None, args=(), key=None):
        self.kind, self.type, self.text, self.fn, self.args, self.key = \
            kind, type_, text, fn, list(args), key

    def has_call(self):
        return self.kind == 'call' or any(isinstance(a, V) and a.has_call() for a in self.args)


class Readable(Lifter):
    def __init__(self, b, fname, name=None):
        super().__init__(b, fname)
        self.name = name or fname
        self.used_lists = {}
        self.decl = {}
        self.vtype = {}
        self.ntmp = 0

    # ---- constants ----
    def list_name(self, a):
        nm = self.names.get(a)
        n = nm if nm and nm.startswith('en_wl_') else f'en_wl_{a:x}'
        if a not in self.used_lists:
            v, k = [], 0
            while self.w(a + 4 * k):
                v.append(self.cstr(self.w(a + 4 * k)))
                k += 1
            self.used_lists[a] = (n, v)
        return n

    def const(self, kv):
        k, v = kv
        if k == 'i':
            sv = v - (1 << 32) if v >= 0x80000000 else v
            return V('const', 'int', f'0x{sv:x}' if sv >= 0x100 else str(sv))
        if k == 's':
            return V('stack', 'void *', text=str(v))
        if k == 'g':
            kind, sym = self.rel.get(v, ('?', ''))
            if sym:
                self.syms.add(sym.split('@')[0])
                return V('const', 'void *', '&' + sym.split('@')[0])
            k, v = 'a', self.w(v)
        if k == 'w':
            if v == 0:
                return V('const', 'int', 'NULL')
            k = 'a'
        if self.sec(v) == '.got':
            return V('const', 'void *', None)
        nm = self.names.get(v)
        if nm and (nm.startswith('en_rx_') or nm.startswith('cst_rx_')):
            self.syms.add(nm)
            return V('const', 'void *', '&' + nm)
        if self.is_list(v):
            return V('const', 'const char *const *', self.list_name(v))
        if (self.sec(v) or '').startswith('.rodata'):
            return V('const', 'const char *', c_lit(self.cstr(v)))
        if nm:
            self.syms.add(nm)
            return V('const', 'void *', '&' + nm)
        return V('const', 'void *', None)

    def opv(self, op, regs):
        if op.startswith('#'):
            return self.const(('i', imm(op)))
        return regs[reg(op)]

    # ---- blocks and liveness over registers and stack slots ----
    def uses_defs(self, a):
        it = self.items[a]
        mn, c = self.split_cond(it.insn.mnemonic)
        o = self.ops(it)
        R = lambda x: {reg(x)} if re.fullmatch(r'r\d+|sb|sl|fp|ip|lr', x) else set()
        use, dfn = set(), set()
        spm = re.match(r'\[sp(?:, #(\w+))?\]$', o[1]) if len(o) > 1 else None
        if mn == 'bl':
            fn = self.names.get(imm(o[0]))
            n = 1 if fn == self.f.name else ARITY.get(fn, 4)
            srcs = ['r0', 'r1', 'r2', 'r3', 'S0', 'S4', 'S8'][:n]
            if fn == 'item_traverse':   # variadic: up to the 0 that ends the path
                for i, r in enumerate(srcs[1:], 1):
                    if self.IN[a].get(r) in (('i', 0), ('w', 0)):
                        srcs = srcs[:i + 1]
                        break
            use |= set(srcs)
            dfn |= {'r0', 'r1', 'r2', 'r3', 'ip', 'lr'}
        elif mn in ('cmp', 'cmn', 'tst'):
            use |= R(o[0]) | R(o[1])
        elif mn == 'pop':
            use.add('r0')
        elif mn in ('b', 'push'):
            pass
        elif mn in ('ldm', 'stm'):
            base = o[0].rstrip('!')
            use |= R(base)
            rl = self.reglist(it)
            if mn == 'ldm':
                dfn |= set(rl)
            else:
                use |= set(rl)
            if o[0].endswith('!'):
                dfn |= R(base)
        elif mn.startswith('ldr') and spm:
            use.add('S%x' % int(spm.group(1) or '0', 0))
            dfn |= R(o[0])
        elif mn == 'str' and spm:
            use |= R(o[0])
            dfn.add('S%x' % int(spm.group(1) or '0', 0))
        elif mn.startswith('ldr'):
            for x in re.findall(r'\w+', o[1]):
                use |= R(x)
            dfn |= R(o[0])
        elif mn.startswith('str'):
            use |= R(o[0])
            for x in re.findall(r'\w+', o[1]):
                use |= R(x)
        else:
            for x in o[1:]:
                use |= R(x)
            dfn |= R(o[0])
        if c is not None and mn != 'b':
            use |= dfn
        return use, dfn

    def build_blocks(self):
        leaders = {self.f.addr}
        for a in self.reached:
            ss = self.succs(a)
            if ss != [a + 4]:
                leaders.update(ss)
                if a + 4 in self.IN:
                    leaders.add(a + 4)
        self.bstart = sorted(x for x in leaders if x in self.IN)
        self.blk = {}
        for i, s in enumerate(self.bstart):
            end = self.bstart[i + 1] if i + 1 < len(self.bstart) else max(self.reached) + 4
            self.blk[s] = [a for a in range(s, end, 4) if a in self.IN]
        self.bsucc = {s: self.succs(self.blk[s][-1]) for s in self.bstart}
        ud = {a: self.uses_defs(a) for a in self.reached}
        self.live_in = {s: set() for s in self.bstart}
        changed = True
        while changed:
            changed = False
            for s in reversed(self.bstart):
                live = set()
                for t in self.bsucc[s]:
                    live |= self.live_in[t]
                for a in reversed(self.blk[s]):
                    u, d = ud[a]
                    live = (live - d) | u
                if live != self.live_in[s]:
                    self.live_in[s] = live
                    changed = True
        self.epilogues = {s for s in self.bstart
                          if self.items[self.blk[s][-1]].insn.mnemonic == 'pop'
                          and all(self.items[a].insn.mnemonic in ('add', 'pop') for a in self.blk[s])}

    def needs_var(self, t, r):
        return r in self.live_in[t] and r not in self.IN[t] and not (t == self.f.addr and r == 'r0')

    def var_name(self, r, ty):
        n = f'{PREFIX.get(ty, "x")}_{r}'
        self.decl[n] = ty
        return n

    # ---- use accounting ----
    def use(self, v):
        if isinstance(v, V) and v.key is not None:
            if self.pass2:
                self.consumed[v.key] = self.consumed.get(v.key, 0) + 1
            else:
                self.uses[v.key] = self.uses.get(v.key, 0) + 1

    def born(self, v, out, pending):
        """v (which has a key) was just created: decide how it is evaluated."""
        if not self.pass2 or not v.has_call():
            return
        n = self.uses.get(v.key, 0)
        if n == 0:
            out.append(f'{self.render(v)};')
        elif n >= 2:
            self.materialize(v, out)
        else:
            pending.append(v)

    def materialize(self, v, out):
        self.ntmp += 1
        t = f'{PREFIX.get(v.type, "x")}{self.ntmp}'
        out.append(f'{t} = {self.render(v)};')
        self.decl[t] = v.type
        self.mat[v.key] = t

    def flush(self, out, pending):
        for p in pending:
            if p.key not in self.mat and self.consumed.get(p.key, 0) < self.uses.get(p.key, 0):
                self.materialize(p, out)
        pending.clear()

    # ---- rendering ----
    def render(self, v):
        if v.key is not None and v.key in self.mat:
            return self.mat[v.key]
        if v.kind in ('const', 'var'):
            if v.text is None:
                raise LiftError(f'{self.cur:x}: an address with no C name is used')
            return v.text
        if v.kind == 'stack':
            return self.stack_array(int(v.text))
        if v.kind == 'call':
            return f'{v.fn}({", ".join(self.render(x) for x in v.args)})'
        if v.kind == 'load':
            base, off = v.args
            b = self.render(base)
            if v.text == 'ptr':
                if off == 0 and b.startswith('&'):
                    return b[1:]
                raise LiftError(f'{self.cur:x}: pointer load from {b}')
            cty = 'signed char' if v.text == 'sb' else 'unsigned char'
            return f'((const {cty} *)({b}))[{off}]'
        if v.kind == 'bin':
            return f'({self.render(v.args[0])} {v.fn} {self.render(v.args[1])})'
        raise LiftError(f'cannot render {v.kind}')

    def stack_array(self, off):
        st = self.IN[self.cur]
        out = []
        while True:
            w = st.get('S%x' % off)
            if w is None or w[0] not in ('w', 'a'):
                raise LiftError(f'{self.cur:x}: stack array at sp+{off:#x} is not constant')
            if w[1] == 0:
                break
            e = self.const(('a', w[1]))
            if e.type != 'const char *':
                raise LiftError(f'{self.cur:x}: stack array holds {e.text}')
            out.append(e.text)
            off += 4
        return '(const char *const []){ ' + ', '.join(out + ['NULL']) + ' }'

    def cond(self, c, count=True):
        a, b, kind = self.flags
        if count:
            self.use(a)
            self.use(b)
        if not self.pass2:
            return ''
        ta, tb = self.render(a), self.render(b)
        if kind == 'tst':
            x = f'({ta} & {tb})'
            return {'eq': f'!{x}', 'ne': x}[c]
        if tb in ('0', 'NULL') and c in ('eq', 'ne'):
            if a.kind == 'call' and a.fn == 'strcmp':
                return f'{ta} {COND_OP[c]} 0'
            return f'!{ta}' if c == 'eq' else ta
        return f'{ta} {COND_OP[c]} {tb}'

    # ---- one block ----
    def run_block(self, s):
        """Simulate block s; returns (statements, terminator)."""
        out, pending = [], []
        self.consumed = {}
        regs = {}
        for r in self.live_in[s]:
            if r in self.IN[s]:
                regs[r] = self.const(self.IN[s][r])
            elif s == self.f.addr and r == 'r0':
                regs[r] = V('var', 'const cst_item *', 'word')
            else:
                ty = self.vtype.get((s, r), 'void *')
                regs[r] = V('var', ty, self.var_name(r, ty))
        self.flags = None
        condval = None
        term = ('fall',)
        for a in self.blk[s]:
            self.cur = a
            it = self.items[a]
            mn, c = self.split_cond(it.insn.mnemonic)
            o = self.ops(it)
            d = reg(o[0]) if o else None
            st, after = self.IN[a], self.step(a, self.IN[a])
            txt = f'{it.insn.mnemonic} {it.insn.op_str}'
            spm = re.match(r'\[sp(?:, #(\w+))?\]$', o[1]) if len(o) > 1 else None
            if mn == 'push' or (mn in ('add', 'sub') and o[0] == 'sp'):
                continue
            if mn == 'pop':
                self.use(regs['r0'])
                term = ('ret', regs['r0'])
                break
            if mn == 'b':
                t = imm(o[0])
                if c is None:
                    if t in self.epilogues:
                        self.use(regs['r0'])
                        term = ('ret', regs['r0'])
                    else:
                        term = ('goto', t)
                else:
                    if t in self.epilogues:
                        if not (condval and condval[0] == c and condval[1] == 'r0'):
                            raise LiftError(f'{a:x}: conditional return of an unknown value')
                        self.use(condval[2])
                        term = ('ifret', self.cond(c), condval[2])
                    else:
                        term = ('if', self.cond(c), t, self.cond(INV[c], count=False))
                break
            if c is not None and mn == 'ldr' and o[1].startswith('[pc'):
                continue
            if c is not None and mn == 'add' and len(o) == 3 and o[1] == 'pc':
                pi = self.items[a - 4]
                if pi.insn.mnemonic != 'ldr' + c or self.ops(pi)[0] != o[2]:
                    raise LiftError(f'{a:x}: {txt} without its load')
                addr = (self.pcrel(a - 4, self.ops(pi)[1]) + a + 8) & 0xffffffff
                condval = (c, d, self.const(('a', addr)))
                continue
            if mn == 'bl':
                fn = self.names.get(imm(o[0]))
                if fn == 'memcpy' and st.get('r0', ('',))[0] == 's':
                    for r in ('r1', 'r2', 'r3', 'ip', 'lr'):
                        regs.pop(r, None)
                    regs['r0'] = self.const(st['r0'])
                    continue
                n = 1 if fn == self.f.name else ARITY[fn]
                args = []
                for r in ['r0', 'r1', 'r2', 'r3', 'S0', 'S4', 'S8'][:n]:
                    v = regs.get(r)
                    if v is None and r in st:
                        v = self.const(st[r])
                    if v is None:
                        raise LiftError(f'{a:x}: argument {r} of {fn} is unknown')
                    args.append(v)
                    if fn == 'item_traverse' and r != 'r0' and v.kind == 'const' and v.text in ('0', 'NULL'):
                        args[-1] = V('const', 'int', '0')
                        break
                for x in args:
                    self.use(x)
                if self.pass2:
                    self.flush(out, [p for p in pending])
                    pending.clear()
                call = V('call', RET.get(fn, 'const char *'), fn=self.name if fn == self.f.name else fn,
                         args=args, key=(a, 'call'))
                for r in ('r0', 'r1', 'r2', 'r3', 'ip', 'lr'):
                    regs.pop(r, None)
                regs['r0'] = call
                self.born(call, out, pending)
                continue
            if c is None and d in after and after[d][0] in ('i', 'a', 'g', 'w') \
                    and mn not in ('cmp', 'cmn', 'tst', 'str', 'ldm', 'stm'):
                regs[d] = self.const(after[d])
                if mn == 'subs':
                    self.flags = (regs[d], self.const(('i', 0)), 'cmp')
                continue
            if mn in ('ldm', 'stm'):
                base = reg(o[0].rstrip('!'))
                if mn == 'ldm':
                    for r in self.reglist(it):
                        if r not in after:
                            raise LiftError(f'{a:x}: {txt}: not a constant load')
                        regs[r] = self.const(after[r])
                if o[0].endswith('!'):
                    if base not in after:
                        raise LiftError(f'{a:x}: {txt}: unknown base')
                    regs[base] = self.const(after[base])
                continue
            if mn == 'mov':
                src = self.opv(o[1], regs)
                if c is None:
                    regs[d] = src
                    continue
                nx = self.items.get(a + 4)
                if d == 'r0' and nx is not None and nx.insn.mnemonic == 'b' + c \
                        and imm(self.ops(nx)[0]) in self.epilogues:
                    condval = (c, d, src)
                    continue
                old = regs.get(d)
                ty = old.type if old is not None and old.kind != 'const' else src.type
                var = self.var_name(d, ty)
                self.use(old)
                self.use(src)
                cnd = self.cond(c)
                if self.pass2:
                    if not (old.kind == 'var' and old.text == var):
                        out.append(f'{var} = {self.render(old)};')
                    out.append(f'if ({cnd})\n        {var} = {self.render(src)};')
                regs[d] = V('var', ty, var)
                continue
            if mn in ('subs', 'cmp', 'tst'):
                x = regs[reg(o[1] if mn == 'subs' else o[0])]
                y = self.opv(o[2] if mn == 'subs' else o[1], regs)
                if mn == 'subs':
                    regs[d] = x
                self.flags = (x, y, 'tst' if mn == 'tst' else 'cmp')
                continue
            if mn == 'cmn':
                k = imm(o[1])
                self.flags = (regs[reg(o[0])], V('const', 'int', str(-(k if k < 0x80000000 else k - (1 << 32)))), 'cmp')
                continue
            if mn in ('add', 'sub', 'and', 'orr', 'eor'):
                if o[1] == 'sp':
                    regs[d] = V('stack', 'void *', text=str(imm(o[2])))
                    continue
                x, y = regs[reg(o[1])], self.opv(o[2], regs)
                self.use(x)
                self.use(y)
                op = {'add': '+', 'sub': '-', 'and': '&', 'orr': '|', 'eor': '^'}[mn]
                regs[d] = V('bin', 'int', fn=op, args=[x, y], key=(a, 'bin'))
                self.born(regs[d], out, pending)
                continue
            if mn.startswith('ldr') and spm:
                sl = 'S%x' % int(spm.group(1) or '0', 0)
                if sl in regs:
                    regs[d] = regs[sl]
                else:
                    ty = self.vtype.get((s, sl), 'void *')
                    regs[d] = V('var', ty, self.var_name(sl, ty))
                continue
            if mn == 'str' and spm:
                regs['S%x' % int(spm.group(1) or '0', 0)] = regs[d]
                continue
            if mn == 'str' and st.get(reg(re.findall(r'\w+', o[1])[0]), ('',))[0] == 's':
                continue   # building a constant stack array (rebuilt from the constants)
            if mn in ('ldr', 'ldrb', 'ldrsb'):
                m = re.match(r'\[(\w+)(?:, #(-?\w+))?\]$', o[1])
                if m is None:
                    raise LiftError(f'{a:x}: readable: {txt}')
                base = regs[reg(m.group(1))]
                self.use(base)
                kind = {'ldr': 'ptr', 'ldrb': 'b', 'ldrsb': 'sb'}[mn]
                ty = 'int'
                if kind == 'ptr':
                    ty = 'const char *' if base.text == '&_ctype_' else 'const cst_regex *'
                regs[d] = V('load', ty, text=kind,
                            args=[base, int(m.group(2) or '0', 0)], key=(a, 'load'))
                self.born(regs[d], out, pending)
                continue
            raise LiftError(f'{a:x}: readable: cannot handle {txt}')
        if term == ('fall',) and self.bsucc[s] and self.bsucc[s][0] in self.epilogues:
            self.use(regs['r0'])
            term = ('ret', regs['r0'])
        # live-out values the successors need in variables
        assigns = []
        for t in self.bsucc[s]:
            if t in self.epilogues and term[0] in ('ret', 'ifret'):
                continue
            for r in sorted(self.live_in[t]):
                if not self.needs_var(t, r) or r not in regs:
                    continue
                v = regs[r]
                self.vtype.setdefault((t, r), v.type)
                var = self.var_name(r, self.vtype[(t, r)])
                if v.kind == 'var' and v.text == var:
                    continue
                if (r, var) in [(x[0], x[1]) for x in assigns]:
                    continue
                self.use(v)
                assigns.append((r, var, v))
        if self.pass2:
            if pending:
                # anything still pending is consumed by the assignments or the terminator
                pass
            for r, var, v in assigns:
                out.append(f'{var} = {self.render(v)};')
            if term[0] == 'ret':
                term = ('ret', self.render(term[1]))
            elif term[0] == 'ifret':
                term = ('ifret', term[1], self.render(term[2]))
        return out, term

    def lift_readable(self):
        self.propagate()
        self.build_blocks()
        self.uses, self.mat = {}, {}
        self.pass2 = False
        for _ in range(3):   # settles the variable types
            self.uses = {}
            for s in self.bstart:
                self.run_block(s)
        self.pass2 = True
        res = {}
        for s in self.bstart:
            res[s] = self.run_block(s)
        return self.assemble(res)

    def assemble(self, res):
        pure = {s: t[1] for s, (st, t) in res.items() if not st and t[0] == 'ret'}
        target = lambda t: f'return {pure[t]};' if t in pure else f'goto L_{t:x};'
        order = self.bstart
        nxt = {order[i]: order[i + 1] for i in range(len(order) - 1)}
        # if (c) goto L; return X; L:  ->  if (!c) return X;
        inverted = set()
        for s in order:
            st, t = res[s]
            n = nxt.get(s)
            if t[0] == 'if' and n in pure and t[2] not in pure and nxt.get(n) == t[2]:
                inverted.add(s)
        labels = set()
        for s, (st, t) in res.items():
            if t[0] == 'goto' and t[1] not in pure:
                labels.add(t[1])
            if t[0] == 'if' and s not in inverted and t[2] not in pure:
                labels.add(t[2])
        body = []
        prev_falls = True
        skip = set()
        for s in order:
            st, t = res[s]
            if s in skip or (not prev_falls and s not in labels):
                prev_falls = False
                continue
            if s in labels:
                body.append(f'L_{s:x}:')
            body += ['    ' + x for x in st]
            if t[0] == 'ret':
                body.append(f'    return {t[1]};')
            elif t[0] == 'goto':
                body.append('    ' + target(t[1]))
            elif t[0] == 'if' and s in inverted:
                body.append(f'    if ({t[3]})\n        return {pure[nxt[s]]};')
                skip.add(nxt[s])
                prev_falls = True
                continue
            elif t[0] == 'if':
                body.append(f'    if ({t[1]})\n        {target(t[2])}')
            elif t[0] == 'ifret':
                body.append(f'    if ({t[1]})\n        return {t[2]};')
            prev_falls = t[0] in ('if', 'fall', 'ifret')
        f = self.f
        h = ['#include <stdlib.h>', '#include <string.h>', '#include "cst_item.h"',
             '#include "cst_regex.h"', '#include "cst_string.h"', '#include "cst_val.h"', '',
             'cst_item *item_traverse(const cst_item *item, ...);',
             'int phrase_contains_word(const cst_item *item, const char *const *words);',
             'cst_item *phrase_start_token(const cst_item *item, const char *punc_break);',
             'cst_item *phrase_end_token(const cst_item *item, const char *punc_break);',
             'int cst_startswith_any(const char *s, const char *const *slist);',
             'int cst_endswith_any(const char *s, const char *const *slist);',
             'int en_lex_select_pos(const cst_item *item, const char *word, const char *s);']
        for sym in sorted(self.syms):
            if sym.startswith('en_rx_'):
                h.append(f'extern const cst_regex *const {sym};')
            elif sym == '_ctype_':
                h.append('extern const char *_ctype_;')
        h.append('')
        for a, (n, v) in sorted(self.used_lists.items()):
            h.append(f'static const char *const {n}[] = {{  /* {a:#x} */')
            line = '   '
            for x in [c_lit(x) for x in v] + ['NULL']:
                if len(line) + len(x) + 2 > 100:
                    h.append(line.rstrip())
                    line = '   '
                line += ' ' + x + ','
            h.append(line.rstrip(','))
            h.append('};')
        h += ['', f'/* {f.addr:#x} arm: generated by tools/asm2c.py --readable */',
              f'const char *{self.name}(const cst_item *word)', '{']
        for n, ty in sorted(self.decl.items()):
            h.append(f'    {ty}{"" if ty.endswith("*") else " "}{n} = {"NULL" if ty.endswith("*") else "0"};')
        h.append('')
        return '\n'.join(h + body + ['}', ''])


def main() -> int:
    import argparse
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('binary')
    ap.add_argument('func')
    ap.add_argument('out', nargs='?')
    ap.add_argument('--name', help='name of the emitted function (default: FUNC)')
    ap.add_argument('--readable', action='store_true', help='typed C with nested calls')
    ap.add_argument('--coverage', action='store_true',
                    help='count each block; written to $IVONA_COVERAGE at exit as "addr hits"')
    args = ap.parse_args()
    b = Binary(args.binary)
    out = args.out or f'build/gen/{b.stem}/lifted_{args.func}.c'
    try:
        if args.readable:
            text = Readable(b, args.func, args.name).lift_readable()
        else:
            L = Lifter(b, args.func)
            text = L.lift(coverage=args.coverage)
            if args.name:
                text = text.replace(f'const char *{args.func}(', f'const char *{args.name}(')
                text = text.replace(f'#define C_{args.func}(a) {args.func}(',
                                    f'#define C_{args.func}(a) {args.name}(')
    except LiftError as e:
        print(f'asm2c {args.func}: {e}', file=sys.stderr)
        return 1
    full = path(out)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    if not os.path.exists(full) or open(full).read() != text:
        with open(full, 'w') as fh:
            fh.write(text)
    return 0


if __name__ == '__main__':
    sys.exit(main())
