/*
 * examples/example.c with arguments and WAV output, for running an engine under qemu
 * (tools/run_android.sh):
 *   tts_run [OPTIONS] CERT LIBVOICE VOXDB TEXT OUT.wav
 * Options (voice parameters, set with tts_voice_param before synthesis):
 *   --rate F     speaking rate factor, 1 normal, about 0.67 (slow) .. 4 (fast)  "rate_factor"
 *   --pitch F    pitch factor, 0.5 .. 2                                          "pitch_factor"
 *   --volume F   volume factor, 0 .. 1.6                                         "volume_factor"
 *   --speed F    tts_streamer_start's speed: how fast audio is delivered, not how it sounds
 *                (1 real time, higher faster, -1 as fast as possible, the default)
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

/* sets a float voice parameter, e.g. "rate_factor" */
static void set_param(tts_voice *voice, const char *name, float value)
{
    char cmd[64];

    snprintf(cmd, sizeof cmd, "set:%s", name);
    if (!tts_voice_param(voice, cmd, &value)) {
        fprintf(stderr, "%s = %g: %s\n", name, value, tts_errmsg());
        exit(1);
    }
    fprintf(stderr, "%s = %g\n", name, value);
}

int main(int argc, char **argv)
{
    const char *names[] = { "--rate", "rate_factor", "--pitch", "pitch_factor",
                            "--volume", "volume_factor", NULL };
    const char *params[8];
    float values[8], speed = -1.0f;
    int nparams = 0, i;
    tts_instance *tts;
    tts_voice *voice;
    tts_streamer *st;
    tts_wave *w;
    FILE *out;
    unsigned int total = 0;
    int rate, chunk;

    while (argc > 2 && strncmp(argv[1], "--", 2) == 0) {
        if (strcmp(argv[1], "--speed") == 0)
            speed = (float)atof(argv[2]);
        else {
            for (i = 0; names[i] && strcmp(names[i], argv[1]) != 0; i += 2)
                ;
            if (names[i] == NULL) {
                fprintf(stderr, "unknown option %s\n", argv[1]);
                return 2;
            }
            params[nparams] = names[i + 1];
            values[nparams++] = (float)atof(argv[2]);
        }
        argc -= 2;
        argv += 2;
    }
    if (argc != 6) {
        fprintf(stderr, "usage: tts_run [--rate F] [--pitch F] [--volume F] [--speed F] "
                        "CERT LIBVOICE VOXDB TEXT OUT.wav\n");
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
    for (i = 0; i < nparams; i++)
        set_param(voice, params[i], values[i]);
    if ((st = tts_streamer_start(voice, argv[4], speed)) == NULL)
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
