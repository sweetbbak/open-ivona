/* The engine instance and language descriptors (tts_api.c, voice.c). Field names are provisional. */
#ifndef TTS_INSTANCE_H
#define TTS_INSTANCE_H

#include <pthread.h>
#include "ivona_tts.h"
#include "ivona_vector.h"

typedef struct res_cache res_cache;

/* a language: entries of the "language_data" array exported by a language or voice library
 * (NULL-name terminated), copied per loaded language */
typedef struct tts_lang_desc {
    const char *name;
    const char *const *codes;         /* +0x04: language codes (NULL-terminated) */
    ivona_vector *parts;              /* +0x08: loaded data files (lang_part *, cache data) */
    void (*part_init)(void *part);    /* +0x0c */
    void (*part_fini)(void *part);    /* +0x10 */
} tts_lang_desc;

/* a loaded language data file (res_cache data) */
typedef struct lang_part {
    const char *name;
    void *lexicons;             /* +0x04: NULL-terminated lex_source chain (PRM lexicons) */
    int unk_8[2];
    unsigned char data[1];      /* +0x10: handed to the language/voice library */
} lang_part;

struct tts_instance_struct {
    int magic;                    /* TTS_INSTANCE_MAGIC */
    ivona_vector *resources;      /* +0x04: of tts_resource * */
    void *certificate;            /* +0x08 */
    ivona_vector *modules;        /* +0x0c */
    pthread_mutex_t *modules_lock;/* +0x10 */
    res_cache *lang_cache;        /* +0x14 */
    ivona_vector *languages;      /* +0x18: of tts_language * */
    tts_lang_desc *lang_table;    /* +0x1c: copies of every language descriptor (+1 spare) */
    pthread_mutex_t *lock_20;     /* +0x20 */
    ivona_vector *addons_list;    /* +0x24 */
    pthread_mutex_t *addons_lock; /* +0x28 */
};

#endif
