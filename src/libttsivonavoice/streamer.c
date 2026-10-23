/*
 * Streaming synthesis (0x3da98..0x4029c): tts_streamer_* runs a text or SSML document through the
 * synthesis pipeline in chunks, producing samples and marks.
 */
#include <limits.h>
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
#include "tts_synth.h"
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
int synth_iter_next(void *iter);
void utt_rewind(cst_utterance *utt, int pos);
void mark_queue_init(void *q, int arg);
void *audio_queue_new(void);
void audio_queue_free(void *q);
void audio_chunk_free(void *chunk, size_t size);
/* paces output to the requested speed (ratelimit.c) */
typedef struct rate_conv {
    int unk_0[3];
    int pending;            /* +0x0c */
} rate_conv;

rate_conv *rate_conv_new(float speed, int sample_rate);
void rate_conv_free(rate_conv *rc);
void rate_conv_reset(rate_conv *rc);
void rate_conv_add(rate_conv *rc, int samples);
int license_check(void *license);
cst_utterance *chunk_utterance(void *chunk);
void *synth_iter_ctx(void *iter);
cst_utterance *utt_synth_frontend(cst_utterance *utt, voice_reg *reg);
cst_wave *utt_synth_wave(cst_utterance *utt, int max_samples, cst_val **marks);
cst_features *tts_streamer_get_metrics(tts_streamer *st);
void delete_val(cst_val *v);
void feat_set_features(cst_features *f, const char *name, const cst_features *v);
void license_release(void *license);


/* an SSML <mark> waiting for its text position */
typedef struct ssml_mark {
    const char *name;
    int start;
    int end;
} ssml_mark;

ssml_mark *mark_queue_pop(void *q, int pos);
const char *phone_viseme(const char *phone);
cst_features *utt_take_metrics(cst_utterance *utt);
void ivona_mutex_lock(pthread_mutex_t *m);
void ivona_mutex_unlock(pthread_mutex_t *m);

void *tts_get_certificate(tts_instance *tts);
void *tts_find_license(tts_instance *tts);

/* a chunk of the input queued for synthesis (vector element) */
typedef struct stream_chunk {
    int unk_0[2];
    int start;              /* +0x08: text span */
    int end;
    int unk_10[2];
    tts_voice *voice;       /* +0x18 */
} stream_chunk;

