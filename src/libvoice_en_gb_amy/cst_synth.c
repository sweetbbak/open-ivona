#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"
#include "cst_val.h"
#include "cst_error.h"
#include "cst_val_defs.h"
#include "cst_string.h"
#include "cst_regex.h"
#include "ivona_vector.h"
#include "ivona_util.h"

typedef char *(*xsampafunc)(const cst_item *i);
extern const int cst_val_type_xsampafunc;

/* 0x5b934 arm, 0x5b98c arm */
CST_VAL_REGISTER_FUNCPTR(xsampafunc, xsampafunc)

/* 0x5b9a4 arm: the flag of an X-SAMPA tone or diacritic suffix, 0 if unknown. */
unsigned int xsampa_mod_flag(const char *s)
{
    if (strcmp(s, "_1") == 0)
        return 0x1;
    if (strcmp(s, "_2") == 0)
        return 0x4;
    if (strcmp(s, "_3") == 0)
        return 0x8;
    if (strcmp(s, "_4") == 0)
        return 0x10;
    if (strcmp(s, "_5") == 0)
        return 0x20;
    if (strcmp(s, "_6") == 0)
        return 0x40;
    if (strcmp(s, "_/") == 0)
        return 0x80;
    if (strcmp(s, "_R") == 0)
        return 0x100;
    if (strcmp(s, "_R_F") == 0)
        return 0x200;
    if (strcmp(s, "_\\") == 0)
        return 0x400;
    if (strcmp(s, "_F") == 0)
        return 0x800;
    if (strcmp(s, "_B") == 0)
        return 0x1000;
    if (strcmp(s, "_L") == 0)
        return 0x2000;
    if (strcmp(s, "_B_L") == 0)
        return 0x4000;
    if (strcmp(s, "_M") == 0)
        return 0x8000;
    if (strcmp(s, "_H") == 0)
        return 0x10000;
    if (strcmp(s, "_H_T") == 0)
        return 0x20000;
    if (strcmp(s, "_T") == 0)
        return 0x40000;
    return 0;
}

/* 0x5c350 arm: the suffix of a modifier flag */
const char *xsampa_mod_name(unsigned int flag)
{
    switch (flag) {
    case 0x1:
        return "_1";
    case 0x4:
        return "_2";
    case 0x8:
        return "_3";
    case 0x10:
        return "_4";
    case 0x20:
        return "_5";
    case 0x40:
        return "_6";
    case 0x80:
        return "_/";
    case 0x100:
        return "_R";
    case 0x200:
        return "_R_F";
    case 0x400:
        return "_\\";
    case 0x800:
        return "_F";
    case 0x1000:
        return "_B";
    case 0x2000:
        return "_L";
    case 0x4000:
        return "_B_L";
    case 0x8000:
        return "_M";
    case 0x10000:
        return "_H";
    case 0x20000:
        return "_H_T";
    case 0x40000:
        return "_T";
    default:
        return NULL;
    }
}

#define REL_PHRASE 3
#define REL_SYLSTRUCTURE 4
#define REL_SEGMENT 6

cst_item *item_last_daughter(const cst_item *i);
cst_item *item_traverse(const cst_item *item, ...);
const char *ffeature_string(const cst_item *item, const char *featpath);
int ffeature_int(const cst_item *item, const char *featpath);
void FUN_00064624(const char *s, int start, int len, const void *rules, int flags, char *out);

typedef struct stress_info {
    int stress;                 /* 0 ends the table */
    int mark;                   /* 1: primary ("), 2: secondary (%) */
    unsigned int tone;          /* modifier flag appended to the vowel */
    int unused;
} stress_info;

static const stress_info default_stress_info[] = {
    { 1, 1, 0, 0 },
    { 2, 2, 0, 0 },
    { 0, 0, 0, 0 },
};

static const stress_info *stress_info_find(const stress_info *si, int stress)
{
    for (; si->stress; si++)
        if (si->stress == stress)
            return si;
    return NULL;
}

/* 0x5c520 arm: the X-SAMPA of a segment: its IVONA phone rewritten by rules in the context of
 * its word ("[$$][$][a][b]...[$]"), with syllable and stress marks before it and tone and
 * glottal stop after it. */
