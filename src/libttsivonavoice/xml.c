/*
 * XML helpers for SSML (0x49970..0x4a464): inline WAVE data, escaping, character classes.
 */
#include <string.h>
#include <strings.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "cst_wave.h"
#include "ivona_ustr.h"

int utf8_decode(const char *s, int *len);
int riff_chunk_skippable(const char *info);

/* 0x49970 arm: reads a mono PCM RIFF/WAVE image into w. 0 on success, 1 if it is not RIFF,
 * 2 if it is malformed. A truncated data chunk keeps what is there. */
int wave_parse_riff(cst_wave *w, const char *buf, int len)
{
    int off, size;

    if (len < 4 || strncmp(buf, "RIFF", 4) != 0)
        return 1;
    if (len < 12 || strncmp(buf + 8, "WAVE", 4) != 0)
        return 2;
    if (len < 16 || strncmp(buf + 12, "fmt ", 4) != 0)
        return 2;
    if (len < 20)
        return 2;
    off = *(const int *)(buf + 16);
    if (len < 22 || *(const short *)(buf + 20) != 1)
        return 2;
    if (len < 24 || *(const short *)(buf + 22) != 1)
        return 2;
    w->num_channels = *(const short *)(buf + 22);
    if (len < 36)
        return 2;
    w->sample_rate = *(const int *)(buf + 24);
    off += 20;
    for (;;) {
        if (len - off < 4)
            return 2;
        if (strncmp(buf + off, "data", 4) == 0)
            break;
        if (!riff_chunk_skippable(buf + off))
            return 2;
        off += 4;
        if (len - off < 4)
            return 2;
        off += *(const int *)(buf + off) + 4;
    }
    off += 4;
    if (len - off < 4)
        return 2;
    size = *(const int *)(buf + off);
    off += 4;
    len -= off;
    cst_wave_resize(w, size / sizeof(short), w->num_channels);
    if (size <= len)
        memcpy(w->samples, buf + off, size);
    else {
        memcpy(w->samples, buf + off, len);
        w->num_samples = len / sizeof(short);
    }
    return 0;
}

typedef struct xml_entity {
    const char *name;
    const char *value;
} xml_entity;

/* indexed by position in xml_special_ws / xml_special */
static const xml_entity xml_entities[] = {
    { "lt", "<" },
    { "gt", ">" },
    { "amp", "&" },
    { "apos", "'" },
    { "quot", "\"" },
    { "#x9", "\t" },
    { "#xA", "\n" },
    { "#xD", "\r" },
    { "#x20", " " },
    { NULL, NULL },
};

static const char xml_special_ws[] = "<>&'\"\t\n\r ";
static const char xml_special[] = "<>&'\"";

typedef struct xml_range {
    unsigned int lo, hi;
} xml_range;

/* NameStartChar of XML 1.0 (5th edition) */
static const xml_range xml_name_start[] = {
    { ':', ':' }, { 'A', 'Z' }, { '_', '_' }, { 'a', 'z' },
    { 0xc0, 0xd6 }, { 0xd8, 0xf6 }, { 0xf8, 0x2ff }, { 0x370, 0x37d },
    { 0x37f, 0x1fff }, { 0x200c, 0x200d }, { 0x2070, 0x218f }, { 0x2c00, 0x2fef },
    { 0x3001, 0xd7ff }, { 0xf900, 0xfdcf }, { 0xfdf0, 0xfffd }, { 0x10000, 0xeffff },
    { 0, 0 },
};

/* NameChar additions */
static const xml_range xml_name_extra[] = {
    { '-', '-' }, { '.', '.' }, { '0', '9' }, { 0xb7, 0xb7 },
    { 0x300, 0x36f }, { 0x203f, 0x2040 },
    { 0, 0 },
};

/* 0x49b24 arm: a copy of n bytes of s with markup characters (and with ws, whitespace too)
 * written as entity references. */
char *xml_escape_n(const char *s, size_t n, int ws)
{
    ivona_ustr *u = ustr_new(0, s, n);
    ivona_ustr *r;
    const char *special = ws ? xml_special_ws : xml_special;
    const char *p;
    ustr_edit e;
    char *out;

    ustr_edit_init(&e, u);
    while (e.pos < u->len) {
        p = strchr(special, u->data[e.pos]);
        if (p) {
            ustr_edit_skip(&e, 1);
            ustr_edit_insert(&e, "&");
            ustr_edit_insert(&e, xml_entities[p - special].name);
            ustr_edit_insert(&e, ";");
        } else
            ustr_edit_keep(&e, 1);
    }
    r = ustr_edit_result(&e);
    if (r)
        out = cst_strndup(r->data, r->len);
    else
        out = cst_strndup(u->data, u->len);
    ivona_ustr_free(u);
    ivona_ustr_free(r);
    return out;
}

/* 0x49c40 arm */
int xml_is_space(unsigned int c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/* 0x49c68 arm */
int xml_is_name_start(unsigned int c)
{
    const xml_range *r = xml_name_start;

    while (r->lo) {
        if (c >= r->lo && c <= r->hi)
            return 1;
        r++;
    }
    return 0;
}

/* 0x49cac arm */
int xml_is_name_char(unsigned int c)
{
    const xml_range *r = xml_name_extra;

    while (r->lo) {
        if (c >= r->lo && c <= r->hi)
            return 1;
        r++;
    }
    return xml_is_name_start(c);
}

/* 0x49cec arm: the text of a predefined entity, or NULL. */
const char *xml_entity_value(const char *name)
{
    const xml_entity *e;

    for (e = xml_entities; e->name; e++)
        if (strcasecmp(name, e->name) == 0)
            return e->value;
    return NULL;
}

/* 0x49d38 arm */
char *xml_escape(const char *s, size_t n)
{
    return xml_escape_n(s, n, 0);
}

/* 0x49d40 arm */
char *xml_escape_str(const char *s)
{
    return xml_escape(s, strlen(s));
}

/* 0x49d5c arm */
char *xml_escape_ws(const char *s, size_t n)
{
    return xml_escape_n(s, n, 1);
}

/* 0x49d64 arm */
char *xml_escape_ws_str(const char *s)
{
    return xml_escape_ws(s, strlen(s));
}

static inline int xml_is_char(unsigned int c)
{
    return c == '\t' || c == '\n' || c == '\r' || (c >= 0x20 && c <= 0xd7ff)
        || (c >= 0xe000 && c <= 0xfffd) || (c >= 0x10000 && c <= 0x10ffff);
}

/* 0x49d80 arm: u with characters XML does not allow replaced by spaces (NULL if none). */
ivona_ustr *xml_clean_chars(const ivona_ustr *u)
{
    ustr_edit e;
    unsigned int c;
    int n;

    ustr_edit_init(&e, u);
    if (u->len > e.pos) {
        do {
            c = utf8_decode(u->data + e.pos, &n);
            if (xml_is_char(c))
                ustr_edit_keep(&e, n);
            else {
                ustr_edit_skip(&e, n);
                ustr_edit_insert_n(&e, " ", 1);
            }
        } while (e.pos < u->len);
    }
    return ustr_edit_result(&e);
}
