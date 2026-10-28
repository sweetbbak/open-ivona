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
#include "cst_error.h"
#include "cst_val_defs.h"
#include "cst_string.h"
#include "cst_tokenstream.h"
#include "cst_regex.h"
#include <string.h>
#include "tts_metrics.h"
#include "tts_voice.h"
#include "ivona_util.h"
#include "ivona_vector.h"

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
    cst_features *f = u->features;
    unsigned long z = (unsigned long)__builtin_return_address(0)
                    - (unsigned long)__builtin_return_address(0);
    const char *k = "input_text" + z;
    if (!text) {
        __asm__("" :: "r"(k));
        text = "";
    }
    feat_set_string(f, k, text);
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
cst_utterance *t2t_tokens(cst_utterance *u);
cst_utterance *default_textanalysis(cst_utterance *u);
cst_utterance *default_phrasing(cst_utterance *u);
cst_utterance *default_lexical_insertion(cst_utterance *u);
cst_utterance *FUN_00069b88(cst_utterance *u);
cst_utterance *default_pause_insertion(cst_utterance *u);
cst_utterance *default_intonation(cst_utterance *u);
cst_utterance *phones_textanalysis(cst_utterance *u);

static const cst_synth_module text_methods[] = {
    { "tokenizer_func", "tokenizer_time", default_tokenization },
    { "t2t_func", "tokenstotokens_time", t2t_tokens },
    { "text_analysis_func", "text_analysis_time", default_textanalysis },
    { "pos_tagger_func", "pos_tagger_time", NULL },
    { "phrasing_func", "phrasing_time", default_phrasing },
    { "lettertosound_func", "lettertosound_time", default_lexical_insertion },
    { "change_language_func", "change_language_time", FUN_00069b88 },
    { "pause_insertion_func", "pause_insertion_time", default_pause_insertion },
    { "postlex_func", "postlex_time", NULL },
    { "intonation_func", "intonation_time", default_intonation },
    { "acoustic_prediction_func", "acoustic_prediction_time", NULL },
    { NULL, NULL, NULL }
};

/* the same without tokenization (the utterance has its tokens) */
static const cst_synth_module token_methods[] = {
    { "tokenizer_func", "tokenizer_time", NULL },
    { "t2t_func", "tokenstotokens_time", t2t_tokens },
    { "text_analysis_func", "text_analysis_time", default_textanalysis },
    { "pos_tagger_func", "pos_tagger_time", NULL },
    { "phrasing_func", "phrasing_time", default_phrasing },
    { "lettertosound_func", "lettertosound_time", default_lexical_insertion },
    { "change_language_func", "change_language_time", FUN_00069b88 },
    { "pause_insertion_func", "pause_insertion_time", default_pause_insertion },
    { "postlex_func", "postlex_time", NULL },
    { "intonation_func", "intonation_time", default_intonation },
    { "acoustic_prediction_func", "acoustic_prediction_time", NULL },
    { NULL, NULL, NULL }
};

