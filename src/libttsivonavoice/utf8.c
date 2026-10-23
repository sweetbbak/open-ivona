/*
 * UTF-8 string helpers (0x59584..0x5a44c): a validating decoder, character-wise
 * strchr/strspn/strcspn, substrings, splitting, offsets and case mapping.
 */
#include <string.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "cst_val.h"
#include "ivona_util.h"

typedef int (*uni_decoder)(const char *s, int *len, void *arg);
typedef int (*uni_map_fn)(int c, const char **str);

int uni_decode(const char *s, int *len, uni_decoder dec, void *arg);
const char *uni_translit(int c, int a, int b);
int uni_tolower(int c);
int utf8_encode(unsigned int c, char *out);
cst_val *string_val_consume(char *s);

/* goto bad unless b is a continuation byte */
#define UTF8_CONT(b) \
    if (!((b) & 0x80)) \
        goto bad; \
    if ((b) > 0xbf) \
        goto bad

/* 0x59584 arm: decodes one UTF-8 sequence; malformed input gives U+FFFD and length 1. */
static int utf8_decode_char(const char *s, int *len, void *arg)
{
    unsigned char c = s[0], b1, b2, b3;
    unsigned int r;

    if (!(c & 0x80)) {
        if (len)
            *len = 1;
        return c;
    }
    if (c < 0xc2)
        goto bad;
    if (c < 0xe0) {
        b1 = s[1];
        UTF8_CONT(b1);
        if (len)
            *len = 2;
        return ((c & 0x1f) << 6) | (b1 & 0x3f);
    }
    if (c < 0xf0) {
        b1 = s[1];
        UTF8_CONT(b1);
        b2 = s[2];
        UTF8_CONT(b2);
        r = ((c & 0xf) << 12) | ((b1 & 0x3f) << 6) | (b2 & 0x3f);
        if (len)
            *len = 3;
        if ((r & 0xf800) == 0xd800 || r < 0x800)
            return 0xfffd;
        return r;
    }
    if (c > 0xf4)
        goto bad;
    b1 = s[1];
    UTF8_CONT(b1);
    b2 = s[2];
    UTF8_CONT(b2);
    b3 = s[3];
    UTF8_CONT(b3);
    r = ((c & 7) << 18) | ((b1 & 0x3f) << 12) | ((b2 & 0x3f) << 6) | (b3 & 0x3f);
    if (len)
        *len = 4;
    if (r < 0x10000 || r >= 0x110000)
        return 0xfffd;
    return r;
bad:
    if (len)
        *len = 1;
    return 0xfffd;
}

/* 0x596f0 arm: code point at s; *len (optional) gets its byte length. */
int utf8_decode(const char *s, int *len)
{
    return uni_decode(s, len, utf8_decode_char, NULL);
}

/* 0x59704 arm: first character of s equal to c, or NULL. */
const char *utf8_strchr(const char *s, int c)
{
    int len;

    while (*s) {
        if (utf8_decode(s, &len) == c)
            break;
        s += len;
    }
    return *s ? s : NULL;
}

/* 0x59770 arm: last character of s equal to c, or NULL. */
const char *utf8_strrchr(const char *s, int c)
{
    const char *last = NULL;
    int len;

    while (*s) {
        if (utf8_decode(s, &len) == c)
            last = s;
        s += len;
    }
    return last;
}

/* 0x597c4 arm: utf8_strchr with the first character of ch. */
const char *utf8_strchr_str(const char *s, const char *ch)
{
    return utf8_strchr(s, utf8_decode(ch, NULL));
}

/* 0x597e8 arm: utf8_strchr if ch is exactly one character, else NULL. */
const char *utf8_strchr_single(const char *s, const char *ch)
{
    int len;
    int c = utf8_decode(ch, &len);

    if (c == 0)
        return NULL;
    if (ch[len] != 0)
        return NULL;
    return utf8_strchr(s, c);
}

/* 0x5983c arm: utf8_strrchr with the first character of ch. */
const char *utf8_strrchr_str(const char *s, const char *ch)
{
    return utf8_strrchr(s, utf8_decode(ch, NULL));
}

/* 0x59860 arm: length of the prefix of s made of characters in set (any if NULL),
 * in characters if chars is set, else in bytes. */
size_t utf8_strspn(const char *s, const char *set, int chars)
{
    size_t n = 0, bytes = 0;
    int len, c;

    while (*s) {
        c = utf8_decode(s, &len);
        if (set && !utf8_strchr(set, c))
            break;
        n++;
        bytes += len;
        s += len;
    }
    if (!chars)
        return bytes;
    return n;
}

/* 0x59920 arm: length of the prefix of s made of characters not in set. */
size_t utf8_strcspn(const char *s, const char *set, int chars)
{
    size_t n = 0, bytes = 0;
    int len, c;

    while (*s) {
        c = utf8_decode(s, &len);
        if (set && utf8_strchr(set, c))
            break;
        n++;
        bytes += len;
        s += len;
    }
    if (!chars)
        return bytes;
    return n;
}

