/*
 * Synthesis glue (0x50044..0x50b68): the audiotag val type, per-voice setup of language data and
 * lexicons, and utterance construction/synthesis for a voice.
 */
#include <string.h>
#include "cst_alloc.h"
#include "cst_error.h"
#include "cst_features.h"
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"
#include "cst_val.h"
#include "cst_wave.h"
#include "tts_instance.h"
#include "tts_synth.h"
#include "tts_voice.h"

CST_VAL_REGISTER_TYPE_NODEL(audiotag, audiotag)

/* 0x500b4 arm */
audiotag *new_audiotag(void)
{
    return cst_safe_alloc(1, sizeof(audiotag));
}

/* 0x500c0 arm */
void delete_audiotag(audiotag *t)
{
    cst_free(t->phone);
    cst_free(t);
}

/* 0x500dc arm */
void val_delete_audiotag(void *v)
{
    delete_audiotag(v);
}

/* breath model of female voices */
typedef struct breath_covariances {
    const float *det;
    const float *inv;
} breath_covariances;

typedef struct breath_stats {
    const float *rms;
    const float *dur;
} breath_stats;

static const float breath_cov_det[] = { 0.000758871029f };
static const float breath_cov_inv[] = { 5.94961596f, 2.32057691f, 4.58900595f };
static const float breath_rms[] = { 0.295f, 0.082f };
static const float breath_dur[] = { -27.657f, 2.925f };

static const breath_covariances female_breath_covariances = { breath_cov_det, breath_cov_inv };
static const breath_stats female_breath_stats = { breath_rms, breath_dur };

cst_val *acoustic_covariances_val(const breath_covariances *v);
cst_val *breaths_stats_val(const breath_stats *v);

/* 0x500e0 arm: female voices breathe. */
void voice_reg_fixup(voice_reg *reg)
{
    if (strcmp(feat_string(reg->features, "voice_gender"), "Female") != 0)
        return;
    feat_set(reg->features, "acoustic_breath_covariances",
             acoustic_covariances_val(&female_breath_covariances));
    feat_set(reg->features, "breaths_rms_dur_stats", breaths_stats_val(&female_breath_stats));
}

extern const int cst_val_type_lang_dat_core_data;
CST_VAL_REGISTER_TYPE_NODEL(lang_dat_core_data, void)

/* 0x501e4 arm: the language with code among its codes (NULL: "english"), in a NULL-name
 * terminated array. */
__attribute__((optimize("no-crossjumping")))
tts_lang_desc *lang_desc_find(tts_lang_desc *table, const char *code)
{
    tts_lang_desc *l;

    if (table == NULL || table->name == NULL)
        return NULL;
    if (__builtin_expect(code == NULL, 0)) {
        for (; table->name; table++)
            if (strcmp("english", table->name) == 0)
                return table;
        return NULL;
    }
    for (l = table; l->name; l++)
        if (cst_member_string(code, l->codes))
            return l;
    return NULL;
}

/* 0x50278 arm: picks the voice's language (by "language_code", defaulting it) and its data. */
void voice_reg_set_language(voice_reg *reg, tts_lang_desc *table,
                            void *(*part_data)(tts_lang_desc *lang, const char *name))
{
    const char *code = get_param_string(reg->features, "language_code", NULL);
    tts_lang_desc *lang = lang_desc_find(table, code);

    if (lang == NULL) {
        if (code)
            cst_errmsg("Invalid language code: %s\n", code);
        else
            cst_errmsg("Default lang missing\n");
        cst_error();
    }
    if (code == NULL) {
        code = lang->codes[0];
        feat_set_string(reg->features, "language_code", code);
    }
    reg->lang_data = part_data(lang, code);
}

/* 0x5032c arm: the language data of the utterance part an item belongs to. */
void *item_lang_data(cst_item *i)
{
    const cst_val *v;
    int rel = i->relation->id;

    if (__builtin_expect(rel == 5 || rel == 6, 0)) {
        i = item_as(i, 4);
        if (i == NULL)
            return NULL;
        rel = i->relation->id;
    }
    if (rel != 3) {
        while (item_parent(i))
            i = item_parent(i);
        if (i == NULL)
            return NULL;
        if (__builtin_expect(i->relation->id == 1, 1))
            goto found;
    }
    i = item_parent(item_as(i, 1));
    if (i == NULL)
        return NULL;
found:
    v = item_feat(i, "lang_dat_core_data");
    if (v == NULL)
        return NULL;
    return val_lang_dat_core_data(v);
}

