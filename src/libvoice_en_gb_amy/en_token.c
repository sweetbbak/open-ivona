/*
 * English tokens to words (0x48fa0..): sentence boundaries and the expansion of tokens (numbers,
 * dates, times, money, abbreviations, ...) into words.
 */
#include <string.h>
#include "cst_item.h"
#include "cst_ffeatures.h"
#include "cst_regex.h"
#include "cst_val.h"

#include "cst_alloc.h"
#include "cst_string.h"

extern const cst_regex *const en_rx_time_ampm, *const en_rx_dotted_letters;

cst_item *item_traverse(const cst_item *item, ...);
int en_is_initial(const cst_item *tok);
cst_val *en_exp_letters(const cst_item *t, const char *s);

static inline int is_british(const cst_item *t)
{
    return strcmp(ffeature_string(item_traverse(t, 0), "language_code"), "en-AU") == 0
           || cst_startswith(ffeature_string(item_traverse(t, 0), "language_code"), "en-GB")
           || strcmp(ffeature_string(item_traverse(t, 0), "language_code"), "en-IN") == 0;
}

/* 0x13159c: the base letters of 0xe0..0xff (a with accents, ae, c cedilla, ...) */
static const char en_base_letters[] = "aaaaaaeseeeeiiii.nooooo.ouuuuy.y";

/* 0x48fa0 arm: whether s is an abbreviation of dotted letters ("U.S.A."), its last full stop
 * possibly the token's punctuation */
int en_is_dotted_abbrev(const cst_item *tok, const char *s)
{
    const char *p;

    if (strlen(s) < 3 || !cst_regex_match(en_rx_dotted_letters, s))
        return 0;
    if (s[strlen(s) - 1] == '.')
        return 1;
    return (p = item_feat_string(tok, "punc")) != NULL && *p == '.';
}

/* 0x49034 arm: a full stop the abbreviation s owns is no longer the token's punctuation */
void en_take_abbrev_stop(cst_item *tok, const char *s)
{
    const char *p;

    if (*s && s[strlen(s) - 1] == '.')
        return;
    if ((p = item_feat_string(tok, "punc")) != NULL && *p == '.')
        item_set_string(tok, "punc", p + 1);
}

/* 0x490a4 arm: a lowercase copy of s (ASCII and Latin-1) */
char *en_lowercase(const char *s)
{
    char *d = cst_strdup(s);
    unsigned int c;
    int i;

    for (i = 0; (c = (unsigned char)s[i]) != 0; i++)
        if (c - 'A' < 26 || ((unsigned char)(c + 0x40) < 0x1f && c != 0xd7))
            d[i] = c + ' ';
    return d;
}

/* 0x49114 arm: s in lowercase ASCII: accented letters lose their accents, "ss" for sharp s,
 * "th" for eth and thorn */
char *en_fold(const char *s)
{
    size_t n = strlen(s);
    char *d = cst_safe_alloc(n + 1, 1);
    unsigned int c;
    int j = 0;

    for (; (c = (unsigned char)*s) != 0; s++) {
        if (c - 'A' < 26) {
            d[j++] = c + ' ';
            continue;
        }
        if (c < 0xbf || (c & 0xdf) == 0xd7 || c == 0xbf) {
            d[j++] = c;
            continue;
        }
        if (c < 0xdf)
            c = (c + 0x20) & 0xff;
        else if (c == 0xdf) {
            d = cst_safe_realloc(d, n + 2, 1);
            n++;
            d[j++] = 's';
            d[j++] = 's';
            continue;
        }
        if (c == 0xf0 || c == 0xfe) {
            d = cst_safe_realloc(d, n + 2, 1);
            n++;
            d[j++] = 't';
            d[j++] = 'h';
            continue;
        }
        d[j++] = en_base_letters[c - 0xe0];
    }
    d[j] = '\0';
    return d;
}

