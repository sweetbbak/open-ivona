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

typedef struct addon_list {
    void *items;
    unsigned int count;
} addon_list;

typedef struct addon {
    cst_filemap *map;
    unit_blacklist *blacklists; /* +0x04 */
    unsigned int blacklist_count;
    addon_list embeddings;      /* +0x0c */
    addon_list dnn_models;      /* +0x14 */
    addon_list voxdbs;          /* +0x1c */
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

int word_embeddings_parse(addon *a, mp_reader *r);
void word_embeddings_free(addon *a);
void word_embeddings_attach(addon *a, tts_voice *v);
void word_embeddings_detach(addon *a, tts_voice *v);
int dnn_models_parse(addon *a, mp_reader *r);
void dnn_models_free(addon *a);
void dnn_models_attach(addon *a, tts_voice *v);
void dnn_models_detach(addon *a, tts_voice *v);
int dynamic_voxdbs_parse(addon *a, mp_reader *r);
void dynamic_voxdbs_free(addon *a);
void dynamic_voxdbs_attach(addon *a, tts_voice *v);
void dynamic_voxdbs_detach(addon *a, tts_voice *v);

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
