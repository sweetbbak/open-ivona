/*
 * Per-phone acoustic data (0x5cf18..0x5d4d0): val types, parameter stream descriptors and
 * the log-probability table built from a phone's frames. This TU is compiled without
 * inlining (see config).
 */
#include <float.h>
#include <math.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_item.h"
#include "cst_val.h"
#include "cst_utterance.h"
#include "cst_val_defs.h"

typedef struct hash hash;
void *hash_get(hash *h, const void *key, size_t len);
cst_item *item_traverse(const cst_item *item, ...);

int ivona_rand(int *seed);
int *utt_diphonedb_extra(const cst_utterance *utt);

#define ITEM_TRAVERSE_PREV 0x103
#define ITEM_TRAVERSE_PARENT 0x106

typedef struct breath_covariances breath_covariances;
typedef struct breath_stats breath_stats;

/* one acoustic parameter stream */
typedef struct acoustic_stream {
    int id;
    int dim;
    int has_voicing;
    int type;   /* 0: full covariance, 1: diagonal */
} acoustic_stream;

/* one stream of one frame */
typedef struct acoustic_elem {
    int unk[6];
    float log_unvoiced;
    float log_voiced;
} acoustic_elem;

typedef struct phone_acoustic_data {
    unsigned int n_frames;
    unsigned int n_streams;
    acoustic_stream *streams;
    const void *unk_c;
    float log_dur;
    float det;
    const float *voicing;
    float *data;
    int unk_20;
    acoustic_elem *elems;
} phone_acoustic_data;

CST_VAL_USER_TYPE_DCLS(float_ptr, float)
CST_VAL_USER_TYPE_DCLS(phone_acoustic_data, phone_acoustic_data)
CST_VAL_USER_TYPE_DCLS(acoustic_covariances, breath_covariances)
CST_VAL_USER_TYPE_DCLS(breaths_stats, breath_stats)

CST_VAL_REGISTER_TYPE_NODEL(float_ptr, float)
CST_VAL_REGISTER_TYPE_NODEL(phone_acoustic_data, phone_acoustic_data)
CST_VAL_REGISTER_TYPE_NODEL(acoustic_covariances, breath_covariances)
CST_VAL_REGISTER_TYPE_NODEL(breaths_stats, breath_stats)

/* 0x5d0d8 arm */
void delete_float_ptr(float *f)
{
    cst_free(f);
}

/* 0x5d0dc arm */
void val_delete_float_ptr(void *v)
{
    delete_float_ptr(v);
}

/* 0x5d0e0 arm: stream type by name ("f0" 1, "rms" 2, "M" 3, else 0). */
int acoustic_stream_type(const char *name)
{
    if (strcmp(name, "f0") == 0)
        return 1;
    if (strcmp(name, "rms") == 0)
        return 2;
    if (strcmp(name, "M") == 0)
        return 3;
    return 0;
}

/* 0x5d150 arm */
const char *acoustic_stream_type_name(int type)
{
    switch (type) {
    case 1:
        return "f0";
    case 2:
        return "rms";
    case 3:
        return "M";
    default:
        return "UNKNOWN";
    }
}

/* 0x5d1a8 arm: number of covariance values of a stream. */
int acoustic_stream_cov_size(const acoustic_stream *s)
{
    switch (s->type) {
    case 0:
        return s->dim * s->dim;
    case 1:
        return s->dim;
    default:
        return 0;
    }
}

/* 0x5d1dc arm */
int acoustic_streams_cov_size(const acoustic_stream *s, unsigned int n, int k)
{
    unsigned int i;
    int total = 0, c;

    for (i = 0; i < n; i++) {
        c = acoustic_stream_cov_size(&s[i]);
        total += c ? c + 1 : 0;
    }
    return k * total;
}

/* 0x5d230 arm */
void acoustic_nop(cst_features *f, phone_acoustic_data *p)
{
}

/* 0x5d234 arm */
void phone_acoustic_data_clear(phone_acoustic_data *p)
{
    cst_free(p->elems);
}

/* 0x5d23c arm */
void delete_phone_acoustic_data(phone_acoustic_data *p)
{
    phone_acoustic_data_clear(p);
    cst_free(p);
}

/* 0x5d254 arm */
void val_delete_phone_acoustic_data(void *v)
{
    delete_phone_acoustic_data(v);
}

/* 0x5d258 arm: copies the item's "acoustic_data" (zeroes if it has none). */
void item_phone_acoustic_data(const cst_item *item, phone_acoustic_data *out)
{
    if (item_feat_present(item, "acoustic_data"))
        *out = *val_phone_acoustic_data(item_feat(item, "acoustic_data"));
    else
        memset(out, 0, sizeof *out);
}

/* 0x5d2c4 arm */
int acoustic_elem_init(phone_acoustic_data *p, acoustic_elem *e, int frame, int id)
{
    return 0;
}

/* 0x5d2cc arm: values per frame of a stream (voicing flag, mean, covariance). */
int acoustic_stream_size(const acoustic_stream *s)
{
    int n = s->has_voicing != 0;
    int c = acoustic_stream_cov_size(s);

    return n + s->dim + c;
}

