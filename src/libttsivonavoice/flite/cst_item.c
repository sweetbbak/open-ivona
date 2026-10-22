/* Imported from CMU Flite 1.4 src/hrg/cst_item.c by `dc flite import`; functions with an address comment
 * are present in the engine (unmodified ones match as-is, others carry IVONA's changes). */
/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 1999                             */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  December 1999                                    */
/*************************************************************************/
/*                                                                       */
/*  Items (and Item Contents)                                            */
/*                                                                       */
/*************************************************************************/
#include "cst_alloc.h"
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"

/* Define functions for using items, rels and utts as vals */
CST_VAL_REGISTER_TYPE_NODEL(relation,cst_relation)
CST_VAL_REGISTER_TYPE_NODEL(item,cst_item)
CST_VAL_REGISTER_TYPE(utterance,cst_utterance)
CST_VAL_REGISTER_FUNCPTR(itemfunc,cst_itemfunc)

/*
 * IVONA: an item's contents record the items sharing them in `contents->relations`, keyed by
 * the relation's id name. Every daughter points to its parent (u), not only the first one.
 */

/* 0x4db18: drop this item's reference to its contents; free them when no item uses them */
void item_unref_contents(cst_item *item)
{
    if (item && item->contents) {
        feat_remove(item->contents->relations, cst_relation_id_name(item->relation->id));
        if (feat_length(item->contents->relations) == 0) {
            delete_features(item->contents->relations);
            delete_features(item->contents->features);
            cst_free(item->contents);
        }
        item->contents = NULL;
    }
}

/* 0x4db90: unlink an item (and delete its daughters) */
void delete_item(cst_item *item)
{
    cst_item *ds, *nds;

    if (item->n != NULL)
        item->n->p = item->p;
    if (item->p != NULL)
        item->p->n = item->n;
    else if (item->u != NULL)
        item->u->d = item->n;  /* first daughter */
    if (item->relation) {
        if (item->relation->head == item)
            item->relation->head = item->n;
        if (item->relation->tail == item)
            item->relation->tail = item->p;
    }
    for (ds = item->d; ds; ds = nds) {
        nds = ds->n;
        delete_item(ds);
    }
    item_unref_contents(item);
    cst_utt_free(item_utt(item), item);
}

/* 0x4dc24 */
cst_item_contents *new_item_contents(cst_item *i)
{
    cst_item_contents *ic;

    ic = cst_alloc(cst_item_contents, 1);
    ic->features = new_features_local(i->relation->utterance->ctx);
    ic->relations = new_features_local(i->relation->utterance->ctx);
    return ic;
}

/* 0x4dc6c: IVONA declares this int but never returns a value (C89 style); the declared return
 * type is what stops GCC from tail-calling the final feat_set(). */
int item_contents_set(cst_item *current, cst_item *i)
{
    cst_item_contents *c;
    cst_item *nn_item;
    const char *name;

    if (i == NULL)
        c = new_item_contents(current);
    else
        c = i->contents;
    if (c == current->contents)
        return;
    name = cst_relation_id_name(current->relation->id);
    item_unref_contents(current);
    current->contents = c;
    /* If this contents is already in this relation, empty the other reference */
    if (feat_present(current->contents->relations, name)) {
        nn_item = val_item(feat_val(current->contents->relations, name));
        feat_set(nn_item->contents->relations, name, item_val(nn_item));
    }
    /* Add back reference */
    feat_set(current->contents->relations, name, item_val(current));
}

/* 0x4dd20 */
cst_item *new_item_relation(cst_relation *r, cst_item *i)
{
    cst_item *ni;

    ni = cst_utt_alloc(r->utterance, cst_item, 1);
    ni->contents = 0;
    ni->n = ni->p = ni->u = ni->d = 0;
    ni->relation = r;
    item_contents_set(ni, i);
    return ni;
}

/* 0x4dd64: i as seen in relation `id`, or NULL */
cst_item *item_as(const cst_item *i, int id)
{
    const cst_val *v;

    if (i == NULL || id == 0)
        return NULL;
    v = feat_val(i->contents->relations, cst_relation_id_name(id));
    if (v != NULL)
        return val_item(v);
    return NULL;
}

/* 0x4dda8 */
cst_item *item_next(const cst_item *i)
{
    if (i == NULL)
        return NULL;
    return i->n;
}

/* 0x4ddb4: delete i and the items after it, n items in total */
void item_delete_n(cst_item *i, int n)
{
    cst_item *next;
    int k;

    for (k = 0; i && k < n; i = next) {
        next = item_next(i);
        k++;
        delete_item(i);
    }
}

/* 0x4ddfc: delete the items from i up to (not including) end */
void item_delete_range(cst_item *i, cst_item *end)
{
    cst_item *next;

    if (i == end || i == NULL)
        return;
    do {
        next = item_next(i);
        delete_item(i);
        i = next;
    } while (i != NULL && i != end);
}

/* 0x4de3c */
cst_item *item_prev(const cst_item *i)
{
    if (i == NULL)
        return NULL;
    return i->p;
}

