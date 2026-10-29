#include "cst_item.h"
#include "cst_val.h"
#include "cst_val_const.h"
#include "cst_utterance.h"
#include "cst_features.h"
#include "cst_phoneset.h"
#include "cst_string.h"

cst_item *item_as(const cst_item *i, int r);
void *item_lang_data(const cst_item *item);
const cst_item *item_traverse(const cst_item *i, int path, ...);
const cst_val *item_feat_func(const cst_item *i, const char *feat);

typedef struct ivona_lang_core_struct {
    char pad_00[0x14];
    cst_features *features;
} ivona_lang_core;

DEF_STATIC_CONST_VAL_STRING(val_string_single, "single");
DEF_STATIC_CONST_VAL_STRING(val_string_mid, "mid");
DEF_STATIC_CONST_VAL_STRING(val_string_final, "final");
DEF_STATIC_CONST_VAL_STRING(val_string_initial, "initial");
DEF_STATIC_CONST_VAL_STRING(val_string_onset, "onset");
DEF_STATIC_CONST_VAL_STRING(val_string_coda, "coda");
DEF_STATIC_CONST_VAL_STRING(val_string_empty, "");

/* 0x5875c: arm */
const cst_val *FUN_0005875c(void)
{
    return VAL_STRING_1;
}

/* 0x5876c: arm */
int FUN_0005876c(const cst_item *item)
{
    cst_item *d;
    int c;

    for (c = 0, d = item_daughter(item_as(item, 4));
         d;
         d = item_next(d), c++);

    return c;
}

/* 0x587a8: arm */
const cst_val *FUN_000587a8(const cst_item *item)
{
    cst_item *s = item_as(item, 4);

    if (s != NULL && item_next(s) != NULL)
        return VAL_STRING_0;
    return VAL_STRING_1;
}

/* 0x587f4: arm */
const cst_val *FUN_000587f4(const cst_item *item)
{
    return val_string_n(FUN_0005876c(item));
}

/* 0x58804: arm */
const cst_val *FUN_00058804(const cst_item *word)
{
    cst_item *ww = item_as(word, 3);

    if (ww == NULL || item_next(ww) != NULL)
        return VAL_STRING_1;
    else {
        cst_item *pp = item_parent(ww);
        const char *pname = val_string(item_feat(pp, "name"));
        if (strcmp("BB", pname) == 0)
            return VAL_STRING_4;
        else if (strcmp("B", pname) == 0)
            return VAL_STRING_3;
        else
            return VAL_STRING_1;
    }
}

/* 0x588b8: arm */
const cst_val *FUN_000588b8(const cst_item *item)
{
    cst_utterance *u = item_utt(item);
    ivona_lang_core *core = (ivona_lang_core *)item_lang_data(item);
    const cst_val *v;

    if (core != NULL && feat_present(core->features, "language_code")) {
        v = feat_val(core->features, "language_code");
        asm volatile("" : "+r"(v));
        return v;
    }
    if (feat_present(u->features, "language_code")) {
        v = feat_val(u->features, "language_code");
        asm volatile("" : "+r"(v));
        return v;
    }
    return VAL_STRING_0;
}

/* 0x58948: arm */
int FUN_00058948(const cst_item *item)
{
    return item_feat_int(item, "stress") > 0;
}

/* 0x5896c: arm */
const cst_val *FUN_0005896c(const cst_item *item)
{
    const cst_item *s;
    int c;

    for (c = 1, s = item_next(item_as(item, 5)); s; s = item_next(s), c++) {
        if (item_feat_int(s, "stress") > 0)
            return val_string_n(c);
    }
    return VAL_STRING_0;
}

/* 0x589dc: arm */
const cst_val *FUN_000589dc(const cst_item *item)
{
    const cst_item *s;
    int c;

    for (c = 1, s = item_prev(item_as(item, 5)); s; s = item_prev(s), c++) {
        if (item_feat_int(s, "stress") > 0)
            return val_string_n(c);
    }
    return VAL_STRING_0;
}

