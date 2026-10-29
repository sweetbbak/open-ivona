/*
 * English syllabification and post-lexical rules (0x482c8..): the syllable boundaries of a
 * pronunciation by the en-US or en-GB tree, and Amy's post-lexical rules.
 */
#include <string.h>
#include "cst_item.h"
#include "cst_ffeatures.h"
#include "cst_phoneset.h"
#include "cst_regex.h"
#include "cst_string.h"
#include "cst_val.h"
#include "syl_cart.h"

cst_val *postlex_apply_rules(const syl_cart *cart, const cst_phoneset *ps, cst_val *phones);
cst_item *item_traverse(const cst_item *item, ...);

extern const syl_cart en_us_syl_cart, en_gb_syl_cart;
extern const cst_phoneset en_us_phoneset, en_gb_phoneset;

/* 0x482c8 arm: the en-US syl_boundaries_func */
cst_val *en_us_syllabify(cst_val *phones)
{
    return postlex_apply_rules(&en_us_syl_cart, &en_us_phoneset, phones);
}

/* 0x482e8 arm: the en-GB syl_boundaries_func */
cst_val *en_gb_syllabify(cst_val *phones)
{
    return postlex_apply_rules(&en_gb_syl_cart, &en_gb_phoneset, phones);
}

/* 0x48308 arm: syllabifies by the item's language: the British tree for en-AU, en-GB* and
 * en-IN, else the American one */
cst_val *en_syllabify(const cst_item *item, cst_val *phones)
{
    if (strcmp(ffeature_string(item_traverse(item, 0), "language_code"), "en-AU") == 0
        || cst_startswith(ffeature_string(item_traverse(item, 0), "language_code"), "en-GB")
        || strcmp(ffeature_string(item_traverse(item, 0), "language_code"), "en-IN") == 0)
        return en_gb_syllabify(phones);
    return en_us_syllabify(phones);
}

#include "cst_utterance.h"
#include "cst_relation.h"
#include "phone_rule.h"

int postlex_voice_is_name(const char *name, cst_utterance *u);
int postlex_is_consonant(cst_item *item);
int postlex_is_final_syl(cst_item *item);
cst_item *postlex_insert_phone(cst_item *syl, const char *phone_name, const void *ps);
void postlex_resyllabify(cst_item *item, const void *ps);
void apply_phone_rules(cst_utterance *u, const phone_rule *rules);

#define REL_WORD 2
#define REL_SYLSTRUCTURE 4
#define REL_SEGMENT 6

/* 0x3c23fc..: voiced stops after "s" (pairs) */
static const char *const en_s_stop_voicing[] = { "p", "b", "t", "d", "k", "g", NULL, NULL };
/* vowels before a syllable's "r" */
static const char *const en_r_vowels[] = {
    "ae", "eax", "eh", "eax", "ey", "eax", "ih", "iax", "iy", "iax", "uh", "uax", "uw", "uax",
    "ah", "ax", "ow", "ao", "o", "ao", NULL, NULL
};
static const char *const en_r_diphthongs[] = { "ay", "aw", "oy", NULL };
static const char *const en_r_dropping[] = { "aa", "ao", "ax", "eax", "er", "iax", "uax", NULL };
/* the en-US phone rules */
static const phone_rule en_us_phone_rules[] = {
    { "m", 2, "uh", 2, 1, NULL, -1, NULL, 0 },
    { "iy", 0, "ow", 2, 2, NULL, -1, NULL, 0 },
    { "ih", -1, "j", -1, 0, "iy", -1, NULL, -1 },
    { "hh", -1, "pau", -1, 0, "", -1, NULL, -1 },
    { "ae", 0, "hh", -1, 0, "ax", -1, NULL, -1 },
    { "ey", -1, "uh", -1, 0, NULL, -1, "uw", -1 },
    { "ih", -1, "eh", -1, 0, "iy", -1, NULL, -1 },
    { "ae", -1, "w", -1, 0, "ax", -1, NULL, -1 },
    { "eh", -1, "w", -1, 0, "ey", -1, NULL, -1 },
    { "ow", -1, "ng", -1, 0, "aa", -1, NULL, -1 },
    { "aw", -1, "uw", -1, 0, "aa", -1, NULL, -1 },
    { "r", -1, "ng", -1, 0, NULL, -1, "n", -1 },
    { "ax", -1, "uh", -1, 0, "ey", -1, NULL, -1 },
    { "iy", -1, "ng", -1, 0, "ih", -1, NULL, -1 },
    { "uh", -1, "b", -1, 0, "uw", -1, NULL, -1 },
    { NULL, 0, NULL, 0, 0, NULL, 0, NULL, 0 }
};

