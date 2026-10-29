/*
 * Tokens of an utterance as list nodes (0x356d8..0x36a3c): what token_chunk_from_utt
 * (utt_chunk.c) keeps of each token while it rearranges them, and how they become tokens again.
 */
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_val.h"

typedef struct strpool strpool;
char *strpool_add(strpool *p, const char *s);
cst_val *lang_dat_core_data_val(const void *v);
void *val_lang_dat_core_data(const cst_val *v);

typedef struct token_node token_node;
struct token_node {
    token_node *prev;
    token_node *next;
};

#define TOKEN_INFO_WORD 0
#define TOKEN_INFO_PHONES 1     /* "phones" */
#define TOKEN_INFO_XSAMPA 2     /* "xsampa" */
#define TOKEN_INFO_RUBY 3       /* "ruby" */
#define TOKEN_INFO_SILENCE 4    /* "silence" */
#define TOKEN_INFO_VAL 5        /* "wave" */

/* what every spoken kind has */
typedef struct token_voice {
    int whisper;                /* +0x28: effect_whisper */
    float amplify;              /* +0x2c */
    float duration_stretch;     /* +0x30 */
    float pitch_stretch;        /* +0x34 */
} token_voice;

typedef struct token_info {
    token_node node;
    int no_pause_start;         /* +0x08 */
    int start_pos;              /* +0x0c: -1 if unknown */
    int end_pos;                /* +0x10 */
    const char *punc;           /* +0x14 */
    const char *whitespace;     /* +0x18 */
    int type;                   /* +0x1c: TOKEN_INFO_* */
    int w_tag_id;               /* +0x20 */
    int sayas_tag_id;           /* +0x24 */
    union {
        float silence;          /* +0x28: TOKEN_INFO_SILENCE */
        cst_val *val;           /* +0x28: TOKEN_INFO_VAL */
        struct {                /* TOKEN_INFO_WORD (0x5c bytes) */
            token_voice v;
            const char *name;   /* +0x38 */
            const char *prepunctuation;
            const char *interpret_as;   /* +0x40 */
            const char *interpret_as_format;
            const char *role;   /* +0x48 */
            const char *extra;
            void *lang;         /* +0x50: lang_dat_core_data */
            int domain;
            int apply_dnn;      /* +0x58 */
        } word;
        struct {                /* the other spoken kinds (0x54 bytes) */
            token_voice v;
            const char *text;   /* +0x38: phones, xsampa or ruby */
            const char *alphabet;       /* +0x3c: TOKEN_INFO_XSAMPA, TOKEN_INFO_RUBY */
            void *lang;         /* +0x40 */
            int domain;
            int apply_dnn;      /* +0x48 */
            float breath_dur;   /* +0x4c: TOKEN_INFO_PHONES */
            float breath_volume;        /* +0x50 */
        } ph;
    } u;
} token_info;

#define TOKEN_INFO_SIZE_VAL 0x2c
#define TOKEN_INFO_SIZE_PH 0x54
#define TOKEN_INFO_SIZE_WORD 0x5c

static inline int item_int_or(const cst_item *t, const char *name, int def)
{
    return item_feat_present(t, name) ? item_feat_int(t, name) : def;
}

static inline float item_float_or(const cst_item *t, const char *name, float def)
{
    return item_feat_present(t, name) ? item_feat_float(t, name) : def;
}

static inline const char *item_pooled(const cst_item *t, const char *name, strpool *pool)
{
    return item_feat_present(t, name) ? strpool_add(pool, item_feat_string(t, name)) : NULL;
}

static inline void token_voice_from_item(token_voice *v, const cst_item *t)
{
    v->whisper = item_int_or(t, "effect_whisper", 0);
    v->amplify = item_float_or(t, "amplify", 1.0f);
    v->duration_stretch = item_float_or(t, "duration_stretch", 1.0f);
    v->pitch_stretch = item_float_or(t, "pitch_stretch", 1.0f);
}

static inline void token_voice_to_item(const token_voice *v, cst_item *t)
{
    if (v->whisper)
        item_set_int(t, "effect_whisper", v->whisper);
    if (v->amplify != 1.0f)
        item_set_float(t, "amplify", v->amplify);
    if (v->duration_stretch != 1.0f)
        item_set_float(t, "duration_stretch", v->duration_stretch);
    if (v->pitch_stretch != 1.0f)
        item_set_float(t, "pitch_stretch", v->pitch_stretch);
}

