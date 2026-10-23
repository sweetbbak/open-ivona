/*
 * MessagePack I/O (0x564ac..0x56960) over the cmp library: readers over memory (strings and binary
 * data are returned in place, or copied when asked), writers to a file or a growing buffer.
 */
#include <stdio.h>
#include <string.h>
#include "cst_alloc.h"
#include "ivona_vector.h"

/* cmp's context */
typedef struct cmp_ctx {
    unsigned char error;
    void *buf;
    int (*read)(struct cmp_ctx *ctx, void *data, size_t limit);
    size_t (*write)(struct cmp_ctx *ctx, const void *data, size_t count);
} cmp_ctx;

typedef struct cmp_object {
    unsigned char type;
    union {
        unsigned int str_size;
        unsigned int bin_size;
        long long s64;
    } as;
} cmp_object;

#define CMP_TYPE_FIXSTR 3
#define CMP_TYPE_BIN8   6
#define CMP_TYPE_BIN16  7
#define CMP_TYPE_BIN32  8
#define CMP_TYPE_STR8   27
#define CMP_TYPE_STR16  28
#define CMP_TYPE_STR32  29
#define INVALID_TYPE_ERROR 13

void cmp_init(cmp_ctx *ctx, void *buf, int (*read)(cmp_ctx *, void *, size_t),
              size_t (*write)(cmp_ctx *, const void *, size_t));
int cmp_read_object(cmp_ctx *ctx, cmp_object *obj);
int cmp_read_int(cmp_ctx *ctx, int *i);
int cmp_read_array(cmp_ctx *ctx, unsigned int *size);
int cmp_write_str(cmp_ctx *ctx, const char *data, unsigned int size);

#define MP_COPY     2   /* strings and binaries are copied */
#define MP_TRACK    4   /* copies are freed with the reader */

/* a memory reader's input */
typedef struct mp_mem_in {
    unsigned int flags;
    const char *cur;
    const char *end;
    ivona_vector *copies;
} mp_mem_in;

/* a memory writer's output */
typedef struct mp_mem_out {
    size_t len;
    size_t cap;
    char *data;
} mp_mem_out;

/* 0x564ac arm */
static inline int mp_mem_read(cmp_ctx *ctx, void *data, size_t limit)
{
    mp_mem_in *in = ctx->buf;

    if (in->flags & MP_COPY || in->cur + limit > in->end)
        return 0;
    memcpy(data, in->cur, limit);
    in->cur += limit;
    return 1;
}

/* 0x56500 arm */
static void mp_free_elem(void *elem, size_t size)
{
    cst_free(elem);
}

/* 0x56504 arm */
static size_t mp_file_write(cmp_ctx *ctx, const void *data, size_t count)
{
    return fwrite(data, 1, count, ctx->buf);
}

/* 0x56514 arm */
static size_t mp_mem_write(cmp_ctx *ctx, const void *data, size_t count)
{
    mp_mem_out *out = ctx->buf;

    if (out->len + count > out->cap) {
        out->cap = out->cap + 0x400 + count;
        out->data = cst_safe_realloc(out->data, out->cap, 1);
    }
    memcpy(out->data + out->len, data, count);
    out->len += count;
    return count;
}

/* 0x56588 arm */
void mp_reader_init(cmp_ctx *ctx, const void *data, size_t size, unsigned int flags)
{
    mp_mem_in *in = cst_safe_alloc(1, sizeof(mp_mem_in));

    in->flags = flags;
    in->cur = data;
    in->end = (const char *)data + size;
    if (flags & MP_TRACK)
        in->copies = vector_new(sizeof(void *), NULL, mp_free_elem);
    else
        in->copies = NULL;
    cmp_init(ctx, in, mp_mem_read, NULL);
}

/* 0x56600 arm: whether the whole input was read. */
int mp_reader_free(cmp_ctx *ctx)
{
    mp_mem_in *in = ctx->buf;
    int ok = in->cur == in->end;

    if (in->copies)
        vector_free(in->copies);
    cst_free(in);
    return ok;
}

/* 0x5663c arm */
int mp_read_int(cmp_ctx *ctx)
{
    int i;

    cmp_read_int(ctx, &i);
    return i;
}

/* 0x56658 arm: a string or binary object (in place unless copying). */
void *mp_read_bin(cmp_ctx *ctx, unsigned int *size, int tag)
{
    cmp_object obj;
    mp_mem_in *in;
    unsigned int len;
    void *p;

    if (!cmp_read_object(ctx, &obj))
        return NULL;
    switch (obj.type) {
    case CMP_TYPE_FIXSTR:
    case CMP_TYPE_STR8:
    case CMP_TYPE_STR16:
    case CMP_TYPE_STR32:
        len = obj.as.str_size;
        break;
    case CMP_TYPE_BIN8:
    case CMP_TYPE_BIN16:
    case CMP_TYPE_BIN32:
        len = obj.as.bin_size;
        break;
    default:
        ctx->error = INVALID_TYPE_ERROR;
        return NULL;
    }
    in = ctx->buf;
    if (in->flags & MP_COPY) {
        p = cst_safe_alloc(len, 1);
        mp_mem_read(ctx, p, len);
        if (in->copies)
            vector_push(in->copies, &p);
    } else {
        if (in->cur + len > in->end)
            return NULL;
        p = (void *)in->cur;
        in->cur += len;
    }
    if (p && size) {
        *size = len;
        ctx->error = tag;
    }
    return p;
}

/* 0x56770 arm */
char **mp_read_str_array(cmp_ctx *ctx, unsigned int *count)
{
    unsigned int n, i;
    char **a, *s;

    cmp_read_array(ctx, &n);
    a = cst_safe_alloc(n, sizeof(char *));
    for (i = 0; i < n; i++) {
        s = a[i] = mp_read_bin(ctx, NULL, 1);
        if (s == NULL) {
            cst_free(a);
            return NULL;
        }
    }
    if (count)
        *count = n;
    return a;
}

/* 0x56808 arm */
int mp_writer_open_file(cmp_ctx *ctx, const char *path)
{
    FILE *f = fopen(path, "w+b");

    if (f == NULL)
        return 0;
    cmp_init(ctx, f, NULL, mp_file_write);
    return 1;
}

/* 0x56854 arm */
void mp_writer_open_mem(cmp_ctx *ctx)
{
    mp_mem_out *out = cst_safe_alloc(1, sizeof(mp_mem_out));

    out->data = cst_safe_alloc(0x400, 1);
    out->len = 0;
    out->cap = 0x400;
    cmp_init(ctx, out, NULL, mp_mem_write);
}

/* 0x568a4 arm */
int mp_writer_close_file(cmp_ctx *ctx)
{
    if (ctx->write != mp_file_write)
        return 0;
    return fclose(ctx->buf) == 0;
}

/* 0x568dc arm: the written data (allocated). */
void *mp_writer_close_mem(cmp_ctx *ctx, size_t *len)
{
    mp_mem_out *out;
    void *data;

    if (ctx->write != mp_mem_write)
        return NULL;
    out = ctx->buf;
    data = cst_safe_realloc(out->data, out->len, 1);
    if (len)
        *len = out->len;
    cst_free(out);
    return data;
}

/* 0x56938 arm: s with its NUL. */
int mp_write_str(cmp_ctx *ctx, const char *s)
{
    return cmp_write_str(ctx, s, strlen(s) + 1);
}
