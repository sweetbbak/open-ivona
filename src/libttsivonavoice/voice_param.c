/*
 * Voice parameters (0x3b4b0..0x3d9a0): tts_voice_param("op.name[.which]", io_val) dispatches
 * through voice_params[] to typed handlers that read/write the voice's feature set.
 */
#include <math.h>
#include <string.h>
#include "cst_features.h"
#include "cst_string.h"
#include "tts_internal.h"
#include "tts_cert.h"
#include "ivona_util.h"
#include "tts_voice.h"
#include "voice_param.h"

/* the voice's default feature values (consulted for "def") */
#define VOICE_DEFAULTS(v) (((cst_features **)(v)->voice_feats)[5])

#define PARAM_KEY_MAGIC 0xe6aa80c7

enum { PARAM_MIN, PARAM_MAX, PARAM_DEF };
static const int param_which_slot[3] = { PARAM_MIN, PARAM_DEF, PARAM_MAX };

/* a parameter's range: v[PARAM_MIN], v[PARAM_MAX], v[PARAM_DEF] */
typedef struct param_range {
    float v[3];
} param_range;

/* SSML prosody limits */
typedef struct float_param_def {
    const char *feat;
    param_range range;
} float_param_def;

static const float_param_def float_params[] = {
    { "eng_max_ssml_rate", { { 0.0f, 200.0f, 200.0f } } },
    { "eng_min_ssml_rate", { { 0.0f, 200.0f, 0.0f } } },
    { "eng_max_ssml_pitch", { { 0.0f, 200.0f, 200.0f } } },
    { "eng_min_ssml_pitch", { { 0.0f, 200.0f, 0.0f } } },
    { NULL, { { 0.0f, 0.0f, 0.0f } } }
};

/* durations: applications use milliseconds (scale converts to the feature's seconds) */
typedef struct time_param_def {
    const char *feat;
    float scale;
    param_range range;
} time_param_def;

static const time_param_def time_params[] = {
    { "eng_sentence_break", 0.001f, { { 0.0f, 60.0004f, 0.4f } } },
    { "eng_paragraph_break", 0.001f, { { 0.0f, 60.0004f, 0.65f } } },
    { "eng_max_ssml_break_time", 0.001f, { { 0.0f, 120.0004f, 120.0004f } } },
    { "pause_start_len", 0.001f, { { 0.01f, 0.0500038f, 0.01f } } },
    { "pause_end_len", 0.001f, { { 0.01f, 0.0500038f, 0.0500038f } } },
    { NULL, 0.0f, { { 0.0f, 0.0f, 0.0f } } }
};

/* index of which in ("min", "def", "max"), or -1 */
static inline int param_which(const char *which)
{
    if (strcmp("min", which) == 0)
        return 0;
    if (strcmp("def", which) == 0)
        return 1;
    if (strcmp("max", which) == 0)
        return 2;
    return -1;
}

/* 0x3b4b0: "set"/"get" of a float voice parameter (amplify, duration_stretch, pitch_stretch).
 * Applications see values as 1..99 across [min, max], XOR-scrambled with the certificate key;
 * with which = "min"/"def"/"max" that point of the range is used instead. */
