/*
 * The synthesis state of an SSML document walk (..0x356d8): the context of the text being read
 * (flags, prosody, voice), the stacks they are saved on, and the utterance being built.
 */
#include <string.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "cst_error.h"
#include "cst_features.h"
#include "cst_utterance.h"
#include "ivona_ustr.h"
#include "ivona_util.h"
#include "ivona_vector.h"
#include "tts_voice.h"

#define REL_TOKEN 1

typedef struct synth_chunk synth_chunk;
void synth_chunk_free(synth_chunk *c);
void flag_reset(int *f);
int cst_parse_uint_n(const char *s, size_t n, unsigned long *out);

typedef struct prosody {
    float volume;
    float pitch;
    float rate;
} prosody;
void prosody_reset(prosody *p);

typedef struct utt_chunk {
    cst_utterance *utt;
    ustr_span span;             /* +0x04 */
    void *src;                  /* +0x0c */
    int phrasing_model;         /* +0x10 */
    int break_dur_model;        /* +0x14 */
} utt_chunk;
void utt_chunk_init_empty(utt_chunk *c, tts_instance **src, int phrasing_model,
                          int break_dur_model);
void utt_chunk_free_utt(utt_chunk *c);

/* a say-as interpretation */
typedef struct say_as_info {
    const char *name;
    int unk_4;
    int unk_8;
} say_as_info;

int say_as_info_flag(const say_as_info *sa);

/* the context text is read in (copied into every chunk) */
typedef struct synth_ctx {
    int unk_0[6];
    tts_voice *voice;           /* +0x18 */
    int unk_1c;
    int flags;                  /* +0x20 */
    prosody prosody;            /* +0x24 */
    ivona_vector *digit_groups; /* +0x30: of int, for say-as characters/serial */
    char *unk_34;
    int unk_38;
    int unk_3c;
    int say_as_depth;           /* +0x40 */
    const say_as_info *say_as;  /* +0x44 */
} synth_ctx;

/* collects tokens into utterances */
typedef struct utt_builder {
    void *unk_0;
    void *unk_4;
    int unk_8;
    int unk_c;
    int unk_10[6];
    utt_chunk chunk;            /* +0x28 */
    cst_relation *tokens;       /* +0x40 */
    int unk_44;
    int unk_48;
    unsigned int max_sentence;  /* +0x4c: eng_max_sentence */
    unsigned int max_word;      /* +0x50: eng_max_word */
    float sentence_break;       /* +0x54: extra pause after sentences */
    float paragraph_break;      /* +0x58: extra pause after paragraphs */
    strbuf text;                /* +0x5c */
} utt_builder;

typedef struct synth_state {
    synth_ctx *ctx;
    utt_builder *builder;       /* +0x04 */
    ivona_vector *flag_stack;   /* +0x08: of int */
    ivona_vector *prosody_stack; /* +0x0c: of prosody */
    ivona_vector *say_as_stack; /* +0x10: of const say_as_info * */
    ivona_vector *digit_groups_stack; /* +0x14: of ivona_vector * */
    strlist *lang_stack;        /* +0x18 */
    ivona_vector *unk_1c;       /* +0x1c: of int */
    int unk_20;
    ivona_vector *unk_24;
    int say_as_count;           /* +0x28 */
    ivona_vector *say_as_depth_stack; /* +0x2c: of int */
    ivona_vector *chunks;       /* +0x30: of synth_chunk * */
    int unk_34;
} synth_state;

void synth_state_flush(synth_state *s);

static const say_as_info no_say_as = { NULL, 0, 0 };

static inline void *vector_last(const ivona_vector *v)
{
    return v->count ? vector_at(v, v->count - 1) : NULL;
}

/* the last element of v, a vector of pointers (NULL if empty) */
static inline void *vector_last_ptr(const ivona_vector *v)
{
    return v->count ? *(void **)vector_at(v, v->count - 1) : NULL;
}

/* 0x314cc arm */
static void synth_chunk_free_ptr(void *p)
{
    synth_chunk_free(*(synth_chunk **)p);
}

/* 0x33798 arm */
synth_ctx *synth_ctx_new(tts_voice *v)
{
    synth_ctx *c = cst_safe_calloc(1, sizeof(synth_ctx));

    c->voice = v;
    c->unk_0[0] = 0;
    c->unk_0[1] = 0;
    c->unk_0[2] = 0;
    c->unk_0[3] = 0;
    c->unk_0[4] = 0;
    c->unk_0[5] = 0;
    c->unk_1c = 0;
    c->digit_groups = NULL;
    c->unk_34 = NULL;
    c->unk_38 = 0;
    c->say_as = NULL;
    return c;
}

/* 0x337e4 arm */
synth_ctx *synth_ctx_copy(const synth_ctx *c)
{
    synth_ctx *n = cst_safe_alloc(1, sizeof(synth_ctx));

    memcpy(n, c, sizeof(synth_ctx));
    if (c->digit_groups)
        n->digit_groups = vector_copy(c->digit_groups);
    if (c->unk_34)
        n->unk_34 = cst_strdup(c->unk_34);
    return n;
}