/* 0x599e0 arm: new copy of take characters of s after skipping skip characters. */
char *utf8_substr(const char *s, size_t skip, size_t take)
{
    const char *p;
    size_t n = 0;
    char *r;
    int len;

    if (s == NULL)
        return NULL;
    while (*s && skip) {
        utf8_decode(s, &len);
        skip--;
        s += len;
    }
    if (skip)
        return NULL;
    p = s;
    while (*p && take) {
        utf8_decode(p, &len);
        take--;
        p += len;
        n += len;
    }
    r = cst_safe_alloc(n + 1, 1);
    strncpy(r, s, n);
    r[n] = 0;
    return r;
}

/* 0x59ae0 arm: start of the last character of s. */
const char *utf8_last_char(const char *s)
{
    const char *last = s;
    int len;

    while (*s) {
        utf8_decode(s, &len);
        last = s;
        s += len;
    }
    return last;
}

/* 0x59b30 arm: splits s at characters in set into a list of strings; mode 1 keeps
 * empty fields. */
cst_val *utf8_split(const char *s, const char *set, int mode)
{
    cst_val *r = NULL;
    size_t n, i;

    if (s == NULL)
        return NULL;
    while (*s) {
        n = utf8_strcspn(s, set, 0);
        if (n == 0) {
            if (mode == 1) {
                n = utf8_strspn(s, set, mode);
                if (r == NULL)
                    r = cons_val(string_val(""), r);
                for (i = 1; i < n; i++)
                    r = cons_val(string_val(""), r);
                s += utf8_strspn(s, set, 0);
                if (*s == 0)
                    r = cons_val(string_val(""), r);
            } else
                s += utf8_strspn(s, set, 0);
        } else {
            r = cons_val(string_val_consume(cst_substr(s, 0, n)), r);
            s += n;
        }
    }
    if (r == NULL && mode == 1)
        return cons_val(string_val(""), NULL);
    return val_reverse(r);
}

/* 0x59cc0 arm: number of bytes needed to encode c. */
int utf8_charlen(unsigned int c)
{
    if (c < 0x80)
        return 1;
    if (c < 0x800)
        return 2;
    if (c < 0x10000)
        return 3;
    if (c < 0x200000)
        return 4;
    if (c < 0x4000000)
        return 5;
    return 6;
}

/* 0x59d10 arm: byte length of the first character of s (0 at the end). */
int utf8_char_bytes(const char *s)
{
    int len;

    if (s == NULL || *s == 0)
        return 0;
    utf8_decode(s, &len);
    return len;
}

/* 0x59d4c arm: number of characters in s. */
size_t utf8_strlen(const char *s)
{
    size_t n = 0;

    while (*s) {
        s += utf8_char_bytes(s);
        n++;
    }
    return n;
}

/* 0x59d84 arm: whether s has more than n characters. */
int utf8_strlen_gt(const char *s, size_t n)
{
    size_t i;

    if (s == NULL)
        return 0;
    for (i = 0; i <= n; i++) {
        if (*s == 0)
            return 0;
        s += utf8_char_bytes(s);
    }
    return 1;
}

/* 0x59dcc arm: whether s has exactly n characters. */
int utf8_strlen_eq(const char *s, size_t n)
{
    size_t i;

    if (s == NULL)
        return 0;
    for (i = 0; i < n; i++) {
        if (*s == 0)
            return 0;
        s += utf8_char_bytes(s);
    }
    return *s == 0;
}

/* 0x59e34 arm: encodes c (nothing for 0) as a NUL-terminated string. */
void utf8_encode_z(int c, char *out)
{
    int n = 0;

    if (c)
        n = utf8_encode(c, out);
    out[n] = 0;
}

/* 0x59e58 arm: copies the first character of s to out; returns its length. */
int utf8_copy_char(const char *s, char *out)
{
    int n = utf8_char_bytes(s);

    if (n)
        memcpy(out, s, n);
    return n;
}

/* 0x59e88 arm: utf8_copy_char plus a terminating NUL. */
int utf8_copy_char_z(const char *s, char *out)
{
    int n = utf8_copy_char(s, out);

    out[n] = 0;
    return n;
}

/* 0x59ea0 arm: new ASCII transliteration of s. */
char *utf8_to_ascii(const char *s)
{
    strbuf sb;
    int len;
    unsigned int c;

    strbuf_init(&sb);
    strbuf_reserve(&sb, strlen(s));
    while ((c = utf8_decode(s, &len)) != 0) {
        if (c < 0x80) {
            char ch = c;
            strbuf_append(&sb, &ch, 1);
        } else
            strbuf_append_str(&sb, uni_translit(c, 0, 1));
        s += len;
    }
    if (sb.data == NULL)
        return cst_strdup("");
    return strbuf_data(&sb);
}

