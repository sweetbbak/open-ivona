/*
 * IVONA's synthesis pipeline, from Flite's cst_synth.c and cst_utt_utils.c (0x663b4..0x6b134).
 *
 * Synth methods carry a metric name for each module: with "core_metrics" set, the time spent in
 * every module is recorded. An optional "synth_module_callback" is called before the first
 * module ("init"), after every module (with its hook name) and at the end ("end"); a non-zero
 * return stops the pipeline.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"
#include "cst_val.h"
#include "cst_val_defs.h"
#include "tts_metrics.h"
#include "tts_voice.h"

unsigned long long ivona_time_usec(void);
cst_relation *val_relation(const cst_val *v);

typedef int (*synth_module_callback)(cst_utterance *u, const char *stage, void *data);
extern const int cst_val_type_synth_module_callback;

typedef struct cst_synth_module {
    const char *hookname;
    const char *metric;         /* "core_metrics" entry for its time */
    cst_uttfunc defhook;
} cst_synth_module;

/* language core data of a voice (see synth.c) */
typedef struct lang_core {
    const char *code;
    int unk_4[4];
    cst_features *features;     /* +0x14 */
    cst_features *ffunctions;   /* +0x18 */
} lang_core;

typedef struct synth_voice {
    cst_features *features;
    lang_core *lang;
    cst_utterance *(*utt_init)(cst_utterance *u, struct synth_voice *v);
} synth_voice;

/* 0x663b4 arm: the items from i on, with their features, relations and daughters. */
void item_print_tree(cst_file fd, const cst_item *i, const char *indent)
{
    const cst_featvalpair *p;
    char sub[256];

    for (; i; i = item_next(i)) {
        cst_fprintf(fd, "%sITEM %p {\n", indent, i);
        for (p = item_feats(i)->head; p; p = p->next) {
            cst_fprintf(fd, "%s\t%s => ", indent, p->name);
            val_print(fd, p->val);
            cst_fprintf(fd, "\n");
        }
        cst_fprintf(fd, "%s\t---\n", indent);
        for (p = i->contents->relations->head; p; p = p->next) {
            cst_fprintf(fd, "%s\tR:%s => ", indent, p->name);
            val_print(fd, p->val);
            cst_fprintf(fd, "\n");
        }
        if (item_daughter(i)) {
            snprintf(sub, 255, "%s\t\t", indent);
            cst_fprintf(fd, "%s\tdaughters {\n", indent);
            item_print_tree(fd, item_daughter(i), sub);
            cst_fprintf(fd, "%s\t}\n", indent);
        }
        cst_fprintf(fd, "%s}\n", indent);
    }
}

/* 0x66580 arm */
const char *utt_input_text(cst_utterance *u)
{
    return val_string(feat_val(u->features, "input_text"));
}

/* 0x665a0 arm */
int utt_set_input_text(cst_utterance *u, const char *text)
{
    if (text == NULL)
        text = "";
    feat_set_string(u->features, "input_text", text);
    return 0;
}

/* 0x665d8 arm */
void item_print_list(cst_file fd, const cst_item *i)
{
    item_print_tree(fd, i, "");
}

/* 0x665e8 arm */
void utt_print_relations(cst_file fd, cst_utterance *u)
{
    const cst_featvalpair *p;

    for (p = u->relations->head; p; p = p->next) {
        cst_fprintf(fd, "*** RELATION: %s\n\n", p->name);
        item_print_tree(fd, relation_head(val_relation(p->val)), "");
        if (p->next == NULL)
            break;
        cst_fprintf(fd, "\n");
    }
}

/* 0x66678 arm: copies the voice's and language's features in and runs the voice's hook. */
cst_utterance *utt_init(cst_utterance *u, synth_voice *v)
{
    feat_copy_into(v->features, u->features);
    feat_copy_into(v->lang->features, u->features);
    feat_copy_into(v->lang->ffunctions, u->ffunctions);
    if (v->utt_init)
        v->utt_init(u, v);
    if (!get_param_int(u->features, "deterministic_path", 1))
        srand48(time(NULL));
    return u;
}

/* 0x666f8 arm, 0x66934 arm */
CST_VAL_REGISTER_FUNCPTR(synth_module_callback, synth_module_callback)

static cst_utterance *apply_synth_module(cst_utterance *u, const cst_synth_module *mod)
{
    const cst_val *v = feat_val(u->features, mod->hookname);

    if (v)
        return (*val_uttfunc(v))(u);
    if (mod->defhook)
        return (*mod->defhook)(u);
    return u;
}

