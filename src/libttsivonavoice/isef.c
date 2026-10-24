/*
 * Encrypted IVONA files ("ISEF", 0x21d08..0x222ac): a header encrypted with AES-256-CBC under a
 * key that is RSA-signed with the IVONA private key, followed by plain data in place.
 *
 *   "ISEF"  00 00 00 01  be32 version  512-byte RSA block (key[32] iv[32])
 *   be32 n  n bytes of encrypted header  be32 m  m bytes of data
 */
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include "cst_alloc.h"
#include "cst_file.h"
#include "ivona_util.h"
#include "tts_internal.h"

typedef struct textstream textstream;
textstream *textstream_open(const char *path);
void textstream_close(textstream *ts);
const char *textstream_ptr(const textstream *ts);
int textstream_ensure(textstream *ts, size_t n);
void textstream_advance(textstream *ts, size_t n);
size_t textstream_offset(const textstream *ts);

/* IVONA public key: modulus and exponent */
static unsigned char isef_key[0x204] = {
    0xe1, 0x4a, 0xfd, 0x52, 0x46, 0x84, 0xc0, 0x5b, 0xfc, 0x21, 0xb6, 0x64,
    0xe0, 0x31, 0x2b, 0xa3, 0x86, 0x24, 0x6a, 0xdb, 0xfd, 0x9d, 0xd0, 0xe6,
    0x17, 0x0a, 0x44, 0x3a, 0xee, 0x77, 0x1d, 0x6d, 0x42, 0x8e, 0x6e, 0xd4,
    0x21, 0x0e, 0x2b, 0x2f, 0x2c, 0xb9, 0x9a, 0x4c, 0x29, 0x05, 0xc6, 0x25,
    0xc3, 0x0e, 0x79, 0xa1, 0xdc, 0x5a, 0x94, 0xae, 0xaa, 0x64, 0x44, 0xcd,
    0x97, 0x66, 0x0b, 0x16, 0x6a, 0x88, 0x20, 0x43, 0x62, 0xf5, 0x1f, 0xa8,
    0xb4, 0x5e, 0xbe, 0xe3, 0xe2, 0x11, 0x65, 0x2a, 0x8e, 0x20, 0xc8, 0x80,
    0x44, 0xec, 0xa3, 0xa7, 0x26, 0x41, 0x9c, 0x15, 0xf4, 0x30, 0x0e, 0x29,
    0xfa, 0xf4, 0x1f, 0x75, 0x42, 0xe7, 0xa2, 0x7d, 0xb0, 0xec, 0xb4, 0xdc,
    0x13, 0x54, 0x6b, 0x8d, 0xe6, 0xe3, 0x21, 0xc4, 0xbe, 0x6c, 0x73, 0xdc,
    0x84, 0x00, 0x72, 0xce, 0xa0, 0xb9, 0x94, 0x71, 0x40, 0x88, 0xc0, 0x55,
    0x33, 0x0c, 0x29, 0xac, 0xab, 0x93, 0x33, 0x5f, 0x91, 0xfe, 0x71, 0x48,
    0xcd, 0x39, 0xfd, 0x40, 0x6a, 0xd9, 0xb4, 0xac, 0xa2, 0x6b, 0xeb, 0xff,
    0xcf, 0xee, 0xf9, 0x7d, 0x1c, 0xa5, 0xd5, 0x43, 0x98, 0x15, 0xd1, 0x76,
    0xf0, 0x87, 0xe3, 0xb4, 0x8a, 0xde, 0xa6, 0xb9, 0x51, 0x28, 0xe4, 0xf1,
    0xb5, 0x7d, 0x14, 0x4c, 0x90, 0x9e, 0xee, 0xba, 0x75, 0x3a, 0x3a, 0xb9,
    0x2b, 0x71, 0xb6, 0x05, 0xcf, 0x0c, 0x41, 0xa4, 0x1a, 0xf9, 0x15, 0x62,
    0xd2, 0xb2, 0x05, 0x71, 0xd2, 0xdf, 0xda, 0x9a, 0x4a, 0x5b, 0x37, 0xc6,
    0xc9, 0xda, 0x04, 0x30, 0xc5, 0xb5, 0x4e, 0x3b, 0x5a, 0xaf, 0x17, 0x17,
    0x49, 0x4b, 0x31, 0x2a, 0x08, 0xfb, 0xf9, 0xa3, 0x98, 0x00, 0x22, 0x2d,
    0x68, 0x28, 0x4b, 0xa9, 0x80, 0x89, 0x96, 0x7f, 0x8a, 0x66, 0xd4, 0x16,
    0x6a, 0xf1, 0x4c, 0xcc, 0x93, 0x91, 0x72, 0x33, 0x8a, 0xba, 0x87, 0xe8,
    0xbb, 0x08, 0x7b, 0xfd, 0xc1, 0xe9, 0x10, 0x96, 0x73, 0x7d, 0x1e, 0x71,
    0x3d, 0x15, 0xb6, 0x3b, 0xf4, 0xac, 0x4c, 0xa4, 0x5d, 0x21, 0x90, 0x0c,
    0x51, 0x71, 0x2c, 0x03, 0xdf, 0xd2, 0xbd, 0x64, 0xd3, 0x77, 0x43, 0xce,
    0x48, 0x9f, 0x43, 0x3d, 0xd6, 0xeb, 0x4c, 0x88, 0x48, 0x88, 0xf8, 0xb0,
    0xb1, 0x67, 0x10, 0x6d, 0x42, 0x97, 0x6b, 0x86, 0x0c, 0x19, 0x1a, 0xd2,
    0x6c, 0x10, 0xd5, 0x3f, 0x99, 0x3a, 0x9a, 0x77, 0xa2, 0x05, 0x03, 0x41,
    0x55, 0x32, 0x18, 0x4f, 0x67, 0xcf, 0x52, 0x55, 0x82, 0x49, 0x5e, 0xc3,
    0x28, 0xe7, 0xe3, 0xaa, 0xc7, 0x84, 0xaf, 0x47, 0x25, 0x4c, 0x56, 0x52,
    0xb6, 0x7f, 0x23, 0xc2, 0x26, 0xc5, 0x32, 0x2c, 0x9f, 0x84, 0x89, 0xb0,
    0x8d, 0x62, 0xf1, 0x54, 0x63, 0x69, 0x78, 0x38, 0x98, 0xdd, 0x99, 0x2e,
    0x1a, 0x53, 0xa1, 0xea, 0xcb, 0x37, 0x71, 0x9f, 0xaa, 0x6a, 0x51, 0xc6,
    0x47, 0x66, 0x41, 0xc6, 0x25, 0xab, 0xcf, 0xde, 0xca, 0x63, 0xc0, 0x1e,
    0xd3, 0x27, 0x1d, 0xac, 0xe8, 0x41, 0xdd, 0xf9, 0x04, 0xe3, 0x1c, 0x4f,
    0x01, 0x01, 0x21, 0x7b, 0xe9, 0x50, 0x5b, 0x20, 0x86, 0xa6, 0xaa, 0x32,
    0x88, 0xe5, 0xc8, 0x43, 0x7a, 0x2e, 0x20, 0xf3, 0x11, 0xa0, 0xc0, 0x0b,
    0x48, 0x17, 0x7b, 0x82, 0x61, 0x1e, 0x3e, 0xac, 0xa5, 0x8a, 0x90, 0xf1,
    0x82, 0x57, 0x78, 0x6b, 0x1a, 0x2f, 0x1b, 0xd7, 0x59, 0xa6, 0xfc, 0x7b,
    0xa9, 0x1a, 0x5c, 0x26, 0xd8, 0xce, 0x6f, 0x3b, 0x70, 0x33, 0xab, 0x4d,
    0x84, 0xe2, 0xf4, 0x5c, 0x51, 0xe1, 0xfb, 0x19, 0x58, 0xd0, 0xe2, 0x83,
    0xfa, 0x21, 0x60, 0x16, 0xc4, 0x00, 0x32, 0xfb, 0x35, 0x9e, 0x5a, 0x94,
    0x5a, 0xf3, 0x92, 0x16, 0x4b, 0x49, 0x49, 0x79, 0x00, 0x01, 0x00, 0x01,
};

