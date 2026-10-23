/*
 * IVONA public API: instance lifecycle, certificates, addons and languages (part of the TU that
 * starts after tts_error.c). Field names of tts_instance are provisional.
 */
#include <dlfcn.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_utterance.h"
#include "tts_internal.h"
#include "ivona_vector.h"
#include "tts_metrics.h"
#include "tts_cert.h"

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

/* language_data exported by a language library (copied per loaded language) */
typedef struct tts_lang_desc {
    void *unk_0;
    void *unk_4;
    ivona_vector *parts;              /* +0x08: loaded data files (cache data pointers) */
    void (*part_init)(void *part);    /* +0x0c */
    void (*part_fini)(void *part);    /* +0x10 */
} tts_lang_desc;

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
    ivona_vector *addons_list;      /* +0x24 */
    pthread_mutex_t *addons_lock; /* +0x28 */
};

typedef struct tts_language {
    void *dl;                     /* dlopen() handle */
    tts_lang_desc *desc;
    tts_instance *tts;
} tts_language;

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

int tts_unload_all_languages(tts_instance *tts);
void FUN_000f435c(int x);
void FUN_000e3070(void);
void curl_global_cleanup(void);
void ivona_mutex_free(pthread_mutex_t *m);
void res_cache_free(res_cache *c);
void *res_cache_acquire(res_cache *c, const char *path, void *arg);
void res_cache_release_all(res_cache *c, ivona_vector *list);
void *textstream_open(const char *path);
void textstream_fill_all(void *buf);
const char *textstream_ptr(void *buf);
void textstream_close(void *buf);
void *FUN_000405ac(void *dl, const char *symbol);


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

static inline void resource_release(tts_resource *a)
{
    if (a->magic == TTS_RESOURCE_MAGIC) {
        FUN_00042a50(a->cert);
        FUN_00047cec(a->license);
        a->magic = 0;
        a->cert = NULL;
        a->license = NULL;
    }
}

/* 0x3a254: destructor for the elements of tts->resources */
static void resource_destroy(void *elem, size_t size)
{
    tts_resource *a = *(tts_resource **)elem;

    if (a == NULL)
        return;
    resource_release(a);
    cst_free(a);
}

static inline tts_resource *resource_new(tts_cert_data *cert)
{
    tts_resource *res = cst_calloc(tts_resource, 1);

    res->magic = TTS_RESOURCE_MAGIC;
    res->cert = cert;
    if (cert->license_id == -1)
        res->license = NULL;
    else {
        res->license = FUN_00047d80(cert->id);
        if (res->license == NULL) {
            resource_release(res);
            cst_free(res);
            return NULL;
        }
    }
    return res;
}

/* 0x3a2b0: parses and verifies a certificate and adds it to the instance. A certificate with the
 * same id replaces an older one; tts->certificate becomes the merge of all loaded ones. */
const tts_certificate *tts_load_certificate(tts_instance *tts, const char *cert_text)
{
    tts_cert_data *cert, *c;
    tts_resource *res, *entry, *old;
    size_t i;
    int dup = -1;

    if (tts == NULL || tts->magic != TTS_INSTANCE_MAGIC || tts->resources == NULL || cert_text == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return NULL;
    }
    cert = FUN_00042ab0(cert_text);
    if (cert == NULL)
        goto out;

    if (FUN_00043a6c(cert)) {
        FUN_00042a50(cert);
        tts_set_error(TTS_STATUS_CERTIFICATE_EXPIRED);
        return NULL;
    }
    res = resource_new(cert);
    if (res == NULL) {
        FUN_00042a50(cert);     /* sic: already freed by resource_new */
        return NULL;
    }

    entry = res;
    for (i = 0; i < vector_size(tts->resources); i++) {
        c = (*(tts_resource **)vector_at(tts->resources, i))->cert;
        if (c == NULL)
            continue;
        if (c->id == ((tts_cert_data *)entry->cert)->id) {
            dup = i;
            if (((tts_cert_data *)entry->cert)->issued <= c->issued) {
                tts_set_errorf(TTS_STATUS_RANGE_ERROR, "Attempt to load the same certificate twice");
                goto fail;
            }
        } else if (!FUN_00043c64(c, (tts_cert_data *)entry->cert))
            goto fail;
    }
    if (dup >= 0) {
        old = *(tts_resource **)vector_at(tts->resources, dup);
        if (old)
            resource_release(old);
    }
    vector_push(tts->resources, &entry);

    FUN_00042a50(tts->certificate);
    tts->certificate = NULL;
    if (vector_size(tts->resources) > 1) {
        for (i = 0; i < vector_size(tts->resources); i++) {
            c = (*(tts_resource **)vector_at(tts->resources, i))->cert;
            if (c == NULL)
                continue;
            if (tts->certificate)
                FUN_00043e84(tts->certificate, c);
            else
                tts->certificate = FUN_000439ac(c);
        }
    }
    return (const tts_certificate *)res;

fail:
    resource_release(res);
    cst_free(res);
out:
    return NULL;
}

