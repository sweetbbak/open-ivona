/*
 * The state of one SSML document being read (0x2842c..0x28fcc).
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_string.h"
#include "cst_wave.h"
#include "ivona_util.h"
#include "ivona_ustr.h"
#include "dom.h"
#include "ivona_vector.h"
#include "tts_cert.h"
#include "tts_internal.h"
#include "tts_voice.h"
#include "ssml_doc.h"

cst_features *wavedict_new(void);
void wavedict_clear(cst_features *d);
void wavedict_free(cst_features *d);
void pls_lexicon_free(pls_lexicon *l);
pls_lexicon *pls_lexicon_parse(const char *data, size_t size, const char *lang, tts_msg **msgs,
                               int level);
const char *dom_get_attr(const dom_elem *e, const char *name);
void dom_remove_preceding(dom_node *n);
void dom_remove_following(dom_node *n);
ivona_ustr *ustr_transcode(const ivona_ustr *s, const char *encoding, int *state, size_t *pos);
int utf8_encode(unsigned int c, char *out);
int utf8_decode(const char *s, int *len);
int xml_is_name_start(unsigned int c);
int xml_is_name_char(unsigned int c);
const char *xml_entity_value(const char *name);
size_t cst_strnspn(const char *s, size_t n, const char *accept);
int cst_parse_hex_n(const char *s, size_t n, unsigned long *out);
int cst_parse_uint_n(const char *s, size_t n, unsigned long *out);


int ssml_parse_fragment(ssml_doc *doc, ivona_ustr *text, int a);

/* a piece of text after applying PLS lexicons */
typedef struct pls_piece {
    int type;                   /* PLS_PIECE_* */
    ivona_ustr *text;           /* +0x04: text, or the pronunciation / alias */
    ivona_ustr *grapheme;       /* +0x08: PLS_PIECE_PHONEME: the text replaced */
    cst_features *attrs;        /* +0x0c: PLS_PIECE_PHONEME: more <phoneme> attributes */
} pls_piece;

#define PLS_PIECE_TEXT 0
#define PLS_PIECE_PHONEME 1
#define PLS_PIECE_SSML 2

ivona_vector *pls_apply(void *lexicons, const ivona_ustr *text, int in_subdoc, const char *lang,
                        int external);
float metrics_add_time(cst_features *metrics, const char *name, int usec);
void *tts_get_certificate(tts_instance *tts);
lang_part *voice_lang_part(tts_voice *v, const char *name);

/* lex_apply.c */
typedef struct lex_applier lex_applier;
typedef struct lex_piece {
    ivona_ustr *text;
    int ssml;                   /* text is an SSML fragment */
    unsigned int next;          /* next dictionary, INT_MAX when done */
} lex_piece;
lex_applier *lex_applier_new(void *chain, int ssml, const ivona_ustr *input, unsigned int pos,
                             void *ctx);
void lex_applier_free(lex_applier *ap);
int lex_applier_run(lex_applier *ap);
int lex_applier_take(lex_applier *ap, lex_piece *out);

/* 0x2842c arm */
static void ssml_audio_free(void *p, size_t size)
{
    cst_free(((ssml_audio *)p)->src);
}

/* 0x28434 arm */
static void ssml_lexicon_free(void *p, size_t size)
{
    ssml_lexicon *l = p;

    cst_free(l->uri);
    cst_free(l->id);
    cst_free(l->lang);
    pls_lexicon_free(l->lexicon);
}

/* 0x28460 arm: flags e and every element below it as part of a subdocument (an ISRA clone
 * taking e->first and &e->in_subdoc). In the target its recursion is inlined into itself
 * eight levels deep, which `static inline` reproduces, but then GCC also inlines it into
 * ssml_parse_subdoc, which the target calls; plain static matches the caller instead. */
static void dom_mark_subdoc(dom_elem *e)
{
    dom_node *c;

    e->in_subdoc = 1;
    for (c = e->first; c; c = c->next)
        if (c->type != DOM_TEXT)
            dom_mark_subdoc((dom_elem *)c);
}

/* 0x28614 arm: the <mark> element named name below e; NULL if there is none or more than
 * one. */
