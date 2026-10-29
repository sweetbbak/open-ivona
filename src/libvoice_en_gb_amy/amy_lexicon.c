/*
 * Amy's lexicon object (0x3bb67c): no entries of its own (the main lexicon is lang.dat's), only
 * an addenda table of 81 words with their pronunciations (0x3bb6a8).
 */
#include <stddef.h>
#include "cst_lexicon.h"

#include "lex_table.h"

/* amy_addenda: generated from libvoice_en_gb_amy 0x3bb6a8 by tools/gen_lex_table.py */
static const char *const amy_addenda_phones[56] = {  /* 0x3bbde4 */
    "-",
    "ax",
    "n",
    "iy1",
    "s",
    "ey",
    "ow1",
    "l",
    "iy",
    "o",
    "ae1",
    "r",
    "ih",
    "m",
    "ah",
    "d",
    "eh1",
    "ae2",
    "p",
    "ch",
    "k",
    "b",
    "t",
    "ih1",
    "eh2",
    "z",
    "ow",
    "ay",
    "ay1",
    "aa",
    "g",
    "j",
    "eh",
    "uh1",
    "o1",
    "ae",
    "sh",
    "er",
    "f",
    "nx",
    "jh",
    "dh",
    "ey1",
    "ao",
    "aw",
    "hh",
    "ih2",
    "w",
    "aa1",
    "uax1",
    "uw1",
    "zh",
    "uw",
    "uh",
    "ao1",
    "v",
};