/* the second of the pair whose first is s in pairs, or NULL */
static inline const char *pair_find(const char *const *pairs, const char *s)
{
    for (; *pairs; pairs += 2)
        if (strcmp(*pairs, s) == 0)
            return pairs[1];
    return NULL;
}

/* 0x483b4 arm: the postlex_func: "the" before a vowel; (en-US, en-AU and Abbey) a final t
 * between k or p and s goes; stops after "s" at a word's start are unaspirated; non-rhotic
 * accents colour a vowel before r and drop r after a vowel; en-US phone rules */
cst_utterance *en_gb_amy_postlex(cst_utterance *u)
{
    const char *lang = get_param_string(u->features, "language_code", "en-US");
    const char *to;
    cst_item *s, *n, *ss, *w, *p, *q, *r;

    for (s = relation_head(utt_relation(u, REL_SEGMENT)); s; s = item_next(s))
        if (strcmp("ax", item_feat_string(s, "name")) == 0
            && strcmp("the", ffeature_string(item_traverse(s, REL_SYLSTRUCTURE, 0x106, 0x106, 0),
                                             "name")) == 0
            && strcmp("+", ffeature_string(item_traverse(s, 0x100, 0), "ph_vc")) == 0)
            item_set_string(s, "name", "iy");
    if (strcmp(lang, "en-US") == 0 || strcmp(lang, "en-AU") == 0
        || postlex_voice_is_name("vox_en_gb_abbey", u)) {
        for (s = relation_head(utt_relation(u, REL_SEGMENT)); s; s = item_next(s)) {
            if (item_feat_present(item_parent(item_parent(item_as(s, REL_SYLSTRUCTURE))), "phones")
                || strcmp("t", item_feat_string(s, "name")) != 0)
                continue;
            ss = item_as(s, REL_SYLSTRUCTURE);
            if ((strcmp("k", ffeature_string(item_traverse(ss, 0x103, 0), "name")) == 0
                 || strcmp("p", ffeature_string(item_traverse(ss, 0x103, 0), "name")) == 0)
                && strcmp("s", ffeature_string(item_traverse(ss, 0x100, 0), "name")) == 0
                && item_next(item_next(ss)) == NULL && item_next(item_parent(ss)) == NULL) {
                s = item_next(s);
                delete_item(item_as(ss, REL_SEGMENT));
                delete_item(ss);
            }
        }
    }
    if (strcmp(lang, "en-IN") != 0) {
        for (w = relation_head(utt_relation(u, REL_WORD)); w; w = item_next(w)) {
            if ((p = item_traverse(w, REL_SYLSTRUCTURE, 0x107, 0x107, 0)) == NULL)
                continue;
            if (strcmp("ih", item_feat_string(p, "name")) == 0 && item_next(p)
                && strcmp("k", item_feat_string(item_next(p), "name")) == 0
                && item_next(item_next(p)) == NULL && item_feat_int(item_parent(p), "stress") == 0) {
                p = item_as(item_next(item_next(item_as(p, REL_SEGMENT))), REL_SYLSTRUCTURE);
                if (p == NULL)
                    continue;
            } else if (strcmp("ax", item_feat_string(p, "name")) == 0 && item_next(p) == NULL) {
                p = item_as(item_next(item_as(p, REL_SEGMENT)), REL_SYLSTRUCTURE);
                if (p == NULL)
                    continue;
            }
            if (strcmp("s", item_feat_string(p, "name")) != 0 || (q = item_next(p)) == NULL)
                continue;
            if ((to = pair_find(en_s_stop_voicing, item_feat_string(q, "name"))) == NULL
                || (r = item_next(q)) == NULL)
                continue;
            if (strcmp("+", ffeature_string(item_traverse(r, 0), "ph_vc")) != 0
                && strcmp(item_feat_string(r, "name"), "r") != 0)
                continue;
            item_set_string(q, "name", to);
        }
    }
    if (strcmp(get_param_string(u->features, "language_code", "en-US"), "en-US") != 0
        && strcmp(lang, "en-IN") != 0) {
        for (s = relation_head(utt_relation(u, REL_SEGMENT)); item_next(s); s = item_next(s)) {
            n = item_next(s);
            if (strcmp(item_feat_string(n, "name"), "r") != 0 || postlex_is_final_syl(s)
                || !(postlex_is_consonant(item_next(n)) || postlex_is_final_syl(n)))
                continue;
            if ((to = pair_find(en_r_vowels, item_feat_string(s, "name"))) != NULL)
                item_set_string(s, "name", to);
            else if (cst_member_string(item_feat_string(s, "name"), en_r_diphthongs)) {
                postlex_resyllabify(s, &en_gb_phoneset);
                postlex_insert_phone(item_next(item_parent(item_as(s, REL_SYLSTRUCTURE))), "ax",
                                     &en_gb_phoneset);
            }
        }
        for (s = relation_head(utt_relation(u, REL_SEGMENT)); s && item_next(s); s = item_next(s)) {
            n = item_next(s);
            if (postlex_is_final_syl(s) || strcmp(item_feat_string(n, "name"), "r") != 0
                || !cst_member_string(item_feat_string(s, "name"), en_r_dropping)
                || strcmp("0", ffeature_string(item_traverse(n, 0x100, 0), "ph_vlng")) != 0)
                continue;
            delete_item(item_as(n, REL_SYLSTRUCTURE));
            delete_item(n);
        }
    }
    if (strcmp(lang, "en-US") == 0)
        apply_phone_rules(u, en_us_phone_rules);
    return u;
}

