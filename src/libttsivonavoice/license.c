/*
 * Concurrent thread licensing (0x47bcc..0x483a8). Every process holding a licensed certificate
 * owns a slot in a system-wide table in shared memory, guarded by a named semaphore. Slots of
 * the same key are summed to enforce the certificate's thread limit; slots of dead processes
 * are reclaimed. (On Android the shared memory calls are stubs, so license_new always fails.)
 */
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include "cst_alloc.h"
#include "tts_internal.h"

typedef struct ivona_sem ivona_sem;
ivona_sem *ivona_sem_open(void);
int ivona_sem_close(ivona_sem *s);
int ivona_sem_wait(ivona_sem *s);
int ivona_sem_post(ivona_sem *s);
int ivona_sem_unlink(void);

typedef struct ivona_shm ivona_shm;
ivona_shm *ivona_shm_create(int key);
ivona_shm *ivona_shm_open(int key, size_t size, int (*init)(void *p, size_t n));
int ivona_shm_free(ivona_shm *shm);
void *ivona_shm_ptr(const ivona_shm *shm);
int ivona_shm_clear(void *p, size_t n);
int ivona_shm_lock(ivona_shm *shm);
int ivona_shm_unlock(ivona_shm *shm);

#define LICENSE_SHM_KEY 0x6da556e8
#define LICENSE_SLOTS 0x2aa

typedef struct license_slot {
    int key;            /* certificate id */
    pid_t pid;          /* 0 for a free slot */
    unsigned int count; /* threads in use */
} license_slot;

typedef struct license {
    license_slot *slots;        /* the shared table */
    ivona_shm *shm;
    pthread_mutex_t mutex;
    ivona_sem *sem;
    int slot;                   /* +0x10: our slot */
    license_slot self;          /* +0x14: what our slot should hold */
} license;

/* 0x47bcc arm: takes a free (or dead process's) slot. */
int license_attach(license *l)
{
    license_slot *s;
    int i;

    if (!ivona_sem_wait(l->sem))
        return 0;
    for (i = 0; i < LICENSE_SLOTS; i++) {
        s = &l->slots[i];
        if (s->pid == 0)
            break;
    }
    if (i == LICENSE_SLOTS) {
        for (i = 0; i < LICENSE_SLOTS; i++) {
            if (kill(l->slots[i].pid, 0) == 0)
                continue;
            if (errno == ESRCH)
                break;
            if (errno == EPERM)
                continue;
            tts_set_error_errno(errno, "kill");
            ivona_sem_post(l->sem);
            return 0;
        }
        if (i == LICENSE_SLOTS) {
            tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "Too many certificates concurrently loaded in the system");
            return 0;
        }
        s = &l->slots[i];
    }
    memcpy(s, &l->self, sizeof(license_slot));
    ivona_sem_post(l->sem);
    l->slot = i;
    return 1;
}

/* 0x47cec arm */
int license_free(license *l)
{
    int ok = 1;
    int err;

    if (l) {
        ok &= ivona_sem_close(l->sem);
        l->sem = NULL;
        if (l->slots) {
            l->slots[l->slot].pid = 0;
            l->slots = NULL;
        }
        ok &= ivona_shm_free(l->shm);
        if ((err = pthread_mutex_destroy(&l->mutex)) != 0) {
            tts_set_error_errno(err, "pthread_mutex_destroy");
            ok = 0;
        }
        l->shm = NULL;
        cst_free(l);
    }
    return ok;
}

/* 0x47d80 arm */
license *license_new(int key)
{
    license *l = cst_safe_calloc(1, sizeof(license));
    int err;

    l->self.key = key;
    l->self.pid = getpid();
    l->self.count = 0;
    if ((l->sem = ivona_sem_open()) == NULL)
        goto fail;
    if ((l->shm = ivona_shm_open(LICENSE_SHM_KEY, LICENSE_SLOTS * sizeof(license_slot), ivona_shm_clear)) == NULL)
        goto fail;
    l->slots = ivona_shm_ptr(l->shm);
    if (!license_attach(l))
        goto fail;
    if ((err = pthread_mutex_init(&l->mutex, NULL)) != 0) {
        tts_set_error_errno(err, "pthread_mutex_init");
        goto fail;
    }
    return l;
fail:
    license_free(l);
    return NULL;
}

