/*
 * UTF-8 string iterator (0x33e94..0x342e4). The iterator sits after its current
 * character: cur points past it, c and len describe it, index counts characters.
 */
#include <string.h>

int utf8_decode(const char *s, int *len);
const char *utf8_strchr(const char *s, int c);
size_t utf8_strlen(const char *s);

typedef struct utf8_iter {
    const char *start;
    const char *cur;
    int index;
    int len;
    int c;
} utf8_iter;

/* 0x33e94 arm: starts iterating s; whether it has any characters. */
int utf8_iter_init(utf8_iter *it, const char *s)
{
    memset(it, 0, sizeof *it);
    it->start = s;
    it->cur = s;
    return s != NULL && *s != 0;
}

/* 0x33ed4 arm: advances one character; returns its start, or NULL at the end. */
const char *utf8_iter_next(utf8_iter *it)
{
    const char *p;

    if (it->cur == NULL || *it->cur == 0)
        return NULL;
    it->c = utf8_decode(it->cur, &it->len);
    if (it->c == 0)
        return NULL;
    p = it->cur;
    it->cur = p + it->len;
    it->index++;
    return p;
}

/* 0x33f2c arm: steps back one character; returns the new position, or NULL at the start. */
const char *utf8_iter_prev(utf8_iter *it)
{
    const char *p, *cur;
    int index;

    if (it->cur == it->start)
        return NULL;
    cur = it->cur - it->len;
    index = it->index - 1;
    it->cur = cur;
    it->index = index;
    if (cur == it->start) {
        it->c = 0;
        it->len = 0;
        return cur;
    }
    p = it->cur - 1;
    while (p != it->start && (*(const unsigned char *)p & 0xc0) == 0x80)
        p--;
    it->c = utf8_decode(p, &it->len);
    return it->cur;
}

/* 0x33fdc arm: advances one character and returns it (0 at the end). */
int utf8_iter_next_char(utf8_iter *it)
{
    if (!utf8_iter_next(it))
        return 0;
    return it->c;
}

/* 0x33ff4 arm: starts iterating s and returns its first character. */
int utf8_iter_init_char(utf8_iter *it, const char *s)
{
    memset(it, 0, sizeof *it);
    it->start = s;
    it->cur = s;
    return utf8_iter_next_char(it);
}

/* 0x34020 arm: moves n characters (back if negative); returns how many it moved. */
int utf8_iter_move(utf8_iter *it, int n)
{
    const char *r;
    int i = 0;

    if (n > 0) {
        for (r = utf8_iter_next(it), i = 1; r && i < n; i++)
            r = utf8_iter_next(it);
        if (!r)
            i--;
    } else if (n < 0) {
        for (r = utf8_iter_prev(it), i = -1; r && i > n; i--)
            r = utf8_iter_prev(it);
        if (!r)
            i++;
    }
    return i;
}

/* 0x340ac arm: bytes left after the current character. */
size_t utf8_iter_bytes_left(utf8_iter *it)
{
    if (it->cur == NULL)
        return 0;
    return strlen(it->cur);
}

/* 0x340bc arm: characters left after the current character. */
size_t utf8_iter_chars_left(utf8_iter *it)
{
    if (it->cur == NULL)
        return 0;
    return utf8_strlen(it->cur);
}

/* 0x340cc arm: advances to the next character in set. */
const char *utf8_iter_find_any(utf8_iter *it, const char *set)
{
    const char *p;

    for (p = utf8_iter_next(it); p; p = utf8_iter_next(it))
        if (utf8_strchr(set, it->c))
            break;
    return p;
}

/* 0x3410c arm: advances to the next occurrence of c. */
const char *utf8_iter_find(utf8_iter *it, int c)
{
    const char *p;

    for (p = utf8_iter_next(it); p; p = utf8_iter_next(it))
        if (it->c == c)
            return p;
    return NULL;
}

/* 0x3415c arm: moves to the last occurrence of c (iterator unchanged at the end if none). */
const char *utf8_iter_find_last(utf8_iter *it, int c)
{
    utf8_iter save;
    const char *p, *last = NULL;

    for (p = utf8_iter_next(it); p; p = utf8_iter_next(it)) {
        if (it->c == c) {
            save = *it;
            last = p;
        }
    }
    if (last)
        *it = save;
    return last;
}

/* 0x341f8 arm: advances to the next character not in set. */
const char *utf8_iter_skip_any(utf8_iter *it, const char *set)
{
    const char *p;

    for (p = utf8_iter_next(it); p; p = utf8_iter_next(it))
        if (!utf8_strchr(set, it->c))
            break;
    return p;
}

/* 0x34238 arm: moves to character index; whether there is text after it. */
int utf8_iter_seek(utf8_iter *it, int index)
{
    int d = index - it->index;

    return utf8_iter_move(it, d) == d && it->cur != NULL && *it->cur != 0;
}

/* 0x3427c arm: moves to character index and returns the character after it. */
int utf8_iter_seek_char(utf8_iter *it, int index)
{
    if (!utf8_iter_seek(it, index))
        return 0;
    return utf8_iter_next_char(it);
}

/* 0x3429c arm: advances one character and copies its bytes to out; returns their count. */
int utf8_iter_copy_next(utf8_iter *it, char *out)
{
    const char *p = utf8_iter_next(it);
    int n = 0;

    if (p) {
        n = it->len;
        memcpy(out, p, n);
    }
    return n;
}

/* 0x342d0 arm: cuts the string at the iterator position. */
void utf8_iter_cut(utf8_iter *it)
{
    if (it->cur)
        *(char *)it->cur = 0;
}