typedef struct isef_reader {
    textstream *ts;
    RSA *rsa;
    unsigned char *key;         /* key and iv */
    EVP_CIPHER_CTX ctx;
} isef_reader;

typedef struct isef_file {
    const char *path;
    cst_filemap *map;
    unsigned int version;
    strbuf header;              /* +0x0c: decrypted */
    const char *data;           /* +0x18: in map */
    size_t size;
} isef_file;

static inline unsigned int be32(const void *p)
{
    unsigned int v = *(const unsigned int *)p;

    return v << 24 | (v & 0xff00) << 8 | (v >> 8 & 0xff00) | v >> 24;
}

static inline void isef_reader_close(isef_reader *r)
{
    RSA_free(r->rsa);
    cst_free(r->key);
    EVP_CIPHER_CTX_cleanup(&r->ctx);
    textstream_close(r->ts);
    cst_free(r);
}

/* 0x21d08 arm */
static void isef_reader_free(isef_reader *r)
{
    if (r == NULL)
        return;
    RSA_free(r->rsa);
    cst_free(r->key);
    EVP_CIPHER_CTX_cleanup(&r->ctx);
    textstream_close(r->ts);
    cst_free(r);
}

static isef_reader *isef_reader_open(const char *path)
{
    isef_reader *r = cst_safe_alloc(1, sizeof(isef_reader));

    if ((r->ts = textstream_open(path)) == NULL) {
        tts_set_error_contextf(TTS_STATUS_LOAD_VOICE_ERROR, "Opening file %s", path);
        cst_free(r);
        return NULL;
    }
    r->rsa = NULL;
    r->key = NULL;
    EVP_CIPHER_CTX_init(&r->ctx);
    return r;
}

