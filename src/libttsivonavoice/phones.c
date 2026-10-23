/*
 * Phone lists (0x71008..): lists of phone name strings with stress digits, and lookups
 * of phone features ignoring the stress.
 */
#include <stdio.h>
#include <string.h>
#include "cst_phoneset.h"
#include "cst_tokenstream.h"
#include "cst_val.h"
#include "cst_val_defs.h"

/* a named table entry */
typedef struct named_entry {
    const char *name;
} named_entry;

typedef const cst_val *(*phonefunc)(const cst_val *p);
CST_VAL_USER_FUNCPTR_DCLS(phonefunc, phonefunc)

/* 0x71008 arm: the entry named name in a NULL-terminated table; the first entry if
 * there is none. */
const named_entry *named_table_find(const named_entry *const *t, const char *name)
{
    int i;

    for (i = 0; t[i]; i++)
        if (strcmp(name, t[i]->name) == 0)
            return t[i];
    return t[0];
}

CST_VAL_REGISTER_FUNCPTR(phonefunc, phonefunc)

/* 0x710c8 arm: the list of phones in s (separated by blanks and '|'). */
cst_val *phones_from_string(const char *s)
{
    cst_tokenstream *ts = ts_open_string(s);
    cst_val *l = NULL;

    ts->p_whitespacesymbols = " |";
    ts->p_singlecharsymbols = "-";
    ts->p_postpunctuationsymbols = "";
    ts->p_prepunctuationsymbols = "";
    while (!ts_eof(ts))
        if (*ts_get(ts))
            l = cons_val(string_val(ts->token), l);
    ts_close(ts);
    return val_reverse(l);
}

/* 0x7115c arm */
const char *phone_name(const cst_val *p)
{
    return val_string(val_car(p));
}

/* 0x7116c arm: feature of the phone at p without its stress ("" for none or "-"). */
const char *phone_feature_base(const cst_phoneset *ps, const cst_val *p, const char *feat)
{
    char buf[8];

    if (p == NULL || strcmp("-", phone_name(p)) == 0)
        return "";
    strncpy(buf, phone_name(p), sizeof buf);
    while (strchr("1230?", buf[strlen(buf) - 1]))
        buf[strlen(buf) - 1] = 0;
    return phone_feature_string(ps, buf, feat);
}

/* 0x7122c arm: inserts a "-" after p. */
cst_val *phones_insert_sep(cst_val *p)
{
    set_cdr(p, cons_val(string_val("-"), val_cdr(p)));
    return p;
}

/* 0x71270 arm: the stress digit of the phone at p, or NULL. */
const char *phone_stress(const cst_val *p)
{
    const char *s;
    size_t n;

    if (p == NULL)
        return NULL;
    s = phone_name(p);
    n = strlen(s);
    if (n == 0)
        return NULL;
    if (strchr("123", (unsigned char)s[n - 1]))
        return &s[n - 1];
    return NULL;
}

/* 0x712c8 arm: appends stress n to the phone at p. */
cst_val *phone_set_stress(cst_val *p, int n)
{
    char buf[8];
    const cst_val *old;

    if (n > 0) {
        snprintf(buf, sizeof buf, "%s%d", phone_name(p), n);
        old = val_car(p);
        set_car(p, string_val(buf));
        delete_val((cst_val *)old);
    }
    return p;
}

/* 0x71338 arm: drops the stress digit of the phone at p. */
cst_val *phone_strip_stress(cst_val *p)
{
    char buf[8];
    const cst_val *old;

    strncpy(buf, phone_name(p), sizeof buf);
    if (strchr("0123", buf[strlen(buf) - 1])) {
        buf[strlen(buf) - 1] = 0;
        old = val_car(p);
        set_car(p, string_val(buf));
        delete_val((cst_val *)old);
    }
    return p;
}

/* 0x713c4 arm */
int phone_is_silence(const cst_phoneset *ps, const cst_val *p)
{
    if (p == NULL)
        return 0;
    return strcmp(ps->silence, phone_name(p)) == 0;
}

/* 0x713fc arm */
int ph_is_vowel(const cst_phoneset *ps, const cst_val *p)
{
    return strcmp("+", phone_feature_base(ps, p, "vc")) == 0;
}

/* 0x71430 arm */
int ph_is_long(const cst_phoneset *ps, const cst_val *p)
{
    return strcmp("l", phone_feature_base(ps, p, "vlng")) == 0;
}

