/* Loaded voices (voice.c): what tts_voice.voice_feats points to. */
#ifndef TTS_VOICE_H
#define TTS_VOICE_H

#include "cst_features.h"
#include "ivona_vector.h"
#include "tts_instance.h"

/* returned by a voice library's register_voice() */
typedef struct voice_reg {
    cst_features *features;
    void *lang_data;        /* +0x04: the language's core data part (voice_reg_set_language) */
} voice_reg;

/* tts_voice.voice_feats */
typedef struct voice_data {
    voice_reg *reg;
    tts_lang_desc *lang;    /* language data loaded from the voice library, NULL if shared */
    void *vox;              /* voice data file (instance cache) */
    ivona_vector *pls;      /* of pls_entry */
    void *unk_10;
    cst_features *features;
} voice_data;

#define VOICE_DATA(v) ((voice_data *)(v)->voice_feats)
#define VOICE_FEATURES(v) (VOICE_DATA(v)->reg->features)

#endif
