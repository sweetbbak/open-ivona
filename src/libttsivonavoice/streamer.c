/*
 * Streaming synthesis (0x3da98..0x4029c): tts_streamer_* runs a text or SSML document through the
 * synthesis pipeline in chunks, producing samples and marks.
 */
#include <string.h>
#include <strings.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_string.h"
#include "cst_utterance.h"
#include "cst_wave.h"
#include "ivona_util.h"
#include "ivona_vector.h"
#include "tts_cert.h"
#include "tts_instance.h"
#include "tts_internal.h"
#include "tts_metrics.h"
#include "tts_voice.h"

typedef struct ivona_ustr {
    char *data;
    int len;
} ivona_ustr;

ivona_ustr *ustr_new(unsigned int pos, const char *s, size_t n);
void ivona_ustr_free(ivona_ustr *u);
ivona_ustr *ustr_decode_n(const char *s, size_t len, const char *encoding);
size_t (*charset_strlen_func(const char *encoding))(const char *s);
const char *charset_detect(const char *encoding, const char *s);
int text_is_ssml(const char *s, int len);

/* SSML document (ssml.c) */
typedef struct dom_node {
    void *root;
} dom_node;

typedef struct ssml_doc {
    int unk_0;
    dom_node *dom;          /* +0x04 */
    int unk_8[6];
    int is_ssml;            /* +0x20 */
    int unk_24;
    int unk_28;             /* +0x28 */
    int unk_2c;
    tts_msg *warnings;      /* +0x30: pending input warnings */
    tts_msg *warning;       /* +0x34: the one last returned */
} ssml_doc;

typedef struct dom_text {
    int unk_0[4];
    ivona_ustr *text;       /* +0x10 */
    int unk_14;
} dom_text;

ssml_doc *ssml_doc_new(void);
void ssml_doc_free(ssml_doc *doc);
void ssml_doc_reset(ssml_doc *doc);
void ssml_doc_set_voice(ssml_doc *doc, tts_voice *v);
int ssml_doc_prepare(ssml_doc *doc, tts_voice *v);
void ssml_doc_set_encoding(ssml_doc *doc, const char *encoding);
int ssml_parse(ssml_doc *doc, ivona_ustr *text, int a, int b);
dom_text *dom_text_new(void);
dom_node *dom_new(void);
void *dom_element_new(const char *name, int type);
void dom_set_attr(void *elem, const char *name, const char *value);
void dom_append_child(void *elem, void *child);

void *synth_iter_new(ssml_doc *doc, ivona_vector *voices, ivona_vector *chunks, void *audio);
void synth_iter_free(void *iter);
void mark_queue_init(void *q, int arg);
void *audio_queue_new(void);
void audio_queue_free(void *q);
void audio_chunk_free(void *chunk, size_t size);
void *rate_conv_new(float speed, int sample_rate);
void rate_conv_free(void *rc);
void license_release(void *license);
cst_features *utt_take_metrics(cst_utterance *utt);
void ivona_mutex_lock(pthread_mutex_t *m);
void ivona_mutex_unlock(pthread_mutex_t *m);

void *tts_get_certificate(tts_instance *tts);
void *tts_find_license(tts_instance *tts);
int cert_status(tts_cert_data *cert);
int cert_allows_voice(tts_cert_data *cert, const char *lector, const char *lang_codes, int custom);

/* tts_streamer.data */
typedef struct streamer {
    void *audio;            /* audio_queue */
    ivona_vector *chunks;   /* +0x04 */
    int unk_8;
    cst_utterance *utt;     /* +0x0c */
    int unk_10;
    int unk_14;
    void *rate_conv;        /* +0x18 */
    float speed;            /* +0x1c */
    ssml_doc *doc;          /* +0x20 */
    void *iter;             /* +0x24 */
    int marks[4];           /* +0x28: mark queue */
    ivona_vector *unk_38;
    void *license;          /* +0x3c */
    ivona_vector *voices;   /* +0x40: of tts_voice * */
    cst_features *metrics;  /* +0x44 */
} streamer;

/* 0x3da98 arm: a document of the plain text as one SSML <speak> in the first voice's language. */
int streamer_plain_text(ssml_doc **pdoc, ivona_vector *voices, const char *text, size_t len)
{
    tts_voice *v = *(tts_voice **)vector_at(voices, 0);
    const char *enc = get_param_string(VOICE_FEATURES(v), "eng_text_encoding", "UTF-8");
    dom_text *t = dom_text_new();

    t->text = ustr_new(0, text, len);
    t->unk_14 = 0;
    (*pdoc)->dom = dom_new();
    (*pdoc)->dom->root = dom_element_new("http://www.w3.org/2001/10/synthesis^speak", 0x11);
    dom_set_attr((*pdoc)->dom->root, "version", "1.1");
    dom_set_attr((*pdoc)->dom->root, "http://www.w3.org/XML/1998/namespace^lang",
                 v->voice->lang_codes);
    dom_append_child((*pdoc)->dom->root, t);
    (*pdoc)->is_ssml = 0;
    ssml_doc_set_encoding(*pdoc, charset_detect(enc, text));
    return 0;
}

