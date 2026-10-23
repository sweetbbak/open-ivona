/*
 * Per-phone acoustic data (0x5cf18..0x5d4d0): val types, parameter stream descriptors and
 * the log-probability table built from a phone's frames. This TU is compiled without
 * inlining (see config).
 */
#include <math.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_item.h"
#include "cst_val.h"
#include "cst_val_defs.h"

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
    int unk_c[4];
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
void acoustic_nop(void)
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
