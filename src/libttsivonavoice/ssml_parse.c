/*
 * The SSML parser (0x2ac44..0x2cda8, with ssml_element_type in .text.unlikely at 0x185b8):
 * Expat callbacks that build the document's DOM, keeping the input offsets of every node.
 */
#include <ctype.h>
#include <string.h>
#include "expat.h"
#include "cst_alloc.h"
#include "cst_string.h"
#include "ivona_tts.h"
#include "ivona_ustr.h"
#include "ssml_doc.h"
#include "tts_internal.h"

int cst_startswith_nocase(const char *s, const char *prefix);
char *cst_strncpy(char *dst, const char *src, size_t n);
int utf8_decode(const char *s, int *len);
int xml_is_space(unsigned int c);
ivona_ustr *xml_clean_chars(const ivona_ustr *u);
ivona_vector *xml_ns_stack_new(void);
void xml_ns_push(ivona_vector *stack, const char *prefix, const char *uri);
const char *xml_ns_lookup(ivona_vector *stack, const char *prefix);
char *xml_ns_qualify(ivona_vector *stack, const char *name);
const char *cst_skip_space(const char *s);
char *cst_rtrim_dup(const char *s);
int xml_unknown_encoding(void *data, const char *name, XML_Encoding *info);
int dom_set_pi(dom_doc *d, const char *name, const char *value);
dom_node *dom_last_child(const dom_elem *e);
const char *dom_get_attr(const dom_elem *e, const char *name);

#define SSML_NS "http://www.w3.org/2001/10/synthesis"
#define AMAZON_NS "https://amazon.com/ssml/2017-01-01/"
#define IVONA_NS "http://www.ivona.com/2009/12/ssml"


typedef struct ssml_parser {
    XML_Parser xp;
    ssml_doc *doc;              /* +0x04 */
    dom_doc *dom;               /* +0x08 */
    dom_elem *cur;              /* +0x0c: the open element */
    int skip;                   /* +0x10: depth inside skipped elements */
    int started;                /* +0x14: an element was seen */
    ivona_vector *ns;           /* +0x18: prefix bindings */
    ivona_vector *ns2;          /* +0x1c */
    const ivona_ustr *input;    /* +0x20 */
    ivona_ustr *cleaned;        /* +0x24 */
    int text_end;               /* +0x28: input offset after the last text */
    int no_ns_warned;           /* +0x2c */
    int unbound_warned;         /* +0x30 */
    int lenient;                /* +0x34: no warnings about missing attributes */
    int wrapped;                /* +0x38: <speak> was added around the input */
    int root_warned;            /* +0x3c */
    int lookup_warned;          /* +0x40 */
    int lexicon_start;          /* +0x44: input offset of an amazon:lexicon's content */
} ssml_parser;

/* an element or attribute name from Expat: "uri^local^prefix" */
typedef struct qname {
    const char *full;
    const char *uri;            /* +0x04 */
    int uri_len;                /* +0x08 */
    const char *local;          /* +0x0c */
    int local_len;              /* +0x10 */
    const char *prefix;         /* +0x14 */
    int no_ns;                  /* +0x18: had no namespace */
    int unbound;                /* +0x1c: had an unbound prefix */
} qname;

/* 0x2ac50 arm: the input bytes of the current Expat event */
static inline void ssml_span(ssml_parser *p, const ivona_ustr *const *input, ustr_span *span)
{
    XML_Index i = XML_GetCurrentByteIndex(p->xp);

    ustr_span_of(*input, i < 0 ? 0 : i, XML_GetCurrentByteCount(p->xp), span);
}

/* appends a message to list */
#define TTS_MSG_ADD_AT(list, line, column, code, ...)                                 \
    do {                                                                              \
        tts_msg **tail_ = (list) ? &tts_msg_last(list)->next : &(list);               \
                                                                                      \
        *tail_ = tts_msg_new((line), (column), (code), __VA_ARGS__);                  \
    } while (0)

/* adds a warning at span to doc */
#define SSML_MSG_ADD(p, doc, span, code, ...)                                         \
    do {                                                                              \
        tts_msg **tail_ = (doc)->warnings ? &tts_msg_last((doc)->warnings)->next      \
                                          : &(doc)->warnings;                         \
                                                                                      \
        *tail_ = tts_msg_new((span).start, XML_GetCurrentLineNumber((p)->xp), (code), \
                             __VA_ARGS__);                                            \
    } while (0)

