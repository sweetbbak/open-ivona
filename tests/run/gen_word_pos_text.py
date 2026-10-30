#!/usr/bin/env python3
"""Text that exercises en_word_pos's homograph rules, for the IVONA_CHECK_WORD_POS build.

  tests/run/gen_word_pos_text.py [N] [SEED] > words.txt

Reads src/libvoice_en_gb_amy/en_word_pos.c. The words each rule compares the word itself with
(`item_traverse(x, 0)`) are the focus words; the words and word lists compared at other
positions are neighbours. Each output sentence puts one focus word between random neighbours,
some capitalized, some followed by a comma. Deterministic for a given SEED. Every word goes
through the checker, so the text only has to reach code, not make sense.
"""
import os
import random
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, 'src/libvoice_en_gb_amy/en_word_pos.c')


def main() -> int:
    n = int(sys.argv[1]) if len(sys.argv) > 1 else 2000
    rng = random.Random(int(sys.argv[2]) if len(sys.argv) > 2 else 1)
    src = open(SRC, encoding='latin-1').read()
    lists = {}
    for m in re.finditer(r'static const char \*const (\w+)\[\] = \{[^\n]*\n(.*?)\};', src, re.S):
        lists[m.group(1)] = re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(2))
    focus, near = set(), set()
    for m in re.finditer(r'item_traverse\(\w+((?:, (?:0x)?[0-9a-f]+)*)\), "(?:name|punc)"\), '
                         r'(?:"((?:[^"\\]|\\.)*)"|(\w+))', src):
        path, lit, lst = m.group(1), m.group(2), m.group(3)
        words = [lit] if lit is not None else lists.get(lst, [])
        (focus if path == ', 0' else near).update(w for w in words if w and ' ' not in w)
    near |= {w for w in re.findall(r'"([A-Za-z\']{2,})"', src)}
    focus, near = sorted(focus), sorted(near)
    out = []
    for i in range(n):
        f = focus[i % len(focus)]
        left = [rng.choice(near) for _ in range(rng.randint(0, 3))]
        right = [rng.choice(near) for _ in range(rng.randint(0, 3))]
        words = left + [f] + right
        words = [w.capitalize() if rng.random() < 0.15 else w for w in words]
        s = ''
        for j, w in enumerate(words):
            s += w + (',' if rng.random() < 0.08 and j + 1 < len(words) else '') + ' '
        out.append(s.strip() + rng.choice('..?!'))
    print('\n'.join(out))
    return 0


if __name__ == '__main__':
    sys.exit(main())
