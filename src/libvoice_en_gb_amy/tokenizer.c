/*
 * Text tokenizers (0x5fd0c..0x603e0): a tokenizer walks the text segments of an utterance (each
 * with its voice) and returns tokens with their punctuation and positions. The default one uses
 * cst_tokenstream with the voice's character classes; a voice can supply its own constructor.
 */
#include <string.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_tokenstream.h"
#include "cst_val.h"
#include "ivona_vector.h"

typedef struct tokenizer tokenizer;
typedef struct token token;

typedef token *(*tokenizer_next_fn)(void *data, const void *segment, tokenizer *t);
typedef tokenizer *(*tokenizer_new_fn)(ivona_vector *segments, cst_features *feats,
                                       cst_features *params);
typedef char *(*tokenizer_expand_str_fn)(const char *s);

/* a token and what surrounded it */
struct token {
    const char *name;
    const char *whitespace;
    const char *prepunctuation;
    const char *postpunctuation;
    int start;              /* +0x10: input position */
    int end;                /* +0x14 */
    int unk_18;
};

struct tokenizer {
    token tok;
    void *data;                     /* +0x1c */
    ivona_vector *segments;         /* +0x20: owned, if set */
    tokenizer_next_fn next;         /* +0x24 */
    void (*free_data)(void *data);  /* +0x28 */
};

/* a text segment: its text and voice */
typedef struct segment {
    const char *text;
    void *voice;
    int unk_8;
} segment;

extern const int cst_val_type_tokenizer_expand_str_func;
extern const int cst_val_type_string_tokenizer_new_func;
CST_VAL_REGISTER_FUNCPTR(tokenizer_expand_str_func, tokenizer_expand_str_fn)

/* 0x5fd7c arm */
static int segment_key_cmp(const void *elem, const void *key)
{
    return *(const void *const *)elem != *(const void *const *)key;
}

CST_VAL_REGISTER_FUNCPTR(string_tokenizer_new_func, tokenizer_new_fn)

/* 0x5fe00 arm */
tokenizer *tokenizer_new(void *data, tokenizer_next_fn next, void (*free_data)(void *data))
{
    tokenizer *t = cst_safe_calloc(1, sizeof(tokenizer));

    t->data = data;
    t->next = next;
    t->free_data = free_data;
    return t;
}

/* 0x5fe2c arm: the next token of segment (NULL: the current one), or NULL at the end. */
token *tokenizer_next(tokenizer *t, const void *segment)
{
    return t->next(t->data, segment, t);
}

/* 0x5fe44 arm */
token *tokenizer_get(tokenizer *t)
{
    return tokenizer_next(t, NULL);
}

/* 0x5fe4c arm */
void tokenizer_free(tokenizer *t)
{
    if (t->free_data && t->data)
        t->free_data(t->data);
    if (t->segments)
        vector_free(t->segments);
    cst_free(t);
}

tokenizer *ts_tokenizer_new(ivona_vector *segments, cst_features *feats, cst_features *params);

/* 0x5fe8c arm: the voice's tokenizer ("string_tokenizer_new_func"), else the default. */
tokenizer *string_tokenizer_new(ivona_vector *segments, cst_features *feats, cst_features *params)
{
    tokenizer_new_fn f = ts_tokenizer_new;

    if (feat_present(feats, "string_tokenizer_new_func"))
        f = val_string_tokenizer_new_func(feat_val(feats, "string_tokenizer_new_func"));
    return f(segments, feats, params);
}

/* 0x5fef4 arm: a tokenizer over one string. */
tokenizer *string_tokenizer_new_single(const char *text, cst_features *feats, cst_features *params)
{
    ivona_vector *segments = vector_new(sizeof(segment), NULL, NULL);
    segment s = { text, NULL, 0 };
    tokenizer *t;

    vector_push(segments, &s);
    t = string_tokenizer_new(segments, feats, params);
    t->segments = segments;
    return t;
}

