/*
 * A small DOM for SSML input (0x4a464..0x4abd8): documents, elements with attributes and
 * text nodes holding ustrs.
 */
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_string.h"
#include "cst_val.h"
#include "ivona_util.h"
#include "ivona_ustr.h"
#include "dom.h"

/* xml.c */
char *xml_escape(const char *s, size_t n);
char *xml_escape_str(const char *s);

void dom_element_free(dom_elem *e);

/* 0x4a464 arm: appends e as XML markup. */
void dom_write_element(const dom_elem *e, strbuf *sb)
{
    const cst_featvalpair *a;
    const dom_node *c;
    char *s;

    strbuf_append_str(sb, "<");
    strbuf_append_str(sb, e->name);
    for (a = e->attrs->head; a; a = a->next) {
        s = xml_escape_str(val_string(a->val));
        strbuf_append_str(sb, " ");
        strbuf_append_str(sb, a->name);
        strbuf_append_str(sb, "=\"");
        strbuf_append_str(sb, s);
        strbuf_append_str(sb, "\"");
        cst_free(s);
    }
    if (e->first == NULL) {
        strbuf_append_str(sb, "/>");
        return;
    }
    strbuf_append_str(sb, ">");
    for (c = e->first; c; c = c->next) {
        if (c->type == DOM_TEXT) {
            const ivona_ustr *t = ((const dom_text *)c)->text;

            s = xml_escape(t->data, t->len);
            strbuf_append_str(sb, s);
            cst_free(s);
        } else
            dom_write_element((const dom_elem *)c, sb);
    }
    strbuf_append_str(sb, "</");
    strbuf_append_str(sb, e->name);
    strbuf_append_str(sb, ">");
}

/* 0x4a5f8 arm */
dom_doc *dom_new(void)
{
    return cst_safe_calloc(1, sizeof(dom_doc));
}

/* 0x4a604 arm */
dom_text *dom_text_new(void)
{
    dom_text *t = cst_safe_alloc(1, sizeof(dom_text));

    t->n.index = -1;
    t->n.type = DOM_TEXT;
    t->n.parent = NULL;
    t->n.next = NULL;
    t->text = NULL;
    t->state = 0;
    t->dict_pos = 0;
    return t;
}

/* 0x4a640 arm */
dom_elem *dom_element_new(const char *name, int type)
{
    dom_elem *e = cst_safe_calloc(1, sizeof(dom_elem));

    e->n.type = type;
    e->n.index = -1;
    e->name = cst_strdup(name);
    e->attrs = new_features();
    e->span.end = 0;
    e->span.start = 0;
    e->unk_28 = 0;
    e->unk_24 = 0;
    e->in_subdoc = 0;
    return e;
}

/* 0x4a69c arm */
dom_node *dom_prev_sibling(const dom_node *n)
{
    dom_node *c;

    if (n->parent == NULL || (c = n->parent->first) == n)
        return NULL;
    while (c->next != n)
        c = c->next;
    return c;
}

/* unlinks n from its parent */
#define DOM_UNLINK(n)                                                                 \
    do {                                                                              \
        if ((n)->parent) {                                                            \
            if ((n) == (n)->parent->first)                                            \
                (n)->parent->first = (n)->next;                                       \
            else                                                                      \
                dom_prev_sibling(n)->next = (n)->next;                                \
        }                                                                             \
    } while (0)

/* 0x4a6d8 arm */
void dom_text_free(dom_text *t)
{
    if (t == NULL)
        return;
    ivona_ustr_free(t->text);
    DOM_UNLINK(&t->n);
    cst_free(t);
}

/* 0x4a838 arm */
void dom_node_free(dom_node *n)
{
    if (n == NULL)
        return;
    if (n->type == DOM_TEXT)
        dom_text_free((dom_text *)n);
    else
        dom_element_free((dom_elem *)n);
}

/* frees a features set and its keys */
#define FREE_OWNED_FEATURES(f)                                                        \
    do {                                                                              \
        cst_featvalpair *p_;                                                          \
        for (p_ = (f)->head; p_; p_ = p_->next)                                       \
            cst_free((void *)p_->name);                                               \
        delete_features(f);                                                           \
    } while (0)

/* 0x4a734 arm */
void dom_element_free(dom_elem *e)
{
    dom_node *c, *next;

    if (e == NULL)
        return;
    for (c = e->first; c; c = next) {
        next = c->next;
        dom_node_free(c);
    }
    FREE_OWNED_FEATURES(e->attrs);
    cst_free(e->name);
    DOM_UNLINK(&e->n);
    cst_free(e);
}

/* 0x4a7dc arm */
void dom_free(dom_doc *d)
{
    if (d == NULL)
        return;
    dom_element_free(d->root);
    cst_free(d->name);
    if (d->pi)
        FREE_OWNED_FEATURES(d->pi);
    cst_free(d);
}

