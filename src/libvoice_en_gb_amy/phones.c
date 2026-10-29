/*
 * Phone lists: lists of phone name strings with stress digits, and lookups
 * of phone features ignoring the stress.
 */
#include <stdio.h>
#include <string.h>
#include "cst_string.h"
#include "cst_phoneset.h"
#include "cst_tokenstream.h"
#include "cst_val.h"
#include "cst_val_defs.h"
#include "cst_item.h"
#include "ivona_util.h"



cst_item *item_traverse(const cst_item *item, ...);

typedef const cst_val *(*phonefunc)(const cst_val *p);
CST_VAL_USER_FUNCPTR_DCLS(phonefunc, phonefunc)
CST_VAL_REGISTER_FUNCPTR(phonefunc, phonefunc)

/* 0x5abe8 arm: the list of phones in s (separated by blanks and '|'). */
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

/* 0x5ac7c arm */
const char *phone_name(const cst_val *p)
{
    return val_string(val_car(p));
}

/* 0x5ac8c arm: feature of the phone at p without its stress ("" for none or "-"). */
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

/* 0x5ad4c arm: inserts a "-" after p. */
cst_val *phones_insert_sep(cst_val *p)
{
    set_cdr(p, cons_val(string_val("-"), val_cdr(p)));
    return p;
}

/* 0x5ad90 arm: the stress digit of the phone at p, or NULL. */
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

/* 0x5ade8 arm: appends stress n to the phone at p. */
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

/* 0x5ae58 arm: drops the stress digit of the phone at p. */
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

/* 0x5aee4 arm */
int phone_is_silence(const cst_phoneset *ps, const cst_val *p)
{
    if (p == NULL)
        return 0;
    return strcmp(ps->silence, phone_name(p)) == 0;
}

/* 0x5af1c arm */
int ph_is_vowel(const cst_phoneset *ps, const cst_val *p)
{
    return strcmp("+", phone_feature_base(ps, p, "vc")) == 0;
}

/* 0x5af50 arm */
int ph_is_long(const cst_phoneset *ps, const cst_val *p)
{
    return strcmp("l", phone_feature_base(ps, p, "vlng")) == 0;
}

/* 0x5af84 arm: a vowel or a syllabic consonant (lx, mx, nx). */
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

/* 0x5b000 arm */
int ph_is_glide(const cst_phoneset *ps, const cst_val *p)
{
    return strcmp("g", phone_feature_base(ps, p, "ctype")) == 0;
}

/* 0x5b034 arm */
int ph_is_nasal(const cst_phoneset *ps, const cst_val *p)
{
    return strcmp("n", phone_feature_base(ps, p, "ctype")) == 0;
}

/* 0x5b068 arm */
int ph_is_short(const cst_phoneset *ps, const cst_val *p)
{
    return strcmp("n", phone_feature_base(ps, p, "vlng")) == 0;
}

/* 0x5b09c arm */
int ph_is_fricative(const cst_phoneset *ps, const cst_val *p)
{
    return strcmp("f", phone_feature_base(ps, p, "ctype")) == 0;
}

/* 0x5b0d0 arm */
int ph_is_stop(const cst_phoneset *ps, const cst_val *p)
{
    return strcmp("p", phone_feature_base(ps, p, "ctype")) == 0;
}

/* 0x5b104 arm */
int ph_is_liquid(const cst_phoneset *ps, const cst_val *p)
{
    return strcmp("r", phone_feature_base(ps, p, "ctype")) == 0;
}

/* 0x5b138 arm: a velar stop. */
int ph_is_velar_stop(const cst_phoneset *ps, const cst_val *p)
{
    if (strcmp("p", phone_feature_base(ps, p, "ctype")) != 0)
        return 0;
    return strcmp("g", phone_feature_base(ps, p, "cplace")) == 0;
}