int tts_voice_param_float(tts_voice *v, const char *op, const char *name, const char *which, int *value)
{
    float val;
    float r[3];
    float min, def, max;
    const tts_cert_data *cert;
    cst_features *f;
    float k;
    unsigned int x;
    int i, n;

    if (strcmp(name, "amplify") == 0) {
        r[PARAM_MIN] = min = get_param_float(VOICE_FEATURES(v), "param_amplify_min", 0.0f);
        r[PARAM_DEF] = def = get_param_float(VOICE_FEATURES(v), "param_amplify_def", 1.0f);
        r[PARAM_MAX] = max = get_param_float(VOICE_FEATURES(v), "param_amplify_max", 1.6f);
    } else if (strcmp(name, "duration_stretch") == 0) {
        r[PARAM_MIN] = min = get_param_float(VOICE_FEATURES(v), "param_dur_min", 0.25f);
        r[PARAM_DEF] = def = get_param_float(VOICE_FEATURES(v), "param_dur_def", 1.0f);
        r[PARAM_MAX] = max = get_param_float(VOICE_FEATURES(v), "param_dur_max", 1.5f);
    } else if (strcmp(name, "pitch_stretch") == 0) {
        r[PARAM_MIN] = min = get_param_float(VOICE_FEATURES(v), "param_pitch_min", 0.5f);
        r[PARAM_DEF] = def = get_param_float(VOICE_FEATURES(v), "param_pitch_def", 1.0f);
        r[PARAM_MAX] = max = get_param_float(VOICE_FEATURES(v), "param_pitch_max", 2.0f);
    } else {
        tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "Invalid float param");
        return 0;
    }

    if (strcmp(op, "set") == 0) {
        if (which) {
            f = VOICE_FEATURES(v);
            i = param_which(which);
            val = def;
            if (i >= 0)
                val = r[param_which_slot[i]];
            feat_set_float(f, name, val);
            return 1;
        }
        cert = tts_get_certificate(v->tts);
        x = *value - 1;
        x = (x ^ PARAM_KEY_MAGIC ^ (cert ? cert->key : 0)) % 99;
        val = x * (max - min) / 98.0f + min;
        if (*value >= 1 && *value <= 99 && val >= min && val <= max) {
            feat_set_float(VOICE_FEATURES(v), name, val);
            return 1;
        }
        tts_set_error(TTS_STATUS_RANGE_ERROR);
        return 0;
    }
    if (strcmp(op, "get") == 0) {
        cert = tts_get_certificate(v->tts);
        if (which) {
            i = param_which(which);
            val = def;
            if (i >= 0)
                val = r[param_which_slot[i]];
        } else
            val = get_param_float(VOICE_FEATURES(v), name, def);
        k = (val - min) * 98.0f / (max - min) + 1.0f;
        n = (int)k;
        if (!(k - n >= 0.5f))
            n--;
        *value = ((n ^ PARAM_KEY_MAGIC) ^ (cert ? cert->key : 0)) % 99 + 1;
        return 1;
    }
    return 1;
}

/* range slot for which (unknown names select the default) */
static inline int param_slot(const char *which)
{
    int i = param_which(which);
    return i >= 0 ? param_which_slot[i] : PARAM_DEF;
}

/* 0x3b9b8: SSML prosody factors (float). volume/rate/pitch factors use the ranges of
 * amplify/duration_stretch/pitch_stretch; rate and pitch are stored inverted. */
int voice_param_factor(tts_voice *v, const char *op, const char *feat, const char *which,
                              float *val)
{
    const float_param_def *p;
    param_range r;
    const char *name;
    int inv;
    float x;
    int i;

    if (strcmp(feat, "amplify_factor") == 0) {
        r.v[PARAM_MIN] = get_param_float(VOICE_FEATURES(v), "param_amplify_min", 0.0f);
        r.v[PARAM_DEF] = get_param_float(VOICE_FEATURES(v), "param_amplify_def", 1.0f);
        r.v[PARAM_MAX] = get_param_float(VOICE_FEATURES(v), "param_amplify_max", 1.6f);
        name = "amplify";
        inv = 0;
    } else if (strcmp(feat, "rate_factor") == 0) {
        r.v[PARAM_MIN] = get_param_float(VOICE_FEATURES(v), "param_dur_min", 0.25f);
        r.v[PARAM_DEF] = get_param_float(VOICE_FEATURES(v), "param_dur_def", 1.0f);
        r.v[PARAM_MAX] = get_param_float(VOICE_FEATURES(v), "param_dur_max", 1.5f);
        name = "duration_stretch";
        inv = 1;
    } else if (strcmp(feat, "pitch_factor") == 0) {
        r.v[PARAM_MIN] = get_param_float(VOICE_FEATURES(v), "param_pitch_min", 0.5f);
        r.v[PARAM_DEF] = get_param_float(VOICE_FEATURES(v), "param_pitch_def", 1.0f);
        r.v[PARAM_MAX] = get_param_float(VOICE_FEATURES(v), "param_pitch_max", 2.0f);
        name = "pitch_stretch";
        inv = 1;
    } else {
        for (p = float_params; p->feat; p++)
            if (strcmp(p->feat, feat) == 0)
                break;
        if (p->feat == NULL) {
            tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "Invalid float param");
            return 0;
        }
        r = p->range;
        name = p->feat;
        inv = 0;
    }

    if (strcmp(op, "set") == 0) {
        if (which) {
            i = param_which(which);
            feat_set_float(VOICE_FEATURES(v), name,
                           r.v[i >= 0 ? param_which_slot[inv ? 2 - i : i] : PARAM_DEF]);
            return 1;
        }
        x = *val;
        if (isnan(x)) {
            tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
            return 0;
        }
        if (inv)
            x = 1.0f / x;
        if (x > r.v[PARAM_MAX] + 1e-6f || x < r.v[PARAM_MIN] - 1e-6f) {
            tts_set_error(TTS_STATUS_RANGE_ERROR);
            return 0;
        }
        feat_set_float(VOICE_FEATURES(v), name, x);
        return 1;
    }
    if (strcmp(op, "get") == 0) {
        if (which) {
            i = param_which(which);
            x = r.v[i >= 0 ? param_which_slot[inv ? 2 - i : i] : PARAM_DEF];
        } else
            x = get_param_float(VOICE_FEATURES(v), name, r.v[PARAM_DEF]);
        *val = inv ? 1.0f / x : x;
        return 1;
    }
    return 1;
}

