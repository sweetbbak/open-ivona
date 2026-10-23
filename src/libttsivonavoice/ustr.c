/*
 * Encoded line reading and offset-tracking strings (0x45c88..0x46cb8).
 *
 * An ivona_ustr is UTF-8 text in which every byte remembers the span of input bytes it came
 * from, so that processed text (dictionary rewrites, normalisation) can be mapped back to
 * positions in the caller's input.
 */
#include <string.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "ivona_util.h"

/* text streams (textstream.c) */
typedef struct textstream textstream;
size_t textstream_avail(const textstream *ts);
const char *textstream_ptr(const textstream *ts);
size_t textstream_fill(textstream *ts, size_t n);
void textstream_advance(textstream *ts, size_t n);

#define TEXTSTREAM_CHUNK 0x1000

/* code unit of an input encoding: size, and where an ASCII byte sits in it */
typedef struct text_encoding {
    size_t unit;
    size_t pos;
} text_encoding;

static const text_encoding text_encodings[] = {
    { 2, 0 },   /* UTF-16LE */
    { 1, 0 },   /* 8-bit and UTF-8 */
    { 2, 1 },   /* UTF-16BE (and BOM-less UTF-16) */
};

typedef struct linereader {
    textstream *ts;
    size_t eol;         /* length of the current line's terminator */
    size_t len;         /* length of the current line */
    int lineno;
    const text_encoding *enc;
} linereader;

/* 0x45c88 arm */
linereader *linereader_new(textstream *ts, const char *encoding)
{
    linereader *lr = cst_safe_calloc(1, sizeof(linereader));
    const text_encoding *e;

    lr->ts = ts;
    if (strcasecmp(encoding, "UTF-16LE") == 0 || strcasecmp(encoding, "UTF16LE") == 0)
        e = &text_encodings[0];
    else if (strcasecmp(encoding, "UTF-16BE") == 0 || strcasecmp(encoding, "UTF16BE") == 0
             || strcasecmp(encoding, "UTF-16") == 0 || strcasecmp(encoding, "UTF16") == 0)
        e = &text_encodings[2];
    else
        e = &text_encodings[1];
    lr->enc = e;
    lr->len = 0;
    lr->eol = 0;
    lr->lineno = 0;
    return lr;
}

/* 0x45de4 arm */
void linereader_free(linereader *lr)
{
    if (lr)
        cst_free(lr);
}

/* 0x45df0 arm */
size_t linereader_len(const linereader *lr)
{
    return lr->len;
}

/* 0x45df8 arm */
int linereader_lineno(const linereader *lr)
{
    return lr->lineno;
}

/* whether the code unit at p is the ASCII character c */
static inline int linereader_is(const linereader *lr, const char *p, char c)
{
    char u[2];

    u[0] = 0;
    u[1] = 0;
    u[lr->enc->pos] = c;
    return memcmp(p, u, lr->enc->unit) == 0;
}

/* 0x45e00 arm: steps to the next line (CR, LF or CRLF terminated); 0 at the end. */
int linereader_next(linereader *lr)
{
    const char *p;
    size_t avail, i, unit;

    textstream_advance(lr->ts, lr->eol + lr->len);
    lr->lineno++;
    i = 0;
    for (;;) {
        avail = textstream_avail(lr->ts);
        p = textstream_ptr(lr->ts);
        unit = lr->enc->unit;
        for (; i + 2 * unit <= avail; i += unit) {
            if (linereader_is(lr, p + i, '\r')) {
                lr->len = i;
                lr->eol = unit << linereader_is(lr, p + i + unit, '\n');
                return 1;
            }
            if (linereader_is(lr, p + i, '\n')) {
                lr->len = i;
                lr->eol = unit;
                return 1;
            }
        }
        if (textstream_fill(lr->ts, TEXTSTREAM_CHUNK) == 0)
            break;
    }
    unit = lr->enc->unit;
    if (i + unit <= avail && (linereader_is(lr, p + i, '\r') || linereader_is(lr, p + i, '\n'))) {
        lr->len = i;
        lr->eol = unit;
        return 1;
    }
    lr->eol = 0;
    lr->len = avail;
    return avail != 0;
}

/* 0x46014 arm: the current line (newly allocated, still encoded). */
char *linereader_line(const linereader *lr)
{
    return cst_strndup(textstream_ptr(lr->ts), linereader_len(lr));
}