/* 0x58a4c: arm */
const cst_val *FUN_00058a4c(const cst_item *item)
{
    const cst_item *s = item_as(item, 3);
    int c;

    if (s == NULL) {
        c = -1;
        asm ("" : "+r"(c));
        return val_string_n(c);
    }

    c = -1;
    do {
        s = item_prev(s);
        c++;
    } while (s != NULL);

    return val_string_n(c);
}

/* 0x58a88: arm */
const cst_val *FUN_00058a88(const cst_item *item)
{
    const cst_item *s = item_as(item, 3);
    int c;

    if (s != NULL) {
        c = 0;
        do {
            s = item_prev(s);
            c++;
        } while (s != NULL);
    } else {
        c = (int)s;
    }

    s = item_next(item_as(item, 3));
    if (s != NULL) {
        do {
            s = item_next(s);
            c++;
        } while (s != NULL);
    }

    return val_string_n(c);
}

/* 0x58af0: arm */
const cst_val *FUN_00058af0(const cst_item *syl)
{
    const cst_item *s = item_as(syl, 4);

    if (s == NULL)
        return (const cst_val *)&val_string_single;
    else if (item_next(s) == NULL) {
        if (item_prev(s) == NULL)
            return (const cst_val *)&val_string_single;
        else
            return (const cst_val *)&val_string_final;
    } else if (item_prev(s) == NULL)
        return (const cst_val *)&val_string_initial;
    else
        return (const cst_val *)&val_string_mid;
}

/* 0x58de0: arm */
const cst_val *FUN_00058de0(const cst_item *seg)
{
    const cst_item *s;
    const cst_phoneset *ps = item_phoneset(seg);

    for (s = item_next(item_as(seg, 4)); s; s = item_next(s)) {
        if (cst_streq("+", phone_feature_string(ps, item_feat_string(s, "name"), "vc")))
            return (const cst_val *)&val_string_onset;
    }
    return (const cst_val *)&val_string_coda;
}

/* 0x58e98: arm */
const cst_val *seg_onset_ctype(const cst_item *seg, const char *ctype)
{
    const cst_item *s;
    const cst_phoneset *ps = item_phoneset(seg);

    for (s = item_daughter(item_parent(item_as(seg, 4))); s; s = item_next(s)) {
        if (cst_streq("+", phone_feature_string(ps, item_feat_string(s, "name"), "vc")))
            return VAL_STRING_0;
        if (cst_streq(ctype, phone_feature_string(ps, item_feat_string(s, "name"), "ctype")))
            return VAL_STRING_1;
    }
    return VAL_STRING_0;
}

/* 0x58f8c: arm */
const cst_val *seg_onset_nasal(const cst_item *seg)
{
    return seg_onset_ctype(seg, "n");
}

/* 0x58f9c: arm */
const cst_val *seg_onset_stop(const cst_item *seg)
{
    return seg_onset_ctype(seg, "s");
}

/* 0x58fac: arm */
const cst_val *seg_onset_fric(const cst_item *seg)
{
    return seg_onset_ctype(seg, "f");
}

/* 0x58fbc: arm */
const cst_val *seg_coda_ctype(const cst_item *seg, const char *ctype)
{
    const cst_item *s;
    const cst_phoneset *ps = item_phoneset(seg);

    for (s = item_last_daughter(item_parent(item_as(seg, 4))); s; s = item_prev(s)) {
        if (cst_streq("+", phone_feature_string(ps, item_feat_string(s, "name"), "vc")))
            return VAL_STRING_0;
        if (cst_streq(ctype, phone_feature_string(ps, item_feat_string(s, "name"), "ctype")))
            return VAL_STRING_1;
    }
    return VAL_STRING_0;
}

