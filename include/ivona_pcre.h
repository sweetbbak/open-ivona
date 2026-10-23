/* The parts of PCRE 8.37's API the engine uses (statically linked, verified as lib:pcre). */
#ifndef IVONA_PCRE_H
#define IVONA_PCRE_H

typedef struct real_pcre pcre;
typedef struct pcre_extra pcre_extra;

#define PCRE_CASELESS 0x00000001
#define PCRE_UTF8 0x00000800
#define PCRE_NO_UTF8_CHECK 0x00002000
#define PCRE_UCP 0x20000000
#define PCRE_STUDY_JIT_COMPILE 0x0001

extern void (*pcre_free)(void *);
pcre *pcre_compile(const char *pattern, int options, const char **errptr, int *erroffset,
                   const unsigned char *tableptr);
pcre_extra *pcre_study(const pcre *code, int options, const char **errptr);
void pcre_free_study(pcre_extra *extra);
int pcre_exec(const pcre *code, const pcre_extra *extra, const char *subject, int length,
              int startoffset, int options, int *ovector, int ovecsize);

#endif