LEX_ENTRY(amy_addenda_0, "aeneas", '0', 1, 0, 2, 3, 0, 1, 4, 255);
LEX_ENTRY(amy_addenda_1, "aeolian", '0', 5, 0, 6, 0, 7, 8, 0, 1, 2, 255);
LEX_ENTRY(amy_addenda_2, "aeon", '0', 3, 0, 9, 2, 255);
LEX_ENTRY(amy_addenda_3, "ahriman", '0', 10, 0, 11, 12, 0, 13, 1, 2, 255);
LEX_ENTRY(amy_addenda_4, "aida", '0', 14, 0, 3, 0, 15, 1, 255);
LEX_ENTRY(amy_addenda_5, "allende", '0', 1, 0, 7, 16, 2, 0, 15, 5, 255);
LEX_ENTRY(amy_addenda_6, "apparatchiks", '0', 17, 0, 18, 1, 0, 11, 10, 0, 19, 12, 20, 4, 255);
LEX_ENTRY(amy_addenda_7, "beatrice", '0', 21, 3, 0, 1, 0, 22, 11, 8, 4, 255);
LEX_ENTRY(amy_addenda_8, "been", '0', 21, 23, 2, 255);
LEX_ENTRY(amy_addenda_9, "benzodiazepines", '0', 21, 24, 2, 0, 25, 26, 0, 15, 27, 0, 10, 0, 25, 1, 0, 18, 8, 2, 25, 255);
LEX_ENTRY(amy_addenda_10, "byzantine", '0', 21, 28, 25, 0, 1, 2, 0, 22, 27, 2, 255);
LEX_ENTRY(amy_addenda_11, "carnegie", '0', 20, 29, 0, 2, 3, 0, 30, 8, 255);
LEX_ENTRY(amy_addenda_12, "celtic", '0', 4, 16, 7, 22, 0, 12, 20, 255);
LEX_ENTRY(amy_addenda_13, "censure", '0', 4, 16, 2, 0, 4, 31, 1, 11, 255);
LEX_ENTRY(amy_addenda_14, "censured", '0', 4, 16, 2, 0, 4, 31, 1, 15, 255);
LEX_ENTRY(amy_addenda_15, "cezanne", '0', 4, 32, 0, 25, 10, 2, 255);
LEX_ENTRY(amy_addenda_16, "chinook", '0', 19, 12, 0, 2, 33, 20, 255);
LEX_ENTRY(amy_addenda_17, "comrade", '0', 20, 34, 13, 0, 11, 35, 15, 255);
LEX_ENTRY(amy_addenda_18, "comrades", '0', 20, 34, 13, 0, 11, 35, 15, 25, 255);
LEX_ENTRY(amy_addenda_19, "cynosure", '0', 4, 23, 0, 2, 1, 0, 36, 37, 255);
LEX_ENTRY(amy_addenda_20, "dante", '0', 15, 10, 2, 0, 22, 5, 255);
LEX_ENTRY(amy_addenda_21, "delphi", '0', 15, 16, 7, 0, 38, 8, 255);
LEX_ENTRY(amy_addenda_22, "des", '0', 15, 16, 25, 255);
LEX_ENTRY(amy_addenda_23, "dilemma", '0', 15, 27, 0, 7, 16, 0, 13, 1, 255);
LEX_ENTRY(amy_addenda_24, "direct", '0', 15, 27, 0, 11, 16, 20, 22, 255);
LEX_ENTRY(amy_addenda_25, "directed", '0', 15, 27, 0, 11, 16, 20, 22, 0, 12, 15, 255);
LEX_ENTRY(amy_addenda_26, "direction", '0', 15, 27, 0, 11, 16, 20, 0, 36, 39, 255);
LEX_ENTRY(amy_addenda_27, "directions", '0', 15, 27, 0, 11, 16, 20, 36, 0, 39, 25, 255);
LEX_ENTRY(amy_addenda_28, "directly", '0', 15, 27, 0, 11, 16, 20, 22, 0, 7, 8, 255);
LEX_ENTRY(amy_addenda_29, "directory", '0', 15, 1, 0, 11, 16, 20, 22, 0, 1, 0, 11, 8, 255);
LEX_ENTRY(amy_addenda_30, "ecological", '0', 24, 20, 0, 1, 0, 7, 34, 40, 0, 12, 20, 0, 1, 7, 255);
LEX_ENTRY(amy_addenda_31, "economic", '0', 24, 0, 20, 1, 0, 2, 34, 0, 13, 12, 20, 255);
LEX_ENTRY(amy_addenda_32, "economics", '0', 24, 0, 20, 1, 0, 2, 34, 0, 13, 12, 20, 4, 255);
LEX_ENTRY(amy_addenda_33, "economies", '0', 32, 0, 20, 34, 0, 2, 1, 0, 13, 8, 25, 255);
LEX_ENTRY(amy_addenda_34, "either", '0', 3, 0, 41, 1, 11, 255);
LEX_ENTRY(amy_addenda_35, "elan", '0', 32, 0, 7, 10, 2, 255);
LEX_ENTRY(amy_addenda_36, "ergo", '0', 37, 0, 30, 6, 255);
LEX_ENTRY(amy_addenda_37, "etcetera", '0', 24, 20, 0, 4, 16, 0, 22, 11, 1, 255);
LEX_ENTRY(amy_addenda_38, "fontaine", '0', 38, 9, 2, 0, 22, 42, 2, 255);
LEX_ENTRY(amy_addenda_39, "forbade", '0', 38, 43, 0, 21, 42, 15, 255);
LEX_ENTRY(amy_addenda_40, "garcia", '0', 30, 29, 0, 4, 3, 0, 1, 255);
LEX_ENTRY(amy_addenda_41, "glaucoma", '0', 30, 7, 44, 0, 20, 6, 0, 13, 1, 255);
LEX_ENTRY(amy_addenda_42, "hegelian", '0', 45, 32, 0, 30, 16, 0, 7, 8, 0, 1, 2, 255);
LEX_ENTRY(amy_addenda_43, "indigestible", '0', 46, 2, 0, 15, 12, 0, 40, 16, 4, 22, 0, 1, 0, 21, 1, 7, 255);
LEX_ENTRY(amy_addenda_44, "indigestion", '0', 46, 2, 0, 15, 12, 0, 40, 16, 4, 0, 19, 1, 2, 255);
LEX_ENTRY(amy_addenda_45, "juanita", '0', 45, 47, 29, 0, 2, 3, 22, 0, 1, 255);
LEX_ENTRY(amy_addenda_46, "junta", '0', 40, 33, 2, 0, 22, 1, 255);
LEX_ENTRY(amy_addenda_47, "leonidas", '0', 7, 8, 0, 1, 0, 2, 3, 0, 15, 1, 4, 255);
LEX_ENTRY(amy_addenda_48, "maelstrom", '0', 13, 42, 7, 4, 0, 22, 11, 1, 13, 255);
LEX_ENTRY(amy_addenda_49, "mama", '0', 13, 1, 0, 13, 48, 255);
LEX_ENTRY(amy_addenda_50, "maraschino", '0', 13, 17, 0, 11, 1, 0, 36, 3, 0, 2, 26, 255);
LEX_ENTRY(amy_addenda_51, "mercurial", '0', 13, 1, 20, 0, 31, 49, 0, 11, 8, 0, 1, 7, 255);
LEX_ENTRY(amy_addenda_52, "minutiae", '0', 13, 27, 2, 0, 31, 50, 36, 0, 27, 255);
LEX_ENTRY(amy_addenda_53, "mirage", '0', 13, 23, 0, 11, 29, 51, 255);
LEX_ENTRY(amy_addenda_54, "naomi", '0', 2, 42, 0, 26, 0, 13, 8, 255);
LEX_ENTRY(amy_addenda_55, "neither", '0', 2, 3, 0, 41, 1, 11, 255);
LEX_ENTRY(amy_addenda_56, "nereid", '0', 2, 16, 0, 11, 8, 15, 255);
LEX_ENTRY(amy_addenda_57, "osama", '0', 26, 0, 4, 10, 0, 13, 1, 255);
LEX_ENTRY(amy_addenda_58, "papa", '0', 18, 1, 0, 18, 48, 255);
LEX_ENTRY(amy_addenda_59, "perez", '0', 18, 16, 0, 11, 32, 4, 255);
LEX_ENTRY(amy_addenda_60, "pilaf", '0', 18, 3, 0, 7, 35, 38, 255);
LEX_ENTRY(amy_addenda_61, "raphael", '0', 11, 10, 0, 38, 27, 0, 32, 7, 255);
LEX_ENTRY(amy_addenda_62, "reinhold", '0', 11, 28, 2, 0, 45, 26, 7, 15, 255);
LEX_ENTRY(amy_addenda_63, "reschedule", '0', 11, 8, 4, 20, 0, 16, 0, 40, 1, 7, 255);
LEX_ENTRY(amy_addenda_64, "rheostat", '0', 11, 3, 0, 26, 0, 4, 22, 35, 22, 255);
LEX_ENTRY(amy_addenda_65, "rhodesia", '0', 11, 26, 0, 15, 3, 0, 51, 1, 255);
LEX_ENTRY(amy_addenda_66, "rubaiyat", '0', 11, 52, 0, 21, 29, 0, 31, 10, 22, 255);
LEX_ENTRY(amy_addenda_67, "samurai", '0', 4, 10, 0, 13, 31, 53, 0, 11, 27, 255);
LEX_ENTRY(amy_addenda_68, "scalar", '0', 4, 20, 42, 7, 0, 29, 11, 255);
LEX_ENTRY(amy_addenda_69, "schedule", '0', 4, 20, 16, 0, 40, 1, 7, 255);
LEX_ENTRY(amy_addenda_70, "scheduled", '0', 4, 20, 16, 0, 40, 1, 7, 15, 255);
LEX_ENTRY(amy_addenda_71, "schedules", '0', 4, 20, 16, 0, 40, 1, 7, 25, 255);
LEX_ENTRY(amy_addenda_72, "senor", '0', 4, 32, 0, 2, 31, 54, 255);
LEX_ENTRY(amy_addenda_73, "sequoia", '0', 4, 12, 0, 20, 34, 0, 31, 1, 255);
LEX_ENTRY(amy_addenda_74, "signor", '0', 4, 8, 0, 2, 31, 54, 255);
LEX_ENTRY(amy_addenda_75, "tsunamis", '0', 4, 53, 0, 2, 48, 0, 13, 8, 25, 255);
LEX_ENTRY(amy_addenda_76, "valenzuela", '0', 55, 35, 0, 7, 1, 2, 0, 25, 47, 16, 0, 7, 1, 255);
LEX_ENTRY(amy_addenda_77, "vanadium", '0', 55, 35, 0, 2, 10, 0, 15, 8, 0, 1, 13, 255);
LEX_ENTRY(amy_addenda_78, "verdi", '0', 55, 16, 11, 0, 15, 8, 255);
LEX_ENTRY(amy_addenda_79, "wuhan", '0', 47, 50, 0, 45, 35, 2, 255);
LEX_ENTRY(amy_addenda_80, "zelig", '0', 25, 3, 7, 0, 12, 30, 255);

