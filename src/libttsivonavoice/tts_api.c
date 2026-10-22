/*
 * IVONA public API: instance lifecycle, certificates, addons and languages (part of the TU that
 * starts after tts_error.c). Field names of tts_instance are provisional.
 */
#include <stddef.h>
#include <string.h>
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
    ivona_vector *sources;        /* +0x18: objects whose +4 points to a 20-byte record */
    void *source_table;           /* +0x1c: flat copy of those records (count + 1 entries) */
    pthread_mutex_t *lock_20;     /* +0x20 */
    ivona_vector *addons_list;      /* +0x24 */
    pthread_mutex_t *addons_lock; /* +0x28 */
};

typedef struct tts_resource {     /* a loaded certificate */
    int magic;                    /* TTS_RESOURCE_MAGIC */
    void *cert;                   /* +4: parsed certificate */
    void *license;                /* +8 */
} tts_resource;

static int g_tts_instance_count;

void *FUN_00022b2c(const char *path, void *arg);
void FUN_000230cc(void *data);
void FUN_00036a3c(void *addon, void *module);
void FUN_00036a7c(void *addon, void *module);
void FUN_00036abc(void *addon);
void *FUN_00036f88(const void *data, int size);
void *FUN_00036fc4(const char *path);
int FUN_00048084(void *license, void *key);
void FUN_00042a50(void *p);
void FUN_00047cec(void *p);
void FUN_0003b180(tts_instance *tts);
void FUN_000f435c(int x);
void FUN_000e3070(void);
void curl_global_cleanup(void);
void ivona_mutex_free(pthread_mutex_t *m);
void res_cache_free(res_cache *c);

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
        FUN_00042a50(a->cert);
        FUN_00047cec(a->license);
        a->magic = 0;
        a->cert = NULL;
        a->license = NULL;
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
    tts->sources = vector_new(sizeof(void *), NULL, NULL);
    tts->lock_20 = ivona_mutex_new();
    tts->source_table = NULL;
    tts->addons_lock = ivona_mutex_new();
    tts->addons_list = vector_new(sizeof(void *), NULL, NULL);
    g_tts_instance_count++;
    return tts;
}

/* 0x3a7b4: add a module and announce every attached addon to it */
void tts_add_module(tts_instance *tts, void *module)
{
    size_t i;

    ivona_mutex_trylock(tts->modules_lock);
    for (i = 0; i < vector_size(tts->addons_list); i++)
        FUN_00036a3c(*(void **)vector_at(tts->addons_list, i), module);
    vector_push(tts->modules, &module);
    ivona_mutex_unlock(tts->modules_lock);
}

/* 0x3a81c */
void tts_remove_module(tts_instance *tts, void *module)
{
    size_t i;

    ivona_mutex_trylock(tts->modules_lock);
    i = vector_size(tts->addons_list);
    while (i != 0) {
        i--;
        FUN_00036a7c(*(void **)vector_at(tts->addons_list, i), module);
    }
    for (i = 0; i < vector_size(tts->modules); i++)
        if (*(void **)vector_at(tts->modules, i) == module) {
            vector_erase(tts->modules, i, 1);
            break;
        }
    ivona_mutex_unlock(tts->modules_lock);
}

/* 0x3a8c0 */
void tts_add_source(tts_instance *tts, void *src)
{
    ivona_vector *v;
    char *table, *p;
    size_t i;

    if (src == NULL)
        return;
    ivona_mutex_trylock(tts->lock_20);
    vector_push(tts->sources, &src);
    if (tts->source_table)
        cst_free(tts->source_table);
    v = tts->sources;
    table = p = cst_safe_alloc(vector_size(v) + 1, 0x14);
    for (i = 0; i < vector_size(v); i++, p += 0x14)
        memcpy(p, ((void **)*(void ***)vector_at(v, i))[1], 0x14);
    tts->source_table = table;
    ivona_mutex_unlock(tts->lock_20);
}

