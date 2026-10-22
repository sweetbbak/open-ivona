/*
 * Flite cst_val.c (1.4 base) as modified by IVONA: most accessors exit silently on type errors,
 * list walks go through val_cdr(), plus a "valfunc" value type and several list helpers.
 */
#include <stdarg.h>
#include <stdio.h>
#include "cst_file.h"
#include "cst_val.h"
#include "cst_string.h"
#include "cst_tokenstream.h"

typedef void (*cst_valfunc)(void);
extern const int cst_val_type_valfunc;

/* 0x55304 */
cst_valfunc val_valfunc(const cst_val *v)
{
    if (v && CST_VAL_TYPE(v) == cst_val_type_valfunc)
        return (cst_valfunc)CST_VAL_VOID(v);
    cst_errmsg("VAL: tried to access %s in %d type val\n", "valfunc", (v ? CST_VAL_TYPE(v) : -1));
    cst_error();
}

/* 0x5535c */
cst_val *int_val(int i)
{
    cst_val *v = cst_alloc(cst_val, 1);

    CST_VAL_TYPE(v) = CST_VAL_TYPE_INT;
    CST_VAL_INT(v) = i;
    return v;
}

/* 0x55380 */
cst_val *float_val(float f)
{
    cst_val *v = cst_alloc(cst_val, 1);

    CST_VAL_TYPE(v) = CST_VAL_TYPE_FLOAT;
    CST_VAL_FLOAT(v) = f;
    return v;
}

/* 0x553a4 */
cst_val *string_val(const char *s)
{
    cst_val *v = cst_alloc(cst_val, 1);

    CST_VAL_TYPE(v) = CST_VAL_TYPE_STRING;
    CST_VAL_STRING_LVAL(v) = cst_strdup(s);
    return v;
}

/* 0x553d8: string val that takes ownership of an allocated string (no copy). */
cst_val *string_val_consume(char *s)
{
    cst_val *v = cst_alloc(cst_val, 1);

    CST_VAL_TYPE(v) = CST_VAL_TYPE_STRING;
    CST_VAL_STRING_LVAL(v) = s;
    return v;
}

/* 0x553fc */
cst_val *val_new_typed(int type, void *vv)
{
    cst_val *v = cst_alloc(cst_val, 1);

    CST_VAL_TYPE(v) = type;
    CST_VAL_VOID(v) = vv;
    return v;
}

/* 0x55420 */
cst_val *valfunc_val(cst_valfunc f)
{
    return val_new_typed(cst_val_type_valfunc, (void *)f);
}

/* 0x55438 */
int val_int(const cst_val *v)
{
    if (v && CST_VAL_TYPE(v) == CST_VAL_TYPE_INT)
        return CST_VAL_INT(v);
    else if (v && CST_VAL_TYPE(v) == CST_VAL_TYPE_FLOAT)
        return (int)CST_VAL_FLOAT(v);
    else if (v && CST_VAL_TYPE(v) == CST_VAL_TYPE_STRING)
        return atoi(CST_VAL_STRING(v));
    cst_error();
}

/* 0x55488 */
float val_float(const cst_val *v)
{
    if (v && CST_VAL_TYPE(v) == CST_VAL_TYPE_INT)
        return (float)CST_VAL_INT(v);
    else if (v && CST_VAL_TYPE(v) == CST_VAL_TYPE_FLOAT)
        return CST_VAL_FLOAT(v);
    else if (v && CST_VAL_TYPE(v) == CST_VAL_TYPE_STRING)
        return cst_atof(CST_VAL_STRING(v));
    cst_error();
}

/* 0x554dc */
const char *val_string(const cst_val *v)
{
    if (v && CST_VAL_TYPE(v) == CST_VAL_TYPE_STRING)
        return CST_VAL_STRING(v);
    cst_error();
}

/* 0x55504 */
void *val_void(const cst_val *v)
{
    if (v == NULL || CST_VAL_TYPE(v) == CST_VAL_TYPE_CONS || CST_VAL_TYPE(v) == CST_VAL_TYPE_INT
        || CST_VAL_TYPE(v) == CST_VAL_TYPE_FLOAT)
        cst_error();
    return CST_VAL_VOID(v);
}

/* 0x55538: non-cons cells have the low bit of their first word set */
int cst_val_consp(const cst_val *v)
{
    const cst_val_atom *t = (const cst_val_atom *)v;

    if (t->type % 2 == 0)
        return TRUE;
    return FALSE;
}

/* 0x55548 */
const cst_val *val_car(const cst_val *v)
{
    if (v && cst_val_consp(v))
        return CST_VAL_CAR(v);
    cst_error();
}

/* 0x55574 */
const cst_val *val_cdr(const cst_val *v)
{
    if (v && cst_val_consp(v))
        return CST_VAL_CDR(v);
    cst_error();
}

/* 0x555a0: last cons cell of a list */
const cst_val *val_last(const cst_val *v)
{
    while (val_cdr(v))
        v = val_cdr(v);
    return v;
}

