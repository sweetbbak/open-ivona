/* Flite cst_string.c as modified by IVONA. */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "cst_val.h"
#include "ivona_util.h"

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

/* 0x5342c arm (IVONA): value for key in a NULL-terminated array of key/value pairs. */
const char *cst_assoc_string(const char *const *pairs, const char *key)
{
    const char *const *p;

    for (p = pairs; *p; p += 2)
        if (strcmp(p[0], key) == 0)
            return p[1];
    return NULL;
}

/* 0x53470 arm (IVONA): like tr(1): characters of from become the corresponding character of to,
 * or are deleted when to is shorter (or NULL). */
char *cst_strtr(const char *s, const char *from, const char *to)
{
    char *r = cst_safe_alloc(strlen(s) + 1, 1);
    size_t n = to ? strlen(to) : 0;
    const char *p;
    size_t i;
    int k = 0;

    for (; *s; s++) {
        p = strchr(from, (unsigned char)*s);
        if (p == NULL)
            r[k++] = *s;
        else if ((i = p - from) < n)
            r[k++] = to[i];
    }
    r[k] = '\0';
    return r;
}

cst_val *string_val_consume(char *s);

/* 0x53520 arm (IVONA): the pieces of s between characters of delims, as a list of strings. */
cst_val *cst_split_val(const char *s, const char *delims)
{
    cst_val *l = NULL;
    size_t i, n;

    if (s == NULL)
        return val_reverse(NULL);
    for (i = 0; s[i]; ) {
        n = strcspn(s + i, delims);
        if (n == 0) {
            i += strspn(s + i, delims);
            continue;
        }
        l = cons_val(string_val_consume(cst_substr(s, i, n)), l);
        i += n;
    }
    return val_reverse(l);
}

/* 0x535c8 arm (IVONA): a + b, newly allocated. */
char *cst_strcat2(const char *a, const char *b)
{
    size_t la = strlen(a);
    size_t lb = strlen(b);
    char *r = cst_safe_alloc(la + lb + 1, 1);

    strncpy(r, a, la);
    strncpy(r + la, b, lb + 1);
    return r;
}

/* 0x53620 arm (IVONA): a[0..na) + b[0..nb), newly allocated. */
char *cst_strncat2(const char *a, size_t na, const char *b, size_t nb)
{
    strbuf buf;

    strbuf_init(&buf);
    strbuf_reserve(&buf, na + nb);
    strbuf_append(&buf, a, na);
    strbuf_append(&buf, b, nb);
    return strbuf_data(&buf);
}

/* 0x53680 arm (IVONA): appends b to the allocated string a (which may be NULL). */
char *cst_strappend(char *a, const char *b)
{
    size_t la = a ? strlen(a) : 0;
    size_t lb;

    if (b == NULL || (lb = strlen(b)) == 0)
        return a;
    a = cst_safe_realloc(a, la + 1 + lb, 1);
    strncpy(a + la, b, lb + 1);
    return a;
}

/* 0x5371c arm (IVONA): length of the prefix of s[0..n) made of characters in accept. */
size_t cst_strnspn(const char *s, size_t n, const char *accept)
{
    size_t i;

    for (i = 0; i < n; i++)
        if (strchr(accept, (unsigned char)s[i]) == NULL)
            break;
    return i;
}

/* 0x53768 arm (IVONA): length of the prefix of s[0..n) without characters in reject. */
size_t cst_strncspn(const char *s, size_t n, const char *reject)
{
    size_t i;

    for (i = 0; i < n && s[i]; i++)
        if (strchr(reject, (unsigned char)s[i]))
            break;
    return i;
}

#define cst_whitespace " \t\n\v\f\r\x85\xa0"

/* 0x53a58 arm (IVONA) */
const char *cst_skip_space(const char *s)
{
    while (*s && strchr(cst_whitespace, *s))
        s++;
    return s;
}

