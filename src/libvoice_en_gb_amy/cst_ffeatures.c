#include "cst_item.h"
#include "cst_val.h"
#include "cst_val_const.h"
#include "cst_utterance.h"
#include "cst_features.h"
#include "cst_phoneset.h"
#include "cst_string.h"

cst_item *item_as(const cst_item *i, int r);
void *FUN_0002f474(const cst_item *item);
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
    return val_int_n(FUN_0005876c(item));
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
    ivona_lang_core *core = (ivona_lang_core *)FUN_0002f474(item);
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
            return val_int_n(c);
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
            return val_int_n(c);
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
        return val_int_n(c);
    }

    c = -1;
    do {
        s = item_prev(s);
        c++;
    } while (s != NULL);

    return val_int_n(c);
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

    return val_int_n(c);
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
const cst_val *FUN_00058fbc(const cst_item *seg, const char *ctype)
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
    return FUN_00058fbc(seg, "n");
}

/* 0x590c0: arm */
const cst_val *seg_coda_stop(const cst_item *seg)
{
    return FUN_00058fbc(seg, "s");
}

/* 0x590d0: arm */
const cst_val *seg_coda_fric(const cst_item *seg)
{
    return FUN_00058fbc(seg, "f");
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







