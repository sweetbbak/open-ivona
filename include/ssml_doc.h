#ifndef SSML_DOC_H
#define SSML_DOC_H

#include "cst_features.h"
#include "dom.h"
#include "ivona_util.h"
#include "ivona_vector.h"

/* An SSML document being read (ssml_doc.c, ssml_parse.c) */

typedef struct fetcher fetcher;
fetcher *fetcher_new(void);
void fetcher_delete(fetcher *f);
void fetcher_reset(fetcher *f);
void fetcher_set_referer(fetcher *f, const char *referer);
void fetcher_set_base(fetcher *f, const char *base);
void fetcher_set_callback(fetcher *f, void *fetch, void *user);
typedef struct pls_lexicon pls_lexicon;

/* a <lexicon> of the document */
typedef struct ssml_lexicon {
    char *uri;
    char *id;
    char *lang;                 /* +0x08 */
    int line;                   /* +0x0c: of the element */
    int unk_10;
    int column;                 /* +0x14 */
    int unk_18[3];
    int is_inline;              /* +0x24 */
    pls_lexicon *lexicon;       /* +0x28 */
} ssml_lexicon;

/* an <audio> of the document */
typedef struct ssml_audio {
    char *src;
    int unk_4[2];
    int pos;                    /* +0x0c: text position */
} ssml_audio;

typedef struct ssml_doc {
    fetcher *fetch;
    dom_doc *dom;               /* +0x04 */
    void *audio_attr_fn;        /* +0x08: eng_audio_attributes_function */
    void *audio_attr_data;      /* +0x0c */
    ivona_vector *lexicons;     /* +0x10: of ssml_lexicon */
    int ssml_version;           /* +0x14: 10 for SSML 1.0, 11 for 1.1, 0 if unknown */
    char *encoding;             /* +0x18 */
    int version;                /* +0x1c */
    int is_ssml;                /* +0x20 */
    int entity_limit;           /* +0x24 */
    ivona_vector *audio;        /* +0x28: of ssml_audio */
    cst_features *waves;        /* +0x2c */
    tts_msg *warnings;          /* +0x30: pending input warnings */
    tts_msg *warning;           /* +0x34: the one last returned */
    cst_features *metrics;      /* +0x38: timing metrics or NULL */
} ssml_doc;

#endif