/* tts_streamer.data */
typedef struct streamer {
    void *audio;            /* audio_queue */
    ivona_vector *chunks;   /* +0x04 */
    int pos;                /* +0x08: current chunk */
    cst_utterance *utt;     /* +0x0c */
    tts_voice *voice;       /* +0x10: voice of the current chunk */
    int done;               /* +0x14: samples produced from the current chunk */
    struct rate_conv *rate_conv; /* +0x18 */
    float speed;            /* +0x1c */
    ssml_doc *doc;          /* +0x20 */
    void *iter;             /* +0x24 */
    int marks[2];           /* +0x28: mark queue (SSML <mark>s by text position) */
    union {                 /* +0x30: text span of the last word mark */
        unsigned char last_span[8];
        struct {
            int last_start;
            int last_end;
        };
    };
    ivona_vector *waves;    /* +0x38: of tts_wave *, pieces of one tts_streamer_synth() */
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

static inline void wave_add_mark(tts_wave *w, enum tts_mark_type type, const char *name,
                                 unsigned int start, unsigned int end, unsigned int sample)
{
    tts_mark *m;

    if ((w->num_marks & 0x7f) == 0)
        w->marks = cst_safe_realloc(w->marks, w->num_marks + 0x80, sizeof(tts_mark));
    m = &w->marks[w->num_marks++];
    m->type = type;
    m->name = name;
    m->start_text_offset = start;
    m->end_text_offset = end;
    m->sample_offset = sample;
}

/* 0x3dfc8 arm: a tts_wave of w with marks for the synthesized marks list (voice and sentence
 * marks at the start of a chunk, then words, SSML <mark>s, visemes and pauses). */
tts_wave *streamer_marks(tts_streamer *st, cst_wave *w, const cst_val *marks)
{
    streamer *s = st->data;
    tts_wave *tw = cst_safe_calloc(1, sizeof(tts_wave));
    stream_chunk *chunk;
    const cst_val *p;
    audiotag *m;
    ssml_mark *sm;
    const char *v;
    int first, lim, start, end, sample;

    tw->data = w;
    tw->samples = w->samples;
    tw->sample_rate = w->sample_rate;
    tw->num_samples = w->num_samples;
    first = 0;
    for (p = marks; p; p = val_cdr(p)) {
        m = val_audiotag(val_car(p));
        if (m->item == NULL || !item_feat_present(m->item, "start_pos"))
            continue;
        first = item_feat_int(m->item, "start_pos");
        if (first >= 0)
            break;
    }
    if (s->done == 0) {
        chunk = vector_at(s->chunks, s->pos);
        if (s->pos == 0
            || ((stream_chunk *)vector_at(s->chunks, s->pos - 1))->voice != chunk->voice)
            wave_add_mark(tw, TTS_MARK_VOICE, chunk->voice->voice->name, 0, 0, 0);
        wave_add_mark(tw, TTS_MARK_SENTENCE, NULL, chunk->start, chunk->end, 0);
        memset(s->last_span, 0, sizeof(s->last_span));
    }
    for (p = marks; p; p = val_cdr(p)) {
        m = val_audiotag(val_car(p));
        s = st->data;
        if (m->sample < 0 || m->sample > (int)tw->num_samples)
            continue;
        if (m->flags & AUDIOTAG_WORD) {
            start = item_feat_int(m->item, "start_pos");
            end = item_feat_int(m->item, "end_pos");
            if (start >= 0 && end >= 0) {
                if (s->pos == 0 && s->last_start == 0 && m->item)
                    lim = item_prev(m->item) ? first : 0;
                else
                    lim = first;
                sample = m->sample;
                while ((sm = mark_queue_pop(s->marks, start)) != NULL) {
                    if (lim >= sm->end)
                        continue;
                    wave_add_mark(tw, TTS_MARK_SSML,
                                  (const char *)cst_strdup((const cst_string *)sm->name),
                                  sm->start, sm->end, sample);
                }
                if (start != s->last_start || end != s->last_end)
                    wave_add_mark(tw, TTS_MARK_WORD, NULL, start, end, m->sample);
                s->last_start = start;
                s->last_end = end;
            }
        }
        if (m->flags & AUDIOTAG_WORD_END) {
            start = item_feat_int(m->item, "end_pos");
            sample = m->sample;
            while ((sm = mark_queue_pop(s->marks, start)) != NULL) {
                if (first >= sm->end)
                    continue;
                wave_add_mark(tw, TTS_MARK_SSML,
                              (const char *)cst_strdup((const cst_string *)sm->name),
                              sm->start, sm->end, sample);
            }
        }
        if (m->flags & AUDIOTAG_PHONE) {
            v = phone_viseme(m->phone);
            if (v)
                wave_add_mark(tw, TTS_MARK_VISEME, v, 0, 0, m->sample);
        } else if (m->flags & AUDIOTAG_PAUSE)
            wave_add_mark(tw, TTS_MARK_VISEME, "sil", 0, 0, m->sample);
    }
    return tw;
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
    vector_free(s->waves);
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
    s->done = 0;
    if (!(speed >= 1.0f) && speed != -1.0f)
        speed = speed < 0.0f ? -1.0f : 1.0f;
    s->speed = speed;
    s->rate_conv = rate_conv_new(speed, rate);
    s->waves = vector_new(sizeof(void *), NULL, NULL);
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

/* tokenizes until chunk pos exists (or the input ends); -1 on error */
static inline int streamer_fill(streamer *s, size_t pos)
{
    cst_features *m;
    int t0, r;

    while (vector_size(s->chunks) <= pos) {
        t0 = s->metrics ? ivona_time_usec() : 0;
        r = synth_iter_next(s->iter);
        if ((m = s->metrics) != NULL)
            metrics_add_time(m, "ssml_tokenizer_time", ivona_time_usec() - t0);
        if (r == -1)
            return -1;
        if (r == 0)
            break;
    }
    return 0;
}

/* drops the current utterance (keeping its metrics) and moves to chunk pos */
static inline void streamer_seek(streamer *s, int pos)
{
    if (s->utt && s->metrics == NULL)
        s->metrics = utt_take_metrics(s->utt);
    delete_utterance(s->utt);
    s->pos = pos;
    s->utt = NULL;
    s->voice = NULL;
    s->done = 0;
    memset(s->last_span, 0, sizeof(s->last_span));
}

/* 0x3ecd8 arm: exported. Synthesizes up to num_samples samples into samples (with num_samples 0
 * and no buffer: the whole input, counting only). Returns the count, 0 at the end, or -1. */
int tts_streamer_synth0(tts_streamer *st, short *samples, int num_samples)
{
    cst_features *tm, *m;
    streamer *s;
    cst_utterance *utt;
    cst_wave *w;
    void *chunk;
    int all, total, remaining, n, ret, t_start, t_fe, t_be;

    if (st == NULL || num_samples < 0 || (samples != NULL && num_samples == 0)
        || (num_samples > 0 && samples == NULL)) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return -1;
    }
    all = num_samples == 0;
    tm = tts_streamer_get_metrics(st);
    t_start = tm ? ivona_time_usec() : 0;
    total = 0;
    remaining = num_samples;
    for (;;) {
        s = st->data;
        if (s == NULL) {
            tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
            ret = -1;
            break;
        }
        if (s->rate_conv == NULL || s->rate_conv->pending == 0)
            rate_conv_reset(s->rate_conv);
        if (s->license && !license_check(s->license)) {
            if (tts_errno() == TTS_STATUS_OK)
                tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "Limits verification error");
            ret = -1;
            break;
        }
    next_chunk:
        t_fe = ivona_time_usec();
        if (streamer_fill(s, s->pos) == -1) {
            ret = -1;
            break;
        }
        if ((int)vector_size(s->chunks) <= s->pos) {
            tts_set_error(TTS_STATUS_OK);
            ret = total;
            break;
        }
        utt = s->utt;
        if (utt == NULL) {
            chunk = vector_at(s->chunks, s->pos);
            utt = s->utt = chunk_utterance(chunk);
            if (s->metrics) {
                feat_set_features(utt->features, "core_metrics", s->metrics);
                utt = s->utt;
                s->metrics = NULL;
            }
            utt->unk_14 = synth_iter_ctx(s->iter);
            s->voice = ((stream_chunk *)chunk)->voice;
            utt = s->utt = utt_synth_frontend(s->utt, VOICE_DATA(s->voice)->reg);
            if (utt == NULL) {
                tts_set_error(TTS_STATUS_INTERNAL_ERROR);
                ret = -1;
                break;
            }
        }
        m = get_param_features(utt->features, "core_metrics", NULL);
        metrics_add_time(m, "frontend_time", ivona_time_usec() - t_fe);
        t_be = ivona_time_usec();
        feat_set_float(s->utt->features, "duration_stretch",
                       feat_float(VOICE_FEATURES(s->voice), "duration_stretch"));
        feat_set_float(s->utt->features, "amplify", feat_float(VOICE_FEATURES(s->voice), "amplify"));
        feat_set_int(s->utt->features, "num_parallel_threads",
                     get_param_int(VOICE_FEATURES(s->voice), "num_parallel_threads", 1));
        w = utt_synth_wave(s->utt, remaining, NULL);
        if (w != NULL) {
            if (w->num_samples <= 0) {
                delete_wave(w);
                delete_val(NULL);
            chunk_done:
                streamer_seek(s, s->pos + 1);
                metrics_add_time(m, "backend_time", ivona_time_usec() - t_be);
                goto next_chunk;
            }
            metrics_add_time(m, "backend_time", ivona_time_usec() - t_be);
            n = w->num_samples;
            if (samples) {
                memcpy(samples, w->samples, n * sizeof(short));
                delete_wave(w);
            }
        } else {
            if (remaining != 0)
                goto chunk_done;
            m = get_param_features(s->utt->features, "core_metrics", NULL);
            metrics_add_time(m, "backend_time", ivona_time_usec() - t_be);
            n = remaining;
        }
        delete_val(NULL);
        s->done += n;
        if (s->speed != -1.0f)
            rate_conv_add(s->rate_conv, n);
        if (n == 0) {
            tts_set_error(TTS_STATUS_OK);
            ret = total;
            break;
        }
        if (n < 0) {
            ret = n;
            break;
        }
        total += n;
        samples += n;
        if (!(num_samples > total || all)) {
            ret = total;
            break;
        }
        remaining = num_samples - total;
    }
    if (tm)
        metrics_add_time(tm, "tts_streamer_synth_time", ivona_time_usec() - t_start);
    return ret;
}

