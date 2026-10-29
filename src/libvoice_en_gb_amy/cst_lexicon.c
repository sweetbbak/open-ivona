#include "cst_alloc.h"
#include "cst_error.h"
#include "cst_val.h"
#include "cst_val_defs.h"
#include "cst_lexicon.h"
#include "cst_lts.h"

static inline int lex_uncompress_word(char *ucword, int max_size, int p, const cst_lexicon *l)
{
    int i, j = 0;
    unsigned char *cword;

    if (l->entry_hufftable == NULL) {
        snprintf(ucword, max_size, "%s", (char *)&l->data[p]);
    } else {
        cword = &l->data[p];
        for (i = 0; cword[i]; i++) {
            const char *h = l->entry_hufftable[cword[i]];
            int length = strlen(h);
            if (j + length + 1 < max_size) {
                memmove(ucword + j, h, length);
                j += length;
            } else
                break;
        }
        ucword[j] = '\0';
    }
    return j;
}

static inline int lex_data_prev_entry(const cst_lexicon *l, int p, int start)
{
    for (p--; p > start; p--)
        if (l->data[p - 1] == 255)
            return p;
    return start;
}

static inline int lex_data_next_entry(const cst_lexicon *l, int p, int end)
{
    for (p++; p < end; p++)
        if (l->data[p - 1] == 255)
            return p;
    return end;
}

static inline int lex_data_closest_entry(const cst_lexicon *l, int p, int start, int end)
{
    int d = 0;
    while ((p - d > start) && (p + d < end)) {
        if (l->data[(p + d) - 1] == 255)
            return p + d;
        else if (l->data[(p - d) - 1] == 255)
            return p - d;
        d++;
    }
    return p - d;
}

static inline int find_full_match(const cst_lexicon *l, int i, const char *word)
{
    int w, match = i;
    char word_pos[64];

    for (w = i; w > 0; ) {
        lex_uncompress_word(word_pos, 64, w, l);
        if (strcmp(word + 1, word_pos + 1) != 0)
            break;
        else if (strcmp(word, word_pos) == 0)
            return w;
        match = w;
        w = lex_data_prev_entry(l, w, 0);
    }

    for (w = i; w < l->num_bytes; ) {
        lex_uncompress_word(word_pos, 64, w, l);
        if (strcmp(word + 1, word_pos + 1) != 0)
            break;
        else if (strcmp(word, word_pos) == 0)
            return w;
        w = lex_data_next_entry(l, w, l->num_bytes);
    }

    return match;
}

/* 0x53a4c: arm */
int lex_lookup_bsearch(const cst_lexicon *l, const char *word)
{
    int start, mid, end, c;
    char word_pos[64];

    start = 0;
    end = l->num_bytes;
    while (start < end) {
        mid = (start + end) / 2;
        mid = lex_data_closest_entry(l, mid, start, end);
        lex_uncompress_word(word_pos, 64, mid, l);

        c = strcmp(word_pos + 1, word + 1);
        if (c == 0)
            return find_full_match(l, mid, word);
        else if (c > 0)
            end = mid;
        else
            start = lex_data_next_entry(l, mid + 1, end);
    }
    return -1;
}

extern const int cst_val_type_lexicon;
CST_VAL_REGISTER_TYPE_NODEL(lexicon, cst_lexicon)

typedef void *(*allwordposfunc)(const char *word);
extern const int cst_val_type_allwordposfunc;
CST_VAL_REGISTER_FUNCPTR(allwordposfunc, allwordposfunc)

/* 0x54008: arm */
cst_lexicon *new_lexicon(void)
{
    return cst_alloc(cst_lexicon, 1);
}

/* 0x54014: arm */
void delete_lexicon(cst_lexicon *lex)
{
    if (lex)
        cst_free(lex);
}

