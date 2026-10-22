/* IVONA's Android logging helper (also linked into the engine). */
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <android/log.h>

/* 0x18dec: __android_log_vprint with "(tid) " prefixed; the format is the first variadic
 * argument. */
void android_syslog(int prio, const char *tag, ...)
{
    char buf[512];
    va_list ap;

    va_start(ap, tag);
    snprintf(buf, sizeof(buf), "(%d) %s", gettid(), va_arg(ap, const char *));
    __android_log_vprint(prio, tag, buf, ap);
    va_end(ap);
}