/* 0x5b1b0 arm: the number of syllables ("-"-separated groups). */
int phones_count_syllables(const cst_val *p)
{
    int sep = 1, n = 0;

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

/* 0x5b21c arm */
int phones_have_stress(const cst_val *p)
{
    for (; p; p = val_cdr(p))
        if (phone_stress(p))
            return 1;
    return 0;
}

/* 0x5b25c arm: the first phone with stress s. */
const cst_val *phones_find_stress(const cst_val *p, const char *s)
{
    const char *st;

    if (s == NULL)
        return NULL;
    while (p != NULL) {
        st = phone_stress(p);
        if (st && strcmp(st, s) == 0)
            return p;
        p = val_cdr(p);
    }
    return NULL;
}

/* 0x5b2b4 arm: gives the vowels of syllable n stress. */
cst_val *phones_stress_syllable(const cst_phoneset *ps, cst_val *p, int stress, int n)
{
    int ok = p != NULL;
    int i = 0, more;
    cst_val *q;

    if (n < 0)
        ok = 0;
    if (!ok)
        return p;
    for (q = p; ; ) {
        if (strcmp("-", phone_name(q)) == 0) {
            i++;
            more = n >= i;
        } else if (i == n) {
            more = ok;
            if (strcmp("+", phone_feature_base(ps, q, "vc")) == 0)
                phone_set_stress(q, stress);
        } else
            more = n >= i;
        q = (cst_val *)val_cdr(q);
        if (q == NULL || !more)
            break;
    }
    return p;
}

/* 0x5b3c4 arm: drops the stress of the phones from..to. */
cst_val *phones_strip_stress_range(cst_val *from, const cst_val *to)
{
    cst_val *p = from;

    if (p == to || p == NULL)
        return from;
    for (; ; ) {
        phone_strip_stress(p);
        p = (cst_val *)val_cdr(p);
        if (to == p || p == NULL)
            break;
    }
    return from;
}

/* 0x5b408 arm */
cst_val *phones_strip_stress(cst_val *p)
{
    return phones_strip_stress_range(p, NULL);
}

/* 0x5b410 arm: keeps only the first stress in from..to. */
cst_val *phones_keep_first_stress_range(cst_val *from, const cst_val *to)
{
    cst_val *p;
    int n;

    if (!(from == to || from == NULL)) {
    n = 0;
    for (p = from; ; ) {
        if (phone_stress(p)) {
            n++;
            if (n != 1)
                phone_strip_stress(p);
        }
        p = (cst_val *)val_cdr(p);
        if (to == p || p == NULL)
            break;
    }
    }
    return from;
}

/* 0x5b474 arm */
cst_val *phones_keep_first_stress(cst_val *p)
{
    return phones_keep_first_stress_range(p, NULL);
}

/* 0x5b47c arm: appends a segment's phone ("|"-terminated, with its syllable's stress on
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
    if (__builtin_expect(mark_glottal != 0, 0)) {
        if (ffeature_int(item_traverse(seg, 0), "pos_in_syl") + 1 == gpos)
            strbuf_append(sb, "?", 1);
    }
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

/* 0x5b628 arm: the phones of a word, syllables separated by "-". */
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

#include "cst_relation.h"
#include "phone_rule.h"

/* 0x5b6ac arm: applies the first matching rule to each pair of adjacent segments */
void apply_phone_rules(cst_utterance *u, const phone_rule *rules)
{
    cst_item *s, *n, *sy1, *sy2;
    const phone_rule *r;

    for (s = relation_head(utt_relation(u, 6)); s && item_next(s); s = item_next(s)) {
        n = item_next(s);
        sy1 = item_parent(item_as(s, 4));
        sy2 = item_parent(item_as(n, 4));
        for (r = rules; r->from1; r++) {
            if (r->syllables == 1 ? sy1 != sy2 : r->syllables == 2 ? sy1 == sy2 : r->syllables != 0)
                continue;
            if ((r->stress1 >= 0 && !(sy1 && item_feat_int(sy1, "stress") == r->stress1))
                || (r->stress2 >= 0 && !(sy2 && item_feat_int(sy2, "stress") == r->stress2))
                || strcmp(item_feat_string(s, "name"), r->from1) != 0
                || strcmp(item_feat_string(n, "name"), r->from2) != 0)
                continue;
            if (r->new_stress1 >= 0)
                item_set_int(sy1, "stress", r->new_stress1);
            if (r->new_stress2 >= 0)
                item_set_int(sy2, "stress", r->new_stress2);
            if (r->to1) {
                if (*r->to1)
                    item_set_string(s, "name", r->to1);
                else {
                    delete_item(item_as(s, 4));
                    delete_item(s);
                    s = item_prev(n);
                }
            }
            if (r->to2) {
                if (*r->to2)
                    item_set_string(n, "name", r->to2);
                else {
                    delete_item(item_as(n, 4));
                    delete_item(n);
                    item_next(s);
                }
            }
            break;
        }
    }
}