/* 0x590b0: arm */
const cst_val *seg_coda_nasal(const cst_item *seg)
{
    return seg_coda_ctype(seg, "n");
}

/* 0x590c0: arm */
const cst_val *seg_coda_stop(const cst_item *seg)
{
    return seg_coda_ctype(seg, "s");
}

/* 0x590d0: arm */
const cst_val *seg_coda_fric(const cst_item *seg)
{
    return seg_coda_ctype(seg, "f");
}

/* 0x590e0: arm */
const cst_val *FUN_000590e0(const cst_item *word)
{
    cst_item *ww = item_as(word, 1);
    const cst_val *v;

    if (ww != NULL && item_next(ww) != NULL)
        return (const cst_val *)&val_string_empty;
    v = item_feat_func(item_traverse(item_parent(ww), 0), "punc");
    asm volatile("" : "+r"(v));
    return v;
}

/* 0x59138: arm */
const cst_val *ph_cvox(const cst_item *p)
{
    return phone_feature(item_phoneset(p), item_name(p), "cvox");
}

/* 0x59178: arm */
const cst_val *ph_cplace(const cst_item *p)
{
    return phone_feature(item_phoneset(p), item_name(p), "cplace");
}

/* 0x591b8: arm */
const cst_val *ph_ctype(const cst_item *p)
{
    return phone_feature(item_phoneset(p), item_name(p), "ctype");
}

/* 0x591f8: arm */
const cst_val *ph_vrnd(const cst_item *p)
{
    return phone_feature(item_phoneset(p), item_name(p), "vrnd");
}

/* 0x59238: arm */
const cst_val *ph_vfront(const cst_item *p)
{
    return phone_feature(item_phoneset(p), item_name(p), "vfront");
}

/* 0x59278: arm */
const cst_val *ph_vheight(const cst_item *p)
{
    return phone_feature(item_phoneset(p), item_name(p), "vheight");
}

/* 0x592b8: arm */
const cst_val *ph_vlng(const cst_item *p)
{
    return phone_feature(item_phoneset(p), item_name(p), "vlng");
}

/* 0x592f8: arm */
const cst_val *ph_vc(const cst_item *p)
{
    return phone_feature(item_phoneset(p), item_name(p), "vc");
}








/* Flite's syllable, phrase and accent features (cst_ffeatures.c) with IVONA's relation ids
 * (3 Phrase, 4 SylStructure, 5 Syllable, 6 Segment, 8 the punctuation phrase), "stress" above 0
 * for a stressed syllable, and accents and end tones other than "NONE". */
#include "cst_ffeatures.h"

typedef const cst_val *(*cst_ffunction2)(const cst_item *i, const char *arg);
cst_item *item_first_sibling(const cst_item *i);
void ff_register(cst_features *ffeatures, const char *name, cst_ffunction f);
void ff_register2(cst_features *ffeatures, const char *name, cst_ffunction2 f);
const cst_val *item_word_embedding_coeff(const cst_item *item, const char *arg);
const cst_val *seg_coda_ctype(const cst_item *seg, const char *ctype);

/* syllables from the start of the phrase (relation rel) to syl: stressed (stress) or all */
static inline int syl_count_in(const cst_item *syl, int rel, int stress)
{
    const cst_item *ss = item_as(syl, 5), *p;
    int c = 0;

    for (p = item_as(item_traverse(syl, 4, 0x106, rel, 0x109, 4, 0x107, 0), 5);
         p && !item_equal(p, ss); p = item_next(p))
        if (!stress || item_feat_int(p, "stress") > 0)
            c++;
    return c;
}

/* stressed syllables after syl to the end of the phrase */
static inline int ssyl_count_out(const cst_item *syl, int rel)
{
    const cst_item *ss = item_as(syl, 5);
    const cst_item *last = item_as(item_traverse(syl, 4, 0x106, rel, 0x10a, 4, 0x108, 0), 5);
    const cst_item *p, *end = item_next(last);
    int c = 0;

    for (p = item_next(ss); p && !item_equal(p, end); p = item_next(p))
        if (item_feat_int(p, "stress") > 0)
            c++;
    return c;
}