/* 0x3f2b0 arm: exported */
void tts_wave_delete(tts_wave *w)
{
    unsigned int i;

    if (w == NULL)
        return;
    if (w->data)
        delete_wave(w->data);
    else
        cst_free(w->samples);
    if (w->num_marks) {
        for (i = 0; i < w->num_marks; i++) {
            tts_mark *m = &w->marks[i];

            switch (m->type) {
            case TTS_MARK_WORD:
            case TTS_MARK_SENTENCE:
            case TTS_MARK_VISEME:
            case TTS_MARK_VOICE:
                break;
            default:
                cst_free((void *)m->name);
                break;
            }
        }
        cst_free(w->marks);
    }
    cst_free(w);
}

/* the pieces in s->waves as one wave (marks with copied names); NULL when out of memory */
static inline tts_wave *waves_merge(ivona_vector *waves, size_t count)
{
    tts_wave *m, *w;
    unsigned int i, j, off, mk;

    m = cst_safe_calloc(1, sizeof(tts_wave));
    if (m == NULL) {
        tts_set_errorf(TTS_STATUS_SYSTEM_ERROR, "Out of memory");
        return NULL;
    }
    m->sample_rate = (*(tts_wave **)vector_at(waves, 0))->sample_rate;
    for (i = 0; i < count; i++) {
        w = *(tts_wave **)vector_at(waves, i);
        m->num_samples += w->num_samples;
        m->num_marks += w->num_marks;
    }
    m->samples = cst_safe_calloc(m->num_samples, sizeof(short));
    if (m->samples == NULL || (m->num_marks
                               && (m->marks = cst_safe_calloc(m->num_marks, sizeof(tts_mark))) == NULL)) {
        tts_set_errorf(TTS_STATUS_SYSTEM_ERROR, "Out of memory");
        if (m->samples)
            cst_free(m->samples);
        if (m->marks)
            cst_free(m->marks);
        cst_free(m);
        return NULL;
    }
    off = 0;
    mk = 0;
    for (i = 0; i < count; i++) {
        w = *(tts_wave **)vector_at(waves, i);
        memcpy(m->samples + off, w->samples, w->num_samples * sizeof(short));
        for (j = 0; j < w->num_marks; j++) {
            tts_mark *d = &m->marks[mk + j], *src = &w->marks[j];

            d->type = src->type;
            switch (src->type) {
            case TTS_MARK_WORD:
            case TTS_MARK_SENTENCE:
            case TTS_MARK_VISEME:
            case TTS_MARK_VOICE:
                d->name = src->name;
                break;
            default:
                d->name = (const char *)cst_strdup((const cst_string *)src->name);
                break;
            }
            d->sample_offset = off + src->sample_offset;
            d->start_text_offset = src->start_text_offset;
            d->end_text_offset = src->end_text_offset;
        }
        off += w->num_samples;
        mk += w->num_marks;
    }
    return m;
}