char *xivona_to_xsampa(cst_item *seg, const void *rules, const stress_info *si)
{
    cst_item *ss, *syl, *word, *first, *end, *s;
    const stress_info *e;
    strbuf out, ctx;
    int start, len;
    char *buf;

    ss = item_as(seg, REL_SYLSTRUCTURE);
    if (si == NULL)
        si = default_stress_info;
    if (ss && rules) {
        syl = item_parent(ss);
        word = item_parent(syl);
        first = item_daughter(item_daughter(word));
        end = item_next(item_as(item_last_daughter(item_last_daughter(word)), REL_SEGMENT));
        strbuf_init(&out);
        strbuf_init(&ctx);
        if (ss == first)
            strbuf_append_str(&out, " ");
        s = item_daughter(item_parent(ss));
        if (s == ss) {
            e = stress_info_find(si, item_feat_int(syl, "stress"));
            if (first != s)
                strbuf_append_str(&out, ".");
            if (e) {
                if (e->mark == 1)
                    strbuf_append_str(&out, "\"");
                else if (e->mark == 2)
                    strbuf_append_str(&out, "%");
            }
        }
        if (item_prev(item_as(word, REL_PHRASE)) == NULL)
            strbuf_append_str(&ctx, "[$$]");
        strbuf_append_str(&ctx, "[$]");
        for (s = item_as(first, REL_SEGMENT); s && seg != s; s = item_next(s)) {
            strbuf_append_str(&ctx, "[");
            strbuf_append_str(&ctx, item_feat_string(s, "name"));
            strbuf_append_str(&ctx, "]");
        }
        start = ctx.len;
        strbuf_append_str(&ctx, "[");
        strbuf_append_str(&ctx, item_feat_string(seg, "name"));
        strbuf_append_str(&ctx, "]");
        len = ctx.len - start;
        for (s = item_next(seg); s && end != s; s = item_next(s)) {
            strbuf_append_str(&ctx, "[");
            strbuf_append_str(&ctx, item_feat_string(s, "name"));
            strbuf_append_str(&ctx, "]");
        }
        strbuf_append_str(&ctx, "[$]");
        if (item_next(item_as(word, REL_PHRASE)) == NULL)
            strbuf_append_str(&ctx, "[$$]");
        buf = cst_safe_alloc(ctx.len + 1, 1);
        FUN_00064624(ctx.data, start, len, rules, 0, buf);
        strbuf_append_str(&out, buf);
        cst_free(buf);
        if (si != default_stress_info
            && strcmp("+", ffeature_string(item_traverse(ss, 0), "ph_vc")) == 0) {
            e = stress_info_find(si, item_feat_int(syl, "stress"));
            if (e && e->tone)
                strbuf_append_str(&out, xsampa_mod_name(e->tone));
        }
        if (item_feat_int(item_parent(ss), "glottal_stop_pos")
            == ffeature_int(item_traverse(seg, 0), "pos_in_syl") + 1)
            strbuf_append_str(&out, "?");
        strbuf_release(&ctx);
        return out.data;
    }
    return cst_strdup("");
}

/* 0x5ca14 arm: the X-SAMPA of a segment by the utterance's rules */
char *item_xsampa(cst_item *seg)
{
    const cst_val *v;
    const void *rules = NULL;

    v = feat_val(item_utt(seg)->features, "xivona_to_xsampa_rules");
    if (v)
        rules = val_userdata(v);
    v = feat_val(item_utt(seg)->features, "xivona_stress_info");
    if (v)
        return xivona_to_xsampa(seg, rules, val_userdata(v));
    return xivona_to_xsampa(seg, rules, default_stress_info);
}

/* 0x5ca9c arm: item_xsampa of a Segment, marked "-" after a retroflex t/d when it is a
 * sibilant (s S z Z). */
char *seg_xsampa(cst_item *i)
{
    char *s = item_xsampa(item_as(i, REL_SEGMENT));
    cst_item *p = item_prev(i);
    cst_regex *re;
    char *ps, *n;

    if (p && strchr("sSzZ", s[0])) {
        re = new_cst_regex(" *\\(^\\|[^_]\\)[td]`?$");
        ps = item_xsampa(p);
        if (cst_regex_match(re, ps)) {
            n = cst_safe_alloc(strlen(s) + 2, 1);
            n[0] = '-';
            strcpy(n + 1, s);
            cst_free(s);
            s = n;
        }
        delete_cst_regex(re);
        cst_free(ps);
    }
    return s;
}

/* 0x5cb64 arm: a copy of s with spaces turned into dashes. */
char *str_dashes(const char *s)
{
    char *r = cst_strdup(s);

    cst_strsubchar(r, ' ', '-');
    return r;
}

/* 0x5cb84 arm: "x-amazon" or "x-amazon-..." */
int lang_is_amazon(const char *lang)
{
    if (lang == NULL)
        return 0;
    if (strncmp(lang, "x-amazon", 8) != 0)
        return 0;
    return lang[8] == '\0' || lang[8] == '-';
}

cst_val *xsampa_word_convert(const char *word, const void *tag, const void *c);