/* 0x3a57c */
const tts_certificate *tts_load_certificate_file(tts_instance *tts, const char *cert_path)
{
    const tts_certificate *cert;
    void *buf;

    if (tts == NULL || cert_path == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return NULL;
    }
    buf = textstream_open(cert_path);
    if (buf == NULL)
        return NULL;
    textstream_fill_all(buf);
    cert = tts_load_certificate(tts, textstream_ptr(buf));
    textstream_close(buf);
    return cert;
}

/* 0x3a5dc: attribute of a certificate, or of an instance's current certificate */
const char *tts_certificate_get_single(const tts_certificate *cert, const char *attr)
{
    const tts_instance *tts = (const tts_instance *)cert;
    void *c;
    const char *v;

    if (cert == NULL || attr == NULL)
        goto invalid;
    if (tts->magic == TTS_INSTANCE_MAGIC) {
        c = tts->certificate;
        if (c == NULL) {
            if (vector_size(tts->resources) == 0) {
                tts_set_errorf(TTS_STATUS_CERTIFICATE_LIMIT, "No certificates loaded");
                return NULL;
            }
            c = (*(tts_resource **)vector_at(tts->resources, 0))->cert;
        }
    } else if (tts->magic == TTS_RESOURCE_MAGIC)
        c = ((const tts_resource *)cert)->cert;
    else
        goto invalid;
    v = get_param_string(*(cst_features **)c, attr, NULL);
    if (v == NULL)
        goto invalid;
    return v;
invalid:
    tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
    return NULL;
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
    tts->languages = vector_new(sizeof(void *), NULL, NULL);
    tts->lock_20 = ivona_mutex_new();
    tts->lang_table = NULL;
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
void tts_add_language(tts_instance *tts, tts_language *lang)
{
    ivona_vector *v;
    char *table, *p;
    size_t i;

    if (lang == NULL)
        return;
    ivona_mutex_trylock(tts->lock_20);
    vector_push(tts->languages, &lang);
    if (tts->lang_table)
        cst_free(tts->lang_table);
    v = tts->languages;
    table = p = cst_safe_alloc(vector_size(v) + 1, 0x14);
    for (i = 0; i < vector_size(v); i++, p += 0x14)
        memcpy(p, ((void **)*(void ***)vector_at(v, i))[1], 0x14);
    tts->lang_table = (tts_lang_desc *)table;
    ivona_mutex_unlock(tts->lock_20);
}

/* 0x3a970 */
void tts_remove_language(tts_instance *tts, tts_language *lang)
{
    ivona_vector *v;
    char *table, *p;
    size_t i;

    if (lang == NULL)
        return;
    ivona_mutex_trylock(tts->lock_20);
    for (i = 0; i < vector_size(tts->languages); i++)
        if (*(void **)vector_at(tts->languages, i) == lang) {
            vector_erase(tts->languages, i, 1);
            break;
        }
    if (tts->lang_table)
        cst_free(tts->lang_table);
    v = tts->languages;
    table = p = cst_safe_alloc(vector_size(v) + 1, 0x14);
    for (i = 0; i < vector_size(v); i++, p += 0x14)
        memcpy(p, ((void **)*(void ***)vector_at(v, i))[1], 0x14);
    tts->lang_table = (tts_lang_desc *)table;
    ivona_mutex_unlock(tts->lock_20);
}

/* 0x3aa50 */
tts_lang_desc *tts_language_table(tts_instance *tts)
{
    return tts->lang_table;
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
    tts_unload_all_languages(tts);
    FUN_00042a50(tts->certificate);
    vector_free(tts->resources);
    vector_free(tts->modules);
    vector_free(tts->languages);
    ivona_mutex_free(tts->lock_20);
    cst_free(tts->lang_table);
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

/* 0x3ae84: load one data file of a language through the instance's cache */
int lang_load_part(tts_lang_desc *desc, tts_instance *tts, const char *path)
{
    void *part = res_cache_acquire(tts->lang_cache, path, desc->unk_4);

    if (part != NULL) {
        if (desc->part_init)
            desc->part_init((char *)part + 0x10);
        vector_push(desc->parts, &part);
    }
    return part != NULL;
}

/* 0x3aee0 */
void lang_desc_free(tts_lang_desc *desc, tts_instance *tts)
{
    void **p;

    if (desc->parts) {
        if (desc->part_fini)
            for (p = desc->parts->data; p != (void **)desc->parts->data + desc->parts->count; p++)
                desc->part_fini((char *)*p + 0x10);
        res_cache_release_all(tts->lang_cache, desc->parts);
        vector_free(desc->parts);
    }
    cst_free(desc);
}

/* 0x3af60: load a language library and its data files (NULL-terminated list) */
tts_language *tts_load_language(tts_instance *tts, const char *lib_path, const char **files)
{
    tts_language *lang;
    tts_lang_desc *desc;
    const char *version;
    void *dl;

    if (tts == NULL || lib_path == NULL || files == NULL || files[0] == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return NULL;
    }
    dl = dlopen(lib_path, 0);
    if (dl == NULL) {
        tts_set_errorf(TTS_STATUS_SYSTEM_ERROR, "%s", dlerror());
        return NULL;
    }
    version = FUN_000405ac(dl, "engine_version");
    if (strcmp(version, "1.6.101.3") != 0) {
        tts_set_errorf(TTS_STATUS_LOAD_LANGUGE_ERROR, "Language library version mismatch: got %s, required %s",
                       version, "1.6.101.3");
        dlclose(dl);
        return NULL;
    }
    desc = FUN_000405ac(dl, "language_data");
    if (desc == NULL) {
        tts_set_errorf(TTS_STATUS_LOAD_LANGUGE_ERROR, "The library does not have language data");
        dlclose(dl);
        return NULL;
    }
    lang = cst_alloc(tts_language, 1);
    lang->desc = cst_alloc(tts_lang_desc, 1);
    *lang->desc = *desc;
    lang->desc->parts = vector_new(sizeof(void *), NULL, NULL);
    lang->dl = dl;
    lang->tts = tts;
    for (; *files; files++)
        if (!lang_load_part(lang->desc, lang->tts, *files))
            goto fail;
    tts_add_language(tts, lang);
    return lang;
fail:
    lang_desc_free(lang->desc, lang->tts);
    dlclose(lang->dl);
    cst_free(lang);
    return NULL;
}

/* 0x3b138 */
int tts_unload_language(tts_language *lang)
{
    if (lang == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return 0;
    }
    tts_remove_language(lang->tts, lang);
    lang_desc_free(lang->desc, lang->tts);
    dlclose(lang->dl);
    cst_free(lang);
    return 1;
}

/* 0x3b180 */
int tts_unload_all_languages(tts_instance *tts)
{
    ivona_vector *v;
    tts_language *last;
    int rv;

    if (tts == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return 0;
    }
    while (vector_size(tts->languages) != 0) {
        v = tts->languages;
        last = NULL;
        if (v->count)
            last = *(tts_language **)vector_at(v, v->count - 1);
        rv = tts_unload_language(last);
        if (rv != 1)
            return rv;
    }
    return 1;
}

/* 0x3b1ec */
cst_features *tts_streamer_get_metrics(tts_streamer *st)
{
    struct { char pad[0xc]; cst_utterance *utt; char pad2[0x34]; cst_features *metrics; } *d;

    if (st == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return NULL;
    }
    d = st->data;
    if (d == NULL)
        return NULL;
    if (d->metrics)
        return d->metrics;
    if (d->utt == NULL)
        return NULL;
    return get_param_features(d->utt->features, "core_metrics", NULL);
}

/* fill one output record from a metric definition */
#define METRIC_FILL(out, m, metrics) do {                                           \
        switch ((m)->type) {                                                        \
        case 0:                                                                     \
            (out)->type = 0;                                                        \
            if ((m)->is_func)                                                       \
                (out)->value.i = (m)->v.ifn(metrics);                               \
            else                                                                    \
                (out)->value.i = (m)->v.i;                                          \
            break;                                                                  \
        case 1:                                                                     \
            (out)->type = 1;                                                        \
            if ((m)->is_func)                                                       \
                (out)->value.f = (m)->v.ffn(metrics);                               \
            else                                                                    \
                (out)->value.f = (m)->v.f;                                          \
            break;                                                                  \
        }                                                                           \
        switch ((m)->kind) {                                                        \
        case 0:                                                                     \
            (out)->kind = 0;                                                        \
            break;                                                                  \
        case 1:                                                                     \
            (out)->kind = 1;                                                        \
            break;                                                                  \
        }                                                                           \
    } while (0)

/* 0x3b254 */
int tts_metrics_get_all(const cst_features *metrics, tts_metric *out, unsigned int max)
{
    const cst_featvalpair *p;
    const tts_metric_def *m;
    unsigned int n;

    if (metrics == NULL)
        return 0;
    if (out == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return 0;
    }
    p = metrics->head;
    if (p == NULL || max == 0)
        return 0;
    for (n = 0; p && n < max; p = p->next, n++) {
        m = val_metric(p->val);
        out[n].name = p->name;
        METRIC_FILL(&out[n], m, metrics);
    }
    return n;
}

/* 0x3b37c */
int tts_metrics_get_size(const cst_features *metrics)
{
    const cst_featvalpair *p;
    int n;

    if (metrics == NULL)
        return 0;
    for (n = 0, p = metrics->head; p; p = p->next)
        n++;
    return n;
}

/* 0x3b3b0 */
int tts_metrics_get(const cst_features *metrics, tts_metric *out, const char *name)
{
    const tts_metric_def *m;

    if (metrics == NULL)
        return 0;
    if (name == NULL || out == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return 0;
    }
    if (!feat_present(metrics, name))
        return 0;
    m = val_metric(feat_val(metrics, name));
    out->name = name;
    METRIC_FILL(out, m, metrics);
    return 1;
}
