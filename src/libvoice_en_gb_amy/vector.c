/* IVONA engine: growable array of fixed-size elements, also usable as a cst_val. */
#include <stdlib.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_error.h"
#include "ivona_vector.h"

/* 0x277b4: arm */
ivona_vector *val_vector(const cst_val *v)
{
    if (v && CST_VAL_TYPE(v) == cst_val_type_vector)
        return (ivona_vector *)CST_VAL_VOID(v);
    cst_errmsg("VAL: tried to access %s in %d type val\n", "vector",
               (v ? CST_VAL_TYPE(v) : -1));
    cst_error();
}

/* 0x2780c: arm */
cst_val *vector_val(const ivona_vector *v)
{
    return val_new_typed(cst_val_type_vector, (void *)v);
}

/* 0x27824: arm */
ivona_vector *vector_new(size_t elem_size, ivona_vector_copy_fn copy, ivona_vector_destroy_fn destroy)
{
    ivona_vector *v = cst_calloc(ivona_vector, 1);

    v->elem_size = elem_size;
    if (copy == NULL)
        copy = memcpy;
    v->copy = copy;
    v->destroy = destroy;
    return v;
}

/* 0x27868: arm: make room for at least n elements */
void vector_reserve(ivona_vector *v, size_t n)
{
    if (v->capacity >= n)
        return;
    v->capacity = n;
    v->data = cst_safe_realloc(v->data, n, v->elem_size);
}

/* 0x27894: arm: append copies of all elements of src */
ivona_vector *vector_append_vector(ivona_vector *v, const ivona_vector *src)
{
    size_t i;

    vector_reserve(v, v->count + src->count);
    for (i = 0; i < src->count; i++)
        v->copy((char *)v->data + (i + v->count) * v->elem_size,
                (const char *)src->data + i * src->elem_size, v->elem_size);
    v->count += src->count;
    return v;
}

/* 0x2790c: arm */
ivona_vector *vector_copy(const ivona_vector *src)
{
    return vector_append_vector(vector_new(src->elem_size, src->copy, src->destroy), src);
}

/* 0x27930: arm */
void vector_insert(ivona_vector *v, size_t idx, const void *elem)
{
    if (v->count + 1 > v->capacity)
        vector_reserve(v, v->count + 2 + (v->count + 1) / 2);
    memmove((char *)v->data + (idx + 1) * v->elem_size, (char *)v->data + idx * v->elem_size,
            (v->count - idx) * v->elem_size);
    v->copy((char *)v->data + idx * v->elem_size, elem, v->elem_size);
    v->count++;
}

/* 0x279b0: arm */
void vector_push(ivona_vector *v, const void *elem)
{
    if (v->count + 1 > v->capacity)
        vector_reserve(v, v->count + 2 + (v->count + 1) / 2);
    v->copy((char *)v->data + v->count * v->elem_size, elem, v->elem_size);
    v->count++;
}

/* 0x27a04: arm: swap two elements (elements must not exceed 2 KiB) */
void vector_swap(ivona_vector *v, size_t a, size_t b)
{
    char tmp[2048];

    if (a != b && a < v->count && b < v->count) {
        memcpy(tmp, (char *)v->data + a * v->elem_size, v->elem_size);
        memcpy((char *)v->data + a * v->elem_size, (char *)v->data + b * v->elem_size, v->elem_size);
        memcpy((char *)v->data + b * v->elem_size, tmp, v->elem_size);
    }
}

/* 0x27a8c: arm */
void vector_reverse(ivona_vector *v)
{
    size_t i;

    if (v->count < 2)
        return;
    for (i = 0; i < v->count - 1 - i; i++)
        vector_swap(v, i, v->count - 1 - i);
}

/* 0x27ad0: arm: remove n elements starting at idx */
void vector_erase(ivona_vector *v, size_t idx, size_t n)
{
    size_t end;

    if (__builtin_expect(v->destroy != NULL, 1)) {
        if (n == 0)
            end = idx;
        else {
            size_t i = idx;
            end = i + n;
            do {
                v->destroy((char *)v->data + i * v->elem_size, v->elem_size);
                i++;
            } while (i != end);
        }
    } else {
        end = idx + n;
    }
    memmove((char *)v->data + idx * v->elem_size,
            (char *)v->data + end * v->elem_size,
            (v->count - idx - n) * v->elem_size);
    v->count -= n;
}

/* 0x27b64: arm */
void vector_clear(ivona_vector *v)
{
    vector_erase(v, 0, v->count);
}

/* 0x27b70: arm */
void vector_free(ivona_vector *v)
{
    if (v == NULL)
        return;
    vector_clear(v);
    cst_free(v->data);
    memset(v, 0, sizeof(*v));
    cst_free(v);
}