/* 0x4de48: the item n steps after i (NULL past the end) */
cst_item *item_next_n(cst_item *i, int n)
{
    if (n == 0 || i == NULL)
        return i;
    do {
        n--;
        i = i->n;
    } while (n != 0 && i != NULL);
    return i;
}

/* 0x4de7c */
cst_item *item_append(cst_item *current, cst_item *ni)
{
    cst_item *rni;

    if (ni && ni->relation == current->relation)
        return ni;  /* already in this relation */
    rni = new_item_relation(current->relation, ni);
    rni->n = current->n;
    if (current->n != NULL)
        current->n->p = rni;
    rni->p = current;
    current->n = rni;
    rni->u = current->u;
    if (current->relation->tail == current)
        current->relation->tail = rni;
    return rni;
}

/* 0x4dee0 */
cst_item *item_prepend(cst_item *current, cst_item *ni)
{
    cst_item *prev = current->p;
    cst_item *rni;

    if (ni && ni->relation == current->relation)
        return ni;  /* already in this relation */
    rni = new_item_relation(current->relation, ni);
    rni->p = current->p;
    if (current->p != NULL)
        current->p->n = rni;
    rni->n = current;
    current->p = rni;
    if (prev == NULL && current->u)
        current->u->d = rni;  /* new first daughter */
    rni->u = current->u;
    if (current->relation->head == current)
        current->relation->head = rni;
    return rni;
}

/* 0x4df5c */
cst_item *item_parent(const cst_item *i)
{
    if (i == NULL)
        return NULL;
    return i->u;
}

/* 0x4df68 */
cst_item *item_daughter(const cst_item *i)
{
    if (i == NULL)
        return NULL;
    return i->d;
}

/* 0x4df74 */
cst_item *item_nth_daughter(const cst_item *i, int n)
{
    int d;
    cst_item *p;

    for (d = 0, p = item_daughter(i); p && (d < n); p = item_next(p), d++)
        ;
    return p;
}

/* 0x4dfa8 */
cst_item *item_last_daughter(const cst_item *i)
{
    cst_item *p;

    for (p = item_daughter(i); item_next(p); p = item_next(p))
        ;
    return p;
}

/* 0x4dfd8 */
cst_item *item_add_daughter(cst_item *i, cst_item *nd)
{
    cst_item *p, *rnd;

    p = item_last_daughter(i);
    if (p)
        return item_append(p, nd);
    /* first new daughter */
    if (nd && nd->relation == i->relation)
        return p;  /* already in this relation */
    rnd = new_item_relation(i->relation, nd);
    rnd->u = i;
    i->d = rnd;
    return rnd;
}

/* 0x4e034: is i the first of its siblings? */
int item_is_first(const cst_item *i)
{
    const cst_item *u = item_parent(i);

    if (u)
        return item_daughter(u) == i;
    return item_prev(i) == NULL;
}

/* 0x4e070 */
cst_features *item_feats(const cst_item *i)
{
    return (i ? i->contents->features : NULL);
}

/* 0x4e080 */
unsigned char item_feat_present(const cst_item *i, const char *name)
{
    return feat_present(item_feats(i), name);
}

/* 0x4e098 */
unsigned char item_feat_remove(const cst_item *i, const char *name)
{
    return feat_remove(item_feats(i), name);
}

/* 0x4e0b0 */
const cst_val *item_feat(const cst_item *i,const char *name)
{
    return feat_val(item_feats(i),name);
}

/* 0x4e0c8 */
int item_feat_int(const cst_item *i,const char *name)
{
    return feat_int(item_feats(i),name);
}

/* 0x4e0e0 */
float item_feat_float(const cst_item *i,const char *name)
{
    return feat_float(item_feats(i),name);
}

/* 0x4e0f8 */
const char *item_feat_string(const cst_item *i,const char *name)
{
    return feat_string(item_feats(i),name);
}

/* 0x4e110: like item_feat_string, but NULL when the feature is absent */
const char *item_feat_string_opt(const cst_item *i, const char *name)
{
    const cst_val *v = item_feat(i, name);

    if (v == NULL)
        return NULL;
    return val_string(v);
}

/* 0x4e128 */
void item_set(const cst_item *i,const char *name,const cst_val *val)
{
    feat_set(item_feats(i),name,val);
}

/* 0x4e148 */
void item_set_int(const cst_item *i,const char *name,int val)
{
    feat_set_int(item_feats(i),name,val);
}

/* 0x4e168 */
void item_set_float(const cst_item *i,const char *name,float val)
{
    feat_set_float(item_feats(i),name,val);
}

/* 0x4e188 */
void item_set_string(const cst_item *i,const char *name,const char *val)
{
    feat_set_string(item_feats(i),name,val);
}

/* 0x4e1a8 */
cst_utterance *item_utt(const cst_item *i)
{
    if (i && i->relation)
	return i->relation->utterance;
    else
	return NULL;
}

/* 0x4e1c0 */
int item_equal(const cst_item *a, const cst_item *b)
{
    if ((a == b) ||
	(a && b && (a->contents == b->contents)))
	return TRUE;
    else
	return FALSE;
}
