/*
 * Applying a chain of lexicons to a piece of text (0x232e4..0x23724). Lexicons whose source is
 * an SSML document ("<speak>...</speak>") may split the text; the pieces wait on a stack and
 * continue through the chain from the next lexicon on.
 */
#include <limits.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "ivona_ustr.h"
#include "ivona_vector.h"

char *xml_escape(const char *s, size_t n);
int cst_startswith_nocase(const char *s, const char *prefix);
int cst_endswith_nocase(const char *s, const char *suffix);

typedef struct lex_source {
    int unk_0;
    const char *text;           /* +0x04 */
    void *rules;                /* +0x08 */
    void *data;                 /* +0x0c */
} lex_source;

/* a piece of text and where in the chain it continues */
typedef struct lex_piece {
    ivona_ustr *text;
    int flags;
    int next;
} lex_piece;

typedef struct lex_applier {
    lex_source **chain;         /* NULL-terminated */
    int ssml;                   /* apply SSML lexicons */
    const ivona_ustr *input;
    ivona_ustr *text;           /* +0x0c: current result, NULL if unchanged */
    unsigned int pos;           /* +0x10: next lexicon, INT_MAX when done */
    ivona_vector *pending;      /* +0x14: of lex_piece */
    void *ctx;                  /* +0x18 */
} lex_applier;

typedef struct lex_result {
    ivona_ustr *text;
    int flags;
} lex_result;

ivona_ustr *lex_apply_text(void *rules, void *data, const char *src, const ivona_ustr *text,
                           void *a, void *b, void *c, lex_applier *ap);
ivona_vector *lex_apply_ssml(void *rules, void *data, const char *src, const ivona_ustr *text,
                             void (*insert)(ustr_edit *e, size_t pos, size_t n), void *a, void *b,
                             lex_applier *ap);

static inline void *vector_last(const ivona_vector *v)
{
    return v->count ? vector_at(v, v->count - 1) : NULL;
}

/* 0x232e4 arm */
static void lex_piece_free(void *p, size_t size)
{
    ivona_ustr_free(((lex_piece *)p)->text);
}

/* 0x232ec arm: inserts src bytes [pos, pos + n) XML escaped */
static void lex_insert_escaped(ustr_edit *e, size_t pos, size_t n)
{
    ustr_span span;
    char *s;
    size_t len;

    if (n != 0) {
        s = xml_escape(e->src->data + pos, n);
        len = strlen(s);
        ustr_span_of(e->src, pos, n, &span);
        ustr_edit_insert_span(e, s, len, &span);
        cst_free(s);
    }
}

/* 0x2335c arm */
lex_applier *lex_applier_new(lex_source **chain, int ssml, const ivona_ustr *input, unsigned int pos,
                             void *ctx)
{
    lex_applier *ap = cst_safe_calloc(1, sizeof(lex_applier));

    ap->chain = chain;
    ap->ssml = ssml;
    ap->input = input;
    ap->pos = pos;
    ap->ctx = ctx;
    ap->text = NULL;
    ap->pending = NULL;
    return ap;
}

/* 0x233a0 arm */
void lex_applier_free(lex_applier *ap)
{
    if (ap == NULL)
        return;
    ivona_ustr_free(ap->text);
    vector_free(ap->pending);
    cst_free(ap);
}

/* 0x233c8 arm: runs the rest of the chain; 1 if there is a result to take */
int lex_applier_run(lex_applier *ap)
{
    lex_source *s;
    const char *src;
    ivona_ustr *t;
    ivona_vector *r;
    lex_result *top, *p;
    lex_piece piece;

    if (ap->pos >= INT_MAX)
        goto done;
    for (s = ap->chain[ap->pos]; s; s = ap->chain[++ap->pos]) {
        if (ap->pending && ap->text == NULL)
            return 1;
        src = s->text;
        if (cst_startswith_nocase(src, "<speak>") && cst_endswith_nocase(src, "</speak>")) {
            if (!ap->ssml)
                continue;
            r = lex_apply_ssml(s->rules, s->data, src, ap->text ? ap->text : ap->input,
                               lex_insert_escaped, NULL, NULL, ap);
            if (r == NULL)
                continue;
            if (ap->pending == NULL)
                ap->pending = vector_new(sizeof(lex_piece), NULL, lex_piece_free);
            if (vector_size(r) != 0) {
                vector_reverse(r);
                top = vector_last(r);
                ivona_ustr_free(ap->text);
                if (top->flags == 0) {
                    ap->text = top->text;
                    top->text = NULL;
                } else
                    ap->text = NULL;
                vector_pop(r);
                for (p = r->data; p != (lex_result *)r->data + r->count; p++) {
                    piece.text = p->text;
                    piece.flags = p->flags;
                    piece.next = ap->pos + 1;
                    p->text = NULL;
                    vector_push(ap->pending, &piece);
                }
            }
            vector_free(r);
        } else {
            t = lex_apply_text(s->rules, s->data, s->text, ap->text ? ap->text : ap->input,
                               NULL, NULL, NULL, ap);
            if (t) {
                ivona_ustr_free(ap->text);
                ap->text = t;
            }
        }
    }
    ap->pos = INT_MAX;
done:
    if (ap->pending)
        return 1;
    return ap->text != NULL;
}

/* 0x2364c arm: takes the next result piece; 0 if there is none */
int lex_applier_take(lex_applier *ap, lex_piece *out)
{
    lex_piece *p;

    if (ap->text) {
        out->text = ap->text;
        ap->text = NULL;
        out->flags = 0;
        out->next = ap->pos;
        return 1;
    }
    if (ap->pending == NULL || vector_size(ap->pending) == 0)
        return 0;
    p = vector_last(ap->pending);
    memcpy(out, p, sizeof(lex_piece));
    memset(p, 0, sizeof(lex_piece));
    vector_pop(ap->pending);
    return 1;
}
