/*
 * Input helpers (0x45054..0x45650): CRC-32, buffered text streams over a file or a memory block,
 * and XML parsing support (Expat unknown-encoding handler, namespace prefix stack).
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "ivona_util.h"
#include "ivona_vector.h"
#include "tts_internal.h"

static unsigned int crc_table[256];

static void make_crc_table(void)
{
    unsigned int c;
    int i, k;

    for (i = 0; i < 256; i++) {
        c = i;
        for (k = 0; k < 8; k++)
            c = c & 1 ? 0xedb88320 ^ (c >> 1) : c >> 1;
        crc_table[i] = c;
    }
}

/* 0x45054 arm: CRC-32 of buf. The table is (re)built whenever crc_table[0] is 0, which it always
 * is, and the running value is inverted after every byte. */
unsigned int ivona_crc32(const unsigned char *buf, int len)
{
    unsigned int crc;
    int i;

    if (crc_table[0] == 0)
        make_crc_table();
    crc = 0xffffffff;
    for (i = 0; i < len; i++)
        crc = ~(crc_table[(crc ^ buf[i]) & 0xff] ^ (crc >> 8));
    return crc;
}

#define TEXTSTREAM_CHUNK 0x1000

/* buffered input; a memory stream has no file */
typedef struct textstream {
    strbuf buf;
    size_t pos;         /* read position in buf */
    size_t consumed;    /* bytes advanced over in total */
    FILE *fp;
} textstream;

FILE *ivona_fopen(const char *path, const char *mode);

static inline textstream *textstream_new(void)
{
    textstream *ts = cst_safe_calloc(1, sizeof(textstream));

    strbuf_init(&ts->buf);
    return ts;
}

/* 0x45178 arm */
textstream *textstream_open(const char *path)
{
    FILE *fp = ivona_fopen(path, "rb");
    textstream *ts;

    if (fp == NULL) {
        tts_set_error_errno(errno, "file_open");
        return NULL;
    }
    ts = textstream_new();
    ts->fp = fp;
    return ts;
}

/* 0x451d4 arm: a stream over a copy of data. */
textstream *textstream_from_mem(const void *data, size_t len)
{
    textstream *ts = textstream_new();

    strbuf_append(&ts->buf, data, len);
    return ts;
}

/* 0x4520c arm */
void textstream_close(textstream *ts)
{
    if (ts == NULL)
        return;
    if (ts->fp)
        fclose(ts->fp);
    strbuf_release(&ts->buf);
    cst_free(ts);
}

/* 0x4523c arm: buffered bytes not yet read. */
size_t textstream_avail(const textstream *ts)
{
    return ts->buf.len - ts->pos;
}

/* 0x4524c arm */
const char *textstream_ptr(const textstream *ts)
{
    return ts->buf.data + ts->pos;
}

/* 0x4525c arm: reads up to n more bytes from the file (dropping consumed data first). */
size_t textstream_fill(textstream *ts, size_t n)
{
    size_t len;

    if (ts->fp == NULL)
        return 0;
    if (ts->pos >= TEXTSTREAM_CHUNK) {
        strbuf_erase(&ts->buf, 0, ts->pos);
        ts->pos = 0;
    }
    len = ts->buf.len;
    strbuf_resize(&ts->buf, len + n);
    n = fread(ts->buf.data + len, 1, n, ts->fp);
    strbuf_resize(&ts->buf, len + n);
    return n;
}

/* 0x452dc arm: reads the rest of the file. */
size_t textstream_fill_all(textstream *ts)
{
    size_t total = 0, n;

    while ((n = textstream_fill(ts, TEXTSTREAM_CHUNK)) != 0)
        total += n;
    return total;
}

/* 0x4530c arm: whether n bytes can be read. */
int textstream_ensure(textstream *ts, size_t n)
{
    while (textstream_avail(ts) < n)
        if (textstream_fill(ts, TEXTSTREAM_CHUNK) == 0)
            break;
    return textstream_avail(ts) >= n;
}

