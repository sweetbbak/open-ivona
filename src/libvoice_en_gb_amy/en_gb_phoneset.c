/* The gb phoneset (Flite-style, generated: tools/gen_phoneset.py). */
#include "cst_phoneset.h"
#include "cst_val_const.h"

/* en_gb_phoneset: generated from libvoice_en_gb_amy 0x3c0be0 by tools/gen_phoneset.py */
static const char *const en_gb_phoneset_featnames[] = {  /* 0x3c0e78 */
    "vc", "cvox", "vlng", "vheight", "vfront", "vrnd", "ctype", "cplace", NULL
};

static const char *const en_gb_phoneset_phonenames[] = {  /* 0x3c0db0 */
    "aa", "ae", "ah", "ao", "aw", "ax", "ay", "b", "brth", "ch",
    "d", "dh", "eax", "eh", "er", "ey", "f", "g", "hh", "iax",
    "ih", "iy", "j", "jh", "k", "l", "lx", "m", "mx", "n",
    "ng", "nx", "o", "ow", "oy", "p", "pau", "r", "s", "sh",
    "t", "th", "uax", "uh", "uw", "v", "w", "z", "zh",
    NULL
};

static const int en_gb_phoneset_fv_000[] = { 0, 1, 2, 3, 4, 5, 1, 1, -1 };  /* aa */
static const int en_gb_phoneset_fv_001[] = { 0, 1, 6, 3, 7, 5, 1, 1, -1 };  /* ae */
static const int en_gb_phoneset_fv_002[] = { 0, 1, 6, 8, 9, 5, 1, 1, -1 };  /* ah */
static const int en_gb_phoneset_fv_003[] = { 0, 1, 2, 4, 4, 0, 1, 1, -1 };  /* ao */
static const int en_gb_phoneset_fv_004[] = { 0, 1, 10, 3, 4, 0, 1, 1, -1 };  /* aw */
static const int en_gb_phoneset_fv_005[] = { 0, 1, 11, 4, 9, 5, 1, 1, -1 };  /* ax */
static const int en_gb_phoneset_fv_006[] = { 0, 1, 10, 3, 7, 5, 1, 1, -1 };  /* ay */
static const int en_gb_phoneset_fv_007[] = { 5, 0, 1, 1, 1, 1, 12, 2, -1 };  /* b */
static const int en_gb_phoneset_fv_008[] = { 1, 1, 1, 1, 1, 1, 1, 1, -1 };  /* brth */
static const int en_gb_phoneset_fv_009[] = { 5, 5, 1, 1, 1, 1, 11, 6, -1 };  /* ch */
static const int en_gb_phoneset_fv_010[] = { 5, 0, 1, 1, 1, 1, 12, 11, -1 };  /* d */
static const int en_gb_phoneset_fv_011[] = { 5, 0, 1, 1, 1, 1, 13, 10, -1 };  /* dh */
static const int en_gb_phoneset_fv_012[] = { 0, 1, 10, 8, 7, 5, 1, 1, -1 };  /* eax */
static const int en_gb_phoneset_fv_013[] = { 0, 1, 6, 4, 7, 5, 1, 1, -1 };  /* eh */
static const int en_gb_phoneset_fv_014[] = { 0, 1, 2, 4, 9, 5, 1, 1, -1 };  /* er */
static const int en_gb_phoneset_fv_015[] = { 0, 1, 10, 4, 7, 5, 1, 1, -1 };  /* ey */
static const int en_gb_phoneset_fv_016[] = { 5, 5, 1, 1, 1, 1, 13, 14, -1 };  /* f */
static const int en_gb_phoneset_fv_017[] = { 5, 0, 1, 1, 1, 1, 12, 15, -1 };  /* g */
static const int en_gb_phoneset_fv_018[] = { 5, 5, 1, 1, 1, 1, 13, 16, -1 };  /* hh */
static const int en_gb_phoneset_fv_019[] = { 0, 1, 10, 9, 7, 5, 1, 1, -1 };  /* iax */
static const int en_gb_phoneset_fv_020[] = { 0, 1, 6, 9, 7, 5, 1, 1, -1 };  /* ih */
static const int en_gb_phoneset_fv_021[] = { 0, 1, 2, 7, 7, 5, 1, 1, -1 };  /* iy */
static const int en_gb_phoneset_fv_022[] = { 5, 0, 1, 1, 1, 1, 16, 12, -1 };  /* j */
static const int en_gb_phoneset_fv_023[] = { 5, 0, 1, 1, 1, 1, 11, 6, -1 };  /* jh */
static const int en_gb_phoneset_fv_024[] = { 5, 5, 1, 1, 1, 1, 12, 15, -1 };  /* k */
static const int en_gb_phoneset_fv_025[] = { 5, 0, 1, 1, 1, 1, 2, 11, -1 };  /* l */
static const int en_gb_phoneset_fv_026[] = { 5, 0, 11, 1, 1, 1, 2, 11, -1 };  /* lx */
static const int en_gb_phoneset_fv_027[] = { 5, 0, 1, 1, 1, 1, 17, 2, -1 };  /* m */
static const int en_gb_phoneset_fv_028[] = { 5, 0, 11, 1, 1, 1, 17, 2, -1 };  /* mx */
static const int en_gb_phoneset_fv_029[] = { 5, 0, 1, 1, 1, 1, 17, 11, -1 };  /* n */
static const int en_gb_phoneset_fv_030[] = { 5, 0, 1, 1, 1, 1, 17, 15, -1 };  /* ng */
static const int en_gb_phoneset_fv_031[] = { 5, 0, 11, 1, 1, 1, 17, 11, -1 };  /* nx */
static const int en_gb_phoneset_fv_032[] = { 0, 1, 6, 3, 4, 0, 1, 1, -1 };  /* o */
static const int en_gb_phoneset_fv_033[] = { 0, 1, 10, 4, 9, 0, 1, 1, -1 };  /* ow */
static const int en_gb_phoneset_fv_034[] = { 0, 1, 10, 4, 4, 0, 1, 1, -1 };  /* oy */
static const int en_gb_phoneset_fv_035[] = { 5, 5, 1, 1, 1, 1, 12, 2, -1 };  /* p */
static const int en_gb_phoneset_fv_036[] = { 1, 1, 1, 1, 1, 1, 1, 1, -1 };  /* pau */
static const int en_gb_phoneset_fv_037[] = { 5, 0, 1, 1, 1, 1, 18, 11, -1 };  /* r */
static const int en_gb_phoneset_fv_038[] = { 5, 5, 1, 1, 1, 1, 13, 11, -1 };  /* s */
static const int en_gb_phoneset_fv_039[] = { 5, 5, 1, 1, 1, 1, 13, 6, -1 };  /* sh */
static const int en_gb_phoneset_fv_040[] = { 5, 5, 1, 1, 1, 1, 12, 11, -1 };  /* t */
static const int en_gb_phoneset_fv_041[] = { 5, 5, 1, 1, 1, 1, 13, 10, -1 };  /* th */
static const int en_gb_phoneset_fv_042[] = { 0, 1, 10, 9, 4, 0, 1, 1, -1 };  /* uax */
static const int en_gb_phoneset_fv_043[] = { 0, 1, 6, 9, 4, 0, 1, 1, -1 };  /* uh */
static const int en_gb_phoneset_fv_044[] = { 0, 1, 2, 7, 4, 0, 1, 1, -1 };  /* uw */
static const int en_gb_phoneset_fv_045[] = { 5, 0, 1, 1, 1, 1, 13, 14, -1 };  /* v */
static const int en_gb_phoneset_fv_046[] = { 5, 0, 1, 1, 1, 1, 16, 15, -1 };  /* w */
static const int en_gb_phoneset_fv_047[] = { 5, 0, 1, 1, 1, 1, 13, 11, -1 };  /* z */
static const int en_gb_phoneset_fv_048[] = { 5, 0, 1, 1, 1, 1, 13, 6, -1 };  /* zh */
static const int en_gb_phoneset_fv_end[] = { 0 };

