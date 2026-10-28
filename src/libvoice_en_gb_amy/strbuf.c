/*
 * Growable strings (strbuf) and monotonic timers (0x33a24..0x33e94).
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "ivona_util.h"

/* 0x33a24 arm */
strbuf *strbuf_new(void)
{
    return cst_safe_calloc(1, sizeof(strbuf));
}

/* 0x33a30 arm */
void strbuf_free(strbuf *b)
{
    if (b == NULL)
        return;
    cst_free(b->data);
    cst_free(b);
}

/* 0x33a50 arm: for strbufs embedded in other objects or on the stack. */
void strbuf_init(strbuf *b)
{
    memset(b, 0, sizeof(*b));
}

/* 0x33a90 arm */
void strbuf_release(strbuf *b)
{
    cst_free(b->data);
}

/* 0x33a98 arm */
char *strbuf_data(strbuf *b)
{
    return b->data;
}

/* 0x33aa0 arm: makes room for n bytes (growing by half again). */
void strbuf_reserve(strbuf *b, size_t n)
{
    if (b->cap >= n)
        return;
    b->cap = n + 1 + n / 2;
    b->data = cst_safe_realloc(b->data, b->cap, 1);
}

/* 0x33ad4 arm */
void strbuf_clear(strbuf *b)
{
    b->len = 0;
    if (b->data)
        b->data[0] = '\0';
}

/* 0x33aec arm: sets the length (contents beyond the old length are undefined). */
void strbuf_resize(strbuf *b, size_t n)
{
    strbuf_reserve(b, n + 1);
    b->len = n;
    b->data[n] = '\0';
}

/* 0x33b14 arm */
void strbuf_append(strbuf *b, const void *data, size_t n)
{
    strbuf_reserve(b, b->len + 1 + n);
    memcpy(b->data + b->len, data, n);
    b->len += n;
    b->data[b->len] = '\0';
}

/* 0x33b68 arm */
void strbuf_append_str(strbuf *b, const char *s)
{
    strbuf_append(b, s, strlen(s));
}

/* 0x33b90 arm */
void strbuf_insert(strbuf *b, size_t pos, const void *data, size_t n)
{
    strbuf_reserve(b, b->len + 1 + n);
    memmove(b->data + pos + n, b->data + pos, b->len + 1 - pos);
    memcpy(b->data + pos, data, n);
    b->len += n;
}

/* 0x33bf8 arm */
void strbuf_insert_str(strbuf *b, size_t pos, const char *s)
{
    strbuf_insert(b, pos, s, strlen(s));
}

/* 0x33c28 arm: removes n bytes at pos. */
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

/* 0x33c90 arm */
void strbuf_swap(strbuf *a, strbuf *b)
{
    strbuf t = *a;
    *a = *b;
    *b = t;
}

/* 0x33cc8 arm */
void strbuf_truncate(strbuf *b, size_t n)
{
    if (b->len > n) {
        b->len = n;
        b->data[n] = '\0';
    }
}

/* 0x33ce4 arm: appends at most max formatted bytes; returns vsnprintf's result. */
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

/* 0x33d44 arm: formats into the spare capacity first, retrying with the exact size if needed. */
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

/* 0x33dd8 arm */
int strbuf_appendf_n(strbuf *b, int max, const char *fmt, ...)
{
    va_list ap;
    int n;

    va_start(ap, fmt);
    n = strbuf_vappendf_n(b, max, fmt, ap);
    va_end(ap);
    return n;
}

/* 0x33e08 arm */
unsigned int ivona_time_msec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + (unsigned int)(ts.tv_nsec / 1000) / 1000;
}

/* 0x33e50 arm */
int ivona_time_usec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}
