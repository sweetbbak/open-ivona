/*
 * IVONA's rewrite of Flite's cst_tokenstream.c (0x5430c..0x55304): tokens are read through
 * per-encoding character callbacks (ASCII, or UTF-8 for ts_open_string_utf8), characters can be
 * pushed back (ts_unget), and a token's trailing punctuation is split off after the fact.
 */
#include <stdio.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_file.h"
#include "cst_string.h"
#include "cst_tokenstream.h"

int utf8_encode(unsigned int c, char *out);
int utf8_decode(const char *s, int *len);
const char *utf8_strchr(const char *s, int c);
size_t utf8_strspn(const char *s, const char *set, int flags);
int utf8_charlen(int c);


/* 0x5430c arm */
static int ascii_charlen(int c)
{
    return c != EOF;
}

/* 0x54318 arm */
static int ascii_decode(const char *s, int *len)
{
    int c = *(const unsigned char *)s;

    *len = 1;
    return c;
}

/* 0x54328 arm */
static int ascii_encode(int c, char *out)
{
    *out = c;
    return 1;
}

/* 0x54334 arm */
static int ts_utf8_encode(int c, char *out)
{
    return utf8_encode(c, out);
}

/* 0x54338 arm */
static int ts_utf8_decode(const char *s, int *len)
{
    return utf8_decode(s, len);
}

/* 0x5433c arm */
static size_t ts_utf8_strspn(const char *s, const char *set)
{
    return utf8_strspn(s, set, 0);
}

/* 0x54344 arm */
static const char *ts_utf8_strchr(const char *set, int c)
{
    return utf8_strchr(set, c);
}

/* 0x54348 arm */
static int ts_utf8_charlen(int c)
{
    if (c == EOF)
        return 0;
    return utf8_charlen(c);
}

/* reads the next character into ts->current_char */
static inline int ts_getc(cst_tokenstream *ts)
{
    int len;

    if (ts->unget) {
        int c = ts->decode(ts->unget + ts->unget_pos, &len);

        ts->unget_pos += len;
        ts->current_char = c;
        if (ts->unget_pos == ts->unget_len) {
            cst_free(ts->unget);
            ts->unget = NULL;
        }
    } else if (ts->fd) {
        len = 1;
        ts->current_char = cst_fgetc(ts->fd);
    } else if (ts->string_buffer) {
        if (ts->string_buffer[ts->file_pos] == '\0') {
            ts->current_char = EOF;
            return EOF;
        }
        ts->current_char = ts->decode(&ts->string_buffer[ts->file_pos], &len);
    }
    if (ts->current_char != EOF) {
        ts->file_pos += len;
        if (ts->current_char == '\n')
            ts->line_number++;
    }
    return ts->current_char;
}

/* 0x5435c arm: pushes s back so that it is read again before the current character. */
static void ts_unget(cst_tokenstream *ts, const char *s)
{
    int n, c, clen, len;
    char *buf;
    int total;

    n = strlen(s);
    if (n == 0)
        return;
    c = ts->current_char;
    clen = ts->charlen(c);
    ts->current_char = ts->decode(s, &len);
    ts->file_pos += len - n - clen;
    if (ts->unget) {
        total = ts->unget_len - (len + ts->unget_pos) + n + clen;
        buf = cst_safe_alloc(total, 1);
        strcpy(buf, s + len);
        ts->encode(c, buf + (n - len));
        memcpy(buf + (n - len) + clen, ts->unget + ts->unget_pos, ts->unget_len - ts->unget_pos);
        cst_free(ts->unget);
        ts->unget_len = total;
        ts->unget = buf;
        ts->unget_pos = 0;
        return;
    }
    if (n <= len && c == EOF)
        return;
    ts->unget = cst_safe_alloc(1 - len + n + clen, 1);
    strcpy(ts->unget, s + len);
    ts->unget_pos = 0;
    ts->unget_len = n - len;
    if (c != EOF) {
        ts->encode(c, ts->unget + ts->unget_len);
        ts->unget_len += clen;
    }
}

#define TS_BUFFER_SIZE 256

/* grows *buffer to hold needed bytes (and a NUL) */
static inline void extend_buffer(char **buffer, int *buffer_max, int needed)
{
    int new_max;

    if (needed <= *buffer_max)
        return;
    new_max = *buffer_max + *buffer_max / 5 + 1;
    if (new_max < needed)
        new_max = needed;
    *buffer = cst_safe_realloc(*buffer, new_max + 1, 1);
    *buffer_max = new_max;
}

