/*
 * IVONA engine: platform layer (Android build). Mutexes map to pthreads; SysV shared memory
 * does not exist on Android, so the shm_* entry points are stubs that report an error.
 * Names below 0x46d34 are descriptive guesses.
 */
#include <pthread.h>
#include <string.h>
#include "cst_alloc.h"
#include "tts_internal.h"

typedef struct ivona_time {
    unsigned int sec;
    unsigned int frac;
} ivona_time;

typedef struct ivona_shm {
    int id;
    void *ptr;
} ivona_shm;

/* 0x46cb8: -1, 0 or 1 as a is before, equal to or after b */
int ivona_time_cmp(const ivona_time *a, const ivona_time *b)
{
    /* NONMATCHING (56%): branch layout differs */
    if (a->sec < b->sec)
        return -1;
    if (a->sec == b->sec) {
        if (a->frac < b->frac)
            return -1;
        return a->frac > b->frac;
    }
    return 1;
}

/* 0x46cf8 */
pthread_mutex_t *ivona_mutex_new(void)
{
    pthread_mutex_t *m = cst_alloc(pthread_mutex_t, 1);

    pthread_mutex_init(m, NULL);
    return m;
}

/* 0x46d1c */
void ivona_mutex_free(pthread_mutex_t *m)
{
    pthread_mutex_destroy(m);
    cst_free(m);
}

/* 0x46d34 */
int ivona_mutex_lock(pthread_mutex_t *m)
{
    return pthread_mutex_lock(m);
}

/* 0x46d38: no trylock on this platform; blocks like ivona_mutex_lock */
int ivona_mutex_trylock(pthread_mutex_t *m)
{
    return pthread_mutex_lock(m);
}

/* 0x46d3c */
int ivona_mutex_unlock(pthread_mutex_t *m)
{
    return pthread_mutex_unlock(m);
}

/* 0x46d40: not available on Android (the allocation is leaked, as in the original) */
ivona_shm *ivona_shm_create(void)
{
    cst_calloc(ivona_shm, 1);
    tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "System function missing: shmget");
    return NULL;
}

/* 0x46d6c */
int ivona_shm_free(ivona_shm *shm)
{
    if (shm)
        cst_free(shm);
    return 1;
}

/* 0x46d84: not available on Android */
ivona_shm *ivona_shm_open(void)
{
    ivona_shm *shm = cst_calloc(ivona_shm, 1);

    tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "System function missing: shmget");
    ivona_shm_free(shm);
    return NULL;
}

/* 0x46dbc */
void *ivona_shm_ptr(const ivona_shm *shm)
{
    return shm->ptr;
}

/* 0x46dc4 */
int ivona_shm_clear(void *p, size_t n)
{
    memset(p, 0, n);
    return 1;
}

/* 0x46ddc */
int ivona_shm_lock(void)
{
    return 1;
}

/* 0x46de4 */
int ivona_shm_unlock(void)
{
    return 0;
}