/* Named waves (audio clips): a feature set whose keys are owned copies. */

/* 0x3dbbc arm */
cst_features *wavedict_new(void)
{
    return new_features();
}

/* 0x3dbc0 arm */
int wavedict_remove(cst_features *d, const char *name)
{
    cst_featvalpair *p;

    for (p = d->head; p; p = p->next)
        if (strcmp(name, p->name) == 0) {
            const char *key = p->name;

            feat_remove(d, key);
            cst_free((void *)key);
            return 1;
        }
    return 0;
}

/* 0x3dc24 arm */
void wavedict_clear(cst_features *d)
{
    const char *key;

    while (d->head) {
        key = d->head->name;
        feat_remove(d, key);
        cst_free((void *)key);
    }
}

/* 0x3dc60 arm */
void wavedict_free(cst_features *d)
{
    wavedict_clear(d);
    delete_features(d);
}

/* 0x3dc78 arm */
int wavedict_has(cst_features *d, const char *name)
{
    return feat_present(d, name);
}

/* 0x3dc84 arm: returns whether name was already present. */
int wavedict_set(cst_features *d, const char *name, cst_wave *w)
{
    if (wavedict_has(d, name)) {
        feat_set(d, name, wave_val(w));
        return 1;
    }
    feat_set(d, (const char *)cst_strdup((const cst_string *)name), wave_val(w));
    return 0;
}

/* 0x3dcf0 arm */
cst_wave *wavedict_get(cst_features *d, const char *name)
{
    const cst_val *v = get_param_val(d, name, NULL);

    if (v == NULL)
        return NULL;
    return val_wave(v);
}

/* SSML documents in s->doc; returns 0, the parse error, or 2 when the first voice rejects it */
static inline int streamer_parse_ssml(streamer *s, ivona_ustr *u)
{
    tts_voice *v = *(tts_voice **)vector_at(s->voices, 0);
    int ret;

    ssml_doc_set_voice(s->doc, v);
    ret = ssml_parse(s->doc, u, 0, 1);
    if (ret == 0) {
        s->doc->is_ssml = 1;
        ssml_doc_set_encoding(s->doc, "UTF-8");
    }
    return ret;
}

/* 0x3dd0c arm: parses text (in the first voice's input type and encoding) into s->doc and sets up
 * synthesis. Returns 0, or an error code. */
int streamer_set_text(streamer *s, const char *text)
{
    tts_voice *v = *(tts_voice **)vector_at(s->voices, 0);
    size_t len = charset_strlen_func(get_param_string(VOICE_FEATURES(v), "eng_text_encoding",
                                                      "UTF-8"))(text);
    tts_cert_data *cert = tts_get_certificate(v->tts);
    const char *type;
    ivona_ustr *u;
    int ret;

    if (cert->ssml
        && strcasecmp(type = get_param_string(VOICE_FEATURES(v), "eng_input_type", ""),
                      "text/plain") != 0) {
        if (strcasecmp(type, "application/ssml+xml") == 0) {
            u = ustr_decode_n(text, len,
                              get_param_string(VOICE_FEATURES(*(tts_voice **)vector_at(s->voices, 0)),
                                               "eng_text_encoding", "UTF-8"));
            ret = streamer_parse_ssml(s, u);
            ivona_ustr_free(u);
        } else {
            /* any other type: SSML if it looks like it and parses */
            u = ustr_decode_n(text, len,
                              get_param_string(VOICE_FEATURES(*(tts_voice **)vector_at(s->voices, 0)),
                                               "eng_text_encoding", "UTF-8"));
            if (!text_is_ssml(u->data, u->len)) {
                ivona_ustr_free(u);
                ssml_doc_reset(s->doc);
                ret = streamer_plain_text(&s->doc, s->voices, text, len);
            } else {
                ret = streamer_parse_ssml(s, u);
                ivona_ustr_free(u);
                if (ret == 1) {
                    ssml_doc_reset(s->doc);
                    ret = streamer_plain_text(&s->doc, s->voices, text, len);
                }
            }
        }
    } else
        ret = streamer_plain_text(&s->doc, s->voices, text, len);
    if (ret)
        return ret;
    if (ssml_doc_prepare(s->doc, v)) {
        s->iter = synth_iter_new(s->doc, s->voices, s->chunks, s->audio);
        mark_queue_init(s->marks, s->doc->unk_28);
        return 0;
    }
    return 2;
}

