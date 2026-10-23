/*
 * Word embeddings (0x51238..0x51910): a hash from words to float vectors, loaded from a
 * MessagePack map {"vector_size", "words", "vectors"}, giving the per-word "word_embedding_coeffs"
 * item features.
 */
#include <stdlib.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_item.h"
#include "cst_utterance.h"
#include "cst_val.h"
#include "cst_val_const.h"
#include "ivona_util.h"
#include "ivona_vector.h"
#include "tts_metrics.h"

typedef struct hash hash;
hash *hash_new(int size, int flags);
void hash_free(hash *h);
void hash_put(hash *h, const char *key, size_t len, void *value);
int hash_remove(hash *h, const char *key, size_t len);
void *hash_get(hash *h, const char *key, size_t len);

/* MessagePack reader (mpack.c) */
typedef struct mp_reader {
    int unk[4];
} mp_reader;
void mp_reader_init(mp_reader *r, const void *data, size_t size, int flags);
void mp_reader_free(mp_reader *r);
int mp_read_map(mp_reader *r, unsigned int *size);
int mp_read_int(mp_reader *r);
void *mp_read_bin(mp_reader *r, unsigned int *size, int type);
char **mp_read_str_array(mp_reader *r, unsigned int *count);

typedef struct word_embeddings {
    unsigned int count;
    unsigned int dim;
    hash *map;              /* word -> const float[dim] */
} word_embeddings;

/* a loaded table */
typedef struct word_vectors {
    unsigned int count;
    unsigned int dim;
    char **words;
    float *vectors;         /* count * dim */
} word_vectors;

/* 0x51238 arm: destructor of a vector of cst_val * */
static void val_vector_elem_free(void *elem, size_t size)
{
    delete_val(*(cst_val **)elem);
}

static inline void word_embeddings_free(word_embeddings *we)
{
    hash_free(we->map);
    cst_free(we);
}

extern const int cst_val_type_word_embeddings;
CST_VAL_REGISTER_TYPE_NODEL(word_embeddings, word_embeddings)

/* 0x51298 arm */
void val_delete_word_embeddings(void *v)
{
    word_embeddings_free(v);
}

/* 0x512cc arm */
word_embeddings *new_word_embeddings(unsigned int count, unsigned int dim)
{
    word_embeddings *we = cst_safe_alloc(1, sizeof(word_embeddings));

    we->map = hash_new(count, 0);
    we->count = 0;
    we->dim = dim;
    return we;
}

/* 0x51308 arm: fails if the dimensions differ (an empty table takes the new ones). */
int word_embeddings_add(word_embeddings *we, const word_vectors *wv)
{
    unsigned int i;
    const char *w;

    if (we->dim != wv->dim) {
        if (we->count != 0)
            return 0;
        we->dim = wv->dim;
    }
    we->count += wv->count;
    for (i = 0; i < wv->count; i++) {
        w = wv->words[i];
        hash_put(we->map, w, strlen(w), wv->vectors + i * wv->dim);
    }
    return 1;
}

/* 0x513a8 arm */
word_embeddings *word_embeddings_from(const word_vectors *wv)
{
    word_embeddings *we = new_word_embeddings(wv->count, wv->dim);

    word_embeddings_add(we, wv);
    return we;
}

/* 0x513cc arm: whether all of wv's words were removed. */
int word_embeddings_remove(word_embeddings *we, const word_vectors *wv)
{
    unsigned int i, n;
    const char *w;

    if (we->count < wv->count || we->dim != wv->dim)
        return 0;
    n = 0;
    for (i = 0; i < wv->count; i++) {
        w = wv->words[i];
        if (hash_remove(we->map, w, strlen(w))) {
            n++;
            we->count--;
        }
    }
    return n == wv->count;
}

