/*
 * The English language module (IVONA's counterpart of Flite's usenglish.c): the exported
 * language_data descriptor and its init/fini, which fill the language's feature set with the
 * phoneset, syllabification, X-SAMPA conversion and text-analysis functions for en-US or for the
 * British set (every other code).
 */
#include <string.h>
#include "cst_features.h"
#include "cst_item.h"
#include "cst_phoneset.h"
#include "cst_utterance.h"
#include "cst_val.h"
#include "str_rewrite.h"
#include "tts_instance.h"

/* what a language's init/fini receive */
typedef struct en_lang_core {
    const char *code;           /* the language code, e.g. "en-GB" */
    char pad_04[0x10];
    cst_features *features;     /* +0x14 */
    cst_features *params;       /* +0x18: feature functions */
} en_lang_core;

void en_lang_global_init(void);
void en_lang_global_fini(void);
void en_lang_setup(en_lang_core *core);
void en_lang_teardown(en_lang_core *core);
void en_ff_register(cst_features *ff);

extern const cst_phoneset en_us_phoneset;
extern const cst_phoneset en_gb_phoneset;
extern const str_rewrite_rule en_us_xivona_to_xsampa_rules[];
extern const str_rewrite_rule en_gb_xivona_to_xsampa_rules[];
void *en_us_stress_info(void);
void *en_gb_stress_info(void);
cst_val *en_us_syllabify(cst_val *phones);
cst_val *en_gb_syllabify(cst_val *phones);
cst_val *en_us_xsampa_to_xivona(const char *xsampa, const char *alphabet, const char *lang);
cst_val *en_gb_xsampa_to_xivona(const char *xsampa, const char *alphabet, const char *lang);
cst_val *en_word_to_segments(cst_item *word);
cst_utterance *en_gb_amy_postlex(cst_utterance *u);
cst_val *en_sentence_boundary(cst_item *token);
cst_val *en_token_to_words(cst_item *token);
cst_val *en_expand_phone_number(const char *s);
cst_val *en_phrase_syl_end_tone(cst_item *syl);

typedef cst_val *(*phonefunc)(cst_val *phones);
typedef cst_val *(*xsampafunc)(const char *xsampa, const char *alphabet, const char *lang);
typedef cst_val *(*tokenizer_expand_str_func)(const char *s);
cst_val *phonefunc_val(phonefunc f);
cst_val *xsampafunc_val(xsampafunc f);
cst_val *tokenizer_expand_str_func_val(tokenizer_expand_str_func f);

/* 0x1315d8.. (emitted in reverse order) */
const char en_punctuation[] = ".,:;!?";
const char en_prepunctuation[] = "";
const char en_singlecharsymbols[] = "";
const char en_whitespace[] = " \t\n\v\f\r\205\240";

/* 0x397d4 arm: sets up the English language core */
void en_lang_init(en_lang_core *core)
{
    en_lang_global_init();
    en_lang_setup(core);
    feat_set_string(core->features, "encoding", "ISO-8859-1");
    if (core->code == NULL || strcmp(core->code, "en-US") == 0) {
        feat_set(core->features, "phoneset", phoneset_val(&en_us_phoneset));
        feat_set_string(core->features, "silence", en_us_phoneset.silence);
        feat_set(core->features, "syl_boundaries_func", phonefunc_val(en_us_syllabify));
        feat_set(core->features, "xsampa_to_xivona_func", xsampafunc_val(en_us_xsampa_to_xivona));
        feat_set(core->features, "xivona_to_xsampa_rules",
                 userdata_val((void *)en_us_xivona_to_xsampa_rules));
        feat_set(core->features, "xivona_stress_info", userdata_val(en_us_stress_info()));
    } else {
        feat_set(core->features, "phoneset", phoneset_val(&en_gb_phoneset));
        feat_set_string(core->features, "silence", en_gb_phoneset.silence);
        feat_set(core->features, "syl_boundaries_func", phonefunc_val(en_gb_syllabify));
        feat_set(core->features, "xsampa_to_xivona_func", xsampafunc_val(en_gb_xsampa_to_xivona));
        feat_set(core->features, "xivona_to_xsampa_rules",
                 userdata_val((void *)en_gb_xivona_to_xsampa_rules));
        feat_set(core->features, "xivona_stress_info", userdata_val(en_gb_stress_info()));
    }
    feat_set(core->features, "wordtosegments_func", itemfunc_val(en_word_to_segments));
    feat_set(core->features, "postlex_func", uttfunc_val(en_gb_amy_postlex));
    feat_set_string(core->features, "text_whitespace", en_whitespace);
    feat_set_string(core->features, "text_postpunctuation", en_punctuation);
    feat_set_string(core->features, "text_prepunctuation", en_prepunctuation);
    feat_set_string(core->features, "text_singlecharsymbols", en_singlecharsymbols);
    feat_set(core->features, "sentence_boundary_func", itemfunc_val(en_sentence_boundary));
    feat_set(core->features, "tokentowords_func", itemfunc_val(en_token_to_words));
    feat_set(core->features, "tokenizer_expand_phone_number_func",
             tokenizer_expand_str_func_val(en_expand_phone_number));
    feat_set(core->features, "phrasesylendtone_func", itemfunc_val(en_phrase_syl_end_tone));
    en_ff_register(core->params);
}

/* 0x39bdc arm */
void en_lang_fini(en_lang_core *core)
{
    en_lang_global_fini();
    en_lang_teardown(core);
}

/* 0x3ba81c */
static const char *const en_lang_codes[] = { "en-US", "en-GB", "en-GB-WLS", "en-AU", "en-IN", NULL };

/* 0x3d7380: the exported language table */
const tts_lang_desc language_data[] = {
    { "english", en_lang_codes, NULL, (void (*)(void *))en_lang_init,
      (void (*)(void *))en_lang_fini },
    { NULL, NULL, NULL, NULL, NULL },
};
