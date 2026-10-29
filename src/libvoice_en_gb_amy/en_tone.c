/*
 * English phrase-final tones (0x3b768..): questions rise ("H") unless they are wh-questions,
 * alternative questions ("tea or coffee?") or "please" requests; statements fall.
 */
#include <string.h>
#include "cst_ffeatures.h"
#include "cst_item.h"
#include "cst_string.h"
#include "cst_val.h"

cst_item *item_traverse(const cst_item *item, ...);
int item_is_phrase_break(const cst_item *item, const char *punc_break);
cst_item *phrase_start_token(const cst_item *item, const char *punc_break);
cst_item *phrase_end_token(const cst_item *item, const char *punc_break);
const char *punc_last_char(const cst_item *item);

#define PHRASE_PUNC ".:;!?"

static const char *const en_so_well[] = { "so", "well", NULL };
static const char *const en_neither[] = { "neither", NULL };
static const char *const en_do_forms[] = {
    "do", "don't", "does", "doesn't", "did", "didn't", "need", "needn't", NULL
};
static const char *const en_alt_question_feat = "en_intonation_is_alt_question";

/* 0x3b768 arm: whether a later phrase of the sentence is a question starting with "or" */
int en_has_or_question(const cst_item *tok)
{
    const cst_item *t;

    for (t = item_next(tok); t; t = item_next(t)) {
        if (strcmp("?", punc_last_char(phrase_end_token(t, PHRASE_PUNC))) == 0 && item_prev(t)
            && strcmp("or", ffeature_string(item_traverse(t, 0x107, 0), "name")) == 0)
            return 1;
        if (item_is_phrase_break(t, PHRASE_PUNC))
            break;
    }
    return 0;
}

/* 0x3b850 arm: whether tok's phrase is a wh-question (its first word, after "so", "well" or a
 * conjunction, a wh-word), "what was that" aside */
int en_is_wh_question(const cst_item *tok)
{
    const cst_item *end = phrase_end_token(tok, PHRASE_PUNC);
    const cst_item *w = item_daughter(end), *t;

    if (ffeature_int(item_traverse(end, 0), "phrase_num_words") == 3
        && strcmp(ffeature_string(item_traverse(w, 0), "name"), "what") == 0
        && strcmp(ffeature_string(item_traverse(w, 0x100, 0), "name"), "was") == 0
        && strcmp(ffeature_string(item_traverse(w, 0x101, 0), "name"), "that") == 0)
        return 0;
    for (t = phrase_start_token(tok, PHRASE_PUNC); t; t = item_next(t)) {
        w = item_daughter(t);
        if (cst_member_string(item_feat_string(w, "name"), en_so_well)
            || strcmp("cc", ffeature_string(item_traverse(w, 0), "gpos")) == 0)
            w = item_next(w);
        if (strcmp("wp", ffeature_string(item_traverse(w, 0), "gpos_left")) == 0)
            return 1;
        if (item_equal(t, end))
            break;
    }
    return 0;
}

/* 0x3ba54 arm: the polarity of the yes/no question whose auxiliary is in tok or before it:
 * 1, -1 when negated ("n't", "not" or after "neither"), 0 without a following word */
int en_question_polarity(const cst_item *tok)
{
    const cst_item *w;
    const char *gpos, *name;
    int neither = 0, neg;
    size_t n;

    for (; tok; tok = item_prev(tok)) {
        for (w = item_daughter(tok); w; w = item_next(w)) {
            gpos = ffeature_string(item_traverse(w, 0), "gpos_left");
            name = item_feat_string(w, "name");
            if (cst_member_string(name, en_neither)) {
                neither = 1;
                continue;
            }
            if (strcmp(gpos, "cc") == 0)
                continue;
            if (strcmp(gpos, "md") != 0 && strcmp(gpos, "aux") != 0
                && !cst_member_string(name, en_do_forms))
                break;
            n = strlen(name);
            neg = n > 3 && (strcmp(name + n - 3, "n't") == 0 || strcmp(name + n - 3, "not") == 0);
            if (neg || neither || strcmp(ffeature_string(item_traverse(w, 0x100, 0), "name"), "not") == 0) {
                if (strcmp(ffeature_string(item_traverse(w, 0x100, 0), "name"), "not") == 0)
                    return item_next(item_next(w)) ? -1 : 0;
                return item_next(w) ? -1 : 0;
            }
            return item_next(w) ? 1 : 0;
        }
    }
    return 0;
}

/* 0x3bcb8 arm: whether tok's phrase is an alternative question (cached on its tokens) */
int en_is_alt_question(const cst_item *tok)
{
    const cst_val *v = item_feat(tok, en_alt_question_feat);
    cst_item *start, *t;
    int alt = 0;

    if (v)
        return val_int(v);
    if ((start = phrase_start_token(tok, PHRASE_PUNC)) == NULL)
        return 0;
    for (t = start; t; t = item_next(t))
        if (en_question_polarity(t)) {
            if (en_question_polarity(t) == 1)
                alt = en_has_or_question(t) != 0;
            break;
        }
    for (t = start; t; t = item_next(t)) {
        item_set_int(t, en_alt_question_feat, alt);
        if (item_is_phrase_break(t, PHRASE_PUNC))
            break;
    }
    return alt;
}

/* 0x3bdac arm: the phrasesylendtone_func: the tone at the end of a phrase ("BB": sentence,
 * "B": phrase) */
cst_val *en_phrase_syl_end_tone(cst_item *item)
{
    if (strcmp("BB", item_feat_string(item, "name")) == 0
        && strcmp("?", punc_last_char(item)) == 0) {
        if (en_is_alt_question(item))
            return string_val("L");
        if (en_is_wh_question(item)) {
            if (ffeature_int(item_traverse(item, 0), "phrase_num_words") == 1
                && strcmp("content", ffeature_string(item_traverse(item, 0x107, 0), "gpos")) == 0)
                return string_val(strcmp("please", ffeature_string(item_traverse(item, 0x107, 0),
                                                                   "name")) == 0 ? "L" : "H");
            return string_val("L");
        }
        if (ffeature_int(item_traverse(item, 0), "phrase_num_words") == 1
            && strcmp("please", ffeature_string(item_traverse(item, 0x107, 0), "name")) == 0)
            return string_val("L");
        return string_val("H");
    }
    if (strcmp("BB", item_feat_string(item, "name")) == 0)
        return string_val("L");
    if (strcmp("B", item_feat_string(item, "name")) == 0) {
        if (en_is_alt_question(item) && en_has_or_question(item) && !en_is_wh_question(item)
            && en_question_polarity(item))
            return string_val("H");
        return string_val("L*");
    }
    return NULL;
}
