/*
 * English X-SAMPA input (0x4b40c..): pronunciations given in X-SAMPA (or Amazon's x-amazon-en-gb
 * and -en-us alphabets) converted to IVONA phones, with syllable boundaries.
 */
#include <string.h>
#include "cst_val.h"
#include "str_rewrite.h"

const char *phone_name(const cst_val *p);
char *xsampa_word(const char *word, const char *lang, const void *a, const void *b);
cst_val *en_gb_syllabify(cst_val *phones);
cst_val *en_us_syllabify(cst_val *phones);

/* as in cst_synth.c */
typedef struct stress_info {
    int stress;                 /* 0 ends the table */
    int mark;                   /* 1: primary ("), 2: secondary (%) */
    unsigned int tone;
    int unused;
} stress_info;

/* how one alphabet converts (xsampa_word_convert) */
typedef struct xsampa_ctx {
    const str_rewrite_rule *rules;      /* X-SAMPA to IVONA phones */
    const char *const *modifiers;       /* modifiers kept */
    const char *const *vowels;
    const char *const *syllabics;
    const stress_info *stress;          /* set on first use */
} xsampa_ctx;

extern const str_rewrite_rule en_gb_xsampa_rules[], en_us_xsampa_rules[];
extern const str_rewrite_rule en_gb_amazon_xsampa_rules[], en_us_amazon_xsampa_rules[];
extern const char *const en_gb_modifiers[];
extern const char *const en_gb_vowels[];
extern const char *const en_gb_syllabics[];
extern const char *const en_us_modifiers[];
extern const char *const en_us_vowels[];
extern const char *const en_us_syllabics[];
extern const char *const en_gb_amazon_modifiers[];
extern const char *const en_gb_amazon_vowels[];
extern const char *const en_gb_amazon_syllabics[];
extern const char *const en_us_amazon_modifiers[];
extern const char *const en_us_amazon_vowels[];
extern const char *const en_us_amazon_syllabics[];

/* vowels that cannot end a syllable */
static const char *const en_checked_vowels[] = { "ih", "eh", "ae", "o", "ah", "uh", NULL };

/* whether name is a checked vowel, optionally with a stress digit */
static inline int is_checked_vowel(const char *name)
{
    const char *const *v;
    size_t n, m;

    for (v = en_checked_vowels; *v; v++) {
        if (cst_startswith(name, *v)) {
            n = strlen(name);
            m = strlen(*v);
            if (n == m || (n == m + 1 && (unsigned char)(name[n - 1] - '0') <= 9))
                return 1;
        }
    }
    return 0;
}

/* 0x4b40c arm: moves a syllable boundary that follows a checked vowel (optionally with a stress
 * digit) past the consonant after it: "ih - t" becomes "ih t -" */
cst_val *en_fix_checked_vowels(cst_val *phones)
{
    const cst_val *p, *b, *c, *t;

    for (p = phones; p; p = val_cdr(p)) {
        if (val_cdr(p) == NULL || val_cdr(val_cdr(p)) == NULL)
            break;
        b = val_cdr(p);
        c = val_cdr(b);
        if (strcmp(phone_name(b), "-") == 0 && is_checked_vowel(phone_name(p))
            && strchr("aeiou", (unsigned char)phone_name(c)[0]) == NULL) {
            t = val_car(b);
            set_car((cst_val *)b, val_car(c));
            set_car((cst_val *)c, t);
        }
    }
    return phones;
}

static stress_info en_gb_stress[] = { { 1, 1, 0, 0 }, { 2, 2, 0, 0 }, { 0, 0, 0, 0 } };
static stress_info en_us_stress[] = { { 1, 1, 0, 0 }, { 2, 2, 0, 0 }, { 0, 0, 0, 0 } };
static stress_info en_gb_amazon_stress[] = { { 1, 1, 0, 0 }, { 2, 2, 0, 0 }, { 0, 0, 0, 0 } };
static stress_info en_us_amazon_stress[] = { { 1, 1, 0, 0 }, { 2, 2, 0, 0 }, { 0, 0, 0, 0 } };

static xsampa_ctx en_gb_xsampa = {
    en_gb_xsampa_rules, en_gb_modifiers, en_gb_vowels, en_gb_syllabics, NULL
};
static xsampa_ctx en_us_xsampa = {
    en_us_xsampa_rules, en_us_modifiers, en_us_vowels, en_us_syllabics, NULL
};
static xsampa_ctx en_gb_amazon_xsampa = {
    en_gb_amazon_xsampa_rules, en_gb_amazon_modifiers, en_gb_amazon_vowels,
    en_gb_amazon_syllabics, NULL
};
static xsampa_ctx en_us_amazon_xsampa = {
    en_us_amazon_xsampa_rules, en_us_amazon_modifiers, en_us_amazon_vowels,
    en_us_amazon_syllabics, NULL
};

/* 0x4b598 arm */
stress_info *en_gb_stress_info(void)
{
    return en_gb_stress;
}

/* 0x4b5a8 arm */
xsampa_ctx *en_gb_xsampa_ctx(void)
{
    if (en_gb_xsampa.stress == NULL)
        en_gb_xsampa.stress = en_gb_stress_info();
    return &en_gb_xsampa;
}

/* 0x4b5e0 arm */
stress_info *en_us_stress_info(void)
{
    return en_us_stress;
}

/* 0x4b5f4 arm */
xsampa_ctx *en_us_xsampa_ctx(void)
{
    if (en_us_xsampa.stress == NULL)
        en_us_xsampa.stress = en_us_stress_info();
    return &en_us_xsampa;
}

/* 0x4b630 arm */
stress_info *en_gb_amazon_stress_info(void)
{
    return en_gb_amazon_stress;
}

/* 0x4b644 arm */
xsampa_ctx *en_gb_amazon_xsampa_ctx(void)
{
    if (en_gb_amazon_xsampa.stress == NULL)
        en_gb_amazon_xsampa.stress = en_gb_amazon_stress_info();
    return &en_gb_amazon_xsampa;
}

/* 0x4b680 arm */
stress_info *en_us_amazon_stress_info(void)
{
    return en_us_amazon_stress;
}

/* 0x4b694 arm */
xsampa_ctx *en_us_amazon_xsampa_ctx(void)
{
    if (en_us_amazon_xsampa.stress == NULL)
        en_us_amazon_xsampa.stress = en_us_amazon_stress_info();
    return &en_us_amazon_xsampa;
}

/* 0x4b6d0 arm: the en-GB xsampa_to_xivona_func */
cst_val *en_gb_xsampa_to_xivona(const char *xsampa, const char *alphabet, const char *lang)
{
    xsampa_ctx *ctx;

    if (alphabet && strcmp(alphabet, "x-amazon-en-gb") == 0)
        ctx = en_gb_amazon_xsampa_ctx();
    else
        ctx = en_gb_xsampa_ctx();
    return en_gb_syllabify(en_fix_checked_vowels(
        (cst_val *)xsampa_word(xsampa, alphabet, lang, ctx)));
}

/* 0x4b730 arm: the en-US xsampa_to_xivona_func */
cst_val *en_us_xsampa_to_xivona(const char *xsampa, const char *alphabet, const char *lang)
{
    xsampa_ctx *ctx;

    if (alphabet && strcmp(alphabet, "x-amazon-en-us") == 0)
        ctx = en_us_amazon_xsampa_ctx();
    else
        ctx = en_us_xsampa_ctx();
    return en_us_syllabify(en_fix_checked_vowels(
        (cst_val *)xsampa_word(xsampa, alphabet, lang, ctx)));
}
