/* IVONA engine utility types: growable strings, string lists, message lists, timers. */
#ifndef IVONA_UTIL_H
#define IVONA_UTIL_H

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

/* growable NUL-terminated string (strutil.c) */
typedef struct strbuf {
    size_t len;
    size_t cap;
    char *data;
} strbuf;

strbuf *strbuf_new(void);
void strbuf_free(strbuf *b);
void strbuf_init(strbuf *b);
void strbuf_release(strbuf *b);
char *strbuf_data(strbuf *b);
void strbuf_reserve(strbuf *b, size_t n);
void strbuf_clear(strbuf *b);
void strbuf_resize(strbuf *b, size_t n);
void strbuf_append(strbuf *b, const void *data, size_t n);
void strbuf_append_str(strbuf *b, const char *s);
void strbuf_insert(strbuf *b, size_t pos, const void *data, size_t n);
void strbuf_insert_str(strbuf *b, size_t pos, const char *s);
void strbuf_erase(strbuf *b, size_t pos, size_t n);
void strbuf_swap(strbuf *a, strbuf *b);
void strbuf_truncate(strbuf *b, size_t n);
int strbuf_vappendf_n(strbuf *b, int max, const char *fmt, va_list ap);
int strbuf_appendf(strbuf *b, const char *fmt, ...);
int strbuf_appendf_n(strbuf *b, int max, const char *fmt, ...);

/* list of owned strings (strutil.c) */
typedef struct strlist {
    char **items;
    size_t count;
    size_t cap;
} strlist;

strlist *strlist_new(void);
strlist *strlist_copy(const strlist *src);
void strlist_clear(strlist *l);
void strlist_free(strlist *l);
void strlist_push(strlist *l, const char *s);
void strlist_push_n(strlist *l, const char *s, size_t n);
void strlist_split_ws_into(strlist *l, const char *s);
strlist *strlist_split_ws(const char *s);
void strlist_split_into(strlist *l, const char *s, const char *delim);
strlist *strlist_split(const char *s, const char *delim);
int strlist_contains(const strlist *l, const char *s);
int strlist_contains_nocase(const strlist *l, const char *s);
void strlist_print(const strlist *l, FILE *fp, const char *sep);
char *strlist_join(const strlist *l, const char *sep);
int strlist_compare(const strlist *a, const strlist *b);
int strlist_compare_ptr(const void *a, const void *b);
void strlist_pop(strlist *l);
void strlist_remove(strlist *l, size_t i);
void strlist_free_ptr(void *p);

unsigned int ivona_time_msec(void);
int ivona_time_usec(void);

/* diagnostics list (tts_msg.c) */
typedef struct tts_msg {
    int line;
    int column;
    int code;
    char *text;
    struct tts_msg *next;
} tts_msg;

tts_msg *tts_msg_new(int line, int column, int code, const char *fmt, ...);
void tts_msg_free(tts_msg *m);
tts_msg *tts_msg_last(tts_msg *m);
char *tts_msg_join(tts_msg *m);

#define TTS_MSG_ADD(list, code, ...) do {                                  \
        tts_msg **tail_ = (list) ? &tts_msg_last(list)->next : &(list);    \
        *tail_ = tts_msg_new(-1, -1, (code), __VA_ARGS__);                 \
    } while (0)

#endif
