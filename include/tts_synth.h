/* Synthesis helpers shared by the streamer and the synthesis modules (synth.c). */
#ifndef TTS_SYNTH_H
#define TTS_SYNTH_H

#include "cst_item.h"
#include "cst_val.h"

/* a mark the backend attached to synthesized audio */
typedef struct audiotag {
    const char *name;       /* segment name */
    char *phone;            /* +0x04: owned */
    int sample;             /* +0x08: offset in the wave */
    unsigned int flags;     /* +0x0c: AUDIOTAG_* */
    cst_item *item;         /* +0x10: its segment/word item */
} audiotag;

#define AUDIOTAG_PHONE    0x01
#define AUDIOTAG_WORD     0x04
#define AUDIOTAG_WORD_END 0x10
#define AUDIOTAG_PAUSE    0x20

CST_VAL_USER_TYPE_DCLS(audiotag, audiotag)
audiotag *new_audiotag(void);
void delete_audiotag(audiotag *t);

#endif
