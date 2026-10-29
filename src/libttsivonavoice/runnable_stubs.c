/*
 * Stand-ins for engine functions not decompiled yet, so tools/build_runnable.py can link a runnable
 * engine. Not part of the matching build: no function row credits this file, and each stub goes
 * when the real function is written. The list is in docs/notes/runnable-engine.md.
 */
#include <stdio.h>
#include "tts_track.h"

/* Vorbis unit decoders: runnable_stubs_vorbis.c (shared with the voice library) */

/* SSML-only functions, unreachable while the runnable build has SSML input off
 * (IVONA_CERT_SSML=0, docs/notes/ssml-deferred.md): plain text never has <audio> or <phoneme>
 * elements, lexicon rules in SSML form are skipped, and input is never sniffed for SSML.
 * Reaching one means SSML was enabled before these were written. */
#include <stdlib.h>

static void ssml_unavailable(const char *name)
{
    fprintf(stderr, "ivona: %s is not decompiled yet (SSML input, see ssml-deferred.md)\n", name);
    abort();
}

void ssml_audio_apply_attrs(void *doc, void *e, void *w)
{
    (void)doc;
    (void)e;
    (void)w;
    ssml_unavailable("ssml_audio_apply_attrs");
}

int phoneme_alphabet_id(const char *alphabet, const char *lang)
{
    (void)alphabet;
    (void)lang;
    ssml_unavailable("phoneme_alphabet_id");
    return 0;
}

void *phoneme_convert(int alphabet, const char *ph)
{
    (void)alphabet;
    (void)ph;
    ssml_unavailable("phoneme_convert");
    return NULL;
}

void *lex_apply_ssml(void *rules, void *data, const char *src, const void *text, void *a,
                     void *b, void *c, void *ap)
{
    (void)rules;
    (void)data;
    (void)src;
    (void)text;
    (void)a;
    (void)b;
    (void)c;
    (void)ap;
    ssml_unavailable("lex_apply_ssml");
    return NULL;
}

int text_is_ssml(const char *s, int len)
{
    (void)s;
    (void)len;
    ssml_unavailable("text_is_ssml");
    return 0;
}

/* PLS lexicons (pls.c): the parser (0x268e4, with its expat handlers 0x24c58..0x25c18) and the
 * applier (0x23cd8, 0x24308, 0x23788, 0x239c4) are not decompiled yet. lang.dat's pls_lex_list
 * lexicons still load, as empty ones, and text goes through unchanged: Amy's
 * pls_lexicon1/pls_lexicon2 aliases (band names and the like) are not applied. */
typedef struct tts_msg tts_msg;
void *pls_lexicon_new(void);

void *pls_lexicon_parse(const char *data, size_t size, void *ctx, tts_msg **msgs, int level)
{
    (void)data;
    (void)size;
    (void)ctx;
    (void)level;
    if (msgs)
        *msgs = NULL;
    return pls_lexicon_new();
}

void *pls_apply(void *lexicons, const void *text, int in_subdoc, const char *lang, int external)
{
    (void)lexicons;
    (void)text;
    (void)in_subdoc;
    (void)lang;
    (void)external;
    return NULL;
}