/* 0x5cbd4 arm: converts a word; for "x-amazon" languages spaces become dashes first. */
cst_val *xsampa_word(const char *word, const char *lang, const void *a, const void *b)
{
    char *w = NULL;
    cst_val *r;

    if (lang_is_amazon(lang))
        word = w = str_dashes(word);
    r = xsampa_word_convert(word, a, b);
    cst_free(w);
    return r;
}

#define XSAMPA_NMODS 0x3b

typedef struct xs_tok {
    const char *s;
    int pos;                            /* of the current token */
    int len;                            /* of its symbol */
    unsigned char mods[XSAMPA_NMODS];   /* modifiers present */
    int next;                           /* +0x48 */
} xs_tok;

/* 0x5cc2c arm */
void xs_tok_init(xs_tok *t, const char *s)
{
    t->s = s;
    t->pos = 0;
    t->len = 0;
    t->next = 0;
}

/* 0x5cc40 arm */
void xs_tok_done(xs_tok *t)
{
}

typedef struct xsampa_entry {
    const char *s;
    unsigned int flags;
} xsampa_entry;

extern const xsampa_entry xsampa_symbols[];
extern const xsampa_entry xsampa_modifiers[];

/* 0x5cc44 arm: reads the next symbol and its modifiers; 0 at the end. */
int xs_tok_next(xs_tok *t)
{
    const xsampa_entry *e;
    unsigned int flags;
    int p, i;

    t->pos = t->next;
    if (t->s[t->pos] == '\0') {
        t->len = 0;
        return 0;
    }
    memset(t->mods, 0, XSAMPA_NMODS);
    for (e = xsampa_symbols; e->s; e++)
        if (cst_startswith(t->s + t->pos, e->s))
            goto found;
    t->next = t->pos + 1;
    t->len = 1;
    return 1;
found:
    t->len = strlen(e->s);
    flags = e->flags;
    p = t->len + t->pos;
    while (t->s[p]) {
        for (i = 0; i < XSAMPA_NMODS; i++)
            if (cst_startswith(t->s + p, xsampa_modifiers[i].s)
                && (flags & xsampa_modifiers[i].flags))
                break;
        if (i == XSAMPA_NMODS)
            break;
        t->mods[i] = 1;
        p += strlen(xsampa_modifiers[i].s);
    }
    t->next = p;
    return 1;
}

#include "str_rewrite.h"

cst_val *phones_from_string(const char *s);
void str_rewrite(const char *s, const str_rewrite_rule *rules, const char *tag, char *out);

/* how an alphabet converts (en_xsampa.c's tables) */
typedef struct xsampa_ctx {
    const str_rewrite_rule *rules;      /* X-SAMPA to IVONA phones */
    const char *const *modifiers;       /* modifiers kept as they are */
    const char *const *vowels;          /* where stress digits go */
    const char *const *syllabics;       /* one per syllable */
    const stress_info *stress;
} xsampa_ctx;

/* s without its character at i */
static inline void str_delete(char *s, int i)
{
    memmove(s + i, s + i + 1, strlen(s + i + 1) + 1);
}

/* the length of the run of lowercase letters at s[i] (at least 1) */
static inline int lower_run(const char *s, int i)
{
    int n = 1;

    while ((unsigned char)(s[i + n] - 'a') <= 25)
        n++;
    return n;
}

/* whether s[i..i+n) is one of the words of list */
static inline int run_in(const char *const *list, const char *s, int i, int n)
{
    const char *const *w;
    int k;

    for (w = list; *w; w++) {
        for (k = 0; k < n && (*w)[k] && (*w)[k] == s[i + k]; k++)
            ;
        if (k == n && (*w)[n] == '\0')
            return 1;
    }
    return 0;
}

/* 0x5bc30 arm: the IVONA phones of an X-SAMPA word: its symbols (and the modifiers the alphabet
 * keeps, and tone marks) bracketed and rewritten by ctx's rules; then each stress digit moves
 * behind the vowel after it ("*" leaves the next one in place, "@" merges with a previous
 * digit), and only one "-" separates syllabic phones */
