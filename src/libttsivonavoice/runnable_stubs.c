/*
 * Stand-ins for engine functions not decompiled yet, so tools/build_engine.py can link a runnable
 * engine. Not part of the matching build: no function row credits this file, and each stub goes
 * when the real function is written. The list is in docs/notes/runnable-engine.md.
 */
#include <stdio.h>
#include "tts_track.h"

typedef struct vox_decoder vox_decoder;

/* Vorbis (Tremor) unit decoders, vox codecs 8 and 12 (the Windows IVONA 2 voices; Android voices
 * use SILK, codec 14). Tremor is deferred: opening such a voice fails. */
static int vorbis_unsupported(void)
{
    fprintf(stderr, "ivona: Vorbis vox databases (codec 8/12) are not supported yet\n");
    return 0;
}

int vorbis_unit_open(vox_decoder *d)
{
    (void)d;
    return vorbis_unsupported();
}

void vorbis_unit_close(vox_decoder *d)
{
    (void)d;
}

track *vorbis_unit_decode(vox_decoder *d, const unsigned char *data, unsigned int size)
{
    (void)d;
    (void)data;
    (void)size;
    return NULL;
}

int vorbis_ogg_unit_open(vox_decoder *d)
{
    (void)d;
    return vorbis_unsupported();
}

void vorbis_ogg_unit_close(vox_decoder *d)
{
    (void)d;
}

int vorbis_ogg_unit_decode(vox_decoder *d, const unsigned char *data, unsigned int size,
                           int n_packets, short *out, int n_samples)
{
    (void)d;
    (void)data;
    (void)size;
    (void)n_packets;
    (void)out;
    (void)n_samples;
    return -1;
}

/* SSML-only functions, unreachable while the runnable build has SSML input off
 * (IVONA_CERT_SSML=0, docs/notes/ssml-deferred.md): plain text never has <audio> or <phoneme>
 * elements, lexicon rules in SSML form are skipped, and input is never sniffed for SSML.
 * Reaching one means SSML was enabled before these were written. */
#include <stdlib.h>

static void ssml_unavailable(const char *name)
{
    fprintf(stderr, "ivona: %s is not decompiled yet (SSML input, see ssml-deferred.md)\n", name);
    abort();
}

void ssml_audio_apply_attrs(void *doc, void *e, void *w)
{
    (void)doc;
    (void)e;
    (void)w;
    ssml_unavailable("ssml_audio_apply_attrs");
}

int phoneme_alphabet_id(const char *alphabet, const char *lang)
{
    (void)alphabet;
    (void)lang;
    ssml_unavailable("phoneme_alphabet_id");
    return 0;
}

void *phoneme_convert(int alphabet, const char *ph)
{
    (void)alphabet;
    (void)ph;
    ssml_unavailable("phoneme_convert");
    return NULL;
}

void *lex_apply_ssml(void *rules, void *data, const char *src, const void *text, void *a,
                     void *b, void *c, void *ap)
{
    (void)rules;
    (void)data;
    (void)src;
    (void)text;
    (void)a;
    (void)b;
    (void)c;
    (void)ap;
    ssml_unavailable("lex_apply_ssml");
    return NULL;
}

int text_is_ssml(const char *s, int len)
{
    (void)s;
    (void)len;
    ssml_unavailable("text_is_ssml");
    return 0;
}