static inline dom_node *ssml_find_mark(dom_elem *e, const char *name)
{
    dom_node *c, *r, *found = NULL;

    if (e->n.type == 9) {
        const char *s = dom_get_attr(e, "name");

        if (s && strcmp(name, s) == 0)
            return &e->n;
        return NULL;
    }
    for (c = e->first; c; c = c->next) {
        if (c->type == DOM_TEXT)
            continue;
        r = ssml_find_mark((dom_elem *)c, name);
        if (r) {
            if (found)
                return NULL;
            found = r;
        }
    }
    return found;
}

/* 0x28bac arm */
ssml_doc *ssml_doc_new(cst_features *metrics)
{
    ssml_doc *d = cst_safe_calloc(1, sizeof(ssml_doc));

    d->ssml_version = 0;
    d->dom = NULL;
    d->lexicons = vector_new(sizeof(ssml_lexicon), (ivona_vector_copy_fn)memcpy, ssml_lexicon_free);
    d->waves = wavedict_new();
    d->audio = vector_new(sizeof(ssml_audio), (ivona_vector_copy_fn)memcpy, ssml_audio_free);
    d->fetch = fetcher_new();
    d->metrics = metrics;
    d->is_ssml = 1;
    d->version = 2;
    return d;
}

/* 0x28c40 arm */
void ssml_doc_free(ssml_doc *d)
{
    if (d == NULL)
        return;
    wavedict_free(d->waves);
    vector_free(d->audio);
    tts_msg_free(d->warnings);
    tts_msg_free(d->warning);
    fetcher_delete(d->fetch);
    dom_free(d->dom);
    vector_free(d->lexicons);
    cst_free(d->encoding);
    cst_free(d);
}

/* 0x28c98 arm */
void ssml_doc_reset(ssml_doc *d)
{
    fetcher_reset(d->fetch);
    dom_free(d->dom);
    d->dom = NULL;
    vector_clear(d->lexicons);
    d->ssml_version = 0;
    cst_free(d->encoding);
    d->is_ssml = 1;
    d->encoding = NULL;
    vector_clear(d->audio);
    wavedict_clear(d->waves);
    tts_msg_free(d->warnings);
    d->warnings = NULL;
    tts_msg_free(d->warning);
    d->warning = NULL;
}

/* 0x28d04 arm */
void ssml_doc_set_voice(ssml_doc *d, tts_voice *v)
{
    d->entity_limit = get_param_int(VOICE_FEATURES(v), "eng_xml_entity_expansion_limit", 100);
}

/* 0x28d34 arm: readies a parsed document for v: default language and base URI, the fetch
 * and audio attribute callbacks, and the startmark/endmark range. */
int ssml_doc_prepare(ssml_doc *d, tts_voice *v)
{
    dom_doc *dom;
    dom_node *n;
    const char *s;

    if (dom_get_attr(d->dom->root, "http://www.w3.org/XML/1998/namespace^lang") == NULL)
        dom_set_attr(d->dom->root, "http://www.w3.org/XML/1998/namespace^lang",
                     v->voice->lang_codes);
    if (d->dom->name == NULL) {
        s = get_param_string(VOICE_FEATURES(v), "eng_ssml_base_uri", NULL);
        if (s && *s)
            d->dom->name = cst_strdup(s);
    }
    if (d->dom->name) {
        fetcher_set_base(d->fetch, d->dom->name);
        fetcher_set_referer(d->fetch, d->dom->name);
    }
    if (feat_present(VOICE_FEATURES(v), "eng_fetch_function")) {
        void *user = NULL;

        if (feat_present(VOICE_FEATURES(v), "eng_fetch_user_data"))
            user = val_userdata(feat_val(VOICE_FEATURES(v), "eng_fetch_user_data"));
        fetcher_set_callback(d->fetch, val_userdata(feat_val(VOICE_FEATURES(v), "eng_fetch_function")),
                             user);
    }
    if (feat_present(VOICE_FEATURES(v), "eng_audio_attributes_function")) {
        d->audio_attr_fn = val_userdata(feat_val(VOICE_FEATURES(v), "eng_audio_attributes_function"));
        if (feat_present(VOICE_FEATURES(v), "eng_audio_attributes_user_data"))
            d->audio_attr_data = val_userdata(feat_val(VOICE_FEATURES(v),
                                                       "eng_audio_attributes_user_data"));
    }
    dom = d->dom;
    if ((s = dom_get_attr(dom->root, "startmark")) != NULL && (n = ssml_find_mark(dom->root, s)) != NULL)
        dom_remove_preceding(n);
    if ((s = dom_get_attr(dom->root, "endmark")) != NULL && (n = ssml_find_mark(dom->root, s)) != NULL)
        dom_remove_following(n);
    return 1;
}