/* 0x3bed4: boolean parameter without a range ("which" is rejected). */
int voice_param_bool_plain(tts_voice *v, const char *op, const char *feat, const char *which, int *val)
{
    if (which) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return 0;
    }
    if (strcmp(op, "set") == 0) {
        if (*val < 0 || *val > 1) {
            tts_set_error(TTS_STATUS_RANGE_ERROR);
            return 0;
        }
        feat_set_int(VOICE_FEATURES(v), feat, *val);
        return 1;
    }
    if (strcmp(op, "get") == 0) {
        *val = get_param_int(VOICE_FEATURES(v), feat, 0);
        return 1;
    }
    return 1;
}

/* 0x3bf9c: boolean parameter (default 0). */
int voice_param_bool(tts_voice *v, const char *op, const char *feat, const char *which, int *val)
{
    int ret = 0;

    if (strcmp(op, "set") == 0) {
        if (which == NULL) {
            if (*val < 0 || *val > 1)
                tts_set_error(TTS_STATUS_RANGE_ERROR);
            else {
                feat_set_int(VOICE_FEATURES(v), feat, *val);
                ret = 1;
            }
        } else if (strcmp(which, "def") == 0) {
            feat_set_int(VOICE_FEATURES(v), feat, 0);
            ret = 1;
        } else
            tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
    } else if (strcmp(op, "get") == 0) {
        if (which == NULL) {
            *val = get_param_int(VOICE_FEATURES(v), feat, 0);
            ret = 1;
        } else if (strcmp(which, "def") == 0) {
            *val = 0;
            ret = 1;
        } else
            tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
    } else
        ret = 1;
    return ret;
}

/* 0x3c0e0: duration parameter in milliseconds. */
int voice_param_time(tts_voice *v, const char *op, const char *feat, const char *which, int *val)
{
    const time_param_def *p;
    cst_features *f;
    float t;
    int ret;

    for (p = time_params; p->feat; p++)
        if (strcmp(feat, p->feat) == 0)
            break;
    if (p->feat == NULL) {
        tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "Invalid floatlength param");
        return 0;
    }
    if (strcmp(op, "set") == 0) {
        if (which) {
            f = VOICE_FEATURES(v);
            feat_set_float(f, feat, p->range.v[param_slot(which)]);
            ret = 1;
        } else {
            t = *val * p->scale;
            if (t < p->range.v[PARAM_MIN] || t > p->range.v[PARAM_MAX]) {
                tts_set_error(TTS_STATUS_RANGE_ERROR);
                ret = 0;
            } else {
                feat_set_float(VOICE_FEATURES(v), feat, t);
                ret = 1;
            }
        }
    } else if (strcmp(op, "get") == 0) {
        if (which)
            t = p->range.v[param_slot(which)];
        else
            t = get_param_float(VOICE_FEATURES(v), feat, 0.0f);
        t /= p->scale;
        *val = t >= 0 ? (int)(t + 0.5) : (int)(t - 0.5);
        ret = 1;
    } else
        ret = 1;
    return ret;
}