/* 0x33838 arm */
void synth_ctx_free(synth_ctx *c)
{
    vector_free(c->digit_groups);
    cst_free(c->unk_34);
    cst_free(c);
}

/* 0x3385c arm */
utt_builder *utt_builder_new(void *a, void *b, tts_voice *v)
{
    utt_builder *u = cst_safe_calloc(1, sizeof(utt_builder));
    float pause = get_param_float(VOICE_FEATURES(v), "pause_start_len", 0.050004f)
                  + get_param_float(VOICE_FEATURES(v), "pause_end_len", 0.050004f);
    float para;
    int n;

    u->unk_8 = 0;
    u->unk_10[0] = 0;
    u->unk_10[1] = 0;
    u->unk_10[2] = 0;
    u->unk_10[3] = 0;
    u->unk_10[4] = 0;
    u->unk_10[5] = 0;
    u->unk_0 = b;
    u->unk_4 = a;
    u->unk_c = 1;
    utt_chunk_init_empty(&u->chunk, (tts_instance **)v, 0, 0);
    u->tokens = utt_relation(u->chunk.utt, REL_TOKEN);
    u->unk_44 = 0;
    u->unk_48 = 0;
    u->max_sentence = get_param_int(VOICE_FEATURES(v), "eng_max_sentence", 0);
    if (u->max_sentence == 0)
        u->max_sentence = 1000;
    u->max_word = get_param_int(VOICE_FEATURES(v), "eng_max_word", 0);
    if (u->max_word == 0)
        u->max_word = 179;
    if (u->max_word > u->max_sentence)
        u->max_word = u->max_sentence;
    u->sentence_break = get_param_float(VOICE_FEATURES(v), "eng_sentence_break", 0.4f);
    para = get_param_float(VOICE_FEATURES(v), "eng_paragraph_break", 0.65f);
    u->paragraph_break = para > u->sentence_break ? para - u->sentence_break : 0;
    u->sentence_break = pause < u->sentence_break ? u->sentence_break - pause : 0;
    strbuf_init(&u->text);
    return u;
}

/* 0x33a50 arm */
void utt_builder_free(utt_builder *u)
{
    utt_chunk_free_utt(&u->chunk);
    strbuf_release(&u->text);
    cst_free(u);
}

/* 0x33a74 arm */
void synth_state_free(synth_state *s)
{
    synth_ctx_free(s->ctx);
    utt_builder_free(s->builder);
    vector_free(s->flag_stack);
    vector_free(s->prosody_stack);
    vector_free(s->say_as_stack);
    vector_free(s->digit_groups_stack);
    vector_free(s->unk_1c);
    strlist_free(s->lang_stack);
    vector_free(s->unk_24);
    vector_free(s->say_as_depth_stack);
    vector_free(s->chunks);
    cst_free(s);
}

/* 0x34f70 arm */
void synth_state_pop_flags(synth_state *s)
{
    cst_dbgmsg("pop flags %d", vector_size(s->flag_stack));
    vector_pop(s->flag_stack);
    s->ctx->flags = *(int *)vector_last(s->flag_stack);
}

/* 0x34fb4 arm */
void synth_state_push_flags(synth_state *s, const int *flags)
{
    vector_push(s->flag_stack, flags);
    s->ctx->flags = *flags;
}

/* 0x34fd8 arm */
int *synth_state_flags(synth_state *s)
{
    return &s->ctx->flags;
}

/* 0x34fe4 arm */
void synth_state_pop_prosody(synth_state *s)
{
    cst_dbgmsg("pop prosody %d", vector_size(s->prosody_stack));
    vector_pop(s->prosody_stack);
    s->ctx->prosody = *(prosody *)vector_last(s->prosody_stack);
}

/* 0x3502c arm */
void synth_state_push_prosody(synth_state *s, const prosody *p)
{
    vector_push(s->prosody_stack, p);
    s->ctx->prosody = *p;
}

/* 0x35054 arm */
synth_state *synth_state_new(void *a, void *b, tts_voice *v)
{
    synth_state *s = cst_safe_calloc(1, sizeof(synth_state));
    int flags;
    prosody p;
    int zero1 = 0, zero2 = 0, zero3 = 0;

    s->ctx = synth_ctx_new(v);
    s->builder = utt_builder_new(a, b, v);
    s->say_as_stack = vector_new(4, NULL, NULL);
    s->digit_groups_stack = vector_new(4, NULL, (ivona_vector_destroy_fn)vector_free_ptr);
    s->lang_stack = strlist_new();
    flag_reset(&flags);
    s->flag_stack = vector_new(sizeof(int), NULL, NULL);
    synth_state_push_flags(s, &flags);
    prosody_reset(&p);
    s->prosody_stack = vector_new(sizeof(prosody), NULL, NULL);
    synth_state_push_prosody(s, &p);
    s->unk_1c = vector_new(4, NULL, NULL);
    vector_push(s->unk_1c, &zero1);
    s->unk_24 = vector_new(4, NULL, NULL);
    vector_push(s->unk_24, &zero2);
    s->say_as_depth_stack = vector_new(4, NULL, NULL);
    vector_push(s->say_as_depth_stack, &zero3);
    s->unk_20 = 1;
    s->say_as_count = 1;
    s->chunks = vector_new(sizeof(synth_chunk *), NULL, (ivona_vector_destroy_fn)synth_chunk_free_ptr);
    s->unk_34 = 0;
    return s;
}

