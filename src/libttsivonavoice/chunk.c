/*
 * Synthesis chunks (0x21bb8..0x21d08): pieces of input, text or an audio clip, each with a copy
 * of the synthesis context it was produced in.
 */
#include "cst_alloc.h"
#include "cst_string.h"
#include "ivona_ustr.h"
#include "synth_chunk.h"

synth_ctx *synth_ctx_copy(const synth_ctx *c);
void synth_ctx_free(synth_ctx *c);


/* 0x21bb8 arm */
static void text_chunk_free_data(text_chunk *c)
{
    ivona_ustr_free(c->text);
}

/* 0x21bc0 arm */
static void audio_chunk_free_data(audio_clip_chunk *c)
{
    cst_free(c->src);
    cst_free(c->alt);
    cst_free(c->mark);
}

/* 0x21be4 arm */
text_chunk *text_chunk_new(const synth_ctx *ctx, const ivona_ustr *text, int flags)
{
    text_chunk *c = cst_safe_calloc(1, sizeof(text_chunk));

    c->c.type = CHUNK_TEXT;
    c->c.ctx = synth_ctx_copy(ctx);
    c->c.free_data = (void (*)(synth_chunk *))text_chunk_free_data;
    c->text = ustr_copy(text);
    c->flags = flags;
    return c;
}

/* 0x21c3c arm */
audio_clip_chunk *audio_clip_chunk_new(const synth_ctx *ctx, const char *src, const char *alt,
                                       const ustr_span *span, int flags, const char *mark)
{
    audio_clip_chunk *c = cst_safe_calloc(1, sizeof(audio_clip_chunk));

    c->c.type = CHUNK_AUDIO;
    c->c.ctx = synth_ctx_copy(ctx);
    c->c.free_data = (void (*)(synth_chunk *))audio_chunk_free_data;
    c->src = cst_strdup(src);
    c->alt = cst_strdup(alt);
    c->span = *span;
    c->flags = flags;
    c->mark = cst_strdup(mark);
    return c;
}

/* 0x21cc4 arm */
void synth_chunk_free(synth_chunk *c)
{
    synth_ctx_free(c->ctx);
    c->free_data(c);
    cst_free(c);
}

/* 0x21cec arm */
void flag_set(int *f)
{
    *f = 1;
}

/* 0x21cf8 arm */
void flag_clear(int *f)
{
    *f = 0;
}

/* 0x21d04 arm */
void flag_reset(int *f)
{
    flag_clear(f);
}
