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

int utf8_decode(const char *s, int *len);
const char *uni_translit(int c, int a, int b);

/* code page tables (charset_tables.c) */
extern const unsigned short charset_iso8859_2[256];
extern const unsigned short charset_iso8859_5[256];
extern const unsigned short charset_iso8859_9[256];
extern const unsigned short charset_iso8859_14[256];
extern const unsigned short charset_iso8859_15[256];
extern const unsigned short charset_iso8859_16[256];

/* whether charset is one of the names, spelled "ISO-8859-n", "ISO8859-n" or "ISO_8859-n" */
static int charset_is_iso8859(const char *charset, const char *n)
{
    static const char *const forms[] = { "ISO-8859-", "ISO8859-", "ISO_8859-" };
    size_t i, len;

    for (i = 0; i < sizeof forms / sizeof forms[0]; i++) {
        len = strlen(forms[i]);
        if (strncasecmp(charset, forms[i], len) == 0 && strcmp(charset + len, n) == 0)
            return 1;
    }
    return 0;
}

/* the byte of c in an ISO 8859 code page (bytes below 0xa0 are the same code points), 0 if it
 * has none */
static unsigned char iso8859_encode(const unsigned short *table, unsigned int c)
{
    int i;

    if (c < 0xa0)
        return c;
    for (i = 0xa0; i < 256; i++)
        if (table[i] == c)
            return i;
    return 0;
}

/* 0x488bc arm: u (UTF-8) in charset: UTF-8 (a leading BOM dropped), ASCII or an ISO 8859 code
 * page; characters it lacks are transliterated (uni_translit). NULL if charset is not one of
 * these. The binary has a hand-written encoder per code page; the 8859 ones other than Latin-1
 * are done here by reverse lookup in the decoders' tables, plus the one extra mapping the binary
 * has (8859-2: Romanian comma-below letters as their cedilla forms). */
ivona_ustr *ustr_to_charset(const ivona_ustr *u, const char *charset)
{
    const unsigned short *table = NULL;
    int utf8 = 0, ascii = 0, latin1 = 0, latin2 = 0, len;
    ustr_edit e;
    ivona_ustr *r;
    unsigned int c;
    unsigned char b;
    const char *t;
    char buf[8];

    if (strcasecmp(charset, "UTF-8") == 0 || strcasecmp(charset, "UTF8") == 0)
        utf8 = 1;
    else if (charset_is_iso8859(charset, "1"))
        latin1 = 1;
    else if (charset_is_iso8859(charset, "2"))
        table = charset_iso8859_2, latin2 = 1;
    else if (charset_is_iso8859(charset, "5"))
        table = charset_iso8859_5;
    else if (charset_is_iso8859(charset, "9"))
        table = charset_iso8859_9;
    else if (charset_is_iso8859(charset, "14"))
        table = charset_iso8859_14;
    else if (charset_is_iso8859(charset, "15"))
        table = charset_iso8859_15;
    else if (charset_is_iso8859(charset, "16"))
        table = charset_iso8859_16;
    else if (strcasecmp(charset, "ASCII") == 0)
        ascii = 1;
    else
        return NULL;
    ustr_edit_init(&e, u);
    while (e.pos < u->len) {
        c = utf8_decode(u->data + e.pos, &len);
        ustr_edit_skip(&e, len);
        if (utf8) {
            if (e.pos != 0 || c != 0xfeff)
                ustr_edit_insert_n(&e, buf, utf8_encode(c, buf));
            continue;
        }
        if (latin1 ? c < 0x100 : ascii ? c < 0x80 : (b = iso8859_encode(table, c)) != 0 || c == 0) {
            buf[0] = latin1 || ascii ? (char)c : (char)b;
            ustr_edit_insert_n(&e, buf, 1);
        } else if (latin2 && c - 0x218 < 4) {
            buf[0] = "\xaa\xba\xde\xfe"[c - 0x218];
            ustr_edit_insert_n(&e, buf, 1);
        } else {
            t = uni_translit(c, 1, 1);
            ustr_edit_insert_n(&e, t, strlen(t));
        }
    }
    r = ustr_edit_result(&e);
    if (r == NULL)
        r = ustr_copy(u);
    return r;
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