extern const cst_regex *const en_rx_word, *const en_rx_time_ampm, *const en_rx_dotted_letters;

/* whether a token name, with its punctuation, abbreviates: a time with am/pm, "etc", "Mr",
 * "Mrs", "Ms." or dotted letters */
static inline int en_is_abbrev(const cst_item *tok, const char *name)
{
    return cst_regex_match(en_rx_time_ampm, name) || strcmp("Etc", name) == 0
           || strcmp("etc", name) == 0 || strcmp("Mr", name) == 0 || strcmp("Mrs", name) == 0
           || (strcmp("Ms", name) == 0
               && ffeature_string(item_traverse(tok, 0), "punc")[0] == '.')
           || cst_regex_match(en_rx_dotted_letters, name);
}

/* 0x48d88 arm: whether tok is a single letter with a full stop ("J.") that does not end a
 * sentence: the first token, or after a pause, or after a token whose own punctuation (past an
 * abbreviation's full stop) ends something */
int en_is_initial(const cst_item *tok)
{
    const char *name = item_feat_string(tok, "name");
    const char *punc = item_feat_string(tok, "punc");
    const cst_item *prev;

    if (strlen(name) != 1 || strcmp(".", punc) != 0 || !cst_regex_match(en_rx_word, name))
        return 0;
    if ((prev = item_prev(tok)) == NULL || item_feat_present(prev, "wave")
        || item_feat_present(prev, "silence"))
        return 1;
    name = item_feat_string(prev, "name");
    punc = item_feat_string(prev, "punc");
    if (en_is_abbrev(prev, name) && punc[0] == '.')
        punc++;
    return punc[0] != '\0';
}

/* 0x48f98 arm */
void en_lang_global_init(void)
{
}

/* 0x48f9c arm */
void en_lang_global_fini(void)
{
}