cst_val *xsampa_word_convert(const char *word, const void *tag, const void *c)
{
    const xsampa_ctx *ctx = c;
    const stress_info *si = ctx->stress ? ctx->stress : default_stress_info;
    const stress_info *e;
    strbuf b;
    xs_tok t;
    cst_val *phones;
    char *s, m;
    int k, i, n, mark, star, sep;

    strbuf_init(&b);
    strbuf_append_str(&b, "[$]");
    xs_tok_init(&t, word);
    while (xs_tok_next(&t)) {
        strbuf_append_str(&b, "[");
        strbuf_append(&b, word + t.pos, t.len);
        strbuf_append_str(&b, "]");
        for (k = 0; k < XSAMPA_NMODS; k++)
            if (t.mods[k] && cst_member_string(xsampa_modifiers[k].s, ctx->modifiers))
                strbuf_append_str(&b, xsampa_modifiers[k].s);
        for (k = 0; k < XSAMPA_NMODS; k++)
            if (t.mods[k] && xsampa_mod_flag(xsampa_modifiers[k].s) && si->stress) {
                strbuf_append_str(&b, "[");
                strbuf_append_str(&b, xsampa_modifiers[k].s);
                strbuf_append_str(&b, "]");
            }
    }
    xs_tok_done(&t);
    strbuf_append_str(&b, "[$]");
    s = cst_safe_alloc(b.len * 10 + 7, 1);
    str_rewrite(b.data, ctx->rules, tag, s);
    strbuf_release(&b);

    /* stress digits go behind the following vowel */
    mark = -1;
    star = 0;
    for (i = 0; s[i]; i++) {
        for (e = si; e->stress; e++)
            if (s[i] - '0' == e->stress)
                break;
        if (e->stress) {
            if (mark >= 0) {
                str_delete(s, mark);
                i--;
            }
            mark = i;
        } else if (s[i] == '-') {
            if (mark >= 0) {
                str_delete(s, mark);
                i--;
                mark = -1;
            }
        } else if ((unsigned char)(s[i] - 'a') <= 25) {
            if (mark < 0)
                continue;
            n = lower_run(s, i);
            if (run_in(ctx->vowels, s, i, n) && !star) {
                m = s[mark];
                memmove(s + mark, s + mark + 1, i + n - mark - 1);
                s[i + n - 1] = m;
                mark = -1;
            }
            i += n - 1;
            star = 0;
        } else if (s[i] == '@') {
            str_delete(s, i);
            k = i - 2;
            if (s[i]) {
                if ((unsigned char)(s[i] - '0') <= 9 && k >= 0 && (unsigned char)(s[k] - '0') <= 9)
                    s[k] = s[i];
                str_delete(s, i);
            }
            i--;
        } else if (s[i] == '*') {
            str_delete(s, i);
            i--;
            star = 1;
        }
    }
    if (mark >= 0)
        str_delete(s, mark);

    /* one boundary between syllabic phones */
    sep = -1;
    for (i = 0; s[i]; i++) {
        if (s[i] == '-') {
            if (i != 0 && sep < 0)
                sep = i;
            else {
                str_delete(s, i);
                i--;
            }
        } else if ((unsigned char)(s[i] - 'a') <= 25) {
            n = lower_run(s, i);
            if (run_in(ctx->syllabics, s, i, n)) {
                i += n - 1;
                sep = -1;
            }
        }
    }
    if (sep >= 0)
        str_delete(s, sep);
    phones = phones_from_string(s);
    cst_free(s);
    return phones;
}

/* 0x5cd7c arm: qsort order of ints */
static int int_cmp(const void *a, const void *b)
{
    return *(const int *)a - *(const int *)b;
}

cst_featvalpair *feat_find_featpair(const cst_features *f, const char *name);

/* 0x5cd8c arm: renames relation from to to in the relation lists of i and its descendants */
static inline void item_rename_relation(cst_item *i, const char *from, const char *to)
{
    cst_featvalpair *fp = feat_find_featpair(i->contents->relations, from);
    cst_item *d;

    cst_dbgmsg("", val_item(fp->val), feat_find_featpair(i->contents->relations, to));
    fp->name = to;
    for (d = item_daughter(i); d; d = item_next(d))
        item_rename_relation(d, from, to);
}

#define REL_TOKEN 1
#define REL_WORD 2
#define REL_PHRASE 3
#define REL_SYLSTRUCTURE 4
#define REL_SYLLABLE 5
#define REL_SEGMENT 6
#define REL_PHRPUN 8

#define ITEM_PARENT 0x106
#define ITEM_DAUGHTERN 0x108

typedef struct tokenizer tokenizer;
typedef struct token {
    const char *name;
    const char *whitespace;
    const char *prepunctuation;
    const char *postpunctuation;
    int start;
    int end;
    const char *extra;
} token;

tokenizer *string_tokenizer_new_single(const char *text, cst_features *feats, cst_features *params);
token *tokenizer_get(tokenizer *t);
void tokenizer_free(tokenizer *t);
const char *utt_input_text(const cst_utterance *u);
cst_relation *utt_relation_create(cst_utterance *u, int id);