/* 0x28f9c arm */
void ssml_doc_set_encoding(ssml_doc *d, const char *encoding)
{
    if (d->encoding)
        cst_free(d->encoding);
    d->encoding = encoding ? cst_strdup(encoding) : NULL;
}

/* 0x28fcc arm: converts a text node from the document encoding to UTF-8 (stage 0 -> 1).
 * Undecodable trailing bytes are split off into a new text node after it. */
void ssml_text_decode(ssml_doc *d, dom_text *t)
{
    int state = t->state;
    size_t pos;
    ivona_ustr *s;

    if (state == 0) {
        pos = state;
        s = ustr_transcode(t->text, d->encoding, &d->version, &pos);
        if (pos < t->text->len) {
            dom_text *rest = dom_text_new();
            ustr_edit e;

            ustr_edit_init(&e, t->text);
            ustr_edit_skip(&e, pos);
            ustr_edit_keep(&e, t->text->len - pos);
            rest->state = state;
            rest->text = ustr_edit_result(&e);
            if (rest->text == NULL) {
                rest->text = t->text;
                t->text = NULL;
            }
            dom_insert_after(&t->n, &rest->n);
        }
        ivona_ustr_free(t->text);
        t->text = s;
        t->state = 1;
    }
}

/* 0x290a8 arm: expands character and entity references in a text node (stage 1 -> 2). */
void ssml_text_expand(ssml_doc *d, dom_text *t)
{
    const ivona_ustr *s;
    ivona_ustr *r;
    ustr_edit e;
    unsigned long code;
    size_t i, start, n, a;

    if (t->state != 1)
        return;
    if (d->is_ssml) {
        s = t->text;
        ustr_edit_init(&e, s);
        for (;;) {
            start = e.pos;
            for (i = start; i < s->len; i++)
                if (s->data[i] == '&')
                    break;
            if (i >= s->len)
                break;
            ustr_edit_keep(&e, i - start);
            if (i + 3 < s->len && s->data[i + 1] == '#') {
                a = i + 2;
                if (s->data[a] == 'x' || s->data[a] == 'X') {
                    n = cst_strnspn(s->data + i + 3, s->len - 3 - i, "0123456789ABCDEFabcdef");
                    if (n && n + i + 3 < s->len && s->data[n + i + 3] == ';'
                        && cst_parse_hex_n(s->data + i + 3, n, &code)) {
                        char buf[12];
                        int len = utf8_encode(code, buf);

                        ustr_edit_skip(&e, n + 4);
                        ustr_edit_insert_n(&e, buf, len);
                    } else
                        ustr_edit_keep(&e, n + 3);
                } else {
                    n = cst_strnspn(s->data + a, s->len - 2 - i, "0123456789");
                    if (n && n + a < s->len && s->data[n + a] == ';'
                        && cst_parse_uint_n(s->data + a, n, &code)) {
                        char buf[12];
                        int len = utf8_encode(code, buf);

                        ustr_edit_skip(&e, n + 3);
                        ustr_edit_insert_n(&e, buf, len);
                    } else
                        ustr_edit_keep(&e, n + 2);
                }
            } else if (i + 2 < s->len) {
                int clen;
                unsigned int c;
                size_t j;

                a = i + 1;
                c = utf8_decode(s->data + a, &clen);
                if (xml_is_name_start(c)) {
                    for (j = a + clen; j < s->len;) {
                        c = utf8_decode(s->data + j, &clen);
                        j += clen;
                        if (!xml_is_name_char(c))
                            break;
                    }
                    n = j - i;
                    if (n > 2) {
                        if (c != ';') {
                            ustr_edit_keep(&e, j - clen - i);
                            continue;
                        }
                        char *name = cst_strndup(s->data + a, n - 2);
                        const char *v;

                        if (d->dom->pi && (v = get_param_string(d->dom->pi, name, NULL)) != NULL) {
                            cst_free(name);
                            ustr_edit_skip(&e, n);
                        } else {
                            v = xml_entity_value(name);
                            cst_free(name);
                            ustr_edit_skip(&e, n);
                            if (v == NULL)
                                continue;
                        }
                        ustr_edit_insert(&e, v);
                    } else
                        ustr_edit_keep(&e, j - clen - i);
                } else
                    ustr_edit_keep(&e, 1);
            } else
                ustr_edit_keep(&e, 1);
        }
        ustr_edit_keep(&e, s->len - start);
        r = ustr_edit_result(&e);
        if (r) {
            ivona_ustr_free(t->text);
            t->text = r;
        }
    }
    t->state = 2;
}