/* 0x49238 arm: s spelt as one dotted word ("u.s.a."), or a single letter's name */
cst_val *en_spell_dotted(const cst_item *tok, const char *s)
{
    char *f = en_fold(s), *buf;
    size_t n = strlen(f);
    cst_val *r;
    int i;

    if (n < 2)
        r = en_exp_letters(tok, s);
    else {
        buf = cst_safe_alloc(n * 2 + 1, 1);
        for (i = 0; f[i]; i++) {
            buf[2 * i] = f[i];
            buf[2 * i + 1] = '.';
        }
        buf[2 * i] = '\0';
        r = cons_val(string_val(buf), NULL);
        cst_free(buf);
    }
    cst_free(f);
    return r;
}

int en_in_lex(const cst_item *item, const char *word, const char *pos, int main_only);
cst_val *en_exp_number(const cst_item *t, const char *s);
cst_val *en_exp_id(const cst_item *t, const char *s);
cst_val *en_exp_real(const cst_item *t, const char *s);
cst_val *en_exp_sign(const cst_item *t, int c);
const cst_val *gpos_string(const char *w);
cst_val *cst_val_strappend(cst_val *val, const char *str);
const cst_val *val_last(const cst_val *v);
cst_val *cst_split_val(const char *s, const char *delims);
char *cst_strtr(const char *s, const char *from, const char *to);

extern const cst_regex *const en_rx_signed_number, *const en_rx_quoted, *const en_rx_word;
extern const cst_regex *const en_rx_spelled;

/* 0x3c26e4: amounts said in the singular ("one degree") */
static const char *const en_ones[] = { "1", "+1", "-1", "\2611", NULL };

#define ASCII_LETTERS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

cst_val *en_split_hyphens(cst_item *tok, const char *s);

/* whether some part of the hyphenated s is a single character */
static inline int has_one_char_part(const char *s)
{
    const char *p;

    for (p = s; *p; p++)
        if ((p == s || p[-1] == '-') && *p != '\'' && *p != '-' && (p[1] == '-' || p[1] == '\0'))
            return 1;
    return 0;
}

/* whether s, its hyphens dropped, is in the lexicon and none of its parts is a content word */
static inline int compound_of_function_words(const cst_item *tok, const char *s)
{
    char *joined = cst_strtr(s, "-", NULL);
    int in = en_in_lex(tok, joined, "0", 0);
    cst_val *parts;
    const cst_val *v;

    cst_free(joined);
    if (!in)
        return 0;
    parts = cst_split_val(s, "-");
    for (v = parts; v; v = val_cdr(v))
        if (strcmp(val_string(gpos_string(val_string(val_car(v)))), "content") == 0) {
            delete_val(parts);
            return 1;
        }
    delete_val(parts);
    return 0;
}

/* 0x49458 arm: the words of a token string not otherwise expanded (Flite's
 * us_tokentowords_one): "a", numbers (as ids when split from letters, "nsw" "nide"), reals,
 * fractions, percent, cents, degrees, dotted abbreviations, words (lowercased, folded),
 * hyphenated words, clitics, and letter/digit runs split apart */