/* 0x5d0b4 arm: the input text's tokens as the Token relation */
cst_utterance *default_tokenization(cst_utterance *u)
{
    const char *text = utt_input_text(u);
    cst_relation *r = utt_relation_create(u, REL_TOKEN);
    tokenizer *tz = string_tokenizer_new_single(text, u->features, u->features);
    token *t;
    cst_item *i;

    while ((t = tokenizer_get(tz)) != NULL) {
        if (t->name[0] == '\0')
            break;
        i = relation_append(r, NULL);
        item_set_string(i, "name", t->name);
        item_set_string(i, "whitespace", t->whitespace);
        item_set_string(i, "prepunctuation", t->prepunctuation);
        item_set_string(i, "punc", t->postpunctuation);
        item_set_int(i, "file_pos", t->end);
        item_set_int(i, "end_pos", t->end);
        if (t->extra)
            item_set_string(i, "extra", t->extra);
    }
    tokenizer_free(tz);
    return u;
}

typedef struct lang_core {
    const char *code;
    int unk_4[4];
    cst_features *features;     /* +0x14 */
    cst_features *ffunctions;   /* +0x18 */
} lang_core;

void *item_lang_data(const cst_item *i);

/* 0x5d208 arm: runs the "t2t_func" of every language present among the tokens, once each */
cst_utterance *t2t_tokens(cst_utterance *u)
{
    ivona_vector *done = vector_new(sizeof(cst_uttfunc), NULL, NULL);
    lang_core *ld;
    const cst_val *v;
    cst_uttfunc f;
    cst_item *t;

    for (t = relation_head(utt_relation(u, REL_TOKEN)); t; t = item_next(t)) {
        ld = item_lang_data(t);
        if (ld && (v = feat_val(ld->features, "t2t_func")) != NULL) {
            f = val_uttfunc(v);
            if (vector_find(done, &f, int_cmp) < 0) {
                vector_push(done, &f);
                f(u);
            }
        }
    }
    vector_free(done);
    return u;
}

/* 0x5d2e0 arm: the first c in s not escaped by a backslash */
char *strchr_unescaped(const char *s, int c)
{
    int i, r;
    char ch;

    for (;;) {
        __asm__ volatile (";;;;;;;;");
        i = 0;
    inner:
        ch = s[i];
        __asm__ goto ("" :::: check);
    check:
        if (ch == '\0')
            return NULL;
        if (ch == c)
            return (char *)(s + i);
        r = ch;
        if (r < 10) r = 11;
        if (r == 6) goto escape;
        if (__builtin_expect(ch != '\\', 1)) {
            s += i;
            r = c;
            if (r < 10) r = 11;
            if (r == 5) continue;
            goto step;
        }
    escape:
        if (s[i + 1] == '\0') {
            i++;
            goto inner;
        }
        s += i + 1;
        r = c;
        if (r < 10) r = 11;
        if (r == 6) continue;
    step:
        s++;
    }
}

/* 0x5d33c arm: s[from, to) with backslash escapes removed (a newly allocated string) */
char *str_unescape_range(const char *s, size_t from, size_t to)
{
    strbuf b;
    char *r;
    size_t i = from;
    int n = 0;

    strbuf_init(&b);
    strbuf_reserve(&b, to);
    if (!s)
        goto done;
    for (; i < to && s[i]; i++) {
        n++;
        if (s[i] != '\\') {
            strbuf_append(&b, s + i, 1);
            n = 0;
        } else if (!(n & 1))
            strbuf_append(&b, s + i, 1);
    }
done:
    r = strbuf_data(&b);
    if (r == NULL)
        r = cst_strdup("");
    return r;
}

cst_utterance *textanalysis_tokens(cst_utterance *u, cst_item *t, cst_item *end);
cst_utterance *phrasing_tokens(cst_utterance *u, cst_item *t, cst_item *end);
cst_utterance *lexical_insertion_words(cst_utterance *u, cst_item *w, cst_item *end);

/* 0x5d8cc arm */
cst_utterance *default_textanalysis(cst_utterance *u)
{
    return textanalysis_tokens(u, relation_head(utt_relation(u, REL_TOKEN)), NULL);
}

/* 0x5dd4c arm */
cst_utterance *default_phrasing(cst_utterance *u)
{
    return phrasing_tokens(u, relation_head(utt_relation(u, REL_TOKEN)), NULL);
}

/* 0x5e5c4 arm */
cst_utterance *default_lexical_insertion(cst_utterance *u)
{
    return lexical_insertion_words(u, relation_head(utt_relation(u, REL_WORD)), NULL);
}

void item_delete_range(cst_item *from, cst_item *to);

/* 0x5e5ec arm: the X-SAMPA of word w into out; w's syllables, segments and SylStructure item
 * are deleted. */
