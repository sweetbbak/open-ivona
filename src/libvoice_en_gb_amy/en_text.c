#include <string.h>
#include <ctype.h>
#include "cst_alloc.h"
#include "cst_item.h"
#include "cst_ffeatures.h"
#include "cst_string.h"
#include "cst_val.h"
#include "cst_regex.h"

cst_val *cst_split_val(const char *s, const char *delims);
int pcre_match(const void *table, const char *str);

/* 0x603dc arm */
void en_text_init(void)
{
}

/* 0x603e0 arm */
void en_text_deinit(void)
{
    en_text_init();
}

cst_item *item_as(const cst_item *i, int id);
cst_item *item_traverse(const cst_item *item, ...);
const char *utf8_last_char(const char *s);

/* 0x603e4 arm */
const char *punc_last_char(const cst_item *item)
{
    const char *s = ffeature_string(item_traverse(item_as(item, 3), 0x108, 1, 0x106, 0), "punc");

    if (*s != '\0')
        return s + strlen(s) - 1;
    return s;
}

/* 0x60448 arm */
const char *punc_last_utf8_char(const cst_item *item)
{
    const char *s = ffeature_string(item_traverse(item_as(item, 3), 0x108, 1, 0x106, 0), "punc");

    if (*s != '\0')
        return utf8_last_char(s);
    return s;
}

/* 0x604a4 arm */
int item_is_phrase_break(const cst_item *item, const char *punc_break)
{
    if (strcmp(item_feat_string(item, "name"), "BB") == 0)
        return 1;
    if (punc_break != NULL)
        return strchr(punc_break, *(const unsigned char *)punc_last_char(item)) != NULL;
    return 0;
}

/* 0x60510 arm */
cst_item *phrase_start_token(const cst_item *item, const char *punc_break)
{
    cst_item *tok = item_as(item, 3);

    if (tok == NULL)
        return NULL;
    while (1) {
        if (!item_prev(tok))
            return tok;
        if (item_is_phrase_break(item_prev(tok), punc_break))
            return tok;
        tok = item_prev(tok);
        if (!tok)
            return NULL;
    }
}


/* 0x60574 arm */
cst_item *phrase_end_token(const cst_item *item, const char *punc_break)
{
    cst_item *tok;

    for (tok = item_as(item, 3); tok; tok = item_next(tok)) {
        if (item_is_phrase_break(tok, punc_break))
            return tok;
    }
    return NULL;
}

/* 0x257c0 arm: Test whether the phrase contains a word from a NULL-terminated list.
 * Descriptive name; the original symbol is unknown. */
__attribute__((cold))
int phrase_contains_word(const cst_item *item, const char * const *words)
{
    cst_item *start = phrase_start_token(item, NULL);
    cst_item *end = item_next(phrase_end_token(item, NULL));
    cst_item *word = item_as(item_daughter(start), 2);

    for (; word && item_parent(item_as(word, 3)) != end; word = item_next(word)) {
        if (cst_member_string(ffeature_string(item_traverse(word, 0), "name"), words))
            return 1;
    }
    return 0;
}

/* 0x605c4 arm: Lowercase ASCII and transliterate high bytes through caller tables. */
char *str_transliterate(const char *str, const char *upper, const char *lower,
                        const char *single, const char * const *multiple)
{
    unsigned int size = strlen(str) + 1;
    char *out = cst_alloc(char, size);
    unsigned int single_len = strlen(single);
    int pos = 0;
    unsigned char c;

    while (*str != '\0') {
        c = *str;
        if (c >= 'A' && c <= 'Z')
            out[pos++] = c + ('a' - 'A');
        else if (!(c & 0x80))
            out[pos++] = c;
        else {
            const char *p = strchr(upper, c);
            unsigned char mapped = c;
            if (p)
                mapped = lower[p - upper];
            p = strchr(lower, mapped);
            if (!p)
                out[pos++] = c;
            else {
                unsigned int index = p - lower;
                if (index >= single_len) {
                    int multi_index = index - single_len;
                    size--;
                    int len = strlen(multiple[multi_index]);
                    size += len;
                    out = cst_realloc(out, char, size);
                    char *dest = out + pos;
                    pos += len;
                    memcpy(dest, multiple[multi_index], len);
                } else
                    out[pos++] = single[index];
            }
        }
        str++;
    }
    out[pos] = '\0';
    return out;
}

