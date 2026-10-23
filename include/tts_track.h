#ifndef TTS_TRACK_H
#define TTS_TRACK_H

#include "cst_val.h"

/* unit audio split at pitchmarks (track.c) */
typedef struct track {
    int flag;
    int sample_rate;            /* +0x04 */
    int num_frames;             /* +0x08 */
    unsigned int *pms;          /* +0x0c: pitchmarks (cumulative sample counts) */
    short **frames;             /* +0x10 */
} track;

CST_VAL_USER_TYPE_DCLS(track, track)

track *new_track(void);
void delete_track(track *t);
void track_resize(track *t, int n);
void track_frame_alloc(track *t, int i, int len);
int track_frame_start(const track *t, int i);
int track_frame_len(const track *t, int i);
track *copy_track(const track *t);
track *track_sub(const track *t, int from, int to);

#endif
