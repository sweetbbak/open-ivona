/*
 * Equivalence check of the decompiled en_word_pos (src/libvoice_en_gb_amy/en_word_pos.c)
 * against the original's code, lifted from target/ by tools/asm2c.py:
 *
 *   tools/build_runnable.py -b libvoice_en_gb_amy -D IVONA_CHECK_WORD_POS
 *
 * builds a voice library whose en_word_pos runs both versions on every word and aborts on the
 * first difference, printing the word and its neighbours. With IVONA_CHECK_VERBOSE set it
 * reports how many words it checked.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cst_item.h"

const char *en_word_pos_src(const cst_item *word);
const char *en_word_pos_lifted(const void *word);

static unsigned long checked;

static void report(void)
{
    fprintf(stderr, "en_word_pos: %lu words checked, no differences\n", checked);
}

const char *en_word_pos(const cst_item *word)
{
    const char *a = en_word_pos_src(word);
    const char *b = en_word_pos_lifted(word);

    if (checked++ == 0 && getenv("IVONA_CHECK_VERBOSE"))
        atexit(report);
    if (strcmp(a, b) != 0) {
        fprintf(stderr, "en_word_pos differs on \"%s\" (after \"%s\", before \"%s\"): "
                "decompiled \"%s\", original \"%s\"\n", ffeature_string(word, "name"),
                ffeature_string(word, "p.name"), ffeature_string(word, "n.name"), a, b);
        abort();
    }
    return a;
}