/* 0x555cc */
void val_print(cst_file fd, const cst_val *v)
{
    const cst_val *p;

    if (v == NULL)
        cst_fprintf(fd, "[null]");
    else if (CST_VAL_TYPE(v) == CST_VAL_TYPE_INT)
        cst_fprintf(fd, "%d", val_int(v));
    else if (CST_VAL_TYPE(v) == CST_VAL_TYPE_FLOAT)
        cst_fprintf(fd, "%f", val_float(v));
    else if (CST_VAL_TYPE(v) == CST_VAL_TYPE_STRING)
        cst_fprintf(fd, "%s", val_string(v));
    else if (cst_val_consp(v)) {
        cst_fprintf(fd, "(");
        for (p = v; p; ) {
            val_print(fd, val_car(p));
            p = val_cdr(p);
            if (p)
                cst_fprintf(fd, " ");
        }
        cst_fprintf(fd, ")");
    } else
        cst_fprintf(fd, "[Val %s %p]", cst_val_defs[CST_VAL_TYPE(v) / 2].name, CST_VAL_VOID(v));
}

/* 0x5572c: destructively reverse a list */
cst_val *val_reverse(cst_val *l)
{
    cst_val *n, *np, *nl;

    for (nl = 0, n = l; n; nl = n, n = np) {
        np = CST_VAL_CDR(n);
        CST_VAL_CDR(n) = nl;
    }
    return nl;
}

/* 0x55754: destructively append l2 to l1 */
cst_val *val_append(cst_val *l1, cst_val *l2)
{
    cst_val *t;

    if (l1 == NULL)
        return l2;
    for (t = l1; val_cdr(t); t = CST_VAL_CDR(t))
        ;
    CST_VAL_CDR(t) = l2;
    return l1;
}

/* 0x55794: destructively append every list argument; the list is terminated by (cst_val *)-1 */
cst_val *val_append_all(cst_val *first, ...)
{
    va_list ap;
    cst_val *head = first, *tail = NULL, *l;

    va_start(ap, first);
    for (l = first; l != (cst_val *)-1; l = va_arg(ap, cst_val *)) {
        if (l == NULL)
            continue;
        if (tail != NULL)
            CST_VAL_CDR(tail) = l;
        else
            head = tail = l;
        for (; val_cdr(tail); tail = CST_VAL_CDR(tail))
            ;
    }
    va_end(ap);
    return head;
}

/* 0x55818 */
int val_length(const cst_val *l)
{
    int i;

    for (i = 0; l; l = val_cdr(l))
        i++;
    return i;
}

#define VAL_SCALARP(t) ((t) == CST_VAL_TYPE_INT || (t) == CST_VAL_TYPE_FLOAT || (t) == CST_VAL_TYPE_STRING)

/* 0x55848: structural equality; ints, floats and strings of different types compare as floats */
int val_equal(const cst_val *v1, const cst_val *v2)
{
    if (v1 == v2)
        return TRUE;
    if (v1 == NULL)
        return FALSE;
    if (v2 == NULL)
        return FALSE;
    if (cst_val_consp(v1) && cst_val_consp(v2))
        return val_equal(val_car(v1), val_car(v2)) && val_equal(val_cdr(v1), val_cdr(v2));
    if (CST_VAL_TYPE(v1) != CST_VAL_TYPE(v2)) {
        if (VAL_SCALARP(CST_VAL_TYPE(v1)) && VAL_SCALARP(CST_VAL_TYPE(v2)))
            return val_float(v1) == val_float(v2);
        return FALSE;
    }
    if (CST_VAL_TYPE(v1) == CST_VAL_TYPE_INT)
        return val_int(v1) == val_int(v2);
    else if (CST_VAL_TYPE(v1) == CST_VAL_TYPE_FLOAT)
        return val_float(v1) == val_float(v2);
    else if (CST_VAL_TYPE(v1) == CST_VAL_TYPE_STRING)
        return cst_streq(CST_VAL_STRING(v1), CST_VAL_STRING(v2));
    return CST_VAL_VOID(v1) == CST_VAL_VOID(v2);
}

/* 0x559c4 */
int val_less(const cst_val *v1, const cst_val *v2)
{
    return val_float(v1) < val_float(v2);
}

/* 0x559f4 */
int val_greater(const cst_val *v1, const cst_val *v2)
{
    return val_float(v1) > val_float(v2);
}

/* 0x55a24 */
int val_member(const cst_val *v1, const cst_val *l)
{
    const cst_val *i;

    for (i = l; i; i = val_cdr(i))
        if (val_equal(val_car(i), v1))
            return TRUE;
    return FALSE;
}

/* 0x55a70 */
int val_member_string(const char *v1, const cst_val *l)
{
    const cst_val *i;

    for (i = l; i; i = val_cdr(i))
        if (cst_streq(v1, val_string(val_car(i))))
            return TRUE;
    return FALSE;
}