/* 0x45358 arm */
void textstream_advance(textstream *ts, size_t n)
{
    if (!textstream_ensure(ts, n))
        n = textstream_avail(ts);
    ts->pos += n;
    ts->consumed += n;
}

/* 0x45398 arm */
size_t textstream_offset(const textstream *ts)
{
    return ts->consumed;
}

/* an in-scope XML namespace declaration */
typedef struct xml_ns {
    char *prefix;   /* NULL for the default namespace */
    char *uri;
} xml_ns;

/* 0x453a0 arm */
void xml_ns_free(xml_ns *ns)
{
    cst_free(ns->prefix);
    cst_free(ns->uri);
}

/* Expat's XML_Encoding */
typedef struct xml_encoding {
    int map[256];
    void *data;
    int (*convert)(void *data, const char *s);
    void (*release)(void *data);
} xml_encoding;

const unsigned short *charset_table(const char *name);

/* 0x453bc arm: Expat unknown-encoding handler for the engine's 8-bit character sets. */
int xml_unknown_encoding(void *data, const char *name, xml_encoding *info)
{
    const unsigned short *t = charset_table(name);
    int i;

    if (t == NULL)
        return 0;
    for (i = 0; i < 256; i++)
        info->map[i] = t[i];
    info->data = NULL;
    info->convert = NULL;
    info->release = NULL;
    return 1;
}

/* 0x45408 arm */
void xml_ns_init(xml_ns *ns, const char *prefix, const char *uri)
{
    ns->prefix = prefix ? (char *)cst_strdup((const cst_string *)prefix) : NULL;
    ns->uri = (char *)cst_strdup((const cst_string *)uri);
}

/* 0x45438 arm: stack of xml_ns. */
ivona_vector *xml_ns_stack_new(void)
{
    return vector_new(sizeof(xml_ns), (ivona_vector_copy_fn)memcpy, (ivona_vector_destroy_fn)xml_ns_free);
}

/* 0x45458 arm */
void xml_ns_push(ivona_vector *stack, const char *prefix, const char *uri)
{
    xml_ns ns;

    xml_ns_init(&ns, prefix, uri);
    vector_push(stack, &ns);
}

/* 0x45480 arm: innermost uri bound to prefix (NULL: the default namespace). */
const char *xml_ns_lookup(ivona_vector *stack, const char *prefix)
{
    xml_ns *ns;
    int i;

    for (i = vector_size(stack) - 1; i >= 0; i--) {
        ns = vector_at(stack, i);
        if (prefix ? ns->prefix && strcmp(ns->prefix, prefix) == 0 : ns->prefix == NULL)
            return ns->uri;
    }
    return NULL;
}

/* 0x45514 arm: "prefix:local" (or "local") as Expat's "uri^local"; NULL if unbound. */
char *xml_ns_qualify(ivona_vector *stack, const char *name)
{
    const char *colon = strchr(name, ':');
    size_t n;
    xml_ns *ns;
    int i;

    if (colon == NULL || colon == name) {
        for (i = vector_size(stack) - 1; i >= 0; i--) {
            ns = vector_at(stack, i);
            if (ns->prefix == NULL)
                return cst_strcat3(ns->uri, "^", colon ? colon + 1 : name);
        }
        return NULL;
    }
    n = colon - name;
    for (i = vector_size(stack) - 1; i >= 0; i--) {
        ns = vector_at(stack, i);
        if (n == 0 ? ns->prefix == NULL
                   : ns->prefix && strncmp(name, ns->prefix, n) == 0 && ns->prefix[n] == '\0')
            return cst_strcat3(ns->uri, "^", colon + 1);
    }
    return NULL;
}

/* 0x4564c arm */
FILE *ivona_fopen(const char *path, const char *mode)
{
    return fopen(path, mode);
}