/* adds a warning at the current event to doc */
#define SSML_WARN_TO(p, doc, code, ...)                                               \
    do {                                                                              \
        ustr_span span_;                                                              \
                                                                                      \
        ssml_span(p, &(p)->input, &span_);                                            \
        SSML_MSG_ADD(p, doc, span_, code, __VA_ARGS__);                               \
    } while (0)
#define SSML_WARN(p, code, ...) SSML_WARN_TO(p, (p)->doc, code, __VA_ARGS__)

/* 0x2ac44 arm */
static void ssml_default(void *data, const XML_Char *s, int len)
{
}

/* 0x2ac48 arm */
static void ssml_end_ns(void *data, const XML_Char *prefix)
{
    vector_pop(((ssml_parser *)data)->ns);
}

/* 0x2ac90 arm: internal entities become processing instruction attributes of the DOM. */
static void ssml_entity_decl(void *data, const XML_Char *name, int is_param, const XML_Char *value,
                             int len, const XML_Char *base, const XML_Char *system_id,
                             const XML_Char *public_id, const XML_Char *notation)
{
    ssml_parser *p = data;
    ssml_doc *doc = p->doc;
    int limit = doc->entity_limit;

    if (limit < len) {
        SSML_WARN_TO(p, doc, 9, "XML entity declaration failed. Expansion length exceeds the limit of %d bytes.",
                  limit);
        return;
    }
    if (!is_param && name && value) {
        char *v = cst_strndup(value, len);

        dom_set_pi(p->dom, name, v);
        cst_free(v);
    }
}

/* 0x2ad80 arm */
static void ssml_start_ns(void *data, const XML_Char *prefix, const XML_Char *uri)
{
    ssml_parser *p = data;

    if (strcmp(uri, IVONA_NS) == 0)
        SSML_WARN(p, 7, "The \"%s\" namespace is deprecated. Please use \"" AMAZON_NS "\" instead.",
                  uri);
    xml_ns_push(p->ns, prefix, uri);
}

/* whether an element of type parent may contain one of type child (DOM_TEXT for text) */
static inline int ssml_may_contain(int parent, int child)
{
    if (parent == SSML_UNKNOWN_SSML || parent == SSML_UNKNOWN)
        return 1;
    if (parent == SSML_METADATA)
        return 1;
    switch (child) {
    case DOM_TEXT:
        if (parent == SSML_SAY_AS || parent == SSML_PHONEME || parent == SSML_SUB)
            return 1;
        if (parent == SSML_DESC)
            return 1;
        /* fall through */
    case SSML_AUDIO:
    case SSML_BREAK:
    case SSML_EMPHASIS:
    case SSML_MARK:
    case SSML_PHONEME:
    case SSML_PROSODY:
    case SSML_SAY_AS:
    case SSML_SUB:
    case AMAZON_BREATH:
    case AMAZON_DOMAIN:
    case AMAZON_EFFECT:
        if (parent == SSML_W)
            return 1;
        /* fall through */
    case SSML_LANG:
    case SSML_LOOKUP:
    case SSML_VOICE:
    case SSML_W:
        if (parent == SSML_EMPHASIS || parent == SSML_S)
            return 1;
        if (parent == AMAZON_LITERAL)
            return 1;
        /* fall through */
    case SSML_S:
    case AMAZON_LITERAL:
        if (parent == SSML_P)
            return 1;
        /* fall through */
    case SSML_P:
        if (parent == SSML_AUDIO || parent == AMAZON_DOMAIN || parent == SSML_LANG
            || parent == AMAZON_EFFECT)
            return 1;
        if (parent == SSML_LOOKUP || parent == SSML_PROSODY)
            return 1;
        if (parent == SSML_VOICE)
            return 1;
        /* fall through */
    case SSML_LEXICON:
    case SSML_META:
    case SSML_METADATA:
    case AMAZON_LEXICON:
        return parent == SSML_SPEAK;
    case SSML_DESC:
        return parent == SSML_AUDIO;
    default:
        return 0;
    }
}

