/*
 * English feature functions (0x3aaa4..; compare Flite's us_ffeatures.c): gpos (with the part
 * after or before an apostrophe), the lexicon's parts of speech, and next_word_punc.
 */
#include <stddef.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_ffeatures.h"
#include "cst_item.h"
#include "cst_string.h"
#include "cst_val.h"
#include "cst_val_const.h"

cst_item *item_traverse(const cst_item *item, ...);
const cst_val *item_feat_func(const cst_item *item, const char *feat);
int en_lex_pos_list(const cst_item *item, char *buf, size_t max_len);
int en_gb_lex_pos_list(const char *word, char *buf, size_t max_len);
int en_us_lex_pos_list(const char *word, char *buf, size_t max_len);
char token_single_alpha(const char *word, int (*pos_list)(const char *word, char *buf, size_t max));
void basic_ff_register(cst_features *ffunctions);
void ff_register(cst_features *ffeatures, const char *name, cst_ffunction f);

extern const cst_val *const *const en_gpos[];

DEF_STATIC_CONST_VAL_STRING(en_val_y, "y");
DEF_STATIC_CONST_VAL_STRING(en_val_n, "n");
DEF_STATIC_CONST_VAL_STRING(en_val_content, "content");

/* 0x3aaa4 arm: the punctuation after the word, or after the next words without any */
static const cst_val *next_word_punc(const cst_item *word)
{
    while (item_next(word)) {
        if (*ffeature_string(item_traverse(word, 0), "word_punc"))
            break;
        word = item_next(word);
    }
    return item_feat_func(item_traverse(word, 0), "word_punc");
}

/* 0x3ab18 arm: the parts of speech the lexicon has for the word (cached as "lex_all_pos") */
static const cst_val *lex_all_pos(const cst_item *word)
{
    char buf[0x101];
    int n;

    if (!item_feat_present(word, "lex_all_pos")) {
        n = en_lex_pos_list(word, buf, 0x100);
        buf[n] = '\0';
        item_set_string((cst_item *)word, "lex_all_pos", buf);
    }
    return item_feat(word, "lex_all_pos");
}

#define LEX_POS(name, c)                                                   \
    static const cst_val *name(const cst_item *word)                      \
    {                                                                       \
        if (strchr(val_string(lex_all_pos(word)), c))                      \
            return &en_val_y;                                           \
        return &en_val_n;                                               \
    }

LEX_POS(lex_pos_v, 'v')   /* 0x3ab8c arm */
LEX_POS(lex_pos_t, 't')   /* 0x3abcc arm */
LEX_POS(lex_pos_r, 'r')   /* 0x3ac0c arm */
LEX_POS(lex_pos_p, 'p')   /* 0x3ac4c arm */
LEX_POS(lex_pos_n, 'n')   /* 0x3ac8c arm */
LEX_POS(lex_pos_i, 'i')   /* 0x3accc arm */
LEX_POS(lex_pos_d, 'd')   /* 0x3ad0c arm */
LEX_POS(lex_pos_c, 'c')   /* 0x3ad4c arm */
LEX_POS(lex_pos_a, 'a')   /* 0x3ad8c arm */

/* 0x3adcc arm: the word's one alphabetic part of speech in its lexicon, or "?" (cached as
 * "lex_pos") */
static const cst_val *lex_pos(const cst_item *word)
{
    char pos[2];
    int (*pos_list)(const char *, char *, size_t);

    if (!item_feat_present(word, "lex_pos")) {
        pos[0] = pos[1] = '\0';
        if (strcmp(ffeature_string(item_traverse(word, 0), "language_code"), "en-AU") == 0
            || cst_startswith(ffeature_string(item_traverse(word, 0), "language_code"), "en-GB")
            || strcmp(ffeature_string(item_traverse(word, 0), "language_code"), "en-IN") == 0)
            pos_list = en_gb_lex_pos_list;
        else
            pos_list = en_us_lex_pos_list;
        pos[0] = token_single_alpha(item_feat_string(word, "name"), pos_list);
        item_set_string((cst_item *)word, "lex_pos", pos);
    }
    return item_feat(word, "lex_pos");
}

/* 0x3af08 arm: the word for content words, else its gpos */
static const cst_val *gpos_name(const cst_item *word)
{
    if (strcmp(ffeature_string(item_traverse(word, 0), "gpos"), "content") != 0)
        return item_feat_func(item_traverse(word, 0), "name");
    return item_feat_func(item_traverse(word, 0), "gpos");
}

/* the part of speech of the function word w, or NULL */
static inline const cst_val *gpos_of(const char *w)
{
    int s, t;

    for (s = 0; en_gpos[s]; s++)
        for (t = 1; en_gpos[s][t]; t++)
            if (strcmp(w, val_string(en_gpos[s][t])) == 0)
                return en_gpos[s][0];
    return NULL;
}

/* 0x3af74 arm: the gpos of the word, else of its part before an apostrophe */
static const cst_val *gpos_left(const cst_item *word)
{
    const char *w = item_feat_string(word, "name"), *a;
    const cst_val *p;
    char *left;

    if ((p = gpos_of(w)) != NULL)
        return p;
    a = strchr(w, '\'');
    if (a == w || a == NULL)
        return &en_val_content;
    left = cst_substr(w, 0, a - w);
    p = gpos_of(left);
    cst_free(left);
    return p ? p : &en_val_content;
}

/* 0x3b0c8 arm: the gpos of the word, else of its last apostrophe's part ("'s") */
static const cst_val *gpos(const cst_item *word)
{
    const char *w = item_feat_string(word, "name"), *a;
    const cst_val *p;

    if ((p = gpos_of(w)) != NULL)
        return p;
    if ((a = strrchr(w, '\'')) != NULL && (p = gpos_of(a)) != NULL)
        return p;
    return &en_val_content;
}

/* 0x3b318 arm */
void en_ff_register(cst_features *ffunctions)
{
    basic_ff_register(ffunctions);
    ff_register(ffunctions, "gpos", gpos);
    ff_register(ffunctions, "gpos_left", gpos_left);
    ff_register(ffunctions, "lex_pos", lex_pos);
    ff_register(ffunctions, "lex_pos_a", lex_pos_a);
    ff_register(ffunctions, "lex_pos_c", lex_pos_c);
    ff_register(ffunctions, "lex_pos_d", lex_pos_d);
    ff_register(ffunctions, "lex_pos_i", lex_pos_i);
    ff_register(ffunctions, "lex_pos_n", lex_pos_n);
    ff_register(ffunctions, "lex_pos_p", lex_pos_p);
    ff_register(ffunctions, "lex_pos_r", lex_pos_r);
    ff_register(ffunctions, "lex_pos_t", lex_pos_t);
    ff_register(ffunctions, "lex_pos_v", lex_pos_v);
    ff_register(ffunctions, "next_word_punc", next_word_punc);
    ff_register(ffunctions, "gpos_name", gpos_name);
}