/* tts_streamer_synth(st, 0): synthesizes one piece and drops it */
static inline tts_wave *streamer_skip(tts_streamer *st, cst_features *tm, int t_start)
{
    cst_features *m;
    streamer *s;
    cst_utterance *utt;
    cst_wave *w;
    void *chunk;
    int n, t_fe, t_be;

    s = st->data;
    if (s == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        goto out;
    }
    if (s->rate_conv == NULL || s->rate_conv->pending == 0)
        rate_conv_reset(s->rate_conv);
    if (s->license && !license_check(s->license)) {
        if (tts_errno() == TTS_STATUS_OK)
            tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "Limits verification error");
        goto out;
    }
next_chunk:
    t_fe = ivona_time_usec();
    if (streamer_fill(s, s->pos) == -1)
        goto out;
    if ((int)vector_size(s->chunks) <= s->pos) {
        tts_set_error(TTS_STATUS_OK);
        goto out;
    }
    utt = s->utt;
    if (utt == NULL) {
        chunk = vector_at(s->chunks, s->pos);
        utt = s->utt = chunk_utterance(chunk);
        if (s->metrics) {
            feat_set_features(utt->features, "core_metrics", s->metrics);
            utt = s->utt;
            s->metrics = NULL;
        }
        utt->unk_14 = synth_iter_ctx(s->iter);
        s->voice = ((stream_chunk *)chunk)->voice;
        utt = s->utt = utt_synth_frontend(s->utt, VOICE_DATA(s->voice)->reg);
        if (utt == NULL) {
            tts_set_error(TTS_STATUS_INTERNAL_ERROR);
            goto out;
        }
    }
    m = get_param_features(utt->features, "core_metrics", NULL);
    metrics_add_time(m, "frontend_time", ivona_time_usec() - t_fe);
    t_be = ivona_time_usec();
    feat_set_float(s->utt->features, "duration_stretch",
                   feat_float(VOICE_FEATURES(s->voice), "duration_stretch"));
    feat_set_float(s->utt->features, "amplify", feat_float(VOICE_FEATURES(s->voice), "amplify"));
    feat_set_int(s->utt->features, "num_parallel_threads",
                 get_param_int(VOICE_FEATURES(s->voice), "num_parallel_threads", 1));
    w = utt_synth_wave(s->utt, 0, NULL);
    if (w == NULL) {
        m = get_param_features(s->utt->features, "core_metrics", NULL);
        metrics_add_time(m, "backend_time", ivona_time_usec() - t_be);
        n = 0;
    } else if (w->num_samples > 0) {
        metrics_add_time(m, "backend_time", ivona_time_usec() - t_be);
        n = w->num_samples;
    } else {
        delete_wave(w);
        delete_val(NULL);
        streamer_seek(s, s->pos + 1);
        metrics_add_time(m, "backend_time", ivona_time_usec() - t_be);
        goto next_chunk;
    }
    delete_val(NULL);
    s->done += n;
    if (s->speed != -1.0f)
        rate_conv_add(s->rate_conv, n);
    if (n == 0)
        tts_set_error(TTS_STATUS_OK);
