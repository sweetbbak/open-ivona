/*
 * Flite cst_alloc.c as modified by IVONA: allocations take (count, size), reject
 * multiplication overflow and zero-size requests become one element.
 */
#include <limits.h>
#include <stdlib.h>
#include "cst_alloc.h"
#include "cst_error.h"

/* 0x51910: zero-filled allocation of count*size bytes; exits on overflow or failure.
 * NONMATCHING: GCC emits `cmp r4, r0; bhi` where the original has `cmp r0, r4; blo`
 * (-fno-tree-dominator-opts reproduces the original, so the source shape differs). */
__attribute__((optimize("no-tree-dominator-opts")))
void *cst_safe_alloc(unsigned int count, unsigned int size)
{
    void *p;

    if (count == 0)
        count = 1;
    if (UINT_MAX / size < count)
        cst_error();
    p = calloc(size * count, 1);
    if (p == NULL)
        cst_error();
    return p;
}

/* 0x51954 */
void *cst_safe_calloc(unsigned int count, unsigned int size)
{
    return cst_safe_alloc(count, size);
}

/* 0x51958: resize to count*size bytes (allocates when p is NULL); exits on overflow or failure.
 * NONMATCHING: same compare-operand swap as cst_safe_alloc. */
__attribute__((optimize("no-tree-dominator-opts")))
void *cst_safe_realloc(void *p, unsigned int count, unsigned int size)
{
    void *np;
    unsigned int n = count;

    if (n == 0)
        n = 1;
    if (UINT_MAX / size < n)
        cst_error();
    n *= size;
    if (n == 0)
        n = 1;
    if (p == NULL)
        np = cst_safe_alloc(count, size);
    else
        np = realloc(p, n);
    if (np == NULL)
        cst_error();
    return np;
}

/* 0x519c4 */
void cst_free(void *p)
{
    if (p != NULL)
        free(p);
}