/* appends the current event's input bytes to the open element */
static inline void ssml_add_text(ssml_parser *p)
{
    XML_Index i = XML_GetCurrentByteIndex(p->xp);
    int start = i < 0 ? 0 : i;
    int n = XML_GetCurrentByteCount(p->xp);
    dom_node *last = dom_last_child(p->cur);

    if (last && last->type == DOM_TEXT && start == p->text_end)
        ustr_append_ustr(((dom_text *)last)->text, p->input, start, n);
    else {
        dom_text *t = dom_text_new();

        t->text = ustr_new(start, NULL, 0);
        ustr_append_ustr(t->text, p->input, start, n);
        t->state = 0;
        t->n.index = XML_GetCurrentLineNumber(p->xp);
        dom_append_child(p->cur, &t->n);
    }
    p->text_end = start + n;
}

/* text s (len bytes) where none is allowed: only whitespace is fine */
#define SSML_STRAY_TEXT(p, s, len)                                                    \
    do {                                                                              \
        if ((len) > 0 && strchr(" \t\n\r", (s)[len]) == NULL)                          \
            SSML_WARN(p, 7, "Text not allowed here");                                 \
    } while (0)

/* text s (len bytes, the current event) in the open element */
static inline void ssml_text(ssml_parser *p, const char *s, int len)
{
    if (p->wrapped && !p->root_warned) {
        p->root_warned = 1;
        SSML_WARN(p, 7, "Root element missing. Expecting <speak>.");
    }
    if (p->skip > 0)
        return;
    if (p->cur && ssml_may_contain(p->cur->n.type, DOM_TEXT)) {
        /* <desc> text is dropped */
        if (p->cur->n.type != SSML_DESC)
            ssml_add_text(p);
    } else if (len > 0 && strchr(" \t\n\r", s[len]) == NULL)
        /* only whitespace is fine */
        SSML_WARN(p, 7, "Text not allowed here");
}

/* 0x2ae44 arm */
static void ssml_chardata(void *data, const XML_Char *s, int len)
{
    ssml_text(data, s, len);
}

/* 0x2b0e4 arm: an undefined entity stays as text */
static void ssml_skipped_entity(void *data, const XML_Char *name, int is_param)
{
    size_t n = strlen(name);
    char *s = cst_safe_alloc(n + 2, 1);

    s[0] = '&';
    memcpy(s + 1, name, n);
    s[n + 1] = ';';
    ssml_text(data, s, n + 2);
    cst_free(s);
}

/* whether q is in namespace ns */
static inline int qname_in_ns(const qname *q, const char *ns)
{
    const char *uri = q->uri;

    return q->uri_len == strlen(ns) && cst_startswith(uri, ns);
}

/* whether q is element name in namespace ns, warning about wrong case */
static inline int ssml_name_is(const qname *q, ssml_parser *p, const char *ns, const char *name)
{
    const char *uri = q->uri;

    if (q->uri_len != strlen(ns) || !cst_startswith(uri, ns) || q->local_len != strlen(name))
        return 0;
    if (cst_startswith(q->local, name))
        return 1;
    if (cst_startswith_nocase(q->local, name)) {
        SSML_WARN(p, 7, "SSML element names should be lowercase");
        return 1;
    }
    return 0;
}

/* 0x2b3d0 arm */
static int ssml_is_amazon(const qname *q, ssml_parser *p, const char *name)
{
    if (!ssml_name_is(q, p, AMAZON_NS, name))
        return 0;
    if (q->unbound && !p->unbound_warned) {
        ustr_span span;

        ssml_span(p, &p->input, &span);
        p->unbound_warned = 1;
        SSML_MSG_ADD(p, p->doc, span, 7, "Unbound XML namespace prefix for element \"%s\". Assuming "
                     AMAZON_NS " for this element.", q->full);
    }
    return 1;
}

static const struct ssml_element_name {
    const char *name;
    int type;
} ssml_elements[] = {
    { "audio", SSML_AUDIO },
    { "break", SSML_BREAK },
    { "emphasis", SSML_EMPHASIS },
    { "mark", SSML_MARK },
    { "phoneme", SSML_PHONEME },
    { "prosody", SSML_PROSODY },
    { "say-as", SSML_SAY_AS },
    { "sub", SSML_SUB },
    { "voice", SSML_VOICE },
    { "lang", SSML_LANG },
    { "w", SSML_W },
    { "s", SSML_S },
    { "p", SSML_P },
    { "lookup", SSML_LOOKUP },
    { "lexicon", SSML_LEXICON },
    { "speak", SSML_SPEAK },
    { "meta", SSML_META },
    { "metadata", SSML_METADATA },
    { "desc", SSML_DESC },
    { "token", SSML_W },
    { "sentence", SSML_S },
    { "paragraph", SSML_P },
    { 0, 0 },
};