/* 0x21d40 arm */
void isef_free(isef_file *f)
{
    if (f == NULL)
        return;
    strbuf_release(&f->header);
    if (f->map)
        cst_munmap_file(f->map);
    cst_free(f);
}

/* checks the header and recovers the key; 0 on error */
static int isef_read_key(isef_file *f, isef_reader *r)
{
    if (!textstream_ensure(r->ts, 4) || memcmp(textstream_ptr(r->ts), "ISEF", 4) != 0) {
        tts_set_errorf(TTS_STATUS_LOAD_VOICE_ERROR, "Invalid file %s", f->path);
        return 0;
    }
    textstream_advance(r->ts, 4);
    if (!textstream_ensure(r->ts, 4) || memcmp(textstream_ptr(r->ts), "\0\0\0\1", 4) != 0) {
        tts_set_errorf(TTS_STATUS_LOAD_VOICE_ERROR, "Invalid version of file %s", f->path);
        return 0;
    }
    textstream_advance(r->ts, 4);
    if (!textstream_ensure(r->ts, 4)) {
        tts_set_errorf(TTS_STATUS_LOAD_VOICE_ERROR, "Invalid file %s", f->path);
        return 0;
    }
    f->version = be32(textstream_ptr(r->ts));
    textstream_advance(r->ts, 4);
    if (!textstream_ensure(r->ts, 0x200)) {
        tts_set_errorf(TTS_STATUS_LOAD_VOICE_ERROR, "Invalid file %s", f->path);
        return 0;
    }
    r->rsa = RSA_new();
    r->rsa->n = BN_bin2bn(isef_key, 0x200, NULL);
    r->rsa->e = BN_bin2bn(isef_key + 0x200, 4, NULL);
    if (r->rsa->n == NULL || r->rsa->e == NULL) {
        tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "Load key");
        return 0;
    }
    r->key = cst_safe_alloc(RSA_size(r->rsa), 1);
    if (RSA_public_decrypt(0x200, (const unsigned char *)textstream_ptr(r->ts), r->key, r->rsa,
                           RSA_PKCS1_PADDING) * 8 != 0x200) {
        tts_set_errorf(TTS_STATUS_LOAD_VOICE_ERROR, "Invalid file %s", f->path);
        return 0;
    }
    textstream_advance(r->ts, 0x200);
    return 1;
}

