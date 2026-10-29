/* Voice parameter handlers (voice_param.c), dispatched by tts_voice_param() (voice_param_api.c). */
#ifndef VOICE_PARAM_H
#define VOICE_PARAM_H

#include "ivona_tts.h"

/* handler(voice, "set"|"get", feature name, "min"|"max"|"def" or NULL, value in/out) */
typedef int (*voice_param_fn)(tts_voice *v, const char *op, const char *feat, const char *which,
                              void *io_val);

int tts_voice_param_float(tts_voice *v, const char *op, const char *name, const char *which, int *value);
int voice_param_factor(tts_voice *v, const char *op, const char *feat, const char *which, float *val);
int voice_param_bool_plain(tts_voice *v, const char *op, const char *feat, const char *which, int *val);
int voice_param_bool(tts_voice *v, const char *op, const char *feat, const char *which, int *val);
int voice_param_time(tts_voice *v, const char *op, const char *feat, const char *which, int *val);
int voice_param_int(tts_voice *v, const char *op, const char *feat, const char *which, int *val);
int voice_param_quality(tts_voice *v, const char *op, const char *feat, const char *which, int *val);
int voice_param_responsiveness(tts_voice *v, const char *op, const char *feat, const char *which, int *val);
int voice_param_traverse_break(tts_voice *v, const char *op, const char *feat, const char *which, int *val);
int voice_param_string(tts_voice *v, const char *op, const char *feat, const char *which, const char **val);
int voice_param_encoding(tts_voice *v, const char *op, const char *feat, const char *which, const char **val);
int voice_param_base_uri(tts_voice *v, const char *op, const char *feat, const char *which, const char **val);
int voice_param_input_type(tts_voice *v, const char *op, const char *feat, const char *which,
                           const char **val);
int voice_param_pointer(tts_voice *v, const char *op, const char *feat, const char *which, void **val);
int voice_param_hash_key(tts_voice *v, const char *op, const char *feat, const char *which, const void **val);
int voice_param_encrypt_key(tts_voice *v, const char *op, const char *feat, const char *which, const void **val);
int voice_param_contrast(tts_voice *v, const char *op, const char *feat, const char *which, int *val);
void voice_quality_init(tts_voice *v);
int voice_params_init(tts_voice *v);

#endif