/* 0x55ac4 */
int val_member_int(int v1, const cst_val *l)
{
    const cst_val *i;

    for (i = l; i; i = val_cdr(i))
        if (val_int(val_car(i)) == v1)
            return TRUE;
    return FALSE;
}

/* 0x55b0c */
cst_val *val_inc_refcount(const cst_val *b)
{
    cst_val *t = (cst_val *)(void *)b;

    if (CST_VAL_REFCOUNT(t) == -1)
        ;  /* constant value -- do nothing */
    else if (cst_val_consp(t))
        ;  /* cons cells have no ref count -- so do nothing */
    else
        CST_VAL_REFCOUNT(t) += 1;
    return t;
}

/* 0x55b3c */
cst_val *cons_val(const cst_val *a, const cst_val *b)
{
    cst_val *v = cst_alloc(cst_val, 1);

    CST_VAL_CAR(v) = ((!a || cst_val_consp(a)) ? (cst_val *)(void *)a : val_inc_refcount(a));
    CST_VAL_CDR(v) = ((!b || cst_val_consp(b)) ? (cst_val *)(void *)b : val_inc_refcount(b));
    return v;
}

/* 0x55bac: copy the cons structure of a value; atoms are shared */
cst_val *copy_val(const cst_val *v)
{
    if (v == NULL || !cst_val_consp(v))
        return (cst_val *)v;
    return cons_val(copy_val(CST_VAL_CAR(v)), copy_val(CST_VAL_CDR(v)));
}

/* 0x55bf0 */
int val_dec_refcount(const cst_val *b)
{
    cst_val *t = (cst_val *)(void *)b;

    if (CST_VAL_REFCOUNT(t) == -1)
        return -1;
    else if (cst_val_consp(t))
        return 0;
    else if (CST_VAL_REFCOUNT(t) == 0)
        return 0;
    CST_VAL_REFCOUNT(t) -= 1;
    return CST_VAL_REFCOUNT(t);
}

/* 0x55c48 */
void delete_val(cst_val *v)
{
    if (v) {
        if (cst_val_consp(v)) {
            delete_val(CST_VAL_CAR(v));
            delete_val(CST_VAL_CDR(v));
            cst_free(v);
        } else if (val_dec_refcount(v) == 0) {
            if (CST_VAL_TYPE(v) == CST_VAL_TYPE_STRING)
                cst_free(CST_VAL_VOID(v));
            else if (CST_VAL_TYPE(v) >= CST_VAL_TYPE_FIRST_FREE)
                (cst_val_defs[CST_VAL_TYPE(v) / 2].delete_function)(CST_VAL_VOID(v));
            cst_free(v);
        }
    }
}

/* 0x55ce4 */
void delete_val_list(cst_val *v)
{
    if (v) {
        if (cst_val_consp(v)) {
            delete_val_list(CST_VAL_CDR(v));
            cst_free(v);
        } else
            delete_val(v);
    }
}

/* 0x55d1c: destructive set cdr */
const cst_val *set_cdr(cst_val *v1, const cst_val *v2)
{
    if (!cst_val_consp(v1))
        cst_error();
    if (CST_VAL_CDR(v1))
        val_dec_refcount(CST_VAL_CDR(v1));
    val_inc_refcount(v1);
    CST_VAL_CDR(v1) = (cst_val *)v2;
    return v1;
}

/* 0x55d60: destructive set car */
const cst_val *set_car(cst_val *v1, const cst_val *v2)
{
    if (!cst_val_consp(v1))
        cst_error();
    if (CST_VAL_CAR(v1))
        val_dec_refcount(CST_VAL_CAR(v1));
    val_inc_refcount(v1);
    CST_VAL_CAR(v1) = (cst_val *)v2;
    return v1;
}

/* 0x55e78 */
const cst_val *val_assoc_string(const char *v1, const cst_val *al)
{
    const cst_val *i;

    for (i = al; i; i = val_cdr(i))
        if (cst_streq(v1, val_string(val_car(val_car(i)))))
            return val_car(i);
    return NULL;
}

/* 0x55ed0: the sublist holding the last n elements (the whole list if it is shorter) */
const cst_val *val_last_n(const cst_val *l, unsigned int n)
{
    unsigned int len = val_length(l);

    for (; n < len; n++)
        l = val_cdr(l);
    return l;
}

/* 0x55f0c: destructively drop the last n elements */
cst_val *val_drop_last(cst_val *l, unsigned int n)
{
    cst_val *t, *rest;

    if (n == 0)
        return l;
    if (n >= (unsigned int)val_length(l)) {
        delete_val(l);
        return NULL;
    }
    t = (cst_val *)val_last_n(l, n + 1);
    rest = (cst_val *)val_cdr(t);
    set_cdr(t, NULL);
    delete_val(rest);
    return l;
}

/* 0x55f70 */
cst_val *int_string_val(int i)
{
    char *s = cst_alloc(char, 16);

    sprintf(s, "%d", i);
    return string_val_consume(s);
}
