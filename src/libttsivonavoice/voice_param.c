/*
 * Voice parameters (0x3b4b0..0x3d9a0): tts_voice_param("op.name[.which]", io_val) dispatches
 * through voice_params[] to typed handlers that read/write the voice's feature set.
 */
#include <string.h>
#include "cst_features.h"
#include "cst_string.h"
#include "tts_internal.h"
#include "tts_cert.h"
#include "ivona_util.h"

/* the voice's feature set (voice_feats -> engine voice -> features) */
#define VOICE_FEATURES(v) (**(cst_features ***)(v)->voice_feats)

#define PARAM_KEY_MAGIC 0xe6aa80c7

enum { PARAM_MIN, PARAM_MAX, PARAM_DEF };
static const int param_which_slot[3] = { PARAM_MIN, PARAM_DEF, PARAM_MAX };

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
static int voice_param_bool(tts_voice *v, const char *op, const char *feat, const char *which, int *val)
{
    if (strcmp(op, "set") == 0) {
        if (which == NULL) {
            if (*val < 0 || *val > 1) {
                tts_set_error(TTS_STATUS_RANGE_ERROR);
                return 0;
            }
            feat_set_int(VOICE_FEATURES(v), feat, *val);
            return 1;
        }
        if (strcmp(which, "def") == 0) {
            feat_set_int(VOICE_FEATURES(v), feat, 0);
            return 1;
        }
    } else if (strcmp(op, "get") == 0) {
        if (which == NULL) {
            *val = get_param_int(VOICE_FEATURES(v), feat, 0);
            return 1;
        }
        if (strcmp(which, "def") == 0) {
            *val = 0;
            return 1;
        }
    } else
        return 1;
    tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
    return 0;
}

typedef int (*voice_param_fn)(tts_voice *v, const char *op, const char *feat, const char *which,
                              void *io_val);

int FUN_0003b9b8(tts_voice *v, const char *op, const char *feat, const char *which, void *io_val);
int FUN_0003c0e0(tts_voice *v, const char *op, const char *feat, const char *which, void *io_val);
int FUN_0003c3b0(tts_voice *v, const char *op, const char *feat, const char *which, void *io_val);
int FUN_0003c630(tts_voice *v, const char *op, const char *feat, const char *which, void *io_val);
int FUN_0003c8f0(tts_voice *v, const char *op, const char *feat, const char *which, void *io_val);
int FUN_0003cae4(tts_voice *v, const char *op, const char *feat, const char *which, void *io_val);
int FUN_0003cd34(tts_voice *v, const char *op, const char *feat, const char *which, void *io_val);
int FUN_0003cf50(tts_voice *v, const char *op, const char *feat, const char *which, void *io_val);
int FUN_0003cff0(tts_voice *v, const char *op, const char *feat, const char *which, void *io_val);
int FUN_0003d0dc(tts_voice *v, const char *op, const char *feat, const char *which, void *io_val);
int FUN_0003d1ec(tts_voice *v, const char *op, const char *feat, const char *which, void *io_val);
int FUN_0003d310(tts_voice *v, const char *op, const char *feat, const char *which, void *io_val);
int FUN_0003d458(tts_voice *v, const char *op, const char *feat, const char *which, void *io_val);
int FUN_0003d648(tts_voice *v, const char *op, const char *feat, const char *which, void *io_val);

typedef struct voice_param_def {
    const char *name;           /* public name */
    const char *feat;           /* voice feature it maps to */
    voice_param_fn fn;
} voice_param_def;

static const char *const voice_param_ops[] = { "set", "get", NULL };
static const char *const voice_param_whiches[] = { "", "min", "max", "def", NULL };