/* 0x5450c arm: reads the run of characters in charclass (no single-character symbols). */
static void get_token_sub_part(cst_tokenstream *ts, const char *charclass, char **buffer,
                               int *buffer_max)
{
    int p = 0, len = ts->charlen(ts->current_char);

    while (ts->member(charclass, ts->current_char)
           && !ts->member(ts->p_singlecharsymbols, ts->current_char)
           && ts->current_char != EOF) {
        extend_buffer(buffer, buffer_max, p + len);
        ts->encode(ts->current_char, *buffer + p);
        p += len;
        ts_getc(ts);
        len = ts->charlen(ts->current_char);
    }
    (*buffer)[p] = '\0';
}

/* 0x546f0 arm: reads characters up to one in endclass (or a single-character symbol), at most
 * max_chars of them when positive. */
static void get_token_sub_part_2(cst_tokenstream *ts, const char *endclass, char **buffer,
                                 int *buffer_max, int max_chars)
{
    int p = 0, n = 0, len = ts->charlen(ts->current_char);

    while (!ts->member(endclass, ts->current_char)
           && !ts->member(ts->p_singlecharsymbols, ts->current_char)
           && !(n >= max_chars && max_chars > 0 && p != 0)
           && ts->current_char != EOF) {
        extend_buffer(buffer, buffer_max, p + len);
        ts->encode(ts->current_char, *buffer + p);
        n++;
        p += len;
        ts_getc(ts);
        len = ts->charlen(ts->current_char);
    }
    (*buffer)[p] = '\0';
}

/* 0x548fc arm: an ASCII token stream with Flite's default symbol sets. */
cst_tokenstream *new_tokenstream(void)
{
    cst_tokenstream *ts = cst_safe_alloc(1, sizeof(cst_tokenstream));

    ts->fd = NULL;
    ts->file_pos = 0;
    ts->line_number = 0;
    ts->string_buffer = NULL;
    ts->unget = NULL;
    ts->token_pos = 0;
    ts->whitespace = cst_safe_alloc(TS_BUFFER_SIZE + 1, 1);
    ts->ws_max = TS_BUFFER_SIZE;
    ts->prepunctuation = cst_safe_alloc(TS_BUFFER_SIZE + 1, 1);
    ts->prep_max = TS_BUFFER_SIZE;
    ts->token = cst_safe_alloc(TS_BUFFER_SIZE + 1, 1);
    ts->max_token_len = -1;
    ts->token_max = TS_BUFFER_SIZE;
    ts->postpunctuation = cst_safe_alloc(TS_BUFFER_SIZE + 1, 1);
    ts->postp_max = TS_BUFFER_SIZE;
    ts->tail = cst_safe_alloc(TS_BUFFER_SIZE + 1, 1);
    ts->tail_max = TS_BUFFER_SIZE;
    ts->current_char = 0;
    ts->p_whitespacesymbols = " \t\n\v\f\r\x85\xa0";
    ts->p_singlecharsymbols = "(){}[]";
    ts->p_prepunctuationsymbols = "\"'`({[";
    ts->p_postpunctuationsymbols = "\"'`.,:;!?(){}[]";
    ts->charlen = ascii_charlen;
    ts->member = (const char *(*)(const char *, int))strchr;
    ts->span = strspn;
    ts->decode = ascii_decode;
    ts->encode = ascii_encode;
    return ts;
}

/* 0x54a44 arm */
cst_tokenstream *new_tokenstream_utf8(void)
{
    cst_tokenstream *ts = new_tokenstream();

    /* Unicode white space */
    ts->p_whitespacesymbols = " \011\012\013\014\015\302\205\342\200\250\342\200\251 \302\240\341\232\200\341\240\216\342\200\200\342\200\201\342\200\202\342\200\203\342\200\204\342\200\205\342\200\206\342\200\207\342\200\210\342\200\211\342\200\212\342\200\257\342\201\237\343\200\200";
    ts->charlen = ts_utf8_charlen;
    ts->member = ts_utf8_strchr;
    ts->span = ts_utf8_strspn;
    ts->decode = ts_utf8_decode;
    ts->encode = ts_utf8_encode;
    return ts;
}

/* 0x54ab0 arm */
void delete_tokenstream(cst_tokenstream *ts)
{
    cst_free(ts->unget);
    cst_free(ts->whitespace);
    cst_free(ts->prepunctuation);
    cst_free(ts->token);
    cst_free(ts->postpunctuation);
    cst_free(ts->tail);
    cst_free(ts);
}

