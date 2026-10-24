/*
 * IVONA's synthesis pipeline, from Flite's cst_synth.c and cst_utt_utils.c (0x663b4..0x6b134).
 *
 * Synth methods carry a metric name for each module: with "core_metrics" set, the time spent in
 * every module is recorded. An optional "synth_module_callback" is called before the first
 * module ("init"), after every module (with its hook name) and at the end ("end"); a non-zero
 * return stops the pipeline.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"
#include "cst_val.h"
#include "cst_val_defs.h"
#include "cst_string.h"
#include "cst_tokenstream.h"
#include "cst_regex.h"
#include <string.h>
#include "tts_metrics.h"
#include "tts_voice.h"
#include "ivona_util.h"

cst_relation *val_relation(const cst_val *v);

typedef int (*synth_module_callback)(cst_utterance *u, const char *stage, void *data);
extern const int cst_val_type_synth_module_callback;

typedef struct cst_synth_module {
    const char *hookname;
    const char *metric;         /* "core_metrics" entry for its time */
    cst_uttfunc defhook;
} cst_synth_module;

/* language core data of a voice (see synth.c) */
typedef struct lang_core {
    const char *code;
    int unk_4[4];
    cst_features *features;     /* +0x14 */
    cst_features *ffunctions;   /* +0x18 */
} lang_core;

typedef struct synth_voice {
    cst_features *features;
    lang_core *lang;
    cst_utterance *(*utt_init)(cst_utterance *u, struct synth_voice *v);
} synth_voice;

/* 0x663b4 arm: the items from i on, with their features, relations and daughters. */
void item_print_tree(cst_file fd, const cst_item *i, const char *indent)
{
    const cst_featvalpair *p;
    char sub[256];

    for (; i; i = item_next(i)) {
        cst_fprintf(fd, "%sITEM %p {\n", indent, i);
        for (p = item_feats(i)->head; p; p = p->next) {
            cst_fprintf(fd, "%s\t%s => ", indent, p->name);
            val_print(fd, p->val);
            cst_fprintf(fd, "\n");
        }
        cst_fprintf(fd, "%s\t---\n", indent);
        for (p = i->contents->relations->head; p; p = p->next) {
            cst_fprintf(fd, "%s\tR:%s => ", indent, p->name);
            val_print(fd, p->val);
            cst_fprintf(fd, "\n");
        }
        if (item_daughter(i)) {
            snprintf(sub, 255, "%s\t\t", indent);
            cst_fprintf(fd, "%s\tdaughters {\n", indent);
            item_print_tree(fd, item_daughter(i), sub);
            cst_fprintf(fd, "%s\t}\n", indent);
        }
        cst_fprintf(fd, "%s}\n", indent);
    }
}

/* 0x66580 arm */
const char *utt_input_text(cst_utterance *u)
{
    return val_string(feat_val(u->features, "input_text"));
}

/* 0x665a0 arm */
int utt_set_input_text(cst_utterance *u, const char *text)
{
    if (text == NULL)
        text = "";
    feat_set_string(u->features, "input_text", text);
    return 0;
}

/* 0x665d8 arm */
void item_print_list(cst_file fd, const cst_item *i)
{
    item_print_tree(fd, i, "");
}

/* 0x665e8 arm */
void utt_print_relations(cst_file fd, cst_utterance *u)
{
    const cst_featvalpair *p;

    for (p = u->relations->head; p; p = p->next) {
        cst_fprintf(fd, "*** RELATION: %s\n\n", p->name);
        item_print_tree(fd, relation_head(val_relation(p->val)), "");
        if (p->next == NULL)
            break;
        cst_fprintf(fd, "\n");
    }
}

/* 0x66678 arm: copies the voice's and language's features in and runs the voice's hook. */
cst_utterance *utt_init(cst_utterance *u, synth_voice *v)
{
    feat_copy_into(v->features, u->features);
    feat_copy_into(v->lang->features, u->features);
    feat_copy_into(v->lang->ffunctions, u->ffunctions);
    if (v->utt_init)
        v->utt_init(u, v);
    if (!get_param_int(u->features, "deterministic_path", 1))
        srand48(time(NULL));
    return u;
}