out:
    if (tm)
        metrics_add_time(tm, "tts_streamer_synth_time", ivona_time_usec() - t_start);
    return NULL;
}

/* 0x3f348 arm: exported. Synthesizes up to num_samples samples (all of the input when negative)
 * as a wave with marks. 0 synthesizes one piece and returns NULL. */
tts_wave *tts_streamer_synth(tts_streamer *st, int num_samples)
{
    cst_features *tm, *m;
    streamer *s;
    cst_utterance *utt;
    cst_wave *w;
    void *chunk;
    ivona_vector *waves;
    tts_wave *ret;
    struct {
        tts_wave *wave;
        cst_val *marks;
    } piece;
    int all, total, remaining, n, t_start, t_fe, t_be;
    size_t i, count;

    if (st == NULL || st->data == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return NULL;
    }
    tm = tts_streamer_get_metrics(st);
    t_start = tm ? ivona_time_usec() : 0;
    if (num_samples == 0)
        return streamer_skip(st, tm, t_start);
    s = st->data;
    piece.wave = NULL;
    piece.marks = NULL;
    total = 0;
    all = num_samples <= 0;
    remaining = num_samples;
    waves = s->waves;
    for (;;) {
        if (s->rate_conv == NULL || s->rate_conv->pending == 0)
            rate_conv_reset(s->rate_conv);
        if (s->license && !license_check(s->license)) {
            if (tts_errno() == TTS_STATUS_OK)
                tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "Limits verification error");
            goto fail;
        }
    next_chunk:
        t_fe = ivona_time_usec();
        if (streamer_fill(s, s->pos) == -1)
            goto fail;
        if ((int)vector_size(s->chunks) <= s->pos)
            goto end;
        utt = s->utt;
        if (utt == NULL) {
            chunk = vector_at(s->chunks, s->pos);
            utt = s->utt = chunk_utterance(chunk);
            if (s->metrics) {
                feat_set_features(utt->features, "core_metrics", s->metrics);
                utt = s->utt;
                s->metrics = NULL;
            }
            utt->unk_14 = synth_iter_ctx(s->iter);
            s->voice = ((stream_chunk *)chunk)->voice;
            utt = s->utt = utt_synth_frontend(s->utt, VOICE_DATA(s->voice)->reg);
            if (utt == NULL) {
                tts_set_error(TTS_STATUS_INTERNAL_ERROR);
                goto fail;
            }
        }
        m = get_param_features(utt->features, "core_metrics", NULL);
        metrics_add_time(m, "frontend_time", ivona_time_usec() - t_fe);
        t_be = ivona_time_usec();
        feat_set_float(s->utt->features, "duration_stretch",
                       feat_float(VOICE_FEATURES(s->voice), "duration_stretch"));
        feat_set_float(s->utt->features, "amplify", feat_float(VOICE_FEATURES(s->voice), "amplify"));
        feat_set_int(s->utt->features, "num_parallel_threads",
                     get_param_int(VOICE_FEATURES(s->voice), "num_parallel_threads", 1));
        w = utt_synth_wave(s->utt, remaining, &piece.marks);
        if (w == NULL) {
            if (remaining != 0)
                goto chunk_done;
            m = get_param_features(s->utt->features, "core_metrics", NULL);
            metrics_add_time(m, "backend_time", ivona_time_usec() - t_be);
            n = remaining;
        } else if (w->num_samples > 0) {
            metrics_add_time(m, "backend_time", ivona_time_usec() - t_be);
            n = w->num_samples;
            if (remaining)
                piece.wave = streamer_marks(st, w, piece.marks);
        } else {
            delete_wave(w);
            delete_val(piece.marks);
            piece.marks = NULL;
        chunk_done:
            streamer_seek(s, s->pos + 1);
            metrics_add_time(m, "backend_time", ivona_time_usec() - t_be);
            goto next_chunk;
        }
        delete_val(piece.marks);
        s->done += n;
        if (s->speed != -1.0f)
            rate_conv_add(s->rate_conv, n);
        if (n == 0)
            goto end;
        if (n < 0)
            goto fail;
        total += n;
        vector_push(waves, &piece);
        if (!(num_samples > total || all))
            break;
        s = st->data;
        piece.wave = NULL;
        piece.marks = NULL;
        remaining = num_samples - total;
        if (s == NULL) {
            tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
            goto fail;
        }
    }
    if (0) {
end:
        tts_set_error(TTS_STATUS_OK);
    }
    count = vector_size(waves);
    if (count == 0)
        ret = NULL;
    else if (count == 1) {
        ret = *(tts_wave **)vector_at(waves, 0);
        vector_pop_front(waves);
    } else
        ret = waves_merge(waves, count);
    goto done;