/* 0x185b8 arm: the type of an element (its uri resolved). Only called on rare paths, so GCC
 * keeps it in .text.unlikely and does not inline the span helper into its own warnings. */
static __attribute__((cold)) int ssml_element_type(const qname *q, ssml_parser *p)
{
    const struct ssml_element_name *e;
    ustr_span span;

    if (ssml_is_amazon(q, p, "breath"))
        return AMAZON_BREATH;
    if (!qname_in_ns(q, SSML_NS))
        return SSML_UNKNOWN;
    for (e = ssml_elements; e->name; e++) {
        if (!ssml_name_is(q, p, SSML_NS, e->name))
            continue;
        if (q->no_ns && !p->no_ns_warned && !(e->type == SSML_SPEAK && p->wrapped)) {
            ssml_span(p, &p->input, &span);
            p->no_ns_warned = 1;
            SSML_MSG_ADD(p, p->doc, span, 7, "Element \"%s\" not placed in any namespace. Assuming "
                         SSML_NS " for all such elements.", q->full);
        } else if (q->unbound && !p->unbound_warned) {
            ssml_span(p, &p->input, &span);
            p->unbound_warned = 1;
            SSML_MSG_ADD(p, p->doc, span, 7, "Unbound XML namespace prefix for element \"%s\". "
                         "Assuming " SSML_NS " for this element.", q->full);
        }
        return e->type;
    }
    return SSML_UNKNOWN_SSML;
}

/* splits an Expat name, replacing the old IVONA namespace */
static inline void qname_split(qname *q, const char *name)
{
    const char *sep = strchr(name, '^');

    q->full = name;
    q->no_ns = 0;
    q->unbound = 0;
    if (sep) {
        const char *local = sep + 1;
        const char *sep2 = strchr(local, '^');
        size_t uri_len = sep - name;

        q->local = local;
        q->uri_len = uri_len;
        q->uri = name;
        if (sep2) {
            q->local_len = sep2 - local;
            q->prefix = sep2 + 1;
        } else {
            q->prefix = NULL;
            q->local_len = strlen(local);
        }
        if (q->uri && strncmp(q->uri, IVONA_NS, uri_len) == 0) {
            q->uri = AMAZON_NS;
            q->uri_len = strlen(AMAZON_NS);
        }
    } else {
        q->local = name;
        q->local_len = strlen(name);
        q->uri = NULL;
        q->uri_len = 0;
        q->prefix = NULL;
    }
}

/* the namespace an element name stands for */
static inline void qname_resolve(qname *q, ssml_parser *p)
{
    if (q->uri == NULL) {
        q->no_ns = 1;
        q->uri = SSML_NS;
        q->uri_len = strlen(SSML_NS);
    } else if (q->uri_len == 13 && cst_startswith(q->uri, "unboundprefix")) {
        const char *ns = q->prefix ? xml_ns_lookup(p->ns, q->prefix) : SSML_NS;

        if (ns == NULL)
            ns = SSML_NS;
        q->uri = ns;
        q->unbound = 1;
        q->uri_len = strlen(ns);
    }
}

/* 0x2b58c arm */
static void ssml_end_element(void *data, const XML_Char *name)
{
    ssml_parser *p = data;
    qname q;
    int type;

    qname_split(&q, name);
    qname_resolve(&q, p);
    if (ssml_is_amazon(&q, p, "literal"))
        type = AMAZON_LITERAL;
    else if (ssml_is_amazon(&q, p, "domain"))
        type = AMAZON_DOMAIN;
    else if (ssml_is_amazon(&q, p, "effect"))
        type = AMAZON_EFFECT;
    else if (ssml_is_amazon(&q, p, "lexicon"))
        type = AMAZON_LEXICON;
    else
        type = ssml_element_type(&q, p);
    if (type == SSML_SPEAK && p->wrapped && p->cur && p->cur->n.type == SSML_SPEAK
        && p->cur->n.parent == NULL) {
        const char *msg = XML_ErrorString(XML_ERROR_TAG_MISMATCH);
        char first[2];

        first[0] = toupper((unsigned char)msg[0]);
        first[1] = 0;
        SSML_WARN(p, 8, "%s%s.", first, msg[0] ? msg + 1 : msg);
    }
    if (p->skip != 0) {
        if (p->skip != 1 || type != AMAZON_LEXICON) {
            p->skip--;
            return;
        }
    } else if (type != AMAZON_LEXICON)
        goto close;
    {
        int start = p->lexicon_start;
        int len = XML_GetCurrentByteIndex(p->xp) - start;
        char *s = cst_substr(p->input->data, start, len);

        dom_set_attr(p->cur, "lexicon-content", s);
        cst_free(s);
        p->skip--;
    }
close:
    ssml_span(p, &p->input, &p->cur->end_span);
    p->cur = p->cur->n.parent;
}

