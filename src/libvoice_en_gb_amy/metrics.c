/* Engine metrics set (0x26fa4..0x27574): named int/float counters and timings in a cst_features. */
#include "cst_alloc.h"
#include "cst_error.h"
#include "cst_features.h"
#include "cst_utterance.h"
#include "tts_metrics.h"

static void delete_metric(tts_metric_def *m)
{
    cst_free(m);
}

/* 0x26fa4: arm */
tts_metric_def *val_metric(const cst_val *v)
{
    if (v && CST_VAL_TYPE(v) == cst_val_type_metric)
        return (tts_metric_def *)CST_VAL_VOID(v);
    cst_errmsg("VAL: tried to access %s in %d type val\n", "metric",
               (v ? CST_VAL_TYPE(v) : -1));
    cst_error();
}

/* 0x26ffc: arm */
void val_delete_metric(void *v)
{
    delete_metric((tts_metric_def *)v);
}

/* 0x27000: arm */
cst_val *metric_val(const tts_metric_def *v)
{
    return val_new_typed(cst_val_type_metric, (void *)v);
}

/* the metric called name, created (zeroed, of the given type) if missing */
static inline tts_metric_def *metric_get(cst_features *metrics, const char *name, int type, int kind)
{
    const cst_val *v;
    tts_metric_def *m;
    cst_val *mv;

    if (metrics == NULL)
        return NULL;
    v = feat_val(metrics, name);
    if (v != NULL)
        m = val_metric(v);
    else {
        m = cst_safe_calloc(1, sizeof(tts_metric_def));
        if (m == NULL)
            return NULL;
        m->type = type;
        m->kind = kind;
        mv = metric_val(m);
        if (mv == NULL) {
            cst_free(m);
            return NULL;
        }
        feat_set(metrics, name, mv);
    }
    return m;
}

static inline float metric_time(cst_features *metrics, const char *name)
{
    return metric_get(metrics, name, TTS_METRIC_FLOAT, 1)->v.f;
}

/* 0x27018: arm */
__attribute__((optimize("no-tree-pre")))
static float metrics_internal_synth_time(cst_features *metrics)
{
    register const char *name asm("r5") = "tts_streamer_synth_time";
    register void *sp_val asm("sp");
    asm("" : "+r"(name) : "X"(sp_val));
    float t = metric_get(metrics, name, TTS_METRIC_FLOAT, 1)->v.f;
    t -= metric_time(metrics, "diphone_graph_cache_get_time");
    t -= metric_time(metrics, "diphone_graph_cache_put_time");
    tts_metric_def *m = metric_get(metrics, "fetch_time", TTS_METRIC_FLOAT, 1);
    return t - m->v.f;
}


/* 0x2729c: arm */
cst_features *utt_take_metrics(cst_utterance *u)
{
    cst_val *v;
    cst_features *metrics;

    if (!feat_present(u->features, "core_metrics"))
        return NULL;
    v = feat_take(u->features, "core_metrics");
    metrics = val_features(v);
    cst_free(v);
    return metrics;
}

/* 0x272ec: arm */
cst_features *metrics_new(void)
{
    register const char *name asm("r5") = "tts_streamer_internal_synth_time";
    cst_features *metrics = new_features();
    asm("" : "+r"(name));
    tts_metric_def *m = metric_get(metrics, name, TTS_METRIC_FLOAT, 1);

    m->is_func = 1;
    m->v.ffn = (float (*)(const cst_features *))metrics_internal_synth_time;
    metric_get(metrics, "tts_streamer_synth_time", TTS_METRIC_FLOAT, 1);
    metric_get(metrics, "diphone_graph_cache_get_time", TTS_METRIC_FLOAT, 1);
    metric_get(metrics, "diphone_graph_cache_put_time", TTS_METRIC_FLOAT, 1);
    metric_get(metrics, "fetch_time", TTS_METRIC_FLOAT, 1);
    return metrics;
}

/* 0x27574 arm */
int metrics_add_int(cst_features *metrics, const char *name, int v)
{
    tts_metric_def *m = metric_get(metrics, name, TTS_METRIC_INT, 0);
    if (m == NULL)
        return 0;
    return m->v.i += v;
}

/* 0x27600 arm */
float metrics_set_float(cst_features *metrics, const char *name, float v)
{
    tts_metric_def *m = metric_get(metrics, name, TTS_METRIC_FLOAT, 0);
    if (m == NULL)
        return 0;
    return m->v.f = v;
}

/* 0x27684 arm */
int metrics_max_int(cst_features *metrics, const char *name, int v)
{
    tts_metric_def *m = metric_get(metrics, name, TTS_METRIC_INT, 0);
    if (m == NULL)
        return 0;
    if (m->v.i < v)
        m->v.i = v;
    return m->v.i;
}

/* 0x27714 arm: adds a duration given in microseconds (stored in seconds). */
float metrics_add_time(cst_features *metrics, const char *name, int usec)
{
    tts_metric_def *m = metric_get(metrics, name, TTS_METRIC_FLOAT, 1);
    if (m == NULL)
        return 0;
    return m->v.f += usec / 1000000.0f;
}
