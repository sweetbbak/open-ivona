/*
 * Flite cst_features.c (1.4 base) as modified by IVONA: inline type-checked val accessor,
 * feat_take(), typed feature getters/setters for nested features, feat_set() that ignores NULL
 * and self-assignment, feat_copy_into() that copies cons structure, and a typed feat_print().
 */
#include "cst_error.h"
#include "cst_features.h"

cst_val *copy_val(const cst_val *v);

/* 0x51a4c */
cst_features *val_features(const cst_val *v)
{
    if (v && CST_VAL_TYPE(v) == cst_val_type_features)
        return (cst_features *)CST_VAL_VOID(v);
    cst_errmsg("VAL: tried to access %s in %d type val\n", "features", (v ? CST_VAL_TYPE(v) : -1));
    cst_error();
}

/* 0x51aa4 */
cst_val *features_val(const cst_features *v)
{
    return val_new_typed(cst_val_type_features, (void *)v);
}

/* 0x51abc */
static cst_featvalpair *feat_find_featpair(const cst_features *f, const char *name)
{
    cst_featvalpair *n;

    if (f == NULL)
        return NULL;
    for (n = f->head; n; n = n->next)
        if (cst_streq(name, n->name))
            return n;
    return NULL;
}

/* 0x51b04 */
cst_features *new_features(void)
{
    cst_features *f;

    f = cst_alloc(cst_features, 1);
    f->head = NULL;
    f->ctx = NULL;
    return f;
}

/* 0x51b24 */
cst_features *new_features_local(cst_alloc_context ctx)
{
    cst_features *f;

    f = (cst_features *)cst_local_alloc(ctx, sizeof(*f));
    f->head = NULL;
    f->ctx = ctx;
    return f;
}

/* 0x51b44 */
void delete_features(cst_features *f)
{
    cst_featvalpair *n, *np;

    if (f) {
        for (n = f->head; n; n = np) {
            np = n->next;
            delete_val(n->val);
            cst_local_free(f->ctx, n);
        }
        cst_local_free(f->ctx, f);
    }
}

/* 0x51b84: destructor used when a features val is deleted */
void val_delete_features(void *v)
{
    delete_features((cst_features *)v);
}

/* 0x51b88 */
unsigned char feat_present(const cst_features *f, const char *name)
{
    return (feat_find_featpair(f, name) != NULL);
}

/* 0x51b9c */
int feat_length(const cst_features *f)
{
    int i = 0;
    cst_featvalpair *p;

    if (f)
        for (i = 0, p = f->head; p; p = p->next)
            i++;
    return i;
}

/* 0x51bd0: unlink feature `name` and hand its value to the caller (NULL if absent). */
cst_val *feat_take(cst_features *f, const char *name)
{
    cst_featvalpair *n, *p, *np;
    cst_val *v;

    if (f == NULL)
        return NULL;
    for (p = NULL, n = f->head; n; p = n, n = np) {
        np = n->next;
        if (cst_streq(name, n->name)) {
            if (p == NULL)
                f->head = np;
            else
                p->next = np;
            v = n->val;
            cst_local_free(f->ctx, n);
            return v;
        }
    }
    return NULL;
}

/* 0x51c54 */
unsigned char feat_remove(cst_features *f, const char *name)
{
    cst_val *v = feat_take(f, name);

    delete_val(v);
    return v != NULL;
}

/* 0x51c70 */
const cst_val *feat_val(const cst_features *f, const char *name)
{
    cst_featvalpair *n;

    n = feat_find_featpair(f, name);
    if (n == NULL)
        return NULL;
    return n->val;
}

/* 0x51c84 */
int feat_int(const cst_features *f, const char *name)
{
    return val_int(feat_val(f, name));
}

/* 0x51c94 */
float feat_float(const cst_features *f, const char *name)
{
    return val_float(feat_val(f, name));
}

/* 0x51ca4 */
const char *feat_string(const cst_features *f, const char *name)
{
    return val_string(feat_val(f, name));
}

/* 0x51cb4 */
cst_features *feat_features(const cst_features *f, const char *name)
{
    return val_features(feat_val(f, name));
}

/* 0x51cc4 */
int get_param_int(const cst_features *f, const char *name, int def)
{
    const cst_val *v;

    v = feat_val(f, name);
    if (v != NULL)
        return val_int(v);
    return def;
}

/* 0x51ce8 */
float get_param_float(const cst_features *f, const char *name, float def)
{
    const cst_val *v;

    v = feat_val(f, name);
    if (v != NULL)
        return val_float(v);
    return def;
}

/* 0x51d0c */
const char *get_param_string(const cst_features *f, const char *name, const char *def)
{
    const cst_val *v;

    v = feat_val(f, name);
    if (v != NULL)
        return val_string(v);
    return def;
}

/* 0x51d30 */
cst_features *get_param_features(const cst_features *f, const char *name, cst_features *def)
{
    const cst_val *v;

    v = feat_val(f, name);
    if (v != NULL)
        return val_features(v);
    return def;
}

/* 0x51d54 */
const cst_val *get_param_val(const cst_features *f, const char *name, cst_val *def)
{
    const cst_val *v;

    v = feat_val(f, name);
    if (v != NULL)
        return v;
    return def;
}

/* 0x51d6c: set (or replace) a feature; the features object takes a reference. */
void feat_set(cst_features *f, const char *name, const cst_val *val)
{
    cst_featvalpair *n;

    n = feat_find_featpair(f, name);
    if (val == NULL)
        return;
    if (n == NULL) {
        cst_featvalpair *p;
        p = (cst_featvalpair *)cst_local_alloc(f->ctx, sizeof(*p));
        p->next = f->head;
        p->name = name;
        p->val = val_inc_refcount(val);
        f->head = p;
    } else if (n->val != val) {
        delete_val(n->val);
        n->val = val_inc_refcount(val);
    }
}

/* 0x51de4 */
void feat_set_int(cst_features *f, const char *name, int v)
{
    feat_set(f, name, int_val(v));
}

/* 0x51e0c */
void feat_set_float(cst_features *f, const char *name, float v)
{
    feat_set(f, name, float_val(v));
}

/* 0x51e34 */
void feat_set_string(cst_features *f, const char *name, const char *v)
{
    feat_set(f, name, string_val(v));
}

/* 0x51e5c */
void feat_set_features(cst_features *f, const char *name, const cst_features *v)
{
    feat_set(f, name, features_val(v));
}

/* 0x51e84: copy every feature of `from` into `to`; returns how many were copied. */
int feat_copy_into(const cst_features *from, cst_features *to)
{
    cst_featvalpair *p;
    int i;

    for (i = 0, p = from->head; p; p = p->next, i++) {
        cst_val *v = copy_val(p->val);
        feat_set(to, p->name, v);
    }
    return i;
}

/* 0x51ed4: print "name value" lines, formatting by value type. */
int feat_print(cst_file fd, const cst_features *f)
{
    cst_featvalpair *p;

    for (p = f->head; p; p = p->next) {
        if (CST_VAL_TYPE(p->val) == CST_VAL_TYPE_STRING)
            cst_fprintf(fd, "%s %s\n", p->name, val_string(p->val));
        else if (CST_VAL_TYPE(p->val) == CST_VAL_TYPE_INT)
            cst_fprintf(fd, "%s %d\n", p->name, val_int(p->val));
        else if (CST_VAL_TYPE(p->val) == CST_VAL_TYPE_FLOAT)
            cst_fprintf(fd, "%s %f\n", p->name, val_float(p->val));
        else
            cst_fprintf(fd, "%s %p\n", p->name, CST_VAL_VOID(p->val));
    }
    return 0;
}
