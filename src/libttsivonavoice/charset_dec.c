/*
 * Character set decoders (0x441c0..0x45054): UTF-8, UTF-16 (either byte order, BOM-detected) and
 * the 8-bit code pages, each read through a 256-entry table to Unicode.
 */
#include <string.h>
#include <strings.h>

typedef int (*uni_decoder)(const char *s, int *len, void *arg);
int uni_decode(const char *s, int *len, uni_decoder dec, void *arg);
int utf8_decode(const char *s, int *len);

/* code page tables (defined with the other conversion data) */
extern const unsigned short charset_iso8859_1[256];
extern const unsigned short charset_iso8859_2[256];
extern const unsigned short charset_iso8859_15[256];
extern const unsigned short charset_cp1250[256];
extern const unsigned short charset_cp1252[256];
extern const unsigned short charset_iso8859_3[256];
extern const unsigned short charset_iso8859_4[256];
extern const unsigned short charset_iso8859_5[256];
extern const unsigned short charset_iso8859_6[256];
extern const unsigned short charset_iso8859_7[256];
extern const unsigned short charset_iso8859_8[256];
extern const unsigned short charset_iso8859_9[256];
extern const unsigned short charset_iso8859_10[256];
extern const unsigned short charset_iso8859_11[256];
extern const unsigned short charset_iso8859_13[256];
extern const unsigned short charset_iso8859_14[256];
extern const unsigned short charset_iso8859_16[256];
extern const unsigned short charset_cp1251[256];
extern const unsigned short charset_cp1253[256];
extern const unsigned short charset_cp1254[256];
extern const unsigned short charset_cp1255[256];
extern const unsigned short charset_cp1256[256];
extern const unsigned short charset_cp1257[256];
extern const unsigned short charset_cp1258[256];

typedef struct charset_decoder charset_decoder;
struct charset_decoder {
    unsigned int (*decode)(const charset_decoder *d, const char *s, int *len);
    union {
        const unsigned short *table;    /* 8-bit code pages */
        int big_endian;                 /* UTF-16 */
    } u;
};

/* 0x441c0 arm */
static unsigned int charset_dec_8bit(const charset_decoder *d, const char *s, int *len)
{
    if (len)
        *len = 1;
    return d->u.table[*(const unsigned char *)s];
}

/* byte offsets of the low and high half of a UTF-16 unit, by big_endian */
static const int utf16_order[2][2] = {
    { 0, 1 },
    { 1, 0 },
};

/* 0x441e0 arm: one UTF-16 code point; unpaired surrogates decode as U+FFFD. */
static int utf16_decode_char(const char *s, int *len, void *arg)
{
    const unsigned char *u = (const unsigned char *)s;
    int lo = utf16_order[*(int *)arg][0];
    int hi = utf16_order[*(int *)arg][1];
    unsigned short c = u[lo] | u[hi] << 8;
    unsigned short c2;

    if ((c & 0xf800) != 0xd800) {
        if (len == NULL)
            return c;
        *len = 2;
        return c;
    }
    if ((c & 0x400) || ((c2 = u[lo + 2] | u[hi + 2] << 8) & 0xfc00) != 0xdc00) {
        if (len)
            *len = 2;
        return 0xfffd;
    }
    if (len)
        *len = 4;
    return (((c & 0x3ff) << 10) | (c2 & 0x3ff)) + 0x10000;
}

/* 0x442a8 arm */
static unsigned int charset_dec_utf8(const charset_decoder *d, const char *s, int *len)
{
    return utf8_decode(s, len);
}

/* 0x442b4 arm: the code point at s, and its length in bytes. */
unsigned int charset_decode(const charset_decoder *d, const char *s, int *len)
{
    return d->decode(d, s, len);
}

/* 0x442c4 arm */
int utf16_decode(const char *s, int big_endian, int *len)
{
    return uni_decode(s, len, utf16_decode_char, &big_endian);
}

/* 0x442f0 arm */
static unsigned int charset_dec_utf16(const charset_decoder *d, const char *s, int *len)
{
    return utf16_decode(s, d->u.big_endian, len);
}

