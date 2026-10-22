/* Diagnostic message lists (0x4983c..0x49970): singly linked, with source position and code. */
#include <stdarg.h>
#include <string.h>
#include "cst_alloc.h"
#include "ivona_util.h"

char *cst_vasprintf(const char *fmt, va_list ap);

/* 0x4983c arm */
tts_msg *tts_msg_new(int line, int column, int code, const char *fmt, ...)
{
    tts_msg *m = cst_safe_alloc(1, sizeof(tts_msg));
    va_list ap;

    m->line = line;
    m->column = column;
    m->code = code;
    va_start(ap, fmt);
    m->text = cst_vasprintf(fmt, ap);
    va_end(ap);
    m->next = NULL;
    return m;
}

/* 0x498a4 arm: frees the whole list. */
void tts_msg_free(tts_msg *m)
{
    if (m == NULL)
        return;
    cst_free(m->text);
    tts_msg_free(m->next);
    cst_free(m);
}

/* 0x498cc arm */
tts_msg *tts_msg_last(tts_msg *m)
{
    if (m == NULL)
        return NULL;
    while (m->next)
        m = m->next;
    return m;
}

/* 0x498e8 arm: all message texts, one per line (newly allocated). */
char *tts_msg_join(tts_msg *m)
{
    strbuf b;

    strbuf_init(&b);
    for (; m; m = m->next) {
        char *t = m->text;
        if (t == NULL)
            continue;
        if (b.len)
            strbuf_append(&b, "\n", 1);
        strbuf_append_str(&b, m->text);
    }
    return strbuf_data(&b);
}