typedef struct {
    char *day;
    char *month;
    char *year;
} cst_date;

/* 0x60718 arm: Parse date fields selected by a format and month recognizer. */
void date_parse(cst_date *d, const char *str, const char *format,
                const cst_regex *month_regex, const void *month_table)
{
    cst_val *val = cst_split_val(str, "./-");
    int i = 0;
    int numeric_month = (month_regex == NULL && month_table == NULL);
    cst_val *v;

    d->year = NULL;
    d->month = NULL;
    d->day = NULL;

    for (v = val; i < strlen(format) && v != NULL; v = val_cdr(v), i++)
    {
        const char *s = val_string(val_car(v));
        char **target;
        if (format[i] == 'm' &&
            (numeric_month ||
             (atoi(s) > 0 && atoi(s) <= 12) ||
             (month_regex != NULL ? cst_regex_match(month_regex, s) : pcre_match(month_table, s))))
        {
            target = &d->month;
            goto set_field;
        }
        if (format[i] == 'd' && atoi(s) > 0 && atoi(s) <= 31)
        {
            target = &d->day;
            goto set_field;
        }
        if (format[i] == 'y')
        {
            target = &d->year;
            goto set_field;
        }
        continue;
set_field:
        if (*target == NULL)
            *target = cst_strdup(s);
    }

    delete_val(val);
}

/* 0x608b4 arm */
void date_free(cst_date *d)
{
    if (d == NULL)
        return;
    cst_free(d->day);
    cst_free(d->month);
    cst_free(d->year);
    d->year = NULL;
    d->month = NULL;
    d->day = NULL;
}

typedef struct {
    char *f0;
    char *f1;
    char *f2;
    char *f3;
    char *f4;
    char *f5;
    char *f6;
} cst_time;

/* 0x608ec arm */
void time_parse_quotes(cst_time *t, const char *str)
{
    cst_val *v = cst_split_val(str, "'\"");
    unsigned int len = val_length(v);

    t->f4 = NULL;
    t->f3 = NULL;
    t->f2 = NULL;
    t->f1 = NULL;
    t->f0 = NULL;

    if (len != 0)
        t->f5 = cst_strdup(val_string(val_car(v)));
    else
        t->f5 = cst_strdup("0");

    if (len > 1)
        t->f6 = cst_strdup(val_string(val_car(val_cdr(v))));
    else
        t->f6 = cst_strdup("0");

    delete_val(v);
}
#include "cst_error.h"
#include <strings.h>

/* 0x609a4 arm: Validate ordered ISO 8601 date and time duration components. */
int iso8601_duration_validate(const char *str)
{
    const char *p;
    unsigned int previous = 7;
    int time = 0;

    if (*str != 'P')
        return 0;
    p = str + 1;
    while (*p) {
        unsigned int len = strspn(p, "0123456789");
        if (p[len] == '.' || p[len] == ',') {
            len++;
            unsigned int fraction = strspn(p + len, "0123456789");
            if (fraction == 0)
                return 0;
            len = fraction + len;
        }
        if (len == 0) {
            if (time)
                return 0;
            if (*p != 'T')
                return 0;
            time = 1;
        } else {
            unsigned int component;
            int date_only, time_only;
            switch (p[len]) {
            case 'Y': component = 6; date_only = 1; time_only = 0; break;
            case 'M':
                if (time) { component = 1; date_only = 0; time_only = 1; }
                else { component = 5; date_only = 1; time_only = 0; }
                break;
            case 'W': component = 4; date_only = 1; time_only = 0; break;
            case 'D': component = 3; date_only = 1; time_only = 0; break;
            case 'H': component = 2; date_only = 0; time_only = 1; break;
            case 'S': component = 0; date_only = 0; time_only = 1; break;
            default: return 0;
            }
            if (component >= previous || (date_only && time) || (time_only && !time))
                return 0;
            previous = component;
        }
        p += len + 1;
    }
    if (time)
        return previous <= 2;
    return previous <= 6;
}

