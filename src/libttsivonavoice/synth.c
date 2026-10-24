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
tts_lang_desc *lang_desc_find(tts_lang_desc *table, const char *code)
{
    tts_lang_desc *l;

    if (table == NULL || table->name == NULL)
        return NULL;
    if (code == NULL) {
        for (l = table; l->name; l++)
            if (strcmp("english", l->name) == 0)
                return l;
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

    if (rel == 5 || rel == 6) {
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
        if (i->relation->id == 1)
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
