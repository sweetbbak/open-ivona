/* Synthesis chunks (chunk.c) */
#ifndef SYNTH_CHUNK_H
#define SYNTH_CHUNK_H

#include "ivona_ustr.h"

typedef struct synth_ctx synth_ctx;

enum { CHUNK_TEXT, CHUNK_AUDIO };

typedef struct synth_chunk synth_chunk;
struct synth_chunk {
    int type;
    synth_ctx *ctx;
    void (*free_data)(synth_chunk *c);
};

typedef struct text_chunk {
    synth_chunk c;
    ivona_ustr *text;           /* +0x0c */
    int flags;                  /* +0x10: the offset of the text still to read */
} text_chunk;

typedef struct audio_clip_chunk {
    synth_chunk c;
    char *src;                  /* +0x0c: what to say (phones, ruby, ...) */
    char *alt;                  /* +0x10: the text it stands for */
    ustr_span span;             /* +0x14 */
    int flags;                  /* +0x1c: 1 phones, 5 phones in alphabet mark, 6 ruby */
    char *mark;                 /* +0x20: the alphabet */
} audio_clip_chunk;

#endif
