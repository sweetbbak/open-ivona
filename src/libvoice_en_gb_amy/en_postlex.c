/*
 * English syllabification and post-lexical rules (0x482c8..): the syllable boundaries of a
 * pronunciation by the en-US or en-GB tree, and Amy's post-lexical rules.
 */
#include <string.h>
#include "cst_item.h"
#include "cst_ffeatures.h"
#include "cst_phoneset.h"
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

/* 0x48f98 arm */
void en_lang_global_init(void)
{
}

/* 0x48f9c arm */
void en_lang_global_fini(void)
{
}
