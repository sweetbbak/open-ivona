/*
 * Addons (0x36a3c..0x37dd0): MessagePack files {"version": 2, "modules": {name: data}} whose
 * modules (unit blacklists, word embeddings, DNN models, voice databases) apply to the voices
 * their selectors match.
 */
#include <string.h>
#include "cst_alloc.h"
#include "cst_file.h"
#include "cst_features.h"
#include "cst_val.h"
#include "tts_voice.h"

/* MessagePack reader (mpack_io.c) */
typedef struct mp_reader {
    int unk[4];
} mp_reader;
void mp_reader_init(mp_reader *r, const void *data, size_t size, int flags);
void mp_reader_free(mp_reader *r);
int cmp_read_map(mp_reader *r, unsigned int *size);
int cmp_read_array(mp_reader *r, unsigned int *size);
int mp_read_int(mp_reader *r);
void *mp_read_bin(mp_reader *r, unsigned int *size, int type);

typedef struct blacklist blacklist;
cst_val *blacklist_add(blacklist *bl, cst_features *f);
void blacklist_remove(blacklist *bl, cst_features *f);

typedef struct diphonedb {
    int unk_0;
    int checksum;               /* +0x04 */
} diphonedb;
diphonedb *val_diphonedb(const cst_val *v);

/* the voices a module applies to (0 or NULL: any) */
typedef struct addon_selector {
    char *vox_name;
    int checksum;               /* +0x04: of the voice database */
    char *language;             /* +0x08 */
    char *gender;               /* +0x0c */
    int sample_rate;            /* +0x10 */
} addon_selector;

/* unit_blacklists */
typedef struct unit_blacklist {
    addon_selector sel;
    int *units;                 /* +0x14: count, then the units */
} unit_blacklist;

/* word_embeddings */
typedef struct word_vectors word_vectors;
typedef struct word_embeddings word_embeddings;
word_vectors *word_embeddings_read(mp_reader *r);
void word_vectors_free(word_vectors *wv);
word_embeddings *word_embeddings_from(const word_vectors *wv);
int word_embeddings_add(word_embeddings *we, const word_vectors *wv);
int word_embeddings_remove(word_embeddings *we, const word_vectors *wv);
cst_val *word_embeddings_val(const word_embeddings *we);
word_embeddings *val_word_embeddings(const cst_val *v);

typedef struct embedding_set {
    addon_selector sel;
    char *name;                 /* +0x14 */
    word_vectors *db;           /* +0x18 */
} embedding_set;

/* dnn_models (the DNN code is stubbed out in this build: dnn_stub.c) */
typedef struct dnn_model dnn_model;
typedef struct dnn_models dnn_models;
dnn_model *dnn_stub_2c(mp_reader *r);
void dnn_stub_34(dnn_model *m);
dnn_models *new_dnn_models(void);
void dnn_models_add(dnn_models *m, dnn_model *d);
void dnn_models_remove(dnn_models *m, dnn_model *d);
cst_val *dnn_models_val(const dnn_models *m);
dnn_models *val_dnn_models(const cst_val *v);

typedef struct dnn_model_set {
    addon_selector sel;
    dnn_model *model;           /* +0x14 */
} dnn_model_set;

/* dynamic_voxdbs */
typedef struct dynamic_voxdbs {
    diphonedb *a;
    diphonedb *b;               /* +0x04: the breaths database */
} dynamic_voxdbs;
dynamic_voxdbs *val_dynamic_voxdbs(const cst_val *v);
int phone_is_breaths(const char *name);
diphonedb *diphonedb_new(const unsigned char *data);
void delete_diphonedb(diphonedb *d);

typedef struct voxdb_set {
    addon_selector sel;
    int breaths;                /* +0x14: type "breaths" */
    diphonedb *db;              /* +0x18 */
} voxdb_set;

typedef struct addon {
    cst_filemap *map;
    unit_blacklist *blacklists; /* +0x04 */
    unsigned int blacklist_count;
    embedding_set *embeddings;  /* +0x0c */
    unsigned int embedding_count;
    dnn_model_set *dnn_models;  /* +0x14 */
    unsigned int dnn_model_count;
    voxdb_set *voxdbs;          /* +0x1c */
    unsigned int voxdb_count;
} addon;

