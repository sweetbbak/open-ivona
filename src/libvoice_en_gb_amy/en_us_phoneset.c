/* The us phoneset (Flite-style, generated: tools/gen_phoneset.py). */
#include "cst_phoneset.h"
#include "cst_val_const.h"

/* en_us_phoneset: generated from libvoice_en_gb_amy 0x3cd0b4 by tools/gen_phoneset.py */
static const char *const en_us_phoneset_featnames[] = {  /* 0x3cd32c */
    "vc", "cvox", "vlng", "vheight", "vfront", "vrnd", "ctype", "cplace", NULL
};

static const char *const en_us_phoneset_phonenames[] = {  /* 0x3cd274 */
    "aa", "ae", "ah", "ao", "aw", "ax", "ay", "b", "brth", "ch",
    "d", "dh", "eh", "er", "ey", "f", "g", "hh", "ih", "iy",
    "j", "jh", "k", "l", "lx", "m", "mx", "n", "ng", "nx",
    "ow", "oy", "p", "pau", "r", "s", "sh", "t", "th", "uh",
    "uw", "v", "w", "z", "zh",
    NULL
};

static const int en_us_phoneset_fv_000[] = { 0, 1, 2, 3, 4, 5, 1, 1, -1 };  /* aa */
static const int en_us_phoneset_fv_001[] = { 0, 1, 6, 3, 7, 5, 1, 1, -1 };  /* ae */
static const int en_us_phoneset_fv_002[] = { 0, 1, 6, 8, 9, 5, 1, 1, -1 };  /* ah */
static const int en_us_phoneset_fv_003[] = { 0, 1, 2, 8, 4, 0, 1, 1, -1 };  /* ao */
static const int en_us_phoneset_fv_004[] = { 0, 1, 10, 3, 4, 0, 1, 1, -1 };  /* aw */
static const int en_us_phoneset_fv_005[] = { 0, 1, 11, 4, 9, 5, 1, 1, -1 };  /* ax */
static const int en_us_phoneset_fv_006[] = { 0, 1, 10, 3, 7, 5, 1, 1, -1 };  /* ay */
static const int en_us_phoneset_fv_007[] = { 5, 0, 1, 1, 1, 1, 12, 2, -1 };  /* b */
static const int en_us_phoneset_fv_008[] = { 1, 1, 1, 1, 1, 1, 1, 1, -1 };  /* brth */
static const int en_us_phoneset_fv_009[] = { 5, 5, 1, 1, 1, 1, 11, 6, -1 };  /* ch */
static const int en_us_phoneset_fv_010[] = { 5, 0, 1, 1, 1, 1, 12, 11, -1 };  /* d */
static const int en_us_phoneset_fv_011[] = { 5, 0, 1, 1, 1, 1, 13, 10, -1 };  /* dh */
static const int en_us_phoneset_fv_012[] = { 0, 1, 6, 8, 7, 5, 1, 1, -1 };  /* eh */
static const int en_us_phoneset_fv_013[] = { 0, 1, 2, 4, 9, 5, 1, 1, -1 };  /* er */
static const int en_us_phoneset_fv_014[] = { 0, 1, 10, 4, 7, 5, 1, 1, -1 };  /* ey */
static const int en_us_phoneset_fv_015[] = { 5, 5, 1, 1, 1, 1, 13, 14, -1 };  /* f */
static const int en_us_phoneset_fv_016[] = { 5, 0, 1, 1, 1, 1, 12, 15, -1 };  /* g */
static const int en_us_phoneset_fv_017[] = { 5, 5, 1, 1, 1, 1, 13, 16, -1 };  /* hh */
static const int en_us_phoneset_fv_018[] = { 0, 1, 6, 9, 7, 5, 1, 1, -1 };  /* ih */
static const int en_us_phoneset_fv_019[] = { 0, 1, 2, 7, 7, 5, 1, 1, -1 };  /* iy */
static const int en_us_phoneset_fv_020[] = { 5, 0, 1, 1, 1, 1, 16, 12, -1 };  /* j */
static const int en_us_phoneset_fv_021[] = { 5, 0, 1, 1, 1, 1, 11, 6, -1 };  /* jh */
static const int en_us_phoneset_fv_022[] = { 5, 5, 1, 1, 1, 1, 12, 15, -1 };  /* k */
static const int en_us_phoneset_fv_023[] = { 5, 0, 1, 1, 1, 1, 2, 11, -1 };  /* l */
static const int en_us_phoneset_fv_024[] = { 5, 0, 11, 1, 1, 1, 2, 11, -1 };  /* lx */
static const int en_us_phoneset_fv_025[] = { 5, 0, 1, 1, 1, 1, 17, 2, -1 };  /* m */
static const int en_us_phoneset_fv_026[] = { 5, 0, 11, 1, 1, 1, 17, 2, -1 };  /* mx */
static const int en_us_phoneset_fv_027[] = { 5, 0, 1, 1, 1, 1, 17, 11, -1 };  /* n */
static const int en_us_phoneset_fv_028[] = { 5, 0, 1, 1, 1, 1, 17, 15, -1 };  /* ng */
static const int en_us_phoneset_fv_029[] = { 5, 0, 11, 1, 1, 1, 17, 11, -1 };  /* nx */
static const int en_us_phoneset_fv_030[] = { 0, 1, 10, 4, 4, 0, 1, 1, -1 };  /* ow */
static const int en_us_phoneset_fv_031[] = { 0, 1, 10, 4, 7, 0, 1, 1, -1 };  /* oy */
static const int en_us_phoneset_fv_032[] = { 5, 5, 1, 1, 1, 1, 12, 2, -1 };  /* p */
static const int en_us_phoneset_fv_033[] = { 1, 1, 1, 1, 1, 1, 1, 1, -1 };  /* pau */
static const int en_us_phoneset_fv_034[] = { 5, 0, 1, 1, 1, 1, 18, 11, -1 };  /* r */
static const int en_us_phoneset_fv_035[] = { 5, 5, 1, 1, 1, 1, 13, 11, -1 };  /* s */
static const int en_us_phoneset_fv_036[] = { 5, 5, 1, 1, 1, 1, 13, 6, -1 };  /* sh */
static const int en_us_phoneset_fv_037[] = { 5, 5, 1, 1, 1, 1, 12, 11, -1 };  /* t */
static const int en_us_phoneset_fv_038[] = { 5, 5, 1, 1, 1, 1, 13, 10, -1 };  /* th */
static const int en_us_phoneset_fv_039[] = { 0, 1, 6, 9, 4, 0, 1, 1, -1 };  /* uh */
static const int en_us_phoneset_fv_040[] = { 0, 1, 2, 7, 4, 0, 1, 1, -1 };  /* uw */
static const int en_us_phoneset_fv_041[] = { 5, 0, 1, 1, 1, 1, 13, 14, -1 };  /* v */
static const int en_us_phoneset_fv_042[] = { 5, 0, 1, 1, 1, 1, 16, 2, -1 };  /* w */
static const int en_us_phoneset_fv_043[] = { 5, 0, 1, 1, 1, 1, 13, 11, -1 };  /* z */
static const int en_us_phoneset_fv_044[] = { 5, 0, 1, 1, 1, 1, 13, 6, -1 };  /* zh */
static const int en_us_phoneset_fv_end[] = { 0 };

