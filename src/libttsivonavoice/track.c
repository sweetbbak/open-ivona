/*
 * Unit audio tracks (0x640b0..0x64558): samples split into frames at pitchmarks.
 * Compiled without inlining (see config).
 */
#include <math.h>
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

float crossfade_weight(int i, int n);

/* 0x64558 arm: inserts len samples of silence (in 10 ms frames) before frame at, fading out
 * the frame before and fading in the frame after. */
void track_insert_silence(track *t, int at, unsigned int len)
{
    int fsz = t->sample_rate * 0.01;
    int n = len / fsz;
    int tail = t->num_frames - at;
    int i, k, flen;
    short *f;

    track_resize(t, t->num_frames + n);
    if (tail > 0) {
        memmove(&t->frames[n + at], &t->frames[at], tail * sizeof(short *));
        memmove(&t->pms[n + at], &t->pms[at], tail * sizeof(unsigned int));
    }
    for (i = 0; i < n; i++) {
        t->frames[at + i] = NULL;
        track_frame_alloc(t, at + i, fsz);
        memset(t->frames[at + i], 0, fsz * sizeof(short));
        t->pms[at + i] = track_frame_start(t, at + i) + fsz;
    }
    for (i = 0; i < tail; i++)
        t->pms[at + n + i] += n * fsz;
    if (at > 0) {
        flen = track_frame_len(t, at - 1);
        for (k = 0; k < flen; k++) {
            f = t->frames[at - 1];
            f[k] = f[k] * crossfade_weight(k, flen);
        }
    }
    if (tail > 0) {
        flen = track_frame_len(t, at + n);
        for (k = 0; k < flen; k++) {
            f = t->frames[at + n];
            f[k] = f[k] * crossfade_weight(flen - k, flen);
        }
    }
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
int wave_crossfade(short *dst, short n_dst, const short *src, short n_src)
{
    int n = n_src > n_dst ? n_dst : n_src;
    int i;

    for (i = 0; i < n; i++)
        dst[i] = (int)(crossfade_weight(i, n) * (dst[i] - src[n_src - n + i])) + src[n_src - n + i];
    return n;
}

/* 0x6487c arm: stretches frames from..to by ratio, overlap-adding each original frame into
 * new frames of ratio times its length (both ends when longer, spilling into the neighbours
 * when shorter); returns the ratio actually reached. */
float track_stretch(track *t, int from, int to, float ratio)
{
    track *nt;
    int i, k, j, d, f, len, olen, nlen;
    int total;
    float w;
    short *dst, *src;

    if (ratio == 1.0f)
        return ratio;
    nt = new_track();
    track_resize(nt, t->num_frames);
    total = 0;
    for (i = 0; i < t->num_frames; i++) {
        if (i >= from && i <= to) {
            len = track_frame_len(t, i) * ratio + 0.5;
            nt->frames[i] = cst_safe_alloc(len, sizeof(short));
            total += len;
        } else {
            len = track_frame_len(t, i);
            nt->frames[i] = t->frames[i];
            t->frames[i] = NULL;
        }
        nt->pms[i] = track_frame_start(nt, i) + len;
    }
    for (k = from; k <= to; k++) {
        olen = track_frame_len(t, k);
        nlen = track_frame_len(nt, k);
        if (ratio < 1.0f) {
            d = nlen - 1;
            f = k;
            for (i = olen - 1; i >= 0; i--, d--) {
                if (d < 0) {
                    if (--f < 0)
                        break;
                    d = track_frame_len(nt, f) - 1;
                }
                w = crossfade_weight(olen - 1 - i, olen);
                dst = nt->frames[f];
                dst[d] = dst[d] + t->frames[k][i] * w;
            }
            d = 0;
            f = k;
            for (j = 0; j < olen; j++) {
                if (d >= nlen) {
                    if (++f >= t->num_frames)
                        break;
                    nlen = track_frame_len(nt, f);
                    d = 0;
                }
                dst = nt->frames[f];
                dst[d] = dst[d] + t->frames[k][j] * crossfade_weight(j, olen);
                d++;
            }
        } else {
            for (j = 0; j < olen; j++) {
                w = crossfade_weight(j, olen);
                dst = nt->frames[k];
                src = t->frames[k];
                dst[j] = dst[j] + src[j] * w;
                dst[nlen - 1 - j] = dst[nlen - 1 - j] + src[olen - 1 - j] * w;
            }
        }
    }
    if (total == 0)
        w = 1.0f;
    else
        w = (float)total / (float)track_span(t, from, to);
    cst_free(t->pms);
    t->pms = nt->pms;
    for (k = from; k <= to; k++)
        cst_free(t->frames[k]);
    cst_free(t->frames);
    t->frames = nt->frames;
    cst_free(nt);
    return w;
}

/* 0x64d60 arm: scales samples from..to by gain, clipped to +-32766; returns gain. */
float track_gain(track *t, int from, int to, float gain)
{
    int a = track_frame_at(t, from), b = track_frame_at(t, to - 1);
    int i, j, s, e, start, len, v;
    short *f;

    for (i = a; i <= b; i++) {
        start = track_frame_start(t, i);
        len = track_frame_len(t, i);
        s = from >= start ? from - start : 0;
        e = to - start;
        if (e >= len)
            e = len;
        for (j = s; j < e; j++) {
            f = t->frames[i];
            v = f[j] * gain;
            if (v < -32766)
                f[j] = -32766;
            else
                f[j] = v >= 32766 ? 32766 : v;
        }
    }
    return gain;
}

/* k * ln 2 in Q15 for k = 14..0 */
static const int fx_ln2[15] = {
    317982, 295269, 272556, 249843, 227130, 204417, 181704, 158991,
    136278, 113565, 90852, 68139, 45426, 22713, 0,
};

/* Q15 natural log of x > 0: normalizes x into [0.5, 1) and divides out factors 1 + 2^-k. */
#define FX_LOG(y, x)                                                                \
    do {                                                                            \
        int k_, t_;                                                                 \
        for (k_ = 1; k_ < 32; k_++)                                                 \
            if ((x) < (1 << k_))                                                    \
                break;                                                              \
        if (k_ == 32)                                                               \
            (y) = 0;                                                                \
        else if (k_ <= 15) {                                                        \
            (x) <<= 15 - k_;                                                        \
            (y) = -fx_ln2[14 - (15 - k_)];                                          \
        } else {                                                                    \
            (x) >>= k_ - 15;                                                        \
            (y) = fx_ln2[14 - (k_ - 15)];                                           \
        }                                                                           \
        if ((t_ = (x) + ((x) >> 1)) < 0x8000) { (y) -= 13286; (x) = t_; }           \
        if ((t_ = (x) + ((x) >> 2)) < 0x8000) { (y) -= 7311; (x) = t_; }            \
        if ((t_ = (x) + ((x) >> 3)) < 0x8000) { (y) -= 3859; (x) = t_; }            \
        if ((t_ = (x) + ((x) >> 4)) < 0x8000) { (y) -= 1986; (x) = t_; }            \
        if ((t_ = (x) + ((x) >> 5)) < 0x8000) { (y) -= 1008; (x) = t_; }            \
        if ((t_ = (x) + ((x) >> 6)) < 0x8000) { (y) -= 508; (x) = t_; }             \
        if ((t_ = (x) + ((x) >> 7)) < 0x8000) { (y) -= 255; (x) = t_; }             \
        (y) = (y) - 0x8000 + (x);                                                  \
    } while (0)

/* Q15 exp of x. */
#define FX_EXP(y, x)                                                                \
    do {                                                                            \
        (y) = 0x8000;                                                               \
        if ((x) > 317982) { (x) -= 317982; (y) <<= 14; }                            \
        if ((x) > 295269) { (x) -= 295269; (y) <<= 13; }                            \
        if ((x) > 272556) { (x) -= 272556; (y) <<= 12; }                            \
        if ((x) > 249843) { (x) -= 249843; (y) <<= 11; }                            \
        if ((x) > 227130) { (x) -= 227130; (y) <<= 10; }                            \
        if ((x) > 204417) { (x) -= 204417; (y) <<= 9; }                             \
        if ((x) > 181704) { (x) -= 181704; (y) <<= 8; }                             \
        if ((x) > 158991) { (x) -= 158991; (y) <<= 7; }                             \
        if ((x) > 136278) { (x) -= 136278; (y) <<= 6; }                             \
        if ((x) > 113565) { (x) -= 113565; (y) <<= 5; }                             \
        if ((x) > 90852) { (x) -= 90852; (y) <<= 4; }                               \
        if ((x) > 68139) { (x) -= 68139; (y) <<= 3; }                               \
        if ((x) > 45426) { (x) -= 45426; (y) <<= 2; }                               \
        if ((x) > 22713) { (x) -= 22713; (y) <<= 1; }                               \
        if ((x) > 13286) { (x) -= 13286; (y) += (y) >> 1; }                         \
        if ((x) > 7311) { (x) -= 7311; (y) += (y) >> 2; }                           \
        if ((x) > 3859) { (x) -= 3859; (y) += (y) >> 3; }                           \
        if ((x) > 1986) { (x) -= 1986; (y) += (y) >> 4; }                           \
        if ((x) > 1008) { (x) -= 1008; (y) += (y) >> 5; }                           \
        if ((x) > 508) { (x) -= 508; (y) += (y) >> 6; }                             \
        if ((x) > 255) { (x) -= 255; (y) += (y) >> 7; }                             \
        (y) = ((long long)(y) * ((x) + 0x8000)) >> 15;                              \
    } while (0)

#define LOG_FLOOR (-339529) /* ~ ln 2^-15 */
#define LOG_KNEE (-15467)

/* 0x64e58 arm: compresses the dynamic range of samples from..to towards level thr: an envelope
 * follower (instant attack, 5 ms release) drives a gain curve built in the Q15 log domain;
 * returns thr. */
float track_compress(track *t, int from, int to, float thr)
{
    int q = thr * 32768.0f;
    int L, top, lo, rel, a, b, i, j, s0, e, start, len;
    int k1, k2, k3, k4;
    int env, v, lg, g, gain, out;
    short *p;

    FX_LOG(L, q);
    L /= 2;
    top = LOG_KNEE - L * 1.4;
    rel = 0x8000 / (t->sample_rate / 1000 * 5);
    k1 = ((long long)L << 15) / (top - LOG_FLOOR);
    k2 = ((long long)L * (int)(((long long)LOG_FLOOR << 15) / (top - LOG_FLOOR))) >> 15;
    k3 = ((long long)L << 15) / (top - LOG_KNEE);
    k4 = ((long long)L * (((long long)LOG_KNEE << 15) / (top - LOG_KNEE))) >> 15;
    a = track_frame_at(t, from);
    b = track_frame_at(t, to - 1);
    lo = top >= LOG_FLOOR ? top : LOG_FLOOR;
    env = 0;
    for (i = a; i <= b; i++) {
        start = track_frame_start(t, i);
        len = track_frame_len(t, i);
        s0 = from >= start ? from - start : 0;
        e = to - start;
        if (e >= len)
            e = len;
        for (j = s0; j < e; j++) {
            short s;

            p = &t->frames[i][j];
            s = *p;

            v = s < 0 ? -s : s;
            env += ((long long)(v - env) * (v > env ? 0x8000 : rel)) >> 15;
            if (env <= 0)
                continue;
            v = env;
            FX_LOG(lg, v);
            if (lo >= lg && lg >= LOG_FLOOR + 1)
                g = (((long long)lg * k1) >> 15) - k2;
            else if (lg < LOG_FLOOR && lg > lo)
                g = (((long long)lg * k3) >> 15) - k4;
            else
                g = 0;
            FX_EXP(gain, g);
            out = ((long long)s * gain) >> 15;
            if (out < -32766)
                *p = -32766;
            else
                *p = out >= 32766 ? 32766 : out;
        }
    }
    return thr;
}

/* 0x655e8 arm: sine waveshaping distortion of frames from..to, amount (<= 100) scaling the
 * modulation depth; returns the amount used. */
float track_distort(track *t, int from, int to, float amount)
{
    double k, a;
    int i, j;
    short *f;

    if (amount > 100.0f) {
        amount = 100.0f;
        k = 100.0 / 750.0;
    } else
        k = amount / 750.0;
    for (i = from; i <= to; i++) {
        for (j = 0; j < track_frame_len(t, i); j++) {
            f = t->frames[i];
            if (f[j] < 20478.75 && f[j] > -20478.75) {
                a = f[j] * M_PI_2 / 20478.75;
                f[j] = sin(a + sin(a * 4.0) * k) * 32766.0 / 1.6;
            }
        }
    }
    return amount;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ABS(a) ((a) < 0 ? -(a) : (a))

/* 0x65780 arm: retimes frames from..to by ratio, pitch-synchronously: each pass halves or
 * doubles at most, dropping (crossfading out) or duplicating frames wherever that moves the
 * running duration ratio closer to the pass's target; returns the ratio reached. */
float track_retime(track *t, int from, int to, float ratio)
{
    float total = 1.0f, cur, step, r, r_keep, fout, fin;
    int n, size, i, o, len, out, in, c;
    unsigned int *pms;
    short **frames;
    short *f;

    if (ratio == 1.0f)
        return total;
    n = t->num_frames;
    cur = total;
    for (;;) {
        if (ratio < 1.0f)
            step = ratio > cur * 0.5f ? ratio / cur : 0.5f;
        else if (ratio > 1.0f)
            step = ratio < cur + cur ? ratio / cur : 2.0f;
        else
            step = 1.0f;
        size = step < 1.0f ? n : to - from + 1 + n;
        pms = cst_safe_alloc(size, sizeof(unsigned int));
        frames = cst_safe_alloc(size, sizeof(short *));
        for (i = 0, o = 0; i < t->num_frames; o++) {
            frames[o] = t->frames[i];
            pms[o] = (o == 0 ? 0 : pms[o - 1]) + track_frame_len(t, i);
            if (i < from || !(i < to || (i == to && to < t->num_frames - 1))) {
                i++;
                continue;
            }
            len = track_frame_len(t, i);
            out = (o == 0 ? 0 : pms[o - 1]) - (from > 0 ? pms[from - 1] : 0);
            in = t->pms[i] - track_frame_start(t, from);
            fout = len + out;
            fin = in;
            r_keep = fout / fin;
            if (step < 1.0f) {
                r = (float)(out + MIN(len, track_frame_len(t, i + 1)))
                    / (in + track_frame_len(t, i + 1));
                if (fabsf(step - r) < fabsf(step - r_keep)) {
                    c = wave_crossfade(frames[o], len, t->frames[i + 1],
                                       track_frame_len(t, i + 1));
                    pms[o] = (o == 0 ? 0 : pms[o - 1]) + c;
                    cst_free(t->frames[i + 1]);
                    i += 2;
                } else
                    i++;
            } else if (step > 1.0f) {
                i++;
                r = (len + out + MIN(len, track_frame_len(t, i))) / fin;
                if (fabsf(step - r) < fabsf(step - r_keep)) {
                    f = track_frame_copy(t, i);
                    frames[o + 1] = f;
                    c = wave_crossfade(f, track_frame_len(t, i), frames[o], len);
                    pms[o + 1] = (o + 1 == 0 ? 0 : pms[o]) + c;
                    o++;
                }
            } else
                i++;
        }
        c = o + to - t->num_frames;
        if (c < 0)
            r = 0.0f;
        else
            r = (float)(pms[c] - (from > 0 ? pms[from - 1] : 0))
                / (float)(t->pms[to] - track_frame_start(t, from));
        cst_free(t->frames);
        t->frames = frames;
        cst_free(t->pms);
        t->pms = pms;
        t->num_frames = o;
        total *= r;
        cur *= step;
        to += o - n;
        if (ratio == cur)
            return total;
        n = o;
    }
}

/* 0x65d50 arm: track_retime over frames from..to with the ratio len / span, scaled back to
 * samples. */
unsigned int track_retime_to(track *t, int from, int to, unsigned int len)
{
    float span = (unsigned int)track_span(t, from, to);

    float r = track_retime(t, from, to, len / span);

    return span * r;
}

/* 0x65da8 arm: splits an unsegmented track (flag 1: lead-in, raw audio, tail) into pitch periods
 * found by AMDF: a coarse search on 1 ms averages, refined on the samples around its best lag. */
track *track_segment(track *t)
{
    int sr, ms, len, nd, i, j, k, pos, next, nf, plen, c, half, lag, idx;
    unsigned int pmin, pmax, lo, hi, sum, best, best_lag, max, max_lag, score, prev, peak;
    short *x, *d, *p;
    track *nt;

    if (t->flag != 1)
        return t;
    sr = t->sample_rate;
    ms = sr / 1000;
    pmin = sr / 300;
    pmax = sr / 50;
    len = t->pms[1] - t->pms[0];
    plen = t->pms[1];
    x = t->frames[1];
    nd = len / ms;
    d = cst_safe_alloc(nd, sizeof(short));
    for (k = 0, p = x; k < nd; k++) {
        sum = 0;
        for (j = 0; j < ms; j++)
            sum += *p++;
        d[k] = (int)sum / ms;
    }
    nt = new_track();
    nt->sample_rate = sr;
    nf = 0;
    if (len > 0) {
        half = ms / 2;
        pos = 0;
        peak = 0;
        prev = 0;
        for (;;) {
            if (nt->num_frames <= nf)
                track_resize(nt, nt->num_frames + (len - pos) * 100 / sr + 1);
            if (len < pos + (int)pmin) {
                score = prev;
                lag = len - pos;
            } else {
                c = pos / ms;
                lo = pmin / ms;
                hi = pmax / ms;
                if (nd < (int)(c + hi)) {
                    hi = nd - c;
                    if (lo >= hi)
                        lo = hi;
                }
                if (hi < lo) {
                    peak = 0;
                    best_lag = 0;
                } else {
                    best = 0;
                    best_lag = 0;
                    max = 0;
                    max_lag = 1;
                    for (lag = lo; lag <= hi; lag++) {
                        sum = 0;
                        for (j = 0; j < lag; j++) {
                            if (c + lag + j < nd)
                                idx = j + lag;
                            else {
                                idx = j - lag;
                                if (c + idx < 0)
                                    idx = j;
                            }
                            sum += ABS(d[c + j] - d[c + idx]);
                        }
                        if (best_lag == 0) {
                            best = sum;
                            best_lag = lag;
                        } else if (best_lag * sum < lag * best) {
                            best_lag = lag;
                            best = sum;
                        }
                        if (max * lag < max_lag * sum) {
                            max = sum;
                            max_lag = lag;
                        }
                    }
                    best_lag *= ms;
                    peak = max / max_lag;
                }
                hi = half + best_lag;
                lo = best_lag - half;
                if (len < (int)(pos + hi)) {
                    hi = len - pos;
                    if (lo >= hi)
                        lo = hi;
                }
                best = 0;
                best_lag = 0;
                if (hi >= lo) {
                    for (lag = lo; lag <= hi; lag++) {
                        sum = 0;
                        for (j = 0; j < lag; j++) {
                            if (pos + lag + j < len)
                                idx = j + lag;
                            else {
                                idx = j - lag;
                                if (pos + idx < 0)
                                    idx = j;
                            }
                            sum += ABS(x[pos + j] - x[pos + idx]);
                        }
                        if (best_lag == 0) {
                            best = sum;
                            best_lag = lag;
                        } else if (best_lag * sum < lag * best) {
                            best = sum;
                            best_lag = lag;
                        }
                    }
                }
                lag = best_lag;
                score = best / best_lag;
            }
            if (len < pos + lag)
                lag = len - pos;
            if (plen != 0 && 3 * prev < 2 * score && 3 * peak > score && pos + plen < len)
                next = pos + plen;
            else {
                next = lag + pos;
                plen = lag;
            }
            track_frame_alloc(nt, nf, plen);
            memcpy(nt->frames[nf], x + pos, plen * sizeof(short));
            nt->pms[nf] = (nf == 0 ? 0 : nt->pms[nf - 1]) + plen;
            nf++;
            if (len <= next)
                break;
            pos = next;
            prev = score;
        }
    }
    track_resize(nt, nf);
    cst_free(d);
    k = nt->num_frames;
    track_resize(t, k + 2);
    cst_free(t->frames[1]);
    t->frames[k + 1] = t->frames[2];
    t->pms[k + 1] = t->pms[2];
    for (i = 0; i < nt->num_frames; i++) {
        t->pms[i + 1] = nt->pms[i] + t->pms[0];
        t->frames[i + 1] = nt->frames[i];
        nt->frames[i] = NULL;
    }
    delete_track(nt);
    t->flag = 0;
    return t;
}
