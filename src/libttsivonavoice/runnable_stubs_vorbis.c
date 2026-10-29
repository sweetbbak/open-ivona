/*
 * Vorbis (Tremor) unit decoders, vox codecs 8 and 12, for the runnable builds of both the engine
 * and the voice library (each embeds a copy of the vox codec code). Tremor is deferred: opening
 * such a voice fails. Android voices use SILK (codec 14). See docs/notes/runnable-engine.md.
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