/* syllables from syl to the end of the phrase */
static inline int syl_count_out(const cst_item *syl, int rel)
{
    const cst_item *p = item_as(syl, 5);
    const cst_item *last = item_as(item_traverse(syl, 4, 0x106, rel, 0x10a, 4, 0x108, 0), 5);
    int c = 0;

    for (; p && !item_equal(p, last); p = item_next(p))
        c++;
    return c;
}

/* 0x59834 arm */
const cst_val *ssyl_in(const cst_item *syl)
{
    return val_string_n(syl_count_in(syl, 3, 1));
}

/* 0x599a4 arm */
const cst_val *ssyl_out(const cst_item *syl)
{
    return val_string_n(ssyl_count_out(syl, 3));
}

/* 0x59bf4 arm */
const cst_val *syl_in(const cst_item *syl)
{
    return val_string_n(syl_count_in(syl, 3, 0));
}

/* 0x59d2c arm */
const cst_val *syl_out(const cst_item *syl)
{
    return val_string_n(syl_count_out(syl, 3));
}

/* 0x598ec arm */
const cst_val *ssyl_in_punc(const cst_item *syl)
{
    return val_string_n(syl_count_in(syl, 8, 1));
}

/* 0x595a8 arm */
const cst_val *ssyl_out_punc(const cst_item *syl)
{
    return val_string_n(ssyl_count_out(syl, 8));
}

/* 0x59c90 arm */
const cst_val *syl_in_punc(const cst_item *syl)
{
    return val_string_n(syl_count_in(syl, 8, 0));
}

/* 0x5950c arm */
const cst_val *syl_out_punc(const cst_item *syl)
{
    return val_string_n(syl_count_out(syl, 8));
}

/* 0x59724 arm: the consonants before the syllable's vowel */
const cst_val *syl_onsetsize(const cst_item *syl)
{
    const cst_item *p;
    int c = 0;

    for (p = item_daughter(item_as(syl, 4)); p; p = item_next(p), c++)
        if (strcmp("+", val_string(phone_feature(item_phoneset(p), item_feat_string(p, "name"),
                                                 "vc"))) == 0)
            break;
    return val_string_n(c);
}

/* 0x59678 arm: the consonants after the syllable's vowel */
const cst_val *syl_codasize(const cst_item *syl)
{
    const cst_item *p;
    int c = 0;

    for (p = item_last_daughter(item_as(syl, 4)); p; p = item_prev(p), c++)
        if (strcmp("+", val_string(phone_feature(item_phoneset(p), item_feat_string(p, "name"),
                                                 "vc"))) == 0)
            break;
    return val_string_n(c);
}

/* 0x59338 arm: whether the syllable has an accent or an end tone */
const cst_val *accented(const cst_item *syl)
{
    if ((item_feat_present(syl, "accent")
         && strcmp(ffeature_string(item_traverse(syl, 0), "accent"), "NONE") != 0)
        || (item_feat_present(syl, "endtone")
            && strcmp(ffeature_string(item_traverse(syl, 0), "endtone"), "NONE") != 0))
        return VAL_STRING_1;
    return VAL_STRING_0;
}

static inline const cst_val *accented_i(const cst_item *syl)
{
    if ((item_feat_present(syl, "accent")
         && strcmp(ffeature_string(item_traverse(syl, 0), "accent"), "NONE") != 0)
        || (item_feat_present(syl, "endtone")
            && strcmp(ffeature_string(item_traverse(syl, 0), "endtone"), "NONE") != 0))
        return VAL_STRING_1;
    return VAL_STRING_0;
}

