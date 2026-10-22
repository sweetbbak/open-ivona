/* Engine-internal declarations shared between TUs of libttsivonavoice.so. */
#ifndef TTS_INTERNAL_H
#define TTS_INTERNAL_H

#include "ivona_tts.h"

#ifdef __cplusplus
extern "C" {
#endif

/* tts_error.c */
void tts_set_error(enum tts_status code);
void tts_set_errorf(enum tts_status code, const char *fmt, ...);
void tts_set_error_errno(int err, const char *func);
void tts_set_error_code(enum tts_status code);
void tts_set_error_contextf(enum tts_status code, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