/* 0x666f8 arm, 0x66934 arm */
CST_VAL_REGISTER_FUNCPTR(synth_module_callback, synth_module_callback)

static cst_utterance *apply_synth_module(cst_utterance *u, const cst_synth_module *mod)
{
    const cst_val *v = feat_val(u->features, mod->hookname);

    if (v)
        return (*val_uttfunc(v))(u);
    if (mod->defhook)
        return (*mod->defhook)(u);
    return u;
}

/* 0x66750 arm */
cst_utterance *apply_synth_method(cst_utterance *u, const cst_synth_module meth[])
{
    cst_features *metrics = get_param_features(u->features, "core_metrics", NULL);
    const cst_val *cb = feat_val(u->features, "synth_module_callback");
    const cst_val *v;
    void *data = NULL;
    unsigned int start;

    if (cb) {
        v = feat_val(u->features, "synth_module_callback_data");
        if (v)
            data = val_userdata(v);
        if (val_synth_module_callback(cb)(u, "init", data))
            return u;
    }
    for (; meth->hookname; meth++) {
        start = metrics ? ivona_time_usec() : 0;
        if ((u = apply_synth_module(u, meth)) == NULL)
            return NULL;
        if (metrics)
            metrics_add_time(metrics, meth->metric, ivona_time_usec() - start);
        if (cb && val_synth_module_callback(cb)(u, meth->hookname, data))
            return u;
    }
    if (cb)
        val_synth_module_callback(cb)(u, "end", data);
    return u;
}

cst_utterance *default_tokenization(cst_utterance *u);
cst_utterance *FUN_0006864c(cst_utterance *u);
cst_utterance *default_textanalysis(cst_utterance *u);
cst_utterance *default_phrasing(cst_utterance *u);
cst_utterance *default_lexical_insertion(cst_utterance *u);
cst_utterance *FUN_00069b88(cst_utterance *u);
cst_utterance *FUN_0006a5b8(cst_utterance *u);
cst_utterance *FUN_0006a84c(cst_utterance *u);
cst_utterance *phones_textanalysis(cst_utterance *u);

static const cst_synth_module text_methods[] = {
    { "tokenizer_func", "tokenizer_time", default_tokenization },
    { "t2t_func", "tokenstotokens_time", FUN_0006864c },
    { "text_analysis_func", "text_analysis_time", default_textanalysis },
    { "pos_tagger_func", "pos_tagger_time", NULL },
    { "phrasing_func", "phrasing_time", default_phrasing },
    { "lettertosound_func", "lettertosound_time", default_lexical_insertion },
    { "change_language_func", "change_language_time", FUN_00069b88 },
    { "pause_insertion_func", "pause_insertion_time", FUN_0006a5b8 },
    { "postlex_func", "postlex_time", NULL },
    { "intonation_func", "intonation_time", FUN_0006a84c },
    { "acoustic_prediction_func", "acoustic_prediction_time", NULL },
    { NULL, NULL, NULL }
};

/* the same without tokenization (the utterance has its tokens) */
static const cst_synth_module token_methods[] = {
    { "tokenizer_func", "tokenizer_time", NULL },
    { "t2t_func", "tokenstotokens_time", FUN_0006864c },
    { "text_analysis_func", "text_analysis_time", default_textanalysis },
    { "pos_tagger_func", "pos_tagger_time", NULL },
    { "phrasing_func", "phrasing_time", default_phrasing },
    { "lettertosound_func", "lettertosound_time", default_lexical_insertion },
    { "change_language_func", "change_language_time", FUN_00069b88 },
    { "pause_insertion_func", "pause_insertion_time", FUN_0006a5b8 },
    { "postlex_func", "postlex_time", NULL },
    { "intonation_func", "intonation_time", FUN_0006a84c },
    { "acoustic_prediction_func", "acoustic_prediction_time", NULL },
    { NULL, NULL, NULL }
};

