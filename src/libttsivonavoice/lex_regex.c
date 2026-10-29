/*
 * Regular expression lexicon rules (0x41edc..0x42a50): a rule's PCRE pattern is replaced, match by
 * match, by its replacement template. Templates know:
 *   \0..\9          the text of a group
 *   \{N}            the same, for any group number
 *   \{N,template}   template expanded in place of group N (keeping its input span)
 *   \x{HHHH}        a character
 *   \a \b \e \f \n \r \t \v \\ \& \}   escapes
 */
#include <string.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "ivona_pcre.h"
#include "ivona_ustr.h"
#include "ivona_util.h"

int cst_parse_hex_n(const char *s, size_t n, unsigned long *out);
int cst_parse_uint_n(const char *s, size_t n, unsigned long *out);
int utf8_decode(const char *s, int *len);

/* inserts src bytes [pos, pos + n) of a match group */
typedef void (*lex_insert_fn)(ustr_edit *e, size_t pos, size_t n);
/* reports a rule error */
typedef void (*lex_error_fn)(void *ctx, const char *msg);
/* told of each replacement: input [pos, pos + n) became out [0, out_len) */
typedef void (*lex_match_fn)(void *ctx, size_t pos, size_t n, const char *out, size_t out_len);

#define LEX_MAX_GROUPS 20

static const char lex_escapes[] = "a\ab\be\033f\fn\nr\rt\tv\v\\\\&&}}";

static void lex_expand(ustr_edit *e, const char *t, const int *ov, int n, lex_insert_fn insert,
                       lex_error_fn err, void *ctx);

static inline __attribute__((always_inline)) void lex_insert_group(ustr_edit *e, unsigned long g, const int *ov, int n,
                             lex_insert_fn insert, lex_error_fn err, void *ctx)
{
    char *msg;

    if (g < (unsigned long)n)
        insert(e, ov[2 * g], ov[2 * g + 1] - ov[2 * g]);
    else if (err) {
        msg = cst_asprintf("Group \\%d does not exist\n", (int)g);
        err(ctx, msg);
        cst_free(msg);
    }
}

/* 0x41ee4 arm: the '}' closing a template that starts at s (nested \{ and \x{ counted), NULL if
 * there is none */
static const char *lex_find_close(const char *s)
{
    int depth = 0, i;

    for (i = 0; s[i]; i++) {
        if (s[i] == '\\') {
            if (s[i + 1] == '{')
                depth++, i++;
            else if (s[i + 1] == 'x' && s[i + 2] == '{')
                depth++, i += 2;
        } else if (s[i] == '}') {
            if (depth == 0)
                return s + i;
            depth--;
        }
    }
    return NULL;
}

/* 0x41f80 arm: "N}" or "N,template}" after "\{"; returns the closing '}', NULL if malformed */
static const char *lex_group_template(const char *p, ustr_edit *e, const int *ov, int n,
                                      lex_insert_fn insert, lex_error_fn err, void *ctx)
{
    const char *comma = strchr(p, ','), *end = lex_find_close(p), *num_end = end;
    unsigned long g;
    ustr_edit sub;
    ustr_span span;
    ivona_ustr *r;
    char *t, *msg;

    if (comma && comma < end)
        num_end = comma;
    if (end == NULL || p == end || !cst_parse_uint_n(p, num_end - p, &g))
        return NULL;
    if (num_end == end) {
        lex_insert_group(e, g, ov, n, insert, err, ctx);
        return end;
    }
    if (g >= (unsigned long)n) {
        if (err) {
            msg = cst_asprintf("Group \\%d does not exist\n", (int)g);
            err(ctx, msg);
            cst_free(msg);
        }
        return end;
    }
    t = cst_substr(comma + 1, 0, end - comma - 1);
    ustr_edit_init(&sub, e->src);
    lex_expand(&sub, t, ov, n, insert, err, ctx);
    r = ustr_edit_result(&sub);
    ustr_span_of(e->src, ov[2 * g], ov[2 * g + 1] - ov[2 * g], &span);
    if (r)
        ustr_edit_insert_span(e, r->data, r->len, &span);
    cst_free(t);
    ivona_ustr_free(r);
    return end;
}

/* 0x42180 arm: appends the expansion of template t for the match ov (n groups) */
static void lex_expand(ustr_edit *e, const char *t, const int *ov, int n, lex_insert_fn insert,
                       lex_error_fn err, void *ctx)
{
    const char *p, *q, *esc;
    unsigned char x;
    unsigned long c;

    for (p = t; *p; p++) {
        if (*p == '\\') {
            x = p[1];
            if (x - '0' < 10) {
                lex_insert_group(e, x - '0', ov, n, insert, err, ctx);
                p++;
                continue;
            }
            if (x == '{') {
                if ((q = lex_group_template(p + 2, e, ov, n, insert, err, ctx)) != NULL) {
                    p = q;
                    continue;
                }
            } else if (x == 'x' && p[2] == '{') {
                q = strchr(p + 3, '}');
                if (q && q != p + 3 && cst_parse_hex_n(p + 3, q - (p + 3), &c)) {
                    ustr_edit_insert_char(e, c);
                    p = q;
                    continue;
                }
            } else {
                for (esc = lex_escapes; *esc; esc += 2)
                    if ((unsigned char)*esc == x)
                        break;
                if (*esc) {
                    ustr_edit_insert_n(e, esc + 1, 1);
                    p++;
                    continue;
                }
            }
        }
        ustr_edit_insert_n(e, p, 1);
    }
}

/* 0x42384 arm: text with every match of the rule re replaced by template t. insert copies a
 * group (default: as it is); err reports problems; match is told of each replacement. NULL if
 * nothing changed. */
ivona_ustr *lex_apply_text(const pcre *re, const pcre_extra *extra, const char *t,
                           const ivona_ustr *text, lex_insert_fn insert, lex_error_fn err,
                           lex_match_fn match, void *ctx)
{
    int ov[LEX_MAX_GROUPS * 3];
    ustr_edit e;
    size_t out0;
    char *msg;
    int rc, len;

    if (insert == NULL)
        insert = ustr_edit_copy;
    ustr_edit_init(&e, text);
    for (;;) {
        rc = pcre_exec(re, extra, text->len ? text->data : "", text->len, e.pos,
                       PCRE_NO_UTF8_CHECK, ov, LEX_MAX_GROUPS * 3);
        if (rc < 1)
            break;
        ustr_edit_keep(&e, ov[0] - e.pos);
        out0 = e.out ? e.out->len : e.pos;
        ustr_edit_skip(&e, ov[1] - ov[0]);
        lex_expand(&e, t, ov, rc, insert, err, ctx);
        if (match) {
            if (e.out)
                match(ctx, ov[0], ov[1] - ov[0], e.out->data + out0, e.out->len - out0);
            else
                match(ctx, ov[0], ov[1] - ov[0], e.src->data + out0, e.pos - out0);
        }
        if (ov[0] == ov[1]) {
            /* an empty match: step over one character */
            if (e.pos == text->len)
                return ustr_edit_result(&e);
            utf8_decode(text->data + e.pos, &len);
            ustr_edit_keep(&e, len);
        }
    }
    if (e.pos < text->len)
        ustr_edit_keep(&e, text->len - e.pos);
    if (err && rc != PCRE_ERROR_NOMATCH) {
        msg = cst_asprintf("Error matching rule: pcre_exec returned %d\n", rc);
        err(ctx, msg);
        cst_free(msg);
    }
    return ustr_edit_result(&e);
}