/* 0x5ff50 arm */
size_t segments_find(ivona_vector *segments, const void *key)
{
    return vector_find(segments, key, segment_key_cmp);
}

/* the default tokenizer's data */
typedef struct ts_tokenizer {
    cst_tokenstream *ts;
    cst_tokenstream *(*open)(const char *text);
    ivona_vector *segments;
    size_t seg;                 /* +0x0c: current segment */
    cst_features *feats;
    cst_features *params;
} ts_tokenizer;

/* 0x5ff60 arm */
static void ts_tokenizer_free(ts_tokenizer *d)
{
    ts_close(d->ts);
    cst_free(d);
}

/* opens segment i with the voice's character classes */
static inline void ts_tokenizer_open(ts_tokenizer *d, size_t i)
{
    segment *s = vector_get(d->segments, i);

    if (d->ts) {
        ts_close(d->ts);
        d->ts = NULL;
    }
    d->ts = d->open(s->text);
    d->ts->p_whitespacesymbols = get_param_string(d->feats, "text_whitespace",
                                                  d->ts->p_whitespacesymbols);
    d->ts->p_singlecharsymbols = get_param_string(d->feats, "text_singlecharsymbols",
                                                  d->ts->p_singlecharsymbols);
    d->ts->p_prepunctuationsymbols = get_param_string(d->feats, "text_prepunctuation",
                                                      d->ts->p_prepunctuationsymbols);
    d->ts->p_postpunctuationsymbols = get_param_string(d->feats, "text_postpunctuation",
                                                       d->ts->p_postpunctuationsymbols);
    d->ts->max_token_len = get_param_int(d->params, "text_maxwordlength", 179);
}

/* the next token of the stream, NULL at its end */
static inline token *ts_tokenizer_token(cst_tokenstream *ts, tokenizer *t)
{
    const char *name;
    int eof = ts_eof(ts);

    if (eof)
        return NULL;
    name = ts_get(ts);
    t->tok.name = name;
    t->tok.whitespace = ts->whitespace;
    t->tok.prepunctuation = ts->prepunctuation;
    t->tok.postpunctuation = ts->postpunctuation;
    t->tok.start = ts->token_pos;
    t->tok.end = ts->file_pos;
    t->tok.unk_18 = eof;
    return &t->tok;
}

/* 0x5ff7c arm */
static token *ts_tokenizer_next(ts_tokenizer *d, const void *key, tokenizer *t)
{
    segment *s = vector_get(d->segments, d->seg);
    size_t i;

    if (key == NULL || *(const void *const *)key == s->text)
        return ts_tokenizer_token(d->ts, t);
    i = segments_find(d->segments, key);
    if (i < d->seg)
        return NULL;
    ts_tokenizer_open(d, i);
    d->seg = i;
    return ts_tokenizer_token(d->ts, t);
}

/* 0x6015c arm */
tokenizer *ts_tokenizer_new(ivona_vector *segments, cst_features *feats, cst_features *params)
{
    ts_tokenizer *d = cst_safe_calloc(1, sizeof(ts_tokenizer));

    d->segments = segments;
    d->feats = feats;
    d->params = params;
    d->open = ts_open_string;
    ts_tokenizer_open(d, 0);
    d->seg = 0;
    return tokenizer_new(d, (tokenizer_next_fn)ts_tokenizer_next, (void (*)(void *))ts_tokenizer_free);
}

/* 0x6029c arm: the same over UTF-8 text. */
tokenizer *ts_tokenizer_new_utf8(ivona_vector *segments, cst_features *feats, cst_features *params)
{
    ts_tokenizer *d = cst_safe_calloc(1, sizeof(ts_tokenizer));

    d->segments = segments;
    d->feats = feats;
    d->params = params;
    d->open = ts_open_string_utf8;
    ts_tokenizer_open(d, 0);
    d->seg = 0;
    return tokenizer_new(d, (tokenizer_next_fn)ts_tokenizer_next, (void (*)(void *))ts_tokenizer_free);
}