/* input given as phones */
static const cst_synth_module phones_methods[] = {
    { "tokenizer_func", "tokenizer_time", default_tokenization },
    { "t2t_func", "tokenstotokens_time", FUN_0006864c },
    { "text_analysis_func", "text_analysis_time", phones_textanalysis },
    { "pos_tagger_func", "pos_tagger_time", NULL },
    { "phrasing_func", "phrasing_time", default_phrasing },
    { "lettertosound_func", "lettertosound_time", default_lexical_insertion },
    { "change_language_func", "change_language_time", FUN_00069b88 },
    { "pause_insertion_func", "pause_insertion_time", FUN_0006a5b8 },
    { "postlex_func", "postlex_time", NULL },
    { "intonation_func", "intonation_time", FUN_0006a84c },
    { "acoustic_prediction_func", "acoustic_prediction_time", NULL },
    { NULL, NULL, NULL }
};

/* 0x668fc arm */
cst_utterance *utt_text_frontend(cst_utterance *u)
{
    return apply_synth_method(u, text_methods);
}

/* 0x6690c arm */
cst_utterance *utt_frontend(cst_utterance *u)
{
    return apply_synth_method(u, token_methods);
}

/* 0x66920 arm */
cst_utterance *utt_phones_frontend(cst_utterance *u)
{
    return apply_synth_method(u, phones_methods);
}

/* ------------------------------------------------------------------------------------------ */
/* Lexicons and X-SAMPA */

cst_val *phones_from_string(const char *s);

/* 0x6694c arm: an IVONA_Lexicon file ("IVONA_Lexicon", then lines of word and phone string;
 * "#" comments to the end of the line) as a list of (word . phones). */
cst_val *lexicon_from_pls(const char *filename)
{
    cst_tokenstream *ts;
    const char *tok;
    char *word;
    cst_val *phones, *entries = NULL;
    cst_val *r = NULL;
    int comment;

    ts = ts_open(filename);
    if (ts == NULL)
        return r;
    ts->p_singlecharsymbols = "#";
    ts->p_prepunctuationsymbols = "";
    ts->p_postpunctuationsymbols = "";
    if (strcmp("IVONA_Lexicon", ts_get(ts)) != 0) {
        cst_errmsg("IVONA_Lexicon: Missing file header\n");
        r = NULL;
        goto out;
    }
    comment = 0;
    while (!ts_eof(ts)) {
        tok = ts_get(ts);
        if (tok == NULL || tok[0] == '\0')
            break;
        if (strcmp("#", tok) == 0) {
            comment = 1;
            continue;
        }
        if (comment && strchr(ts->whitespace, '\n') == NULL) {
            comment = 1;
            continue;
        }
        word = cst_strdup(tok);
        tok = ts_get(ts);
        if (tok[0] == '\0' || (phones = phones_from_string(tok)) == NULL) {
            cst_errmsg("IVONA_Lexicon: Incorrect token at line %d\n", ts->line_number);
            cst_free(word);
            delete_val(entries);
            r = NULL;
            goto out;
        }
        entries = cons_val(cons_val(string_val(word), phones), entries);
        comment = 0;
        cst_free(word);
    }
    r = val_reverse(entries);
out:
    ts_close(ts);
    return r;
}

/* 0x66ae4 arm: the entry of word (with part of speech pos) in a lexicon list. */
const cst_val *in_lex(const cst_val *entries, const char *word, const char *pos)
{
    char *wp = cst_safe_alloc(strlen(word) + 2, 1);
    const cst_val *r;

    sprintf(wp, "%c%s", pos ? pos[0] : '0', word);
    r = val_assoc_string(wp, entries);
    cst_free(wp);
    return r;
}

typedef char *(*xsampafunc)(const cst_item *i);
extern const int cst_val_type_xsampafunc;

/* 0x66b4c arm, 0x66ba4 arm */
CST_VAL_REGISTER_FUNCPTR(xsampafunc, xsampafunc)

/* 0x66bbc arm: the flag of an X-SAMPA tone or diacritic suffix, 0 if unknown. */
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

/* 0x67568 arm: the suffix of a modifier flag */
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

