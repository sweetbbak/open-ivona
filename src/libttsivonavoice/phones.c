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
#include "cst_item.h"
#include "ivona_util.h"

/* a named table entry */
typedef struct named_entry {
    const char *name;
} named_entry;

cst_item *item_traverse(const cst_item *item, ...);

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
    if (strchr("lmn", s[0]) && s[1] == 'x')
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
        if (to == p || p == NULL)
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

    for (p = from; to != p && p; p = (cst_val *)val_cdr(p)) {
        if (phone_stress(p)) {
            n++;
            if (n != 1)
                phone_strip_stress(p);
        }
    }
    return from;
}

/* 0x71954 arm */
cst_val *phones_keep_first_stress(cst_val *p)
{
    return phones_keep_first_stress_range(p, NULL);
}

/* 0x7195c arm: appends a segment's phone ("|"-terminated, with its syllable's stress on
 * vowels, "?" before a glottal stop if mark_glottal, "-" after a syllable's last phone if
 * sep_syls). */
void seg_append_phone(const cst_item *seg, strbuf *sb, int sep_syls, int mark_glottal)
{
    const char *name = item_feat_string(seg, "name");
    const cst_item *syl;
    int stress, gpos;
    char c;

    if (strcmp(name, "pau") == 0)
        return;
    syl = item_parent(item_as(seg, 4));
    stress = ffeature_int(item_traverse(syl, 0), "stress");
    gpos = ffeature_int(item_traverse(syl, 0), "glottal_stop_pos");
    strbuf_append_str(sb, name);
    if (mark_glottal && ffeature_int(item_traverse(seg, 0), "pos_in_syl") + 1 == gpos)
        strbuf_append(sb, "?", 1);
    if (strcmp("+", ffeature_string(item_traverse(seg, 0), "ph_vc")) == 0) {
        if (stress != 0) {
            c = stress + '0';
            strbuf_append(sb, &c, 1);
        }
    }
    strbuf_append(sb, "|", 1);
    if (sep_syls && item_next(seg) == NULL && item_next(syl) != NULL)
        strbuf_append(sb, "-", 1);
}

/* 0x71b08 arm: the phones of a word, syllables separated by "-". */
char *word_phone_string(const cst_item *word)
{
    const cst_item *w = item_as(word, 4), *syl, *seg;
    strbuf sb;

    strbuf_init(&sb);
    for (syl = item_daughter(w); syl; syl = item_next(syl))
        for (seg = item_daughter(syl); seg; seg = item_next(seg))
            seg_append_phone(seg, &sb, 1, 0);
    return strbuf_data(&sb);
}

/* a compact lexicon entry: name, part of speech ('*' any), phone ids ending in 0xff */
typedef struct lex_entry {
    const char *name;
    char pos;
    unsigned char phones[1];
} lex_entry;

typedef struct lex_table {
    int n;
    const lex_entry *const *entries;    /* sorted by name */
    const char *const *phone_names;
} lex_table;

int cst_bsearch(const void *key, const void *base, int n, int size,
                int (*cmp)(const void *key, const void *elem, void *arg), void *arg);

/* 0x71e14 arm */
static int lex_entry_cmp(const void *key, const void *elem, void *arg)
{
    return strcmp(key, (*(const lex_entry *const *)elem)->name);
}

/* 0x71e20 arm: the entry for name with part of speech pos ("0" or NULL: any), or -1. */
int lex_lookup_index(const lex_table *t, const char *name, const char *pos)
{
    const lex_entry *const *e = t->entries;
    int i;

    if (pos == NULL || *pos == 0)
        pos = "0";
    i = cst_bsearch(name, e, t->n, sizeof *e, lex_entry_cmp, NULL);
    if (i == -1)
        return i;
    while (i > 0 && strcmp(name, e[i - 1]->name) == 0)
        i--;
    for (; i < t->n; i++) {
        if (strcmp(name, e[i]->name) != 0)
            break;
        if (*pos == '0' || e[i]->pos == '*' || *pos == e[i]->pos)
            return i;
    }
    return -1;
}

/* 0x71f40 arm: the first entry name not below name, or NULL. */
const char *lex_lower_bound_name(const lex_table *t, const char *name)
{
    unsigned int n = t->n, lo = 0, hi = n, mid;
    const char *s;
    int c;

    while (lo < hi) {
        mid = lo + (hi - lo) / 2;
        s = t->entries[mid]->name;
        c = strcmp(name, s);
        if (c == 0)
            return s;
        if (c > 0)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo == n)
        return NULL;
    return t->entries[lo]->name;
}

/* 0x71fc4 arm */
int lex_has_entry(const lex_table *t, const char *name, const char *pos)
{
    return lex_lookup_index(t, name, pos) >= 0;
}

/* 0x71fd8 arm: the parts of speech of name's entries (up to max of them in out);
 * returns how many there are. */
int lex_lookup_pos_all(const lex_table *t, const char *name, char *out, unsigned int max)
{
    const lex_entry *const *e = t->entries, *x;
    int i, k = 0;

    i = cst_bsearch(name, e, t->n, sizeof *e, lex_entry_cmp, NULL);
    if (i == -1)
        return 0;
    while (i > 0 && strcmp(name, e[i - 1]->name) == 0)
        i--;
    if (i >= t->n)
        return 0;
    for (e += i; ; ) {
        x = *e++;
        if (strcmp(name, x->name) != 0)
            break;
        if (max > (unsigned int)k)
            out[k] = x->pos;
        k++;
        if (e[-1]->pos == '*' || t->n <= k + i)
            break;
    }
    return k;
}

/* 0x720d4 arm: the phone ids of name's entry, or NULL. */
const unsigned char *lex_lookup_data(const lex_table *t, const char *name, const char *pos)
{
    const lex_entry *const *e = t->entries;
    int i = lex_lookup_index(t, name, pos);

    if (i < 0)
        return NULL;
    return e[i]->phones;
}

/* 0x720f4 arm: the phones of name's entry as a list, or NULL. */
cst_val *lex_lookup_phones(const lex_table *t, const char *name, const char *pos)
{
    const unsigned char *p = lex_lookup_data(t, name, pos);
    cst_val *l = NULL;

    if (p == NULL)
        return NULL;
    for (; *p != 0xff; p++)
        l = cons_val(string_val(t->phone_names[*p]), l);
    return val_reverse(l);
}
