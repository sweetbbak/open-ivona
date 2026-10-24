/*
 * Real-time pacing (0x483a8..0x484fc): holds synthesis back so that audio is produced no faster
 * than `speed` times real time.
 */
#include <stddef.h>
#include <sys/select.h>
#include "cst_alloc.h"
#include "ivona_util.h"

typedef struct rate_conv {
    float speed;
    unsigned int sample_rate;
    unsigned int samples;       /* produced since start */
    unsigned int start;         /* ivona_time_msec() of the first block, 0 before */
} rate_conv;

/* 0x483a8 arm */
rate_conv *rate_conv_new(float speed, unsigned int sample_rate)
{
    rate_conv *rc = cst_safe_calloc(1, sizeof(rate_conv));

    rc->speed = speed;
    rc->sample_rate = sample_rate;
    return rc;
}

/* 0x483cc arm */
void rate_conv_free(rate_conv *rc)
{
    if (rc)
        cst_free(rc);
}

/* 0x483d8 arm */
void rate_conv_reset(rate_conv *rc)
{
    if (rc)
        rc->start = ivona_time_msec();
}

/* 0x483f0 arm */
void ivona_sleep_ms(int ms)
{
    struct timeval tv;

    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    select(1, NULL, NULL, NULL, &tv);
}

/* 0x48448 arm: accounts for `samples` more samples and sleeps while ahead of the clock. */
void rate_conv_add(rate_conv *rc, unsigned int samples)
{
    unsigned int audio_ms;
    float elapsed;

    if (rc == NULL)
        return;
    if (rc->start == 0)
        rate_conv_reset(rc);
    rc->samples += samples;
    audio_ms = (float)rc->samples / (float)rc->sample_rate * 1000.0f;
    elapsed = (float)(ivona_time_msec() - rc->start) * rc->speed;
    if (elapsed < (float)audio_ms)
        ivona_sleep_ms((unsigned int)((float)audio_ms - elapsed));
}
