/* Flite cst_error.c, Android variant: messages go to logcat with tag "ivonatts". */
#include <stdarg.h>
#include <android/log.h>
#include "cst_error.h"

static int cst_log_level = 2;  /* 0x275248; set but never read in this build */

/* 0x519f8 */
int cst_errmsg(const char *fmt, ...)
{
    va_list args;
    int rv;

    va_start(args, fmt);
    rv = __android_log_vprint(ANDROID_LOG_VERBOSE, "ivonatts", fmt, args);
    va_end(args);
    return rv;
}

/* 0x51a38 */
void cst_set_log_level(int level)
{
    cst_log_level = level;
}
