/* Parsed IVONA certificates (cert.c, not yet decompiled). */
#ifndef TTS_CERT_H
#define TTS_CERT_H

#include "ivona_tts.h"

/* parsed certificate (layout partly known) */
typedef struct tts_cert_data {
    char unk_0[0x10];
    long long issued;             /* +0x10 */
    int unk_18;
    int license_id;               /* +0x1c: -1 when the certificate carries no license */
    int id;                       /* +0x20 */
    int unk_24;
    int ssml;                     /* +0x28: SSML input is licensed */
    unsigned int key;             /* +0x2c: obfuscates voice parameter values */
} tts_cert_data;
tts_cert_data *FUN_00042ab0(const char *text);
int cert_status(tts_cert_data *cert);
void *FUN_00047d80(int key);
int FUN_00043c64(tts_cert_data *a, tts_cert_data *b);
void FUN_00043e84(void *merged, tts_cert_data *c);
void *FUN_000439ac(tts_cert_data *c);

void *tts_get_certificate(tts_instance *tts);

#endif