/* 0x356d8 arm: the fields every token kind has */
static void token_info_init(token_info *n, const cst_item *t, strpool *pool)
{
    n->node.prev = NULL;
    n->node.next = NULL;
    n->w_tag_id = item_int_or(t, "w_tag_id", 0);
    n->sayas_tag_id = item_int_or(t, "sayas_tag_id", 0);
    n->start_pos = item_int_or(t, "start_pos", -1);
    n->end_pos = item_int_or(t, "end_pos", -1);
    n->no_pause_start = item_int_or(t, "no_pause_start", 0);
    n->punc = item_pooled(t, "punc", pool);
    n->whitespace = item_pooled(t, "whitespace", pool);
}

/* 0x35868 arm: token t as a node, its strings kept in pool */
token_info *token_info_from_item(const cst_item *t, strpool *pool)
{
    token_info *n;
    const cst_val *lang;

    if (item_feat_present(t, "phones")) {
        n = cst_safe_alloc(1, TOKEN_INFO_SIZE_PH);
        lang = item_feat(t, "lang_dat_core_data");
        n->type = TOKEN_INFO_PHONES;
        token_info_init(n, t, pool);
        token_voice_from_item(&n->u.ph.v, t);
        n->u.ph.text = item_pooled(t, "phones", pool);
        n->u.ph.lang = lang ? val_lang_dat_core_data(lang) : NULL;
        n->u.ph.domain = get_param_int(item_feats(t), "domain", 0);
        n->u.ph.apply_dnn = get_param_int(item_feats(t), "tts_apply_dnn", 0);
        n->u.ph.breath_dur = item_float_or(t, "breath_dur", 0);
        n->u.ph.breath_volume = item_float_or(t, "breath_volume", 0);
    } else if (item_feat_present(t, "xsampa")) {
        n = cst_safe_alloc(1, TOKEN_INFO_SIZE_PH);
        lang = item_feat(t, "lang_dat_core_data");
        n->type = TOKEN_INFO_XSAMPA;
        token_info_init(n, t, pool);
        token_voice_from_item(&n->u.ph.v, t);
        n->u.ph.text = item_pooled(t, "xsampa", pool);
        n->u.ph.lang = lang ? val_lang_dat_core_data(lang) : NULL;
        n->u.ph.domain = get_param_int(item_feats(t), "domain", 0);
        n->u.ph.apply_dnn = get_param_int(item_feats(t), "tts_apply_dnn", 0);
        n->u.ph.alphabet = item_pooled(t, "alphabet", pool);
    } else if (item_feat_present(t, "ruby")) {
        n = cst_safe_alloc(1, TOKEN_INFO_SIZE_PH);
        lang = item_feat(t, "lang_dat_core_data");
        n->type = TOKEN_INFO_RUBY;
        token_info_init(n, t, pool);
        token_voice_from_item(&n->u.ph.v, t);
        n->u.ph.text = item_pooled(t, "ruby", pool);
        n->u.ph.domain = get_param_int(item_feats(t), "domain", 0);
        n->u.ph.apply_dnn = get_param_int(item_feats(t), "tts_apply_dnn", 0);
        n->u.ph.alphabet = item_pooled(t, "alphabet", pool);
        n->u.ph.lang = lang ? val_lang_dat_core_data(lang) : NULL;
    } else if (item_feat_present(t, "silence")) {
        n = cst_safe_alloc(1, TOKEN_INFO_SIZE_VAL);
        n->type = TOKEN_INFO_SILENCE;
        token_info_init(n, t, pool);
        n->u.silence = item_feat_float(t, "silence");
    } else if (item_feat_present(t, "wave")) {
        n = cst_safe_alloc(1, TOKEN_INFO_SIZE_VAL);
        n->type = TOKEN_INFO_VAL;
        token_info_init(n, t, pool);
        n->u.val = (cst_val *)item_feat(t, "wave");
        val_inc_refcount(n->u.val);
    } else {
        n = cst_safe_alloc(1, TOKEN_INFO_SIZE_WORD);
        lang = item_feat(t, "lang_dat_core_data");
        n->type = TOKEN_INFO_WORD;
        token_info_init(n, t, pool);
        token_voice_from_item(&n->u.word.v, t);
        n->u.word.name = item_pooled(t, "name", pool);
        n->u.word.prepunctuation = item_pooled(t, "prepunctuation", pool);
        n->u.word.interpret_as = item_pooled(t, "interpret-as", pool);
        n->u.word.interpret_as_format = item_pooled(t, "interpret-as-format", pool);
        n->u.word.role = item_pooled(t, "role", pool);
        n->u.word.extra = item_pooled(t, "extra", pool);
        n->u.word.lang = lang ? val_lang_dat_core_data(lang) : NULL;
        n->u.word.domain = get_param_int(item_feats(t), "domain", 0);
        n->u.word.apply_dnn = get_param_int(item_feats(t), "tts_apply_dnn", 0);
    }
    return n;
}