static const voice_param_def voice_params[] = {
    { "volume", "amplify", (voice_param_fn)tts_voice_param_float },
    { "contrast", "eng_contrast", FUN_0003d648 },
    { "dur_stretch", "duration_stretch", (voice_param_fn)tts_voice_param_float },
    { "pitch_stretch", "pitch_stretch", (voice_param_fn)tts_voice_param_float },
    { "sentence_break", "eng_sentence_break", FUN_0003c0e0 },
    { "paragraph_break", "eng_paragraph_break", FUN_0003c0e0 },
    { "initial_silence", "pause_start_len", FUN_0003c0e0 },
    { "final_silence", "pause_end_len", FUN_0003c0e0 },
    { "max_ssml_break_time", "eng_max_ssml_break_time", FUN_0003c0e0 },
    { "max_ssml_rate_factor", "eng_max_ssml_rate", FUN_0003b9b8 },
    { "min_ssml_rate_factor", "eng_min_ssml_rate", FUN_0003b9b8 },
    { "max_ssml_pitch_factor", "eng_max_ssml_pitch", FUN_0003b9b8 },
    { "min_ssml_pitch_factor", "eng_min_ssml_pitch", FUN_0003b9b8 },
    { "text_encoding", "eng_text_encoding", FUN_0003cf50 },
    { "base_uri", "eng_ssml_base_uri", FUN_0003cff0 },
    { "input_type", "eng_input_type", FUN_0003d0dc },
    { "fetch_function", "eng_fetch_function", FUN_0003d1ec },
    { "fetch_user_data", "eng_fetch_user_data", FUN_0003d1ec },
    { "audio_attributes_function", "eng_audio_attributes_function", FUN_0003d1ec },
    { "audio_attributes_user_data", "eng_audio_attributes_user_data", FUN_0003d1ec },
    { "max_sentence_length", "eng_max_sentence", FUN_0003c3b0 },
    { "max_word_length", "text_maxwordlength", FUN_0003c3b0 },
    { "dict_insert_id", "eng_dict_insert_id", FUN_0003cd34 },
    { "quality", "viterbi_beam", FUN_0003c630 },
    { "responsiveness", "responsiveness", FUN_0003c8f0 },
    { "num_parallel_threads", "num_parallel_threads", FUN_0003c3b0 },
    { "xml_entity_expansion_limit", "eng_xml_entity_expansion_limit", FUN_0003c3b0 },
    { "traverse_break", "traverse_break_dist", FUN_0003cae4 },
    { "volume_factor", "amplify_factor", FUN_0003b9b8 },
    { "rate_factor", "rate_factor", FUN_0003b9b8 },
    { "pitch_factor", "pitch_factor", FUN_0003b9b8 },
    { "cache_get_function", "cache_get_func", FUN_0003d1ec },
    { "cache_put_function", "cache_put_func", FUN_0003d1ec },
    { "cache_user_data", "cache_user_data", FUN_0003d1ec },
    { "cache_hash_key", "cache_hash_key", FUN_0003d310 },
    { "cache_encrypt_key", "cache_encrypt_key", FUN_0003d458 },
    { "disable_hybrid", "disable_hybrid", (voice_param_fn)voice_param_bool },
    { "disable_phrasing_dnn", "disable_phrasing_dnn", (voice_param_fn)voice_param_bool },
    { NULL, NULL, NULL }
};

/* 0x3d86c: exported. command is "set:<name>[:min|max|def]" or "get:<name>[:...]". */
int tts_voice_param(tts_voice *v, const char *command, void *io_val)
{
    strlist *l;
    const char *op, *name, *which;
    const voice_param_def *p;
    int ret;

    if (v == NULL || command == NULL)
        goto error;
    l = strlist_new();
    strlist_split_into(l, command, ":");
    if (l->count < 2 || l->count > 3)
        goto bad_list;
    op = l->items[0];
    name = l->items[1];
    which = l->count == 3 ? l->items[2] : "";
    if (!cst_member_string(op, voice_param_ops) || !cst_member_string(which, voice_param_whiches))
        goto bad_list;
    for (p = voice_params; p->name; p++) {
        if (strcmp(p->name, name) == 0) {
            ret = p->fn(v, op, p->feat, strcmp(which, "") ? which : NULL, io_val);
            strlist_free(l);
            goto end;
        }
    }
bad_list:
    strlist_free(l);
error:
    tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
    ret = 0;
end:
    return ret;
}
