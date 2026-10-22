/*
 * IVONA public API: instance lifecycle, certificates, addons and languages (part of the TU that
 * starts after tts_error.c). Field names of tts_instance are provisional.
 */
#include <stddef.h>
#include <pthread.h>
#include "cst_alloc.h"
#include "tts_internal.h"
#include "ivona_vector.h"

#define TTS_INSTANCE_MAGIC 0x3e1b8f65
#define TTS_RESOURCE_MAGIC    0x730366b1
#define CURL_GLOBAL_ALL    3

int curl_global_init(long flags);
const char *curl_easy_strerror(int code);

typedef struct res_cache res_cache;
res_cache *res_cache_new(void *(*load)(const char *path, void *arg), void (*free_fn)(void *data));
pthread_mutex_t *ivona_mutex_new(void);
int ivona_mutex_trylock(pthread_mutex_t *m);
int ivona_mutex_unlock(pthread_mutex_t *m);

struct tts_instance_struct {
    int magic;                    /* TTS_INSTANCE_MAGIC */
    ivona_vector *resources;      /* +0x04: of tts_resource * */
    void *certificate;            /* +0x08 */
    ivona_vector *modules;        /* +0x0c */
    pthread_mutex_t *modules_lock;/* +0x10 */
    res_cache *lang_cache;        /* +0x14 */
    ivona_vector *vec_18;         /* +0x18 */
    void *unk_1c;                 /* +0x1c */
    pthread_mutex_t *lock_20;     /* +0x20 */
    ivona_vector *addons_list;      /* +0x24 */
    pthread_mutex_t *addons_lock; /* +0x28 */
};

typedef struct tts_resource {
    int magic;                    /* TTS_RESOURCE_MAGIC */
    void *unk_4;
    void *unk_8;
} tts_resource;

static int g_tts_instance_count;

void *FUN_00022b2c(const char *path, void *arg);
void FUN_000230cc(void *data);
void FUN_00036a3c(void *a, void *b);
void FUN_00042a50(void *p);
void FUN_00047cec(void *p);
void FUN_0003b180(tts_instance *tts);
void FUN_000f435c(int x);
void FUN_000e3070(void);
void curl_global_cleanup(void);
void ivona_mutex_free(pthread_mutex_t *m);
void res_cache_free(res_cache *c);
int tts_unload_addon(tts_instance *tts, void *addon);

/* 0x3a188 */
static void lang_cache_free(void *data)
{
    FUN_000230cc(data);
}

/* 0x3a18c */
static void *lang_cache_load(const char *path, void *arg)
{
    return FUN_00022b2c(path, arg);
}

/* 0x3a190: attach an addon to the instance once and announce it to every module */
int tts_attach_addon(tts_instance *tts, void *obj)
{
    size_t i;

    ivona_mutex_trylock(tts->addons_lock);
    for (i = 0; i < vector_size(tts->addons_list); i++)
        if (*(void **)vector_at(tts->addons_list, i) == obj) {
            ivona_mutex_unlock(tts->addons_lock);
            return 0;
        }
    vector_push(tts->addons_list, &obj);
    ivona_mutex_trylock(tts->modules_lock);
    for (i = 0; i < vector_size(tts->modules); i++) {
        void *module = *(void **)vector_at(tts->modules, i);
        FUN_00036a3c(obj, module);
    }
    ivona_mutex_unlock(tts->modules_lock);
    ivona_mutex_unlock(tts->addons_lock);
    return 1;
}

/* 0x3a254: destructor for the elements of tts->resources */
static void resource_destroy(void *elem, size_t size)
{
    tts_resource *a = *(tts_resource **)elem;

    if (a == NULL)
        return;
    if (a->magic == TTS_RESOURCE_MAGIC) {
        FUN_00042a50(a->unk_4);
        FUN_00047cec(a->unk_8);
        a->magic = 0;
        a->unk_4 = NULL;
        a->unk_8 = NULL;
    }
    cst_free(a);
}

/* 0x3a69c */
const char *tts_certificate_get(const tts_instance *tts, const char *attr)
{
    return tts_certificate_get_single((const tts_certificate *)tts, attr);
}

/* 0x3a6a0 */
tts_instance *tts_create(void)
{
    tts_instance *tts;
    int rc;

    rc = curl_global_init(CURL_GLOBAL_ALL);
    if (rc != 0) {
        tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "CURL initialization error: %s", curl_easy_strerror(rc));
        return NULL;
    }
    tts = cst_calloc(tts_instance, 1);
    tts->magic = TTS_INSTANCE_MAGIC;
    tts->resources = vector_new(sizeof(tts_resource *), NULL, resource_destroy);
    tts->certificate = NULL;
    tts->modules = vector_new(sizeof(void *), NULL, NULL);
    tts->modules_lock = ivona_mutex_new();
    tts->lang_cache = res_cache_new(lang_cache_load, lang_cache_free);
    tts->vec_18 = vector_new(sizeof(void *), NULL, NULL);
    tts->lock_20 = ivona_mutex_new();
    tts->unk_1c = NULL;
    tts->addons_lock = ivona_mutex_new();
    tts->addons_list = vector_new(sizeof(void *), NULL, NULL);
    g_tts_instance_count++;
    return tts;
}

/* 0x3ad00 */
int tts_destroy(tts_instance *tts)
{
    int i;

    if (tts == NULL)
        return 1;
    if (tts->magic != TTS_INSTANCE_MAGIC) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return 0;
    }
    if (g_tts_instance_count == 0) {
        tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "Free instances below 0");
        return 0;
    }
    i = vector_size(tts->addons_list);
    while (--i >= 0)
        tts_unload_addon(tts, *(void **)vector_at(tts->addons_list, i));
    vector_free(tts->addons_list);
    FUN_0003b180(tts);
    FUN_00042a50(tts->certificate);
    vector_free(tts->resources);
    vector_free(tts->modules);
    vector_free(tts->vec_18);
    ivona_mutex_free(tts->lock_20);
    cst_free(tts->unk_1c);
    res_cache_free(tts->lang_cache);
    ivona_mutex_free(tts->addons_lock);
    ivona_mutex_free(tts->modules_lock);
    tts->certificate = NULL;
    tts->resources = NULL;
    tts->magic = 0;
    tts->modules_lock = NULL;
    tts->addons_lock = NULL;
    cst_free(tts);
    curl_global_cleanup();
    FUN_000f435c(0);
    if (--g_tts_instance_count == 0)
        FUN_000e3070();
    return 1;
}

/* 0x3ae44: create an instance and load its certificate */
tts_instance *tts_init(const char *certificate_text)
{
    tts_instance *tts;

    tts = tts_create();
    if (tts == NULL)
        return NULL;
    if (tts_load_certificate(tts, certificate_text) == NULL) {
        tts_destroy(tts);
        return NULL;
    }
    return tts;
}
