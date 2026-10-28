/*
 * Rule-based rewriting and syllabification of phone lists (..0x6e1a0): "-" entries separate
 * syllables.
 */
#include <string.h>
#include "cst_val.h"
#include "cst_phoneset.h"

const char *phone_name(const cst_val *p);
int ph_is_syllabic(const cst_phoneset *ps, const cst_val *p);
int phone_is_silence(const cst_phoneset *ps, const cst_val *p);
cst_val *phones_insert_sep(cst_val *p);
const char *phone_feature_base(const cst_phoneset *ps, const cst_val *p, const char *feat);

/* A context is a NULL-terminated list of alternatives (character sets). */
typedef const char *const *str_ctx;

int str_ctx_match_left_repeat(const str_ctx *left, unsigned int n, const char *s, unsigned int pos);

/* 0x6db2c arm: whether s[..pos) ends with the context sequence left[0..n) (matched from the
 * end). "*" and "+" after a context repeat it as in str_ctx_match. */
int str_ctx_match_left(const str_ctx *left, unsigned int n, const char *s, unsigned int pos)
{
    const char *const *alt = left[n - 1];

    if (n > 1 && *alt) {
        if (strcmp(*alt, "*") == 0) {
            if (n - 2 == 0)
                return 1;
            if (str_ctx_match_left(left, n - 2, s, pos))
                return 1;
            return str_ctx_match_left_repeat(left, n - 1, s, pos);
        }
        if (strcmp(*alt, "+") == 0)
            return str_ctx_match_left_repeat(left, n - 1, s, pos);
    }
    for (; *alt; alt++) {
        unsigned int len = strlen(*alt);

        if (pos < len || memcmp(s + pos - len, *alt, len) != 0)
            continue;
        if (n - 1 == 0)
            return 1;
        if (str_ctx_match_left(left, n - 1, s, pos - len))
            return 1;
    }
    return 0;
}

typedef struct str_rewrite_rule {
    const unsigned char *tag;   /* +0x00: NULL or "" applies everywhere */
    const str_ctx *left;        /* +0x04: contexts before match, nearest first; NULL ends the table */
    const char *match;          /* +0x08 */
    const str_ctx *right;       /* +0x0c: contexts after match */
    const char *output;         /* +0x10 */
} str_rewrite_rule;

int str_ctx_match(const str_ctx *ctx, const char *s);

/* 0x6d550 arm: whether s starts with one or more repetitions of ctx[0] followed by ctx + 2.
 * TODO: the binary inlines the recursion ~5 levels deep, this builds ~8. */
static inline int str_ctx_match_repeat(const str_ctx *ctx, const char *s)
{
    const char *const *alt;

    for (alt = ctx[0]; *alt; alt++) {
        unsigned int n = strlen(*alt);

        if (strncmp(s, *alt, n) == 0
            && (str_ctx_match(ctx + 2, s + n) || str_ctx_match_repeat(ctx, s + n)))
            return 1;
    }
    return 0;
}

/* 0x6d794 arm: whether s starts with the context sequence ctx. A context followed by "*" may
 * repeat zero or more times, one followed by "+" one or more times. */
int str_ctx_match(const str_ctx *ctx, const char *s)
{
    const char *const *alt = ctx[0];

    if (alt == NULL || alt[0] == NULL)
        return 1;
    if (ctx[1] && ctx[1][0]) {
        if (strcmp(ctx[1][0], "*") == 0) {
            if (str_ctx_match(ctx + 2, s))
                return 1;
            return str_ctx_match_repeat(ctx, s);
        }
        if (strcmp(ctx[1][0], "+") == 0)
            return str_ctx_match_repeat(ctx, s);
    }
    for (; *alt; alt++) {
        unsigned int n = strlen(*alt);

        if (strncmp(s, *alt, n) == 0 && str_ctx_match(ctx + 1, s + n))
            return 1;
    }
    return 0;
}

/* 0x6dc94 arm: rewrites s[start..start+len) into out: at each position the first rule whose
 * match and contexts fit is applied; characters no rule matches are dropped. */
void str_rewrite_span(const char *s, unsigned int start, unsigned int len,
                      const str_rewrite_rule *rules, const char *tag, char *out)
{
    unsigned int end = start + len;
    unsigned int pos = start;

    while (pos < end) {
        const str_rewrite_rule *r;

        for (r = rules; r->left; r++) {
            const char *m = r->match;
            unsigned int n = strlen(m);

            if (r->tag && r->tag[0] && (tag == NULL || strcmp((const char *)r->tag, tag) != 0))
                continue;
            if (pos + n > end || strncmp(s + pos, m, n) != 0)
                continue;
            if (r->left[0] && r->left[0][0]) {
                int i = 1;

                while (r->left[i] && r->left[i][0])
                    i++;
                if (!str_ctx_match_left(r->left, i, s, pos))
                    continue;
            }
            if (!str_ctx_match(r->right, s + pos + n))
                continue;
            pos += n;
            n = strlen(r->output);
            memcpy(out, r->output, n);
            out += n;
            break;
        }
        if (r->left == NULL)
            pos++;
    }
    *out = '\0';
}

/* 0x6de1c arm: rewrites all of s into out with the rules tagged tag (or untagged). */
void str_rewrite(const char *s, const str_rewrite_rule *rules, const char *tag, char *out)
{
    str_rewrite_span(s, 0, strlen(s), rules, tag, out);
}

/* 0x6de58 arm: sonority of a consonant (Flite's cmu_sonority): liquids and glides 4, nasals 3,
 * voiced fricatives, affricates and plosives 2, others 1. */
int syl_cons_sonority(const cst_phoneset *ps, const cst_val *p)
{
    const char *ctype = phone_feature_base(ps, p, "ctype");

    if (strchr("grl", (unsigned char)ctype[0]))
        return 4;
    if (strcmp("n", ctype) == 0)
        return 3;
    if (strcmp("+", phone_feature_base(ps, p, "cvox")) == 0 && strchr("fap", (unsigned char)ctype[0]))
        return 2;
    return 1;
}

/* 0x6df20 arm: whether the syllable starting at p has a syllabic phone. */
int syl_has_syllabic(const cst_phoneset *ps, const cst_val *p)
{
    for (; p && strcmp(phone_name(p), "-") != 0; p = val_cdr(p))
        if (ph_is_syllabic(ps, p))
            return 1;
    return 0;
}


/* Sonority of a phone: 5 for syllabic phones and silence. */
static inline int syl_sonority(const cst_phoneset *ps, const cst_val *p)
{
    if (ph_is_syllabic(ps, p) || phone_is_silence(ps, p))
        return 5;
    return syl_cons_sonority(ps, p);
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