cst_item *item_traverse(const cst_item *item, ...);
const char *ffeature_string(const cst_item *item, const char *featpath);
int ffeature_int(const cst_item *item, const char *featpath);
void FUN_0006dc94(const char *s, int start, int len, const void *rules, int flags, char *out);

/* how syllable stress values are written */
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

/* 0x67738 arm: the X-SAMPA of a segment: its IVONA phone rewritten by rules in the context of
 * its word ("[$$][$][a][b]...[$]"), with syllable and stress marks before it and tone and
 * glottal stop after it. */
char *xivona_to_xsampa(cst_item *seg, const void *rules, const stress_info *si)
{
    cst_item *ss, *syl, *word, *first, *end, *s;
    const stress_info *e;
    strbuf out, ctx;
    int start, len, st;
    char *buf;

    ss = item_as(seg, REL_SYLSTRUCTURE);
    if (si == NULL)
        si = default_stress_info;
    if (ss == NULL || rules == NULL)
        return cst_strdup("");
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
    for (s = item_as(first, REL_SEGMENT); s && s != seg; s = item_next(s)) {
        strbuf_append_str(&ctx, "[");
        strbuf_append_str(&ctx, item_feat_string(s, "name"));
        strbuf_append_str(&ctx, "]");
    }
    start = ctx.len;
    strbuf_append_str(&ctx, "[");
    strbuf_append_str(&ctx, item_feat_string(seg, "name"));
    strbuf_append_str(&ctx, "]");
    len = ctx.len - start;
    for (s = item_next(seg); s && s != end; s = item_next(s)) {
        strbuf_append_str(&ctx, "[");
        strbuf_append_str(&ctx, item_feat_string(s, "name"));
        strbuf_append_str(&ctx, "]");
    }
    strbuf_append_str(&ctx, "[$]");
    if (item_next(item_as(word, REL_PHRASE)) == NULL)
        strbuf_append_str(&ctx, "[$$]");
    buf = cst_safe_alloc(ctx.len + 1, 1);
    FUN_0006dc94(ctx.data, start, len, rules, 0, buf);
    strbuf_append_str(&out, buf);
    cst_free(buf);
    if (si != default_stress_info
        && strcmp("+", ffeature_string(item_traverse(ss, 0), "ph_vc")) == 0) {
        st = item_feat_int(syl, "stress");
        for (; si->stress; si++)
            if (st == si->stress) {
                if (si->tone)
                    strbuf_append_str(&out, xsampa_mod_name(si->tone));
                break;
            }
    }
    if (item_feat_int(item_parent(ss), "glottal_stop_pos")
        == ffeature_int(item_traverse(seg, 0), "pos_in_syl") + 1)
        strbuf_append_str(&out, "?");
    strbuf_release(&ctx);
    return out.data;
}

/* 0x67c2c arm: the X-SAMPA of a segment by the utterance's rules */
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

/* 0x67cb4 arm: item_xsampa of a Segment, marked "-" after a retroflex t/d when it is a
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

/* 0x67d7c arm: a copy of s with spaces turned into dashes. */
char *str_dashes(const char *s)
{
    char *r = cst_strdup(s);

    cst_strsubchar(r, ' ', '-');
    return r;
}

/* 0x67d9c arm: "x-amazon" or "x-amazon-..." */
int lang_is_amazon(const char *lang)
{
    if (lang == NULL)
        return 0;
    if (strncmp(lang, "x-amazon", 8) != 0)
        return 0;
    return lang[8] == '\0' || lang[8] == '-';
}

char *xsampa_word_convert(const char *word, const void *a, const void *b);

/* 0x67dec arm: converts a word; for "x-amazon" languages spaces become dashes first. */
char *xsampa_word(const char *word, const char *lang, const void *a, const void *b)
{
    char *w = NULL;
    char *r;

    if (lang_is_amazon(lang))
        word = w = str_dashes(word);
    r = xsampa_word_convert(word, a, b);
    cst_free(w);
    return r;
}

/* X-SAMPA tokens: a symbol and the modifiers following it */
typedef struct xsampa_entry {
    const char *s;
    unsigned int flags;
} xsampa_entry;

#define XSAMPA_NMODS 0x3b

