/*
 * tts_voice_param() (0x3d86c): parses "set|get:<name>[:min|max|def]" and dispatches through
 * voice_params[] to the typed handlers of voice_param.c.
 */
#include <string.h>
#include "cst_features.h"
#include "cst_string.h"
#include "tts_internal.h"
#include "ivona_util.h"
#include "tts_voice.h"
#include "voice_param.h"

typedef struct voice_param_def {
    const char *name;           /* public name */
    const char *feat;           /* voice feature it maps to */
    voice_param_fn fn;
} voice_param_def;

static const char *const voice_param_ops[] = { "set", "get", NULL };
static const char *const voice_param_whiches[] = { "", "min", "max", "def", NULL };

static const voice_param_def voice_params[] = {
    { "volume", "amplify", (voice_param_fn)tts_voice_param_float },
    { "contrast", "eng_contrast", (voice_param_fn)voice_param_contrast },
    { "dur_stretch", "duration_stretch", (voice_param_fn)tts_voice_param_float },
    { "pitch_stretch", "pitch_stretch", (voice_param_fn)tts_voice_param_float },
    { "sentence_break", "eng_sentence_break", (voice_param_fn)voice_param_time },
    { "paragraph_break", "eng_paragraph_break", (voice_param_fn)voice_param_time },
    { "initial_silence", "pause_start_len", (voice_param_fn)voice_param_time },
    { "final_silence", "pause_end_len", (voice_param_fn)voice_param_time },
    { "max_ssml_break_time", "eng_max_ssml_break_time", (voice_param_fn)voice_param_time },
    { "max_ssml_rate_factor", "eng_max_ssml_rate", (voice_param_fn)voice_param_factor },
    { "min_ssml_rate_factor", "eng_min_ssml_rate", (voice_param_fn)voice_param_factor },
    { "max_ssml_pitch_factor", "eng_max_ssml_pitch", (voice_param_fn)voice_param_factor },
    { "min_ssml_pitch_factor", "eng_min_ssml_pitch", (voice_param_fn)voice_param_factor },
    { "text_encoding", "eng_text_encoding", (voice_param_fn)voice_param_encoding },
    { "base_uri", "eng_ssml_base_uri", (voice_param_fn)voice_param_base_uri },
    { "input_type", "eng_input_type", (voice_param_fn)voice_param_input_type },
    { "fetch_function", "eng_fetch_function", (voice_param_fn)voice_param_pointer },
    { "fetch_user_data", "eng_fetch_user_data", (voice_param_fn)voice_param_pointer },
    { "audio_attributes_function", "eng_audio_attributes_function", (voice_param_fn)voice_param_pointer },
    { "audio_attributes_user_data", "eng_audio_attributes_user_data", (voice_param_fn)voice_param_pointer },
    { "max_sentence_length", "eng_max_sentence", (voice_param_fn)voice_param_int },
    { "max_word_length", "text_maxwordlength", (voice_param_fn)voice_param_int },
    { "dict_insert_id", "eng_dict_insert_id", (voice_param_fn)voice_param_string },
    { "quality", "viterbi_beam", (voice_param_fn)voice_param_quality },
    { "responsiveness", "responsiveness", (voice_param_fn)voice_param_responsiveness },
    { "num_parallel_threads", "num_parallel_threads", (voice_param_fn)voice_param_int },
    { "xml_entity_expansion_limit", "eng_xml_entity_expansion_limit", (voice_param_fn)voice_param_int },
    { "traverse_break", "traverse_break_dist", (voice_param_fn)voice_param_traverse_break },
    { "volume_factor", "amplify_factor", (voice_param_fn)voice_param_factor },
    { "rate_factor", "rate_factor", (voice_param_fn)voice_param_factor },
    { "pitch_factor", "pitch_factor", (voice_param_fn)voice_param_factor },
    { "cache_get_function", "cache_get_func", (voice_param_fn)voice_param_pointer },
    { "cache_put_function", "cache_put_func", (voice_param_fn)voice_param_pointer },
    { "cache_user_data", "cache_user_data", (voice_param_fn)voice_param_pointer },
    { "cache_hash_key", "cache_hash_key", (voice_param_fn)voice_param_hash_key },
    { "cache_encrypt_key", "cache_encrypt_key", (voice_param_fn)voice_param_encrypt_key },
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

/* 0x3d9a0: sets every parameter to its default. Returns 0 if one failed. */
int voice_params_init(tts_voice *v)
{
    const voice_param_def *p;
    int ret = 1;

    feat_set_float(VOICE_FEATURES(v), "pause_base_len", 0);
    voice_quality_init(v);
    if (!feat_present(VOICE_FEATURES(v), "param_amplify_max"))
        feat_set_float(VOICE_FEATURES(v), "param_amplify_max", 1.6f);
    for (p = voice_params; p->name; p++)
        if (p->fn != (voice_param_fn)voice_param_bool_plain && !p->fn(v, "set", p->feat, "def", NULL))
            ret = 0;
    return ret;
}
