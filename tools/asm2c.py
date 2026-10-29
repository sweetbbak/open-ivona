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
        if mn == 'bl':
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
                new = out if s not in self.IN else {k: v for k, v in self.IN[s].items()
                                                    if out.get(k) == v}
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

    def lift(self) -> str:
        self.propagate()
        body = []
        for a in self.reached:
            if a in self.labels:
                body.append(f'L{a:x}:')
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
        h += ['', f'const char *{f.name}(const void *arg)', '{',
              '    uint32_t r0 = (uint32_t)arg, r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0, r7 = 0;',
              '    uint32_t r8 = 0, r9 = 0, r10 = 0, r11 = 0, ip = 0, lr = 0, fa = 0, fb = 0;',
              '    uint32_t stk[64] = { 0 };', '    uint8_t *stkb = (uint8_t *)stk;', '']
        return '\n'.join(h + body + ['}', ''])


def main() -> int:
    if len(sys.argv) not in (3, 4):
        print(__doc__, file=sys.stderr)
        return 2
    b = Binary(sys.argv[1])
    out = sys.argv[3] if len(sys.argv) == 4 else f'build/gen/{b.stem}/lifted_{sys.argv[2]}.c'
    try:
        text = Lifter(b, sys.argv[2]).lift()
    except LiftError as e:
        print(f'asm2c {sys.argv[2]}: {e}', file=sys.stderr)
        return 1
    full = path(out)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    if not os.path.exists(full) or open(full).read() != text:
        with open(full, 'w') as fh:
            fh.write(text)
    return 0


if __name__ == '__main__':
    sys.exit(main())
