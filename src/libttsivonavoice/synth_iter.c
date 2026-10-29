/*
 * The synthesis state of an SSML document walk (..0x356d8): the context of the text being read
 * (flags, prosody, voice), the stacks they are saved on, and the utterance being built.
 */
#include <string.h>
#include <strings.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "cst_error.h"
#include "cst_features.h"
#include "cst_utterance.h"
#include "cst_wave.h"
#include "ivona_ustr.h"
#include "ivona_util.h"
#include "ivona_vector.h"
#include "tts_voice.h"
#include "ssml_doc.h"

#define REL_TOKEN 1

typedef struct lex_ctx lex_ctx;
lex_ctx *lex_ctx_new(void *voice);
void lex_ctx_free(lex_ctx *c);
void lex_ctx_set_voice(lex_ctx *c, void *voice);
void lex_ctx_clear_marks(lex_ctx *c);
void lex_ctx_push_mark(lex_ctx *c, int mark);
void lex_ctx_set_ctx(lex_ctx *c, void *ctx);
tts_lang_desc *tts_language_table(tts_instance *tts);
tts_lang_desc *lang_desc_find(tts_lang_desc *table, const char *code);
void *lang_part_data(tts_lang_desc *lang, const char *name);
const char *dom_get_attr(const dom_elem *e, const char *name);

typedef struct synth_chunk synth_chunk;
void synth_chunk_free(synth_chunk *c);
void flag_reset(int *f);
int cst_parse_uint_n(const char *s, size_t n, unsigned long *out);
const char *cst_skip_space(const char *s);

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

#include "ssml_say_as.h"

int say_as_info_flag(const say_as_info *sa);

/* language core data */
typedef struct lang_core {
    const char *code;
    int unk_4[4];
    cst_features *features;     /* +0x14 */
    cst_features *params;       /* +0x18 */
} lang_core;

const char *utf8_last_char(const char *s);
const char *utf8_strchr_str(const char *s, const char *ch);