cst_val *en_token_word(cst_item *tok, const char *s)
{
    const char *nsw = item_feat_present(tok, "nsw") ? item_feat_string(tok, "nsw") : "";
    const char *q;
    char *d, *w, *low, *stem, *right, *apos;
    cst_val *r;
    size_t n, i;
    int same, c, last;

    if (strcmp("a", s) == 0 || strcmp("A", s) == 0)
        return cons_val(string_val(strcmp(s, item_feat_string(tok, "name")) == 0
                                   || strcmp(nsw, "hyphenated-word") == 0 ? "a" : "_a"), NULL);
    if (cst_regex_match(cst_rx_digits, s)) {
        if (s[0] == '0' && strlen(s) > 1)
            return en_exp_letters(tok, s);
        if (strcmp("nide", nsw) != 0)
            return en_exp_number(tok, s);
        if (strlen(s) > 4)
            return en_exp_letters(tok, s);
        if (is_british(tok))
            return en_exp_number(tok, s);
        return en_exp_id(tok, s);
    }
    if (*nsw == '\0' && cst_regex_match(en_rx_signed_number, s))
        return en_exp_real(tok, s);
    if (*s) {
        last = s[strlen(s) - 1];
        if (strchr("\274\275\276", last)) {
            d = cst_strdup(s);
            d[strlen(d) - 1] = '\0';
            if (strlen(d) == 1 && strchr("-+\261", d[0]))
                r = en_exp_sign(tok, d[0]);
            else if (cst_regex_match(cst_rx_int, d))
                r = val_append(en_exp_real(tok, d), cons_val(string_val("and"), NULL));
            else
                r = en_token_word(tok, d);
            r = val_append(r, en_exp_letters(tok, s + strlen(s) - 1));
            cst_free(d);
            return r;
        }
        if (last == '%' || last == (char)0xa2 || last == (char)0xb0) {
            d = cst_strdup(s);
            d[strlen(d) - 1] = '\0';
            if (last == '%')
                r = val_append(en_token_word(tok, d), cons_val(string_val("percent"), NULL));
            else if (last == (char)0xa2)
                r = val_append(en_token_word(tok, d),
                               cons_val(string_val(!cst_regex_match(en_rx_signed_number, d)
                                                           || cst_member_string(d, en_ones)
                                                       ? "cent" : "cents"), NULL));
            else
                r = val_append(en_token_word(tok, d),
                               cons_val(string_val(cst_member_string(d, en_ones) ? "degree"
                                                                                  : "degrees"),
                                        NULL));
            cst_free(d);
            return r;
        }
    }
    if (*nsw == '\0' && en_is_dotted_abbrev(tok, s)) {
        n = strlen(s);
        d = cst_safe_alloc(n + 2, 1);
        memcpy(d, s, n);
        if (d[n - 1] != '.')
            d[n++] = '.';
        d[n] = '\0';
        w = en_fold(d);
        r = cons_val(string_val(w), NULL);
        cst_free(w);
        cst_free(d);
        en_take_abbrev_stop(tok, s);
        return r;
    }
    if (strlen(s) > 1 && cst_regex_match(en_rx_quoted, s)) {
        /* a word */
        low = en_lowercase(s);
        if (!en_in_lex(tok, low, "0", 1)) {
            w = en_fold(low);
            cst_free(low);
        } else
            w = low;
        if (strchr(w, '-') && !cst_regex_match(en_rx_spelled, s)) {
            same = strcasecmp(item_feat_string(tok, "name"), w) == 0;
            if (en_in_lex(tok, w, "0", 0))
                goto whole;
            if (same && cst_regex_match(en_rx_quoted, w) && !has_one_char_part(w)
                && compound_of_function_words(tok, w))
                goto whole;
            if ((apos = strchr(w, '\'')) != NULL && cst_regex_match(en_rx_word, apos + 1)) {
                stem = cst_substr(w, 0, apos - w);
                if (en_in_lex(tok, stem, "0", 0)
                    || (same && cst_regex_match(en_rx_quoted, stem) && !has_one_char_part(stem)
                        && compound_of_function_words(tok, stem))) {
                    cst_free(stem);
                    goto whole;
                }
                cst_free(stem);
            }
            item_set_string(tok, "nsw", "hyphenated-word");
            r = en_split_hyphens(tok, s);
            cst_free(w);
            return r;
        }
    whole:
        r = cons_val(string_val(w), NULL);
        cst_free(w);
        return r;
    }
    if (strchr(s, '-') && strlen(s) > 1) {
        if (*nsw == '\0') {
            item_set_string(tok, "nsw", "hyphenated");
            r = en_split_hyphens(tok, s);
            if (strcmp(item_feat_string(tok, "nsw"), "nide") != 0)
                return r;
            delete_val(r);
        }
        return en_split_hyphens(tok, s);
    }
    if ((q = strrchr(s, '\'')) != NULL && cst_regex_match(en_rx_word, q + 1)) {
        /* "dvd's" */
        stem = cst_substr(s, 0, q - s);
        w = en_fold(q);
        if ((r = en_token_word(tok, stem)) == NULL)
            r = cons_val(string_val(w), NULL);
        else
            cst_val_strappend((cst_val *)val_car(val_last(r)), w);
        cst_free(w);
        cst_free(stem);
        return r;
    }
    n = strlen(s);
    if (n <= 1) {
        if (n != 1 || !strchr("\341\340\342\351\350\352\355\363\364\366\370\301\300\302\311\310"
                              "\312\315\323\324\326\330", s[0]))
            return en_exp_letters(tok, s);
        low = en_lowercase(s);
        if (en_in_lex(tok, low, "0", 1)) {
            r = cons_val(string_val(low), NULL);
            cst_free(low);
            return r;
        }
        cst_free(low);
        w = en_fold(s);
        r = en_exp_letters(tok, w);
        cst_free(w);
        return r;
    }
    /* split where letters meet digits or other characters */
    for (i = 0; s[i]; i++) {
        c = s[i];
        if (strchr(ASCII_LETTERS, c) && strchr(ASCII_LETTERS, s[i + 1]))
            continue;
        if (strchr("0123456789", c) && strchr("0123456789", s[i + 1]))
            continue;
        break;
    }
    d = cst_strdup(s);
    d[i + 1] = '\0';
    right = cst_strdup(s + i + 1);
    item_set_string(tok, "nsw", "nide");
    r = val_append(en_token_word(tok, d), en_token_word(tok, right));
    cst_free(d);
    cst_free(right);
    return r;
}