static const lex_entry *const amy_addenda_entries[81] = {  /* 0x3bb6b4 */
    (const lex_entry *)&amy_addenda_0,
    (const lex_entry *)&amy_addenda_1,
    (const lex_entry *)&amy_addenda_2,
    (const lex_entry *)&amy_addenda_3,
    (const lex_entry *)&amy_addenda_4,
    (const lex_entry *)&amy_addenda_5,
    (const lex_entry *)&amy_addenda_6,
    (const lex_entry *)&amy_addenda_7,
    (const lex_entry *)&amy_addenda_8,
    (const lex_entry *)&amy_addenda_9,
    (const lex_entry *)&amy_addenda_10,
    (const lex_entry *)&amy_addenda_11,
    (const lex_entry *)&amy_addenda_12,
    (const lex_entry *)&amy_addenda_13,
    (const lex_entry *)&amy_addenda_14,
    (const lex_entry *)&amy_addenda_15,
    (const lex_entry *)&amy_addenda_16,
    (const lex_entry *)&amy_addenda_17,
    (const lex_entry *)&amy_addenda_18,
    (const lex_entry *)&amy_addenda_19,
    (const lex_entry *)&amy_addenda_20,
    (const lex_entry *)&amy_addenda_21,
    (const lex_entry *)&amy_addenda_22,
    (const lex_entry *)&amy_addenda_23,
    (const lex_entry *)&amy_addenda_24,
    (const lex_entry *)&amy_addenda_25,
    (const lex_entry *)&amy_addenda_26,
    (const lex_entry *)&amy_addenda_27,
    (const lex_entry *)&amy_addenda_28,
    (const lex_entry *)&amy_addenda_29,
    (const lex_entry *)&amy_addenda_30,
    (const lex_entry *)&amy_addenda_31,
    (const lex_entry *)&amy_addenda_32,
    (const lex_entry *)&amy_addenda_33,
    (const lex_entry *)&amy_addenda_34,
    (const lex_entry *)&amy_addenda_35,
    (const lex_entry *)&amy_addenda_36,
    (const lex_entry *)&amy_addenda_37,
    (const lex_entry *)&amy_addenda_38,
    (const lex_entry *)&amy_addenda_39,
    (const lex_entry *)&amy_addenda_40,
    (const lex_entry *)&amy_addenda_41,
    (const lex_entry *)&amy_addenda_42,
    (const lex_entry *)&amy_addenda_43,
    (const lex_entry *)&amy_addenda_44,
    (const lex_entry *)&amy_addenda_45,
    (const lex_entry *)&amy_addenda_46,
    (const lex_entry *)&amy_addenda_47,
    (const lex_entry *)&amy_addenda_48,
    (const lex_entry *)&amy_addenda_49,
    (const lex_entry *)&amy_addenda_50,
    (const lex_entry *)&amy_addenda_51,
    (const lex_entry *)&amy_addenda_52,
    (const lex_entry *)&amy_addenda_53,
    (const lex_entry *)&amy_addenda_54,
    (const lex_entry *)&amy_addenda_55,
    (const lex_entry *)&amy_addenda_56,
    (const lex_entry *)&amy_addenda_57,
    (const lex_entry *)&amy_addenda_58,
    (const lex_entry *)&amy_addenda_59,
    (const lex_entry *)&amy_addenda_60,
    (const lex_entry *)&amy_addenda_61,
    (const lex_entry *)&amy_addenda_62,
    (const lex_entry *)&amy_addenda_63,
    (const lex_entry *)&amy_addenda_64,
    (const lex_entry *)&amy_addenda_65,
    (const lex_entry *)&amy_addenda_66,
    (const lex_entry *)&amy_addenda_67,
    (const lex_entry *)&amy_addenda_68,
    (const lex_entry *)&amy_addenda_69,
    (const lex_entry *)&amy_addenda_70,
    (const lex_entry *)&amy_addenda_71,
    (const lex_entry *)&amy_addenda_72,
    (const lex_entry *)&amy_addenda_73,
    (const lex_entry *)&amy_addenda_74,
    (const lex_entry *)&amy_addenda_75,
    (const lex_entry *)&amy_addenda_76,
    (const lex_entry *)&amy_addenda_77,
    (const lex_entry *)&amy_addenda_78,
    (const lex_entry *)&amy_addenda_79,
    (const lex_entry *)&amy_addenda_80,
};

static const lex_table amy_addenda = { 81, amy_addenda_entries, amy_addenda_phones };  /* 0x3bb6a8 */

/* 0x3bb67c */
const cst_lexicon amy_lexicon = {
    (char *)"vox_en_gb_amy",
    0, NULL, 0, NULL, NULL,
    (void *)&amy_addenda,
    NULL, NULL, NULL, NULL,
};
