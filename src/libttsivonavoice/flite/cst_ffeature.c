/* Imported from CMU Flite 1.4 src/hrg/cst_ffeature.c by `dc flite import`; functions with an address comment
 * are present in the engine (unmodified ones match as-is, others carry IVONA's changes). */
/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                       Copyright (c) 1999-2007                         */
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
/*               Date:  April 2000                                       */
/*************************************************************************/
/*                                                                       */
/*  Item features and paths                                              */
/*                                                                       */
/*************************************************************************/
/* IVONA: feature paths are '.'-separated directives ("n", "pp", "parent", "R:<relation>"...)
 * resolved by item_path(); feature functions live in the language data and the utterance,
 * and "<name>_<digits>" calls a two-argument ffunc2 with the digits. */
#include <stdarg.h>
#include "cst_alloc.h"
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"
#include "cst_tokenstream.h"

typedef const cst_val *(*cst_ffunction2)(const cst_item *i, const char *arg);
CST_VAL_USER_FUNCPTR_DCLS(ffunc2, cst_ffunction2)

typedef cst_item *(*item_traverse_fn)(const cst_item *i);

#define ITEM_TRAVERSE_FIRST_OP 0x100
#define ITEM_TRAVERSE_N_OPS 11

void *item_lang_data(const cst_item *i);
int cst_relation_name_id(const char *name);
int item_traverse_op(const char *s);

/* language data features (partly known) */
typedef struct lang_data_ff {
    int unk[6];
    cst_features *ffunctions;   /* +0x18 */
} lang_data_ff;

/* 0x638e0 */
static cst_item *item_first_sibling(const cst_item *i)
{
    return item_daughter(item_parent(i));
}

/* 0x638f0 */
static cst_item *item_last_sibling(const cst_item *i)
{
    return item_last_daughter(item_parent(i));
}

/* 0x63900 */
static cst_item *item_nnn(const cst_item *i)
{
    return item_next(item_next(item_next(i)));
}

/* 0x63914 */
static cst_item *item_nn(const cst_item *i)
{
    return item_next(item_next(i));
}

/* 0x63924 */
static cst_item *item_ppp(const cst_item *i)
{
    return item_prev(item_prev(item_prev(i)));
}

/* 0x63938 */
static cst_item *item_pp(const cst_item *i)
{
    return item_prev(item_prev(i));
}

/* path directives 0x100..0x10a */
const item_traverse_fn item_traverse_ops[ITEM_TRAVERSE_N_OPS] = {
    item_next, item_nn, item_nnn, item_prev, item_pp, item_ppp,
    item_parent, item_daughter, item_last_daughter, item_first_sibling, item_last_sibling
};

const char *item_traverse_names[ITEM_TRAVERSE_N_OPS] = {
    "n", "nn", "nnn", "p", "pp", "ppp", "parent", "daughter", "daughtern", "first", "last"
};

CST_VAL_REGISTER_FUNCPTR(ffunc,cst_ffunction)
CST_VAL_REGISTER_FUNCPTR(ffunc2,cst_ffunction2)

DEF_STATIC_CONST_VAL_STRING(ffeature_default_val,"0");

/* 0x63a28 arm: applies one path directive. */
cst_item *item_traverse_one(const cst_item *item, int op)
{
    if (item == NULL)
        return NULL;
    if ((unsigned int)(op - ITEM_TRAVERSE_FIRST_OP) > ITEM_TRAVERSE_N_OPS - 1) {
        cst_errmsg("item_traverse_one(): path element enumerator (%d) outside valid range\n", op);
        return NULL;
    }
    return item_traverse_ops[op - ITEM_TRAVERSE_FIRST_OP](item);
}

/* 0x63a70 arm: follows a 0-terminated list of relation ids (<= 10) and directives. */
cst_item *item_traverse(const cst_item *item, ...)
{
    va_list ap;
    unsigned int op;

    va_start(ap, item);
    op = va_arg(ap, unsigned int);
    while (op != 0) {
        if (op <= 10)
            item = item_as(item, op);
        else
            item = item_traverse_one(item, op);
        op = va_arg(ap, unsigned int);
    }
    va_end(ap);
    return (cst_item *)item;
}

/* 0x63aec arm: feature name of item: a feature function of the language or the
 * utterance, else the item's feature; "0" if missing. */