static const int *const en_us_phoneset_fvtable[] = {  /* 0x3cd1bc */
    en_us_phoneset_fv_000, en_us_phoneset_fv_001, en_us_phoneset_fv_002, en_us_phoneset_fv_003, en_us_phoneset_fv_004, en_us_phoneset_fv_005,
    en_us_phoneset_fv_006, en_us_phoneset_fv_007, en_us_phoneset_fv_008, en_us_phoneset_fv_009, en_us_phoneset_fv_010, en_us_phoneset_fv_011,
    en_us_phoneset_fv_012, en_us_phoneset_fv_013, en_us_phoneset_fv_014, en_us_phoneset_fv_015, en_us_phoneset_fv_016, en_us_phoneset_fv_017,
    en_us_phoneset_fv_018, en_us_phoneset_fv_019, en_us_phoneset_fv_020, en_us_phoneset_fv_021, en_us_phoneset_fv_022, en_us_phoneset_fv_023,
    en_us_phoneset_fv_024, en_us_phoneset_fv_025, en_us_phoneset_fv_026, en_us_phoneset_fv_027, en_us_phoneset_fv_028, en_us_phoneset_fv_029,
    en_us_phoneset_fv_030, en_us_phoneset_fv_031, en_us_phoneset_fv_032, en_us_phoneset_fv_033, en_us_phoneset_fv_034, en_us_phoneset_fv_035,
    en_us_phoneset_fv_036, en_us_phoneset_fv_037, en_us_phoneset_fv_038, en_us_phoneset_fv_039, en_us_phoneset_fv_040, en_us_phoneset_fv_041,
    en_us_phoneset_fv_042, en_us_phoneset_fv_043, en_us_phoneset_fv_044,
    en_us_phoneset_fv_end
};

DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_0, "+");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_1, "0");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_2, "l");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_3, "5");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_4, "3");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_5, "-");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_6, "s");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_7, "1");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_8, "4");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_9, "2");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_10, "d");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_11, "a");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_12, "p");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_13, "f");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_14, "b");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_15, "v");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_16, "g");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_17, "n");
DEF_STATIC_CONST_VAL_STRING(en_us_phoneset_fval_18, "x");

static const cst_val *const en_us_phoneset_featvals[] = {  /* 0x3cd0d4 */
    &en_us_phoneset_fval_0, &en_us_phoneset_fval_1, &en_us_phoneset_fval_2, &en_us_phoneset_fval_3, &en_us_phoneset_fval_4, &en_us_phoneset_fval_5,
    &en_us_phoneset_fval_6, &en_us_phoneset_fval_7, &en_us_phoneset_fval_8, &en_us_phoneset_fval_9, &en_us_phoneset_fval_10, &en_us_phoneset_fval_11,
    &en_us_phoneset_fval_12, &en_us_phoneset_fval_13, &en_us_phoneset_fval_14, &en_us_phoneset_fval_15, &en_us_phoneset_fval_16, &en_us_phoneset_fval_17,
    &en_us_phoneset_fval_18,
    NULL
};

/* 0x3cd0b4 */
const cst_phoneset en_us_phoneset = {
    "x-ivona-en_us", en_us_phoneset_featnames, en_us_phoneset_featvals, en_us_phoneset_phonenames,
    "pau", 45, en_us_phoneset_fvtable, 1
};