/* 0x59f58 arm: first character of s that is in set, or NULL. */
const char *utf8_strpbrk(const char *s, const char *set)
{
    const char *p = s;
    size_t n = 0;
    int len, c;

    while (*p) {
        c = utf8_decode(p, &len);
        if (set && utf8_strchr(set, c))
            break;
        n += len;
        p += len;
    }
    if (s[n])
        return s + n;
    return NULL;
}

/* 0x5a014 arm: pointer to character n of s (counted from the end if negative), or NULL
 * past the end. */
const char *utf8_offset(const char *s, int n)
{
    size_t l, m, cnt, i;
    int len;

    if (s == NULL)
        return NULL;
    if (n < 0) {
        m = -n;
        l = utf8_strlen(s);
        if (m > l)
            return NULL;
        cnt = l - m;
    } else
        cnt = n;
    for (i = 0; i < cnt; i++) {
        if (!utf8_decode(s, &len))
            return NULL;
        s += len;
    }
    return s;
}

/* 0x5a09c arm: byte offset of character n of s, or -1. */
int utf8_byte_offset(const char *s, int n)
{
    const char *p;

    if (s == NULL && n == 0)
        return 0;
    p = utf8_offset(s, n);
    return p ? p - s : -1;
}

/* 0x5a0d4 arm: new lower-case copy of the first len bytes of s. */
char *utf8_lower_n(const char *s, size_t len)
{
    strbuf sb;
    char buf[8];
    size_t i;
    int l, c, n;
    char *r;

    strbuf_init(&sb);
    strbuf_reserve(&sb, len);
    i = 0;
    l = 0;
    while ((c = utf8_decode(s + i, &l)) != 0) {
        n = utf8_encode(uni_tolower(c), buf);
        strbuf_append(&sb, buf, n);
        i += l;
        if (i >= len)
            break;
    }
    r = strbuf_data(&sb);
    if (r == NULL)
        return cst_strdup("");
    return r;
}

/* 0x5a180 arm: new lower-case copy of s. */
char *utf8_lower(const char *s)
{
    return utf8_lower_n(s, strlen(s));
}

/* 0x5a19c arm: end of s[0..len) without trailing (Unicode) white space. */
const char *utf8_rtrim(const char *s, size_t len)
{
    char buf[8];
    size_t n = 0, k;

    while (len > 0) {
        k = n + 1;
        if (((unsigned char)s[len - 1] & 0xc0) == 0x80) {
            if (k > 3)
                break;
            n = k;
        } else {
            memcpy(buf, s + len - 1, k);
            buf[k] = 0;
            if (!strstr(" \t\n\v\f\r\xc2\x85\xe2\x80\xa8\xe2\x80\xa9 \xc2\xa0\xe1\x9a\x80"
                        "\xe1\xa0\x8e\xe2\x80\x80\xe2\x80\x81\xe2\x80\x82\xe2\x80\x83"
                        "\xe2\x80\x84\xe2\x80\x85\xe2\x80\x86\xe2\x80\x87\xe2\x80\x88"
                        "\xe2\x80\x89\xe2\x80\x8a\xe2\x80\xaf\xe2\x81\x9f\xe3\x80\x80", buf))
                break;
            n = 0;
        }
        len--;
    }
    return s + len + n;
}

/* appends c, or str when c is 0 */
static inline void utf8_append_mapped(strbuf *sb, int c, const char *str, char *buf)
{
    if (c) {
        utf8_encode_z(c, buf);
        str = buf;
    }
    strbuf_append_str(sb, str);
}

/* 0x5a290 arm: new copy of s with each character mapped by f1 (lower case if NULL or
 * unmapped), then by f2. A map returns a code point or sets *str to a replacement. */
char *utf8_map(const char *s, uni_map_fn f1, uni_map_fn f2)
{
    strbuf sb;
    char buf[8];
    int len;
    const char *str;
    const char *s2;
    int l;
    const char *p;
    char *r;
    int c, m;

    strbuf_init(&sb);
    strbuf_reserve(&sb, strlen(s));
    while ((c = utf8_decode(s, &len)) != 0) {
        str = NULL;
        m = 0;
        if (f1)
            m = f1(c, &str);
        if (str == NULL && m == 0) {
            m = uni_tolower(c);
            if (m == 0)
                m = c;
        }
        if (f2 == NULL)
            utf8_append_mapped(&sb, m, str, buf);
        else if (str) {
            for (p = str; s2 = NULL, (c = utf8_decode(p, &l)) != 0; p += l)
                utf8_append_mapped(&sb, f2(c, &s2), s2, buf);
        } else {
            s2 = NULL;
            utf8_append_mapped(&sb, f2(m, &s2), s2, buf);
        }
        s += len;
    }
    r = strbuf_data(&sb);
    if (r == NULL)
        return cst_strdup("");
    return r;
}
