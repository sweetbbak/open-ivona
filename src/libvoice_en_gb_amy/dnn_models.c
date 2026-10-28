/* DNN model lists (0x26e44..0x26fa4): the voice's "dnn_models" feature. */
#include <string.h>
#include "cst_features.h"
#include "cst_val.h"
#include "ivona_vector.h"

typedef struct dnn_model {
    const char *name;
} dnn_model;

typedef struct dnn_models {
    ivona_vector *models;   /* of dnn_model * */
} dnn_models;

void *new_dnn_models(void);
void delete_dnn_models(void *m);

/* 0x26e44 arm: the last added model called name. */
dnn_model *dnn_models_find(dnn_models *m, const char *name)
{
    size_t i = vector_size(m->models);
    dnn_model *d;

    while (i) {
        i--;
        d = *(dnn_model **)vector_at(m->models, i);
        if (strcmp(d->name, name) == 0)
            return d;
    }
    return NULL;
}

/* 0x26ea4 arm */
void dnn_models_add(dnn_models *m, dnn_model *d)
{
    vector_push(m->models, &d);
}

/* 0x26ec8 arm */
void dnn_models_remove(dnn_models *m, dnn_model *d)
{
    vector_remove_ptr(m->models, d);
}

extern const int cst_val_type_dnn_models;
CST_VAL_REGISTER_TYPE(dnn_models, dnn_models)

/* 0x26f44 arm: the "dnn_models" of a feature set, created on first use. */
dnn_models *voice_dnn_models(cst_features **f)
{
    cst_features *feats = *f;
    dnn_models *m;

    if (feat_present(feats, "dnn_models"))
        return val_dnn_models(feat_val(feats, "dnn_models"));
    m = new_dnn_models();
    feat_set(feats, "dnn_models", dnn_models_val(m));
    return m;
}