/* 0x5a2b0 arm: accented syllables before syl since the phrase's start */
const cst_val *asyl_in(const cst_item *syl)
{
    const cst_item *ss = item_as(syl, 5), *p;
    const cst_item *fs = item_traverse(syl, 4, 0x106, 3, 0x109, 4, 0x107, 0);
    int c = 0;

    if (item_equal(ss, fs))
        return VAL_STRING_0;
    for (p = item_prev(ss); p; p = item_prev(p)) {
        if (val_int(accented_i(p)) == 1)
            c++;
        if (item_equal(p, fs))
            break;
    }
    return val_string_n(c);
}

/* 0x5a120 arm: accented syllables after syl to the phrase's end */
const cst_val *asyl_out(const cst_item *syl)
{
    const cst_item *ss = item_as(syl, 5), *p;
    const cst_item *ls = item_traverse(syl, 4, 0x106, 3, 0x10a, 4, 0x108, 0);
    int c = 0;

    if (item_equal(ss, ls))
        return VAL_STRING_0;
    for (p = item_next(ss); p; p = item_next(p)) {
        if (val_int(accented_i(p)) == 1)
            c++;
        if (item_equal(p, ls))
            break;
    }
    return val_string_n(c);
}

/* 0x59454 arm */
const cst_val *seg_coda_glide(const cst_item *seg)
{
    if (seg_coda_ctype(seg, "r") != VAL_STRING_0)
        return VAL_STRING_1;
    return seg_coda_ctype(seg, "l");
}

/* 0x59400 arm */
const cst_val *seg_onset_glide(const cst_item *seg)
{
    if (seg_onset_ctype(seg, "r") != VAL_STRING_0)
        return VAL_STRING_1;
    return seg_onset_ctype(seg, "l");
}

/* 0x58b78 arm: the segment's position in its syllable */
const cst_val *pos_in_syl(const cst_item *seg)
{
    const cst_item *p;
    int c = -1;

    for (p = item_as(seg, 4); p; p = item_prev(p))
        c++;
    return val_string_n(c);
}

/* 0x58d00 arm: the phrases before the word's */
const cst_val *sub_phrases(const cst_item *syl)
{
    const cst_item *p;
    int c = 0;

    for (p = item_traverse(syl, 4, 0x106, 3, 0x106, 0x103, 0); p; p = item_prev(p))
        c++;
    return val_string_n(c);
}

/* 0x59ec0 arm: syllables back to the last accented one */
const cst_val *last_accent(const cst_item *syl)
{
    const cst_item *p;
    int c = 0;

    for (p = item_prev(item_as(syl, 5)); p; p = item_prev(p), c++)
        if (val_int(accented_i(p)) == 1)
            break;
    return val_string_n(c);
}

/* 0x59ff0 arm: syllables on to the next accented one */
const cst_val *next_accent(const cst_item *syl)
{
    const cst_item *p;
    int c = 0;

    for (p = item_next(item_as(syl, 5)); p; p = item_next(p), c++)
        if (val_int(accented_i(p)) == 1)
            break;
    return val_string_n(c);
}

/* 0x58d54 arm: the syllable's vowel */
const cst_val *syl_vowel(const cst_item *syl)
{
    const cst_item *p;

    for (p = item_daughter(item_as(syl, 4)); p; p = item_next(p))
        if (strcmp(ffeature_string(item_traverse(p, 0), "ph_vc"), "+") == 0)
            return item_feat(p, "name");
    return VAL_STRING_0;
}

/* 0x58c4c arm */
const cst_val *segment_duration(const cst_item *seg)
{
    const cst_item *s = item_as(seg, 6);

    if (s == NULL)
        return VAL_STRING_0;
    if (item_prev(s) == NULL)
        return item_feat(s, "end");
    if (item_feat_present(s, "end") && item_feat_present(item_prev(s), "end"))
        return float_val(item_feat_float(s, "end") - item_feat_float(item_prev(s), "end"));
    return VAL_STRING_0;
}

