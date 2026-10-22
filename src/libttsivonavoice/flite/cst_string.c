/* Flite cst_string.c as modified by IVONA. */
#include <ctype.h>
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