/* 0x54af4 arm */
cst_tokenstream *ts_open_fd(cst_file fd)
{
    cst_tokenstream *ts;

    if (fd == NULL)
        return NULL;
    ts = new_tokenstream();
    ts->fd = fd;
    ts_getc(ts);
    return ts;
}

/* 0x54bbc arm: "-" is stdin. */
cst_tokenstream *ts_open(const char *filename)
{
    cst_file fd;

    if (strcmp("-", filename) == 0)
        fd = stdin;
    else
        fd = cst_fopen(filename, CST_OPEN_READ | CST_OPEN_BINARY);
    if (fd == NULL)
        return NULL;
    return ts_open_fd(fd);
}

/* 0x54c10 arm */
cst_tokenstream *ts_open_string(const char *string)
{
    cst_tokenstream *ts = new_tokenstream();

    ts->string_buffer = (char *)cst_strdup((const cst_string *)string);
    ts_getc(ts);
    return ts;
}

/* 0x54d1c arm */
cst_tokenstream *ts_open_string_utf8(const char *string)
{
    cst_tokenstream *ts = new_tokenstream_utf8();

    ts->string_buffer = (char *)cst_strdup((const cst_string *)string);
    ts_getc(ts);
    return ts;
}

/* 0x54e28 arm */
void ts_close(cst_tokenstream *ts)
{
    if (ts->fd) {
        if (ts->fd != stdin)
            cst_fclose(ts->fd);
        ts->fd = NULL;
    }
    if (ts->string_buffer) {
        cst_free(ts->string_buffer);
        ts->string_buffer = NULL;
    }
    delete_tokenstream(ts);
}

/* 0x54e80 arm */
int ts_eof(cst_tokenstream *ts)
{
    return ts->current_char == EOF;
}

/* a token of only prepunctuation is the prepunctuation (up to the length limit) */
static inline void ts_prepunct_token(cst_tokenstream *ts)
{
    int n;

    if (ts->token[0] != '\0' || ts->prepunctuation[0] == '\0')
        return;
    n = strlen(ts->prepunctuation);
    if (ts->max_token_len > 0 && n >= ts->max_token_len)
        n = ts->max_token_len;
    extend_buffer(&ts->token, &ts->token_max, n);
    strncpy(ts->token, ts->prepunctuation, n);
    ts->token[n] = '\0';
    ts_unget(ts, ts->prepunctuation + n);
    ts->prepunctuation[0] = '\0';
}

/* 0x54e94 arm: the next token. Prepunctuation alone becomes the token; trailing
 * postpunctuation (up to white space) goes to ts->postpunctuation. */
const char *ts_get(cst_tokenstream *ts)
{
    int n, len, cut, total, c;
    const char *p;

    get_token_sub_part(ts, ts->p_whitespacesymbols, &ts->whitespace, &ts->ws_max);
    ts->token_pos = ts->file_pos
                  - (ts->current_char == EOF ? 1 : ts->charlen(ts->current_char));
    get_token_sub_part(ts, ts->p_prepunctuationsymbols, &ts->prepunctuation, &ts->prep_max);
    if (ts->member(ts->p_singlecharsymbols, ts->current_char)) {
        n = ts->charlen(ts->current_char);
        extend_buffer(&ts->token, &ts->token_max, n);
        ts->token[ts->encode(ts->current_char, ts->token)] = '\0';
        ts->current_char = ts_getc(ts);
    } else
        get_token_sub_part_2(ts, ts->p_whitespacesymbols, &ts->token, &ts->token_max,
                             ts->max_token_len);

    ts_prepunct_token(ts);
    get_token_sub_part_2(ts, ts->p_whitespacesymbols, &ts->tail, &ts->tail_max, -1);
    if (ts->span(ts->tail, ts->p_postpunctuationsymbols) == strlen(ts->tail)) {
        /* the token's trailing postpunctuation joins the tail */
        cut = 0;
        for (p = ts->token; (c = ts->decode(p, &len)) != 0; ) {
            p += len;
            if (!ts->member(ts->p_postpunctuationsymbols, c))
                cut = p - ts->token;
        }
        total = strlen(ts->token);
        if (total) {
            if (cut == 0) {
                ts->decode(ts->token, &len);
                cut = len;
            }
            total = total - cut + 1;
            extend_buffer(&ts->postpunctuation, &ts->postp_max, total);
            memmove(ts->postpunctuation, ts->token + cut, total);
            ts->token[cut] = '\0';
        }
        total += strlen(ts->tail);
        extend_buffer(&ts->postpunctuation, &ts->postp_max, total);
        strcat(ts->postpunctuation, ts->tail);
    } else
        ts_unget(ts, ts->tail);
    return ts->token;
}