/* 0x4ad94 arm: a hyphenated token as its parts: split at the first hyphen, or for a run of
 * single letters ("a-b-c") after them */
cst_val *en_split_hyphens(cst_item *tok, const char *s)
{
    const char *h = strchr(s, '-'), *at = h;
    char *d = cst_strdup(s);
    cst_val *r;

    if (s[1] == '-')
        while (h && strlen(h) >= 3 && (h[2] == '-' || h[2] == '\'')) {
            if ((h = strchr(h + 1, '-')) != NULL)
                at = h;
        }
    if (at == NULL)
        r = en_token_word(tok, d);
    else {
        d[strlen(s) - strlen(at)] = '\0';
        r = val_append(en_token_word(tok, d), en_token_word(tok, at + 1));
    }
    cst_free(d);
    return r;
}

typedef struct en_money {
    int sign;
    char *currency;
    char *amount;
    const char *scale;
} en_money;

cst_val *en_token_say_as(cst_item *tok, const char *name);
int token_no_punc_prev(const cst_item *item);
int token_no_punc_next(const cst_item *item);
cst_val *en_exp_time(const cst_item *t, const char *s, int no_oclock);
cst_val *en_exp_utc_offset(const cst_item *t, const char *s);
cst_val *en_exp_date(const cst_item *t, const char *s, const char *order);
cst_val *en_exp_ordinal(const cst_item *t, const char *s);
cst_val *en_exp_currency(const cst_item *t, const char *amount, const char *cur);
cst_val *en_exp_money(const cst_item *t, en_money *m);
int en_parse_money(const char *s, en_money *m);
int en_era_before(const cst_item *t);
int en_era_after(const cst_item *t);
int en_is_era(const char *s);
void en_money_free(en_money *m);

extern const cst_regex *const en_rx_time_colon, *const en_rx_one_two_digits;
extern const cst_regex *const en_rx_utc_offset, *const en_rx_date_month_day;
extern const cst_regex *const en_rx_date_dmy, *const en_rx_date_ymd, *const en_rx_nonzero_int;
extern const cst_regex *const en_rx_number, *const en_rx_commaint, *const en_rx_illion;

/* 0x3c26f8: what makes a time's "o'clock" redundant */
static const char *const en_ampm[] = { "am", "AM", "a.m", "A.M", "pm", "PM", "p.m", "P.M", NULL };
/* 0x3c271c: units after which a four-digit number is not a year */
static const char *const en_year_units[] = {
    "meters", "miles", "feet", "hectares", "square", "cubic", NULL
};

/* 0x4a17c arm: the tokentowords_func: a token's words (Flite's us_tokentowords): a "?^"/"?_"
 * ending sets the end tone; then SSML say-as, times, UTC offsets, dates, years, eras, ordinals,
 * decades, amounts, money and abbreviations; anything else word by word (en_token_word) */
