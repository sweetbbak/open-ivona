/* PCRE helpers (0x64b30..): matching precompiled patterns (stored as "ERCP" blobs). */
#include <string.h>

int pcre_exec(const void *re, const void *extra, const char *subject, int length,
              int startoffset, int options, int *ovector, int ovecsize);

/* 0x64b84 arm: whether the compiled pattern re matches s */
int pcre_match(const void *re, const char *s)
{
    return pcre_exec(re, NULL, s, strlen(s), 0, 0, NULL, 0) >= 0;
}