/* unlocks the table mutex, then returns ret (0 if unlocking fails) */
static inline int license_unlock(license *l, int ret)
{
    int err;

    if ((err = pthread_mutex_unlock(&l->mutex)) != 0) {
        tts_set_error_errno(err, "pthread_mutex_unlock");
        return 0;
    }
    return ret;
}

/* 0x47e3c arm: re-attaches after the table was recreated or the process forked, and makes sure
 * our slot is still ours. */
int license_check(license *l)
{
    license_slot *s;
    int err, r;
    pid_t pid;

    if (l->shm != NULL) {
        if ((err = pthread_mutex_lock(&l->mutex)) != 0) {
            tts_set_error_errno(err, "pthread_mutex_lock");
            return 0;
        }
        r = ivona_shm_unlock(l->shm);
        if (r < 0)
            return license_unlock(l, 0);
        if (r > 0) {
            l->slots = NULL;
            ivona_shm_free(l->shm);
            l->shm = NULL;
            ivona_sem_close(l->sem);
            if ((l->sem = ivona_sem_open()) == NULL)
                return license_unlock(l, 0);
            if ((l->shm = ivona_shm_open(LICENSE_SHM_KEY, LICENSE_SLOTS * sizeof(license_slot), ivona_shm_clear)) == NULL)
                return license_unlock(l, 0);
            l->slots = ivona_shm_ptr(l->shm);
            if (!license_attach(l))
                return license_unlock(l, 0);
        }
        pid = getpid();
        s = &l->slots[l->slot];
        if (l->self.pid != pid) {
            l->self.pid = pid;
            if (!license_attach(l))
                return license_unlock(l, 0);
        } else if (memcmp(s, &l->self, sizeof(license_slot)) != 0) {
            if (!license_attach(l))
                return license_unlock(l, 0);
        }
        return license_unlock(l, 1);
    }
    tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "No access to shared memory");
    return 0;
}

/* 0x48084 arm: takes one of max threads of our key. */
int license_acquire(license *l, unsigned int max)
{
    int total;
    license_slot *s;
    int i, err;

    if (!license_check(l))
        return 0;
    total = 0;
    for (i = 0; i < LICENSE_SLOTS; i++)
        if (l->slots[i].pid && l->slots[i].key == l->self.key)
            total += l->slots[i].count;
    if (total >= max) {
        /* recount without dead processes */
        total = 0;
        for (i = 0; i < LICENSE_SLOTS; i++) {
            s = &l->slots[i];
            if (!s->pid || s->key != l->self.key || !s->count)
                continue;
            if (kill(s->pid, 0) != 0) {
                if (errno == ESRCH) {
                    s->pid = 0;
                    continue;
                }
                if (errno != EPERM) {
                    tts_set_error_errno(errno, "kill");
                    return 0;
                }
            }
            total += s->count;
        }
        if (total >= max) {
            tts_set_errorf(TTS_STATUS_CERTIFICATE_LIMIT, "Too many threads");
            return 0;
        }
    }
    if ((err = pthread_mutex_lock(&l->mutex)) != 0) {
        tts_set_error_errno(err, "pthread_mutex_lock");
        return 0;
    }
    l->slots[l->slot].count++;
    l->self.count++;
    if ((err = pthread_mutex_unlock(&l->mutex)) != 0) {
        tts_set_error_errno(err, "pthread_mutex_unlock");
        return 0;
    }
    return 1;
}

/* 0x48250 arm: gives a thread back. */
int license_release(license *l)
{
    int err;

    if (l == NULL) {
        tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "TCE free NULL argument");
        return 0;
    }
    if (!license_check(l))
        return 0;
    if ((err = pthread_mutex_lock(&l->mutex)) != 0) {
        tts_set_error_errno(err, "pthread_mutex_lock");
        return 0;
    }
    if (l->self.count == 0) {
        if ((err = pthread_mutex_unlock(&l->mutex)) != 0) {
            tts_set_error_errno(err, "pthread_mutex_unlock");
            return 0;
        }
        tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "Free thread below 0");
        return 0;
    }
    l->slots[l->slot].count--;
    l->self.count--;
    return license_unlock(l, 1);
}

/* 0x48358 arm: removes the semaphore and the shared table. */
int license_remove_all(void)
{
    int ok = ivona_sem_unlink();
    ivona_shm *shm = ivona_shm_create(LICENSE_SHM_KEY);

    if (shm == NULL)
        return 0;
    if (!ivona_shm_lock(shm)) {
        ivona_shm_free(shm);
        return 0;
    }
    ivona_shm_free(shm);
    return ok;
}