static const int *const en_gb_phoneset_fvtable[] = {  /* 0x3c0ce8 */
    en_gb_phoneset_fv_000, en_gb_phoneset_fv_001, en_gb_phoneset_fv_002, en_gb_phoneset_fv_003, en_gb_phoneset_fv_004, en_gb_phoneset_fv_005,
    en_gb_phoneset_fv_006, en_gb_phoneset_fv_007, en_gb_phoneset_fv_008, en_gb_phoneset_fv_009, en_gb_phoneset_fv_010, en_gb_phoneset_fv_011,
    en_gb_phoneset_fv_012, en_gb_phoneset_fv_013, en_gb_phoneset_fv_014, en_gb_phoneset_fv_015, en_gb_phoneset_fv_016, en_gb_phoneset_fv_017,
    en_gb_phoneset_fv_018, en_gb_phoneset_fv_019, en_gb_phoneset_fv_020, en_gb_phoneset_fv_021, en_gb_phoneset_fv_022, en_gb_phoneset_fv_023,
    en_gb_phoneset_fv_024, en_gb_phoneset_fv_025, en_gb_phoneset_fv_026, en_gb_phoneset_fv_027, en_gb_phoneset_fv_028, en_gb_phoneset_fv_029,
    en_gb_phoneset_fv_030, en_gb_phoneset_fv_031, en_gb_phoneset_fv_032, en_gb_phoneset_fv_033, en_gb_phoneset_fv_034, en_gb_phoneset_fv_035,
    en_gb_phoneset_fv_036, en_gb_phoneset_fv_037, en_gb_phoneset_fv_038, en_gb_phoneset_fv_039, en_gb_phoneset_fv_040, en_gb_phoneset_fv_041,
    en_gb_phoneset_fv_042, en_gb_phoneset_fv_043, en_gb_phoneset_fv_044, en_gb_phoneset_fv_045, en_gb_phoneset_fv_046, en_gb_phoneset_fv_047,
    en_gb_phoneset_fv_048,
    en_gb_phoneset_fv_end
};

DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_0, "+");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_1, "0");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_2, "l");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_3, "5");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_4, "3");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_5, "-");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_6, "s");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_7, "1");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_8, "4");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_9, "2");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_10, "d");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_11, "a");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_12, "p");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_13, "f");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_14, "b");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_15, "v");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_16, "g");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_17, "n");
DEF_STATIC_CONST_VAL_STRING(en_gb_phoneset_fval_18, "x");

static const cst_val *const en_gb_phoneset_featvals[] = {  /* 0x3c0c00 */
    &en_gb_phoneset_fval_0, &en_gb_phoneset_fval_1, &en_gb_phoneset_fval_2, &en_gb_phoneset_fval_3, &en_gb_phoneset_fval_4, &en_gb_phoneset_fval_5,
    &en_gb_phoneset_fval_6, &en_gb_phoneset_fval_7, &en_gb_phoneset_fval_8, &en_gb_phoneset_fval_9, &en_gb_phoneset_fval_10, &en_gb_phoneset_fval_11,
    &en_gb_phoneset_fval_12, &en_gb_phoneset_fval_13, &en_gb_phoneset_fval_14, &en_gb_phoneset_fval_15, &en_gb_phoneset_fval_16, &en_gb_phoneset_fval_17,
    &en_gb_phoneset_fval_18,
    NULL
};

/* 0x3c0be0 */
const cst_phoneset en_gb_phoneset = {
    "x-ivona-en_gb", en_gb_phoneset_featnames, en_gb_phoneset_featvals, en_gb_phoneset_phonenames,
    "pau", 49, en_gb_phoneset_fvtable, 1
};