static void ssml_start_element(void *data, const XML_Char *name, const XML_Char **attrs);
/* 0x2b97c arm: parses text into doc's DOM. A fragment does not warn about elements in no
 * namespace; with may_wrap, input without a root element is put into <speak>. 0, or 1 on
 * errors (tts_errno). */
int ssml_parse(ssml_doc *doc, ivona_ustr *text, int fragment, int may_wrap)
{
    ssml_parser p;
    ivona_ustr *wrapped_text = NULL;
    int own = 0, wrapped = 0, ret, at_root;

    if (may_wrap) {
        ustr_edit e;
        size_t off;

        if (text->data == NULL)
            off = 0;
        else {
            const char *s;

            off = cst_startswith(text->data, "\xef\xbb\xbf") ? 3 : 0;
            s = text->data + off;
            if (strncmp(s, "<?xml", 5) == 0 || strncmp(s, "<!DOCTYPE", 9) == 0)
                goto parse;
            while (off < text->len) {
                int len;

                if (!xml_is_space(utf8_decode(text->data + off, &len)))
                    break;
                off += len;
            }
            if (strncmp(text->data + off, "<speak", 6) == 0)
                goto parse;
        }
        ustr_edit_init(&e, text);
        ustr_edit_skip(&e, off);
        if (text->data == NULL
            || (strncmp(text->data + off, "<?xml", 5) != 0 && strncmp(text->data + off, "<!DOCTYPE", 9) != 0
                && strncmp(text->data + off, "<speak", 6) != 0))
            ustr_edit_insert(&e, "<speak>");
        ustr_edit_keep(&e, text->len - off);
        wrapped_text = ustr_edit_result(&e);
        if (wrapped_text) {
            own = 1;
            wrapped = 1;
            text = wrapped_text;
        }
    }
parse:
    p.xp = XML_ParserCreateNS("UTF-8", '^');
    p.doc = doc;
    p.dom = dom_new();
    p.cur = NULL;
    p.skip = 0;
    p.started = 0;
    p.ns = xml_ns_stack_new();
    p.ns2 = xml_ns_stack_new();
    p.cleaned = xml_clean_chars(text);
    p.text_end = -1;
    p.unbound_warned = 0;
    p.root_warned = 0;
    p.lookup_warned = 0;
    p.no_ns_warned = fragment;
    p.wrapped = wrapped;
    p.input = p.cleaned ? p.cleaned : text;
    p.lenient = wrapped ? 1 : fragment;
    XML_SetReturnNSTriplet(p.xp, 1);
    XML_SetUserData(p.xp, &p);
    XML_SetElementHandler(p.xp, ssml_start_element, ssml_end_element);
    XML_SetCharacterDataHandler(p.xp, ssml_chardata);
    XML_SetUnknownEncodingHandler(p.xp, xml_unknown_encoding, NULL);
    XML_SetNamespaceDeclHandler(p.xp, ssml_start_ns, ssml_end_ns);
    XML_SetEntityDeclHandler(p.xp, ssml_entity_decl);
    XML_SetDefaultHandler(p.xp, ssml_default);
    XML_SetSkippedEntityHandler(p.xp, ssml_skipped_entity);
    xml_ns_push(p.ns, "amazon", AMAZON_NS);
    xml_ns_push(p.ns, "ivona", AMAZON_NS);
    xml_ns_push(p.ns2, "vxml", "http://www.w3.org/2001/vxml");
    if (XML_Parse(p.xp, p.input->data, p.input->len, 1) != XML_STATUS_OK) {
        at_root = p.cur && p.cur->n.type == SSML_SPEAK && p.cur->n.parent == NULL;
        while (p.cur)
            ssml_end_element(&p, "");
        if (!p.started) {
            const char *msg = XML_ErrorString(XML_GetErrorCode(p.xp));
            char first[2];
            ustr_span span;

            first[0] = toupper((unsigned char)msg[0]);
            first[1] = 0;
            ssml_span(&p, &p.input, &span);
            tts_set_errorf(TTS_STATUS_FORMAT_ERROR, "line %lu (byte %d..%d): %s%s",
                           XML_GetCurrentLineNumber(p.xp), span.start, span.end, first,
                           msg[0] ? msg + 1 : msg);
            ret = 1;
            goto done;
        } else {
            enum XML_Error code = XML_GetErrorCode(p.xp);

            if (code == XML_ERROR_ABORTED) {
                ret = 1;
                goto done;
            }
            if (!(code == XML_ERROR_NO_ELEMENTS && at_root && p.wrapped)) {
                const char *msg = XML_ErrorString(code);
                char first[2];

                first[0] = toupper((unsigned char)msg[0]);
                first[1] = 0;
                SSML_WARN(&p, 8, "%s%s. Stopped parsing at this point. All following input is ignored.",
                          first, msg[0] ? msg + 1 : msg);
            }
        }
    }
    doc->dom = p.dom;
    p.dom = NULL;
    ret = 0;
done:
    XML_ParserFree(p.xp);
    vector_free(p.ns);
    vector_free(p.ns2);
    dom_free(p.dom);
    ivona_ustr_free(p.cleaned);
    if (own)
        ivona_ustr_free(wrapped_text);
    return ret;
}