/* language core data */
typedef struct lang_core {
    const char *code;
    int unk_4[4];
    cst_features *features;     /* +0x14 */
    cst_features *params;       /* +0x18 */
} lang_core;

/* 0x503e8 arm */
__attribute__((optimize("no-optimize-sibling-calls")))
void lang_core_init(lang_core *l)
{
    l->features = new_features();
    l->params = new_features();
    feat_set_string(l->features, "language_code", l->code);
}

/* 0x5041c arm */
void lang_core_free(lang_core *l)
{
    delete_features(l->features);
    delete_features(l->params);
}

void utt_set_input_text(cst_utterance *u, const char *text);
int utt_init(cst_utterance *u, voice_reg *reg);
int utt_text_frontend(cst_utterance *u);
int utt_phones_frontend(cst_utterance *u);
int utt_frontend(cst_utterance *u);

/* 0x50438 arm */
cst_utterance *utt_from_text(const char *text, voice_reg *reg, void *engine_a, void *engine_b)
{
    cst_utterance *u = new_utterance(engine_a, engine_b);

    utt_set_input_text(u, text);
    if (utt_init(u, reg) && utt_text_frontend(u))
        return u;
    delete_utterance(u);
    return NULL;
}

/* 0x50498 arm */
cst_utterance *utt_from_phones(const char *phones, voice_reg *reg, void *engine_a, void *engine_b)
{
    cst_utterance *u = new_utterance(engine_a, engine_b);

    utt_set_input_text(u, phones);
    if (utt_init(u, reg) && utt_phones_frontend(u))
        return u;
    delete_utterance(u);
    return NULL;
}

/* 0x504f8 arm: runs the front end on an utterance that has its tokens; NULL (and freed) on error. */
cst_utterance *utt_synth_frontend(cst_utterance *u, voice_reg *reg)
{
    if (utt_relation_present(u, 1) && utt_init(u, reg) && utt_frontend(u))
        return u;
    delete_utterance(u);
    return NULL;
}

/* a mark of the concatenation (see wavesynth.c) */
typedef struct wavesynth_mark {
    cst_item *seg;          /* phone mark: the segment starting here */
    cst_item *unit;         /* unit mark: the silence/wave word */
    int more;               /* +0x08: a unit mark that is not the end of the last unit */
    int start;              /* +0x0c */
    int end;                /* +0x10 */
} wavesynth_mark;

cst_wave *concat_synth(cst_utterance *utt, int n, cst_val **marks);
void wavesynth_mark_free(wavesynth_mark *m);
void *val_userdata(const cst_val *v);
char *item_xsampa(cst_item *seg);
cst_item *item_traverse(const cst_item *item, ...);

#define SEG_WORD(s) item_traverse(s, 4, 0x106, 0x106, 1, 0x106, 0)

/* pushes a new tag for item at sample pos */
#define PUSH_TAG(l, t) (*(l) = cons_val(audiotag_val(t), *(l)))

/* 0x50550 arm: synthesizes up to max_samples samples; marks (optional) gets the audiotags
 * of the phones, words and inline silences/waves in them. */
