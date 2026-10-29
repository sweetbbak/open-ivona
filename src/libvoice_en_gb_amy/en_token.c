/*
 * English tokens to words (0x48fa0..): sentence boundaries and the expansion of tokens (numbers,
 * dates, times, money, abbreviations, ...) into words.
 */
#include <string.h>
#include "cst_item.h"
#include "cst_ffeatures.h"
#include "cst_regex.h"
#include "cst_val.h"

cst_item *item_traverse(const cst_item *item, ...);
int en_is_initial(const cst_item *tok);

extern const cst_regex *const en_rx_time_ampm, *const en_rx_dotted_letters;

/* 0x4ae84 arm: the sentence_boundary_func: 1 when the token's punctuation ends a sentence
 * (an abbreviation's own full stop aside), -1 for an initial, else 0 */
cst_val *en_sentence_boundary(cst_item *tok)
{
    const char *name = item_feat_string(tok, "name");
    const char *punc = item_feat_string(tok, "punc");

    if (cst_regex_match(en_rx_time_ampm, name) || strcmp("Etc", name) == 0
        || strcmp("etc", name) == 0 || strcmp("Mr", name) == 0 || strcmp("Mrs", name) == 0
        || (strcmp("Ms", name) == 0 && ffeature_string(item_traverse(tok, 0), "punc")[0] == '.')
        || cst_regex_match(en_rx_dotted_letters, name)) {
        if (punc[0] == '.')
            punc++;
    }
    if (en_is_initial(tok))
        return int_val(-1);
    if (punc[0] && strchr(".!?", punc[strlen(punc) - 1]))
        return int_val(1);
    return int_val(0);
}
