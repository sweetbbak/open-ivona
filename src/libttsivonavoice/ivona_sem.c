/*
 * IVONA engine: process-wide named POSIX semaphore (shared by every process using the engine).
 * Names are descriptive guesses.
 */
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <time.h>
#include "cst_alloc.h"
#include "tts_internal.h"

#define IVONA_SEM_NAME "/IVONA-e3a8a7e2d4d4fca3"

typedef struct ivona_sem {
    sem_t *sem;
} ivona_sem;

/* 0x459f4: open the semaphore, creating it (value 1) if it does not exist yet */
ivona_sem *ivona_sem_open(void)
{
    ivona_sem *r;
    mode_t old;
    sem_t *s;

    for (;;) {
        old = umask(0);
        s = sem_open(IVONA_SEM_NAME, 0, 0, 0);
        umask(old);
        if (s)
            break;
        if (errno != ENOENT) {
            tts_set_error_errno(errno, "sem_open");
            return NULL;
        }
        old = umask(0);
        s = sem_open(IVONA_SEM_NAME, O_CREAT | O_EXCL, 0666, 1);
        umask(old);
        if (s)
            break;
        if (errno != EEXIST) {
            tts_set_error_errno(errno, "sem_open");
            return NULL;
        }
    }
    r = cst_alloc(ivona_sem, 1);
    r->sem = s;
    return r;
}

/* 0x45af0 */
int ivona_sem_close(ivona_sem *s)
{
    int ok = 1;

    if (s != NULL) {
        if (sem_close(s->sem) != 0) {
            tts_set_error_errno(errno, "sem_close");
            ok = 0;
        }
        cst_free(s);
    }
    return ok;
}

/* 0x45b44: wait at most ~2 seconds; a timeout also counts as success */
int ivona_sem_wait(ivona_sem *s)
{
    struct timespec ts;

    ts.tv_sec = time(NULL) + 2;
    ts.tv_nsec = 0;
    while (sem_timedwait(s->sem, &ts) != 0) {
        if (errno == ETIMEDOUT)
            return 1;
        if (errno != EINTR) {
            tts_set_error_errno(errno, "sem_wait");
            return 0;
        }
    }
    return 1;
}

/* 0x45bd0: post unless the semaphore is already available */
int ivona_sem_post(ivona_sem *s)
{
    int v = 0;

    if (sem_getvalue(s->sem, &v) == 0 && v > 0)
        return 1;
    if (sem_post(s->sem) == 0)
        return 1;
    tts_set_error_errno(errno, "sem_post");
    return 0;
}

/* 0x45c44 */
int ivona_sem_unlink(void)
{
    if (sem_unlink(IVONA_SEM_NAME) == 0)
        return 1;
    tts_set_errorf(errno, "sem_unlink");  /* sic: errno passed as the status code */
    return 0;
}
