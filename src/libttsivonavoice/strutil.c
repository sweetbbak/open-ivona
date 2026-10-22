/*
 * Growable strings (strbuf), lists of owned strings (strlist) and monotonic timers
 * (0x574bc..0x57e8c).
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "ivona_util.h"

/* 0x574bc arm */
strbuf *strbuf_new(void)
{
    return cst_safe_calloc(1, sizeof(strbuf));
}

/* 0x574c8 arm */
void strbuf_free(strbuf *b)
{
    if (b == NULL)
        return;
    cst_free(b->data);
    cst_free(b);
}

/* 0x574e8 arm: for strbufs embedded in other objects or on the stack. */
void strbuf_init(strbuf *b)
{
    memset(b, 0, sizeof(*b));
}

/* 0x57528 arm */
void strbuf_release(strbuf *b)
{
    cst_free(b->data);
}

/* 0x57530 arm */
char *strbuf_data(strbuf *b)
{
    return b->data;
}

/* 0x57538 arm: makes room for n bytes (growing by half again). */
void strbuf_reserve(strbuf *b, size_t n)
{
    if (b->cap >= n)
        return;
    b->cap = n + 1 + n / 2;
    b->data = cst_safe_realloc(b->data, b->cap, 1);
}

/* 0x5756c arm */
void strbuf_clear(strbuf *b)
{
    b->len = 0;
    if (b->data)
        b->data[0] = '\0';
}

/* 0x57584 arm: sets the length (contents beyond the old length are undefined). */
void strbuf_resize(strbuf *b, size_t n)
{
    strbuf_reserve(b, n + 1);
    b->len = n;
    b->data[n] = '\0';
}

/* 0x575ac arm */
void strbuf_append(strbuf *b, const void *data, size_t n)
{
    strbuf_reserve(b, b->len + 1 + n);
    memcpy(b->data + b->len, data, n);
    b->len += n;
    b->data[b->len] = '\0';
}

/* 0x57600 arm */
void strbuf_append_str(strbuf *b, const char *s)
{
    strbuf_append(b, s, strlen(s));
}

/* 0x57628 arm */
void strbuf_insert(strbuf *b, size_t pos, const void *data, size_t n)
{
    strbuf_reserve(b, b->len + 1 + n);
    memmove(b->data + pos + n, b->data + pos, b->len + 1 - pos);
    memcpy(b->data + pos, data, n);
    b->len += n;
}

/* 0x57690 arm */
void strbuf_insert_str(strbuf *b, size_t pos, const char *s)
{
    strbuf_insert(b, pos, s, strlen(s));
}

/* 0x576c0 arm: removes n bytes at pos. */
void strbuf_erase(strbuf *b, size_t pos, size_t n)
{
    if (pos + n >= b->len) {
        if (pos < b->len) {
            b->len = pos;
            b->data[pos] = '\0';
        }
        return;
    }
    memmove(b->data + pos, b->data + pos + n, b->len - pos - n);
    b->len -= n;
    b->data[b->len] = '\0';
}

/* 0x57728 arm */
void strbuf_swap(strbuf *a, strbuf *b)
{
    strbuf t = *a;
    *a = *b;
    *b = t;
}

/* 0x57760 arm */
void strbuf_truncate(strbuf *b, size_t n)
{
    if (b->len > n) {
        b->len = n;
        b->data[n] = '\0';
    }
}

/* 0x5777c arm: appends at most max formatted bytes; returns vsnprintf's result. */
int strbuf_vappendf_n(strbuf *b, int max, const char *fmt, va_list ap)
{
    int n;

    strbuf_reserve(b, b->len + 1 + max);
    n = vsnprintf(b->data + b->len, max + 1, fmt, ap);
    if (n < 0)
        return n;
    b->len += n > max ? max : n;
    return n;
}

/* 0x577dc arm: formats into the spare capacity first, retrying with the exact size if needed. */
int strbuf_appendf(strbuf *b, const char *fmt, ...)
{
    va_list ap;
    int avail, n;

    avail = b->cap - b->len;
    if (avail > 0)
        avail--;
    va_start(ap, fmt);
    n = strbuf_vappendf_n(b, avail, fmt, ap);
    va_end(ap);
    if (n >= 0 && n >= avail) {
        b->len -= avail;
        if (b->data)
            b->data[b->len] = '\0';
        va_start(ap, fmt);
        n = strbuf_vappendf_n(b, n, fmt, ap);
        va_end(ap);
    }
    return n;
}

/* 0x57870 arm */
int strbuf_appendf_n(strbuf *b, int max, const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = strbuf_vappendf_n(b, max, fmt, ap);
    va_end(ap);
    return n;
}

/* 0x578a0 arm */
strlist *strlist_new(void)
{
    return cst_safe_calloc(1, sizeof(strlist));
}

/* 0x578ac arm: deep copy. */
strlist *strlist_copy(const strlist *src)
{
    strlist *l = strlist_new();
    size_t i;

    if (src->count) {
        l->cap = l->count = src->count;
        l->items = cst_safe_alloc(l->count, sizeof(char *));
        for (i = 0; i < src->count; i++)
            l->items[i] = cst_strdup(src->items[i]);
    }
    return l;
}