/* 0x71464 arm: a vowel or a syllabic consonant (lx, mx, nx). */
int ph_is_syllabic(const cst_phoneset *ps, const cst_val *p)
{
    const char *s;

    if (p == NULL)
        return 0;
    if (strcmp("+", phone_feature_base(ps, p, "vc")) == 0)
        return 1;
    s = phone_name(p);
    if (strchr("lmn", s[0]) == NULL)
        return 0;
    if (s[1] == 'x')
        return 1;
    return 0;
}

/* 0x714e0 arm */
int ph_is_glide(const cst_phoneset *ps, const cst_val *p)
{
    return strcmp("g", phone_feature_base(ps, p, "ctype")) == 0;
}

/* 0x71514 arm */
int ph_is_nasal(const cst_phoneset *ps, const cst_val *p)
{
    return strcmp("n", phone_feature_base(ps, p, "ctype")) == 0;
}

/* 0x71548 arm */
int ph_is_short(const cst_phoneset *ps, const cst_val *p)
{
    return strcmp("n", phone_feature_base(ps, p, "vlng")) == 0;
}

/* 0x7157c arm */
int ph_is_fricative(const cst_phoneset *ps, const cst_val *p)
{
    return strcmp("f", phone_feature_base(ps, p, "ctype")) == 0;
}

/* 0x715b0 arm */
int ph_is_stop(const cst_phoneset *ps, const cst_val *p)
{
    return strcmp("p", phone_feature_base(ps, p, "ctype")) == 0;
}

/* 0x715e4 arm */
int ph_is_liquid(const cst_phoneset *ps, const cst_val *p)
{
    return strcmp("r", phone_feature_base(ps, p, "ctype")) == 0;
}

/* 0x71618 arm: a velar stop. */
int ph_cplace(const cst_phoneset *ps, const cst_val *p)
{
    if (strcmp("p", phone_feature_base(ps, p, "ctype")) != 0)
        return 0;
    return strcmp("g", phone_feature_base(ps, p, "cplace")) == 0;
}

/* 0x71690 arm: the number of syllables ("-"-separated groups). */
int phones_count_syllables(const cst_val *p)
{
    int n = 0, sep = 1;

    for (; p; p = val_cdr(p)) {
        if (strcmp("-", phone_name(p)) == 0)
            sep = 1;
        else if (sep) {
            n++;
            sep = 0;
        }
    }
    return n;
}

/* 0x716fc arm */
int phones_have_stress(const cst_val *p)
{
    for (; p; p = val_cdr(p))
        if (phone_stress(p))
            return 1;
    return 0;
}

/* 0x7173c arm: the first phone with stress s. */
const cst_val *phones_find_stress(const cst_val *p, const char *s)
{
    const char *st;

    if (s == NULL || p == NULL)
        return NULL;
    for (; p; p = val_cdr(p)) {
        st = phone_stress(p);
        if (st && strcmp(st, s) == 0)
            return p;
    }
    return NULL;
}

/* 0x71794 arm: gives the vowels of syllable n stress. */
cst_val *phones_stress_syllable(const cst_phoneset *ps, cst_val *p, int stress, int n)
{
    int ok = p != NULL && n >= 0, i = 0, more;
    cst_val *q;

    if (!ok)
        return p;
    for (q = p; ; ) {
        if (strcmp("-", phone_name(q)) == 0) {
            i++;
            more = i <= n;
        } else if (i == n) {
            more = ok;
            if (strcmp("+", phone_feature_base(ps, q, "vc")) == 0)
                phone_set_stress(q, stress);
        } else
            more = i <= n;
        q = (cst_val *)val_cdr(q);
        if (q == NULL || !more)
            break;
    }
    return p;
}

/* 0x718a4 arm: drops the stress of the phones from..to. */
cst_val *phones_strip_stress_range(cst_val *from, const cst_val *to)
{
    cst_val *p;

    if (from == to || from == NULL)
        return from;
    for (p = from; ; ) {
        phone_strip_stress(p);
        p = (cst_val *)val_cdr(p);
        if (p == to || p == NULL)
            break;
    }
    return from;
}

/* 0x718e8 arm */
cst_val *phones_strip_stress(cst_val *p)
{
    return phones_strip_stress_range(p, NULL);
}

/* 0x718f0 arm: keeps only the first stress in from..to. */
cst_val *phones_keep_first_stress_range(cst_val *from, const cst_val *to)
{
    cst_val *p;
    int n = 0;

    if (from == to || from == NULL)
        return from;
    for (p = from; ; ) {
        if (phone_stress(p) && ++n != 1)
            phone_strip_stress(p);
        p = (cst_val *)val_cdr(p);
        if (p == to || p == NULL)
            break;
    }
    return from;
}

/* 0x71954 arm */
cst_val *phones_keep_first_stress(cst_val *p)
{
    return phones_keep_first_stress_range(p, NULL);
}