/* 0x44300 arm: the table of an 8-bit character set, or NULL. */
const unsigned short *charset_table(const char *name)
{
    if (strcasecmp(name, "ISO-8859-1") == 0
        || strcasecmp(name, "ISO8859-1") == 0
        || strcasecmp(name, "ISO_8859-1") == 0
        || strcasecmp(name, "LATIN-1") == 0
        || strcasecmp(name, "LATIN1") == 0)
        return charset_iso8859_1;
    if (strcasecmp(name, "ISO-8859-2") == 0
        || strcasecmp(name, "ISO8859-2") == 0
        || strcasecmp(name, "ISO_8859-2") == 0
        || strcasecmp(name, "LATIN-2") == 0
        || strcasecmp(name, "LATIN2") == 0)
        return charset_iso8859_2;
    if (strcasecmp(name, "ISO-8859-15") == 0
        || strcasecmp(name, "ISO8859-15") == 0
        || strcasecmp(name, "ISO_8859-15") == 0
        || strcasecmp(name, "LATIN-9") == 0
        || strcasecmp(name, "LATIN9") == 0)
        return charset_iso8859_15;
    if (strcasecmp(name, "WINDOWS-1250") == 0
        || strcasecmp(name, "WINDOWS1250") == 0
        || strcasecmp(name, "CP1250") == 0)
        return charset_cp1250;
    if (strcasecmp(name, "WINDOWS-1252") == 0
        || strcasecmp(name, "WINDOWS1252") == 0
        || strcasecmp(name, "CP1252") == 0)
        return charset_cp1252;
    if (strcasecmp(name, "ISO-8859-3") == 0
        || strcasecmp(name, "ISO8859-3") == 0
        || strcasecmp(name, "ISO_8859-3") == 0
        || strcasecmp(name, "LATIN-3") == 0
        || strcasecmp(name, "LATIN3") == 0)
        return charset_iso8859_3;
    if (strcasecmp(name, "ISO-8859-4") == 0
        || strcasecmp(name, "ISO8859-4") == 0
        || strcasecmp(name, "ISO_8859-4") == 0
        || strcasecmp(name, "LATIN-4") == 0
        || strcasecmp(name, "LATIN4") == 0)
        return charset_iso8859_4;
    if (strcasecmp(name, "ISO-8859-5") == 0
        || strcasecmp(name, "ISO8859-5") == 0
        || strcasecmp(name, "ISO_8859-5") == 0)
        return charset_iso8859_5;
    if (strcasecmp(name, "ISO-8859-6") == 0
        || strcasecmp(name, "ISO8859-6") == 0
        || strcasecmp(name, "ISO_8859-6") == 0)
        return charset_iso8859_6;
    if (strcasecmp(name, "ISO-8859-7") == 0
        || strcasecmp(name, "ISO8859-7") == 0
        || strcasecmp(name, "ISO_8859-7") == 0)
        return charset_iso8859_7;
    if (strcasecmp(name, "ISO-8859-8") == 0
        || strcasecmp(name, "ISO8859-8") == 0
        || strcasecmp(name, "ISO_8859-8") == 0)
        return charset_iso8859_8;
    if (strcasecmp(name, "ISO-8859-9") == 0
        || strcasecmp(name, "ISO8859-9") == 0
        || strcasecmp(name, "ISO_8859-9") == 0
        || strcasecmp(name, "LATIN-5") == 0
        || strcasecmp(name, "LATIN5") == 0)
        return charset_iso8859_9;
    if (strcasecmp(name, "ISO-8859-10") == 0
        || strcasecmp(name, "ISO8859-10") == 0
        || strcasecmp(name, "ISO_8859-10") == 0
        || strcasecmp(name, "LATIN-6") == 0
        || strcasecmp(name, "LATIN6") == 0)
        return charset_iso8859_10;
    if (strcasecmp(name, "ISO-8859-11") == 0
        || strcasecmp(name, "ISO8859-11") == 0
        || strcasecmp(name, "ISO_8859-11") == 0)
        return charset_iso8859_11;
    if (strcasecmp(name, "ISO-8859-13") == 0
        || strcasecmp(name, "ISO8859-13") == 0
        || strcasecmp(name, "ISO_8859-13") == 0
        || strcasecmp(name, "LATIN-7") == 0
        || strcasecmp(name, "LATIN7") == 0)
        return charset_iso8859_13;
    if (strcasecmp(name, "ISO-8859-14") == 0
        || strcasecmp(name, "ISO8859-14") == 0
        || strcasecmp(name, "ISO_8859-14") == 0
        || strcasecmp(name, "LATIN-8") == 0
        || strcasecmp(name, "LATIN8") == 0)
        return charset_iso8859_14;
    if (strcasecmp(name, "ISO-8859-16") == 0
        || strcasecmp(name, "ISO8859-16") == 0
        || strcasecmp(name, "ISO_8859-16") == 0
        || strcasecmp(name, "LATIN-10") == 0
        || strcasecmp(name, "LATIN10") == 0)
        return charset_iso8859_16;
    if (strcasecmp(name, "WINDOWS-1251") == 0
        || strcasecmp(name, "WINDOWS1251") == 0
        || strcasecmp(name, "CP1251") == 0)
        return charset_cp1251;
    if (strcasecmp(name, "WINDOWS-1253") == 0
        || strcasecmp(name, "WINDOWS1253") == 0
        || strcasecmp(name, "CP1253") == 0)
        return charset_cp1253;
    if (strcasecmp(name, "WINDOWS-1254") == 0
        || strcasecmp(name, "WINDOWS1254") == 0
        || strcasecmp(name, "CP1254") == 0)
        return charset_cp1254;
    if (strcasecmp(name, "WINDOWS-1255") == 0
        || strcasecmp(name, "WINDOWS1255") == 0
        || strcasecmp(name, "CP1255") == 0)
        return charset_cp1255;
    if (strcasecmp(name, "WINDOWS-1256") == 0
        || strcasecmp(name, "WINDOWS1256") == 0
        || strcasecmp(name, "CP1256") == 0)
        return charset_cp1256;
    if (strcasecmp(name, "WINDOWS-1257") == 0
        || strcasecmp(name, "WINDOWS1257") == 0
        || strcasecmp(name, "CP1257") == 0)
        return charset_cp1257;
    if (strcasecmp(name, "WINDOWS-1258") == 0
        || strcasecmp(name, "WINDOWS1258") == 0
        || strcasecmp(name, "CP1258") == 0)
        return charset_cp1258;
    return NULL;
}