/* 0x66750 arm */
cst_utterance *apply_synth_method(cst_utterance *u, const cst_synth_module meth[])
{
    cst_features *metrics = get_param_features(u->features, "core_metrics", NULL);
    const cst_val *cb = feat_val(u->features, "synth_module_callback");
    const cst_val *v;
    void *data = NULL;
    unsigned int start;

    if (cb) {
        v = feat_val(u->features, "synth_module_callback_data");
        if (v)
            data = val_userdata(v);
        if (val_synth_module_callback(cb)(u, "init", data))
            return u;
    }
    for (; meth->hookname; meth++) {
        start = metrics ? ivona_time_usec() : 0;
        if ((u = apply_synth_module(u, meth)) == NULL)
            return NULL;
        if (metrics)
            metrics_add_time(metrics, meth->metric, ivona_time_usec() - start);
        if (cb && val_synth_module_callback(cb)(u, meth->hookname, data))
            return u;
    }
    if (cb)
        val_synth_module_callback(cb)(u, "end", data);
    return u;
}

cst_utterance *default_tokenization(cst_utterance *u);
cst_utterance *FUN_0006864c(cst_utterance *u);
cst_utterance *FUN_00068d10(cst_utterance *u);
cst_utterance *FUN_00069190(cst_utterance *u);
cst_utterance *FUN_00069a08(cst_utterance *u);
cst_utterance *FUN_00069b88(cst_utterance *u);
cst_utterance *FUN_0006a5b8(cst_utterance *u);
cst_utterance *FUN_0006a84c(cst_utterance *u);
cst_utterance *FUN_0006b084(cst_utterance *u);

static const cst_synth_module text_methods[] = {
    { "tokenizer_func", "tokenizer_time", default_tokenization },
    { "t2t_func", "tokenstotokens_time", FUN_0006864c },
    { "text_analysis_func", "text_analysis_time", FUN_00068d10 },
    { "pos_tagger_func", "pos_tagger_time", NULL },
    { "phrasing_func", "phrasing_time", FUN_00069190 },
    { "lettertosound_func", "lettertosound_time", FUN_00069a08 },
    { "change_language_func", "change_language_time", FUN_00069b88 },
    { "pause_insertion_func", "pause_insertion_time", FUN_0006a5b8 },
    { "postlex_func", "postlex_time", NULL },
    { "intonation_func", "intonation_time", FUN_0006a84c },
    { "acoustic_prediction_func", "acoustic_prediction_time", NULL },
    { NULL, NULL, NULL }
};

/* the same without tokenization (the utterance has its tokens) */
static const cst_synth_module token_methods[] = {
    { "tokenizer_func", "tokenizer_time", NULL },
    { "t2t_func", "tokenstotokens_time", FUN_0006864c },
    { "text_analysis_func", "text_analysis_time", FUN_00068d10 },
    { "pos_tagger_func", "pos_tagger_time", NULL },
    { "phrasing_func", "phrasing_time", FUN_00069190 },
    { "lettertosound_func", "lettertosound_time", FUN_00069a08 },
    { "change_language_func", "change_language_time", FUN_00069b88 },
    { "pause_insertion_func", "pause_insertion_time", FUN_0006a5b8 },
    { "postlex_func", "postlex_time", NULL },
    { "intonation_func", "intonation_time", FUN_0006a84c },
    { "acoustic_prediction_func", "acoustic_prediction_time", NULL },
    { NULL, NULL, NULL }
};

/* input given as phones */
static const cst_synth_module phones_methods[] = {
    { "tokenizer_func", "tokenizer_time", default_tokenization },
    { "t2t_func", "tokenstotokens_time", FUN_0006864c },
    { "text_analysis_func", "text_analysis_time", FUN_0006b084 },
    { "pos_tagger_func", "pos_tagger_time", NULL },
    { "phrasing_func", "phrasing_time", FUN_00069190 },
    { "lettertosound_func", "lettertosound_time", FUN_00069a08 },
    { "change_language_func", "change_language_time", FUN_00069b88 },
    { "pause_insertion_func", "pause_insertion_time", FUN_0006a5b8 },
    { "postlex_func", "postlex_time", NULL },
    { "intonation_func", "intonation_time", FUN_0006a84c },
    { "acoustic_prediction_func", "acoustic_prediction_time", NULL },
    { NULL, NULL, NULL }
};

/* 0x668fc arm */
cst_utterance *utt_text_frontend(cst_utterance *u)
{
    return apply_synth_method(u, text_methods);
}

/* 0x6690c arm */
cst_utterance *utt_frontend(cst_utterance *u)
{
    return apply_synth_method(u, token_methods);
}

/* 0x66920 arm */
cst_utterance *utt_phones_frontend(cst_utterance *u)
{
    return apply_synth_method(u, phones_methods);
}
