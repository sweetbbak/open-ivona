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
int track_span(const track *t, int from, int to);
int track_frame_at(const track *t, unsigned int pos);
void track_insert_silence(track *t, int at, unsigned int len);
int wave_crossfade(short *dst, short n_dst, const short *src, short n_src);
float track_stretch(track *t, int from, int to, float ratio);
float track_gain(track *t, int from, int to, float gain);
float track_compress(track *t, int from, int to, float thr);
float track_distort(track *t, int from, int to, float amount);
float track_retime(track *t, int from, int to, float ratio);
unsigned int track_retime_to(track *t, int from, int to, unsigned int len);
track *track_segment(track *t);

#endif