typedef struct addon_module {
    const char *name;
    int (*parse)(addon *a, mp_reader *r);
    void (*free)(addon *a);
    void (*attach)(addon *a, tts_voice *v);
    void (*detach)(addon *a, tts_voice *v);
} addon_module;

extern addon_module *g_modules[4];

/* 0x36a3c arm: applies the addon to the voice */
void addon_attach(addon *a, tts_voice *v)
{
    int i;

    for (i = 0; i < 4; i++)
        g_modules[i]->attach(a, v);
}

/* 0x36a7c arm */
void addon_detach(addon *a, tts_voice *v)
{
    int i;

    for (i = 0; i < 4; i++)
        g_modules[i]->detach(a, v);
}

/* 0x36abc arm */
void addon_free(addon *a)
{
    int i;

    if (a == NULL)
        return;
    for (i = 0; i < 4; i++)
        g_modules[i]->free(a);
    if (a->map)
        cst_munmap_file(a->map);
    cst_free(a);
}

/* the module with the name */
static inline addon_module *addon_find_module(const char *name)
{
    if (strcmp(name, g_modules[0]->name) == 0)
        return g_modules[0];
    if (strcmp(name, g_modules[1]->name) == 0)
        return g_modules[1];
    if (strcmp(name, g_modules[2]->name) == 0)
        return g_modules[2];
    if (strcmp(name, g_modules[3]->name) == 0)
        return g_modules[3];
    return NULL;
}

/* 0x36b10 arm */
static addon *addon_parse(mp_reader *r)
{
    addon *a = cst_safe_calloc(1, sizeof(addon));
    unsigned int n, m;
    int version_ok = 0;

    if (!cmp_read_map(r, &n))
        goto fail;
    while (n--) {
        char *key = mp_read_bin(r, NULL, 1);

        if (key == NULL)
            goto fail;
        if (strcmp("version", key) == 0) {
            if (mp_read_int(r) != 2)
                goto fail;
            version_ok = 1;
            continue;
        }
        if (strcmp("modules", key) != 0)
            goto fail;
        if (!cmp_read_map(r, &m))
            goto fail;
        while (m--) {
            char *name = mp_read_bin(r, NULL, 1);
            addon_module *mod;

            if (name == NULL || (mod = addon_find_module(name)) == NULL || !mod->parse(a, r))
                goto fail;
        }
    }
    if (version_ok)
        return a;
fail:
    addon_free(a);
    return NULL;
}

/* 0x36cc4 arm */
int addon_read_selector(mp_reader *r, addon_selector *s)
{
    unsigned int n;

    memset(s, 0, sizeof(addon_selector));
    if (!cmp_read_map(r, &n))
        return 0;
    while (n--) {
        char *key = mp_read_bin(r, NULL, 1);

        if (key == NULL)
            return 0;
        if (strcmp(key, "vox_name") == 0) {
            if ((s->vox_name = mp_read_bin(r, NULL, 1)) == NULL)
                return 0;
        } else if (strcmp(key, "voxdb_checksum") == 0) {
            if ((s->checksum = mp_read_int(r)) == 0)
                return 0;
        } else if (strcmp(key, "language_code") == 0) {
            if ((s->language = mp_read_bin(r, NULL, 1)) == NULL)
                return 0;
        } else if (strcmp(key, "voice_gender") == 0) {
            if ((s->gender = mp_read_bin(r, NULL, 1)) == NULL)
                return 0;
        } else if (strcmp(key, "voxdb_sample_rate") == 0) {
            if ((s->sample_rate = mp_read_int(r)) == 0)
                return 0;
        } else
            return 0;
    }
    return 1;
}