/* 0x5d2f4 arm */
int acoustic_streams_size(const acoustic_stream *s, unsigned int n)
{
    unsigned int i;
    int total = 0;

    for (i = 0; i < n; i++)
        total += acoustic_stream_size(&s[i]);
    return total;
}

/* 0x5d338 arm: builds the per-frame, per-stream element table. */
void phone_acoustic_data_init(phone_acoustic_data *p)
{
    float *data = p->data;
    unsigned int i, j;
    int pos;
    acoustic_elem *e;
    float v, q;
    double d;

    p->elems = cst_safe_alloc(p->n_frames * p->n_streams, sizeof(acoustic_elem));
    for (i = 0; i < p->n_frames; i++) {
        pos = i * acoustic_streams_size(p->streams, p->n_streams);
        for (j = 0; j < p->n_streams; j++) {
            e = &p->elems[i * p->n_streams + j];
            acoustic_elem_init(p, e, i, p->streams[j].id);
            if (p->streams[j].has_voicing) {
                v = data[pos];
                d = v < 1e-5 ? (float)1e-5 : v;
                q = v > 0.99999 ? (float)1e-5 : 1.0f - v;
                e->log_voiced = log(d);
                e->log_unvoiced = log(q);
            }
            pos += acoustic_stream_size(&p->streams[j]);
        }
    }
}

/* 0x5d4bc arm */
int acoustic_streams_frame_size(const acoustic_stream *s, unsigned int n, int k)
{
    return acoustic_streams_size(s, n) * k;
}

/* 0x5d4d0 arm: stores the DNN acoustic model outputs in h (per stream: voicing, means,
 * deltas, delta-deltas; then the voice's covariances) as the item's acoustic data, and sets
 * its duration. */
void item_set_dnn_acoustic_data(cst_item *item, hash *h)
{
    const cst_utterance *utt;
    const cst_val *v;
    const phone_acoustic_data *proto;
    const float *const *covs;
    phone_acoustic_data *p;
    float ldur, dur, x;
    float *data, *mean, *d, *dd;
    const char *name, *dname, *ddname;
    unsigned int f, s, dim, k;
    int pos;

    item_feat_string(item, "name");
    if (h == NULL)
        return;
    utt = item_utt(item);
    v = feat_val(utt->features, "dnn_model_acoustic_data");
    proto = v ? val_phone_acoustic_data(v) : NULL;
    v = feat_val(utt->features, "acoustic_covariances");
    covs = v ? (const float *const *)val_acoustic_covariances(v) : NULL;
    p = cst_safe_alloc(1, sizeof *p);
    *p = *proto;
    ldur = *(float *)hash_get(h, "dur", 3);
    dur = exp(ldur) / 1000.0;
    item_set_float(item, "end",
                   ffeature_float(item_traverse(item, ITEM_TRAVERSE_PREV, 0), "end") + dur);
    item_set_float(item, "dur", dur);
    p->log_dur = ldur;
    data = cst_safe_alloc(acoustic_streams_frame_size(p->streams, p->n_streams, p->n_frames),
                          sizeof(float));
    p->data = data;
    pos = 0;
    for (f = 0; f < p->n_frames; f++) {
        for (s = 0; s < p->n_streams; s++) {
            name = acoustic_stream_type_name(p->streams[s].id);
            switch (p->streams[s].id) {
            case 1:
                dname = "f0_delta";
                break;
            case 2:
                dname = "rms_delta";
                break;
            case 3:
                dname = "M_delta";
                break;
            default:
                dname = "UNKNOWN_delta";
                break;
            }
            switch (p->streams[s].id) {
            case 1:
                ddname = "f0_delta_delta";
                break;
            case 2:
                ddname = "rms_delta_delta";
                break;
            case 3:
                ddname = "M_delta_delta";
                break;
            default:
                ddname = "UNKNOWN_delta_delta";
                break;
            }
            dim = p->streams[s].dim;
            if (p->streams[s].has_voicing) {
                x = ((float *)hash_get(h, "L", 1))[f];
                if (x > 1.0f)
                    x = 1.0f;
                else if (x < 0.0f)
                    x = 0.0f;
                data[pos++] = x;
            }
            mean = hash_get(h, name, strlen(name));
            d = hash_get(h, dname, strlen(dname));
            dd = hash_get(h, ddname, strlen(ddname));
            if (d == NULL) {
                k = dim;
                memcpy(&data[pos], mean + k * f, k * sizeof(float));
                pos += k;
            } else {
                k = dd ? dim / 3 : dim / 2;
                if (dd)
                    dd += k * f;
                d += k * f;
                memcpy(&data[pos], mean + k * f, k * sizeof(float));
                pos += k;
                if (d) {
                    memcpy(&data[pos], d, k * sizeof(float));
                    pos += k;
                }
            }
            if (dd) {
                memcpy(&data[pos], dd, k * sizeof(float));
                pos += k;
            }
            memcpy(&data[pos], covs[s], acoustic_stream_cov_size(&p->streams[s]) * sizeof(float));
            pos += acoustic_stream_cov_size(&p->streams[s]);
        }
    }
    item_set(item, "acoustic_raw_data", float_ptr_val(p->data));
    item_set(item, "acoustic_data", phone_acoustic_data_val(p));
    acoustic_nop(item_feats(item), p);
    phone_acoustic_data_init(p);
}