/* 0x60b84 arm: Extract ISO 8601 duration fields, normalizing decimal separators. */
void iso8601_duration_parse(cst_time *t, const char *str, char decimal_point)
{
    const char *p;
    int is_time = 0;
    int len;

    t->f6 = NULL;
    t->f5 = NULL;
    t->f4 = NULL;
    t->f3 = NULL;
    t->f2 = NULL;
    t->f1 = NULL;
    t->f0 = NULL;

    for (p = str + 1; *p != '\0'; p += len + 1)
    {
        len = strspn(p, "0123456789,.");
        if (len == 0)
        {
            is_time = 1;
            continue;
        }

        char *sub = cst_substr(p, 0, len);
        if (decimal_point != '\0')
        {
            cst_strsubchar(sub, '.', decimal_point);
            cst_strsubchar(sub, ',', decimal_point);
        }

        switch (p[len])
        {
        case 'Y':
            t->f0 = sub;
            break;
        case 'M':
            if (is_time)
                t->f5 = sub;
            else
                t->f1 = sub;
            break;
        case 'W':
            t->f2 = sub;
            break;
        case 'D':
            t->f3 = sub;
            break;
        case 'H':
            t->f4 = sub;
            break;
        case 'S':
            t->f6 = sub;
            break;
        default:
            cst_errmsg("Invalid format of ISO8601 duration");
            return;
        }

    }
}

static inline int clock_duration_fields(const char *str)
{
    unsigned int count = 0;
    while (1) {
        unsigned int span = strspn(str, "0123456789");
        if (span == 0)
            return -1;
        switch (str[span]) {
        case '\0': return count + 1;
        case ':': str += span + 1; count++; break;
        default: return -1;
        }
    }
}

/* 0x60cf4 arm: Validate a colon-separated duration and optional field format. */
int clock_duration_validate(const char *str, const char *format)
{
    int count = 0;
    while (1) {
        unsigned int span = strspn(str, "0123456789");
        if (span == 0)
            return 0;
        switch (str[span]) {
        case '\0': goto counted;
        case ':': str += span + 1; count++; break;
        default: return 0;
        }
    }
counted:
    count++;
    if (count < 1 || count > 3)
        return 0;

    if (format != NULL && *format != '\0')
    {
        if (strcasecmp("hms", format) != 0 &&
            strcasecmp("hm", format) != 0 &&
            strcasecmp("ms", format) != 0 &&
            (format[1] != '\0' || strchr("hms", (unsigned char)format[0]) == NULL))
        {
            cst_errmsg("Invalid duration_format: %s", format);
            return 0;
        }
        return strlen(format) == count;
    }

    return count == 2 || count == 3;
}

/* 0x60e38 arm: Parse hours/minutes/seconds according to the supplied format.
 * WIP: instruction layout and register allocation do not match yet. */
void clock_duration_parse(cst_time *t, const char *str, const char *format)
{
    unsigned int count = clock_duration_fields(str);
    if (t) {
        t->f6 = NULL;
        t->f5 = NULL;
        t->f4 = NULL;
        t->f3 = NULL;
        t->f2 = NULL;
        t->f1 = NULL;
        t->f0 = NULL;
    }
    if (count - 1 > 2)
        return;
    if (format && *format) {
        if (strcasecmp("hms", format) != 0 &&
            strcasecmp("hm", format) != 0 &&
            strcasecmp("ms", format) != 0 &&
            (format[1] != '\0' || strchr("hms", (unsigned char)format[0]) == NULL)) {
            cst_errmsg("Invalid duration_format: %s", format);
            return;
        }
        if (strlen(format) != count)
            return;
    } else {
        if (count == 2)
            format = "hm";
        else if (count == 3)
            format = "hms";
        else
            return;
    }
    if (!t)
        return;
    for (const char *f = format; *f; f++) {
        const char *end = cst_strchr(str, ':');
        char *field = cst_substr(str, 0, end - str);
        switch (*f) {
        case 'h': t->f4 = field; break;
        case 'm': t->f5 = field; break;
        case 's': t->f6 = field; break;
        }
        str = end + 1;
    }
}