/* 0x36e58 arm */
int addon_selector_matches(const addon_selector *s, tts_voice *v)
{
    cst_features *f = VOICE_FEATURES(v);

    if (s->checksum && (!feat_present(f, "diphonedb")
                        || val_diphonedb(feat_val(f, "diphonedb"))->checksum != s->checksum))
        return 0;
    if (s->vox_name && strcmp(v->vox_name, s->vox_name) != 0)
        return 0;
    if (s->language && strcmp(val_string(feat_val(f, "language_code")), s->language) != 0)
        return 0;
    if (s->gender && strcmp(val_string(feat_val(f, "voice_gender")), s->gender) != 0)
        return 0;
    if (s->sample_rate <= 0)
        return 1;
    return val_int(feat_val(f, "sample_rate")) == s->sample_rate;
}

/* 0x36f88 arm */
addon *addon_load(const void *data, size_t size)
{
    mp_reader r;
    addon *a;

    mp_reader_init(&r, data, size, 1);
    a = addon_parse(&r);
    mp_reader_free(&r);
    return a;
}

/* 0x36fc4 arm */
addon *addon_load_file(const char *path)
{
    cst_filemap *map = cst_mmap_file(path);
    mp_reader r;
    addon *a;

    if (map == NULL)
        return NULL;
    mp_reader_init(&r, map->mem, map->mapsize, 1);
    a = addon_parse(&r);
    mp_reader_free(&r);
    if (a == NULL) {
        cst_munmap_file(map);
        return NULL;
    }
    a->map = map;
    return a;
}

/* 0x370bc arm */
static void unit_blacklists_detach(addon *a, tts_voice *v)
{
    unsigned int i;

    for (i = 0; i < a->blacklist_count; i++)
        if (addon_selector_matches(&a->blacklists[i].sel, v))
            blacklist_remove((blacklist *)a->blacklists[i].units, VOICE_FEATURES(v));
}

/* 0x3713c arm */
static void unit_blacklists_attach(addon *a, tts_voice *v)
{
    unsigned int i;

    for (i = 0; i < a->blacklist_count; i++)
        if (addon_selector_matches(&a->blacklists[i].sel, v))
            blacklist_add((blacklist *)a->blacklists[i].units, VOICE_FEATURES(v));
}

/* 0x371bc arm */
static void unit_blacklists_free(addon *a)
{
    unsigned int i;

    for (i = 0; i < a->blacklist_count; i++)
        cst_free(a->blacklists[i].units);
    cst_free(a->blacklists);
}

/* 0x37208 arm: [{"selectors": selector, "units": [unit...]}...] */
static int unit_blacklists_parse(addon *a, mp_reader *r)
{
    unsigned int n, m, k, i, j;

    cmp_read_array(r, &n);
    a->blacklists = cst_safe_alloc(n, sizeof(unit_blacklist));
    a->blacklist_count = n;
    for (i = 0; i < n; i++) {
        unit_blacklist *b = &a->blacklists[i];

        memset(b, 0, sizeof(unit_blacklist));
        if (!cmp_read_map(r, &m))
            return 0;
        while (m--) {
            char *key = mp_read_bin(r, NULL, 1);

            if (key == NULL)
                return 0;
            if (strcmp(key, "selectors") == 0) {
                if (!addon_read_selector(r, &b->sel))
                    return 0;
            } else if (strcmp(key, "units") == 0) {
                int *u;

                if (!cmp_read_array(r, &k))
                    return 0;
                u = cst_safe_alloc(k + 1, sizeof(int));
                u[0] = k;
                for (j = 0; j < k; j++)
                    u[j + 1] = mp_read_int(r);
                b->units = u;
            } else
                return 0;
        }
    }
    return 1;
}

/* 0x373b4 arm */
static void word_embeddings_detach(addon *a, tts_voice *v)
{
    unsigned int i;

    for (i = 0; i < a->embedding_count; i++)
        if (addon_selector_matches(&a->embeddings[i].sel, v)) {
            cst_features *f = VOICE_FEATURES(v);
            word_vectors *db = a->embeddings[i].db;

            if (feat_present(f, "word_embeddings"))
                word_embeddings_remove(val_word_embeddings(feat_val(f, "word_embeddings")), db);
        }
}

