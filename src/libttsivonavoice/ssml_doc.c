/*
 * The state of one SSML document being read (0x2842c..0x28fcc).
 */
#include <string.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_string.h"
#include "ivona_util.h"
#include "ivona_vector.h"
#include "tts_voice.h"

typedef struct fetcher fetcher;
fetcher *fetcher_new(void);
void fetcher_delete(fetcher *f);
void fetcher_reset(fetcher *f);
typedef struct dom_doc dom_doc;
void dom_free(dom_doc *d);
cst_features *wavedict_new(void);
void wavedict_clear(cst_features *d);
void wavedict_free(cst_features *d);
typedef struct pls_lexicon pls_lexicon;
void pls_lexicon_free(pls_lexicon *l);

/* a <lexicon> of the document */
typedef struct ssml_lexicon {
    char *uri;
    char *id;
    char *type;
    int unk_c[7];
    pls_lexicon *lexicon;       /* +0x28 */
} ssml_lexicon;

/* an <audio> of the document */
typedef struct ssml_audio {
    char *src;
    int unk_4[3];
} ssml_audio;

typedef struct ssml_doc {
    fetcher *fetch;
    dom_doc *dom;               /* +0x04 */
    int unk_8[2];
    ivona_vector *lexicons;     /* +0x10: of ssml_lexicon */
    int unk_14;
    char *encoding;             /* +0x18 */
    int version;                /* +0x1c */
    int is_ssml;                /* +0x20 */
    int entity_limit;           /* +0x24 */
    ivona_vector *audio;        /* +0x28: of ssml_audio */
    cst_features *waves;        /* +0x2c */
    tts_msg *warnings;          /* +0x30: pending input warnings */
    tts_msg *warning;           /* +0x34: the one last returned */
    void *engine;               /* +0x38 */
} ssml_doc;

/* 0x2842c arm */
static void ssml_audio_free(void *p, size_t size)
{
    cst_free(((ssml_audio *)p)->src);
}

/* 0x28434 arm */
static void ssml_lexicon_free(void *p, size_t size)
{
    ssml_lexicon *l = p;

    cst_free(l->uri);
    cst_free(l->id);
    cst_free(l->type);
    pls_lexicon_free(l->lexicon);
}

/* 0x28bac arm */
ssml_doc *ssml_doc_new(void *engine)
{
    ssml_doc *d = cst_safe_calloc(1, sizeof(ssml_doc));

    d->unk_14 = 0;
    d->dom = NULL;
    d->lexicons = vector_new(sizeof(ssml_lexicon), (ivona_vector_copy_fn)memcpy, ssml_lexicon_free);
    d->waves = wavedict_new();
    d->audio = vector_new(sizeof(ssml_audio), (ivona_vector_copy_fn)memcpy, ssml_audio_free);
    d->fetch = fetcher_new();
    d->engine = engine;
    d->is_ssml = 1;
    d->version = 2;
    return d;
}

/* 0x28c40 arm */
void ssml_doc_free(ssml_doc *d)
{
    if (d == NULL)
        return;
    wavedict_free(d->waves);
    vector_free(d->audio);
    tts_msg_free(d->warnings);
    tts_msg_free(d->warning);
    fetcher_delete(d->fetch);
    dom_free(d->dom);
    vector_free(d->lexicons);
    cst_free(d->encoding);
    cst_free(d);
}

/* 0x28c98 arm */
void ssml_doc_reset(ssml_doc *d)
{
    fetcher_reset(d->fetch);
    dom_free(d->dom);
    d->dom = NULL;
    vector_clear(d->lexicons);
    d->unk_14 = 0;
    cst_free(d->encoding);
    d->is_ssml = 1;
    d->encoding = NULL;
    vector_clear(d->audio);
    wavedict_clear(d->waves);
    tts_msg_free(d->warnings);
    d->warnings = NULL;
    tts_msg_free(d->warning);
    d->warning = NULL;
}

/* 0x28d04 arm */
void ssml_doc_set_voice(ssml_doc *d, tts_voice *v)
{
    d->entity_limit = get_param_int(VOICE_FEATURES(v), "eng_xml_entity_expansion_limit", 100);
}

/* 0x28f9c arm */
void ssml_doc_set_encoding(ssml_doc *d, const char *encoding)
{
    if (d->encoding)
        cst_free(d->encoding);
    d->encoding = encoding ? cst_strdup(encoding) : NULL;
}