/* 0x6102c arm */
void time_free(cst_time *t)
{
    if (t == NULL)
        return;
    cst_free(t->f0);
    cst_free(t->f1);
    cst_free(t->f2);
    cst_free(t->f3);
    cst_free(t->f4);
    cst_free(t->f5);
    cst_free(t->f6);
    t->f6 = NULL;
    t->f5 = NULL;
    t->f4 = NULL;
    t->f3 = NULL;
    t->f2 = NULL;
    t->f1 = NULL;
    t->f0 = NULL;
}

typedef struct {
    char *f0;
    char *f1;
    char *f2;
} cst_duration;

/* 0x614b4 arm */
void duration_parse(cst_duration *d, const char *str)
{
    cst_val *v = cst_split_val(str, ":");
    unsigned int len = val_length(v);

    if (len != 0)
        d->f0 = cst_strdup(val_string(val_car(v)));
    else
        d->f0 = cst_strdup("0");

    if (len > 1)
        d->f1 = cst_substr(val_string(val_car(val_cdr(v))), 0, 2);
    else
        d->f1 = cst_strdup("00");

    if (len > 1 && strlen(val_string(val_car(val_cdr(v)))) > 2)
        d->f2 = cst_strdup(val_string(val_car(val_cdr(v))) + 2);
    else
        d->f2 = cst_strdup("");

    delete_val(v);
}

/* 0x615ac arm */
void duration_free(cst_duration *d)
{
    if (d == NULL)
        return;
    cst_free(d->f0);
    cst_free(d->f1);
    cst_free(d->f2);
    d->f2 = NULL;
    d->f1 = NULL;
    d->f0 = NULL;
}

/* 0x615e4 arm: the one alphabetic part of speech pos_list gives word, else '?' */
char token_single_alpha(const char *word, int (*pos_list)(const char *word, char *buf, size_t max))
{
    char buf[0x100];
    int n = pos_list(word, buf, sizeof buf), i, at = -1;

    if (n == 0)
        return '?';
    for (i = 0; i < n; i++) {
        if (buf[i] >= 0 && isalpha(buf[i])) {
            if (at != -1)
                return '?';
            at = i;
        }
    }
    if (at == -1)
        return '?';
    return buf[at];
}

/* 0x6168c arm */
int token_no_punc_prev(const cst_item *item)
{
    const cst_item *prev = item_prev(item);
    if (!prev || *(const unsigned char *)item_feat_string(prev, "punc") != '\0')
        return 0;
    if (!item_feat_present(item, "prepunctuation"))
        return 1;
    return *(const unsigned char *)item_feat_string(item, "prepunctuation") == '\0';
}

/* 0x61708 arm */
int token_no_punc_next(const cst_item *item)
{
    const cst_item *next = item_next(item);
    if (!next || *(const unsigned char *)item_feat_string(item, "punc") != '\0')
        return 0;
    if (!item_feat_present(next, "prepunctuation"))
        return 1;
    return *(const unsigned char *)item_feat_string(next, "prepunctuation") == '\0';
}

/* 0x61788 arm */
cst_item *token_prev_interpret_as(const cst_item *item, const char *str)
{
    cst_item *prev = item_prev(item);
    if (prev && cst_streq(str, ffeature_string(item_traverse(prev, 0), "interpret-as")))
        return prev;
    return NULL;
}