/* 0x57928 arm */
void strlist_clear(strlist *l)
{
    size_t i;

    for (i = 0; i < l->count; i++)
        cst_free(l->items[i]);
    l->count = 0;
}

/* 0x57968 arm */
void strlist_free(strlist *l)
{
    if (l == NULL)
        return;
    strlist_clear(l);
    cst_free(l->items);
    cst_free(l);
}

static void strlist_reserve(strlist *l, size_t n)
{
    if (n > l->cap) {
        l->cap = n + 1 + n / 2;
        l->items = cst_safe_realloc(l->items, l->cap, sizeof(char *));
    }
}

/* 0x5798c arm: appends a copy of s (NULL is stored as is). */
void strlist_push(strlist *l, const char *s)
{
    strlist_reserve(l, l->count + 1);
    l->items[l->count++] = s ? cst_strdup(s) : NULL;
}

/* 0x579f8 arm: appends a copy of the first n bytes of s. */
void strlist_push_n(strlist *l, const char *s, size_t n)
{
    char *copy;

    strlist_reserve(l, l->count + 1);
    copy = cst_safe_alloc(n + 1, 1);
    memcpy(copy, s, n);
    copy[n] = '\0';
    l->items[l->count++] = copy;
}

static const char strlist_ws[] = " \t\n\v\f\r\x85\xa0";

/* 0x57a6c arm: appends the whitespace-separated words of s. */
void strlist_split_ws_into(strlist *l, const char *s)
{
    const char *end;

    while (*(s += strspn(s, strlist_ws)) != '\0') {
        const char *p = s + 1;
        size_t n = strcspn(p, strlist_ws);
        end = p + n;
        strlist_push_n(l, s, end - s);
        if (p[n] == '\0')
            break;
        s = end + 1;
    }

}

/* 0x57ae0 arm */
strlist *strlist_split_ws(const char *s)
{
    strlist *l = strlist_new();
    strlist_split_ws_into(l, s);
    return l;
}

/* 0x57b00 arm: appends the pieces of s separated by delim. */
void strlist_split_into(strlist *l, const char *s, const char *delim)
{
    size_t n = strlen(delim);
    const char *p;

    if (n) {
        while ((p = strstr(s, delim)) != NULL) {
            strlist_push_n(l, s, p - s);
            s = p + n;
        }
    }
    strlist_push(l, s);
}

/* 0x57b5c arm */
strlist *strlist_split(const char *s, const char *delim)
{
    strlist *l = strlist_new();
    strlist_split_into(l, s, delim);
    return l;
}

/* 0x57b84 arm */
int strlist_contains(const strlist *l, const char *s)
{
    size_t i;

    for (i = 0; i < l->count; i++)
        if (strcmp(s, l->items[i]) == 0)
            return 1;
    return 0;
}

/* 0x57bd8 arm */
int strlist_contains_nocase(const strlist *l, const char *s)
{
    size_t i;

    for (i = 0; i < l->count; i++)
        if (strcasecmp(s, l->items[i]) == 0)
            return 1;
    return 0;
}

/* 0x57c2c arm */
void strlist_print(const strlist *l, FILE *fp, const char *sep)
{
    size_t i;

    for (i = 0; i < l->count; i++) {
        if (i)
            fputs(sep, fp);
        fputs(l->items[i], fp);
    }
}

/* 0x57c88 arm: newly allocated string. */
char *strlist_join(const strlist *l, const char *sep)
{
    strbuf b;
    size_t i;

    strbuf_init(&b);
    strbuf_reserve(&b, 1);
    for (i = 0; i < l->count; i++) {
        if (i)
            strbuf_append_str(&b, sep);
        strbuf_append_str(&b, l->items[i]);
    }
    return strbuf_data(&b);
}

/* 0x57d08 arm: lexicographic order. */
int strlist_compare(const strlist *a, const strlist *b)
{
    size_t na = a->count, nb = b->count;
    size_t i;
    int r;

    for (i = 0; i < na; i++) {
        if (i == nb)
            break;
        r = strcmp(a->items[i], b->items[i]);
        if (r)
            return r;
    }
    if (na < nb)
        return -1;
    return nb != na;

}

/* 0x57d88 arm: qsort comparator for strlist pointers. */
int strlist_compare_ptr(const void *a, const void *b)
{
    return strlist_compare(*(strlist *const *)a, *(strlist *const *)b);
}

/* 0x57d94 arm */
void strlist_pop(strlist *l)
{
    cst_free(l->items[--l->count]);
}

/* 0x57da8 arm */
void strlist_remove(strlist *l, size_t i)
{
    cst_free(l->items[i]);
    memmove(&l->items[i], &l->items[i + 1], (l->count - i - 1) * sizeof(char *));
    l->count--;
}

/* 0x57df8 arm: destructor for containers holding strlist pointers. */
void strlist_free_ptr(void *p)
{
    strlist_free(*(strlist **)p);
}

/* 0x57e00 arm */
unsigned int ivona_time_msec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + (unsigned int)(ts.tv_nsec / 1000) / 1000;
}

/* 0x57e48 arm */
int ivona_time_usec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}