/* 0x4a854 arm: frees everything before n in document order (except its ancestors). */
void dom_remove_preceding(dom_node *n)
{
    dom_node *c, *next;

    while (n->parent) {
        for (c = n->parent->first; c != n; c = next) {
            next = c->next;
            dom_node_free(c);
        }
        n = &n->parent->n;
    }
}

/* 0x4a8a4 arm: frees everything after n in document order (except its ancestors). */
void dom_remove_following(dom_node *n)
{
    for (; n; n = &n->parent->n)
        while (n->next)
            dom_node_free(n->next);
}

/* 0x4a8d4 arm */
void dom_element_clear(dom_elem *e)
{
    dom_node *c, *next;

    for (c = e->first; c; c = next) {
        next = c->next;
        dom_node_free(c);
    }
}

/* 0x4a8fc arm: sets a processing instruction attribute. Like dom_set_attr it is declared
 * int but returns nothing (the calls at its end are not tail calls). */
int dom_set_pi(dom_doc *d, const char *name, const char *value)
{
    if (d->pi == NULL)
        d->pi = new_features();
    if (feat_present(d->pi, name))
        feat_set_string(d->pi, name, value);
    else {
        cst_features *f = d->pi;

        feat_set_string(f, cst_strdup(name), value);
    }
}

/* 0x4a968 arm */
dom_node *dom_last_child(const dom_elem *e)
{
    dom_node *c = e->first;

    if (c == NULL)
        return NULL;
    while (c->next)
        c = c->next;
    return c;
}

/* 0x4a988 arm: replaces e by its children. */
void dom_element_unwrap(dom_elem *e)
{
    dom_node *c, *prev, *last;

    if (e->first) {
        for (c = e->first; c; c = c->next)
            c->parent = e->n.parent;
        prev = dom_prev_sibling(&e->n);
        last = dom_last_child(e);
        if (prev)
            prev->next = e->first;
        else
            e->n.parent->first = e->first;
        last->next = e->n.next;
        e->first = NULL;
        e->n.next = NULL;
        e->n.parent = NULL;
    }
    dom_element_free(e);
}

/* 0x4aa00 arm */
void dom_append_child(dom_elem *e, dom_node *child)
{
    dom_node *last;

    child->next = NULL;
    child->parent = e;
    if (e == NULL)
        return;
    last = dom_last_child(e);
    if (last)
        last->next = child;
    else
        e->first = child;
}

/* 0x4aa34 arm */
void dom_insert_after(dom_node *n, dom_node *new_node)
{
    new_node->parent = n->parent;
    new_node->next = n->next;
    n->next = new_node;
}

/* 0x4aa44 arm */
int dom_set_attr(dom_elem *e, const char *name, const char *value)
{
    if (feat_present(e->attrs, name))
        feat_set_string(e->attrs, name, value);
    else {
        cst_features *f = e->attrs;

        feat_set_string(f, cst_strdup(name), value);
    }
}

/* 0x4aa98 arm: the attribute value (left in r0 by get_param_string).
 * The call is not a sibcall, so the original's return value location differed from a
 * pointer's (arm_function_ok_for_sibcall compares them); a missing return in a function of
 * a wider type reproduces that. */
long long dom_get_attr(const dom_elem *e, const char *name)
{
    get_param_string(e->attrs, name, NULL);
}

/* 0x4aaac arm: attribute name of the nearest ancestor-or-self of the given type that
 * has it. */
const char *dom_find_attr_up(const dom_elem *e, int type, const char *name)
{
    const char *s;

    for (; e; e = e->n.parent)
        if (e->n.type == type && (s = get_param_string(e->attrs, name, NULL)) != NULL)
            return s;
    return NULL;
}

/* 0x4ab00 arm */
const char *dom_inherited_attr(const dom_elem *e, const char *name)
{
    const char *s;

    for (; e; e = e->n.parent)
        if ((s = get_param_string(e->attrs, name, NULL)) != NULL)
            return s;
    return NULL;
}

/* 0x4ab3c arm (its printing is compiled out) */
void dom_element_dump(const dom_elem *e)
{
    const dom_node *c;

    for (c = e->first; c; c = c->next)
        if (c->type != DOM_TEXT)
            dom_element_dump((const dom_elem *)c);
}

/* 0x4ab70 arm */
void dom_dump(const dom_doc *d)
{
    if (d->root)
        dom_element_dump(d->root);
}

/* 0x4ab80 arm */
char *dom_to_string(const dom_doc *d)
{
    strbuf sb;

    if (d->root == NULL)
        return cst_strdup("");
    strbuf_init(&sb);
    dom_write_element(d->root, &sb);
    return strbuf_data(&sb);
}
