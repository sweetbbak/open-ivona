/*
 * Engine error state: the last error code plus an optional formatted message, as reported by
 * tts_errno() / tts_errmsg(). Not thread-safe: a single process-wide record.
 */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "tts_internal.h"

static struct {
    enum tts_status code;
    char msg[256];  /* empty: use the generic text for `code` */
} g_tts_error;

/* 0x394b0 */
const char *tts_vendor_tag(void)
{
    return "IVONA Text-To-Speech\nivona-1.6.101.3\nApril 2017\n";
}

/* 0x394c0 */
enum tts_status tts_errno(void)
{
    return g_tts_error.code;
}

/* 0x394d0: custom message if one was set, otherwise a generic text for the code. */
const char *tts_errmsg(void)
{
    if (g_tts_error.msg[0] != '\0')
        return g_tts_error.msg;
    switch (g_tts_error.code) {
    case TTS_STATUS_OK: return "Success";
    case TTS_STATUS_INVALID_CERTIFICATE: return "Invalid certificate of authenticity";
    case TTS_STATUS_CERTIFICATE_EXPIRED: return "The certificate of authenticity has expired";
    case TTS_STATUS_CERTIFICATE_LIMIT: return "The certificate of authenticity constraints would be violated";
    case TTS_STATUS_LOAD_VOICE_ERROR: return "Failed to load voice";
    case TTS_STATUS_FORMAT_ERROR: return "Format error";
    case TTS_STATUS_RANGE_ERROR: return "Argument out of range";
    case TTS_STATUS_USER_ABORTED: return "Aborted by client";
    case TTS_STATUS_INVALID_ARGUMENT: return "Invalid argument";
    case TTS_STATUS_SYSTEM_ERROR: return "Operating system error";
    case TTS_STATUS_INTERNAL_ERROR: return "Internal error";
    default: return "Unknown error";
    }
}

/* 0x395ec: set an error code with the generic message. */
void tts_set_error(enum tts_status code)
{
    g_tts_error.code = code;
    g_tts_error.msg[0] = '\0';
}

/* 0x39608: set an error code with a printf-style message; long messages end in "...". */
void tts_set_errorf(enum tts_status code, const char *fmt, ...)
{
    va_list ap;

    g_tts_error.code = code;
    va_start(ap, fmt);
    if ((size_t)vsnprintf(g_tts_error.msg, sizeof g_tts_error.msg, fmt, ap) >= sizeof g_tts_error.msg)
        memcpy(&g_tts_error.msg[sizeof g_tts_error.msg - 4], "...", 3);
    va_end(ap);
}

/* 0x3967c: TTS_STATUS_SYSTEM_ERROR from an errno value, optionally naming the failed call. */
void tts_set_error_errno(int err, const char *func)
{
    char buf[256];

    strerror_r(err, buf, sizeof buf);
    if (func != NULL)
        tts_set_errorf(TTS_STATUS_SYSTEM_ERROR, "%s() failed: %s", func, buf);
    else
        tts_set_errorf(TTS_STATUS_SYSTEM_ERROR, "%s", buf);
}

/* 0x396e0: change the error code but keep the current message text. */
void tts_set_error_code(enum tts_status code)
{
    char buf[256];

    strncpy(buf, tts_errmsg(), sizeof buf);
    tts_set_errorf(code, "%s", buf);
}

/* 0x39720: new code; message is "<formatted context>: <previous message>". */
void tts_set_error_contextf(enum tts_status code, const char *fmt, ...)
{
    char ctx[256];
    char prev[256];
    va_list ap;

    strncpy(prev, tts_errmsg(), sizeof prev);
    va_start(ap, fmt);
    vsnprintf(ctx, sizeof ctx, fmt, ap);
    va_end(ap);
    tts_set_errorf(code, "%s: %s", ctx, prev);
}
