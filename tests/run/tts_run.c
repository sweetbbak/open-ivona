/*
 * examples/example.c with arguments and WAV output, for running an engine under qemu
 * (tools/run_android.sh):
 *   tts_run CERT LIBVOICE VOXDB TEXT OUT.wav
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ivona_tts.h"

static void put32(FILE *f, unsigned int v)
{
    fputc(v & 0xff, f), fputc(v >> 8 & 0xff, f), fputc(v >> 16 & 0xff, f), fputc(v >> 24, f);
}

static void put16(FILE *f, unsigned int v)
{
    fputc(v & 0xff, f), fputc(v >> 8 & 0xff, f);
}

static void fail(const char *what)
{
    fprintf(stderr, "%s: %s (status %d)\n", what, tts_errmsg(), tts_errno());
    exit(1);
}

int main(int argc, char **argv)
{
    tts_instance *tts;
    tts_voice *voice;
    tts_streamer *st;
    tts_wave *w;
    FILE *out;
    unsigned int total = 0;
    int rate, chunk;

    if (argc != 6) {
        fprintf(stderr, "usage: %s CERT LIBVOICE VOXDB TEXT OUT.wav\n", argv[0]);
        return 2;
    }
    if ((tts = tts_create()) == NULL)
        fail("tts_create");
    if (!tts_load_certificate_file(tts, argv[1]))
        fail("tts_load_certificate_file");
    fprintf(stderr, "certificate loaded\n");
    if ((voice = tts_load_voice(tts, argv[2], argv[3])) == NULL)
        fail("tts_load_voice");
    rate = tts_voice_sample_rate(voice);
    fprintf(stderr, "voice loaded, %d Hz\n", rate);
    if ((st = tts_streamer_start(voice, argv[4], -1.0f)) == NULL)
        fail("tts_streamer_start");
    if ((out = fopen(argv[5], "wb")) == NULL) {
        perror(argv[5]);
        return 1;
    }
    fwrite("RIFF\0\0\0\0WAVEfmt ", 1, 16, out);
    put32(out, 16), put16(out, 1), put16(out, 1), put32(out, rate), put32(out, rate * 2);
    put16(out, 2), put16(out, 16);
    fwrite("data\0\0\0\0", 1, 8, out);
    chunk = (int)(rate * 0.1f);
    while ((w = tts_streamer_synth(st, chunk)) != NULL) {
        fwrite(w->samples, 2, w->num_samples, out);
        total += w->num_samples;
        tts_wave_delete(w);
    }
    if (tts_errno() != TTS_STATUS_OK)
        fail("tts_streamer_synth");
    fseek(out, 4, SEEK_SET), put32(out, 36 + total * 2);
    fseek(out, 40, SEEK_SET), put32(out, total * 2);
    fclose(out);
    fprintf(stderr, "%u samples written to %s\n", total, argv[5]);
    tts_streamer_stop(st);
    tts_unload_voice(voice);
    tts_destroy(tts);
    return 0;
}