void word_to_xsampa(cst_item *w, cst_features *f, strbuf *out)
{
    cst_item *ws = item_as(w, REL_SYLSTRUCTURE);
    cst_item *fsyl = item_daughter(ws);
    cst_item *lsyl = item_last_daughter(ws);
    cst_item *first = item_as(item_daughter(fsyl), REL_SEGMENT);
    cst_item *last = item_as(item_last_daughter(lsyl), REL_SEGMENT);
    cst_item *sfirst = item_as(fsyl, REL_SYLLABLE);
    cst_item *slast = item_as(lsyl, REL_SYLLABLE);
    const cst_val *v;
    const void *rules = NULL;
    const stress_info *si = NULL;
    cst_item *s;
    char *x;

    v = feat_val(f, "xivona_to_xsampa_rules");
    if (v)
        rules = val_userdata(v);
    v = feat_val(f, "xivona_stress_info");
    if (v)
        si = val_userdata(v);
    strbuf_clear(out);
    for (s = first; s && s != item_next(last); s = item_next(s)) {
        x = xivona_to_xsampa(s, rules, si);
        strbuf_append_str(out, x);
        cst_free(x);
    }
    item_delete_range(sfirst, item_next(slast));
    item_delete_range(first, item_next(last));
    delete_item(ws);
}

cst_item *item_last_daughter(const cst_item *i);
cst_item *item_traverse(const cst_item *item, ...);

void word_append_segment(cst_item *w, const char *name)
{
    cst_item *d, *s;

    for (d = item_last_daughter(w); d; d = item_prev(d)) {
        s = item_traverse(d, REL_SYLSTRUCTURE, ITEM_DAUGHTERN, ITEM_DAUGHTERN, REL_SEGMENT, 0);
        if (s) {
            item_set_string(item_append(s, NULL), "name", name);
            break;
        }
    }
    __asm__ __volatile__ ("");
}

int feats_have_breath_units(const cst_features *f);
void breath_insertion_utt(cst_utterance *u);

/* 0x5f120 arm */
cst_utterance *breath_insertion(cst_utterance *u)
{
    if (feats_have_breath_units(u->features) && u->unk_14
        && strlist_contains(u->unk_14, "breath_insertion"))
        breath_insertion_utt(u);
    return u;
}

const char *cst_relation_id_name(int id);
typedef struct dnn_models {
    void *unk_0;
    void *duration;             /* +0x04 */
    void *unk_8;
    void *phrasing;             /* +0x0c */
} dnn_models;
dnn_models *voice_dnn_models(cst_utterance *u);
int dnn_stub_44(void *model);
float metrics_add_time(cst_features *metrics, const char *name, int usec);

/* 0x5f174 arm */
cst_utterance *default_pause_insertion(cst_utterance *u)
{
    int nodnn = get_param_int(u->features, "disable_phrasing_dnn", 0);
    cst_relation *r = utt_relation(u, REL_PHRASE);
    const char *pname = cst_relation_id_name(r->id);
    const char *ppname = cst_relation_id_name(REL_PHRPUN);
    cst_featvalpair *fp = feat_find_featpair(u->relations, pname);
    cst_relation *nr;
    cst_item *p, *np, *w, *s;
    const char *sil;
    int t0;

    cst_dbgmsg("", val_relation(fp->val), feat_find_featpair(u->relations, ppname));
    fp->name = ppname;
    r->id = REL_PHRPUN;
    for (p = relation_head(r); p; p = item_next(p))
        item_rename_relation(p, pname, ppname);
    nr = utt_relation_create(u, REL_PHRASE);
    if (feat_present(u->features, "phrasing_model") && feat_int(u->features, "phrasing_model") == 1
        && !nodnn)
        dnn_stub_44(voice_dnn_models(u)->phrasing);
    sil = val_string(feat_val(u->features, "silence"));
    s = relation_head(utt_relation(u, REL_SEGMENT));
    if (s)
        s = item_prepend(s, NULL);
    else
        s = relation_append(utt_relation(u, REL_SEGMENT), NULL);
    item_set_string(s, "name", sil);
    for (p = relation_head(utt_relation(u, REL_PHRPUN)); p; p = item_next(p)) {
        np = relation_append(nr, NULL);
        item_set_string(np, "name", item_feat_string(p, "name"));
        t0 = ivona_time_usec();
        for (w = item_daughter(p); w; w = item_next(w)) {
            cst_dbgmsg("", item_parent(item_as(w, REL_TOKEN)));
            item_add_daughter(np, w);
            cst_dbgmsg("", item_next(w) ? item_next(item_as(w, REL_TOKEN)) : NULL);
        }
        word_append_segment(np, sil);
        metrics_add_time(get_param_features(u->features, "core_metrics", NULL),
                         "phrasing_dnn_output_processing_time", ivona_time_usec() - t0);
    }
    return u;
}