/* 0x44ea8 arm: sets d up for the named character set; data (if any) is the start of the text,
 * checked for a byte order mark. 0 if the set is not supported. */
int charset_decoder_init(charset_decoder *d, const char *name, const char *data)
{
    const unsigned short *t;

    if (strcasecmp(name, "UTF-8") == 0 || strcasecmp(name, "UTF8") == 0) {
        d->decode = charset_dec_utf8;
        return 1;
    }
    if (strcasecmp(name, "UTF-16") == 0 || strcasecmp(name, "UTF16") == 0) {
        d->decode = charset_dec_utf16;
        d->u.big_endian = data == NULL || data[0] != (char)0xff || data[1] != (char)0xfe;
        return 1;
    }
    if (strcasecmp(name, "UTF-16LE") == 0 || strcasecmp(name, "UTF16LE") == 0) {
        d->decode = charset_dec_utf16;
        d->u.big_endian = 0;
        return 1;
    }
    if (strcasecmp(name, "UTF-16BE") == 0 || strcasecmp(name, "UTF16BE") == 0) {
        d->decode = charset_dec_utf16;
        d->u.big_endian = 1;
        return 1;
    }
    t = charset_table(name);
    if (t == NULL)
        return 0;
    d->u.table = t;
    d->decode = charset_dec_8bit;
    return 1;
}
