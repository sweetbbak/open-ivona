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
#include "en_lang.h"
#include "str_rewrite.h"
#include "tts_instance.h"


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
cst_val *en_phrase_syl_end_tone(cst_item *syl);
void en_expand_phone_number(const char *s, size_t n,
                            void (*cb)(int, int, const char *, const char *, int, const char *, void *),
                            void *d);

typedef cst_val *(*phonefunc)(cst_val *phones);
typedef cst_val *(*xsampafunc)(const char *xsampa, const char *alphabet, const char *lang);
typedef void (*tokenizer_expand_str_func)(const char *s, size_t n, void (*cb)(int, int, const char *, const char *, int, const char *, void *), void *d);
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

size_t cst_strnspn(const char *s, size_t n, const char *accept);
size_t cst_strncspn(const char *s, size_t n, const char *reject);

/* a word of an expansion: n bytes at pos, followed by punc (as the engine's word_cb_fn) */
typedef void (*en_word_cb)(int pos, int n, const char *prepunc, const char *punc, int len_punc,
                           const char *extra, void *d);

#define PHONE_SEPS "-.,/() "
#define PHONE_DIGITS "*#0123456789"

/* 0x3a5d0 arm: the tokenizer_expand_phone_number_func: the groups of a telephone number as
 * words, the first marked "first"; groups of 7, 10 and 11 digits (1-800 numbers aside) are split
 * the North American way, each part followed by a comma */
void en_expand_phone_number(const char *s, size_t n, en_word_cb cb, void *d)
{
    size_t i, j, k, e, glen;
    const char *punc, *p2, *extra;
    int first = 1, first_word;

    i = cst_strnspn(s, n, PHONE_SEPS);
    if (i < n && s[i] == '+') {
        cb(i, 1, NULL, "", 0, "first", d);
        i++;
        first = 0;
    }
    first_word = 1;
    for (;;) {
        if (i >= n)
            return;
        j = i + cst_strnspn(s + i, n - i, PHONE_SEPS);
        if (j >= n)
            return;
        extra = first ? "first" : NULL;
        if (strchr(PHONE_DIGITS, (unsigned char)s[j]))
            glen = cst_strnspn(s + j, n - j, PHONE_DIGITS);
        else
            glen = cst_strncspn(s + j, n - j, PHONE_SEPS PHONE_DIGITS);
        k = j + glen;
        e = k + cst_strnspn(s + k, n - j - glen, PHONE_SEPS);
        if (e == n)
            punc = "";
        else
            punc = strchr(PHONE_SEPS, s[k]) ? "," : "";
        if (!(j < k && s[j] == '0') && cst_strnspn(s + j, glen, "0123456789") == glen) {
            if (glen == 7) {
                cb(j, 3, NULL, ",", 0, extra, d);
                cb(j + 3, 4, NULL, "", 0, NULL, d);
                goto next;
            }
            if (glen == 10) {
                cb(j, 3, NULL, ",", 0, extra, d);
                cb(j + 3, 3, NULL, ",", 0, NULL, d);
                cb(j + 6, 4, NULL, "", 0, NULL, d);
                goto next;
            }
            if (glen == 11 && s[j] == '1') {
                p2 = ",";
                if ((unsigned char)(s[j + 1] - '8') <= 1 && s[j + 2] == '0' && s[j + 3] == '0')
                    p2 = "";
                cb(j, 1, NULL, p2, 0, extra, d);
                cb(j + 1, 3, NULL, ",", 0, NULL, d);
                cb(j + 4, 3, NULL, ",", 0, NULL, d);
                cb(j + 7, 4, NULL, "", 0, NULL, d);
                goto next;
            }
        }
        if (first_word && e + 3 < n && glen == 1 && s[j] == '1'
            && (unsigned char)(s[e] - '8') <= 1 && s[e + 1] == '0' && s[e + 2] == '0')
            punc = "";
        cb(j, glen, NULL, punc, 0, extra, d);
        first_word = 0;
    next:
        first = 0;
        i = k;
    }
}

/* 0x3ba81c */
static const char *const en_lang_codes[] = { "en-US", "en-GB", "en-GB-WLS", "en-AU", "en-IN", NULL };

/* 0x3d7380: the exported language table */
const tts_lang_desc language_data[] = {
    { "english", en_lang_codes, NULL, (void (*)(void *))en_lang_init,
      (void (*)(void *))en_lang_fini },
    { NULL, NULL, NULL, NULL, NULL },
};