/* 0x37468 arm */
static void word_embeddings_attach(addon *a, tts_voice *v)
{
    unsigned int i;

    for (i = 0; i < a->embedding_count; i++)
        if (addon_selector_matches(&a->embeddings[i].sel, v)) {
            cst_features *f = VOICE_FEATURES(v);
            word_vectors *db = a->embeddings[i].db;

            if (!feat_present(f, "word_embeddings"))
                feat_set(f, "word_embeddings", word_embeddings_val(word_embeddings_from(db)));
            else
                word_embeddings_add(val_word_embeddings(feat_val(f, "word_embeddings")), db);
        }
}

/* 0x37530 arm */
static void word_embeddings_free(addon *a)
{
    unsigned int i;

    for (i = 0; i < a->embedding_count; i++)
        word_vectors_free(a->embeddings[i].db);
    cst_free(a->embeddings);
}

/* 0x3757c arm: [{"selectors": selector, "name": name, "db": word vectors}...] */
static int word_embeddings_parse(addon *a, mp_reader *r)
{
    unsigned int n, m, i;

    cmp_read_array(r, &n);
    a->embeddings = cst_safe_alloc(n, sizeof(embedding_set));
    a->embedding_count = n;
    for (i = 0; i < n; i++) {
        embedding_set *e = &a->embeddings[i];
        int have_sel;

        memset(e, 0, sizeof(embedding_set));
        if (!cmp_read_map(r, &m))
            return 0;
        have_sel = 0;
        while (m--) {
            char *key = mp_read_bin(r, NULL, 1);

            if (key == NULL)
                return 0;
            if (strcmp(key, "selectors") == 0) {
                if (!addon_read_selector(r, &e->sel))
                    return 0;
                have_sel = 1;
            } else if (strcmp(key, "name") == 0) {
                char *name = mp_read_bin(r, NULL, 1);

                if (name == NULL)
                    return 0;
                e->name = name;
            } else if (strcmp(key, "db") == 0) {
                word_vectors *db = word_embeddings_read(r);

                if (db == NULL)
                    return 0;
                e->db = db;
            } else
                return 0;
        }
        if (!have_sel || e->db == NULL || e->name == NULL)
            return 0;
    }
    return 1;
}

/* 0x37728 arm */
static void dnn_models_detach(addon *a, tts_voice *v)
{
    unsigned int i;

    for (i = 0; i < a->dnn_model_count; i++)
        if (addon_selector_matches(&a->dnn_models[i].sel, v)) {
            cst_features *f = VOICE_FEATURES(v);

            if (feat_present(f, "dnn_models"))
                dnn_models_remove(val_dnn_models(feat_val(f, "dnn_models")), a->dnn_models[i].model);
        }
}

/* 0x377d8 arm */
static void dnn_models_attach(addon *a, tts_voice *v)
{
    unsigned int i;

    for (i = 0; i < a->dnn_model_count; i++)
        if (addon_selector_matches(&a->dnn_models[i].sel, v)) {
            cst_features *f = VOICE_FEATURES(v);

            if (!feat_present(f, "dnn_models"))
                feat_set(f, "dnn_models", dnn_models_val(new_dnn_models()));
            dnn_models_add(val_dnn_models(feat_val(f, "dnn_models")), a->dnn_models[i].model);
        }
}

/* 0x378b0 arm */
static void dnn_models_free(addon *a)
{
    unsigned int i;

    for (i = 0; i < a->dnn_model_count; i++)
        dnn_stub_34(a->dnn_models[i].model);
    cst_free(a->dnn_models);
}

/* 0x378fc arm: [{"selectors": selector, "model": model}...] */
static int dnn_models_parse(addon *a, mp_reader *r)
{
    unsigned int n, m, i;

    cmp_read_array(r, &n);
    a->dnn_models = cst_safe_alloc(n, sizeof(dnn_model_set));
    a->dnn_model_count = n;
    for (i = 0; i < n; i++) {
        dnn_model_set *d = &a->dnn_models[i];

        memset(d, 0, sizeof(dnn_model_set));
        if (!cmp_read_map(r, &m))
            return 0;
        while (m--) {
            char *key = mp_read_bin(r, NULL, 1);

            if (key == NULL)
                return 0;
            if (strcmp(key, "selectors") == 0) {
                if (!addon_read_selector(r, &d->sel))
                    return 0;
            } else if (strcmp(key, "model") == 0) {
                dnn_model *model = dnn_stub_2c(r);

                if (model == NULL)
                    return 0;
                d->model = model;
            } else
                return 0;
        }
        if (d->model == NULL)
            return 0;
    }
    return 1;
}

