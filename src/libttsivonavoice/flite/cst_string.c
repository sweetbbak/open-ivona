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

/* 0x52768 arm (IVONA): upper-case copy of str. Unlike cst_downcase it reads plain (signed) chars,
 * so bytes >= 0x80 go through ctype's EOF/range checks. */
cst_string *cst_upcase(const cst_string *str)
{
    const char *s = (const char *)str;
    cst_string *uc;
    int i;

    uc = cst_strdup(str);
    for (i = 0; s[i] != '\0'; i++) {
        if (islower((int)s[i]))
            uc[i] = toupper((int)s[i]);
    }
    return uc;
}

/* 0x527fc arm (IVONA): compares the start of s with prefix. */
int cst_strprefixcmp(const char *s, const char *prefix)
{
    return strncmp(s, prefix, strlen(prefix));
}

/* 0x52824 arm (IVONA): compares the end of s with suffix (-1 if s is shorter). */
int cst_strsuffixcmp(const char *s, const char *suffix)
{
    size_t n = strlen(s);
    size_t m = strlen(suffix);

    if (n < m)
        return -1;
    return strcmp(s + n - m, suffix);
}

/* 0x52864 arm (IVONA) */
int cst_startswith(const char *s, const char *prefix)
{
    return cst_strprefixcmp(s, prefix) == 0;
}

/* 0x52878 arm (IVONA) */
int cst_endswith(const char *s, const char *suffix)
{
    return cst_strsuffixcmp(s, suffix) == 0;
}

/* 0x539c8 arm (IVONA): a + b + c, newly allocated. */
char *cst_strcat3(const char *a, const char *b, const char *c)
{
    size_t la = strlen(a);
    size_t lb = strlen(b);
    size_t lc = strlen(c);
    char *r = cst_safe_alloc(la + lb + lc + 1, 1);

    memcpy(r, a, la);
    memcpy(r + la, b, lb);
    memcpy(r + la + lb, c, lc);
    r[la + lb + lc] = '\0';
    return r;
}

/* 0x536ec arm (IVONA): the first n bytes of s as a new string. */
char *cst_strndup(const char *s, size_t n)
{
    char *r = cst_safe_alloc(n + 1, 1);

    memcpy(r, s, n);
    r[n] = '\0';
    return r;
}

/* 0x5258c arm (IVONA): bsearch adapter, compares key with the string an element points to. */
static int cst_bsearch_str_cmp(const void *key, const void *elem, int (*cmp)(const char *, const char *))
{
    return cmp(key, *(const char *const *)elem);
}

int cst_bsearch(const void *key, const void *base, int n, int size,
                int (*cmp)(const void *, const void *, void *), void *arg);

/* 0x528dc arm (IVONA): whether some element of slist starts with str. */
int cst_member_prefix(const char *str, const char *const *slist)
{
    const char *const *p;

    for (p = slist; *p; p++)
        if (cst_strprefixcmp(*p, str) == 0)
            break;
    return *p != NULL;
}

/* 0x52930 arm (IVONA): whether some element of slist ends with str. */
int cst_member_suffix(const char *str, const char *const *slist)
{
    const char *const *p;

    for (p = slist; *p; p++)
        if (cst_strsuffixcmp(*p, str) == 0)
            break;
    return *p != NULL;
}

/* 0x52984 arm (IVONA): removes every c from s in place. */
void cst_strrmchar(char *s, int c)
{
    char *d = s;

    for (; *s; s++) {
        *d = *s;
        if (*d != c)
            d++;
    }
    *d = '\0';
}

/* 0x529c0 arm (IVONA): index of the first element of slist that cmp() finds equal to key, or -1. */
int cst_list_find(const char *const *slist, const char *key, int (*cmp)(const char *, const char *))
{
    const char *const *p;

    for (p = slist; *p; p++)
        if (cmp(*p, key) == 0)
            break;
    if (*p == NULL)
        return -1;
    return p - slist;
}

/* 0x52a20 arm (IVONA) */
int cst_list_index(const char *const *slist, const char *key)
{
    return cst_list_find(slist, key, strcmp);
}

/* 0x52a30 arm (IVONA): where in s a member of slist ends it, or NULL. */
const char *cst_find_suffix(const char *s, const char *const *slist)
{
    const char *const *p;
    const char *t;
    size_t n, m;

    if (*slist == NULL)
        return NULL;
    n = strlen(s);
    for (p = slist; *p; p++) {
        m = strlen(*p);
        t = s + n - m;
        if (m > n)
            continue;
        if (strcmp(t, *p) == 0 && t)
            return t;
    }
    return NULL;
}

/* 0x52aac arm (IVONA): whether s starts with a member of slist. */
int cst_startswith_any(const char *s, const char *const *slist)
{
    const char *const *p;

    for (p = slist; *p; p++)
        if (cst_strprefixcmp(s, *p) == 0)
            break;
    return *p != NULL;
}

/* 0x52afc arm (IVONA): whether s ends with a member of slist. */
int cst_endswith_any(const char *s, const char *const *slist)
{
    const char *const *p;

    for (p = slist; *p; p++)
        if (cst_strsuffixcmp(s, *p) == 0)
            break;
    return *p != NULL;
}

/* 0x52b4c arm (IVONA): whether key is in the sorted array slist[0..n). */
int cst_sorted_member(const char *key, const char *const *slist, int n)
{
    return cst_bsearch(key, slist, n, sizeof(char *), (int (*)(const void *, const void *, void *))cst_bsearch_str_cmp,
                       strcmp) != -1;
}

/* 0x52b8c arm (IVONA): the longest prefix of s found in the sorted array slist[0..n), or NULL. */
const char *cst_sorted_prefix(const char *s, const char *const *slist, int n)
{
    char *t = (char *)cst_strdup((const cst_string *)s);
    size_t len = strlen(t);
    int i;

    if (len == 0) {
        cst_free(t);
        return NULL;
    }
    do {
        i = cst_bsearch(t, slist, n, sizeof(char *), (int (*)(const void *, const void *, void *))cst_bsearch_str_cmp,
                        strcmp);
        t[--len] = '\0';
    } while (i == -1 && len != 0);
    cst_free(t);
    return i != -1 ? slist[i] : NULL;
}

/* 0x52c48 arm (IVONA) */
int cst_has_sorted_prefix(const char *s, const char *const *slist, int n)
{
    return cst_sorted_prefix(s, slist, n) != NULL;
}

int cst_roman_value(const char *s);

/* 0x53114 arm (IVONA): the value of Roman numeral s in decimal (newly allocated). */
char *cst_roman_to_decimal(const char *s)
{
    int v = cst_roman_value(s);
    int n = snprintf(NULL, 0, "%d", v);
    char *r = NULL;

    if (n > 0) {
        r = cst_safe_alloc(n + 1, 1);
        snprintf(r, n + 1, "%d", v);
    }
    return r;
}

/* 0x5317c arm (IVONA): replaces every c in s with r. */
void cst_strsubchar(char *s, int c, char r)
{
    for (; (s = strchr(s, c)) != NULL; s++)
        *s = r;
}