fail:
    ret = NULL;
done:
    for (i = 0; i < vector_size(waves); i++)
        tts_wave_delete(*(tts_wave **)vector_at(waves, i));
    vector_clear(waves);
    if (tm)
        metrics_add_time(tm, "tts_streamer_synth_time", ivona_time_usec() - t_start);
    return ret;
}

/* 0x40044 arm: exported. Skips n chunks (back for negative n); returns how far it moved. */
int tts_streamer_jump(tts_streamer *st, int n)
{
    streamer *s;
    int pos, limit, moved;

    if (st == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return 0;
    }
    s = st->data;
    pos = s->pos + n;
    if (n < 0) {
        if (s->pos >= pos)
            pos = pos < 0 ? 0 : pos;
        else
            pos = 0;
    } else if (n > 0) {
        limit = s->pos > pos ? INT_MAX : pos;
        pos = limit;
        if (streamer_fill(s, limit) == -1)
            return 0;
        limit = vector_size(s->chunks);
        if (pos >= limit)
            pos = limit;
    }
    moved = pos - s->pos;
    if (moved != 0) {
        if (s->utt && s->metrics == NULL)
            s->metrics = utt_take_metrics(s->utt);
        delete_utterance(s->utt);
        s->pos = pos;
        s->utt = NULL;
        s->voice = NULL;
    } else if (s->done) {
        if (s->utt)
            utt_rewind(s->utt, 0);
    } else
        goto done;
    s->done = 0;
    memset(s->last_span, 0, sizeof(s->last_span));
done:
    tts_set_error(TTS_STATUS_OK);
    return moved;
}