static inline void streamer_free(streamer *s)
{
    if (s->license)
        license_release(s->license);
    synth_iter_free(s->iter);
    ssml_doc_free(s->doc);
    vector_free(s->chunks);
    rate_conv_free(s->rate_conv);
    if (s->utt && s->metrics == NULL)
        s->metrics = utt_take_metrics(s->utt);
    delete_utterance(s->utt);
    s->utt = NULL;
    vector_free(s->unk_38);
    vector_free(s->voices);
    audio_queue_free(s->audio);
    delete_features(s->metrics);
    cst_free(s);
}

/* 0x3e5bc arm: exported. speed: 1 is real time and higher is faster, -1 as fast as possible;
 * capped by the certificate. */
tts_streamer *tts_streamer_start(tts_voice *v, const char *text, float speed)
{
    tts_cert_data *cert;
    tts_streamer *st;
    streamer *s;
    void *license;
    int metrics;
    int rate;

    if (v == NULL || text == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return NULL;
    }
    cert = tts_get_certificate(v->tts);
    if (cert_status(cert) != 0) {
        tts_set_error(TTS_STATUS_CERTIFICATE_EXPIRED);
        return NULL;
    }
    if (cert->max_speed != -1.0f && (cert->max_speed < speed || speed < 0.0f))
        speed = cert->max_speed;
    license = NULL;
    if (cert->license_id != -1) {
        license = tts_find_license(v->tts);
        if (license == NULL)
            return NULL;
    }
    metrics = strcmp(get_param_string(cert->attrs, "Extension-Metrics", ""), "TRUE");
    rate = tts_voice_sample_rate(v);
    s = cst_safe_calloc(1, sizeof(streamer));
    s->audio = audio_queue_new();
    s->chunks = vector_new(0x1c, NULL, audio_chunk_free);
    s->unk_14 = 0;
    if (!(speed >= 1.0f) && speed != -1.0f)
        speed = speed < 0.0f ? -1.0f : 1.0f;
    s->speed = speed;
    s->rate_conv = rate_conv_new(speed, rate);
    s->unk_38 = vector_new(sizeof(void *), NULL, NULL);
    s->license = license;
    s->voices = vector_new(sizeof(tts_voice *), NULL, NULL);
    s->metrics = metrics ? NULL : metrics_new();
    s->doc = ssml_doc_new();
    vector_push(s->voices, &v);
    if (streamer_set_text(s, text) != 0) {
        streamer_free(s);
        return NULL;
    }
    st = cst_safe_calloc(1, sizeof(tts_streamer));
    st->voice_ptr = v;
    st->data = s;
    return st;
}

static inline int voice_allowed(tts_cert_data *cert, tts_voice *v)
{
    return cert_allows_voice(cert, v->voice->lector, v->voice->lang_codes,
                             get_param_int(VOICE_FEATURES(v), "custom_voice", 0));
}

static inline int streamer_add(tts_streamer *st, tts_voice *v)
{
    streamer *s = st->data;
    tts_cert_data *cert = tts_get_certificate(st->voice_ptr->tts);
    tts_voice **p, **end;

    if (st->voice_ptr->sample_rate != v->sample_rate) {
        tts_set_errorf(TTS_STATUS_RANGE_ERROR, "Voices with different sample rate");
        return 0;
    }
    if (st->voice_ptr->tts != v->tts || cert == NULL || !voice_allowed(cert, st->voice_ptr)
        || !voice_allowed(cert, v)) {
        tts_set_errorf(TTS_STATUS_RANGE_ERROR, "Voices with different TTS instance");
        return 0;
    }
    for (p = s->voices->data, end = p + s->voices->count; p != end; p++)
        if (*p == v)
            return 1;
    vector_push(s->voices, &v);
    return 1;
}

/* 0x3e88c arm: exported. TTS_ALL_VOICES adds every loaded voice that fits. */
int tts_streamer_add_voice(tts_streamer *st, tts_voice *v)
{
    tts_instance *tts;
    tts_voice **p;

    if (st == NULL || v == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return 0;
    }
    if (v == TTS_ALL_VOICES) {
        tts = st->voice_ptr->tts;
        ivona_mutex_lock(tts->modules_lock);
        for (p = tts->modules->data; p != (tts_voice **)tts->modules->data + tts->modules->count;
             p++)
            streamer_add(st, *p);
        ivona_mutex_unlock(tts->modules_lock);
        return 1;
    }
    return streamer_add(st, v);
}

/* 0x3ec18 arm: exported */
int tts_streamer_stop(tts_streamer *st)
{
    if (st == NULL)
        return 1;
    if (st->data) {
        streamer_free(st->data);
        st->data = NULL;
    }
    cst_free(st);
    return 1;
}
