/* IVONA engine: growable array of fixed-size elements (libttsivonavoice 0x5a44c..0x5ab54). */
#ifndef IVONA_VECTOR_H
#define IVONA_VECTOR_H

#include <stddef.h>
#include "cst_val.h"

typedef void *(*ivona_vector_copy_fn)(void *dst, const void *src, size_t size);
typedef void (*ivona_vector_destroy_fn)(void *elem, size_t size);
typedef int (*ivona_vector_cmp_fn)(const void *elem, const void *key);

typedef struct ivona_vector {
    size_t elem_size;
    size_t count;
    size_t capacity;
    void *data;
    ivona_vector_copy_fn copy;        /* memcpy unless given */
    ivona_vector_destroy_fn destroy;  /* optional per-element destructor */
} ivona_vector;

CST_VAL_USER_TYPE_DCLS(vector, ivona_vector)

ivona_vector *vector_new(size_t elem_size, ivona_vector_copy_fn copy, ivona_vector_destroy_fn destroy);
void vector_reserve(ivona_vector *v, size_t n);
ivona_vector *vector_from_array(const void *data, size_t elem_size, size_t n,
                                ivona_vector_copy_fn copy, ivona_vector_destroy_fn destroy);
void vector_free_ptr(void *p);
ivona_vector *vector_append_vector(ivona_vector *v, const ivona_vector *src);
ivona_vector *vector_copy(const ivona_vector *src);
void vector_insert(ivona_vector *v, size_t idx, const void *elem);
void vector_push(ivona_vector *v, const void *elem);
void vector_swap(ivona_vector *v, size_t a, size_t b);
void vector_reverse(ivona_vector *v);
void vector_erase(ivona_vector *v, size_t idx, size_t n);
void vector_clear(ivona_vector *v);
void vector_free(ivona_vector *v);
void vector_pop(ivona_vector *v);
void vector_pop_front(ivona_vector *v);
int vector_find(const ivona_vector *v, const void *key, ivona_vector_cmp_fn cmp);
int vector_bsearch(const ivona_vector *v, const void *key, ivona_vector_cmp_fn cmp);
int vector_lower_bound(const ivona_vector *v, const void *key, ivona_vector_cmp_fn cmp);
void vector_sort(ivona_vector *v, int (*cmp)(const void *, const void *));
size_t vector_size(const ivona_vector *v);
void vector_set(ivona_vector *v, size_t idx, const void *elem);
void *vector_at(const ivona_vector *v, size_t idx);
void vector_remove_ptr(ivona_vector *v, const void *ptr);
void *vector_get(const ivona_vector *v, size_t idx);
int vector_equal(const ivona_vector *a, const ivona_vector *b, ivona_vector_cmp_fn cmp);

#endif