/* a normal sample (Box-Muller) seeded from the item's "end_pos" */
#define BREATH_GAUSS(out, w, mean, std, seed)                                         \
    do {                                                                              \
        float u1_, g_;                                                                \
        int u2_;                                                                      \
        seed = ffeature_int(item_traverse(w, 0), "end_pos");                          \
        do {                                                                          \
            u1_ = ivona_rand(&seed) * (1.0f / 32767);                                 \
            u2_ = ivona_rand(&seed);                                                  \
        } while (u1_ <= FLT_MIN);                                                     \
        g_ = sqrt(-2.0 * log(u1_)) * cos(2.0 * M_PI * (u2_ * (1.0f / 32767)));    \
        out = std * g_ + mean;                                                        \
    } while (0)

static const float breath_unk[4] = { 0, 0, 10.0f, 0 };
static const acoustic_stream breath_streams[1] = { { 2, 1, 0, 0 } };

/* 0x5da14 arm: gives a breath item its duration and volume (from the phrase's "breath_dur"
 * and "breath_volume", else sampled from the voice's breath statistics) and its acoustic
 * data; returns the duration. */
float item_set_breath_acoustic_data(cst_item *item)
{
    const cst_item *w;
    const cst_val *v;
    const float *st = NULL;
    const float *const *covs;
    phone_acoustic_data *p;
    float dur, rms, m, sd;
    float *data;
    unsigned int i;
    int seed1, seed2;

    w = item_traverse(item_traverse(item, 4, ITEM_TRAVERSE_PARENT, 0),
                      ITEM_TRAVERSE_PARENT, 1, ITEM_TRAVERSE_PARENT, 0);
    v = feat_val(item_utt(item)->features, "breaths_rms_dur_stats");
    if (v == NULL || (st = *(const float *const *)val_breaths_stats(v)) == NULL) {
        rms = -27.657f;
        dur = 0.295f;
    } else {
        if (!item_feat_present(w, "breath_dur")) {
            m = st[0];
            sd = st[1];
            BREATH_GAUSS(dur, w, m, sd, seed1);
        } else
            dur = ffeature_float(item_traverse(w, 0), "breath_dur");
        if (item_feat_present(w, "breath_volume"))
            rms = st[2] + 20.0f * (float)log10(ffeature_float(item_traverse(w, 0), "breath_volume"));
        else {
            m = st[2];
            sd = st[3];
            BREATH_GAUSS(rms, w, m, sd, seed2);
        }
    }
    item_set_float(item, "dur", dur);
    item_set_float(item, "end",
                   ffeature_float(item_traverse(item, ITEM_TRAVERSE_PREV, 0), "end") + dur);
    {
        const cst_utterance *utt = item_utt(item);

        v = feat_val(utt->features, "acoustic_breath_covariances");
        covs = v ? (const float *const *)val_acoustic_covariances(v) : NULL;
        p = cst_safe_alloc(1, sizeof *p);
        p->n_streams = 1;
        p->log_dur = log(dur * 1000.0f);
        p->streams = (acoustic_stream *)breath_streams;
        p->unk_c = breath_unk;
        p->det = covs[0][0];
        p->n_frames = *utt_diphonedb_extra(utt);
    }
    data = cst_safe_alloc(acoustic_streams_frame_size(p->streams, p->n_streams, p->n_frames),
                          sizeof(float));
    p->data = data;
    for (i = 0; i < p->n_frames; i++) {
        data[2 * i] = rms;
        data[2 * i + 1] = covs[1][i];
    }
    item_set(item, "acoustic_raw_data", float_ptr_val(data));
    item_set(item, "acoustic_data", phone_acoustic_data_val(p));
    acoustic_nop(item_feats(item), p);
    phone_acoustic_data_init(p);
    return dur;
}

/* 0x5dea0 arm */
int acoustic_streams_frame_size_voiced(const acoustic_stream *s, unsigned int n, int k)
{
    return k * acoustic_streams_size(s, n) + k;
}

/* 0x5deb4 arm */
int acoustic_stub_0(void)
{
    return 0;
}

/* 0x5debc arm */
int acoustic_stub_1(void)
{
    return 0;
}

/* 0x5dec4 arm: writes the frame count and the frames to out (if not NULL); returns the
 * byte size. */
size_t phone_acoustic_data_serialize(char *out, const phone_acoustic_data *p)
{
    size_t size, head = 0;

    if (p == NULL)
        return 0;
    size = acoustic_streams_frame_size(p->streams, p->n_streams, p->n_frames) * sizeof(float);
    if (p->voicing)
        head = p->n_frames * sizeof(float);
    if (out) {
        if (p->voicing) {
            memcpy(out, p->voicing, head);
            out += head;
        }
        memcpy(out, p->data, size);
    }
    return head + size;
}