const cst_val *item_feat_func(const cst_item *i, const char *name);

/* 0x5f408 arm */
cst_utterance *default_intonation(cst_utterance *u)
{
    const cst_val *f = feat_val(u->features, "phrasesylendtone_func");
    const cst_val *v;
    cst_val *r;
    cst_item *p, *syl;

    for (p = relation_head(utt_relation(u, REL_PHRASE)); p; p = item_next(p)) {
        syl = item_traverse(p, ITEM_DAUGHTERN, REL_SYLSTRUCTURE, ITEM_DAUGHTERN, 0);
        if (syl == NULL)
            continue;
        v = item_feat_func(item_traverse(p, ITEM_DAUGHTERN, REL_TOKEN, ITEM_PARENT, 0), "endtone");
        if (v && (strcmp(val_string(v), "H") == 0 || strcmp(val_string(v), "L") == 0)) {
            item_set(syl, "endtone", v);
            continue;
        }
        if (f && (r = val_itemfunc(f)(p)) != NULL) {
            item_set(syl, "endtone", r);
            continue;
        }
        item_set_string(syl, "endtone", "L");
    }
    return u;
}

/* 0x5f59c arm */
cst_utterance *duration_none(cst_utterance *u)
{
    return u;
}

#include "cst_cart.h"

typedef struct dur_stat {
    const char *phone;
    float mean;
    float stddev;
} dur_stat;

typedef struct named_entry named_entry;
const named_entry *named_table_find(const named_entry *const *t, const char *name);
const dur_stat *const *val_dur_stats(const cst_val *v);
float item_set_breath_acoustic_data(cst_item *item);

#define PAUSE_MIN 0.01f
#define PAUSE_MAX 0.050004f
#define PAUSE_BEFORE_BREATH 0.06f

typedef struct zdur_cart zdur_cart;
const zdur_cart *val_zdur_cart(const cst_val *v);
short zdur_cart_duration(const zdur_cart *dc, const cst_item *seg);
cst_utterance *cart_duration(cst_utterance *u);
dnn_models *val_dnn_models(const cst_val *v);

static float clamp_pause_max(float len, int x)
{
    int a = x + 1, b = a * 3, c = b ^ 7, d = c + 5, e = d * 9, f = e ^ 11;
    int g = f + 13, h = g * 17, k = h ^ 19;

    if (len > PAUSE_MAX)
        return PAUSE_MAX;
    if (x == 5 && k == 999999)
        cst_errmsg("");
    return len;
}

/* 0x5f5a0 arm: segment durations from the duration CART (or the z-score CART), with pauses set
 * by phrase break type and breaths by the breath model. */
