/*
 * Lexicon matching state (0x23724..0x24308): which lexicons apply where in the input, and the
 * regex that splits text into lexicon tokens.
 */
#include <string.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "ivona_pcre.h"
#include "ivona_ustr.h"
#include "ivona_util.h"
#include "ivona_vector.h"
#include "tts_internal.h"

void strlist_free_ptr(void *p);

typedef struct str_n {
    const char *s;
    size_t n;
} str_n;

/* 0x23724 arm: orders a NUL-terminated key against n bytes of s */
int lex_key_cmp(const char *const *key, const str_n *s)
{
    const char *k = *key;
    size_t n = s->n;
    int r = strncmp(k, s->s, n);

    if (r != 0)
        return r;
    return k[n] != '\0';
}

typedef struct lex_match {
    int unk_0;
    ivona_ustr *from;           /* +0x04 */
    ivona_ustr *to;             /* +0x08 */
    cst_features *feats;        /* +0x0c */
} lex_match;

/* 0x23754 arm */
void lex_match_clear(lex_match *m)
{
    ivona_ustr_free(m->from);
    m->from = NULL;
    ivona_ustr_free(m->to);
    m->to = NULL;
    delete_features(m->feats);
    m->feats = NULL;
}

typedef struct lex_ctx {
    void *voice;
    pcre *tokens;               /* +0x04 */
    ivona_vector *langs;        /* +0x08: of int */
    void *ctx;                  /* +0x0c */
    ivona_vector *names;        /* +0x10: of strlist * */
    ivona_vector *marks;        /* +0x14: of int */
} lex_ctx;

/* 0x23af8 arm */
void lex_ctx_set_voice(lex_ctx *c, void *voice)
{
    c->voice = voice;
}

/* 0x23b00 arm */
lex_ctx *lex_ctx_new(void *voice)
{
    const char *err;
    int erroff;
    pcre *re;
    lex_ctx *c = NULL;

    re = pcre_compile("[\\p{Han}\\x{3220}-\\x{3243}\\x{3280}-\\x{337F}]|[\\p{Katakana}]+|[\\p{Hiragana}]+"
                      "|[\\p{L}\\p{M}\\p{N}]+|[\\p{S}\\p{Cn}]|[\\p{P}]|[\\p{Z}\\p{Cc}\\p{Cf}\\p{Cs}\\p{Co}]+",
                      PCRE_UTF8 | PCRE_NO_UTF8_CHECK | PCRE_UCP, &err, &erroff, NULL);
    if (re == NULL) {
        tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "Token regex compile error: %s", err);
        return c;
    }
    c = cst_safe_alloc(1, sizeof(lex_ctx));
    c->tokens = re;
    c->voice = voice;
    c->langs = vector_new(sizeof(int), NULL, NULL);
    c->marks = vector_new(sizeof(int), NULL, NULL);
    c->names = vector_new(sizeof(strlist *), NULL, (ivona_vector_destroy_fn)strlist_free_ptr);
    return c;
}

/* 0x23bc4 arm */
void lex_ctx_free(lex_ctx *c)
{
    if (c == NULL)
        return;
    vector_free(c->names);
    vector_free(c->langs);
    vector_free(c->marks);
    pcre_free(c->tokens);
    cst_free(c);
}

/* 0x23c0c arm: enters a scope using the lexicons named (whitespace separated) */
void lex_ctx_push_names(lex_ctx *c, const char *names)
{
    strlist *l = strlist_split_ws(names);

    vector_push(c->names, &l);
}

/* 0x23c38 arm */
void lex_ctx_pop_names(lex_ctx *c)
{
    if (vector_size(c->names))
        vector_pop(c->names);
}

/* 0x23c5c arm */
void lex_ctx_push_lang(lex_ctx *c, int lang)
{
    vector_push(c->langs, &lang);
}

/* 0x23c80 arm */
void lex_ctx_pop_lang(lex_ctx *c)
{
    if (vector_size(c->langs))
        vector_pop(c->langs);
}

/* 0x23ca4 arm */
void lex_ctx_clear_marks(lex_ctx *c)
{
    vector_clear(c->marks);
}

/* 0x23cac arm */
void lex_ctx_push_mark(lex_ctx *c, int mark)
{
    vector_push(c->marks, &mark);
}

/* 0x23cd0 arm */
void lex_ctx_set_ctx(lex_ctx *c, void *ctx)
{
    c->ctx = ctx;
}