cst_wave *utt_synth_wave(cst_utterance *u, int max_samples, cst_val **marks)
{
    cst_val *ml, *l;
    cst_wave *w;
    wavesynth_mark *m;
    cst_item *it, *p, *pw, *cw, *pword, *cword, *word, *x;
    audiotag *t;
    unsigned int flags;
    int pos;

    if (marks == NULL)
        return concat_synth(u, max_samples, NULL);
    w = concat_synth(u, max_samples, &ml);
    if (w == NULL)
        return w;
    *marks = NULL;
    for (l = ml; l; l = (cst_val *)val_cdr(l)) {
        m = val_userdata(val_car(l));
        if (m->seg == NULL) {
            it = m->unit;
            if (it == NULL)
                goto next;
            pos = m->start;
            if (m->more == 0)
                flags = AUDIOTAG_WORD_END;
            else {
                p = item_prev(it);
                if (p && (item_feat_present(p, "silence") || item_feat_present(p, "wave"))) {
                    t = new_audiotag();
                    t->sample = pos;
                    t->flags = AUDIOTAG_WORD_END;
                    t->name = NULL;
                    t->phone = NULL;
                    t->item = p;
                    PUSH_TAG(marks, t);
                    pos = m->start;
                }
                it = m->unit;
                flags = m->more == 0 ? AUDIOTAG_WORD_END : AUDIOTAG_WORD;
            }
            if (it && (item_feat_present(it, "silence") || item_feat_present(it, "wave"))) {
                t = new_audiotag();
                t->name = NULL;
                t->phone = NULL;
                t->sample = pos;
                t->item = it;
                t->flags = flags;
                PUSH_TAG(marks, t);
            }
            goto next;
        }
        pw = item_as(item_prev(m->seg), 4);
        cw = item_as(m->seg, 4);
        pword = pw ? SEG_WORD(pw) : NULL;
        if (cw == NULL)
            cword = NULL;
        else {
            cword = SEG_WORD(cw);
            if (pw == NULL && cword) {
                p = item_prev(cword);
                pos = m->start;
                if (p && (item_feat_present(p, "silence") || item_feat_present(p, "wave"))) {
                    t = new_audiotag();
                    t->flags = AUDIOTAG_WORD_END;
                    t->name = NULL;
                    t->phone = NULL;
                    t->sample = pos;
                    t->item = p;
                    PUSH_TAG(marks, t);
                }
                goto start;
            }
        }
        if (pw && pword != cword) {
            word = item_as(item_parent(item_parent(pw)), 1);
            for (x = item_next(item_as(word, 3)); x; x = item_next(x))
                if (item_daughter(item_as(x, 4)))
                    break;
            t = new_audiotag();
            t->name = item_feat_string(item_prev(m->seg), "name");
            t->phone = item_xsampa(item_prev(m->seg));
            t->sample = m->start;
            t->flags = x ? AUDIOTAG_WORD_END : AUDIOTAG_WORD_END | AUDIOTAG_PAUSE;
            t->item = item_parent(word);
            PUSH_TAG(marks, t);
        }
        if (cw == NULL)
            goto next;
    start:
        word = item_as(item_parent(item_parent(cw)), 1);
        t = new_audiotag();
        t->name = item_feat_string(m->seg, "name");
        t->phone = item_xsampa(m->seg);
        t->sample = m->start;
        t->flags = AUDIOTAG_PHONE;
        if (pword != cword) {
            for (x = item_prev(item_as(word, 3)); x; x = item_prev(x))
                if (item_daughter(item_as(x, 4)))
                    break;
            t->flags |= x ? AUDIOTAG_WORD : AUDIOTAG_WORD | 0x08;
        }
        t->item = item_parent(word);
        PUSH_TAG(marks, t);
    next:
        wavesynth_mark_free(m);
    }
    delete_val(ml);
    *marks = val_reverse(*marks);
    return w;
}

/* where the backend continues in an utterance */
typedef struct wavesynth {
    int unk_0;
    cst_item *seg;          /* +0x04 */
    int unk_8;
    int unk_c;
    int unk_10;
} wavesynth;

wavesynth *val_wavesynth(const cst_val *v);

/* 0x50a70 arm: restarts synthesis of the utterance from its first segment. */
int utt_rewind(cst_utterance *u, int unused)
{
    wavesynth *p = val_wavesynth(feat_val(u->features, "streamer"));

    p->seg = relation_head(utt_relation(u, 7));
    p->unk_c = 0;
    p->unk_10 = 0;
    return 0;
}

void *lexicon_from_pls(void *pls);

/* 0x50abc arm */
int voice_reg_set_lexicon(voice_reg *reg, void *pls)
{
    cst_val *lex = lexicon_from_pls(pls);

    if (lex)
        feat_set(reg->features, "user_lexicon", lex);
    return lex != NULL;
}

cst_wave *utt_synth_wave(cst_utterance *u, int max_samples, cst_val **marks);

/* 0x50af8 arm */
cst_wave *synth_text(const char *text, voice_reg *reg, void *engine_a, void *engine_b)
{
    cst_utterance *u = utt_from_text(text, reg, engine_a, engine_b);
    cst_wave *w;

    if (u == NULL)
        return NULL;
    w = utt_synth_wave(u, -1, NULL);
    delete_utterance(u);
    return w;
}

/* 0x50b30 arm */
cst_wave *synth_phones(const char *phones, voice_reg *reg, void *engine_a, void *engine_b)
{
    cst_utterance *u = utt_from_phones(phones, reg, engine_a, engine_b);
    cst_wave *w;

    if (u == NULL)
        return NULL;
    w = utt_synth_wave(u, -1, NULL);
    delete_utterance(u);
    return w;
}
