/* Flite cst_string.c as modified by IVONA. */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_string.h"

/* 0x5259c */
cst_string *cst_strrchr(const cst_string *str, int c)
{
    return (cst_string *)strrchr((const char *)str, (unsigned char)c);
}

/* 0x525a4 */
double cst_atof(const char *str)
{
    return strtod(str, NULL);
}

/* 0x525ac: like strchr, but returns the terminating NUL when c is not found. */
cst_string *cst_strchr(const cst_string *s, int c)
{
    cst_string *p = strchr(s, (unsigned char)c);

    if (p)
        return p;
    return (cst_string *)s + strlen(s);
}

/* 0x525d4: copy of a memory block (NULL for a NULL or empty block). */
void *cst_memdup(const void *p, size_t n)
{
    void *d;

    if (p && n) {
        d = cst_alloc(char, n);
        memcpy(d, p, n);
        return d;
    }
    return NULL;
}

/* 0x52610 */
cst_string *cst_strdup(const cst_string *str)
{
    if (str == NULL)
        return NULL;
    return cst_memdup(str, cst_strlen(str) + 1);
}

/* 0x52630 */
char *cst_substr(const char *str, int start, int length)
{
    char *nstr = NULL;

    if (str) {
        nstr = cst_alloc(char, length + 1);
        strncpy(nstr, str + start, length);
        nstr[length] = '\0';
    }
    return nstr;
}

/* 0x52678: copy at most n chars and always terminate; a NULL source clears dst. */
char *cst_strncpy(char *dst, const char *src, size_t n)
{
    if (src) {
        strncpy(dst, src, n);
        dst[n] = '\0';
    } else
        memset(dst, 0, n + 1);
    return dst;
}

/* 0x526b0: copy of s up to the first occurrence of c (c must occur in s). */
char *cst_string_before(const char *s, const char *c)
{
    char *p;
    char *q;

    p = (char *)cst_strstr(s, c);
    q = (char *)cst_strdup((cst_string *)s);
    q[cst_strlen(s) - cst_strlen(p)] = '\0';
    return q;
}

/* 0x526f4 */
cst_string *cst_downcase(const cst_string *str)
{
    cst_string *dc;
    int i;

    dc = cst_strdup(str);
    for (i = 0; str[i] != '\0'; i++) {
        if (isupper((int)str[i]))
            dc[i] = tolower((int)str[i]);
    }
    return dc;
}

/* 0x537d0 arm (IVONA): number of non-overlapping occurrences of sub in s. */
int cst_strcount(const char *s, const char *sub)
{
    int n = 0;
    size_t len = strlen(sub);

    while ((s = strstr(s, sub)) != NULL) {
        s += len;
        n++;
    }
    return n;
}

/* 0x53814 arm (IVONA): s with every from replaced by to (newly allocated; NULL for NULL). */
char *cst_strreplace(const char *from, const char *to, const char *s)
{
    size_t slen, flen, tlen, di, si, n;
    const char *p;
    char *r, *d;
    int count;

    if (s == NULL)
        return NULL;
    slen = strlen(s);
    flen = strlen(from);
    tlen = strlen(to);
    count = cst_strcount(s, from);
    r = cst_safe_alloc(slen + 1 + tlen * count - flen * count, 1);
    di = si = 0;
    while ((p = strstr(s + si, from)) != NULL) {
        n = p - (s + si);
        memcpy(r + di, s + si, n);
        memcpy(r + di + n, to, tlen);
        si += n + flen;
        di += n + tlen;
    }
    d = r + di;
    n = slen - si;
    memcpy(d, s + si, n);
    d[n] = '\0';
    return r;
}

/* 0x5390c arm (IVONA): formats at most n bytes into a new string. */
char *cst_vnsprintf(size_t n, const char *fmt, va_list ap)
{
    char *buf = cst_safe_alloc(n + 1, 1);
    char *s;

    vsnprintf(buf, n, fmt, ap);
    s = (char *)cst_strdup((cst_string *)buf);
    cst_free(buf);
    return s;
}

/* 0x53958 arm (IVONA) */
char *cst_vasprintf(const char *fmt, va_list ap)
{
    return cst_vnsprintf(0x400, fmt, ap);
}

/* 0x53968 arm (IVONA) */
char *cst_nsprintf(size_t n, const char *fmt, ...)
{
    va_list ap;
    char *s;

    va_start(ap, fmt);
    s = cst_vnsprintf(n, fmt, ap);
    va_end(ap);
    return s;
}

/* 0x53998 arm (IVONA) */
char *cst_asprintf(const char *fmt, ...)
{
    va_list ap;
    char *s;

    va_start(ap, fmt);
    s = cst_vasprintf(fmt, ap);
    va_end(ap);
    return s;
}