/* 0x3a970 */
void tts_remove_source(tts_instance *tts, void *src)
{
    ivona_vector *v;
    char *table, *p;
    size_t i;

    if (src == NULL)
        return;
    ivona_mutex_trylock(tts->lock_20);
    for (i = 0; i < vector_size(tts->sources); i++)
        if (*(void **)vector_at(tts->sources, i) == src) {
            vector_erase(tts->sources, i, 1);
            break;
        }
    if (tts->source_table)
        cst_free(tts->source_table);
    v = tts->sources;
    table = p = cst_safe_alloc(vector_size(v) + 1, 0x14);
    for (i = 0; i < vector_size(v); i++, p += 0x14)
        memcpy(p, ((void **)*(void ***)vector_at(v, i))[1], 0x14);
    tts->source_table = table;
    ivona_mutex_unlock(tts->lock_20);
}

/* 0x3aa50 */
void *tts_source_table(tts_instance *tts)
{
    return tts->source_table;
}

/* 0x3aa58: the explicitly selected certificate, else the first loaded one */
void *tts_get_certificate(tts_instance *tts)
{
    if (tts == NULL || tts->magic != TTS_INSTANCE_MAGIC) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return NULL;
    }
    if (tts->certificate)
        return tts->certificate;
    if (vector_size(tts->resources) == 0) {
        tts_set_errorf(TTS_STATUS_CERTIFICATE_LIMIT, "No certificates loaded");
        return NULL;
    }
    return (*(tts_resource **)vector_at(tts->resources, 0))->cert;
}

/* 0x3aae0: license of the first certificate that accepts the certificate's key */
void *tts_find_license(tts_instance *tts)
{
    tts_resource *r;
    size_t i;

    for (i = 0; i < vector_size(tts->resources); ) {
        r = *(tts_resource **)vector_at(tts->resources, i);
        i++;
        if (r->cert != NULL && FUN_00048084(r->license, ((void **)r->cert)[7]))
            return r->license;
    }
    if (i == 0)
        tts_set_errorf(TTS_STATUS_CERTIFICATE_LIMIT, "No certificates loaded");
    return NULL;
}

/* 0x3ab64 */
void *tts_load_addon(tts_instance *tts, const void *data, int size)
{
    void *addon;

    if (tts == NULL || data == NULL || size == 0) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return NULL;
    }
    addon = FUN_00036f88(data, size);
    if (addon == NULL) {
        tts_set_error(TTS_STATUS_FORMAT_ERROR);
        return NULL;
    }
    tts_attach_addon(tts, addon);
    return addon;
}

/* 0x3abc8 */
void *tts_load_addon_file(tts_instance *tts, const char *path)
{
    void *addon;

    if (tts == NULL || path == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return NULL;
    }
    addon = FUN_00036fc4(path);
    if (addon == NULL) {
        tts_set_error(TTS_STATUS_FORMAT_ERROR);
        return NULL;
    }
    tts_attach_addon(tts, addon);
    return addon;
}

/* 0x3ac20 */
int tts_unload_addon(tts_instance *tts, void *addon)
{
    size_t i, j;
    int ok = 0;

    if (tts == NULL || addon == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return 0;
    }
    ivona_mutex_trylock(tts->addons_lock);
    for (i = 0; i < vector_size(tts->addons_list); i++)
        if (*(void **)vector_at(tts->addons_list, i) == addon) {
            ivona_mutex_trylock(tts->modules_lock);
            for (j = 0; j < vector_size(tts->modules); j++)
                FUN_00036a7c(addon, *(void **)vector_at(tts->modules, j));
            vector_erase(tts->addons_list, i, 1);
            ivona_mutex_unlock(tts->modules_lock);
            FUN_00036abc(addon);
            ok = 1;
            break;
        }
    ivona_mutex_unlock(tts->addons_lock);
    return ok;
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
    vector_free(tts->sources);
    ivona_mutex_free(tts->lock_20);
    cst_free(tts->source_table);
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