/* 0x58c14 arm */
const cst_val *syl_num_phones(const cst_item *syl)
{
    const cst_item *p;
    int c = 0;

    for (p = item_daughter(item_as(syl, 4)); p; p = item_next(p))
        c++;
    return val_int_n(c);
}

/* the words of the word's phrase (relation rel) */
static inline int phrase_words(const cst_item *word, int rel)
{
    const cst_item *p;
    int c = 0;

    for (p = item_daughter(item_as(word, rel)); p && !item_equal(p, NULL); p = item_next(p))
        c++;
    return c;
}

/* the syllables of the word's phrase */
static inline int phrase_syls(const cst_item *word, int rel)
{
    const cst_item *p;
    int c = 0;

    for (p = item_daughter(item_as(word, rel)); p && !item_equal(p, NULL); p = item_next(p))
        c += FUN_0005876c(p);
    return c;
}

/* 0x59a74 arm */
const cst_val *phrase_num_words(const cst_item *word)
{
    return val_string_n(phrase_words(word, 3));
}

/* 0x59ad0 arm */
const cst_val *punc_phrase_num_words(const cst_item *word)
{
    return val_string_n(phrase_words(word, 8));
}

/* 0x597d0 arm */
const cst_val *phrase_num_syls(const cst_item *word)
{
    return val_string_n(phrase_syls(word, 3));
}

/* 0x594a8 arm */
const cst_val *punc_phrase_num_syls(const cst_item *word)
{
    return val_string_n(phrase_syls(word, 8));
}

/* 0x58bb4 arm: the syllable's position in its word */
const cst_val *pos_in_word(const cst_item *syl)
{
    const cst_item *ss = item_as(syl, 4), *p;
    int c = 0;

    for (p = item_first_sibling(ss); p && !item_equal(p, ss); p = item_next(p))
        c++;
    return val_string_n(c);
}

/* 0x59b2c arm: the word's position in its phrase */
const cst_val *pos_in_phrase(const cst_item *word)
{
    const cst_item *ww = item_as(word, 3), *p;
    int c = 0;

    for (p = item_daughter(item_parent(ww)); p && !item_equal(p, ww); p = item_next(p))
        c++;
    return val_string_n(c);
}

/* 0x59b90 arm: the word's position in its punctuation phrase */
const cst_val *punc_pos_in_phrase(const cst_item *word)
{
    const cst_item *ww = item_as(word, 8), *p;
    int c = 0;

    for (p = item_daughter(item_parent(ww)); p && !item_equal(p, ww); p = item_next(p))
        c++;
    return val_string_n(c);
}

/* 0x59dc8 arm: the break after the syllable: 0 inside a word, 1 between words, 3 at a phrase
 * end ("B"), 4 at a sentence end ("BB") */
const cst_val *syl_break(const cst_item *syl)
{
    const cst_item *ss = item_as(syl, 4), *w;
    const char *name;

    if (ss == NULL)
        return VAL_STRING_1;
    if (item_next(ss))
        return VAL_STRING_0;
    if (item_parent(ss) == NULL)
        return VAL_STRING_1;
    w = item_as(item_parent(ss), 3);
    if (w == NULL || item_next(w))
        return VAL_STRING_1;
    name = val_string(item_feat(item_parent(w), "name"));
    if (strcmp("BB", name) == 0)
        return VAL_STRING_4;
    if (strcmp("B", name) == 0)
        return VAL_STRING_3;
    return VAL_STRING_1;
}

