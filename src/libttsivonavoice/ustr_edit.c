/*
 * Copy-on-write editing of an ivona_ustr (0x47868..0x47bcc).
 *
 * An editor walks its source left to right. Kept bytes are only copied once the first change is
 * made, so an untouched source yields no output at all. Inserted text takes the input span of
 * the bytes skipped just before it.
 */
#include <string.h>
#include "ivona_ustr.h"

/* starts the output with everything kept so far */
static inline void ustr_edit_begin(ustr_edit *e)
{
    if (e->out == NULL) {
        e->out = ustr_new(0, NULL, 0);
        e->out->start = e->src->start;
        e->out->end = e->src->end;
        ustr_append_ustr(e->out, e->src, 0, e->pos);
    }
}

/* 0x47868 arm */
void ustr_edit_init_out(ustr_edit *e, const ivona_ustr *src, ivona_ustr *out)
{
    e->src = src;
    e->out = out;
    e->pos = 0;
    e->skipped = 0;
}

/* 0x47878 arm */
void ustr_edit_init(ustr_edit *e, const ivona_ustr *src)
{
    ustr_edit_init_out(e, src, NULL);
}

/* 0x47880 arm: the edited string, or NULL if nothing changed. */
ivona_ustr *ustr_edit_result(ustr_edit *e)
{
    if (e->out == NULL && e->pos != e->src->len)
        ustr_edit_begin(e);
    return e->out;
}

/* 0x478ec arm: drops the next n source bytes. */
void ustr_edit_skip(ustr_edit *e, size_t n)
{
    if (n == 0)
        return;
    ustr_edit_begin(e);
    e->skipped = n;
    e->pos += n;
}

/* 0x47954 arm: appends source bytes [from, from + n). */
void ustr_edit_copy(ustr_edit *e, size_t from, size_t n)
{
    if (n == 0)
        return;
    ustr_edit_begin(e);
    ustr_append_ustr(e->out, e->src, from, n);
}

/* 0x479c8 arm: keeps the next n source bytes. */
void ustr_edit_keep(ustr_edit *e, size_t n)
{
    if (e->out)
        ustr_edit_copy(e, e->pos, n);
    e->pos += n;
}

/* 0x479fc arm: inserts n bytes of s, spanning the last skipped source bytes. */
void ustr_edit_insert_n(ustr_edit *e, const char *s, size_t n)
{
    ustr_span span;

    if (n == 0)
        return;
    ustr_span_of(e->src, e->pos - e->skipped, e->skipped, &span);
    ustr_edit_begin(e);
    ustr_append(e->out, s, n, &span);
}

/* 0x47a98 arm */
void ustr_edit_insert_span(ustr_edit *e, const char *s, size_t n, const ustr_span *span)
{
    if (n != 0 && span != NULL) {
        ustr_edit_begin(e);
        ustr_append(e->out, s, n, span);
    }
}

/* 0x47b18 arm */
void ustr_edit_insert(ustr_edit *e, const char *s)
{
    ustr_edit_insert_n(e, s, strlen(s));
}

/* 0x47b40 arm: inserts code point c, spanning the last skipped source bytes. */
void ustr_edit_insert_char(ustr_edit *e, unsigned int c)
{
    ustr_span span;

    ustr_span_of(e->src, e->pos - e->skipped, e->skipped, &span);
    ustr_edit_begin(e);
    ustr_append_char(e->out, c, &span);
}