/* the context text is read in (copied into every chunk) */
typedef struct synth_ctx {
    int unk_0[6];
    tts_voice *voice;           /* +0x18 */
    lang_core *lang;            /* +0x1c: of the text, NULL for the voice's */
    int flags;                  /* +0x20 */
    prosody prosody;            /* +0x24 */
    ivona_vector *digit_groups; /* +0x30: of int, for say-as characters/serial */
    char *unk_34;
    int unk_38;
    int block_id;               /* +0x3c */
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
    cst_item *last_item;        /* +0x44 */
    int last_end;               /* +0x48: input offset after the last item */
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
cst_item *utt_builder_add_item(synth_ctx *ctx, utt_builder *b, int kind);
void utt_builder_item_added(synth_ctx *ctx, cst_item *prev);
int utt_builder_check_size(synth_ctx *ctx, utt_builder *b, int n);
void utt_chunk_add_span(utt_chunk *c, const ustr_span *s);
typedef struct audio_clip_chunk audio_clip_chunk;
audio_clip_chunk *audio_clip_chunk_new(const synth_ctx *ctx, const char *src, const char *alt,
                                       const ustr_span *span, int flags, const char *mark);

static const say_as_info no_say_as = { NULL, 0, 0 };

#define TTS_MSG_ADD_AT(list, line, column, code, ...)                                 \
    do {                                                                              \
        tts_msg **tail_ = (list) ? &tts_msg_last(list)->next : &(list);               \
                                                                                      \
        *tail_ = tts_msg_new((line), (column), (code), __VA_ARGS__);                  \
    } while (0)

/* a voice in effect */
typedef struct voice_frame {
    tts_voice *voice;
    int unk_4;
    int unk_8;
} voice_frame;

typedef struct synth_iter_src {
    int unk_0;
    void **unk_4;
} synth_iter_src;

/* walks a document's DOM, producing synthesis chunks */
typedef struct synth_iter {
    ssml_doc *doc;
    synth_iter_src *unk_4;      /* +0x04 */
    float max_break;            /* +0x08: eng_max_ssml_break_time, seconds */
    ivona_vector *voices;       /* +0x0c: of tts_voice * */
    synth_state *state;         /* +0x10 */
    ivona_vector *unk_14;       /* of 12 bytes */
    ivona_vector *unk_18;       /* of 8 bytes */
    lex_ctx *lex;               /* +0x1c */
    int unk_20;
    int unk_24;
    ivona_vector *voice_stack;  /* +0x28: of voice_frame */
    strlist *unk_2c;
} synth_iter;

void synth_state_set_voice(synth_state *s, tts_voice *v);
synth_state *synth_state_new(void *a, void *b, tts_voice *v);
void synth_state_free(synth_state *s);

static inline void *vector_last(const ivona_vector *v)
{
    return v->count ? vector_at(v, v->count - 1) : NULL;
}

/* the last element of v, a vector of pointers (NULL if empty) */
static inline void *vector_last_ptr(const ivona_vector *v)
{
    return v->count ? *(void **)vector_at(v, v->count - 1) : NULL;
}

/* 0x2d798 arm: the lexicons of v and its PLS lexicons for c */
void lex_ctx_load_voice(lex_ctx **c, tts_voice *v)
{
    lang_part *vox;
    ivona_vector *marks;
    lex_ctx *lc;
    size_t i;

    lex_ctx_clear_marks(*c);
    lex_ctx_set_voice(*c, v);
    lc = *c;
    vox = VOICE_DATA(v)->vox;
    if (vox && (marks = (ivona_vector *)vox->unk_8[0]) != NULL)
        for (i = vector_size(marks); i > 0;) {
            i--;
            lex_ctx_push_mark(lc, *(int *)vector_at(marks, i));
        }
    marks = VOICE_DATA(v)->pls;
    for (i = vector_size(marks); i > 0;) {
        i--;
        lex_ctx_push_mark(*c, ((int *)vector_at(marks, i))[1]);
    }
    vox = VOICE_DATA(v)->vox;
    if (vox)
        lex_ctx_set_ctx(*c, (void *)vox->unk_8[1]);
    else
        lex_ctx_set_ctx(*c, NULL);
}

/* 0x2d71c arm */
int say_as_info_flag(const say_as_info *sa)
{
    if (sa == NULL)
        return 0;
    return sa->whole != 0;
}

/* 0x2d734 arm: whether the say-as reads its text as speech */
int say_as_is_speech(const say_as_info *sa)
{
    const char *name;

    if (sa == NULL)
        return 0;
    name = sa->name;
    if (name == NULL)
        return 0;
    if (strcmp("social-speech", name) == 0)
        return 1;
    return strcmp("interjection", name) == 0;
}

/* 0x2d878 arm: the element's apply-pls attribute (default 1) */
int ssml_apply_pls_attr(synth_iter *it, dom_elem *e)
{
    const char *v = dom_get_attr(e, "apply-pls");
    int ret = 1;

    if (v == NULL || strcmp("1", v) == 0 || strcmp("true", v) == 0)
        goto out;
    if ((ret = strcmp("0", v)) == 0 || (ret = strcmp("false", v)) == 0)
        goto out;
    TTS_MSG_ADD_AT(it->doc->warnings, e->span.start, e->n.index, 6,
                   "Invalid value of attribute \"%s\"", "apply-pls");
    ret = 1;
out:
    return ret;
}

/* 0x2d980 arm: warns about unknown tokens in the attribute attr of a <voice> (e.g.
 * "required") */
void ssml_check_voice_attr_list(synth_iter *it, dom_elem *e, const char *attr)
{
    const char *v = dom_get_attr(e, attr);
    strlist *l;
    char **w;

    if (v == NULL || *v == 0)
        return;
    l = strlist_split_ws(v);
    for (w = l->items; w != l->items + l->count; w++) {
        if (strcasecmp(*w, "gender") == 0 || strcasecmp(*w, "age") == 0
            || strcasecmp(*w, "variant") == 0 || strcasecmp(*w, "name") == 0
            || strcasecmp(*w, "languages") == 0)
            continue;
        TTS_MSG_ADD_AT(it->doc->warnings, e->span.start, e->n.index, 6,
                       "Invalid value of attribute \"%s\"", attr);
    }
    strlist_free(l);
}

/* 0x2dfec arm: the language part for lang, NULL if it is the voice's own */
void *synth_iter_lang(synth_iter *it, const char *lang)
{
    size_t n = vector_size(it->voice_stack);
    tts_voice *v = ((voice_frame *)vector_at(it->voice_stack, n - 1))->voice;
    const char *code = get_param_string(VOICE_FEATURES(v), "language_code", NULL);
    tts_lang_desc *desc;

    if (lang == NULL || strcmp(lang, code) == 0)
        return NULL;
    desc = VOICE_DATA(v)->lang;
    if (desc == NULL)
        desc = lang_desc_find(tts_language_table(v->tts), lang);
    return lang_part_data(desc, lang);
}

/* 0x2ed64 arm */
synth_iter *synth_iter_new(synth_iter_src *a, ivona_vector *voices, ssml_doc *doc, void *b)
{
    tts_voice *v = *(tts_voice **)vector_at(voices, 0);
    synth_iter *it = cst_safe_calloc(1, sizeof(synth_iter));
    float f;

    it->unk_4 = a;
    it->voices = voices;
    it->doc = doc;
    it->unk_20 = 0;
    it->state = synth_state_new(doc, b, v);
    it->unk_14 = vector_new(12, NULL, NULL);
    it->unk_18 = vector_new(8, NULL, NULL);
    if (a->unk_4[0]) {
        struct {
            void *p;
            int n;
        } e;

        e.p = a->unk_4[0];
        e.n = 0;
        vector_push(it->unk_18, &e);
    }
    it->lex = lex_ctx_new(v);
    it->unk_24 = 0;
    it->voice_stack = vector_new(sizeof(voice_frame), NULL, NULL);
    {
        voice_frame fr;

        fr.voice = v;
        fr.unk_4 = 0;
        fr.unk_8 = 0;
        vector_push(it->voice_stack, &fr);
    }
    synth_state_set_voice(it->state, v);
    lex_ctx_load_voice(&it->lex, v);
    f = get_param_int(VOICE_FEATURES(v), "eng_max_ssml_break_time", 1000);
    if (!(f < 0 || f > 120.000397f))
        it->max_break = f;
    else
        it->max_break = 120.000397f;
    it->unk_2c = strlist_new();
    return it;
}

/* 0x2eed0 arm */
void synth_iter_free(synth_iter *it)
{
    if (it == NULL)
        return;
    lex_ctx_free(it->lex);
    vector_free(it->unk_18);
    synth_state_free(it->state);
    vector_free(it->unk_14);
    vector_free(it->voice_stack);
    strlist_free(it->unk_2c);
    cst_free(it);
}

/* 0x3134c arm */
strlist *synth_iter_ctx(synth_iter *it)
{
    return it->unk_2c;
}

/* 0x31354 arm: the decimal number s, or -1 */
int parse_uint_or_minus1(const char *s)
{
    unsigned long v;

    if (!cst_parse_uint_n(s, strlen(s), &v))
        return -1;
    return v;
}

/* 0x31388 arm: the voice_age feature. In the target the call to feat_string is not a sibcall
 * (engine code never tail-calls feat_string/get_param_string, Flite code does: the engine's
 * prototypes for them probably differed). */
const char *voice_age(tts_voice *v)
{
    cst_features *f = VOICE_FEATURES(v);

    if (!feat_present(f, "voice_age"))
        return "Unspecified";
    return feat_string(f, "voice_age");
}

/* voice age classes: by name or by years (-1: open) */
typedef struct age_class {
    int mask;
    const char *name;
    int min;
    int max;
} age_class;

age_class voice_age_classes[] = {
    { 1, "child", 1, 15 },
    { 2, "teen", 9, 19 },
    { 4, "adult", 18, -1 },
    { 8, "senior", 45, -1 },
    { 0, 0, -1, -1 },
};

/* 0x313d8 arm: the classes (mask) an age ("adult", "30") belongs to */
int voice_age_class(const char *age)
{
    const age_class *c;
    int mask = 0;

    for (c = voice_age_classes; c->mask; c++) {
        int n;

        if (strcasecmp(c->name, age) == 0
            || ((n = parse_uint_or_minus1(age)) != -1 && (c->min == -1 || n >= c->min)
                && (c->max == -1 || n <= c->max)))
            mask |= c->mask;
    }
    return mask;
}

/* 0x31478 arm */
int voice_age_matches(tts_voice *v, int mask)
{
    return voice_age_class(voice_age(v)) & mask;
}

/* 0x31490 arm (it clears the first pointer twice) */
void str_pair_free(char **p)
{
    if (p[0]) {
        cst_free(p[0]);
        p[0] = NULL;
    }
    if (p[1]) {
        cst_free(p[1]);
        p[0] = NULL;
    }
}

/* 0x314cc arm */
static void synth_chunk_free_ptr(void *p)
{
    synth_chunk_free(*(synth_chunk **)p);
}

/* the language of the context */
static inline lang_core *ctx_lang(const synth_ctx *c)
{
    return c->lang ? c->lang : VOICE_DATA(c->voice)->reg->lang_data;
}

/* whether s ends in one of the characters of set */
static inline int ends_in(const synth_ctx *c, const char *s, const char *set)
{
    size_t n;

    if (s == NULL || (n = strlen(s)) == 0)
        return 0;
    if (strcmp(get_param_string(ctx_lang(c)->features, "encoding", ""), "UTF-8") == 0)
        return utf8_strchr_str(set, utf8_last_char(s)) != NULL;
    return strchr(set, s[n - 1]) != NULL;
}

/* 0x31754 arm: gives the token before a break a pause if its punctuation has none */
void utt_builder_item_added(synth_ctx *c, cst_item *prev)
{
    const char *punc;

    if (prev == NULL)
        return;
    punc = item_feat_string(prev, "punc");
    if (ends_in(c, punc, get_param_string(ctx_lang(c)->features, "hard_punc", ".!?")))
        return;
    if (ends_in(c, punc, get_param_string(ctx_lang(c)->features, "soft_punc", ",:;")))
        return;
    item_set_string(prev, "punc", ",");
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
    c->lang = NULL;
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
    u->last_item = NULL;
    u->last_end = 0;
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

/* 0x34104 arm */
void synth_state_enter_block(synth_state *s)
{
    s->ctx->unk_0[2]++;
}

/* 0x34118 arm */
void synth_state_leave_block(synth_state *s)
{
    s->ctx->unk_0[2]--;
}

/* 0x3412c arm */
int synth_state_last_end(synth_state *s)
{
    synth_state_flush(s);
    return s->builder->last_end;
}

/* 0x34414 arm */
int synth_state_check_size(synth_state *s, int n)
{
    synth_state_flush(s);
    return utt_builder_check_size(s->ctx, s->builder, n);
}

/* 0x34434 arm */
void synth_state_push_block(synth_state *s)
{
    synth_state_enter_block(s);
    vector_push(s->unk_24, &s->unk_20);
    s->ctx->block_id = s->unk_20;
    s->unk_20++;
}

/* 0x34464 arm */
void synth_state_pop_block(synth_state *s)
{
    synth_state_leave_block(s);
    vector_pop(s->unk_24);
    s->ctx->block_id = *(int *)vector_last(s->unk_24);
}

/* 0x34b10 arm */
void synth_state_add_audio(synth_state *s, const ustr_span *span, const char *src, int flags,
                           const char *mark, const char *alt)
{
    audio_clip_chunk *c = audio_clip_chunk_new(s->ctx, src, alt, span, flags, mark);

    vector_push(s->chunks, &c);
}

/* 0x34e70 arm: adds a token holding an audio wave */
void synth_state_add_wave(synth_state *s, const ustr_span *span, cst_wave *w)
{
    cst_item *item;
    utt_builder *b;

    synth_state_flush(s);
    item = utt_builder_add_item(s->ctx, s->builder, 1);
    item_set(item, "wave", wave_val(w));
    item_set_int(item, "start_pos", span->start);
    item_set_int(item, "end_pos", span->end);
    item_set_string(item, "name", "");
    item_set_string(item, "punc", "");
    b = s->builder;
    b->last_item = item;
    b->last_end = span->end;
    b->unk_10[5]++;
    b->unk_10[4] = 0;
    utt_chunk_add_span(&b->chunk, span);
    utt_builder_item_added(s->ctx, item_prev(item));
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

/* 0x355dc arm */
int synth_state_unk_c_zero(synth_state *s)
{
    return s->ctx->unk_0[3] == 0;
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
    if (b->last_item) {
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
void synth_state_set_lang(synth_state *s, lang_core *l)
{
    if (s->ctx->lang == l)
        return;
    synth_state_flush(s);
    s->ctx->lang = l;
}