/* 0x29458 arm */
void ssml_expand_tree(ssml_doc *d, dom_elem *e)
{
    dom_node *c;

    for (c = e->first; c; c = c->next)
        if (c->type == DOM_TEXT)
            ssml_text_expand(d, (dom_text *)c);
        else
            ssml_expand_tree(d, (dom_elem *)c);
}

/* 0x294a8 arm */
void dom_set_text_state(dom_elem *e, int state)
{
    dom_node *c;

    for (c = e->first; c; c = c->next)
        if (c->type == DOM_TEXT)
            ((dom_text *)c)->state = state;
        else
            dom_set_text_state((dom_elem *)c, state);
}

/* 0x294e8 arm: parses text as an SSML fragment and returns its root (NULL on errors, which
 * are added to d's warnings), its text nodes at the given stage. */
dom_elem *ssml_parse_subdoc(ssml_doc *d, ivona_ustr *text, int state)
{
    ssml_doc *sub = ssml_doc_new(d->metrics);
    dom_elem *root;
    tts_msg *w, **tail;

    if (ssml_parse_fragment(sub, text, 1) != 0) {
        const char *err = tts_errmsg();
        ustr_span span;

        ustr_span_of(text, 0, text->len, &span);
        tail = d->warnings ? &tts_msg_last(d->warnings)->next : &d->warnings;
        *tail = tts_msg_new(span.start, 0, 8, "Subdocument parse error: %s", err);
        ssml_doc_free(sub);
        return NULL;
    }
    sub->is_ssml = 1;
    dom_set_text_state(sub->dom->root, 1);
    ssml_expand_tree(sub, sub->dom->root);
    dom_set_text_state(sub->dom->root, state);
    dom_mark_subdoc(sub->dom->root);
    for (w = sub->warnings; w; w = w->next) {
        tail = d->warnings ? &tts_msg_last(d->warnings)->next : &d->warnings;
        *tail = tts_msg_new(w->line, w->column, w->code, "Parsed subdocument: %s", w->text);
    }
    root = sub->dom->root;
    sub->dom->root = NULL;
    ssml_doc_free(sub);
    return root;
}

/* 0x29644 arm: replaces a text node by the result of applying the lexicons to it: text nodes,
 * <phoneme> elements and parsed SSML fragments, all at the given stage. */
void ssml_apply_pls(void *lexicons, dom_text *t, ssml_doc *d, int state, int external)
{
    const char *lang = dom_inherited_attr(t->n.parent, "http://www.w3.org/XML/1998/namespace^lang");
    ivona_vector *v;
    pls_piece *p;
    dom_node *cur;

    v = pls_apply(lexicons, t->text, t->n.parent->in_subdoc != 0, lang ? lang : "", external);
    if (v) {
        ustr_clear(t->text);
        cur = &t->n;
        for (p = v->data; p != (pls_piece *)v->data + v->count; p++) {
            if (p->type == PLS_PIECE_TEXT) {
                dom_text *n;

                if (cur->type == DOM_TEXT && ((dom_text *)cur)->text->len == 0) {
                    n = (dom_text *)cur;
                    ivona_ustr_free(n->text);
                } else {
                    n = dom_text_new();
                    n->state = state;
                    dom_insert_after(cur, &n->n);
                    cur = &n->n;
                }
                n->text = p->text;
                p->text = NULL;
            } else {
                dom_elem *e;

                if (p->type == PLS_PIECE_SSML) {
                    e = ssml_parse_subdoc(d, p->text, t->state);
                    if (e == NULL)
                        continue;
                } else {
                    dom_text *n;
                    cst_featvalpair *a;

                    e = dom_element_new("http://www.w3.org/2001/10/synthesis^phoneme", 13);
                    n = dom_text_new();
                    dom_set_attr(e, "ph", p->text->data);
                    if (p->attrs)
                        for (a = p->attrs->head; a; a = a->next)
                            dom_set_attr(e, a->name, val_string(a->val));
                    n->state = state;
                    n->text = p->grapheme;
                    p->grapheme = NULL;
                    dom_append_child(e, &n->n);
                    memcpy(&e->span, &p->text->start, sizeof(ustr_span));
                    e->end_span.start = 0;
                    e->end_span.end = 0;
                }
                dom_insert_after(cur, &e->n);
                cur = &e->n;
            }
        }
        vector_free(v);
    }
    t->state = state;
}

