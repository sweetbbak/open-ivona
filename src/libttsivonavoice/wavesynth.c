/*
 * Waveform synthesis state (0x62174..0x638e0): the output wave of an utterance, the
 * unit selector and the marks collected while concatenating. Compiled without inlining.
 */
#include <string.h>
#include "cst_alloc.h"
#include "cst_val.h"
#include "cst_val_defs.h"
#include "cst_wave.h"

typedef struct unit_selector unit_selector;
void unit_selector_free(unit_selector *s);
void *val_userdata(const cst_val *v);

/* a mark (0x14 bytes) */
typedef struct wavesynth_mark {
    int unk[5];
} wavesynth_mark;

typedef struct wavesynth {
    int unk_0;
    void *seg;                  /* +0x04 */
    cst_wave *wave;             /* +0x08 */
    int pos;                    /* +0x0c: samples written */
    int pending;                /* +0x10 */
    int total;                  /* +0x14 */
    cst_val *marks;             /* +0x18: userdata wavesynth_mark */
    unit_selector *selector;    /* +0x1c */
    int unk_20[6];
} wavesynth;

CST_VAL_USER_TYPE_DCLS(wavesynth, wavesynth)

/* 0x628e4 arm: speaking style by name (0: generic). */
int speaking_style_id(const char *name)
{
    if (name == NULL)
        return 0;
    if (strcmp(name, "ebook") == 0)
        return 2;
    if (strcmp(name, "news") == 0)
        return 1;
    if (strcmp(name, "speechcon") == 0)
        return 3;
    if (strcmp(name, "joke") == 0)
        return 4;
    return 0;
}

/* 0x62984 arm */
const char *speaking_style_name(int id)
{
    switch (id) {
    case 1:
        return "news";
    case 2:
        return "ebook";
    case 3:
        return "speechcon";
    case 4:
        return "joke";
    default:
        return "generic";
    }
}

/* 0x629f4 arm */
wavesynth_mark *wavesynth_mark_new(void)
{
    return cst_safe_alloc(1, sizeof(wavesynth_mark));
}

/* 0x62a00 arm */
void wavesynth_mark_free(wavesynth_mark *m)
{
    cst_free(m);
}

CST_VAL_REGISTER_TYPE_NODEL(wavesynth, wavesynth)

/* 0x62a74 arm */
wavesynth *new_wavesynth(void)
{
    wavesynth *s = cst_safe_calloc(1, sizeof(wavesynth));

    s->wave = new_wave();
    return s;
}

/* 0x62a98 arm */
void delete_wavesynth(wavesynth *s)
{
    const cst_val *l;

    if (s == NULL)
        return;
    if (s->selector)
        unit_selector_free(s->selector);
    if (s->wave)
        delete_wave(s->wave);
    if (s->marks) {
        for (l = s->marks; l; l = val_cdr(l))
            wavesynth_mark_free(val_userdata(val_car(l)));
        delete_val(s->marks);
    }
    cst_free(s);
}

/* 0x62b04 arm */
void val_delete_wavesynth(void *v)
{
    delete_wavesynth(v);
}

/* 0x62b08 arm: appends n samples (silence if samples is NULL). */
int wavesynth_append(wavesynth *s, const short *samples, int n)
{
    if (s->wave->num_samples - s->pos < n)
        cst_wave_resize(s->wave, s->wave->num_samples + n, 1);
    if (samples)
        memcpy(s->wave->samples + s->pos, samples, n * sizeof(short));
    else
        memset(s->wave->samples + s->pos, 0, n * sizeof(short));
    s->pos += n;
    s->total += n;
    s->pending = 0;
    return n;
}

/* 0x62b94 arm */
int wavesynth_flush(wavesynth *s)
{
    int n = s->pending;

    s->pos += n;
    s->total += n;
    s->pending = 0;
    return n;
}