/* input given as phones */
static const cst_synth_module phones_methods[] = {
    { "tokenizer_func", "tokenizer_time", default_tokenization },
    { "t2t_func", "tokenstotokens_time", t2t_tokens },
    { "text_analysis_func", "text_analysis_time", phones_textanalysis },
    { "pos_tagger_func", "pos_tagger_time", NULL },
    { "phrasing_func", "phrasing_time", default_phrasing },
    { "lettertosound_func", "lettertosound_time", default_lexical_insertion },
    { "change_language_func", "change_language_time", FUN_00069b88 },
    { "pause_insertion_func", "pause_insertion_time", default_pause_insertion },
    { "postlex_func", "postlex_time", NULL },
    { "intonation_func", "intonation_time", default_intonation },
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
cst_featvalpair *feat_find_featpair(const cst_features *f, const char *name);
cst_item *val_item(const cst_val *v);

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
        goto ret;
    ts->p_singlecharsymbols = "#";
    ts->p_prepunctuationsymbols = "";
    ts->p_postpunctuationsymbols = "";
    if (__builtin_expect(strcmp("IVONA_Lexicon", ts_get(ts)) != 0, 0)) {
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
ret:
    return r;
}

/* 0x66ae4 arm: the entry of word (with part of speech pos) in a lexicon list. */
const cst_val *val_in_lex(const cst_val *entries, const char *word, const char *pos)
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
void str_rewrite_span(const char *s, int start, int len, const void *rules, int flags, char *out);

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
        str_rewrite_span(ctx.data, start, len, rules, 0, buf);
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

/* compact duration CART: z-scores of phone durations */
typedef struct zdur_phone {
    unsigned char name;         /* index into names, 0xff ends the table */
    unsigned char pad;
    unsigned short mean;
    unsigned short stddev;
} zdur_phone;

typedef struct zdur_node {
    unsigned char feat;         /* index of "<op><feature path>"; 0xff for a leaf */
    unsigned char value;        /* index of the value compared with */
    unsigned short next;        /* node if the question fails; a leaf's z-score (x1000) */
} zdur_node;

typedef struct zdur_cart {
    const zdur_phone *phones;
    const char (*names)[64];
    const zdur_node *nodes;
} zdur_cart;

extern const int cst_val_type_zdur_cart;

/* 0x67f94 arm, 0x67fec arm */
CST_VAL_REGISTER_TYPE_NODEL(zdur_cart, zdur_cart)

/* 0x681c0 arm: qsort order of ints */
static int int_cmp(const void *a, const void *b)
{
    return *(const int *)a - *(const int *)b;
}


/* 0x68004 arm: the duration of seg in ms */
short zdur_cart_duration(const zdur_cart *dc, const cst_item *seg)
{
    const char *name = item_feat_string(seg, "name");
    const zdur_phone *p = dc->phones;
    const zdur_phone *q;
    const zdur_node *n;
    const char *f, *val;
    const cst_val *v;
    char op;
    int r;
    short z;

    for (q = p; q->name != 0xff; q++)
        if (strcmp(name, dc->names[q->name]) == 0) {
            p = q;
            break;
        }
    for (n = dc->nodes; n->feat != 0xff; ) {
        f = dc->names[n->feat];
        op = f[0];
        val = dc->names[n->value];
        v = ffeature(seg, f + 1);
        if (__builtin_expect(op == '<', 0) || __builtin_expect(op == '>', 0)) {
            if (op == '>')
                r = val_float(v) > cst_atof(val);
            else
                r = val_float(v) < cst_atof(val);
        }
        else if (CST_VAL_TYPE(v) == CST_VAL_TYPE_INT)
            r = val_int(v) == atoi(val);
        else
            r = strcmp(val_string(v), val) == 0;
        if (r)
            n++;
        else
            n = dc->nodes + n->next;
    }
    z = n->next;
    return (z * (short)p->stddev + (z > 0 ? 500 : -500)) / 1000 + p->mean;
}

/* 0x681d0 arm: renames relation from to to in the relation lists of i and its descendants */
static inline void item_rename_relation(cst_item *i, const char *from, const char *to)
{
    cst_featvalpair *fp = feat_find_featpair(i->contents->relations, from);
    cst_item *d;

    cst_dbgmsg("", val_item(fp->val), feat_find_featpair(i->contents->relations, to));
    fp->name = to;
    for (d = item_daughter(i); d; d = item_next(d))
        item_rename_relation(d, from, to);
}

/* ------------------------------------------------------------------------------------------ */
/* Front end modules */

#define REL_TOKEN 1
#define REL_WORD 2
#define ITEM_DAUGHTERN 0x108

/* tokenizer.c */
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
void *item_lang_data(const cst_item *i);

/* 0x684f8 arm: the input text's tokens as the Token relation */
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

/* 0x6864c arm: runs the "t2t_func" of every language present among the tokens, once each */
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

/* 0x68724 arm: the first c in s not escaped by a backslash */
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

/* 0x68780 arm: s[from, to) with backslash escapes removed (a newly allocated string) */
char *str_unescape_range(const char *s, size_t from, size_t to)
{
    strbuf b;
    char *r;
    size_t i = from;
    int n = 0;

    strbuf_init(&b);
    strbuf_reserve(&b, to);
    if (s) {
        for (; i < to && s[i]; i++) {
            n++;
            if (s[i] != '\\') {
                strbuf_append(&b, s + i, 1);
                n = 0;
            } else if (!(n & 1))
                strbuf_append(&b, s + i, 1);
        }
    }
    r = strbuf_data(&b);
    if (r == NULL)
        r = cst_strdup("");
    return r;
}

typedef cst_val *(*cst_valfunc)(const cst_val *v);
cst_valfunc val_valfunc(const cst_val *v);
char *item_feat_string_opt(const cst_item *i, const char *name);
cst_features *val_features(const cst_val *v);

/* 0x6886c arm: the words of tokens [t, end): phone and X-SAMPA tokens are one word each, ruby
 * tokens go through "rubytowords_func", others through the token's language's (or the
 * utterance's) "tokentowords_func"; "?^" and "?_" endings set a rising or falling end tone. */
cst_utterance *textanalysis_tokens(cst_utterance *u, cst_item *t, cst_item *end)
{
    const cst_val *ttw = feat_val(u->features, "tokentowords_func");
    const cst_val *mwf = feat_val(u->features, "makewordwithfeats_func");
    const cst_val *rtw = feat_val(u->features, "rubytowords_func");
    cst_relation *wrel = utt_relation_create(u, REL_WORD);
    lang_core *ld;
    const cst_val *v;
    cst_val *words, *nl, *w;
    const cst_val *p;
    cst_valfunc mf;
    cst_item *wi;
    char *name;
    size_t len;

    for (; t && t != end; t = item_next(t)) {
        ld = item_lang_data(t);
        if (item_feat(t, "phones") || item_feat(t, "xsampa")) {
            name = item_feat_string_opt(t, "name");
            if (name == NULL)
                name = "";
            words = cons_val(string_val(name), NULL);
        } else {
            if (item_feat_present(t, "ruby")) {
                v = ld ? feat_val(ld->features, "rubytowords_func") : rtw;
                if (v) {
                    words = val_itemfunc(v)(t);
                    goto add;
                }
            }
            name = item_feat_string_opt(t, "name");
            if (name == NULL || name[0] == '\0') {
                words = NULL;
                goto next;
            }
            len = strlen(name);
            if (len > 2) {
                const char *tone = NULL;
                if (strcmp(name + len - 2, "?^") == 0)
                    tone = "H";
                else if (strcmp(name + len - 2, "?_") == 0)
                    tone = "L";
                if (tone) {
                    item_set_string(t, "endtone", tone);
                    item_set_string(t, "punc", "?");
                    name[len - 2] = '\0';
                }
            }
            if (ld == NULL) {
                if (ttw)
                    words = val_itemfunc(ttw)(t);
                else
                    words = cons_val(string_val(name), NULL);
            } else {
                v = feat_val(ld->features, "tokentowords_func");
                if (v == NULL) {
                    words = NULL;
                    goto next;
                }
                words = val_itemfunc(v)(t);
            }
            if (words && mwf) {
                mf = val_valfunc(mwf);
                nl = NULL;
                for (w = words; w; w = (cst_val *)val_cdr(w)) {
                    v = mf(val_car(w));
                    if (v == val_car(w))
                        set_car(w, NULL);
                    nl = cons_val(v, nl);
                }
                delete_val(words);
                words = val_reverse(nl);
            }
        }
add:
        for (p = words; p; p = val_cdr(p)) {
            wi = item_add_daughter(t, NULL);
            if (cst_val_consp(val_car(p))) {
                item_set_string(wi, "name", val_string(val_car(val_car(p))));
                feat_copy_into(val_features(val_cdr(val_car(p))), item_feats(wi));
            } else
                item_set_string(wi, "name", val_string(val_car(p)));
            if (item_feat_string(wi, "name"))
                relation_append(wrel, wi);
            else
                delete_item(wi);
        }
next:
        delete_val(words);
    }
    return u;
}

/* 0x68d10 arm */
cst_utterance *default_textanalysis(cst_utterance *u)
{
    return textanalysis_tokens(u, relation_head(utt_relation(u, REL_TOKEN)), NULL);
}

const char *utf8_last_char(const char *s);
const char *utf8_strchr(const char *s, int c);
int utf8_decode(const char *s, int *len);

/* 0x68d38 arm: groups the words of tokens [t, end) into phrases, ending a phrase where
 * "phrase_boundary_func" says, or at sentence (BB) and clause (B) punctuation, spelled tokens
 * (SB), short pause marks and blank lines; silences and audio clips end phrases too. */
cst_utterance *phrasing_tokens(cst_utterance *u, cst_item *t, cst_item *end)
{
    const cst_val *pbf = feat_val(u->features, "phrase_boundary_func");
    cst_relation *prel = utt_relation_create(u, REL_PHRASE);
    cst_item *ph = NULL, *w;
    lang_core *ld;
    const cst_val *f, *v;
    const char *punc, *ws, *p, *b;
    int c;

    for (; t && t != end; t = item_next(t)) {
        for (w = item_daughter(t); w; w = item_next(w)) {
            if (ph == NULL) {
                ph = relation_append(prel, NULL);
                item_set_string(ph, "name", "BB");
            }
            item_add_daughter(ph, w);
        }
        if (ph == NULL)
            goto reset;
        ld = item_lang_data(t);
        f = ld ? feat_val(ld->features, "phrase_boundary_func") : pbf;
        if (f) {
            v = val_ffunc(f)(t);
            if (v == NULL || (b = val_string(v)) == NULL)
                goto check;
            goto set;
        }
        punc = ffeature_string(item_traverse(t, 0), "punc");
        if (ffeature_int(item_traverse(t, 0), "spelling_break") == 1) {
            b = "SB";
            goto set;
        }
        if (punc[0]) {
            v = feat_val(item_utt(t)->features, "encoding");
            if (v && strcmp(val_string(v), "UTF-8") == 0) {
                c = utf8_decode(utf8_last_char(punc), NULL);
                if (utf8_strchr("?!\u2047\u2048\u2049\uff1f.\u3002\uff0e\u2024\u2025\u2026\u203c!\uff01", c))
                    b = "BB";
                else if (utf8_strchr(",\u3001\uff0c;:\u037e\uff1a", c))
                    b = "B";
                else
                    b = NULL;
            } else {
                c = (unsigned char)punc[strlen(punc) - 1];
                if (strchr(".?!", c))
                    b = "BB";
                else if (strchr(",;:", c))
                    b = "B";
                else
                    b = NULL;
            }
            if (b)
                goto set;
        }
        if (ffeature_int(item_traverse(item_last_daughter(t), 0), "shortPause") == 1) {
            b = "B";
            goto set;
        }
        ws = ffeature_string(item_traverse(t, 0x100, 0), "whitespace");
        if ((p = strchr(ws, '\n')) != NULL && strchr(p + 1, '\n') != NULL) {
            b = "BB";
            goto set;
        }
        v = feat_val(item_utt(t)->features, "encoding");
        if (v == NULL || strcmp(val_string(v), "UTF-8") != 0)
            goto check;
        if (utf8_strchr(ffeature_string(item_traverse(t, 0x100, 0), "whitespace"), 0x2029) == NULL)
            goto check;
        b = "BB";
set:
        item_set_string(ph, "name", b);
        ph = NULL;
check:
        if (item_feat_present(t, "silence") || item_feat_present(t, "wave"))
reset:
            ph = NULL;
    }
    return u;
}

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

#define REL_SYLLABLE 5

void item_delete_range(cst_item *from, cst_item *to);

/* 0x69a30 arm: the X-SAMPA of word w into out; w's syllables, segments and SylStructure item
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

/* 0x6a078 arm: appends a segment named name after the last segment of the last word of w
 * that has one. (Declared int without a value, which keeps the last call from being a
 * sibcall.) */
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

/* 0x6a564 arm */
cst_utterance *breath_insertion(cst_utterance *u)
{
    if (feats_have_breath_units(u->features) && u->unk_14
        && strlist_contains(u->unk_14, "breath_insertion"))
        breath_insertion_utt(u);
    return u;
}

#define REL_PHRPUN 8

const char *cst_relation_id_name(int id);
static inline void item_rename_relation(cst_item *i, const char *from, const char *to);
typedef struct dnn_models {
    void *unk_0;
    void *duration;             /* +0x04 */
    void *unk_8;
    void *phrasing;             /* +0x0c */
} dnn_models;
dnn_models *voice_dnn_models(cst_utterance *u);
int dnn_stub_44(void *model);

/* 0x6a5b8 arm: keeps the punctuation phrases as PhrPun and rebuilds Phrase from them, with
 * silence before the first segment and after each phrase. */
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

#define ITEM_PARENT 0x106

const cst_val *item_feat_func(const cst_item *i, const char *name);

/* 0x6a84c arm: the end tone of each phrase's last syllable: its token's "endtone" if H or L,
 * else what "phrasesylendtone_func" says for the phrase, else L. */
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

/* 0x6a9e0 arm: leaves durations to the DNN acoustic model */
cst_utterance *duration_none(cst_utterance *u)
{
    return u;
}

cst_utterance *cart_duration(cst_utterance *u);
dnn_models *val_dnn_models(const cst_val *v);

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

/* 0x6a9e4 arm: segment durations from the duration CART (or the z-score CART), with pauses set
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

/* 0x6af2c arm: CART durations unless a DNN predicts them (hybrid mode, a DNN-forced token or
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

/* 0x6b084 arm: text analysis of phone input: each token is one word of the given phones. */
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
