/* SSML say-as interpretations (say_as.c) */
#ifndef SSML_SAY_AS_H
#define SSML_SAY_AS_H

#include <stddef.h>
#include "dom.h"

/* a say-as interpretation */
typedef struct say_as_info {
    const char *name;
    const char *format;         /* date/time/duration field order, ... */
    int whole;                  /* replaces the whole text (expletive, interjection, ...) */
} say_as_info;

typedef struct say_as_ext say_as_ext;

say_as_ext *say_as_ext_find(const say_as_info *sa);
const say_as_info *say_as_lookup(dom_elem *e);
int ssml_date_parse(const char *s, char **ymd);
void ssml_date_join(const char *y, const char *m, const char *d, char *out, char *order);
int ssml_time_expand(const char *s, size_t len, char *out);

#endif