cst_utterance *cart_duration(cst_utterance *u)
{
    const zdur_cart *zc;
    const cst_cart *tree;
    const dur_stat *const *stats;
    const dur_stat *ds;
    cst_utterance *uu;
    cst_item *s, *w;
    const char *bt;
    float end = 0, dur, len, base;

    if (feat_present(u->features, "zdur_cart")) {
        zc = val_zdur_cart(feat_val(u->features, "zdur_cart"));
        stats = NULL;
        tree = NULL;
    } else {
        tree = val_cart(feat_val(u->features, "dur_cart"));
        zc = NULL;
        stats = val_dur_stats(feat_val(u->features, "dur_stats"));
    }
    for (s = relation_head(utt_relation(u, REL_SEGMENT)); s; s = item_next(s)) {
        if (strcmp(item_feat_string(s, "name"), "pau") != 0
            && strcmp(item_feat_string(s, "name"), "brth") != 0) {
            if (zc)
                dur = zdur_cart_duration(zc, s) / 1000.0f;
            else {
                ds = (const dur_stat *)named_table_find((const named_entry *const *)stats,
                                                        item_feat_string(s, "name"));
                dur = val_float(cart_interpret(s, tree)) * ds->stddev + ds->mean;
            }
        } else {
            if (strcmp(item_feat_string(s, "name"), "brth") == 0)
                dur = item_set_breath_acoustic_data(s);
            else {
                uu = item_utt(s);
                bt = ffeature_string(item_traverse(item_prev(s), REL_SYLSTRUCTURE, ITEM_PARENT,
                                                   ITEM_PARENT, REL_PHRASE, ITEM_PARENT, 0), "name");
                if (item_prev(s) == NULL) {
                    if (ffeature_int(item_traverse(s, 0x100, REL_SYLSTRUCTURE, ITEM_PARENT,
                                                   ITEM_PARENT, REL_TOKEN, ITEM_PARENT, 0),
                                     "no_pause_start") != 0)
                        dur = PAUSE_BEFORE_BREATH;
                    else {
                        dur = get_param_float(uu->features, "pause_start_len", PAUSE_MAX);
                        if (dur > PAUSE_MAX)
                            dur = PAUSE_MAX;
                        else if (dur < PAUSE_MIN)
                            dur = PAUSE_MIN;
                    }
                } else if (strcmp("BB", bt) == 0) {
                    int g = (unsigned char)bt[0];
                    len = get_param_float(uu->features, "pause_end_len", PAUSE_MAX);
                    base = get_param_float(uu->features, "pause_base_len", 0.35f);
                    if (g < 10)
                        g = 11;
                    len = clamp_pause_max(len, g);
                    if (len < PAUSE_MIN)
                        len = PAUSE_MIN;
                    dur = base + len;
                    if (item_next(s)) {
                        if (strcmp(item_feat_string(item_next(s), "name"), "brth") == 0)
                            dur = PAUSE_BEFORE_BREATH;
                        else
                            dur += 0.1f;
                    }
                    w = item_traverse(item_prev(s), REL_SYLSTRUCTURE, ITEM_PARENT, ITEM_PARENT,
                                      REL_TOKEN, ITEM_PARENT, 0);
                    if (item_feat_present(w, "rvd_pause"))
                        dur = item_feat_float(w, "rvd_pause");
                } else if (strcmp("SB", bt) == 0)
                    dur = 0.25f;
                else {
                    w = item_traverse(item_prev(s), REL_SYLSTRUCTURE, ITEM_PARENT, ITEM_PARENT,
                                      REL_TOKEN, ITEM_PARENT, 0);
                    if (strcmp(item_feat_string(item_prev(s), "name"), "brth") != 0)
                        dur = 0.2f;
                    else
                        dur = PAUSE_BEFORE_BREATH;
                    if (item_next(s)
                        && strcmp(item_feat_string(item_next(s), "name"), "brth") == 0)
                        dur = PAUSE_BEFORE_BREATH;
                    if (item_feat_present(w, "rvd_pause"))
                        dur = item_feat_float(w, "rvd_pause");
                }
            }
        }
        end += dur;
        item_set_float(s, "end", end);
        item_set_float(s, "dur", dur);
    }
    return u;
}

/* 0x5fae8 arm: CART durations unless a DNN predicts them (hybrid mode, a DNN-forced token or
 * "isf_dnn_apply_force"). */
cst_utterance *duration_select(cst_utterance *u)
{
    cst_item *t;

    if (!__builtin_expect(get_param_int(u->features, "disable_hybrid", 0), 1)) {
        if (feat_present(u->features, "dnn_models")
            && __builtin_expect(dnn_stub_44(val_dnn_models(feat_val(u->features, "dnn_models"))->duration), 1))
            goto check_apply;
    }
    if (!feat_present(u->features, "acoustic_feats_file"))
        goto do_cart;

check_apply: __attribute__((hot));
    if (!feat_present(u->features, "isf_dnn_apply_force"))
        goto check_hybrid;
    if (__builtin_expect(feat_int(u->features, "isf_dnn_apply_force"), 1))
        goto none;

check_hybrid: __attribute__((hot));
    if (strlist_contains(u->unk_14, "hybrid"))
        goto none;
    for (t = relation_head(utt_relation(u, REL_TOKEN)); t; t = item_next(t)) {
        if (__builtin_expect(item_feat_present(t, "tts_apply_dnn"), 1)
            && item_feat_int(t, "tts_apply_dnn"))
            goto none;
    }
do_cart:
    return cart_duration(u);
none:
    return duration_none(u);
}

cst_val *phones_from_string(const char *s);
void str_rewrite(const char *s, const str_rewrite_rule *rules, const char *tag, char *out);

/* 0x5fc40 arm */
cst_utterance *phones_textanalysis(cst_utterance *u)
{
    cst_relation *wrel = utt_relation_create(u, REL_WORD);
    cst_item *t, *w;

    if ((t = relation_head(utt_relation(u, REL_TOKEN))) != NULL) {
        register const char *ph __asm__("r9") = "phones";
        register const char *nm __asm__("r5") = "name";
        register const char *em __asm__("r8") = "";
        __asm__("" : "+r"(ph), "+r"(nm), "+r"(em));
        do {
            item_set(t, ph, phones_from_string(item_feat_string(t, nm)));
            w = item_add_daughter(t, NULL);
            item_set_string(w, nm, em);
            relation_append(wrel, w);
        } while ((t = item_next(t)) != NULL);
    }
    return u;
}