/* 0x5a444 arm: registers the feature functions */
void basic_ff_register(cst_features *ffunctions)
{
    ff_register(ffunctions, "ph_vc", ph_vc);
    ff_register(ffunctions, "ph_vlng", ph_vlng);
    ff_register(ffunctions, "ph_vheight", ph_vheight);
    ff_register(ffunctions, "ph_vfront", ph_vfront);
    ff_register(ffunctions, "ph_vrnd", ph_vrnd);
    ff_register(ffunctions, "ph_ctype", ph_ctype);
    ff_register(ffunctions, "ph_cplace", ph_cplace);
    ff_register(ffunctions, "ph_cvox", ph_cvox);
    ff_register(ffunctions, "word_num_syls", FUN_000587f4);
    ff_register(ffunctions, "word_break", FUN_00058804);
    ff_register(ffunctions, "word_punc", FUN_000590e0);
    ff_register(ffunctions, "ssyl_in", ssyl_in);
    ff_register(ffunctions, "ssyl_out", ssyl_out);
    ff_register(ffunctions, "syl_in", syl_in);
    ff_register(ffunctions, "syl_out", syl_out);
    ff_register(ffunctions, "ssyl_in_punc", ssyl_in_punc);
    ff_register(ffunctions, "ssyl_out_punc", ssyl_out_punc);
    ff_register(ffunctions, "syl_in_punc", syl_in_punc);
    ff_register(ffunctions, "syl_out_punc", syl_out_punc);
    ff_register(ffunctions, "syl_break", syl_break);
    ff_register(ffunctions, "syl_onsetsize", syl_onsetsize);
    ff_register(ffunctions, "syl_codasize", syl_codasize);
    ff_register(ffunctions, "accented", accented);
    ff_register(ffunctions, "asyl_in", asyl_in);
    ff_register(ffunctions, "asyl_out", asyl_out);
    ff_register(ffunctions, "seg_coda_fric", seg_coda_fric);
    ff_register(ffunctions, "seg_onset_fric", seg_onset_fric);
    ff_register(ffunctions, "seg_coda_stop", seg_coda_stop);
    ff_register(ffunctions, "seg_onset_stop", seg_onset_stop);
    ff_register(ffunctions, "seg_coda_nasal", seg_coda_nasal);
    ff_register(ffunctions, "seg_onset_nasal", seg_onset_nasal);
    ff_register(ffunctions, "seg_coda_glide", seg_coda_glide);
    ff_register(ffunctions, "seg_onset_glide", seg_onset_glide);
    ff_register(ffunctions, "seg_onsetcoda", FUN_00058de0);
    ff_register(ffunctions, "pos_in_syl", pos_in_syl);
    ff_register(ffunctions, "position_type", FUN_00058af0);
    ff_register(ffunctions, "sub_phrases", sub_phrases);
    ff_register(ffunctions, "last_accent", last_accent);
    ff_register(ffunctions, "next_accent", next_accent);
    ff_register(ffunctions, "syl_final", FUN_000587a8);
    ff_register(ffunctions, "syl_vowel", syl_vowel);
    ff_register(ffunctions, "segment_duration", segment_duration);
    ff_register(ffunctions, "syl_num_phones", syl_num_phones);
    ff_register(ffunctions, "phrase_num_words", phrase_num_words);
    ff_register(ffunctions, "punc_phrase_num_words", punc_phrase_num_words);
    ff_register(ffunctions, "phrase_num_syls", phrase_num_syls);
    ff_register(ffunctions, "punc_phrase_num_syls", punc_phrase_num_syls);
    ff_register(ffunctions, "num_phrases", FUN_00058a88);
    ff_register(ffunctions, "pos_in_word", pos_in_word);
    ff_register(ffunctions, "pos_in_phrase", pos_in_phrase);
    ff_register(ffunctions, "punc_pos_in_phrase", punc_pos_in_phrase);
    ff_register(ffunctions, "pos_in_utt", FUN_00058a4c);
    ff_register(ffunctions, "ssyl_from", FUN_000589dc);
    ff_register(ffunctions, "ssyl_to", FUN_0005896c);
    ff_register(ffunctions, "language_code", FUN_000588b8);
    ff_register(ffunctions, "exists", (cst_ffunction)FUN_0005875c);
    ff_register2(ffunctions, "word_embedding_", item_word_embedding_coeff);
}
