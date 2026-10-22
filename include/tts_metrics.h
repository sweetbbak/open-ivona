/* Engine metrics: a cst_features set of "metric" vals (int or float, stored or computed). */
#ifndef TTS_METRICS_H
#define TTS_METRICS_H

#include "cst_features.h"

#define TTS_METRIC_INT      0
#define TTS_METRIC_FLOAT    1

typedef union tts_metric_value {
    int i;
    float f;
} tts_metric_value;

/* a "metric" val: a value or a function computing it from the metrics set */
typedef struct tts_metric_def {
    int type;      /* TTS_METRIC_INT / TTS_METRIC_FLOAT */
    int kind;      /* 0 or 1 (1 for timings) */
    union {
        int i;
        float f;
        int (*ifn)(const cst_features *metrics);
        float (*ffn)(const cst_features *metrics);
    } v;
    int is_func;
} tts_metric_def;

/* public record filled by tts_metrics_get* */
typedef struct tts_metric {
    const char *name;
    int type;
    tts_metric_value value;
    int kind;
} tts_metric;

extern const int cst_val_type_metric;
tts_metric_def *val_metric(const cst_val *v);
cst_val *metric_val(const tts_metric_def *v);

cst_features *metrics_new(void);
int metrics_add_int(cst_features *metrics, const char *name, int v);
float metrics_set_float(cst_features *metrics, const char *name, float v);
int metrics_max_int(cst_features *metrics, const char *name, int v);
float metrics_add_time(cst_features *metrics, const char *name, int usec);

#endif