/* range and default of the integer parameters */
typedef struct int_param_def {
    const char *feat;
    int min;
    int max;
    int def;
} int_param_def;

static const int_param_def int_params[] = {
    { "eng_max_sentence", 1, 0x7fffffff, 1000 },
    { "text_maxwordlength", 1, 0x7fffffff, 179 },
    { "num_parallel_threads", 1, 8, 1 },
    { "eng_xml_entity_expansion_limit", 0, 0x7fffffff, 100 },
    { "ja_mecab_variant", 0, 0x7fffffff, 0 },
    { NULL, 0, 0, 0 }
};

/* 0x3c3b0: integer parameter with a range ("def" prefers the voice's default). */
int voice_param_int(tts_voice *v, const char *op, const char *feat, const char *which, int *val)
{
    const int_param_def *p;
    const cst_val *dv;
    int ret;

    for (p = int_params; p->feat; p++)
        if (strcmp(feat, p->feat) == 0)
            break;
    if (p->feat == NULL) {
        tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "Invalid length param");
        return 0;
    }
    if (strcmp(op, "set") == 0) {
        if (which == NULL) {
            if (*val < p->min || *val > p->max) {
                tts_set_error(TTS_STATUS_RANGE_ERROR);
                ret = 0;
            } else {
                feat_set_int(VOICE_FEATURES(v), feat, *val);
                ret = 1;
            }
        } else if (strcmp(which, "min") == 0) {
            feat_set_int(VOICE_FEATURES(v), feat, p->min);
            ret = 1;
        } else if (strcmp(which, "max") == 0) {
            feat_set_int(VOICE_FEATURES(v), feat, p->max);
            ret = 1;
        } else {
            dv = feat_val(VOICE_DEFAULTS(v), feat);
            feat_set(VOICE_FEATURES(v), feat, dv ? dv : int_val(p->def));
            ret = 1;
        }
    } else if (strcmp(op, "get") == 0) {
        if (which == NULL) {
            *val = get_param_int(VOICE_FEATURES(v), feat, 0);
            ret = 1;
        } else if (strcmp(which, "min") == 0) {
            *val = p->min;
            ret = 1;
        } else if (strcmp(which, "max") == 0) {
            *val = p->max;
            ret = 1;
        } else {
            *val = get_param_int(VOICE_DEFAULTS(v), feat, p->def);
            ret = 1;
        }
    } else
        ret = 1;
    return ret;
}

/* defaults of the string parameters ("def") */
typedef struct string_param_def {
    const char *feat;
    const char *def;
} string_param_def;

static const string_param_def string_params[] = {
    { "eng_ssml_base_uri", "" },
    { "eng_input_type", "" },
    { "eng_text_encoding", "UTF-8" },
    { "eng_dict_insert_id", "" },
    { NULL, NULL }
};

static const char *const uri_schemes[] = { "file://", "http://", NULL };

static inline const char *string_param_default(const char *feat)
{
    int i;

    for (i = 0; string_params[i].feat; i++)
        if (strcmp(string_params[i].feat, feat) == 0)
            return string_params[i].def;
    return NULL;
}

int charset_supported(const char *name);

/* 0x3cd34: string parameter; "set" with NULL stores "". */
int voice_param_string(tts_voice *v, const char *op, const char *feat, const char *which,
                              const char **val)
{
    const char *s;
    int ret;

    if (strcmp(op, "set") == 0) {
        if (which == NULL)
            s = val == NULL ? "" : *val ? *val : "";
        else if (strcmp(which, "def") == 0) {
            s = string_param_default(feat);
            if (s == NULL)
                goto missing;
        } else
            goto invalid;
        feat_set_string(VOICE_FEATURES(v), feat, s);
        ret = 1;
    } else if (strcmp(op, "get") == 0) {
        if (which == NULL)
            *val = get_param_string(VOICE_FEATURES(v), feat, "");
        else if (strcmp(which, "def") == 0) {
            *val = string_param_default(feat);
            if (*val == NULL)
                goto missing;
        } else
            goto invalid;
        ret = 1;
    } else
        ret = 1;
    return ret;
invalid:
    tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
    return 0;
missing:
    tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "Default string missing");
    return 0;
}