/* 0x27ba4: arm */
void val_delete_vector(void *v)
{
    vector_free((ivona_vector *)v);
}

/* 0x27ba8: arm */
void vector_pop(ivona_vector *v)
{
    vector_erase(v, v->count - 1, 1);
}

/* 0x27bb8: arm */
void vector_pop_front(ivona_vector *v)
{
    vector_erase(v, 0, 1);
}

/* 0x27bc4: arm: index of the first element for which cmp(elem, key) == 0, or -1 */
int vector_find(const ivona_vector *v, const void *key, ivona_vector_cmp_fn cmp)
{
    size_t i;

    for (i = 0; i < v->count; i++)
        if (cmp((char *)v->data + i * v->elem_size, key) == 0)
            return i;
    return -1;
}

/* 0x27c24: arm: binary search in a sorted vector; index or -1 */
int vector_bsearch(const ivona_vector *v, const void *key, ivona_vector_cmp_fn cmp)
{
    int lo = 0, hi = v->count - 1, mid, r;

    while (lo <= hi) {
        mid = (lo + hi) >> 1;
        r = cmp((char *)v->data + mid * v->elem_size, key);
        if (r < 0)
            lo = mid + 1;
        else if (r == 0)
            return mid;
        else
            hi = mid - 1;
    }
    return -1;
}

/* 0x27c8c: arm: first index whose element is not less than key */
int vector_lower_bound(const ivona_vector *v, const void *key, ivona_vector_cmp_fn cmp)
{
    int lo = 0;
    int hi = v->count - 1;
    int mid;

    while (lo <= hi) {
        mid = (lo + hi) >> 1;
        if (cmp((char *)v->data + mid * v->elem_size, key) < 0)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    /* sanity checks whose results are unused (assertions compiled out) */
    if (lo > 0)
        cmp((char *)v->data + (lo - 1) * v->elem_size, key);
    asm goto("cmp %0, %1\n\tbhs %l[done]" : : "r"(lo), "r"(v->count) : : done);
    cmp((char *)v->data + lo * v->elem_size, key);
done:
    return lo;
}

/* 0x27d24: arm */
void vector_sort(ivona_vector *v, int (*cmp)(const void *, const void *))
{
    qsort(v->data, v->count, v->elem_size, cmp);
}

/* 0x27d3c: arm */
size_t vector_size(const ivona_vector *v)
{
    return v->count;
}

/* 0x27d44: arm: replace element idx (destroying the old one) */
void vector_set(ivona_vector *v, size_t idx, const void *elem)
{
    if (v->destroy)
        v->destroy((char *)v->data + idx * v->elem_size, v->elem_size);
    v->copy((char *)v->data + idx * v->elem_size, elem, v->elem_size);
}

/* 0x27d90: arm */
void *vector_at(const ivona_vector *v, size_t idx)
{
    return (char *)v->data + idx * v->elem_size;
}

/* 0x27da0: arm: remove the first element that holds pointer ptr */
void vector_remove_ptr(ivona_vector *v, const void *ptr)
{
    size_t i;

    for (i = 0; i < vector_size(v); i++)
        if (*(void **)vector_at(v, i) == ptr) {
            vector_erase(v, i, 1);
            return;
        }
}

/* 0x27df8: arm */
void *vector_get(const ivona_vector *v, size_t idx)
{
    return (char *)v->data + idx * v->elem_size;
}

/* 0x27e08: arm: element-wise equality, with cmp or (without) memcmp */
int vector_equal(const ivona_vector *a, const ivona_vector *b, ivona_vector_cmp_fn cmp)
{
    size_t i;

    if (a->count != b->count)
        return 0;
    for (i = 0; i < a->count; i++) {
        if (cmp) {
            if (cmp((char *)a->data + i * a->elem_size, (char *)b->data + i * b->elem_size) != 0)
                return 0;
        } else if (memcmp((char *)a->data + a->elem_size * i, (char *)b->data + b->elem_size * i, a->elem_size) != 0)
            return 0;
    }
    return 1;
}

/* 0x27ebc: arm: a vector holding copies of the n elements at data */
ivona_vector *vector_from_array(const void *data, size_t elem_size, size_t n,
                                ivona_vector_copy_fn copy, ivona_vector_destroy_fn destroy)
{
    ivona_vector *v = vector_new(elem_size, copy, destroy);
    size_t i;

    vector_reserve(v, n);
    for (i = 0; i < n; i++)
        vector_push(v, (const char *)data + i * elem_size);
    return v;
}

/* 0x27f18: arm: destroy function for elements that are vectors */
void vector_free_ptr(void *p)
{
    vector_free(*(ivona_vector **)p);
}