const cst_val *item_feat_func(const cst_item *item, const char *name)
{
    const char *arg = cst_strrchr(name, '_');
    char *base = NULL;
    const cst_utterance *utt;
    const lang_data_ff *ld;
    const cst_val *v, *r;

    if (arg) {
        if (arg[1] == 0)
            arg = NULL;
        else {
            arg++;
            if (strspn(arg, "0123456789") == strlen(arg))
                base = cst_strndup(name, arg - name);
            else
                arg = NULL;
        }
    }
    if (item == NULL)
        goto def;
    utt = item_utt(item);
    ld = item_lang_data(item);
    v = ld ? feat_val(ld->ffunctions, name) : NULL;
    if (v == NULL && utt)
        v = feat_val(utt->ffunctions, name);
    if (base) {
        if (v == NULL && ld)
            v = feat_val(ld->ffunctions, base);
        if (v == NULL && utt)
            v = feat_val(utt->ffunctions, base);
    }
    if (v == NULL)
        r = item_feat(item, name);
    else if (arg && CST_VAL_TYPE(v) == cst_val_type_ffunc2)
        r = val_ffunc2(v)(item, arg);
    else if (CST_VAL_TYPE(v) == cst_val_type_ffunc)
        r = val_ffunc(v)(item);
    else
        goto def;
    if (r == NULL)
def:
        r = &ffeature_default_val;
    cst_free(base);
    return r;
}

/* 0x63cf4 */
const char *ffeature_string(const cst_item *item,const char *featpath)
{
    return val_string(item_feat_func(item,featpath));
}
/* 0x63d04 */
int ffeature_int(const cst_item *item,const char *featpath)
{
    return val_int(item_feat_func(item,featpath));
}
/* 0x63d14 */
float ffeature_float(const cst_item *item,const char *featpath)
{
    return val_float(item_feat_func(item,featpath));
}

/* 0x63d24 arm: the item path leads to from item (without its last element if
 * drop_last), or NULL. */
cst_item *item_path(const cst_item *item, const char *path, int drop_last)
{
    char tok[64];
    const char *dot;
    int rel, len, op, last;

    if (item == NULL)
        return NULL;
    if (path == NULL)
        return NULL;
    while (*path) {
        rel = 0;
        if (cst_startswith(path, "R:") || cst_startswith(path, "R.")) {
            path += 2;
            rel = 1;
        }
        dot = strchr(path, '.');
        last = dot == NULL;
        if (last & (drop_last & 1))
            return (cst_item *)item;
        len = !last ? dot - path : (int)strlen(path);
        if ((unsigned int)(len + 1) > sizeof tok)
            return NULL;
        strncpy(tok, path, len);
        tok[len] = 0;
        path = path[len] ? path + len + 1 : path + len;
        if (rel) {
            if ((rel = cst_relation_name_id(tok)) == 0)
                return NULL;
            item = item_as(item, rel);
        } else {
            if ((op = item_traverse_op(tok)) == 0)
                return NULL;
            item = item_traverse_one(item, op);
        }
        if (item == NULL)
            return NULL;
    }
    return (cst_item *)item;
}

/* 0x63e88 arm */
void ff_register(cst_features *ffunctions, const char *name, cst_ffunction f)
{
    if (feat_present(ffunctions, name))
        cst_dbgmsg("warning: ffunction %s redefined\n", name);
    feat_set(ffunctions, name, ffunc_val(f));
}

/* 0x63eb8 arm */
void ff_register2(cst_features *ffunctions, const char *name, cst_ffunction2 f)
{
    if (feat_present(ffunctions, name))
        cst_dbgmsg("warning: ffunction %s redefined\n", name);
    feat_set(ffunctions, name, ffunc2_val(f));
}

/* 0x63ee8 */
void ff_unregister(cst_features *ffunctions, const char *name)
{
    feat_remove(ffunctions, name);
}

/* 0x63eec */
const cst_val *ffeature(const cst_item *item,const char *featpath)
{
    const cst_item *i = item_path(item, featpath, 1);
    const char *name = featpath, *dot;

    if (featpath && (dot = strrchr(featpath, '.')) != NULL)
        name = dot + 1;
    return item_feat_func(i, name);
}

/* 0x63f34 arm */
const char *item_traverse_name(int op)
{
    if ((unsigned int)(op - ITEM_TRAVERSE_FIRST_OP) > ITEM_TRAVERSE_N_OPS - 1)
        return NULL;
    return item_traverse_names[op - ITEM_TRAVERSE_FIRST_OP];
}

/* 0x63f5c arm: the directive named s, or 0. */
int item_traverse_op(const char *s)
{
    int i;

    if (s == NULL)
        return 0;
    for (i = 0; i < ITEM_TRAVERSE_N_OPS; i++)
        if (strcmp(item_traverse_names[i], s) == 0)
            return ITEM_TRAVERSE_FIRST_OP + i;
    return 0;
}
