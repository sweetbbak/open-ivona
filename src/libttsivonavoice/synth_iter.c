/*
 * The synthesis state of an SSML document walk (..0x356d8): the context of the text being read
 * (flags, prosody, voice), the stacks they are saved on, and the utterance being built.
 */
#include <ctype.h>
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

#include "synth_chunk.h"
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
void utt_chunk_add_span(utt_chunk *c, const ustr_span *s);
audio_clip_chunk *audio_clip_chunk_new(const synth_ctx *ctx, const char *src, const char *alt,
                                       const ustr_span *span, int flags, const char *mark);

static const say_as_info no_say_as = { NULL, 0, 0 };

const char *voice_age(tts_voice *v);
int parse_uint_or_minus1(const char *s);
int voice_age_class(const char *age);
int voice_age_matches(tts_voice *v, int mask);

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

/* language match levels of a voice for a language (4: exact) */
typedef int (*voice_lang_match_fn)(const char *codes, const char *lang);

/* 0x2db0c arm: appends the voices whose languages match lang exactly, then (unless exact)
 * the closer matches before the looser ones */
static void voices_by_lang(const ivona_vector *voices, const char *lang, voice_lang_match_fn match,
                           int exact, ivona_vector *out)
{
    ivona_vector *v3 = NULL, *v2 = NULL, *v1 = NULL;
    tts_voice **p;
    int m;

    for (p = voices->data; p != (tts_voice **)voices->data + voices->count; p++) {
        m = match((*p)->voice->lang_codes, lang);
        if (m == 4)
            vector_push(out, p);
        else if (!exact) {
            if (m == 3) {
                if (v3 == NULL)
                    v3 = vector_new(4, NULL, NULL);
                vector_push(v3, p);
            } else if (m == 2) {
                if (v2 == NULL)
                    v2 = vector_new(4, NULL, NULL);
                vector_push(v2, p);
            } else if (m == 1) {
                if (v1 == NULL)
                    v1 = vector_new(4, NULL, NULL);
                vector_push(v1, p);
            }
        }
    }
    if (v3) {
        vector_append_vector(out, v3);
        vector_free(v3);
    }
    if (v2) {
        vector_append_vector(out, v2);
        vector_free(v2);
    }
    if (v1) {
        vector_append_vector(out, v1);
        vector_free(v1);
    }
}

char *str_to_ascii(const char *s);

/* 0x2dcc0 arm: appends the voices named in names (white space separated, in that order) */
static void voices_by_name(const ivona_vector *voices, const char *names, ivona_vector *out)
{
    char *ascii = str_to_ascii(names);
    strlist *l = strlist_split_ws(ascii);
    char **w;
    tts_voice **p;

    for (w = l->items; w != l->items + l->count; w++)
        for (p = voices->data; p != (tts_voice **)voices->data + voices->count; p++) {
            const char *name = (*p)->voice->lector_ascii;

            if (*w == NULL || **w == 0 || strcasecmp(*w, name) == 0)
                vector_push(out, p);
        }
    strlist_free(l);
    cst_free(ascii);
}

/* how far a voice's age is from age (0: the same) */
static inline float voice_age_distance(tts_voice *v, int age)
{
    int a = parse_uint_or_minus1(voice_age(v));

    return 1.0f - (age < a ? (float)age / a : (float)a / age);
}

/* 0x2ddac arm: inserts v into out, sorted by how close its age is to age (appended if age
 * is not a number) */
static void voice_insert_by_age(ivona_vector *out, tts_voice *v, int age)
{
    float d;
    size_t i;

    if (age > 0) {
        d = voice_age_distance(v, age);
        for (i = 0; i < vector_size(out); i++) {
            if (d < voice_age_distance(*(tts_voice **)vector_at(out, i), age)) {
                vector_insert(out, i, &v);
                return;
            }
        }
    }
    vector_push(out, &v);
}

/* 0x2df00 arm: appends the voices of the age class of age, closest age first; the others only
 * if none is */
static void voices_by_age(const ivona_vector *voices, const char *age, ivona_vector *out)
{
    int mask = voice_age_class(age);
    int n = parse_uint_or_minus1(age);
    ivona_vector *others = NULL;
    tts_voice **p;

    for (p = voices->data; p != (tts_voice **)voices->data + voices->count; p++) {
        if (voice_age_matches(*p, mask))
            voice_insert_by_age(out, *p, n);
        else {
            if (others == NULL)
                others = vector_new(4, NULL, NULL);
            voice_insert_by_age(others, *p, n);
        }
    }
    if (others == NULL)
        return;
    if (vector_size(out) == 0)
        vector_append_vector(out, others);
    vector_free(others);
}

#define XML_LANG "http://www.w3.org/XML/1998/namespace^lang"