/* 0x3cf50: "text_encoding": the encoding must be known. */
int voice_param_encoding(tts_voice *v, const char *op, const char *feat, const char *which,
                                const char **val)
{
    if (strcmp(op, "set") == 0 && which == NULL) {
        if (val == NULL || *val == NULL) {
            tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
            return 0;
        }
        if (!charset_supported(*val)) {
            tts_set_errorf(TTS_STATUS_RANGE_ERROR, "Invalid character encoding");
            return 0;
        }
    }
    return voice_param_string(v, op, feat, which, val);
}

/* 0x3cff0: "base_uri": only file:// and http:// URIs. */
int voice_param_base_uri(tts_voice *v, const char *op, const char *feat, const char *which,
                                const char **val)
{
    const char *const *p;

    if (strcmp(op, "set") == 0 && which == NULL && val && *val && **val) {
        for (p = uri_schemes; *p; p++)
            if (strncasecmp(*val, *p, strlen(*p)) == 0)
                goto ok;
        tts_set_errorf(TTS_STATUS_RANGE_ERROR, "Unsupported URI scheme; must be file:// or http://");
        return 0;
    }
ok:
    return voice_param_string(v, op, feat, which, val);
}

/* 0x3d0dc: "input_type": plain text, or SSML when the certificate allows it. */
int voice_param_input_type(tts_voice *v, const char *op, const char *feat, const char *which,
                                  const char **val)
{
    const tts_cert_data *cert;

    if (strcmp(op, "set") == 0 && which == NULL && val && *val) {
        if (strcasecmp(*val, "application/ssml+xml") == 0) {
            cert = tts_get_certificate(v->tts);
            if (cert == NULL || !cert->ssml) {
                tts_set_errorf(TTS_STATUS_CERTIFICATE_LIMIT, "The certificate does not allow for SSML input");
                return 0;
            }
        } else if (strcasecmp(*val, "") != 0 && strcasecmp(*val, "text/plain") != 0) {
            tts_set_errorf(TTS_STATUS_RANGE_ERROR, "Unsupported MIME type");
            return 0;
        }
    }
    return voice_param_string(v, op, feat, which, val);
}

/* 0x3d1ec: pointer parameter (callbacks and their user data); default NULL. */
int voice_param_pointer(tts_voice *v, const char *op, const char *feat, const char *which,
                               void **val)
{
    const cst_val *pv;
    int ret = 1;

    if (strcmp(op, "set") == 0) {
        if (which == NULL)
            feat_set(VOICE_FEATURES(v), feat, userdata_val(*val));
        else if (strcmp(which, "def") == 0)
            feat_set(VOICE_FEATURES(v), feat, userdata_val(NULL));
        else {
            tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
            ret = 0;
        }
    } else if (strcmp(op, "get") == 0) {
        if (which == NULL) {
            pv = get_param_val(VOICE_FEATURES(v), feat, NULL);
            *val = pv ? val_userdata(pv) : NULL;
        } else if (strcmp(which, "def") == 0)
            *val = NULL;
        else {
            tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
            ret = 0;
        }
    }
    return ret;
}

/* features copied from the voice library's registration into the voice's own */
static const char *const voice_copied_feats[] = { "text_maxwordlength", NULL };

/* 0x3d5a0: the unit selection beam ("quality") from the voice library, and the features in
 * voice_copied_feats. */
void voice_quality_init(tts_voice *v)
{
    voice_data *d = VOICE_DATA(v);
    int beam = get_param_int(d->reg->features, "viterbi_beam", -1);
    const char *const *f;
    const cst_val *val;

    if (beam < 1) {
        d = VOICE_DATA(v);
        d->unk_10 = (void *)50;
        beam = 25;
    } else {
        d = VOICE_DATA(v);
        d->unk_10 = (void *)beam;
        beam >>= 1;
    }
    feat_set_int(d->features, "viterbi_beam", beam);
    for (f = voice_copied_feats; *f; f++)
        if ((val = feat_val(VOICE_FEATURES(v), *f)) != NULL)
            feat_set(VOICE_DATA(v)->features, *f, val);
}