/* 0x46040 arm: the next n lines, each followed by "\n"; NULL if there are fewer. */
char *linereader_read_lines(linereader *lr, int n)
{
    strbuf b;
    int i;

    strbuf_init(&b);
    if (n == 0)
        return strbuf_data(&b);
    i = 0;
    while (linereader_next(lr)) {
        i++;
        strbuf_append(&b, textstream_ptr(lr->ts), linereader_len(lr));
        strbuf_append_str(&b, "\n");
        if (i == n)
            return strbuf_data(&b);
    }
    strbuf_release(&b);
    return NULL;
}

/* input bytes [start, end) */
typedef struct ustr_span {
    unsigned int start;
    unsigned int end;
} ustr_span;

typedef struct ivona_ustr {
    char *data;         /* NUL-terminated */
    size_t len;
    size_t cap;
    ustr_span *spans;   /* span of each byte of data */
    unsigned int start; /* span of the whole string */
    unsigned int end;
} ivona_ustr;

int utf8_encode(unsigned int c, char *out);

static inline void ustr_reserve(ivona_ustr *u, size_t n)
{
    if (u->cap < n) {
        u->data = cst_safe_realloc(u->data, n + 1, 1);
        u->data[n] = '\0';
        u->spans = cst_safe_realloc(u->spans, n, sizeof(ustr_span));
        u->cap = n;
    }
}

static inline void ustr_grow(ivona_ustr *u, size_t n)
{
    if (u->len + n > u->cap)
        ustr_reserve(u, 2 * (u->len + n));
}

/* 0x460e8 arm: n bytes of s, read from input offset pos onwards (one byte each). */
ivona_ustr *ustr_new(unsigned int pos, const char *s, size_t n)
{
    ivona_ustr *u = cst_safe_alloc(1, sizeof(ivona_ustr));
    size_t i;

    ustr_reserve(u, n);
    memcpy(u->data, s, n);
    for (i = 0; i < n; i++) {
        u->spans[i].start = pos + i;
        u->spans[i].end = pos + i + 1;
    }
    u->len = n;
    u->start = pos;
    u->end = pos + n;
    return u;
}

/* 0x461ac arm */
ivona_ustr *ustr_copy(const ivona_ustr *src)
{
    ivona_ustr *u;

    if (src == NULL)
        return NULL;
    u = cst_safe_alloc(1, sizeof(ivona_ustr));
    ustr_reserve(u, src->len);
    memcpy(u->data, src->data, src->len);
    memcpy(u->spans, src->spans, src->len * sizeof(ustr_span));
    u->len = src->len;
    u->start = src->start;
    u->end = src->end;
    return u;
}

/* 0x46260 arm */
void ivona_ustr_free(ivona_ustr *u)
{
    if (u == NULL)
        return;
    cst_free(u->data);
    cst_free(u->spans);
    cst_free(u);
}

/* 0x46288 arm */
void ustr_clear(ivona_ustr *u)
{
    u->len = 0;
    if (u->data)
        u->data[0] = '\0';
}

/* 0x462a0 arm: input span of bytes [idx, idx + n); for n == 0 the gap at idx. */
void ustr_span_of(const ivona_ustr *u, size_t idx, size_t n, ustr_span *out)
{
    size_t i;
    unsigned int t;

    if (n == 0) {
        if (idx > 0 && idx - 1 < u->len)
            out->start = u->spans[idx - 1].end;
        else if (idx < u->len)
            out->start = u->spans[idx].start;
        else
            out->start = u->start;
        if (idx < u->len)
            out->end = u->spans[idx].start;
        else if (idx > 0 && idx - 1 < u->len)
            out->end = u->spans[idx - 1].end;
        else
            out->end = u->end;
    } else {
        out->start = u->end;
        out->end = u->start;
        for (i = 0; i < n; i++) {
            if (u->spans[idx + i].start < out->start)
                out->start = u->spans[idx + i].start;
            if (u->spans[idx + i].end > out->end)
                out->end = u->spans[idx + i].end;
        }
    }
    if (out->end < out->start) {
        t = out->end;
        out->end = out->start;
        out->start = t;
    }
}