int lang_tag_match(const char *a, const char *b);
int lang_tags_match(const char *a, const char *list);
const char *dom_find_attr_up(const dom_elem *e, int type, const char *name);

/* the attribute name (lowercased) of e or its nearest <voice> ancestor */
static inline const char *voice_attr(const dom_elem *e, const char *name)
{
    char *n = cst_strdup(name), *p;
    const char *v;

    for (p = n; *p; p++)
        *p = tolower(*p);
    v = dom_find_attr_up(e, SSML_VOICE, n);
    cst_free(n);
    return v;
}

/* appends the voices of cur matching the <voice> attribute attr = value to out (with exact,
 * only exact language matches) */
#define VOICES_FILTER(cur, attr, value, exact, out)                                      \
    do {                                                                                 \
        tts_voice **p_;                                                                  \
                                                                                         \
        if (strcasecmp(attr, "name") == 0)                                               \
            voices_by_name(cur, value, out);                                             \
        else if (strcasecmp(attr, "gender") == 0) {                                      \
            for (p_ = (cur)->data; p_ != (tts_voice **)(cur)->data + (cur)->count; p_++) \
                if (*(value) == 0 || strcasecmp(value, (*p_)->voice->gender) == 0)       \
                    vector_push(out, p_);                                                \
        } else if (strcasecmp(attr, "age") == 0)                                         \
            voices_by_age(cur, value, out);                                              \
        else if (strcasecmp(attr, "languages") == 0)                                     \
            voices_by_lang(cur, value, lang_tags_match, exact, out);                     \
        else if (strcasecmp(attr, XML_LANG) == 0)                                        \
            voices_by_lang(cur, value, lang_tag_match, exact, out);                      \
    } while (0)

/* the value of the <voice> attribute attr (xml:lang is inherited) */
static inline const char *voice_attr_value(const dom_elem *e, const char *attr)
{
    if (strcmp(attr, XML_LANG) == 0)
        return dom_inherited_attr(e, attr);
    return voice_attr(e, attr);
}

/* 0x2e084 arm: the voice a <voice> element (or, with by_lang, a language change) selects:
 * the voices meeting all of its required attributes (by onvoicefailure if none does),
 * narrowed in its ordering of attributes, then its variant-th */