/* 0x37a44 arm */
static void dynamic_voxdbs_detach(addon *a, tts_voice *v)
{
    unsigned int i;

    for (i = 0; i < a->voxdb_count; i++)
        if (addon_selector_matches(&a->voxdbs[i].sel, v)) {
            diphonedb *db = a->voxdbs[i].db;
            dynamic_voxdbs *dv = val_dynamic_voxdbs(feat_val(VOICE_FEATURES(v), "dynamic_voxdbs"));

            if (dv->b == db && a->voxdbs[i].breaths == 1)
                dv->b = NULL;
        }
}

/* 0x37afc arm */
static void dynamic_voxdbs_attach(addon *a, tts_voice *v)
{
    unsigned int i;

    for (i = 0; i < a->voxdb_count; i++)
        if (addon_selector_matches(&a->voxdbs[i].sel, v)) {
            diphonedb *db = a->voxdbs[i].db;
            dynamic_voxdbs *dv = val_dynamic_voxdbs(feat_val(VOICE_FEATURES(v), "dynamic_voxdbs"));

            if (a->voxdbs[i].breaths == 1)
                dv->b = db;
        }
}

/* 0x37ba4 arm */
static void dynamic_voxdbs_free(addon *a)
{
    unsigned int i;

    for (i = 0; i < a->voxdb_count; i++)
        delete_diphonedb(a->voxdbs[i].db);
    cst_free(a->voxdbs);
}

/* 0x37bf0 arm: [{"selectors": selector, "type": "breaths", "data": voice database}...] */
static int dynamic_voxdbs_parse(addon *a, mp_reader *r)
{
    unsigned int n, m, i;

    cmp_read_array(r, &n);
    a->voxdbs = cst_safe_alloc(n, sizeof(voxdb_set));
    a->voxdb_count = n;
    for (i = 0; i < n; i++) {
        voxdb_set *d = &a->voxdbs[i];
        unsigned char *data;
        unsigned int size;
        int have_sel;

        memset(d, 0, sizeof(voxdb_set));
        if (!cmp_read_map(r, &m))
            return 0;
        data = NULL;
        have_sel = 0;
        while (m--) {
            char *key = mp_read_bin(r, NULL, 1);

            if (key == NULL)
                return 0;
            if (strcmp(key, "selectors") == 0) {
                int ok = addon_read_selector(r, &d->sel);

                have_sel = 1;
                if (!ok)
                    return 0;
            } else if (strcmp(key, "type") == 0) {
                char *type = mp_read_bin(r, NULL, 1);

                if (type == NULL)
                    return 0;
                d->breaths = phone_is_breaths(type);
            } else if (strcmp(key, "data") == 0) {
                if ((data = mp_read_bin(r, &size, 2)) == NULL)
                    return 0;
            } else
                return 0;
        }
        if (!have_sel || d->breaths == 0)
            return 0;
        if ((d->db = diphonedb_new(data)) == NULL)
            return 0;
    }
    return 1;
}

addon_module unit_blacklists_module = {
    "unit_blacklists", unit_blacklists_parse, unit_blacklists_free, unit_blacklists_attach,
    unit_blacklists_detach,
};

addon_module word_embeddings_module = {
    "word_embeddings", word_embeddings_parse, word_embeddings_free, word_embeddings_attach,
    word_embeddings_detach,
};

addon_module dnn_models_module = {
    "dnn_models", dnn_models_parse, dnn_models_free, dnn_models_attach, dnn_models_detach,
};

addon_module dynamic_voxdbs_module = {
    "dynamic_voxdbs", dynamic_voxdbs_parse, dynamic_voxdbs_free, dynamic_voxdbs_attach,
    dynamic_voxdbs_detach,
};

addon_module *g_modules[4] = {
    &dnn_models_module, &unit_blacklists_module, &word_embeddings_module, &dynamic_voxdbs_module,
};