int FUN_00063d5c(void *d, const char *word, const char *pos);
char *cst_strcat(const char *a, const char *b);
/* 0x54020: arm */
int in_lex(const cst_lexicon *l, const char *word, const char *pos)
{
    char word_pos[64];
    char *wp;
    int index;
    int r = 1;

    if (!l->addenda || !FUN_00063d5c(l->addenda, word, pos)) {
        char *(*word_norm)(const char *) = l->word_norm;
        (word_pos[0] = pos ? pos[0] : '0', word_pos[1] = '\0');

        if (word_norm) {
            char *w = l->word_norm(word);
            wp = cst_strcat(word_pos, w);
            cst_free(w);
        } else {
            wp = cst_strcat(word_pos, word);
        }

        index = lex_lookup_bsearch(l, wp);
        if (index < 0) {
            r = 0;
        } else {
            lex_uncompress_word(word_pos, 64, index, l);
            if (wp[0] == '0')
                r = 1;
            else
                r = (word_pos[0] == wp[0]);
        }
        cst_free(wp);
    }
    return r;
}

int FUN_00063d70(void *addenda, const char *word, char *buf, size_t max_len);

/* 0x541a0: arm */
int lex_word_pos_list(const cst_lexicon *l, const char *word, char *buf, size_t max_len)
{
    size_t count;
    int index;
    int w;
    char word_pos[64];
    char *wp;
    const char *target_word;

    count = FUN_00063d70(l->addenda, word, buf, max_len);

    if (buf && memchr(buf, '*', count < max_len ? count : max_len))
        return count;

    word_pos[0] = '0';
    word_pos[1] = '\0';
    if (l->word_norm) {
        char *w = l->word_norm(word);
        wp = cst_strcat(word_pos, w);
        cst_free(w);
    } else {
        wp = cst_strcat(word_pos, word);
    }

    index = lex_lookup_bsearch(l, wp);
    if (index >= 0) {
        target_word = wp + 1;
        for (w = index; w > 0; ) {
            int prev = lex_data_prev_entry(l, w, 0);
            if (prev == 0)
                break;
            lex_uncompress_word(word_pos, 64, prev, l);
            if (strcmp(target_word, word_pos + 1) != 0)
                break;
            w = prev;
        }

        for (; w < l->num_bytes; ) {
            lex_uncompress_word(word_pos, 64, w, l);
            if (strcmp(target_word, word_pos + 1) != 0)
                break;
            if (count < max_len)
                buf[count] = word_pos[0];
            count++;
            w = lex_data_next_entry(l, w, l->num_bytes);
        }
    }

    cst_free(wp);
    return count;
}

/* 0x544f0: arm */
int lex_select_pos(const cst_lexicon *l, const char *word, const char *s)
{
    char buf[33];
    int n;

    n = lex_word_pos_list(l, word, buf, 33);
    if (n == 0)
        return -1;
    buf[n] = '\0';
    return (signed char)s[strcspn(s, buf)];
}

cst_val *FUN_00063e8c(void *addenda, const char *word, const char *pos);

/* 0x54544: arm */
cst_val *lex_lookup(const cst_lexicon *l, const char *word, const char *pos)
{
    char word_pos[4];
    char *wp;
    cst_val *phones = 0;
    int index;
    int p;

    word_pos[0] = pos ? pos[0] : '0';
    word_pos[1] = '\0';
    if (l->word_norm) {
        char *w = l->word_norm(word);
        wp = cst_strcat(word_pos, w);
        cst_free(w);
    } else {
        wp = cst_strcat(word_pos, word);
    }

    if (l->addenda && (phones = FUN_00063e8c(l->addenda, word, pos))) {
        cst_free(wp);
        return phones;
    }

    index = lex_lookup_bsearch(l, wp);
    if (index >= 0) {
        if (l->phone_hufftable) {
            for (p = index - 2; l->data[p]; p--) {
                const unsigned char *h = ((const unsigned char * const *)l->phone_hufftable)[l->data[p]];
                cst_val *new_phones = 0;
                for (; *h; h++)
                    new_phones = val_append(new_phones, cons_val(string_val(l->phone_table[*h]), 0));
                phones = val_append(new_phones, phones);
            }
        } else {
            for (p = index - 2; l->data[p]; p--)
                phones = cons_val(string_val(l->phone_table[l->data[p]]), phones);
        }
    } else if (l->lts_rule_set) {
        const char *w = wp + 1;
        if (l->lts_norm)
            w = l->lts_norm(w);
        phones = lts_apply(w, "", l->lts_rule_set);
        if (l->lts_norm)
            cst_free((void *)w);
    }

    cst_free(wp);
    return phones;
}