tts_voice *ssml_select_voice(synth_iter *it, dom_elem *e, int by_lang)
{
    const char *required = voice_attr(e, "required"), *lang, *ordering, *value, *ovf,
        *variant;
    const char *order[5];
    ivona_vector *cur = vector_copy(it->voices), *next;
    unsigned long n = 1;
    strlist *l;
    char **w;
    const char **o;
    tts_voice *v, *top;

    if (it->doc->ssml_version == 10 && voice_attr(e, "languages") == NULL)
        lang = XML_LANG;
    else
        lang = "languages";
    if (required == NULL)
        required = lang;
    if (by_lang)
        required = XML_LANG;
    if (*required) {
        l = strlist_split_ws(required);
        for (w = l->items; w != l->items + l->count; w++) {
            value = voice_attr_value(e, *w);
            if (value == NULL)
                continue;
            next = vector_new(4, NULL, NULL);
            VOICES_FILTER(cur, *w, value, 1, next);
            vector_free(cur);
            cur = next;
            if (vector_size(cur) == 0)
                break;
        }
        strlist_free(l);
    }
    if (vector_size(cur) == 0) {
        ovf = voice_attr(e, "onvoicefailure");
        TTS_MSG_ADD_AT(it->doc->warnings, e->span.start, e->n.index, 14,
                       "None of the voices match required criteria");
        if (ovf == NULL || strcasecmp(ovf, "processorchoice") == 0)
            ovf = "priorityselect";
        if (strcasecmp(ovf, "keepexisting") == 0) {
            top = ((voice_frame *)vector_at(it->voice_stack, vector_size(it->voice_stack) - 1))->voice;
            vector_push(cur, &top);
        } else if (strcasecmp(ovf, "priorityselect") != 0)
            TTS_MSG_ADD_AT(it->doc->warnings, e->span.start, e->n.index, 6,
                           "Invalid value of attribute \"%s\"", "onvoicefailure");
        if (vector_size(cur) == 0)
            vector_append_vector(cur, it->voices);
    }
    if (vector_size(cur) > 1) {
        ordering = voice_attr(e, "ordering");
        order[0] = lang;
        order[1] = "gender";
        order[2] = "name";
        order[3] = "age";
        order[4] = NULL;
        l = strlist_split_ws(ordering ? ordering : lang);
        for (o = order; *o; o++)
            if (!strlist_contains_nocase(l, *o))
                strlist_push(l, *o);
        for (w = l->items; w != l->items + l->count; w++) {
            value = voice_attr_value(e, *w);
            if (value == NULL)
                continue;
            next = vector_new(4, NULL, NULL);
            VOICES_FILTER(cur, *w, value, 0, next);
            if (vector_size(next) == 0)
                vector_free(next);
            else {
                vector_free(cur);
                cur = next;
            }
        }
        strlist_free(l);
    }
    variant = voice_attr(e, "variant");
    if (variant && *variant && (!cst_parse_uint_n(variant, strlen(variant), &n) || (int)n <= 0)) {
        TTS_MSG_ADD_AT(it->doc->warnings, e->span.start, e->n.index, 6,
                       "Invalid value of attribute \"%s\"", "variant");
        dom_set_attr(e, "variant", "1");
        n = 1;
    }
    v = *(tts_voice **)vector_at(cur, (n - 1) % vector_size(cur));
    vector_free(cur);
    if (e->in_subdoc) {
        top = ((voice_frame *)vector_at(it->voice_stack, vector_size(it->voice_stack) - 1))->voice;
        if (top != v) {
            top = ((voice_frame *)vector_at(it->voice_stack, vector_size(it->voice_stack) - 1))->voice;
            TTS_MSG_ADD_AT(it->doc->warnings, e->span.start, e->n.index, 14,
                           "It is not possible to change voice within SSML subdocument");
            v = top;
        }
    }
    return v;
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
/* the tokens of an utterance chunk (utt_chunk.c) */
typedef struct token_chunk {
    void *head;
    void *tail;
    ustr_span span;             /* +0x08 */
    int phrasing_model;         /* +0x10 */
    int break_dur_model;        /* +0x14 */
    void *src;                  /* +0x18 */
} token_chunk;
void token_chunk_from_utt(const utt_chunk *c, token_chunk *tc, void *ctx);

#define UTT_BUILDER_FLUSH(v, b)                                                         \
    do {                                                                                \
        tts_voice **v_ = (v);                                                           \
        utt_builder *b_ = (b);                                                        \
        token_chunk tc_;                                                                \
        float pause_;                                                                   \
        cst_item *t_;                                                                   \
                                                                                        \
        b_->unk_c = 0;                                                                   \
        if (relation_head(b_->tokens)) {                                                 \
            if (b_->unk_10[2] && (pause_ = b_->paragraph_break) > 0) {                    \
                t_ = relation_append(b_->tokens, NULL);                                  \
                item_set_string(t_, "name", "");                                        \
                item_set_int(t_, "start_pos", -1);                                      \
                item_set_int(t_, "end_pos", -1);                                        \
                item_set_string(t_, "punc", "");                                        \
                item_set_float(t_, "silence", pause_);                                  \
                b_->unk_8 = 0;                                                           \
            }                                                                           \
            token_chunk_from_utt(&b_->chunk, &tc_, b_->unk_0);                            \
            vector_push(b_->unk_4, &tc_);                                                \
            utt_chunk_free_utt(&b_->chunk);                                              \
            utt_chunk_init_empty(&b_->chunk, (tts_instance **)*v_, b_->chunk.phrasing_model,\
                                 b_->chunk.break_dur_model);                             \
            b_->tokens = utt_relation(b_->chunk.utt, REL_TOKEN);                          \
            b_->last_item = NULL;                                                        \
            b_->unk_10[5] = 0;                                                           \
        }                                                                               \
        b_->unk_10[2] = 0;                                                               \
    } while (0)

/* 0x314d4 arm: turns the utterance built so far into a token chunk (ending a paragraph
 * with its pause) and starts a new one */
void utt_builder_flush(tts_voice **v, utt_builder *b)
{
    UTT_BUILDER_FLUSH(v, b);

}

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

/* 0x31620 arm: ends a sentence at prev (gives it a "." unless its punctuation ends one) */
void utt_builder_end_sentence(synth_ctx *c, cst_item *prev)
{
    const char *punc;

    if (prev == NULL)
        return;
    punc = item_feat_string(prev, "punc");
    if (ends_in(c, punc, get_param_string(ctx_lang(c)->features, "hard_punc", ".!?")))
        return;
    item_set_string(prev, "punc", ".");
}

/* utt_builder_end_sentence, inline */
#define END_SENTENCE(c, prev)                                                                 \
    do {                                                                                      \
        synth_ctx *c_ = (c);                                                                  \
        cst_item *p_ = (prev);                                                                \
                                                                                              \
        if (p_ && !ends_in(c_, item_feat_string(p_, "punc"),                                  \
                           get_param_string(ctx_lang(c_)->features, "hard_punc", ".!?")))     \
            item_set_string(p_, "punc", ".");                                                 \
    } while (0)

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

cst_utterance *utt_split_tokens(cst_item *t);
void utt_chunk_init(utt_chunk *c, cst_utterance *utt, tts_instance **src, int phrasing_model,
                    int break_dur_model);
size_t utf8_strlen(const char *s);

/* 0x31978 arm: makes room for n more characters in the utterance: splits it after the last
 * punctuated token that allows it (the rest becomes a chunk), or flushes it */
void utt_builder_check_size(synth_ctx *c, utt_builder *b, int n)
{
    token_chunk tc;
    cst_item *t, *i;
    cst_utterance *u;
    const char *s;
    int utf8, len;

    if (b->max_sentence < n + b->unk_10[5]) {
        for (t = relation_tail(b->tokens); t; t = item_prev(t)) {
            s = item_feat_string(t, "punc");
            if (s == NULL || *s == 0 || (u = utt_split_tokens(t)) == NULL)
                continue;
            b->chunk.span.end = item_feat_int(t, "end_pos");
            token_chunk_from_utt(&b->chunk, &tc, b->unk_0);
            vector_push(b->unk_4, &tc);
            utt_chunk_free_utt(&b->chunk);
            utt_chunk_init(&b->chunk, u, (tts_instance **)c->voice, b->chunk.phrasing_model,
                           b->chunk.break_dur_model);
            b->tokens = utt_relation(u, REL_TOKEN);
            utf8 = strcmp(get_param_string(ctx_lang(c)->features, "encoding", ""), "UTF-8");
            len = 0;
            for (i = relation_head(b->tokens); i; i = item_next(i)) {
                if (item_feat_present(i, "name")) {
                    s = item_feat_string(i, "name");
                    len += utf8 == 0 ? utf8_strlen(s) : strlen(s);
                }
                if (item_feat_present(i, "phones"))
                    len += strlen(item_feat_string(i, "phones"));
                if (item_feat_present(i, "xsampa"))
                    len += strlen(item_feat_string(i, "xsampa"));
                len++;
            }
            b->unk_10[5] = len;
            b->last_item = relation_tail(b->tokens);
            break;
        }
        if (b->max_sentence < n + b->unk_10[5])
            utt_builder_flush(&c->voice, b);
    }
}

#define ASCII_ALPHA(ch) ((unsigned char)(((ch) & ~0x20) - 'A') < 26)

/* 0x31be8 arm: whether a sentence ends after token t (by the language's
 * sentence_boundary_func, else by its punctuation; "A.B." abbreviations do not) */
int utt_is_sentence_end(synth_ctx *c, cst_item *t)
{
    const cst_val *f = get_param_val(ctx_lang(c)->features, "sentence_boundary_func", NULL);
    const char *punc, *name, *p;
    cst_val *v;
    int r;

    if (f) {
        v = ((cst_val *(*)(cst_item *))val_itemfunc(f))(t);
        if (v == NULL)
            return 0;
        r = val_int(v);
        delete_val(v);
        return r;
    }
    punc = item_feat_string(t, "punc");
    if (punc == NULL)
        return 0;
    if (strcmp(punc, ".") == 0 && item_feat_present(t, "name")) {
        name = item_feat_string(t, "name");
        if (ASCII_ALPHA(name[0]) && name[1] != 0 && name[1] == '.') {
            for (p = name; ASCII_ALPHA(p[2]); p += 2) {
                if (p[3] == 0)
                    return p + 3 - name <= 2;
                if (p[3] != '.')
                    break;
            }
        }
        return 1;
    }
    return ends_in(c, punc, get_param_string(ctx_lang(c)->features, "hard_punc", ".!?"));
}

cst_val *string_val_consume(char *s);

/* strips the sentence-ending punctuation (but hard_non_strip_punc, "!?") off the last token
 * when a sentence is being merged on */
static inline void utt_builder_strip_end(synth_ctx *c, utt_builder *b)
{
    cst_item *t = b->last_item;
    const char *keep, *punc;
    char *buf = NULL;
    int utf8, k;

    if (!item_feat_present(t, "punc") || !utt_is_sentence_end(c, t))
        return;
    keep = get_param_string(ctx_lang(c)->features, "hard_non_strip_punc", "!?");
    punc = item_feat_string(t, "punc");
    utf8 = strcmp(get_param_string(ctx_lang(c)->features, "encoding", ""), "UTF-8");
    while (utt_is_sentence_end(c, t)) {
        if (ends_in(c, punc, keep))
            break;
        if (utf8 == 0)
            k = utf8_last_char(punc) - punc;
        else
            k = strlen(punc) - 1;
        if (buf == NULL) {
            punc = buf = cst_substr(punc, 0, k);
            item_set(t, "punc", string_val_consume(buf));
        }
        buf[k] = 0;
    }
}

/* appends a new token (after the pause of a sentence break if one is due) */
static inline cst_item *utt_builder_append(synth_ctx *c, utt_builder *b)
{
    float pause;
    cst_item *t;

    if (vector_size(b->unk_4) && relation_head(b->tokens) == NULL && (pause = b->sentence_break) > 0) {
        if (b->unk_10[0] == 0) {
            t = relation_append(b->tokens, NULL);
            item_set_string(t, "name", "");
            item_set_int(t, "start_pos", -1);
            item_set_int(t, "end_pos", -1);
            item_set_string(t, "punc", "");
            item_set_float(t, "silence", pause);
            b->unk_8 = 0;
        } else
            b->unk_10[0] = 0;
    }
    b->unk_8 = 0;
    t = relation_append(b->tokens, NULL);
    item_set_string(t, "whitespace", b->text.data ? b->text.data : "");
    strbuf_clear(&b->text);
    if (c->block_id)
        item_set_int(t, "w_tag_id", c->block_id);
    if (c->say_as_depth)
        item_set_int(t, "sayas_tag_id", c->say_as_depth);
    if (b->unk_10[1]) {
        item_set_int(t, "no_pause_start", 1);
        b->unk_10[1] = 0;
    }
    return t;
}

/* 0x31e1c arm: adds a pause token covering span */
void utt_builder_add_pause(synth_ctx *c, utt_builder *b, const ustr_span *span, float silence)
{
    int saved = b->unk_c;
    cst_item *t;

    b->unk_c = 0;
    if ((c->unk_0[1] || c->unk_0[2]) && b->unk_8 == 0)
        utt_builder_strip_end(c, b);
    if (relation_head(b->tokens))
        utt_builder_check_size(c, b, 1);
    t = utt_builder_append(c, b);
    if (item_prev(t))
        b->unk_c = saved;
    item_set_string(t, "name", "");
    item_set_int(t, "start_pos", span->start);
    item_set_int(t, "end_pos", span->end);
    item_set_string(t, "punc", "");
    item_set_float(t, "silence", silence);
    b->last_end = span->end;
    utt_builder_item_added(c, item_prev(t));
    b->unk_10[5]++;
}

/* 0x32338 arm: adds a token for n characters of text (starting a new utterance first if one
 * is due) */
cst_item *utt_builder_add_item(synth_ctx *c, utt_builder *b, int n)
{
    if (b->unk_c) {
        if (b->unk_10[3]) {
            utt_builder_item_added(c, b->last_item);
            b->unk_10[3] = 0;
        }
        utt_builder_flush(&c->voice, b);
    } else {
        if ((c->unk_0[1] || c->unk_0[2]) && b->unk_8 == 0)
            utt_builder_strip_end(c, b);
        if (n && relation_head(b->tokens))
            utt_builder_check_size(c, b, n);
    }
    return utt_builder_append(c, b);
}

cst_val *lang_dat_core_data_val(const void *v);

/* 0x327cc arm: adds a word token with its punctuation and the context's prosody, domain,
 * say-as and role */
void utt_builder_add_word(synth_ctx *c, utt_builder *b, const char *name, const char *prepunc,
                          const char *punc, int start, int end, const char *extra)
{
    int n, domain;
    const say_as_info *sa;
    const char *role;
    cst_item *t;
    float f;

    if (strcmp(get_param_string(ctx_lang(c)->features, "encoding", ""), "UTF-8") == 0)
        n = utf8_strlen(name) + 1;
    else
        n = strlen(name) + 1;
    t = utt_builder_add_item(c, b, n);
    domain = c->unk_38;
    sa = c->say_as;
    role = c->unk_34;
    item_set_string(t, "name", name);
    if (prepunc)
        item_set_string(t, "prepunctuation", prepunc);
    item_set_string(t, "punc", punc);
    item_set_int(t, "start_pos", start);
    item_set_int(t, "end_pos", end);
    if (c->lang)
        item_set(t, "lang_dat_core_data", lang_dat_core_data_val(c->lang));
    else if (item_feat_present(t, "lang_dat_core_data"))
        item_feat_remove(t, "lang_dat_core_data");
    if (extra)
        item_set_string(t, "extra", extra);
    if (domain) {
        item_set_int(t, "domain", c->unk_38);
        if (c->unk_0[4])
            item_set_int(t, "tts_apply_dnn", 1);
    }
    if (c->flags)
        item_set_int(t, "effect_whisper", c->flags);
    if (c->prosody.volume != 1.0f)
        item_set_float(t, "amplify", c->prosody.volume);
    if ((f = c->prosody.rate) != 1.0f)
        item_set_float(t, "duration_stretch", f < 0.05f ? 20.0f : 1.0f / f);
    if ((f = c->prosody.pitch) != 1.0f)
        item_set_float(t, "pitch_stretch", f < 0.5f ? 2.0f : f > 2.0f ? 0.5f : 1.0f / f);
    if (sa) {
        if (sa->name)
            item_set_string(t, "interpret-as", sa->name);
        if (sa->format)
            item_set_string(t, "interpret-as-format", sa->format);
    }
    if (role)
        item_set_string(t, "role", role);
    if (!c->unk_0[1] && !c->unk_0[2] && utt_is_sentence_end(c, t) == 1)
        b->unk_c = 1;
    b->last_item = t;
    b->last_end = end;
    b->unk_10[5] += n;
}

/* a token and what surrounded it (tokenizer.c) */
typedef struct token {
    const char *name;
    const char *whitespace;
    const char *prepunctuation;
    const char *postpunctuation;
    int start;                  /* +0x10: input position */
    int end;                    /* +0x14 */
    const char *extra;          /* +0x18 */
} token;
typedef struct tokenizer tokenizer;
token *tokenizer_next(tokenizer *t, const void *segment);
int text_has_breaks(const char *s, int n);

/* adds the word of n bytes at pos of text (n + len_punc with its punctuation) */
static inline void utt_builder_add_text_word(synth_ctx *c, utt_builder *b, const ivona_ustr *text,
                                             int pos, int n, const char *prepunc, const char *punc,
                                             int len_punc, const char *extra)
{
    ustr_span span;
    char *w;

    if (n) {
        w = cst_strndup(text->data + pos, n);
        ustr_span_of(text, pos, n, &span);
        utt_builder_add_word(c, b, w, prepunc, punc, span.start, span.end, extra);
        ustr_span_of(text, pos, n + len_punc, &span);
        utt_chunk_add_span(&b->chunk, &span);
        cst_free(w);
    }
    b->unk_10[4] = 0;
}

/* 0x32b8c arm: adds the tokens of segment of text (at offset), keeping their white space and
 * ending a sentence at line breaks */
void utt_builder_add_tokens(synth_ctx *c, utt_builder *b, tokenizer *tz, const void *segment,
                            const ivona_ustr *text, int offset)
{
    token *t;

    while ((t = tokenizer_next(tz, segment)) != NULL) {
        if (!c->unk_0[1] && !c->unk_0[2] && t->whitespace && !b->unk_10[4] && b->last_item
            && text_has_breaks(t->whitespace, c->unk_0[5])) {
            utt_builder_end_sentence(c, b->last_item);
            b->unk_c = 1;
            if (c->unk_0[0] == 0)
                b->unk_10[2] = 1;
        }
        if (t->whitespace)
            strbuf_append_str(&b->text, t->whitespace);
        if (*t->name)
            utt_builder_add_text_word(c, b, text, offset + t->start + strlen(t->prepunctuation),
                                      strlen(t->name), t->prepunctuation, t->postpunctuation,
                                      strlen(t->postpunctuation), t->extra);
    }
}

/* where utt_builder_add_word_cb adds its words */
typedef struct add_word_data {
    synth_ctx *ctx;
    utt_builder *builder;
    const ivona_ustr *text;
    int offset;
} add_word_data;

/* 0x32d5c arm: a word callback: adds the word of n bytes at pos */
void utt_builder_add_word_cb(int pos, int n, const char *prepunc, const char *punc, int len_punc,
                             const char *extra, add_word_data *d)
{
    utt_builder_add_text_word(d->ctx, d->builder, d->text, pos + d->offset, n, prepunc, punc,
                              len_punc, extra);
}




/* a tokenizer text segment (tokenizer.c) */
typedef struct text_segment {
    char *text;
    char *alt;
    int unk_8;
} text_segment;
tokenizer *string_tokenizer_new(ivona_vector *segments, cst_features *feats, cst_features *params);
void tokenizer_free(tokenizer *t);
typedef void (*word_cb_fn)(int pos, int n, const char *prepunc, const char *punc, int len_punc,
                           const char *extra, add_word_data *d);
typedef void (*tokenizer_expand_str_fn)(const char *s, int n, word_cb_fn cb, add_word_data *d);
tokenizer_expand_str_fn val_tokenizer_expand_str_func(const cst_val *v);

/* adds the n bytes at pos of text as one word (with punctuation punc) */
#define ADD_WORD(c, b, text, pos, n, punc)                                                \
    do {                                                                                  \
        ustr_span span_;                                                                  \
        char *w_;                                                                         \
                                                                                          \
        if (n) {                                                                          \
            w_ = cst_strndup((text)->data + (pos), n);                                    \
            ustr_span_of(text, pos, n, &span_);                                           \
            utt_builder_add_word(c, b, w_, NULL, punc, span_.start, span_.end, NULL);     \
            ustr_span_of(text, pos, n, &span_);                                           \
            utt_chunk_add_span(&(b)->chunk, &span_);                                      \
            cst_free(w_);                                                                 \
        }                                                                                 \
        (b)->unk_10[4] = 0;                                                               \
    } while (0)

/* 0x32e18 arm: turns the pending chunks into tokens of the utterance: text through the
 * tokenizer (or as characters/serial digit groups, a date, a time or a phone number for
 * their say-as), phonemes as one token */
void synth_state_flush(synth_state *s)
{
    int count = vector_size(s->chunks), i;
    lang_core *lang;
    cst_features *params;
    ivona_vector *segs;
    text_segment seg;
    tokenizer *tz;
    synth_chunk **p;
    synth_ctx *c;
    utt_builder *b;
    text_chunk *tc;
    audio_clip_chunk *ac;
    const ivona_ustr *text;
    unsigned int off, n, g;
    const char *name, *ptype;
    char *str, *ymd[3];
    char date[12], order[4], buf[16];
    const say_as_info *sa;
    say_as_info local;
    const cst_val *f;
    add_word_data d;
    cst_item *t;
    int domain;
    size_t len;
    float v;

    if (count == 0)
        return;
    lang = ctx_lang(s->ctx);
    params = VOICE_DATA(s->ctx->voice)->reg->features;
    segs = vector_new(sizeof(text_segment), NULL, (ivona_vector_destroy_fn)str_pair_free);
    for (p = s->chunks->data; p != (synth_chunk **)s->chunks->data + s->chunks->count; p++) {
        if ((*p)->type == CHUNK_TEXT) {
            tc = (text_chunk *)*p;
            seg.text = cst_strndup(tc->text->data + tc->flags, tc->text->len - tc->flags);
            seg.alt = NULL;
        } else if ((*p)->type == CHUNK_AUDIO) {
            ac = (audio_clip_chunk *)*p;
            seg.alt = cst_strdup(ac->src);
            seg.text = cst_strdup(ac->alt);
        } else
            cst_error();
        seg.unk_8 = (*p)->ctx->unk_0[2];
        vector_push(segs, &seg);
    }
    tz = string_tokenizer_new(segs, lang->features, params);
    vector_size(segs);
    for (i = 0; i != count; i++) {
        synth_chunk *ch = *(synth_chunk **)vector_at(s->chunks, i);

        if (ch->type == CHUNK_TEXT) {
            const text_segment *sg = vector_at(segs, i);

            tc = (text_chunk *)ch;
            c = ch->ctx;
            text = tc->text;
            b = s->builder;
            off = tc->flags;
            sa = c->say_as;
            if (sa == NULL || (name = sa->name) == NULL)
                goto tokens;
            if (strcmp(name, "characters") == 0 || strcmp(name, "serial") == 0) {
                if (c->digit_groups) {
                    while (off < text->len) {
                        if (vector_size(c->digit_groups) < 2)
                            goto whole;
                        g = *(unsigned int *)vector_get(c->digit_groups, 0);
                        n = text->len - off;
                        if (g < n)
                            n = g;
                        ADD_WORD(c, b, text, off, n, ",");
                        off += n;
                        if (g <= n)
                            vector_pop_front(c->digit_groups);
                        else {
                            g -= n;
                            vector_set(c->digit_groups, 0, &g);
                        }
                    }
                    continue;
                }
            whole:
                if (off < text->len) {
                    n = text->len - off;
                    ADD_WORD(c, b, text, off, n, "");
                }
            } else if (strcmp(name, "date") == 0) {
                if (text->len != off) {
                    str = cst_substr(text->data, off, text->len - off);
                    if (!ssml_date_parse(str, ymd)) {
                        cst_free(str);
                        goto tokens;
                    }
                    ssml_date_join(ymd[0], ymd[1], ymd[2], date, order);
                    cst_free(ymd[0]);
                    cst_free(ymd[1]);
                    cst_free(ymd[2]);
                    local.name = sa->name;
                    local.format = order;
                    local.whole = sa->whole;
                    c->say_as = &local;
                    utt_builder_add_word(c, b, date, NULL, "", off, text->len, NULL);
                    c->say_as = sa;
                    cst_free(str);
                }
            } else if (strcmp(name, "time") == 0) {
                if (!ssml_time_expand(text->data + off, text->len - off, buf))
                    goto tokens;
                utt_builder_add_word(c, b, buf, NULL, "", off, text->len, NULL);
            } else if (strcmp(name, "telephone") == 0
                       && (f = get_param_val(ctx_lang(c)->features, "tokenizer_expand_phone_number_func",
                                             NULL)) != NULL) {
                d.ctx = c;
                d.builder = b;
                d.text = text;
                d.offset = off;
                val_tokenizer_expand_str_func(f)(text->data + off, text->len - off,
                                                 utt_builder_add_word_cb, &d);
            } else
            tokens:
                utt_builder_add_tokens(c, b, tz, sg, text, off);
        } else if (ch->type == CHUNK_AUDIO) {
            ac = (audio_clip_chunk *)ch;
            str = ac->src;
            c = ch->ctx;
            len = strlen(str);
            ptype = ac->mark;
            b = s->builder;
            t = utt_builder_add_item(c, b, len + 1);
            domain = c->unk_38;
            if (c->lang)
                item_set(t, "lang_dat_core_data", lang_dat_core_data_val(c->lang));
            else if (item_feat_present(t, "lang_dat_core_data"))
                item_feat_remove(t, "lang_dat_core_data");
            if (domain) {
                item_set_int(t, "domain", c->unk_38);
                if (c->unk_0[4])
                    item_set_int(t, "tts_apply_dnn", 1);
            }
            item_set_string(t, "name", "");
            if (ac->flags == 6) {
                item_set_string(t, "ruby", str);
                item_set_string(t, "alphabet", "x-ivona-katakana");
            } else if (ac->flags == 5) {
                item_set_string(t, "alphabet", ptype);
                item_set_string(t, "xsampa", str);
            } else
                item_set_string(t, ac->flags == 1 ? "phones" : "xsampa", str);
            item_set_int(t, "start_pos", ac->span.start);
            item_set_int(t, "end_pos", ac->span.end);
            item_set_string(t, "punc", "");
            if (c->flags)
                item_set_int(t, "effect_whisper", c->flags);
            if (c->prosody.volume != 1.0f)
                item_set_float(t, "amplify", c->prosody.volume);
            if ((v = c->prosody.rate) != 1.0f)
                item_set_float(t, "duration_stretch", v < 0.05f ? 20.0f : 1.0f / v);
            if ((v = c->prosody.pitch) != 1.0f)
                item_set_float(t, "pitch_stretch", v < 0.5f ? 2.0f : v > 2.0f ? 0.5f : 1.0f / v);
            b->last_end = ac->span.end;
            b->last_item = t;
            b->unk_10[5] += len + 1;
            b->unk_10[4] = 0;
            utt_chunk_add_span(&b->chunk, &ac->span);
        } else
            cst_error();
    }
    vector_clear(s->chunks);
    s->unk_34 = 0;
    vector_free(segs);
    tokenizer_free(tz);
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

/* 0x33ae0 arm: a paragraph break: flushes the utterance */
void synth_state_break_paragraph(synth_state *s)
{
    synth_state_flush(s);
    UTT_BUILDER_FLUSH(&s->ctx->voice, s->builder);
}

/* 0x33c34 arm: enters a paragraph (with its phrasing and break duration models) */
void synth_state_enter_p(synth_state *s, int phrasing_model, int break_dur_model)
{
    utt_builder *b;

    if (s->ctx->unk_0[0]++ != 0)
        return;
    synth_state_flush(s);
    b = s->builder;
    utt_builder_end_sentence(s->ctx, b->last_item);
    b->unk_c = 1;
    b->unk_10[2] = 1;
    s->builder->chunk.phrasing_model = phrasing_model;
    feat_set_int(s->builder->chunk.utt->features, "phrasing_model", phrasing_model);
    s->builder->chunk.break_dur_model = break_dur_model;
    feat_set_int(s->builder->chunk.utt->features, "break_dur_model", break_dur_model);
}

/* 0x33cc4 arm: leaves a paragraph */
void synth_state_leave_p(synth_state *s)
{
    utt_builder *b;

    if (--s->ctx->unk_0[0] != 0)
        return;
    synth_state_flush(s);
    b = s->builder;
    END_SENTENCE(s->ctx, b->last_item);
    b->unk_c = 1;
    b->unk_10[2] = 1;
    s->builder->chunk.phrasing_model = 0;
    s->builder->chunk.break_dur_model = 0;
}

/* 0x33e44 arm: enters a sentence */
void synth_state_enter_s(synth_state *s)
{
    utt_builder *b;

    if (s->ctx->unk_0[1]++ != 0)
        return;
    synth_state_flush(s);
    b = s->builder;
    END_SENTENCE(s->ctx, b->last_item);
    b->unk_c = 1;
}

/* 0x33fa4 arm: leaves a sentence */
void synth_state_leave_s(synth_state *s)
{
    utt_builder *b;

    if (--s->ctx->unk_0[1] != 0)
        return;
    synth_state_flush(s);
    b = s->builder;
    END_SENTENCE(s->ctx, b->last_item);
    b->unk_c = 1;
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
void synth_state_check_size(synth_state *s, int n)
{
    synth_state_flush(s);
    utt_builder_check_size(s->ctx, s->builder, n);
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