extern const xsampa_entry xsampa_symbols[];
extern const xsampa_entry xsampa_modifiers[];

typedef struct xs_tok {
    const char *s;
    int pos;                            /* of the current token */
    int len;                            /* of its symbol */
    unsigned char mods[XSAMPA_NMODS];   /* modifiers present */
    int next;                           /* +0x48 */
} xs_tok;

/* 0x67e44 arm */
void xs_tok_init(xs_tok *t, const char *s)
{
    t->s = s;
    t->pos = 0;
    t->len = 0;
    t->next = 0;
}

/* 0x67e58 arm */
void xs_tok_done(xs_tok *t)
{
}

/* 0x67e5c arm: reads the next symbol and its modifiers; 0 at the end. */
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

typedef struct zdur_cart zdur_cart;
extern const int cst_val_type_zdur_cart;

/* 0x67f94 arm, 0x67fec arm */
CST_VAL_REGISTER_TYPE_NODEL(zdur_cart, zdur_cart)

/* 0x681c0 arm: qsort order of ints */
int int_cmp(const void *a, const void *b)
{
    return *(const int *)a - *(const int *)b;
}

/* ------------------------------------------------------------------------------------------ */
/* Front end modules */

#define REL_TOKEN 1
#define REL_WORD 2
#define ITEM_DAUGHTERN 0x108

/* 0x68724 arm: the first c in s not escaped by a backslash */
char *strchr_unescaped(const char *s, int c)
{
    for (;;) {
        if (*s == '\0')
            return NULL;
        if (*s == c)
            return (char *)s;
        if (*s == '\\' && s[1])
            s++;
        s++;
    }
}

cst_utterance *textanalysis_tokens(cst_utterance *u, cst_item *t, cst_item *end);

/* 0x68d10 arm */
cst_utterance *default_textanalysis(cst_utterance *u)
{
    return textanalysis_tokens(u, relation_head(utt_relation(u, REL_TOKEN)), NULL);
}

cst_utterance *phrasing_tokens(cst_utterance *u, cst_item *t, cst_item *end);

/* 0x69190 arm */
cst_utterance *default_phrasing(cst_utterance *u)
{
    return phrasing_tokens(u, relation_head(utt_relation(u, REL_TOKEN)), NULL);
}

cst_utterance *lexical_insertion_words(cst_utterance *u, cst_item *w, cst_item *end);

/* 0x69a08 arm */
cst_utterance *default_lexical_insertion(cst_utterance *u)
{
    return lexical_insertion_words(u, relation_head(utt_relation(u, REL_WORD)), NULL);
}

/* 0x6a078 arm: appends a segment named name after the last segment of the last word of w
 * that has one. (Declared int without a value, which keeps the last call from being a
 * sibcall.) */
int word_append_segment(cst_item *w, const char *name)
{
    cst_item *d, *s;

    for (d = item_last_daughter(w); d; d = item_prev(d)) {
        s = item_traverse(d, REL_SYLSTRUCTURE, ITEM_DAUGHTERN, ITEM_DAUGHTERN, REL_SEGMENT, 0);
        if (s) {
            item_set_string(item_append(s, NULL), "name", name);
            break;
        }
    }
}

int feats_have_breath_units(const cst_features *f);
void breath_insertion_utt(cst_utterance *u);

/* 0x6a564 arm */
cst_utterance *breath_insertion(cst_utterance *u)
{
    if (feats_have_breath_units(u->features) && u->unk_14
        && strlist_contains(u->unk_14, "breath_insertion"))
        breath_insertion_utt(u);
    return u;
}

/* 0x6b084 arm: text analysis of phone input: each token is one word of the given phones. */
cst_utterance *phones_textanalysis(cst_utterance *u)
{
    cst_relation *wrel = utt_relation_create(u, REL_WORD);
    cst_item *t, *w;

    for (t = relation_head(utt_relation(u, REL_TOKEN)); t; t = item_next(t)) {
        item_set(t, "phones", phones_from_string(item_feat_string(t, "name")));
        w = item_add_daughter(t, NULL);
        item_set_string(w, "name", "");
        relation_append(wrel, w);
    }
    return u;
}