/* 0x36220 arm */
void token_info_free(token_info *t)
{
    if (t->type == TOKEN_INFO_VAL)
        delete_val(t->u.val);
    cst_free(t);
}

/* 0x36254 arm: appends node n to relation r as a token again */
void token_info_to_item(const token_info *n, cst_relation *r)
{
    cst_item *t = relation_append(r, NULL);

    item_set_int(t, "start_pos", n->start_pos);
    item_set_int(t, "end_pos", n->end_pos);
    item_set_int(t, "no_pause_start", n->no_pause_start);
    item_set_string(t, "punc", n->punc);
    if (n->w_tag_id)
        item_set_int(t, "w_tag_id", n->w_tag_id);
    if (n->sayas_tag_id)
        item_set_int(t, "sayas_tag_id", n->sayas_tag_id);
    if (n->whitespace)
        item_set_string(t, "whitespace", n->whitespace);
    switch (n->type) {
    case TOKEN_INFO_WORD:
        token_voice_to_item(&n->u.word.v, t);
        item_set_string(t, "name", n->u.word.name);
        if (n->u.word.prepunctuation)
            item_set_string(t, "prepunctuation", n->u.word.prepunctuation);
        if (n->u.word.interpret_as)
            item_set_string(t, "interpret-as", n->u.word.interpret_as);
        if (n->u.word.interpret_as_format)
            item_set_string(t, "interpret-as-format", n->u.word.interpret_as_format);
        if (n->u.word.role)
            item_set_string(t, "role", n->u.word.role);
        if (n->u.word.extra)
            item_set_string(t, "extra", n->u.word.extra);
        if (n->u.word.lang)
            item_set(t, "lang_dat_core_data", lang_dat_core_data_val(n->u.word.lang));
        item_set_int(t, "domain", n->u.word.domain);
        item_set_int(t, "tts_apply_dnn", n->u.word.apply_dnn);
        break;
    case TOKEN_INFO_PHONES:
        token_voice_to_item(&n->u.ph.v, t);
        item_set_string(t, "name", "");
        item_set_string(t, "phones", n->u.ph.text);
        if (n->u.ph.lang)
            item_set(t, "lang_dat_core_data", lang_dat_core_data_val(n->u.ph.lang));
        item_set_int(t, "domain", n->u.ph.domain);
        item_set_int(t, "tts_apply_dnn", n->u.ph.apply_dnn);
        if (n->u.ph.breath_dur != 0)
            item_set_float(t, "breath_dur", n->u.ph.breath_dur);
        if (n->u.ph.breath_volume != 0)
            item_set_float(t, "breath_volume", n->u.ph.breath_volume);
        break;
    case TOKEN_INFO_XSAMPA:
        token_voice_to_item(&n->u.ph.v, t);
        item_set_string(t, "name", "");
        item_set_string(t, "xsampa", n->u.ph.text);
        if (n->u.ph.alphabet)
            item_set_string(t, "alphabet", n->u.ph.alphabet);
        if (n->u.ph.lang)
            item_set(t, "lang_dat_core_data", lang_dat_core_data_val(n->u.ph.lang));
        item_set_int(t, "domain", n->u.ph.domain);
        item_set_int(t, "tts_apply_dnn", n->u.ph.apply_dnn);
        break;
    case TOKEN_INFO_RUBY:
        token_voice_to_item(&n->u.ph.v, t);
        item_set_string(t, "name", "");
        item_set_string(t, "ruby", n->u.ph.text);
        item_set_string(t, "alphabet", n->u.ph.alphabet);
        if (n->u.ph.lang)
            item_set(t, "lang_dat_core_data", lang_dat_core_data_val(n->u.ph.lang));
        item_set_int(t, "domain", n->u.ph.domain);
        item_set_int(t, "tts_apply_dnn", n->u.ph.apply_dnn);
        break;
    case TOKEN_INFO_SILENCE:
        item_set_string(t, "name", "");
        item_set_float(t, "silence", n->u.silence);
        break;
    case TOKEN_INFO_VAL:
        item_set_string(t, "name", "");
        item_set(t, "wave", n->u.val);
        break;
    }
}