/* 0x5146c arm */
const float *word_embeddings_lookup(word_embeddings *we, const char *word, cst_features *metrics)
{
    int t0 = ivona_time_usec();
    const float *v = hash_get(we->map, word, strlen(word));

    metrics_add_time(metrics, "word_embeddings_fetch_time", ivona_time_usec() - t0);
    if (v)
        metrics_add_int(metrics, "word_embeddings_hits", 1);
    else
        metrics_add_int(metrics, "word_embeddings_misses", 1);
    return v;
}

/* 0x51500 arm */
word_vectors *word_embeddings_read(mp_reader *r)
{
    unsigned int n, count, size;
    char **words = NULL;
    float *vectors = NULL;
    int dim = 0;
    const char *key;
    word_vectors *wv;

    mp_read_map(r, &n);
    while (n--) {
        key = mp_read_bin(r, NULL, 1);
        if (strcmp(key, "vector_size") == 0)
            dim = mp_read_int(r);
        else if (strcmp(key, "words") == 0)
            words = mp_read_str_array(r, &count);
        else if (strcmp(key, "vectors") == 0)
            vectors = mp_read_bin(r, &size, 2);
        else
            goto fail;
    }
    if (words == NULL || dim == 0 || vectors == NULL || dim * count * sizeof(float) != size)
        goto fail;
    wv = cst_safe_alloc(1, sizeof(word_vectors));
    wv->words = words;
    wv->dim = dim;
    wv->vectors = vectors;
    wv->count = count;
    return wv;
fail:
    cst_free(words);
    return NULL;
}

/* 0x51658 arm */
void delete_word_embeddings(word_embeddings *we)
{
    word_embeddings_free(we);
}

/* 0x51674 arm */
void word_vectors_free(word_vectors *wv)
{
    cst_free(wv->words);
    cst_free(wv);
}

/* 0x51690 arm */
word_embeddings *word_embeddings_load(const void *data, size_t size)
{
    mp_reader r;
    word_vectors *wv;
    word_embeddings *we;

    mp_reader_init(&r, data, size, 1);
    wv = word_embeddings_read(&r);
    mp_reader_free(&r);
    if (wv == NULL)
        return NULL;
    we = new_word_embeddings(wv->count, wv->dim);
    word_embeddings_add(we, wv);
    word_vectors_free(wv);
    return we;
}

/* 0x516f8 arm: the embedding of an item's word. */
const float *item_word_embedding(const cst_item *item)
{
    const cst_val *v = feat_val(item_utt(item)->features, "word_embeddings");
    word_embeddings *we = v ? val_word_embeddings(v) : NULL;
    cst_features *m = get_param_features(item_utt(item)->features, "core_metrics", NULL);

    return word_embeddings_lookup(we, item_feat_string(item, "name"), m);
}

/* 0x51774 arm: coefficient idx of the word's embedding ("0" if none). */
const cst_val *item_word_embedding_coeff(const cst_item *item, const char *idx)
{
    int i = atoi(idx);
    ivona_vector *coeffs;
    const cst_val *v;
    word_embeddings *we;
    const float *f;
    cst_val *fv;
    unsigned int k;

    if (item_feat_present(item, "word_embedding_coeffs"))
        coeffs = val_vector(item_feat(item, "word_embedding_coeffs"));
    else {
        coeffs = NULL;
        v = feat_val(item_utt(item)->features, "word_embeddings");
        if (v && (we = val_word_embeddings(v)) != NULL) {
            cst_features *m = get_param_features(item_utt(item)->features, "core_metrics", NULL);

            f = word_embeddings_lookup(we, item_feat_string(item, "name"), m);
            if (f) {
                coeffs = vector_new(sizeof(cst_val *), NULL, val_vector_elem_free);
                vector_reserve(coeffs, we->dim);
                for (k = 0; k < we->dim; k++) {
                    fv = float_val(f[k]);
                    vector_push(coeffs, &fv);
                }
            }
        }
        item_set(item, "word_embedding_coeffs", vector_val(coeffs));
    }
    if (coeffs && i < (int)vector_size(coeffs))
        return *(cst_val **)vector_at(coeffs, i);
    return VAL_STRING_0;
}
