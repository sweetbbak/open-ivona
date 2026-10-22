/*
 * Extension interfaces handed out by e1()/e2() (Thumb C++ TU at the start of .text): versioned
 * tables of extra API functions that the public tts_* table does not have.
 */
#include <stdlib.h>
#include "ivona_tts.h"

extern "C" {
typedef struct cst_features_struct cst_features;
typedef struct tts_metric tts_metric;
cst_features *tts_streamer_get_metrics(tts_streamer *st);
int tts_metrics_get_all(const cst_features *metrics, tts_metric *out, unsigned int max);
int tts_metrics_get_size(const cst_features *metrics);
int tts_metrics_get(const cst_features *metrics, tts_metric *out, const char *name);
void *tts_load_addon_file(tts_instance *tts, const char *path);
}

/* the engine's record (tts_metrics.h) */
struct tts_metric {
    const char *name;
    int type;
    union { int i; float f; } value;
    int kind;
};

/* the record of the version-2 metrics interface */
struct tts_metric_v2 {
    const char *name;
    int type;               /* 0 int, 1 float */
    union { int i; float f; } value;
};

/* 0x18d74 thumb: tts_metrics_get_all() for the 12-byte public record. */
static int metrics_get_all_v2(const cst_features *metrics, tts_metric_v2 *out, unsigned int max)
{
    tts_metric_v2 *dst, *end;
    tts_metric *src, *tmp;
    unsigned int n;

    tmp = (tts_metric *)malloc(max * sizeof(tts_metric));
    if (tmp == NULL)
        return -2;
    n = tts_metrics_get_all(metrics, tmp, max);
    if (n > max)
        return -3;
    end = out + n;
    for (src = tmp, dst = out; dst != end; src++, dst++) {
        dst->name = src->name;
        dst->type = src->type;
        switch (src->type) {
        case 1:
            dst->value.f = src->value.f;
        case 0:
            dst->value.i = src->value.i;
        }
    }
    free(tmp);
    return n;
}

struct tts_metrics_ext {
    int version;
    cst_features *(*streamer_get_metrics)(tts_streamer *st);
    int (*get_size)(const cst_features *metrics);
    int (*get_all)(const cst_features *metrics, tts_metric_v2 *out, unsigned int max);
    int (*get)(const cst_features *metrics, tts_metric *out, const char *name);
};

struct tts_addon_ext {
    int version;
    void *(*load_addon_file)(tts_instance *tts, const char *path);
};

static tts_metrics_ext metrics_ext = {
    2, tts_streamer_get_metrics, tts_metrics_get_size, metrics_get_all_v2, tts_metrics_get
};

static tts_addon_ext addon_ext = { 1, tts_load_addon_file };

/* 0x18dd4 thumb */
extern "C" tts_metrics_ext *e1(void)
{
    return &metrics_ext;
}

/* 0x18de0 thumb */
extern "C" tts_addon_ext *e2(void)
{
    return &addon_ext;
}
