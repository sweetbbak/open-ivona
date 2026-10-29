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
