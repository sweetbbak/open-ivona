/* The voice library's English language module (en_lang.c and friends). */
#ifndef EN_LANG_H
#define EN_LANG_H

#include "cst_features.h"

/* what a language's init/fini (tts_lang_desc part_init/part_fini) receive */
typedef struct en_lang_core {
    const char *code;           /* the language code, e.g. "en-GB" */
    unsigned char *lex_data;    /* +0x04: the main lexicon's entries (from the language file) */
    int lex_num_bytes;          /* +0x08 */
    char pad_0c[8];
    cst_features *features;     /* +0x14 */
    cst_features *params;       /* +0x18: feature functions */
} en_lang_core;

#endif