/* 0x351c4 arm */
prosody *synth_state_prosody(synth_state *s)
{
    return &s->ctx->prosody;
}

/* whether any entry of v (of int) is 1 or 2 */
static inline int ints_have_1_or_2(const ivona_vector *v)
{
    const int *p;

    for (p = v->data; p != (const int *)v->data + v->count; p++)
        if (*p == 1 || *p == 2)
            return 1;
    return 0;
}

/* 0x351d0 arm */
void synth_state_push_unk_1c(synth_state *s, int x)
{
    vector_push(s->unk_1c, &x);
    s->ctx->unk_38 = x;
    s->ctx->unk_0[4] = ints_have_1_or_2(s->unk_1c);
}

/* 0x3526c arm */
void synth_state_pop_unk_1c(synth_state *s)
{
    cst_dbgmsg("pop %d", vector_size(s->unk_1c));
    vector_pop(s->unk_1c);
    s->ctx->unk_38 = *(int *)vector_last(s->unk_1c);
    s->ctx->unk_0[4] = ints_have_1_or_2(s->unk_1c);
}

/* 0x35328 arm: enters a say-as; for "characters" and "serial", detail lists the sizes of the
 * digit groups to read. */
void synth_state_push_say_as(synth_state *s, const say_as_info *sa, const char *detail)
{
    ivona_vector *groups = NULL;

    vector_push(s->say_as_stack, &sa);
    s->ctx->say_as = sa;
    if (sa && sa->name && (strcmp("characters", sa->name) == 0 || strcmp("serial", sa->name) == 0)
        && detail) {
        strlist *l = strlist_split_ws(detail);
        char **w;

        groups = vector_new(sizeof(int), NULL, NULL);
        for (w = l->items; w != l->items + l->count; w++) {
            unsigned long n;

            if (cst_parse_uint_n(*w, strlen(*w), &n) && n != 0 && (int)n >= 0) {
                int g = n;

                vector_push(groups, &g);
            }
        }
        strlist_free(l);
        if (vector_size(groups) == 0) {
            vector_free(groups);
            groups = NULL;
        }
    }
    vector_push(s->digit_groups_stack, &groups);
    vector_push(s->say_as_depth_stack, &s->say_as_count);
    s->ctx->say_as_depth = s->say_as_count;
    s->say_as_count++;
    s->ctx->digit_groups = groups;
}

/* 0x354a8 arm */
void synth_state_push_no_say_as(synth_state *s, int keep)
{
    synth_state_push_say_as(s, &no_say_as, NULL);
    if (!keep)
        s->ctx->unk_0[3]++;
}

/* 0x354e0 arm */
void synth_state_pop_say_as(synth_state *s)
{
    vector_pop(s->say_as_stack);
    s->ctx->say_as = vector_last_ptr(s->say_as_stack);
    vector_pop(s->digit_groups_stack);
    vector_pop(s->say_as_depth_stack);
    s->ctx->say_as_depth = *(int *)vector_last(s->say_as_depth_stack);
    s->ctx->digit_groups = vector_last_ptr(s->digit_groups_stack);
}

/* 0x35580 arm */
void synth_state_pop_no_say_as(synth_state *s, int keep)
{
    synth_state_pop_say_as(s);
    if (!keep && s->ctx->unk_0[3] > 0)
        s->ctx->unk_0[3]--;
}

/* 0x355b0 arm */
int synth_state_say_as_flag(synth_state *s)
{
    if (s->ctx->say_as)
        return say_as_info_flag(s->ctx->say_as) != 0;
    return 1;
}

/* 0x355f0 arm */
void synth_state_push_lang(synth_state *s, const char *lang)
{
    strlist *l;

    strlist_push(s->lang_stack, lang);
    l = s->lang_stack;
    s->ctx->unk_34 = l->count ? l->items[l->count - 1] : NULL;
}

/* 0x35624 arm */
void synth_state_pop_lang(synth_state *s)
{
    strlist *l;

    strlist_pop(s->lang_stack);
    l = s->lang_stack;
    s->ctx->unk_34 = l->count ? l->items[l->count - 1] : NULL;
}

/* 0x35658 arm */
void synth_state_set_voice(synth_state *s, tts_voice *v)
{
    utt_builder *b;

    if (s->ctx->voice == v)
        return;
    synth_state_flush(s);
    b = s->builder;
    if (b->unk_44) {
        b->unk_10[3] = 1;
        b->unk_c = 1;
    } else
        b->chunk.src = v;
    s->ctx->voice = v;
}

/* 0x356a0 arm */
void synth_state_set_unk_14(synth_state *s, int x)
{
    s->ctx->unk_0[5] = x;
}

/* 0x356ac arm */
void synth_state_set_unk_1c(synth_state *s, int x)
{
    if (s->ctx->unk_1c == x)
        return;
    synth_state_flush(s);
    s->ctx->unk_1c = x;
}