/* 0x617d8 arm */
cst_item *token_next_interpret_as(const cst_item *item, const char *str)
{
    cst_item *next = item_next(item);
    if (next && cst_streq(str, ffeature_string(item_traverse(next, 0), "interpret-as")))
        return next;
    return NULL;
}

/* 0x61828 arm */
cst_item *token_unpunctuated_prev_interpret_as(const cst_item *item, const char *str)
{
    if (token_no_punc_prev(item) == 0)
        return NULL;
    return token_prev_interpret_as(item, str);
}

/* 0x61850 arm */
cst_item *token_unpunctuated_next_interpret_as(const cst_item *item, const char *str)
{
    if (token_no_punc_next(item) == 0)
        return NULL;
    return token_next_interpret_as(item, str);
}

/* 0x61878 arm */
const char *strip_leading_zeros(const char *str)
{
    if (*str != '0' || str[1] == '\0')
        return str;

    while (str[1] != '\0')
    {
        str++;
        if (*str != '0')
            return str;
    }
    return str;
}

/* 0x618bc arm */
void strip_non_ascii(char *str)
{
    const char *p = str;
    int c;
    while ((c = *(const unsigned char *)p++) != 0) {
        if ((c & 0x80) == 0)
            *str++ = c;
    }
    *str = 0;
}

/* 0x3d72f8: SSML roles and the parts of speech they give (pairs) */
static const char *const en_role_pos[] = {
    "a", "JJ", "c", "CC", "n", "NN", "p", "IN", "r", "DT", "t", "VBD", "v", "VB",
    "0", "DEFAULT", "1", "SENSE_1", "2", "SENSE_2", "3", "SENSE_3", "4", "SENSE_4", NULL
};

/* 0x618e4 arm: the part of speech the role of the word's token gives, else pos_func's */
const char *word_role_pos(const cst_item *word, const char *(*pos_func)(const cst_item *))
{
    const cst_val *v = item_feat(item_parent(item_as(word, 1)), "role");
    const char *const *p;
    const char *role;

    if (v) {
        role = val_string(v);
        for (p = en_role_pos; p[0]; p += 2)
            if (strcmp(role, p[1]) == 0)
                return p[0];
    }
    return pos_func(word);
}

int utf8_decode(const char *str, int *len);
int utf8_strlen_gt(const char *str, int count);
int uni_category(unsigned int codepoint);
int uni_cat_is_text(int category);
const char *utf8_strchr(const char *str, unsigned int codepoint);

/* 0x61978 arm: Detect a category boundary after the codepoint at a byte offset.
 * Descriptive name; the original symbol is unknown. */
int utf8_category_boundary(const char *str, int offset)
{
    int len;
    int category = uni_category(utf8_decode(str + offset, &len));
    if (!uni_cat_is_text(category))
        return 1;
    return uni_category(utf8_decode(str + offset + len, NULL)) != category;
}

/* 0x619d4 arm: Find a category split, returning zero if no suffix remains.
 * Descriptive name; the original symbol is unknown. */
int utf8_category_split(const char *str)
{
    int offset = 0;
    int len;
    if (!utf8_strlen_gt(str, 1))
        return 0;
    while (utf8_decode(str + offset, &len)) {
        if (utf8_category_boundary(str, offset))
            break;
        offset += len;
    }
    offset += len;
    if (str[offset])
        return offset;
    return 0;
}

/* 0x61a54 arm: Require a letter and allow only letters or explicitly ignored codepoints.
 * Descriptive name; the original symbol is unknown. */
int utf8_is_letters_except(const char *str, const char *ignore)
{
    int offset, found;
    int len, c;
    offset = found = 0;
    while ((c = utf8_decode(str + offset, &len)) != 0) {
        offset += len;
        if (!ignore || !utf8_strchr(ignore, c)) {
            found = 1;
            if (uni_category(c) != 4)
                return 0;
        }
    }
    return found;
}










