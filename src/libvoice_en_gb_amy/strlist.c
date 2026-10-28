/*
 * Lists of owned strings (0x62240..0x627a0).
 */
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "ivona_util.h"

/* 0x62240 arm */
strlist *strlist_new(void)
{
    return cst_safe_calloc(1, sizeof(strlist));
}

/* 0x6224c arm: deep copy. */
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

/* 0x622c8 arm */
void strlist_clear(strlist *l)
{
    size_t i;

    for (i = 0; i < l->count; i++)
        cst_free(l->items[i]);
    l->count = 0;
}

/* 0x62308 arm */
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

/* 0x6232c arm: appends a copy of s (NULL is stored as is). */
void strlist_push(strlist *l, const char *s)
{
    strlist_reserve(l, l->count + 1);
    l->items[l->count++] = s ? cst_strdup(s) : NULL;
}

/* 0x62398 arm: appends a copy of the first n bytes of s. */
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

/* 0x6240c arm: appends the whitespace-separated words of s. */
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

/* 0x62480 arm */
strlist *strlist_split_ws(const char *s)
{
    strlist *l = strlist_new();
    strlist_split_ws_into(l, s);
    return l;
}

/* 0x624a0 arm: appends the pieces of s separated by delim. */
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

/* 0x624fc arm */
strlist *strlist_split(const char *s, const char *delim)
{
    strlist *l = strlist_new();
    strlist_split_into(l, s, delim);
    return l;
}

/* 0x62524 arm */
int strlist_contains(const strlist *l, const char *s)
{
    size_t i;

    for (i = 0; i < l->count; i++)
        if (strcmp(s, l->items[i]) == 0)
            return 1;
    return 0;
}

/* 0x62578 arm */
int strlist_contains_nocase(const strlist *l, const char *s)
{
    size_t i;

    for (i = 0; i < l->count; i++)
        if (strcasecmp(s, l->items[i]) == 0)
            return 1;
    return 0;
}

/* 0x625cc arm */
void strlist_print(const strlist *l, FILE *fp, const char *sep)
{
    size_t i;

    for (i = 0; i < l->count; i++) {
        if (i)
            fputs(sep, fp);
        fputs(l->items[i], fp);
    }
}

/* 0x62628 arm: newly allocated string. */
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

/* 0x626a8 arm: lexicographic order. */
int strlist_compare(const strlist *a, const strlist *b)
{
    size_t i;
    int r;

    for (i = 0; i < a->count; i++) {
        if (i == b->count)
            break;
        r = strcmp(a->items[i], b->items[i]);
        if (r)
            return r;
    }
    if (a->count < b->count)
        return -1;
    return b->count != a->count;
}

/* 0x62728 arm: qsort comparator for strlist pointers. */
int strlist_compare_ptr(const void *a, const void *b)
{
    return strlist_compare(*(strlist *const *)a, *(strlist *const *)b);
}

/* 0x62734 arm */
void strlist_pop(strlist *l)
{
    cst_free(l->items[--l->count]);
}

/* 0x62748 arm */
void strlist_remove(strlist *l, size_t i)
{
    cst_free(l->items[i]);
    memmove(&l->items[i], &l->items[i + 1], (l->count - i - 1) * sizeof(char *));
    l->count--;
}

/* 0x62798 arm: destructor for containers holding strlist pointers. */
void strlist_free_ptr(void *p)
{
    strlist_free(*(strlist **)p);
}