/* 0x29888 arm */
void ssml_apply_internal_pls(void *lexicons, dom_text *t, ssml_doc *d)
{
    int t0 = d->metrics ? ivona_time_usec() : 0;
    cst_features *m;

    if (t->state < 4)
        ssml_apply_pls(lexicons, t, d, 4, 1);
    if ((m = d->metrics) != NULL)
        metrics_add_time(m, "ssml_apply_internal_pls_time", ivona_time_usec() - t0);
}

/* 0x29920 arm */
void ssml_apply_external_pls(void *lexicons, dom_text *t, ssml_doc *d)
{
    int t0 = d->metrics ? ivona_time_usec() : 0;
    cst_features *m;

    if (t->state < 3)
        ssml_apply_pls(lexicons, t, d, 3, 0);
    if ((m = d->metrics) != NULL)
        metrics_add_time(m, "ssml_apply_external_pls_time", ivona_time_usec() - t0);
}

/* 0x299b8 arm */
void dom_set_dict_pos(dom_elem *e, unsigned int pos)
{
    dom_node *c;

    for (c = e->first; c; c = c->next)
        if (c->type == DOM_TEXT)
            ((dom_text *)c)->dict_pos = pos;
        else
            dom_set_dict_pos((dom_elem *)c, pos);
}

/* applies a lexicon chain to a text node at the given stage, leaving it at the next one.
 * Pieces still to go through later lexicons of the chain stay at the stage. */
static inline void ssml_apply_lexicons(ssml_doc *d, void *chain, tts_instance *tts, dom_text *t,
                                       int stage)
{
    if (t->state == stage) {
        tts_cert_data *cert = tts_get_certificate(tts);
        lex_applier *ap = lex_applier_new(chain, cert ? cert->ssml : 0, t->text, t->dict_pos,
                                          d->metrics);
        lex_piece p;
        dom_node *cur;

        if (lex_applier_run(ap)) {
            ustr_clear(t->text);
            cur = &t->n;
            while (lex_applier_take(ap, &p)) {
                if (p.ssml == 0) {
                    dom_text *n;

                    if (cur->type == DOM_TEXT && ((dom_text *)cur)->text->len == 0) {
                        n = (dom_text *)cur;
                        ivona_ustr_free(n->text);
                    } else {
                        n = dom_text_new();
                        dom_insert_after(cur, &n->n);
                        cur = &n->n;
                    }
                    {
                        int done = p.next == INT_MAX;

                        n->dict_pos = done ? 0 : p.next;
                        n->state = stage + done;
                    }
                    n->text = p.text;
                    p.text = NULL;
                } else {
                    dom_elem *e = ssml_parse_subdoc(d, p.text, stage);

                    if (cur->type == DOM_TEXT && ((dom_text *)cur)->text->len == 0) {
                        ((dom_text *)cur)->state = stage + 1;
                        ((dom_text *)cur)->dict_pos = 0;
                    }
                    if (e) {
                        dom_set_dict_pos(e, p.next);
                        dom_insert_after(cur, &e->n);
                        cur = &e->n;
                    }
                    ivona_ustr_free(p.text);
                    p.text = NULL;
                }
            }
        } else {
            t->dict_pos = 0;
            t->state = stage + 1;
        }
        lex_applier_free(ap);
    }
}

