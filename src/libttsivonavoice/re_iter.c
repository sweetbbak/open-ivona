/*
 * Iterating over the matches of a compiled PCRE pattern (0x484fc..0x48610).
 */
#include <stddef.h>
#include "cst_alloc.h"
#include "ivona_pcre.h"
#include "ivona_util.h"

typedef struct re_iter {
    const pcre *re;
    const char *subject;
    int len;
    int pos;            /* where the next match is searched from */
} re_iter;

/* 0x484fc arm */
re_iter *re_iter_new(const pcre *re)
{
    re_iter *it = cst_safe_calloc(1, sizeof(re_iter));

    it->re = re;
    return it;
}

/* 0x48518 arm */
void re_iter_free(re_iter *it)
{
    if (it)
        cst_free(it);
}

/* 0x48524 arm */
void re_iter_reset(re_iter *it, const char *subject, int pos, int len)
{
    it->subject = subject;
    it->len = len;
    it->pos = pos;
}

/* 0x48530 arm: the next match as an offset and length in the subject; 0 when there is none. */
int re_iter_next(re_iter *it, int *start, int *len)
{
    int ov[30];

    if (pcre_exec(it->re, NULL, it->subject + it->pos, it->len - it->pos, 0, 0, ov, 30) <= 0)
        return 0;
    *start = ov[0] + it->pos;
    *len = ov[1] - ov[0];
    it->pos += ov[1];
    return 1;
}

/* 0x485c0 arm: all remaining matches. */
strlist *re_iter_split(re_iter *it)
{
    strlist *l = strlist_new();
    int start, len;

    while (re_iter_next(it, &start, &len))
        strlist_push_n(l, it->subject + start, len);
    return l;
}
