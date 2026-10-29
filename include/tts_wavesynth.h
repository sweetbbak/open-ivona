#ifndef TTS_WAVESYNTH_H
#define TTS_WAVESYNTH_H

#include "cst_val.h"
#include "cst_item.h"
#include "cst_wave.h"
#include "tts_track.h"
#include "tts_voxdb.h"

/* waveform synthesis state (wavesynth.c) */

/* a mark (0x14 bytes) */
typedef struct wavesynth_mark {
    cst_val *a;                 /* items, stored as vals */
    cst_val *b;
    int unit;                   /* +0x08 */
    int start;                  /* +0x0c */
    int end;                    /* +0x10 */
} wavesynth_mark;

typedef struct wavesynth {
    cst_item *word;             /* next word to build diphones for */
    cst_item *seg;              /* +0x04: next diphone to synthesize */
    cst_wave *wave;             /* +0x08 */
    int pos;                    /* +0x0c: samples written */
    int pending;                /* +0x10 */
    int total;                  /* +0x14 */
    cst_val *marks;             /* +0x18: userdata wavesynth_mark */
    unit_selector *selector;    /* +0x1c */
    dynamic_voxdbs *voxdbs;     /* +0x20 */
    cst_item *sel_first;        /* +0x24: first diphone of the running selection */
    unsigned char *cache_key;   /* +0x28: graph cache key of the running selection */
    unsigned int cache_key_len; /* +0x2c */
    int *path;                  /* +0x30: selected (checksum, unit) pairs so far */
    unsigned int path_len;      /* +0x34 */
} wavesynth;

CST_VAL_USER_TYPE_DCLS(wavesynth, wavesynth)

wavesynth *new_wavesynth(void);
int wavesynth_append(wavesynth *s, const short *samples, int n);
int wavesynth_flush(wavesynth *s);
int wavesynth_add_track(wavesynth *s, const track *t, int keep_last);
void wavesynth_add_mark(wavesynth *s, const cst_item *item, int start);
void wavesynth_add_unit_marks(wavesynth *s, const cst_item *item, int start, int end);
cst_wave *wavesynth_take(wavesynth *s, int max, cst_val **marks);

#endif