static inline void ustr_extend(ivona_ustr *u, const ustr_span *span)
{
    if (span->start < u->start)
        u->start = span->start;
    if (u->end < span->end)
        u->end = span->end;
}

/* 0x463b4 arm: appends n bytes of s, all from input span. */
void ustr_append(ivona_ustr *u, const char *s, size_t n, const ustr_span *span)
{
    size_t i;

    if (n == 0)
        return;
    ustr_grow(u, n);
    memcpy(u->data + u->len, s, n);
    for (i = 0; i < n; i++)
        memcpy(&u->spans[u->len + i], span, sizeof(ustr_span));
    u->len += n;
    u->data[u->len] = '\0';
    ustr_extend(u, span);
}

/* 0x464a4 arm */
ivona_ustr *ustr_from(const char *s, size_t n, const ustr_span *span)
{
    ivona_ustr *u = ustr_new(span->start, NULL, 0);

    ustr_append(u, s, n, span);
    return u;
}

/* 0x464e0 arm: appends code point c (UTF-8), from input span. */
void ustr_append_char(ivona_ustr *u, unsigned int c, const ustr_span *span)
{
    size_t i, n;

    ustr_grow(u, 8);
    n = utf8_encode(c, u->data + u->len);
    for (i = 0; i < n; i++)
        memcpy(&u->spans[u->len + i], span, sizeof(ustr_span));
    u->len += n;
    u->data[u->len] = '\0';
    ustr_extend(u, span);
}

/* 0x465cc arm: appends bytes [from, from + n) of src with their spans. */
void ustr_append_ustr(ivona_ustr *u, const ivona_ustr *src, size_t from, size_t n)
{
    size_t i;

    if (n == 0)
        return;
    ustr_grow(u, n);
    memcpy(u->data + u->len, src->data + from, n);
    memcpy(&u->spans[u->len], &src->spans[from], n * sizeof(ustr_span));
    u->len += n;
    u->data[u->len] = '\0';
    for (i = 0; i < n; i++) {
        if (src->spans[from + i].start < u->start)
            u->start = src->spans[from + i].start;
        if (src->spans[from + i].end > u->end)
            u->end = src->spans[from + i].end;
    }
}

/* 0x466d0 arm */
ivona_ustr *ustr_sub(const ivona_ustr *src, size_t from, size_t n)
{
    ivona_ustr *u = ustr_new(from, NULL, 0);

    ustr_append_ustr(u, src, from, n);
    return u;
}

int cst_strn_cmp(const char *a, size_t na, const char *b, size_t nb);
int ivona_time_cmp(const void *a, const void *b);   /* also orders spans (start, then end) */

/* 0x4670c arm: same text and spans. */
int ustr_equal(const ivona_ustr *a, const ivona_ustr *b)
{
    const ustr_span *sa, *sb;
    size_t i, n;

    if (cst_strn_cmp(a->data, a->len, b->data, b->len) != 0)
        return 0;
    n = a->len;
    sa = a->spans;
    sb = b->spans;
    if (n != b->len)
        return 0;
    for (i = 0; i < n; i++, sa++, sb++)
        if (ivona_time_cmp(sa, sb) != 0)
            return 0;
    return 1;
}

/* 0x46bcc arm: whether s holds a line break (n == 1) or at least two (paragraph break). */
int text_has_breaks(const char *s, int n)
{
    int seen = 0;

    for (; *s; s++) {
        if (*s == '\n' || *s == '\f' || (*s == '\r' && (s[1] != '\n' || (s++, 0)))) {
            if (n == 1 || seen)
                return 1;
            seen = 1;
        }
    }
    return 0;
}

const char *utf8_rtrim(const char *s, size_t len);
const char *cst_rtrim_n(const char *s, size_t len, int n, int flags);

/* 0x46c44 arm: whether the trailing whitespace of s[0..len) holds n line breaks. */
int text_ends_with_breaks(const char *s, size_t len, int n, int utf8)
{
    const char *p, *end;
    char *t;
    int r;

    p = utf8 ? utf8_rtrim(s, len) : cst_rtrim_n(s, len, n, 0);
    end = s + len;
    r = 0;
    if (end != p) {
        t = cst_substr(p, 0, end - p);
        r = text_has_breaks(t, n) != 0;
        cst_free(t);
    }
    return r;
}
