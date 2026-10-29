/*
 * English word pronunciation (0x3c0d0..): the lexicon of the item's language (British for en-AU,
 * en-GB* and en-IN, else American), and the rules for words it lacks: "-in'" for "-ing",
 * "rock'n'roll", clitics ("'s", "n't", ...) whose pronunciation follows the stem's last phone,
 * hyphens, apostrophes, compounds of street names, and spelling.
 */
#include <string.h>
#include "cst_alloc.h"
#include "cst_ffeatures.h"
#include "cst_item.h"
#include "cst_phoneset.h"
#include "cst_regex.h"
#include "cst_string.h"
#include "cst_val.h"

cst_item *item_traverse(const cst_item *item, ...);
int en_gb_in_lex(const char *word, const char *pos, int main_only);
int en_us_in_lex(const char *word, const char *pos, int main_only);
cst_val *en_gb_lex_lookup(const char *word, const char *pos, float *score);
cst_val *en_us_lex_lookup(const char *word, const char *pos, float *score);
int en_gb_lex_select_pos(const char *word, const char *s);
int en_us_lex_select_pos(const char *word, const char *s);
int en_gb_lex_pos_list(const char *word, char *buf, size_t max_len);
int en_us_lex_pos_list(const char *word, char *buf, size_t max_len);
const char *phone_name(const cst_val *p);
const char *phone_feature_base(const cst_phoneset *ps, const cst_val *p, const char *feat);
int ph_is_vowel(const cst_phoneset *ps, const cst_val *p);
cst_val *phones_from_string(const char *s);
const cst_val *val_last_n(const cst_val *l, unsigned int n);
const cst_val *val_last(const cst_val *v);
const char *cst_find_suffix(const char *s, const char *const *slist);
char *cst_strtr(const char *s, const char *from, const char *to);

extern const cst_regex *const en_rx_has_vowel;
extern const cst_regex *const en_rx_spelled;

