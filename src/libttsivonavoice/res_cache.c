/*
 * IVONA engine: reference-counted cache of loaded files keyed by path. An entry is reused while
 * the file's mtime is unchanged; otherwise the file is loaded again through the cache's load
 * callback. Names are descriptive guesses.
 */
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "tts_internal.h"
#include "ivona_vector.h"

typedef void *(*res_load_fn)(const char *path, void *arg);
typedef void (*res_free_fn)(void *data);

typedef struct res_entry {
    res_free_fn free_fn;
    char *path;
    void *data;
    int refcount;
    time_t mtime;
} res_entry;

typedef struct res_cache {
    ivona_vector *entries;   /* of res_entry */
    pthread_mutex_t *mutex;
    res_load_fn load;
    res_free_fn free_fn;
} res_cache;

pthread_mutex_t *ivona_mutex_new(void);
void ivona_mutex_free(pthread_mutex_t *m);
int ivona_mutex_trylock(pthread_mutex_t *m);
int ivona_mutex_unlock(pthread_mutex_t *m);

/* 0x45650: vector element destructor */
static void res_entry_destroy(void *p, size_t size)
{
    res_entry *e = p;

    if (e == NULL)
        return;
    if (e->data)
        e->free_fn(e->data);
    cst_free(e->path);
}

/* 0x4567c */
res_cache *res_cache_new(res_load_fn load, res_free_fn free_fn)
{
    res_cache *c = cst_calloc(res_cache, 1);

    c->load = load;
    c->free_fn = free_fn;
    c->entries = vector_new(sizeof(res_entry), NULL, res_entry_destroy);
    c->mutex = ivona_mutex_new();
    return c;
}

/* 0x456cc */
void res_cache_free(res_cache *c)
{
    if (c == NULL)
        return;
    ivona_mutex_free(c->mutex);
    vector_free(c->entries);
    cst_free(c);
}

/* 0x456f4: data for `path`, loading it (with `arg`) unless an up-to-date copy is cached */
void *res_cache_acquire(res_cache *c, const char *path, void *arg)
{
    struct stat st;
    res_entry ne;
    res_entry *e, *found = NULL;
    void *data;
    size_t i;

    if (stat(path, &st) != 0) {
        tts_set_errorf(TTS_STATUS_SYSTEM_ERROR, "File not found: %s", path);
        return NULL;
    }
    ivona_mutex_trylock(c->mutex);
    for (i = 0; i < vector_size(c->entries); i++) {
        e = vector_at(c->entries, i);
        if (strcmp(e->path, path) == 0 && e->mtime > 0)
            found = e;
    }
    if (found && found->mtime == st.st_mtime) {
        found->refcount++;
        data = found->data;
        ivona_mutex_unlock(c->mutex);
        return data;
    }
    data = c->load(path, arg);
    if (data) {
        ne.refcount = 1;
        ne.data = data;
        ne.path = cst_strdup(path);
        ne.mtime = st.st_mtime;
        ne.free_fn = c->free_fn;
        if (found) {
            /* drop unused stale copies of this file */
            for (i = 0; i < vector_size(c->entries); ) {
                e = vector_at(c->entries, i);
                if (strcmp(e->path, path) == 0 && e->refcount == 0)
                    vector_erase(c->entries, i, 1);
                else
                    i++;
            }
        }
        vector_push(c->entries, &ne);
    }
    ivona_mutex_unlock(c->mutex);
    return data;
}

static res_entry *res_find_data(res_cache *c, void *data)
{
    size_t i;
    res_entry *e;

    for (i = 0; i < vector_size(c->entries); i++) {
        e = vector_at(c->entries, i);
        if (data == e->data)
            return e;
    }
    return NULL;
}

/* 0x45898: drop one reference; unreferenced stale copies are removed.
 * NONMATCHING (~16%): same logic, different register allocation and branch layout. */
void res_cache_release(res_cache *c, void *data)
{
    res_entry *e, *x, *latest;
    const char *path;
    size_t i;

    ivona_mutex_trylock(c->mutex);
    e = res_find_data(c, data);
    if (--e->refcount == 0) {
        path = e->path;
        latest = NULL;
        for (i = 0; i < vector_size(c->entries); i++) {
            x = vector_at(c->entries, i);
            if (strcmp(x->path, path) == 0 && x->mtime > 0)
                latest = x;
        }
        if (e != latest) {
            for (i = 0; i < vector_size(c->entries); i++)
                if (vector_at(c->entries, i) == e) {
                    vector_erase(c->entries, i, 1);
                    break;
                }
        }
    }
    ivona_mutex_unlock(c->mutex);
}

/* 0x459b0: release every data pointer held in `list` (a vector of pointers) */
void res_cache_release_all(res_cache *c, ivona_vector *list)
{
    size_t i;

    for (i = 0; i < vector_size(list); i++)
        res_cache_release(c, *(void **)vector_at(list, i));
}
