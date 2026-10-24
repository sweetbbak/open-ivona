#ifndef IVONA_USTR_H
#define IVONA_USTR_H

#include <stddef.h>

/* Offset-tracking UTF-8 strings (ustr.c). */

/* input bytes [start, end) */
typedef struct ustr_span {
    unsigned int start;
    unsigned int end;
} ustr_span;

typedef struct ivona_ustr {
    char *data;         /* NUL-terminated */
    size_t len;
    size_t cap;
    ustr_span *spans;   /* span of each byte of data */
    unsigned int start; /* span of the whole string */
    unsigned int end;
} ivona_ustr;

ivona_ustr *ustr_new(unsigned int pos, const char *s, size_t n);
ivona_ustr *ustr_copy(const ivona_ustr *src);
void ivona_ustr_free(ivona_ustr *u);
void ustr_clear(ivona_ustr *u);
void ustr_span_of(const ivona_ustr *u, size_t idx, size_t n, ustr_span *out);
void ustr_append(ivona_ustr *u, const char *s, size_t n, const ustr_span *span);
ivona_ustr *ustr_from(const char *s, size_t n, const ustr_span *span);
void ustr_append_char(ivona_ustr *u, unsigned int c, const ustr_span *span);
void ustr_append_ustr(ivona_ustr *u, const ivona_ustr *src, size_t from, size_t n);
ivona_ustr *ustr_sub(const ivona_ustr *src, size_t from, size_t n);
int ustr_equal(const ivona_ustr *a, const ivona_ustr *b);

/* copy-on-write editing (ustr_edit.c) */
typedef struct ustr_edit {
    const ivona_ustr *src;
    ivona_ustr *out;    /* NULL until the first change */
    size_t pos;         /* read position in src */
    size_t skipped;     /* length of the last skip */
} ustr_edit;

void ustr_edit_init_out(ustr_edit *e, const ivona_ustr *src, ivona_ustr *out);
void ustr_edit_init(ustr_edit *e, const ivona_ustr *src);
ivona_ustr *ustr_edit_result(ustr_edit *e);
void ustr_edit_skip(ustr_edit *e, size_t n);
void ustr_edit_copy(ustr_edit *e, size_t from, size_t n);
void ustr_edit_keep(ustr_edit *e, size_t n);
void ustr_edit_insert_n(ustr_edit *e, const char *s, size_t n);
void ustr_edit_insert_span(ustr_edit *e, const char *s, size_t n, const ustr_span *span);
void ustr_edit_insert(ustr_edit *e, const char *s);
void ustr_edit_insert_char(ustr_edit *e, unsigned int c);

#endif