/* 0x53ac4 arm (IVONA): end of s[0..n) without trailing whitespace. */
const char *cst_rtrim_n(const char *s, size_t n)
{
    while (n && strchr(cst_whitespace, s[n - 1]))
        n--;
    return s + n;
}

/* 0x53b20 arm (IVONA): s without trailing whitespace, or NULL if there was none. */
char *cst_rtrim_dup(const char *s)
{
    size_t n = strlen(s);
    const char *e = cst_rtrim_n(s, n);

    if (e < s + n)
        return cst_strndup(s, e - s);
    return NULL;
}

/* 0x53b64 arm (IVONA): copy of s with A-Z lowered. */
char *cst_ascii_downcase(const char *s)
{
    char *r = (char *)cst_strdup((const cst_string *)s);
    char *p;

    for (p = r; *p; p++)
        if ((unsigned char)*p >= 'A' && (unsigned char)*p <= 'Z')
            *p += 'a' - 'A';
    return r;
}

/* 0x53ba0 arm (IVONA) */
char cst_ascii_tolower(char c)
{
    unsigned char u = c;

    if (u >= 'A' && u <= 'Z')
        u += 'a' - 'A';
    return u;
}

/* 0x53bc0 arm (IVONA) */
int cst_startswith_nocase(const char *s, const char *prefix)
{
    return strncasecmp(s, prefix, strlen(prefix)) == 0;
}

/* 0x53bf0 arm (IVONA) */
int cst_endswith_nocase(const char *s, const char *suffix)
{
    size_t n = strlen(s);
    size_t m = strlen(suffix);

    if (n < m)
        return 0;
    return strncasecmp(s + n - m, suffix, m) == 0;
}

/* 0x53c3c arm: a decimal number ([+-]digits[.digits]); 0 if s does not start with one */
int cst_parse_float(const char *s, const char **end, float *out)
{
    const char *p = s;
    float v, scale;
    int neg = 0, n, m;

    if (*p == '-') {
        neg = 1;
        p++;
    } else if (*p == '+')
        p++;
    if ((unsigned int)(*p - '0') <= 9) {
        v = 0;
        n = 0;
        do {
            v = v * 10.0f + (*p - '0');
            n++;
            p++;
        } while ((unsigned int)(*p - '0') <= 9);
        if (*p != '.')
            goto done;
    } else if (*p == '.') {
        v = 0;
        n = 0;
    } else
        return 0;
    p++;
    m = 0;
    if ((unsigned int)(*p - '0') <= 9) {
        scale = 1.0f;
        do {
            scale *= 0.1f;
            m++;
            v += (*p - '0') * scale;
            p++;
        } while ((unsigned int)(*p - '0') <= 9);
    }
    if ((m | n) == 0)
        return 0;
done:
    if (neg)
        v = -v;
    if (out)
        *out = v;
    if (end)
        *end = p;
    return 1;
}

/* 0x53dd4 arm: s is a number followed by exactly suffix */
int cst_parse_float_suffix(const char *s, const char *suffix, float *out)
{
    const char *end = NULL;

    if (!cst_parse_float(s, &end, out))
        return 0;
    if (end == s)
        return 0;
    return strcmp(end, suffix) == 0;
}

/* 0x53e30 arm */
int cst_parse_unsigned_suffix(const char *s, const char *suffix, float *out)
{
    if (strchr("+-", *s))
        return 0;
    return cst_parse_float_suffix(s, suffix, out);
}

/* 0x53e78 arm */
int cst_parse_signed_suffix(const char *s, const char *suffix, float *out)
{
    if (!strchr("+-", *s))
        return 0;
    return cst_parse_float_suffix(s, suffix, out);
}

/* 0x53eb8 arm */
int cst_parse_number(const char *s, float *out)
{
    return cst_parse_float_suffix(s, "", out);
}

/* 0x53ecc arm */
int cst_parse_signed_number(const char *s, float *out)
{
    if (!strchr("+-", *s))
        return 0;
    return cst_parse_number(s, out);
}
