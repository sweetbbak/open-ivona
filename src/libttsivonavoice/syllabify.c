/*
 * Syllabification of phone lists (..0x6e1a0): "-" entries separate syllables.
 */
#include <string.h>
#include "cst_val.h"
#include "cst_phoneset.h"

const char *phone_name(const cst_val *p);
int ph_is_syllabic(const cst_phoneset *ps, const cst_val *p);

/* 0x6df20 arm: whether the syllable starting at p has a syllabic phone. */
int syl_has_syllabic(const cst_phoneset *ps, const cst_val *p)
{
    for (; p && strcmp(phone_name(p), "-") != 0; p = val_cdr(p))
        if (ph_is_syllabic(ps, p))
            return 1;
    return 0;
}

int phone_is_silence(const cst_phoneset *ps, const cst_val *p);
cst_val *phones_insert_sep(cst_val *p);
int FUN_0006de58(const cst_phoneset *ps, const cst_val *p);

/* Sonority of a phone: 5 for syllabic phones and silence. */
static inline int syl_sonority(const cst_phoneset *ps, const cst_val *p)
{
    if (ph_is_syllabic(ps, p) || phone_is_silence(ps, p))
        return 5;
    return FUN_0006de58(ps, p);
}

/* Flite's cmu_syl_boundary: whether a syllable (with syllabic phone syl so far) ends after p. */
static inline int syl_boundary(const cst_phoneset *ps, const cst_val *p, const cst_val *rest,
                               const cst_val *syl, int (*boundary)(const cst_val *p))
{
    int sp, sn, snn;

    if (phone_is_silence(ps, rest))
        return 1;
    if (!syl_has_syllabic(ps, rest))
        return 0;
    if (!syl)
        return 0;
    if (ph_is_syllabic(ps, rest))
        return 1;
    if (val_cdr(rest) == NULL)
        return 0;
    if (boundary)
        return boundary(p);
    sp = syl_sonority(ps, p);
    sn = syl_sonority(ps, val_cdr(p));
    snn = syl_sonority(ps, val_cdr(val_cdr(p)));
    return sp <= sn && sn <= snn;
}

/* 0x6df8c arm: inserts "-" syllable boundaries into phones, asking boundary(p), when given,
 * instead of the sonority rule whether a syllable ends after p. */
cst_val *syl_split(const cst_phoneset *ps, cst_val *phones, int (*boundary)(const cst_val *p))
{
    cst_val *p;
    const cst_val *syl = NULL;

    if (phones == NULL)
        return phones;
    for (p = phones; val_cdr(p); p = (cst_val *)val_cdr(p)) {
        const cst_val *rest = val_cdr(p);

        if (strcmp("-", phone_name(p)) == 0)
            syl = NULL;
        else if (ph_is_syllabic(ps, p))
            syl = p;

        if (syl_boundary(ps, p, rest, syl, boundary)) {
            p = (cst_val *)val_cdr(phones_insert_sep(p));
            syl = NULL;
        }
    }
    return phones;
}

/* 0x6e1a0 arm */
cst_val *syl_split_default(const cst_phoneset *ps, cst_val *phones)
{
    return syl_split(ps, phones, NULL);
}