/* 0x299f8 arm: applies the voice's user dictionaries to a text node (stage 4 -> 5). */
void ssml_apply_dict(ssml_doc *d, tts_voice *v, dom_text *t)
{
    int t0 = d->metrics ? ivona_time_usec() : 0;
    cst_features *m;

    ssml_apply_lexicons(d, v->dict, v->tts, t, 4);
    if ((m = d->metrics) != NULL)
        metrics_add_time(m, "ssml_apply_dict_time", ivona_time_usec() - t0);
}

/* 0x29bc4 arm: applies the PRM lexicons of the voice (stage 5 -> 6), then those of the
 * language of the text (6 -> 7). */
void ssml_apply_prm(ssml_doc *d, tts_voice *v, dom_text *t)
{
    lang_part *lp = voice_lang_part(v, dom_inherited_attr(t->n.parent,
                                       "http://www.w3.org/XML/1998/namespace^lang"));
    void *none = NULL;
    int t0 = d->metrics ? ivona_time_usec() : 0;
    lang_part *vox = VOICE_DATA(v)->vox;
    cst_features *m;

    ssml_apply_lexicons(d, vox ? vox->lexicons : &none, v->tts, t, 5);
    ssml_apply_lexicons(d, lp ? lp->lexicons : &none, v->tts, t, 6);
    if ((m = d->metrics) != NULL)
        metrics_add_time(m, "ssml_apply_prm_time", ivona_time_usec() - t0);
}

/* 0x29f5c arm: adds the lexicon given inline in element e (data, size bytes) to the
 * document; its parse warnings become document warnings. */
ssml_lexicon *ssml_add_inline_lexicon(ssml_doc *d, dom_elem *e, const char *data, size_t size)
{
    const char *id = dom_get_attr(e, "http://www.w3.org/XML/1998/namespace^id");
    ssml_lexicon lex;
    tts_msg *msgs, *w, **tail;

    if (d->ssml_version != 10 && id == NULL) {
        tail = d->warnings ? &tts_msg_last(d->warnings)->next : &d->warnings;
        *tail = tts_msg_new(e->span.start, e->n.index, 5,
                            "Missing attribute \"%s\" in element \"%s\"", "xml:id", e->name);
        return NULL;
    }
    memset(&lex, 0, sizeof lex);
    lex.uri = cst_strdup("");
    lex.id = cst_strdup(id);
    lex.lang = cst_strdup(dom_inherited_attr(e, "http://www.w3.org/XML/1998/namespace^lang"));
    lex.line = e->span.start;
    lex.unk_10 = e->end_span.end;
    lex.column = e->n.index;
    lex.loaded = 1;
    lex.lexicon = pls_lexicon_parse(data, size, lex.lang, &msgs, 0);
    for (w = msgs; w; w = w->next) {
        tail = d->warnings ? &tts_msg_last(d->warnings)->next : &d->warnings;
        *tail = tts_msg_new(w->line + e->span.end, lex.column + w->column - 1, w->code,
                            "Warning in inline lexicon \"%s\": %s", lex.id, w->text);
    }
    tts_msg_free(msgs);
    vector_push(d->lexicons, &lex);
    return d->lexicons->count ? vector_at(d->lexicons, d->lexicons->count - 1) : NULL;
}

typedef struct fetch_result {
    void *data;
    size_t size;
} fetch_result;
int fetcher_fetch(fetcher *f, const char *uri, float timeout, int maxage, int maxstale,
                  cst_features *metrics);
fetch_result *fetcher_result(fetcher *f);
tts_msg *fetcher_messages(fetcher *f);
void dom_element_clear(dom_elem *e);
int wavedict_has(cst_features *d, const char *name);
int wavedict_set(cst_features *d, const char *name, cst_wave *w);
int wave_parse_riff(cst_wave *w, const char *buf, int len);

#define FETCH_ABORTED 1
#define FETCH_OK 3

/* adds a warning about element e to the document */
#define SSML_WARN(d, e, code, ...) do {                                                   \
        tts_msg **tail_ = (d)->warnings ? &tts_msg_last((d)->warnings)->next : &(d)->warnings; \
        *tail_ = tts_msg_new((e)->span.start, (e)->n.index, (code), __VA_ARGS__);          \
    } while (0)

