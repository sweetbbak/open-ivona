/*
 * String pool (0x476a0..0x47868): interned strings packed into 4 KiB chunks, freed all at once.
 * Chunks with room left are kept on `partial`; nearly full ones move to `full`.
 */
#include <string.h>
#include "cst_alloc.h"

#define STRPOOL_CHUNK_DATA 0xff8

typedef struct strpool_chunk {
    struct strpool_chunk *next;
    int used;
    char data[];
} strpool_chunk;

typedef struct strpool {
    strpool_chunk *full;
    strpool_chunk *partial;
} strpool;

/* 0x476a0 arm */
strpool *strpool_new(void)
{
    strpool *p = cst_safe_alloc(1, sizeof(strpool));

    p->full = NULL;
    p->partial = NULL;
    return p;
}

/* 0x476c0 arm */
void strpool_free(strpool *p)
{
    strpool_chunk *c, *next;

    for (c = p->full; c; c = next) {
        next = c->next;
        cst_free(c);
    }
    for (c = p->partial; c; c = next) {
        next = c->next;
        cst_free(c);
    }
    cst_free(p);
}

/* 0x47714 arm: a pooled, NUL-terminated copy of n bytes of s. */
char *strpool_add_n(strpool *p, const char *s, size_t n)
{
    strpool_chunk *c, *prev;
    char *d;

    if (n == 0)
        return "";
    for (prev = NULL, c = p->partial; c; prev = c, c = c->next)
        if (c->used + 1 + n <= STRPOOL_CHUNK_DATA)
            break;
    if (c == NULL) {
        c = cst_safe_alloc(n + 1 <= STRPOOL_CHUNK_DATA ? 0x1000 : n + 9, 1);
        c->next = NULL;
        c->used = 0;
        c->next = p->partial;
        p->partial = c;
    }
    d = memcpy(c->data + c->used, s, n);
    d[n] = '\0';
    c->used += n + 1;
    if (c->used > STRPOOL_CHUNK_DATA - 2) {
        if (p->partial == c)
            p->partial = c->next;
        else
            prev->next = c->next;
        c->next = p->full;
        p->full = c;
    }
    return d;
}

/* 0x47840 arm */
char *strpool_add(strpool *p, const char *s)
{
    return strpool_add_n(p, s, strlen(s));
}