cst_val *en_token_to_words(cst_item *tok)
{
    char *name = (char *)item_feat_string(tok, "name"), *p, *d;
    cst_val *r = NULL, *v;
    en_money m;
    size_t n;
    int k;

    if ((n = strlen(name)) > 2) {
        p = name + n - 2;
        if (strcmp(p, "?^") == 0 || strcmp(p, "?_") == 0) {
            item_set_string(tok, "endtone", strcmp(p, "?^") == 0 ? "H" : "L");
            item_set_string(tok, "punc", "?");
            name[strlen(name) - 2] = '\0';
        }
    }
    if ((r = en_token_say_as(tok, name)) != NULL)
        return r == (cst_val *)-1 ? NULL : r;
    if (strcmp("_a", name) == 0 || strcmp("_A", name) == 0)
        r = cons_val(string_val("_a"), NULL);
    else if (cst_regex_match(en_rx_time_ampm, name)) {
        /* "5pm", "5:30p.m" */
        d = cst_strdup(name);
        n = strlen(name);
        if (n > 3 && en_is_dotted_abbrev(tok, name + n - 3)) {
            d[strlen(name) - 3] = '\0';
            r = val_append(en_exp_time(tok, d, 1), en_token_word(tok, name + strlen(name) - 3));
        } else if ((n = strlen(name)) > 2 && cst_regex_match(cst_rx_alpha, name + n - 2)) {
            d[strlen(name) - 2] = '\0';
            r = val_append(en_exp_time(tok, d, 1), en_spell_dotted(tok, name + strlen(name) - 2));
        }
        cst_free(d);
    } else if (cst_regex_match(en_rx_time_colon, name))
        r = en_exp_time(tok, name,
                        token_no_punc_next(tok)
                        && cst_member_string(ffeature_string(item_traverse(tok, 0x100, 0), "name"),
                                             en_ampm));
    else if ((strcmp(name, "am") == 0 || strcmp(name, "AM") == 0) && token_no_punc_prev(tok)
             && (cst_regex_match(en_rx_time_colon,
                                 ffeature_string(item_traverse(tok, 0x103, 0), "name"))
                 || cst_regex_match(en_rx_one_two_digits,
                                    ffeature_string(item_traverse(tok, 0x103, 0), "name"))))
        r = en_spell_dotted(tok, name);
    else if (cst_regex_match(en_rx_utc_offset, name))
        r = en_exp_utc_offset(tok, name);
    else if (cst_regex_match(en_rx_date_month_day, name) || cst_regex_match(en_rx_date_dmy, name)) {
        if (strcmp(ffeature_string(item_traverse(tok, 0), "language_code"), "en-US") == 0
            && cst_regex_match(en_rx_date_month_day, name))
            r = en_exp_date(tok, name, "mdy");
        else if (cst_regex_match(en_rx_date_dmy, name))
            r = en_exp_date(tok, name, "dmy");
        else
            r = en_exp_date(tok, name, "mdy");
    } else if (cst_regex_match(en_rx_date_ymd, name))
        r = en_exp_date(tok, name, "ymd");
    else {
        if (cst_regex_match(en_rx_nonzero_int, name)) {
            /* years: 1701..1999 unless a unit follows, others only with an era */
            if (strlen(name) == 4 && (unsigned int)(atoi(name) - 1701) <= 298
                && !(token_no_punc_next(tok)
                     && cst_member_string(item_feat_string(item_next(tok), "name"),
                                          en_year_units)))
                goto year;
            if (en_era_before(tok) || en_era_after(tok)) {
            year:
                if ((n = strlen(name)) <= 4) {
                    r = n == 4 ? en_exp_date(tok, name, "y") : en_exp_id(tok, name);
                    goto done;
                }
            }
        }
        if (en_is_era(name)
            && ((token_no_punc_prev(tok)
                 && cst_regex_match(en_rx_nonzero_int,
                                    ffeature_string(item_traverse(tok, 0x103, 0), "name")))
                || (token_no_punc_next(tok)
                    && cst_regex_match(en_rx_nonzero_int,
                                       ffeature_string(item_traverse(tok, 0x100, 0), "name")))))
            r = en_spell_dotted(tok, name);
        else if (cst_regex_match(en_rx_number, name)) {
            /* ordinals: "1st", "2nd's", "3rds" */
            n = strlen(name);
            if (name[n - 2] == '\'')
                k = 2;
            else if (strchr("sS", name[n - 1]))
                k = 1;
            else {
                d = cst_substr(name, 0, n - 2);
                r = en_exp_ordinal(tok, d);
                cst_free(d);
                goto done;
            }
            d = cst_substr(name, 0, n - 2 - k);
            r = en_exp_ordinal(tok, d);
            cst_free(d);
            cst_val_strappend((cst_val *)val_car(val_last(r)), &"'s"[2 - k]);
        } else if (cst_regex_match(en_rx_commaint, name)) {
            /* "1990s" */
            d = cst_strdup(name);
            d[strlen(name) - 1] = '\0';
            r = en_exp_number(tok, d);
            cst_val_strappend((cst_val *)val_car(val_last(r)), "'s");
            cst_free(d);
        } else if (strcmp(name, "00s") == 0 || strcmp(name, "00's") == 0) {
            if (is_british(tok))
                r = cons_val(string_val("noughties"), NULL);
            else
                r = cons_val(string_val("oh"), cons_val(string_val("oh's"), NULL));
        } else {
            if (cst_regex_match(en_rx_illion, name) && token_no_punc_prev(tok)
                && en_parse_money(ffeature_string(item_traverse(tok, 0x103, 0), "name"), &m)) {
                /* "$5 million" */
                if (m.scale == NULL) {
                    d = en_fold(name);
                    r = cons_val(string_val(d), en_exp_currency(tok, NULL, m.currency));
                    cst_free(d);
                }
                en_money_free(&m);
            } else if (en_parse_money(name, &m)) {
                if (m.scale == NULL && token_no_punc_next(tok)
                    && cst_regex_match(en_rx_illion,
                                       ffeature_string(item_traverse(tok, 0x100, 0), "name")))
                    r = val_append(en_exp_sign(tok, (signed char)m.sign), en_exp_real(tok, m.amount));
                else
                    r = en_exp_money(tok, &m);
                en_money_free(&m);
            } else if (strcmp("Etc", name) == 0 || strcmp("etc", name) == 0
                       || strcmp("Mr", name) == 0 || strcmp("Mrs", name) == 0
                       || (strcmp("Ms", name) == 0
                           && ffeature_string(item_traverse(tok, 0), "punc")[0] == '.')) {
                d = en_lowercase(name);
                r = cons_val(string_val(d), NULL);
                cst_free(d);
                en_take_abbrev_stop(tok, name);
            }
        }
    }
done:
    if (en_is_initial(tok))
        feat_set_int(item_feats(tok), "spelling_break", 1);
    if (r)
        return r;
    (void)v;
    return en_token_word(tok, name);
}

/* 0x4ae84 arm: the sentence_boundary_func: 1 when the token's punctuation ends a sentence
 * (an abbreviation's own full stop aside), -1 for an initial, else 0 */
cst_val *en_sentence_boundary(cst_item *tok)
{
    const char *name = item_feat_string(tok, "name");
    const char *punc = item_feat_string(tok, "punc");

    if (cst_regex_match(en_rx_time_ampm, name) || strcmp("Etc", name) == 0
        || strcmp("etc", name) == 0 || strcmp("Mr", name) == 0 || strcmp("Mrs", name) == 0
        || (strcmp("Ms", name) == 0 && ffeature_string(item_traverse(tok, 0), "punc")[0] == '.')
        || cst_regex_match(en_rx_dotted_letters, name)) {
        if (punc[0] == '.')
            punc++;
    }
    if (en_is_initial(tok))
        return int_val(-1);
    if (punc[0] && strchr(".!?", punc[strlen(punc) - 1]))
        return int_val(1);
    return int_val(0);
}