/* 0x21d70 arm */
isef_file *isef_open(const char *path)
{
    isef_file *f = cst_safe_alloc(1, sizeof(isef_file));
    isef_reader *r;
    unsigned int n;
    int outl, finl;

    f->path = path;
    f->version = 0;
    strbuf_init(&f->header);
    if ((r = isef_reader_open(f->path)) == NULL) {
        isef_free(f);
        return NULL;
    }
    if ((f->map = cst_mmap_file(f->path)) == NULL) {
        tts_set_error_contextf(TTS_STATUS_LOAD_VOICE_ERROR, "Opening file %s", f->path);
        goto close;
    }
    if (!isef_read_key(f, r)) {
close:
        isef_reader_close(r);
        isef_free(f);
        return NULL;
    }
    if (!EVP_DecryptInit_ex(&r->ctx, EVP_aes_256_cbc(), NULL, r->key, r->key + 0x20)) {
        tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "Broken file %s", f->path);
        isef_reader_free(r);
        isef_free(f);
        return NULL;
    }
    if (!textstream_ensure(r->ts, 4)) {
        tts_set_errorf(TTS_STATUS_LOAD_VOICE_ERROR, "Invalid file %s", f->path);
        isef_reader_free(r);
        isef_free(f);
        return NULL;
    }
    n = be32(textstream_ptr(r->ts));
    textstream_advance(r->ts, 4);
    if (!textstream_ensure(r->ts, n)) {
        tts_set_errorf(TTS_STATUS_LOAD_VOICE_ERROR, "Invalid file %s", f->path);
        isef_reader_free(r);
        isef_free(f);
        return NULL;
    }
    outl = 0;
    finl = 0;
    strbuf_resize(&f->header, n + 16);
    if (!EVP_DecryptUpdate(&r->ctx, (unsigned char *)f->header.data, &outl,
                           (const unsigned char *)textstream_ptr(r->ts), n)
        || !EVP_DecryptFinal_ex(&r->ctx, (unsigned char *)f->header.data + outl, &finl)) {
        isef_reader_free(r);
        isef_free(f);
        return NULL;
    }
    strbuf_resize(&f->header, outl + finl);
    textstream_advance(r->ts, n);
    if (!textstream_ensure(r->ts, 4)) {
        tts_set_errorf(TTS_STATUS_LOAD_VOICE_ERROR, "Invalid file %s", f->path);
        isef_reader_free(r);
        isef_free(f);
        return NULL;
    }
    n = be32(textstream_ptr(r->ts));
    textstream_advance(r->ts, 4);
    if (f->map->mapsize < textstream_offset(r->ts) + n) {
        tts_set_errorf(TTS_STATUS_LOAD_VOICE_ERROR, "Invalid file %s", f->path);
        isef_reader_free(r);
        isef_free(f);
        return NULL;
    }
    f->data = (const char *)f->map->mem + textstream_offset(r->ts);
    f->size = n;
    isef_reader_free(r);
    return f;
}

/* 0x222ac arm */
strbuf *isef_header(isef_file *f)
{
    return &f->header;
}

/* 0x222b4 arm */
const char *isef_path(const isef_file *f)
{
    return f->path;
}

/* 0x222bc arm */
unsigned int isef_version(const isef_file *f)
{
    return f->version;
}

/* entries of the header and the data: char name[16] (zero padded), be32 size, data */
#define ISEF_NAME 16
#define ISEF_ENTRY 0x14

static const char isef_zeros[ISEF_NAME];

static inline int isef_find_in(const char *base, size_t total, const char *name, size_t len,
                               const char **data, size_t *size)
{
    size_t off, n;

    for (off = 0; off + ISEF_ENTRY <= total; off += ISEF_ENTRY + n) {
        n = be32(base + off + ISEF_NAME);
        if (memcmp(base + off, name, len) == 0
            && memcmp(base + off + len, isef_zeros, ISEF_NAME - len) == 0) {
            *data = base + off + ISEF_ENTRY;
            *size = n;
            return 1;
        }
    }
    return 0;
}

/* 0x222c4 arm: the entry named name, in the header or else in the data */
int isef_find(const isef_file *f, const char *name, const char **data, size_t *size)
{
    size_t len = strlen(name);
    const char *base = f->header.data;
    size_t total = f->header.len;

    if (len <= ISEF_NAME
        && (isef_find_in(base, total, name, len, data, size)
            || isef_find_in(f->data, f->size, name, len, data, size)))
        return 1;
    return 0;
}

/* 0x2243c arm: the entry after the one whose data is [p, p + n) (the first for p NULL); returns
 * its name or NULL. */
const char *isef_next(const isef_file *f, const char *p, size_t n, const char **data, size_t *size)
{
    const char *e;

    if (p == NULL) {
        if (f->header.len >= ISEF_ENTRY)
            e = f->header.data;
        else if (f->size >= ISEF_ENTRY)
            e = f->data;
        else
            return NULL;
    } else if (p > f->header.data && p + n < f->header.data + f->header.len) {
        if (f->header.len - n + (f->header.data - p) < ISEF_ENTRY)
            return NULL;
        e = p + n;
    } else if (p > f->header.data && p + n == f->header.data + f->header.len) {
        if (f->size < ISEF_ENTRY)
            return NULL;
        e = f->data;
    } else {
        if (p <= f->data || p + n >= f->data + f->size)
            return NULL;
        if (f->size - n + (f->data - p) < ISEF_ENTRY)
            return NULL;
        e = p + n;
    }
    *data = e + ISEF_ENTRY;
    *size = be32(e + ISEF_NAME);
    return e;
}