/* 0x2a16c arm: <sub>: replaces the content of e by its alias */
int ssml_sub_alias(ssml_doc *d, dom_elem *e)
{
    const char *alias = dom_get_attr(e, "alias");
    dom_text *t;
    ustr_span span;

    if (alias == NULL) {
        SSML_WARN(d, e, 5, "Missing attribute \"%s\" in element \"%s\"", "alias", e->name);
        return 0;
    }
    t = dom_text_new();
    span.start = e->span.end;
    span.end = e->end_span.start;
    if (span.start > span.end) {
        span.start = e->end_span.start;
        span.end = e->span.end;
    }
    t->text = ustr_from(alias, strlen(alias), &span);
    t->state = 2;
    dom_element_clear(e);
    dom_append_child(e, &t->n);
    return 1;
}

/* 0x2a264 arm: fetches and parses an external lexicon the first time it is used. Returns 0 if
 * the fetch was aborted. */
int ssml_lexicon_load(ssml_doc *d, ssml_lexicon *lex)
{
    tts_msg *msgs, *m;
    fetch_result *r;
    int status;

    if (lex->loaded)
        return 1;
    lex->loaded = 1;
    status = fetcher_fetch(d->fetch, lex->uri, lex->timeout, lex->maxage, lex->maxstale,
                           d->metrics);
    if (status == FETCH_OK) {
        r = fetcher_result(d->fetch);
        lex->lexicon = pls_lexicon_parse(r->data, r->size, lex->lang, &msgs, 0);
        for (m = msgs; m; m = m->next) {
            tts_msg **tail = d->warnings ? &tts_msg_last(d->warnings)->next : &d->warnings;
            *tail = tts_msg_new(lex->line, lex->column, m->code,
                                "Warning in lexicon \"%s\" in line %d: %s", lex->uri, m->column,
                                m->text);
        }
        tts_msg_free(msgs);
    } else if (status == FETCH_ABORTED) {
        tts_set_error(TTS_STATUS_USER_ABORTED);
        return 0;
    } else if ((m = fetcher_messages(d->fetch)) != NULL) {
        do {
            tts_msg **tail = d->warnings ? &tts_msg_last(d->warnings)->next : &d->warnings;
            *tail = tts_msg_new(lex->line, lex->column, m->code,
                                "Failed fetching lexicon file \"%s\": %s", lex->uri, m->text);
        } while ((m = m->next) != NULL);
    } else {
        tts_msg **tail = d->warnings ? &tts_msg_last(d->warnings)->next : &d->warnings;
        *tail = tts_msg_new(lex->line, lex->column, 10, "Failed fetching lexicon file \"%s\"",
                            lex->uri);
    }
    return 1;
}

/* 0x2a42c arm: a time attribute ("1.5s", "300ms") in seconds, -1 if invalid */
float ssml_time_attr(ssml_doc *d, dom_elem *e, const char *name)
{
    const char *value = dom_get_attr(e, name);
    float t;
    char unit[4];
    int n = sscanf(value, "%f%3s", &t, unit);

    if (n < 1) {
        SSML_WARN(d, e, 6, "Invalid value of attribute \"%s\"", name);
        return -1.0f;
    }
    if (t < 0) {
        SSML_WARN(d, e, 6, "Negative value not allowed for attribute \"%s\"", name);
        return -1.0f;
    }
    if (n == 1) {
        SSML_WARN(d, e, 6,
                  "Missing unit \"s\" or \"ms\" in value of attribute \"%s\". Assuming seconds.",
                  name);
        return t;
    }
    if (strcasecmp("s", unit) == 0)
        return t;
    if (strcasecmp("ms", unit) == 0)
        return t * 0.001f;
    SSML_WARN(d, e, 6, "Invalid unit in value of attribute \"%s\". Assuming seconds.", name);
    return t;
}