/* 0x2bfa8 arm */
int ssml_parse_fragment(ssml_doc *doc, ivona_ustr *text, int fragment)
{
    return ssml_parse(doc, text, fragment, 0);
}

/* 0x2bfb0 arm: "uri^local" (or "local") */
static char *qname_join(const qname *q)
{
    char *s;

    if (q->uri == NULL)
        return cst_substr(q->local, 0, q->local_len);
    s = cst_safe_alloc(q->local_len + q->uri_len + 2, 1);
    cst_strncpy(s, q->uri, q->uri_len);
    s[q->uri_len] = '^';
    cst_strncpy(s + q->uri_len + 1, q->local, q->local_len);
    return s;
}

/* 0x2c024 arm: frees name, an Expat name, returning it as "uri^local" with the IVONA namespace
 * replaced. */
static char *ssml_canonical_name(char *name)
{
    qname q;
    char *s;

    qname_split(&q, name);
    s = qname_join(&q);
    cst_free(name);
    return s;
}

/* 0x2c120 arm */
static void ssml_start_element(void *data, const XML_Char *name, const XML_Char **attrs)
{
    ssml_parser *p = data;
    qname q;
    int type;
    dom_elem *e;
    char *n;

    qname_split(&q, name);
    qname_resolve(&q, p);
    if (ssml_is_amazon(&q, p, "literal"))
        type = AMAZON_LITERAL;
    else if (ssml_is_amazon(&q, p, "domain"))
        type = AMAZON_DOMAIN;
    else if (ssml_is_amazon(&q, p, "effect"))
        type = AMAZON_EFFECT;
    else if (ssml_is_amazon(&q, p, "lexicon"))
        type = AMAZON_LEXICON;
    else
        type = ssml_element_type(&q, p);
    if (p->skip || type == SSML_METADATA) {
        p->skip++;
        return;
    }
    n = qname_join(&q);
    e = dom_element_new(n, type);
    cst_free(n);
    ssml_span(p, &p->input, &e->span);
    e->n.index = XML_GetCurrentLineNumber(p->xp);
    for (; *attrs; attrs += 2) {
        const char *v = cst_skip_space(attrs[1]);
        char *t = cst_rtrim_dup(v);
        const char *an = attrs[0];
        const char *sep = strchr(an, '^');
        char *key;

        if (t)
            v = t;
        /* drop the prefix part of the name */
        if (sep && (sep = strchr(sep + 1, '^')) != NULL)
            key = cst_substr(an, 0, sep - an);
        else
            key = cst_strdup(an);
        dom_set_attr(e, key, v);
        cst_free(key);
        cst_free(t);
    }
    if (p->cur) {
        if (type == SSML_SPEAK)
            SSML_MSG_ADD(p, p->doc, e->span, 7, "Element \"speak\"  can only be the root.");
        else if (!ssml_may_contain(p->cur->n.type, type))
            SSML_MSG_ADD(p, p->doc, e->span, 7, "Element \"%s\" cannot be a child of \"%s\"", name,
                         p->cur->name);
    }
    dom_append_child(p->cur, &e->n);
    p->cur = e;
    if (p->dom->root == NULL) {
        p->started = 1;
        if (type == SSML_SPEAK) {
            const char *base = dom_get_attr(e, "http://www.w3.org/XML/1998/namespace^base");
            const char *version = dom_get_attr(e, "version");
            const char *lang = dom_get_attr(e, "http://www.w3.org/XML/1998/namespace^lang");

            p->dom->root = e;
            if (base) {
                p->dom->name = cst_strdup(base);
                XML_SetBase(p->xp, base);
            }
            if (version == NULL) {
                if (!p->lenient)
                    TTS_MSG_ADD_AT(p->doc->warnings, e->span.start, e->n.index, 5,
                                   "Missing attribute \"%s\" in element \"%s\"", "version", e->name);
                p->doc->ssml_version = 0;
            } else if (strcmp("1.0", version) == 0)
                p->doc->ssml_version = 10;
            else if (strcmp("1.1", version) == 0)
                p->doc->ssml_version = 11;
            else {
                p->doc->ssml_version = 0;
                SSML_MSG_ADD(p, p->doc, e->span, 4, "Unsupported SSML version");
            }
            if (lang == NULL && !p->lenient)
                TTS_MSG_ADD_AT(p->doc->warnings, e->span.start, e->n.index, 5,
                               "Missing attribute \"%s\" in element \"%s\"", "xml:lang", e->name);
            return;
        }
        SSML_MSG_ADD(p, p->doc, e->span, 7, "Invalid root element. Expecting <speak>.");
        p->dom->root = dom_element_new("speak", SSML_SPEAK);
        dom_append_child(p->dom->root, &e->n);
    } else if (p->wrapped && !p->root_warned) {
        p->root_warned = 1;
        SSML_MSG_ADD(p, p->doc, e->span, 7, "Invalid root element. Expecting <speak>.");
    }
    if (type == SSML_W) {
        const char *role = dom_get_attr(e, "role");
        strbuf sb;
        const char *s;
        size_t len;

        if (role == NULL)
            return;
        len = strspn(role, " \t\n\v\f\r\x85\xa0");
        strbuf_init(&sb);
        for (s = role + len; *s; s += len, s += strspn(s, " \t\n\v\f\r\x85\xa0")) {
            char *tok, *qn;

            len = strcspn(s, " \t\n\v\f\r\x85\xa0");
            tok = cst_strndup(s, len);
            qn = xml_ns_qualify(p->ns, tok);
            if (qn) {
                char *c = ssml_canonical_name(qn);

                if (sb.len)
                    strbuf_append(&sb, " ", 1);
                strbuf_append_str(&sb, c);
                cst_free(c);
            }
            cst_free(tok);
        }
        if (sb.len)
            dom_set_attr(e, "role", sb.data);
        strbuf_release(&sb);
    } else if (type == SSML_SAY_AS) {
        const char *v = dom_get_attr(e, "interpret-as");
        char *qn, *c;

        if (v == NULL)
            return;
        qn = xml_ns_qualify(p->ns, v);
        if (qn == NULL && (qn = xml_ns_qualify(p->ns2, v)) == NULL)
            return;
        c = ssml_canonical_name(qn);
        dom_set_attr(e, "interpret-as", c);
        cst_free(c);
    } else if (type == SSML_LOOKUP) {
        if (!p->lookup_warned && p->doc->ssml_version == 10) {
            p->lookup_warned = 1;
            TTS_MSG_ADD_AT(p->doc->warnings, e->span.start, e->n.index, 7,
                           "Element \"lookup\" is not defined in SSML 1.0");
        }
    } else if (type == SSML_META) {
        const char *v = dom_get_attr(e, "name");
        char *qn, *c;

        if (v == NULL || (qn = xml_ns_qualify(p->ns, v)) == NULL)
            return;
        c = ssml_canonical_name(qn);
        dom_set_attr(e, "name", c);
        cst_free(c);
    } else if (type == AMAZON_LEXICON) {
        if (p->skip == 0) {
            XML_Index i = XML_GetCurrentByteIndex(p->xp);
            int count = XML_GetCurrentByteCount(p->xp);

            p->skip++;
            p->lexicon_start = i + count;
        }
    } else if (type == SSML_UNKNOWN_SSML)
        SSML_MSG_ADD(p, p->doc, e->span, 7, "Invalid element \"%s\"", name);
}
