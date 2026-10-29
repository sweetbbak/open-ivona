/* Context-sensitive string rewrite rules (str_rewrite_span, 0x64624 in the voice library): the
 * X-SAMPA conversion tables of a language. */
#ifndef STR_REWRITE_H
#define STR_REWRITE_H

typedef struct str_rewrite_rule {
    const char *cond;               /* NULL or "": always; else compared with the caller's tag */
    const char *const *const *left; /* the left context, nearest first: per position a
                                       NULL-terminated set of alternatives (empty: any) */
    const char *match;              /* the text replaced, e.g. "[aa]" */
    const char *const *const *right;/* the right context, likewise */
    const char *out;                /* its replacement */
} str_rewrite_rule;                 /* tables end in a row whose left is NULL */

#endif
