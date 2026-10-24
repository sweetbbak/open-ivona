#ifndef DOM_H
#define DOM_H

#include "cst_features.h"
#include "ivona_ustr.h"

/* The SSML DOM (dom.c) */

#define DOM_TEXT 1

typedef struct dom_elem dom_elem;

typedef struct dom_node {
    dom_elem *parent;
    struct dom_node *next;
    int index;                  /* +0x08 */
    int type;                   /* +0x0c: DOM_TEXT or an element type */
} dom_node;

typedef struct dom_text {
    dom_node n;
    ivona_ustr *text;           /* +0x10 */
    int state;                  /* +0x14: processing stage (ssml_doc.c) */
    unsigned int dict_pos;      /* +0x18: next user dictionary to apply */
} dom_text;

struct dom_elem {
    dom_node n;
    dom_node *first;            /* +0x10 */
    char *name;                 /* +0x14 */
    cst_features *attrs;        /* +0x18: keys owned */
    ustr_span span;             /* +0x1c: input bytes */
    int unk_24;
    int unk_28;
    int in_subdoc;              /* +0x2c */
};

typedef struct dom_doc {
    dom_elem *root;
    char *name;                 /* +0x04 */
    cst_features *pi;           /* +0x08: keys owned */
} dom_doc;

dom_doc *dom_new(void);
dom_text *dom_text_new(void);
dom_elem *dom_element_new(const char *name, int type);
void dom_free(dom_doc *d);
void dom_element_free(dom_elem *e);
void dom_node_free(dom_node *n);
void dom_insert_after(dom_node *n, dom_node *new_node);
void dom_append_child(dom_elem *e, dom_node *child);
int dom_set_attr(dom_elem *e, const char *name, const char *value);
const char *dom_inherited_attr(const dom_elem *e, const char *name);

#endif