/* ^[a-z]+'n'[a-z]+$ */
static const unsigned char en_rx_n_conj_rxprog[] = {  /* 0x131544 */
    156, 6, 0, 82, 1, 0, 3, 11, 0, 33, 4, 0, 0, 97, 98, 99,
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
    116, 117, 118, 119, 120, 121, 122, 0, 8, 0, 7, 39, 110, 39, 0, 11,
    0, 33, 4, 0, 0, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
    108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 0,
    2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_n_conj_rx = {  /* 0x3c0ed8 */
   0, 1, NULL, 0, 86,
   (char *)en_rx_n_conj_rxprog
};

/* clitics, whose pronunciation (part of speech "0", "1" or "2") follows the stem */
static const char *const en_clitics[] = { "'s", "'d", "'ll", "'ve", "'tve", "n't", "'re", NULL };

/* the last parts of compound street names */
static const char *const en_street_suffixes[] = {
    "castle", "heather", "park", "vale", "spring", "circle", "court", "cove", "drive", "garth",
    "green", "knoll", "lawn", "loop", "manor", "nook", "place", "spur", "square", "terrace",
    "walk", NULL
};

static const char *const en_compound_suffixes[] = { "oak", NULL };

/* the British lexicon serves en-AU, en-GB* and en-IN */
static inline int is_british(const cst_item *item)
{
    return strcmp(ffeature_string(item_traverse(item, 0), "language_code"), "en-AU") == 0
           || cst_startswith(ffeature_string(item_traverse(item, 0), "language_code"), "en-GB")
           || strcmp(ffeature_string(item_traverse(item, 0), "language_code"), "en-IN") == 0;
}

/* a syllable boundary in front of phones that hold a syllabic one */
static inline cst_val *syllable_start(cst_val *phones)
{
    const cst_val *p;
    const char *s;

    for (p = phones; p; p = val_cdr(p)) {
        s = val_string(val_car(p));
        if (strchr("aeiou", s[0]) || (strchr("lmn", s[0]) && s[1] == 'x'))
            return val_append(cons_val(string_val("-"), NULL), phones);
    }
    return phones;
}

/* primary stresses become secondary */
static inline void demote_stress(const cst_val *phones)
{
    const cst_val *p;
    char *s;
    size_t n;

    for (p = phones; p; p = val_cdr(p)) {
        s = (char *)val_string(val_car(p));
        if (*s && s[(n = strlen(s)) - 1] == '1')
            s[n - 1] = '2';
    }
}

/* without a final "r" (a non-rhotic accent) */
static inline void drop_final_r(cst_val *phones)
{
    cst_val *p, *last;

    if (val_cdr(phones) == NULL)
        return;
    for (p = phones; val_cdr(val_cdr(p)); p = (cst_val *)val_cdr(p))
        ;
    last = (cst_val *)val_cdr(p);
    if (strcmp(phone_name(last), "r") == 0) {
        set_cdr(p, NULL);
        delete_val(last);
    }
}

/* 0x3c0d0 arm: whether word (as pos) is in the item's lexicon */
int en_in_lex(const cst_item *item, const char *word, const char *pos, int main_only)
{
    if (!is_british(item))
        return en_us_in_lex(word, pos, main_only);
    return en_gb_in_lex(word, pos, main_only);
}

/* 0x3c194 arm: word's pronunciation from the item's lexicon or letter-to-sound rules */
cst_val *en_lex_lookup(const cst_item *item, const char *word, const char *pos, float *score)
{
    if (is_british(item))
        return en_gb_lex_lookup(word, pos, score);
    return en_us_lex_lookup(word, pos, score);
}

/* 0x3c258 arm: the first of the parts of speech in s that word has, or -1 */
int en_lex_select_pos(const cst_item *item, const char *word, const char *s)
{
    if (is_british(item))
        return en_gb_lex_select_pos(word, s);
    return en_us_lex_select_pos(word, s);
}

/* 0x3c310 arm: the parts of speech of the item's word */
int en_lex_pos_list(const cst_item *item, char *buf, size_t max_len)
{
    if (is_british(item))
        return en_gb_lex_pos_list(item_feat_string(item, "name"), buf, max_len);
    return en_us_lex_pos_list(item_feat_string(item, "name"), buf, max_len);
}

/* 0x3c3f0 arm: a then b, a syllable boundary between them; in British English a's final "r"
 * goes before a consonant */
cst_val *en_join_pron(const cst_item *item, cst_val *a, cst_val *b)
{
    if (a == NULL)
        return b;
    if (b == NULL)
        return a;
    if (is_british(item) && !ph_is_vowel(item_phoneset(item), b))
        drop_final_r(a);
    return val_append(a, cons_val(string_val("-"), b));
}

/* 0x3c558 arm: the word spelt letter by letter ("-" and "." skipped); the letters between the
 * first and the last lose their primary stress */
cst_val *en_spell_word(const cst_item *item, const char *word)
{
    cst_val *acc = NULL, *prev = NULL, *p;
    char letter[2];

    letter[1] = '\0';
    for (; *word; word++) {
        if ((unsigned char)(*word - '-') <= 1)
            continue;
        letter[0] = *word;
        p = en_lex_lookup(item, letter, "n", NULL);
        if (acc == NULL)
            acc = p;
        else {
            if (prev) {
                demote_stress(prev);
                acc = en_join_pron(item, acc, prev);
            }
            prev = p;
        }
    }
    if (prev)
        acc = en_join_pron(item, acc, prev);
    return acc;
}

/* 0x3c670 arm: the pronunciation of word as pos; *score (if given) is 0 when it is spelt, as
 * en_lex_lookup's otherwise */
cst_val *en_word_pron(const cst_item *item, const char *word, const char *pos, float *score)
{
    const cst_phoneset *ps;
    const cst_val *last;
    const char *suffix, *variant;
    cst_val *a, *b, *phones;
    char *stem, *s;
    float local, s1, s2;
    size_t n;

    if (score == NULL)
        score = &local;
    if (en_in_lex(item, word, pos, 0))
        return en_lex_lookup(item, word, pos, score);
    if (pos && strcmp(pos, "0") != 0 && en_in_lex(item, word, "0", 0))
        return en_lex_lookup(item, word, pos, score);
    /* "nothin'" */
    if (cst_endswith(word, "in'")) {
        stem = cst_strdup(word);
        stem[strlen(word) - 1] = '\0';
        if (!en_in_lex(item, stem, "0", 0)) {
            stem[strlen(word) - 1] = 'g';
            if (en_in_lex(item, stem, "0", 0)) {
                phones = en_lex_lookup(item, stem, pos, score);
                if (val_length(phones) > 1) {
                    last = val_last_n(phones, 2);
                    if (strcmp(phone_name(last), "ih") == 0) {
                        s = (char *)phone_name(val_cdr(last));
                        if (strcmp(s, "ng") == 0)
                            s[1] = '\0';
                    }
                }
                cst_free(stem);
                return phones;
            }
        }
        cst_free(stem);
    }
    /* "rock'n'roll" */
    if (cst_regex_match(&en_rx_n_conj_rx, word)) {
        stem = cst_string_before(word, "'n'");
        a = en_word_pron(item, stem, "n", &s1);
        b = en_lex_lookup(item, "'n'", NULL, NULL);
        phones = en_word_pron(item, word + strlen(stem) + 3, "n", &s2);
        if (phones)
            phones = syllable_start(phones);
        phones = val_append(b, phones);
        if (phones)
            phones = syllable_start(phones);
        phones = val_append(a, phones);
        cst_free(stem);
        *score = s1 > s2 ? s1 : s2;
        return phones;
    }
    /* clitics */
    if ((suffix = cst_find_suffix(word, en_clitics)) != NULL) {
        stem = cst_substr(word, 0, strlen(word) - strlen(suffix));
        a = en_word_pron(item, stem, pos, score);
        ps = item_phoneset(item);
        if (a == NULL) {
            phones = en_lex_lookup(item, suffix, "0", NULL);
            cst_free(stem);
            return phones;
        }
        last = val_last(a);
        if (strcmp("'s", suffix) == 0) {
            if (strchr("af", phone_feature_base(ps, last, "ctype")[0])
                && strchr("as", phone_feature_base(ps, last, "cplace")[0]))
                variant = "1";
            else if (strcmp("-", phone_feature_base(ps, last, "cvox")) == 0)
                variant = "2";
            else
                variant = "0";
        } else if (strcmp("'d", suffix) == 0) {
            if (strcmp("t", phone_name(last)) == 0)
                variant = "1";
            else if (strcmp("-", phone_feature_base(ps, last, "cvox")) == 0)
                variant = "2";
            else
                variant = "0";
        } else if ((strcmp("n't", suffix) == 0 || strcmp("'ll", suffix) == 0
                    || strcmp("'ve", suffix) == 0 || strcmp("'tve", suffix) == 0)
                   && !ph_is_vowel(ps, last) && strcmp("r", phone_name(last)) != 0)
            variant = "1";
        else
            variant = "0";
        if (is_british(item))
            drop_final_r(a);
        b = en_lex_lookup(item, suffix, variant, NULL);
        if (b)
            b = syllable_start(b);
        phones = val_append(a, b);
        cst_free(stem);
        return phones;
    }
    /* hyphenated words that are not spelt letters */
    if (strchr(word, '-') && !cst_regex_match(en_rx_spelled, word)) {
        s = cst_strtr(word, "-", NULL);
        phones = en_word_pron(item, s, pos, score);
        cst_free(s);
        return phones;
    }
    if ((suffix = strrchr(word, '\'')) != NULL) {
        stem = cst_substr(word, 0, strlen(word) - strlen(suffix));
        if (strlen(suffix) == 1)
            phones = en_word_pron(item, stem, pos, score);
        else if (en_in_lex(item, suffix, NULL, 0)) {
            a = en_word_pron(item, stem, pos, score);
            b = en_lex_lookup(item, suffix, NULL, NULL);
            if (b)
                b = syllable_start(b);
            phones = val_append(a, b);
        } else if (!cst_regex_match(en_rx_has_vowel, word)) {
            phones = en_spell_word(item, suffix + 1);
            a = en_word_pron(item, stem, NULL, score);
            if (a) {
                if (phones)
                    phones = syllable_start(phones);
                phones = val_append(a, phones);
            }
        } else if (word[0] == '\'')
            phones = en_lex_lookup(item, word + 1, pos, score);
        else if (cst_startswith(word, "o'") && cst_regex_match(cst_rx_alpha, word + 2)
                 && cst_regex_match(en_rx_has_vowel, word + 2)) {
            a = phones_from_string("ow|");
            b = en_lex_lookup(item, word + 2, pos, score);
            if (b)
                b = syllable_start(b);
            phones = val_append(a, b);
        } else if (cst_startswith(word, "c'") && cst_regex_match(cst_rx_alpha, word + 2)
                   && cst_regex_match(en_rx_has_vowel, word + 2)) {
            a = phones_from_string("k|ax|");
            b = en_lex_lookup(item, word + 2, pos, score);
            if (b)
                b = syllable_start(b);
            phones = val_append(a, b);
        } else if (cst_startswith(word, "d'") && cst_regex_match(cst_rx_alpha, word + 2)
                   && cst_regex_match(en_rx_has_vowel, word + 2)) {
            a = phones_from_string("d|");
            phones = val_append(a, en_lex_lookup(item, word + 2, pos, score));
        } else if (cst_startswith(word, "y'") && cst_regex_match(cst_rx_alpha, word + 2)
                   && cst_regex_match(en_rx_has_vowel, word + 2)) {
            a = phones_from_string("j|ax|");
            b = en_lex_lookup(item, word + 2, pos, score);
            if (b)
                b = syllable_start(b);
            phones = val_append(a, b);
        } else
            phones = en_lex_lookup(item, word, pos, score);
        cst_free(stem);
        return phones;
    }
    /* compound street names: the second part's stress is secondary */
    if ((suffix = cst_find_suffix(word, en_street_suffixes)) != NULL) {
        if ((n = suffix - word) <= 2)
            return en_lex_lookup(item, word, pos, score);
        stem = cst_substr(word, 0, n);
        if (!cst_regex_match(en_rx_has_vowel, stem))
            phones = en_lex_lookup(item, word, pos, score);
        else {
            a = en_word_pron(item, stem, pos, &s1);
            b = en_word_pron(item, suffix, pos, &s2);
            if (b)
                demote_stress(b);
            phones = en_join_pron(item, a, b);
            *score = s1 > s2 ? s1 : s2;
        }
        cst_free(stem);
        return phones;
    }
    if ((suffix = cst_find_suffix(word, en_compound_suffixes)) != NULL) {
        if ((n = suffix - word) <= 2)
            return en_lex_lookup(item, word, pos, score);
        stem = cst_substr(word, 0, n);
        if (!cst_regex_match(en_rx_has_vowel, stem))
            phones = en_lex_lookup(item, word, pos, score);
        else {
            a = en_word_pron(item, stem, pos, score);
            phones = en_join_pron(item, a, en_word_pron(item, suffix, pos, NULL));
        }
        cst_free(stem);
        return phones;
    }
    if (strchr(word, '.') || strchr(word, '-') || !cst_regex_match(en_rx_has_vowel, word)) {
        *score = 0;
        return en_spell_word(item, word);
    }
    return en_lex_lookup(item, word, pos, score);
}

const char *en_word_pos(const cst_item *word);
const char *word_role_pos(const cst_item *word, const char *(*pos_func)(const cst_item *));
cst_val *en_syllabify(const cst_item *item, cst_val *phones);

/* 0x3d7ec arm: the wordtosegments_func: the word's part of speech (its token's role, else
 * en_word_pos), recorded as "pos" (or, for "0", the lexicon's single part of speech), and its
 * syllabified pronunciation; "psa-score" when the pronunciation was guessed */
cst_val *en_word_to_segments(cst_item *word)
{
    char pos1[2] = "";
    float score = -1;
    const char *pos = word_role_pos(word, en_word_pos);
    cst_val *phones;

    if (strcmp(pos, "0") == 0 && en_lex_pos_list(word, pos1, 1)) {
        pos1[1] = '\0';
        item_set_string(word, "pos", pos1);
    } else
        item_set_string(word, "pos", pos);
    phones = en_syllabify(word, en_word_pron(word, item_feat_string(word, "name"), pos, &score));
    if (score >= 0)
        item_set_float(word, "psa-score", score);
    return phones;
}
