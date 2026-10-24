/*
 * Converting text between character sets and UTF-8 ustrs (0x48610..0x4962c).
 */
#include <string.h>
#include <strings.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "ivona_ustr.h"

int utf8_encode(unsigned int c, char *out);

/* charset_dec.c */
typedef struct charset_decoder {
    unsigned int (*decode)(const struct charset_decoder *d, const char *s, int *len);
    const void *arg;
} charset_decoder;
int charset_decoder_init(charset_decoder *d, const char *name, const char *data);
unsigned int charset_decode(const charset_decoder *d, const char *s, int *len);

ivona_ustr *ustr_to_charset(const ivona_ustr *u, const char *charset);

/* 0x48610 arm: length in bytes of a NUL-terminated UTF-16 string */
size_t u16_strlen(const char *s)
{
    const unsigned short *p = (const unsigned short *)s;

    while (*p)
        p++;
    return (const char *)p - s;
}

/* 0x48644 arm: the strlen for text in the named character set */
size_t (*charset_strlen_func(const char *charset))()
{
    if (charset
        && (strcasecmp(charset, "UTF-16LE") == 0 || strcasecmp(charset, "UTF16LE") == 0
            || strcasecmp(charset, "UTF-16BE") == 0 || strcasecmp(charset, "UTF16BE") == 0
            || strcasecmp(charset, "UTF-16") == 0 || strcasecmp(charset, "UTF16") == 0))
        return (size_t (*)())u16_strlen;
    return (size_t (*)())strlen;
}

/* 0x48720 arm: u (text in charset, one byte per position) decoded to UTF-8, appended to out (or
 * a new ustr); a leading byte order mark is dropped. NULL if the charset is unknown. */
ivona_ustr *ustr_decode_into(const ivona_ustr *u, ivona_ustr *out, const char *charset)
{
    char buf[8];
    charset_decoder d;
    ustr_edit e;
    ivona_ustr *r;
    unsigned int c;
    int n;

    if (!charset_decoder_init(&d, charset, u->data))
        return NULL;
    ustr_edit_init_out(&e, u, out);
    while (e.pos < u->len) {
        c = charset_decode(&d, u->data + e.pos, &n);
        ustr_edit_skip(&e, n);
        if (e.pos != 0 || c != 0xfeff)
            ustr_edit_insert_n(&e, buf, utf8_encode(c, buf));
    }
    r = ustr_edit_result(&e);
    if (r == NULL)
        r = ustr_copy(u);
    return r;
}

/* 0x4880c arm */
ivona_ustr *ustr_decode(const ivona_ustr *u, const char *charset)
{
    return ustr_decode_into(u, NULL, charset);
}

/* 0x48818 arm */
ivona_ustr *ustr_decode_n(const char *s, size_t n, const char *charset)
{
    ivona_ustr *u = ustr_new(0, s, n);
    ivona_ustr *r = ustr_decode(u, charset);

    ivona_ustr_free(u);
    return r;
}

/* 0x48854 arm: s is NUL-terminated in its character set. */
ivona_ustr *ivona_ustr_decode(const char *s, const char *charset)
{
    return ustr_decode_n(s, charset_strlen_func(charset)(s), charset);
}

static char charset_probe[4];

/* 0x48888 arm */
int charset_supported(const char *charset)
{
    ivona_ustr *u = ivona_ustr_decode(charset_probe, charset);

    if (u == NULL)
        return 0;
    ivona_ustr_free(u);
    return 1;
}

/* 0x49544 arm: UTF-16 resolved to the byte order its BOM says. */
const char *charset_detect(const char *charset, const char *data)
{
    if (strcasecmp(charset, "UTF-16") == 0 || strcasecmp(charset, "UTF16") == 0) {
        if (data[0] != (char)0xff)
            return "UTF-16BE";
        if (data[1] != (char)0xfe)
            return "UTF-16BE";
        return "UTF-16LE";
    }
    return charset;
}

/* 0x495d4 arm: s transliterated to ASCII. */
char *str_to_ascii(const char *s)
{
    ivona_ustr *u = ustr_new(0, s, strlen(s));
    ivona_ustr *r = ustr_to_charset(u, "ASCII");
    char *out = cst_strndup(r->data, r->len);

    ivona_ustr_free(r);
    ivona_ustr_free(u);
    return out;
}