/* 0x2a618 arm: <lexicon>: adds the external lexicon e refers to (fetched on first use) */
ssml_lexicon *ssml_add_lexicon(ssml_doc *d, dom_elem *e)
{
    const char *uri = dom_get_attr(e, "uri");
    const char *id = dom_get_attr(e, "http://www.w3.org/XML/1998/namespace^id");
    const char *type = dom_get_attr(e, "type");
    const char *maxage, *maxstale;
    ssml_lexicon lex, *ret = NULL;
    float timeout;

    if (uri == NULL) {
        SSML_WARN(d, e, 5, "Missing attribute \"%s\" in element \"%s\"", "uri", e->name);
    } else if (d->ssml_version != 10 && id == NULL) {
        SSML_WARN(d, e, 5, "Missing attribute \"%s\" in element \"%s\"", "xml:id", e->name);
    } else if (type != NULL && strcasecmp(type, "application/pls+xml") != 0) {
        SSML_WARN(d, e, 3, "Unsupported lexicon type: \"%s\"", type);
    } else {
        maxage = dom_get_attr(e, "maxage");
        maxstale = dom_get_attr(e, "maxstale");
        if (dom_get_attr(e, "fetchtimeout") == NULL
            || !((timeout = ssml_time_attr(d, e, "fetchtimeout")) >= 0))
            timeout = 120.0f;
        memset(&lex, 0, sizeof lex);
        lex.uri = cst_strdup(uri);
        lex.id = cst_strdup(id);
        lex.lang = cst_strdup(dom_inherited_attr(e, "http://www.w3.org/XML/1998/namespace^lang"));
        lex.timeout = timeout;
        lex.maxage = maxage ? atoi(maxage) : -1;
        lex.maxstale = maxstale ? atoi(maxstale) : -1;
        lex.line = e->span.start;
        lex.unk_10 = e->span.end;
        lex.column = e->n.index;
        vector_push(d->lexicons, &lex);
        ret = d->lexicons->count ? vector_at(d->lexicons, d->lexicons->count - 1) : NULL;
    }
    return ret;
}

/* 0x2a914 arm: <audio>: fetches src into the document's waves, dropping the fallback content.
 * Returns 1, 0 if it failed (the content is kept), -1 if aborted. */
int ssml_audio_fetch(ssml_doc *d, dom_elem *e)
{
    const char *src = dom_get_attr(e, "src");
    const char *s_maxage, *s_maxstale;
    int maxage, maxstale, status;
    float timeout;
    tts_msg *m;
    fetch_result *r;
    cst_wave *w;

    if (src == NULL) {
        SSML_WARN(d, e, 5, "Missing attribute \"%s\" in element \"%s\"", "src", e->name);
        return 0;
    }
    if (!wavedict_has(d->waves, src)) {
        s_maxage = dom_get_attr(e, "maxage");
        s_maxstale = dom_get_attr(e, "maxstale");
        maxage = s_maxage ? atoi(s_maxage) : -1;
        maxstale = s_maxstale ? atoi(s_maxstale) : -1;
        if (dom_get_attr(e, "fetchtimeout") == NULL
            || !((timeout = ssml_time_attr(d, e, "fetchtimeout")) >= 0))
            timeout = 120.0f;
        status = fetcher_fetch(d->fetch, src, timeout, maxage, maxstale, d->metrics);
        if (status != FETCH_OK) {
            if (status == FETCH_ABORTED)
                return -1;
            if ((m = fetcher_messages(d->fetch)) == NULL) {
                SSML_WARN(d, e, 10, "Failed fetching audio file \"%s\"", src);
                return 0;
            }
            do
                SSML_WARN(d, e, m->code, "Failed fetching audio file \"%s\": %s", src, m->text);
            while ((m = m->next) != NULL);
            return 0;
        }
        r = fetcher_result(d->fetch);
        w = new_wave();
        if (wave_parse_riff(w, r->data, r->size) != 0) {
            delete_wave(w);
            SSML_WARN(d, e, 13, "Unsupported format of audio file \"%s\"", src);
            return 0;
        }
        wavedict_set(d->waves, src, w);
    }
    dom_element_clear(e);
    return 1;
}

/* the marks of a document in text order, consumed as synthesis passes them */
typedef struct mark_queue {
    ivona_vector *marks;        /* of ssml_audio */
    size_t next;
} mark_queue;

/* 0x2abe4 arm */
void mark_queue_init(mark_queue *q, ivona_vector *marks)
{
    q->marks = marks;
    q->next = 0;
}

/* 0x2abf0 arm: the next mark if it is at or before pos */
ssml_audio *mark_queue_pop(mark_queue *q, int pos)
{
    ssml_audio *m;

    if (q->marks && q->next < vector_size(q->marks)) {
        m = vector_get(q->marks, q->next);
        if (m->pos <= pos) {
            q->next++;
            return m;
        }
    }
    return NULL;
}
