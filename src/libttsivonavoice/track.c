/*
 * Unit audio tracks (0x640b0..0x64558): samples split into frames at pitchmarks.
 * Compiled without inlining (see config).
 */
#include <stdio.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_val.h"
#include "cst_val_defs.h"
#include "tts_track.h"

CST_VAL_REGISTER_TYPE_NODEL(track, track)

/* 0x64120 arm */
track *new_track(void)
{
    track *t = cst_safe_alloc(1, sizeof(track));

    t->num_frames = 0;
    t->flag = 0;
    return t;
}

/* 0x64140 arm */
void delete_track(track *t)
{
    int i;

    if (t == NULL)
        return;
    for (i = 0; i < t->num_frames; i++)
        cst_free(t->frames[i]);
    cst_free(t->frames);
    cst_free(t->pms);
    cst_free(t);
}

/* 0x64194 arm */
void val_delete_track(void *v)
{
    delete_track(v);
}

/* 0x64198 arm: resizes to n frames, keeping the first ones. */
void track_resize(track *t, int n)
{
    void *p;

    p = cst_safe_calloc(n, sizeof(unsigned int));
    if (t->num_frames > 0) {
        memmove(p, t->pms, (t->num_frames < n ? t->num_frames : n) * sizeof(unsigned int));
        cst_free(t->pms);
    }
    t->pms = p;
    p = cst_safe_calloc(n, sizeof(short *));
    if (t->num_frames > 0) {
        memmove(p, t->frames, (t->num_frames < n ? t->num_frames : n) * sizeof(short *));
        cst_free(t->frames);
    }
    t->frames = p;
    t->num_frames = n;
}

/* 0x64224 arm */
void track_frame_alloc(track *t, int i, int len)
{
    t->frames[i] = cst_safe_realloc(t->frames[i], len, sizeof(short));
}

/* 0x64248 arm: first sample of frame i. */
int track_frame_start(const track *t, int i)
{
    if (i == 0)
        return 0;
    return t->pms[i - 1];
}

/* 0x64260 arm: samples in frame i. */
int track_frame_len(const track *t, int i)
{
    if (i == 0)
        return t->pms[0];
    return t->pms[i] - t->pms[i - 1];
}

/* 0x64280 arm */
track *copy_track(const track *t)
{
    track *c = new_track();
    int i, len;

    c->sample_rate = t->sample_rate;
    track_resize(c, t->num_frames);
    for (i = 0; i < t->num_frames; i++) {
        len = track_frame_len(t, i);
        c->frames[i] = cst_safe_alloc(len, sizeof(short));
        memcpy(c->frames[i], t->frames[i], len * sizeof(short));
        c->pms[i] = t->pms[i];
    }
    c->flag = t->flag;
    return c;
}

/* 0x64318 arm: a new track of frames from..to. */
track *track_sub(const track *t, int from, int to)
{
    int n = to - from, i, len;
    track *c = new_track();

    c->sample_rate = t->sample_rate;
    c->flag = t->flag;
    track_resize(c, n + 1);
    for (i = 0; i <= n; i++) {
        len = track_frame_len(t, from + i);
        c->frames[i] = cst_safe_alloc(len, sizeof(short));
        memcpy(c->frames[i], t->frames[from + i], len * sizeof(short));
        c->pms[i] = t->pms[from + i] - t->pms[from];
    }
    return c;
}

/* 0x643d4 arm: a new copy of frame i's samples. */
short *track_frame_copy(const track *t, int i)
{
    short *f = t->frames[i];
    int len;

    if (f) {
        len = track_frame_len(t, i);
        f = cst_safe_alloc(len, sizeof(short));
        memcpy(f, t->frames[i], len * sizeof(short));
    }
    return f;
}

/* 0x6441c arm: samples from the start of frame from to the end of frame to. */
int track_span(const track *t, int from, int to)
{
    return t->pms[to] - track_frame_start(t, from);
}

/* 0x64434 arm: the frame holding sample pos, or -1. */
int track_frame_at(const track *t, unsigned int pos)
{
    int i;

    if (pos == 0)
        return 0;
    for (i = 0; i < t->num_frames; i++)
        if (pos < t->pms[i])
            return i;
    return -1;
}

/* 0x644a0 arm */
void track_print(const track *t)
{
    int i;

    printf("num_frames = %d\n", t->num_frames);
    printf("sample_rate = %d\n", t->sample_rate);
    printf("pms = { ");
    for (i = 0; i < t->num_frames; i++)
        printf("%d%s", t->pms[i], i < t->num_frames - 1 ? ", " : " ");
    puts("}");
}

/* Raised-cosine fade-out sampled at 16 points, and the step to the next point. */
static const float crossfade_curve[17] = {
    1.0f, 0.99039f, 0.96194f, 0.91573f, 0.85355f, 0.77779f, 0.69134f, 0.59755f, 0.5f,
    0.40245f, 0.30866f, 0.22221f, 0.14645f, 0.08427f, 0.03806f, 0.00961f, 0.0f,
};
static const float crossfade_step[17] = {
    -0.00961f, -0.02845f, -0.04621f, -0.06218f, -0.07576f, -0.08645f, -0.09379f, -0.09755f,
    -0.09755f, -0.09379f, -0.08645f, -0.07576f, -0.06218f, -0.04621f, -0.02845f, -0.00961f, -0.0f,
};

/* 0x64798 arm: fade-out weight of sample i of n, interpolated from crossfade_curve. */
float crossfade_weight(int i, int n)
{
    float x = (float)(i * 16) / n;
    int k = (int)x;

    return crossfade_curve[k] + (x - k) * crossfade_step[k];
}

/* 0x64800 arm: crossfades the start of dst into the end of src over min(n_dst, n_src) samples;
 * returns that count. */
int wave_crossfade(short *dst, int n_dst, const short *src, int n_src)
{
    int n = n_src > n_dst ? n_dst : n_src;
    int i;

    for (i = 0; i < n; i++)
        dst[i] = (int)(crossfade_weight(i, n) * (dst[i] - src[n_src - n + i])) + src[n_src - n + i];
    return n;
}
