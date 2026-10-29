/*
 * Waveform synthesis state (0x62174..0x638e0): the output wave of an utterance, the
 * unit selector and the marks collected while concatenating. Compiled without inlining.
 */
#include <stdio.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_val.h"
#include "cst_item.h"
#include "cst_utterance.h"
#include "cst_val_defs.h"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include "cst_file.h"
#include "cst_wave.h"
#include "tts_track.h"
#include "tts_voxdb.h"

void unit_selector_free(unit_selector *s);
void *val_userdata(const cst_val *v);
cst_val *userdata_val(const void *v);
const unsigned char *val_binary_key(const cst_val *v);
int track_span(const track *t, int from, int to);
int wave_crossfade(short *dst, short n, const short *src, short len);
int ivona_time_usec(void);
float metrics_add_time(cst_features *metrics, const char *name, int usec);

#include "tts_wavesynth.h"

/* 0x62174 arm: writes a unit's context as a full-context label. */
void vox_unit_print(cst_file f, const vox_unit *u)
{
    cst_fprintf(f, "%f  ", u->dur);
    cst_fprintf(f, "%s^%s-%s+%s=%s@%d_%d/", u->phones[0]->name, u->phones[1]->name,
                u->phones[2]->name, u->phones[3]->name, u->phones[4]->name, u->f18, u->f19);
    cst_fprintf(f, "A:%d_%d_%d/", u->f1a, u->f1b, u->f1c);
    cst_fprintf(f, "B:%d-%d-%d@%d-%d&%d-%d#%d-%d$%d-%d!%d-%d;%d-%d|%s/", u->f1d, u->f1e,
                u->f1f[0], u->f1f[1], u->f1f[2], u->f1f[3], u->f1f[4], u->f1f[5], u->f1f[6],
                0, 0, u->f1f[7], u->f1f[8], 0, 0, u->ph30->name);
    cst_fprintf(f, "C:%d+%d+%d/", u->f34, u->f35, u->f36);
    cst_fprintf(f, "D:%s_%d/", u->ph38->name, u->f3c);
    cst_fprintf(f, "E:%s+%d@%d+%d&%d+%d#%d+%d/", u->ph40->name, u->f44, u->f45, u->f46,
                0, 0, 0, 0);
    cst_fprintf(f, "F:%s_%d/", u->ph48->name, u->f4c);
    cst_fprintf(f, "G:%d_%d/", u->f4d[0], u->f4d[1]);
    cst_fprintf(f, "H:%d=%d@%d=%d|%s/", u->f4d[2], u->f4d[3], u->f4d[4], u->f4d[5],
                u->ph54->name);
    cst_fprintf(f, "I:%d_%d/", u->f58, u->f59);
    cst_fprintf(f, "J:%d+%d-%d/", 0, 0, 0);
}

size_t phone_acoustic_data_serialize(char *out, const void *p);

/* appends a string with its NUL */
#define PUT_STR(s)                                                                    \
    do {                                                                              \
        size_t l_ = strlen(s) + 1;                                                    \
        memcpy(out + n, s, l_);                                                       \
        n += l_;                                                                      \
    } while (0)

/* 0x623a8 arm: writes a unit (acoustic data, duration in microseconds, context) to out;
 * returns the size (also if out is NULL). */
size_t vox_unit_serialize(char *out, const vox_unit *u)
{
    unsigned int us = u->dur * 1000.0f * 1000.0f;
    size_t n = phone_acoustic_data_serialize(out, u->unk_60);

    if (out == NULL) {
        n += 0x2c + strlen(u->phones[0]->name) + strlen(u->phones[1]->name) +
             strlen(u->phones[2]->name) + strlen(u->phones[3]->name) +
             strlen(u->phones[4]->name) + strlen(u->ph30->name) + strlen(u->ph38->name) +
             strlen(u->ph40->name) + strlen(u->ph48->name) + strlen(u->ph54->name);
    } else {
        memcpy(out + n, &us, sizeof us);
        n += sizeof us;
        PUT_STR(u->phones[0]->name);
        PUT_STR(u->phones[1]->name);
        PUT_STR(u->phones[2]->name);
        PUT_STR(u->phones[3]->name);
        PUT_STR(u->phones[4]->name);
        out[n] = u->f18;
        out[n + 1] = u->f19;
        out[n + 2] = u->f1a;
        out[n + 3] = u->f1b;
        out[n + 4] = u->f1c;
        out[n + 5] = u->f1d;
        out[n + 6] = u->f1e;
        out[n + 7] = u->f1f[0];
        out[n + 8] = u->f1f[1];
        out[n + 9] = u->f1f[2];
        out[n + 10] = u->f1f[3];
        out[n + 11] = u->f1f[4];
        out[n + 12] = u->f1f[5];
        out[n + 13] = u->f1f[6];
        out[n + 14] = u->f1f[7];
        out[n + 15] = u->f1f[8];
        n += 16;
        PUT_STR(u->ph30->name);
        out[n] = u->f34;
        out[n + 1] = u->f35;
        out[n + 2] = u->f36;
        n += 3;
        PUT_STR(u->ph38->name);
        out[n] = u->f3c;
        n += 1;
        PUT_STR(u->ph40->name);
        out[n] = u->f44;
        out[n + 1] = u->f45;
        out[n + 2] = u->f46;
        n += 3;
        PUT_STR(u->ph48->name);
        out[n] = u->f4c;
        out[n + 1] = u->f4d[0];
        out[n + 2] = u->f4d[1];
        out[n + 3] = u->f4d[2];
        out[n + 4] = u->f4d[3];
        out[n + 5] = u->f4d[4];
        out[n + 6] = u->f4d[5];
        n += 7;
        PUT_STR(u->ph54->name);
        out[n] = u->f58;
        out[n + 1] = u->f59;
        memcpy(out + n + 2, &u->f98, sizeof u->f98);
    }
    return n + 6;
}

/* 0x6277c arm: a short name for the unit ("0" for none): the centre phone with markers
 * and its stress; the first '0' is dropped. */
char *vox_unit_name(char *buf, size_t size, const vox_unit *u)
{
    const char *pre, *mid, *p;

    if (u == NULL) {
        snprintf(buf, size, "0");
        return buf;
    }
    pre = "";
    if (u->f18 == 1)
        pre = u->f1f[1] == 1 ? "#" : "~";
    mid = "";
    if (u->f1e != 0) {
        if (u->f18 != u->f1e)
            mid = "";
        else
            mid = "?";
    }
    snprintf(buf, size, "%s%s%s%d%s", pre, u->phones[2]->name, mid,
             strcmp(u->phones[2]->name, u->ph30->name) == 0 ? u->f1d : 0,
             u->f19 == 1 ? (u->f1f[2] == 1 ? "#" : "~") : "");
    p = strchr(buf, '0');
    if (p)
        memmove((char *)p, p + 1, buf + strlen(buf) - p);
    return buf;
}

/* 0x628e4 arm: speaking style by name (0: generic). */
int speaking_style_id(const char *name)
{
    if (name == NULL)
        return 0;
    if (strcmp(name, "ebook") == 0)
        return 2;
    if (strcmp(name, "news") == 0)
        return 1;
    if (strcmp(name, "speechcon") == 0)
        return 3;
    if (strcmp(name, "joke") == 0)
        return 4;
    return 0;
}

/* 0x62984 arm */
const char *speaking_style_name(int id)
{
    switch (id) {
    case 1:
        return "news";
    case 2:
        return "ebook";
    case 3:
        return "speechcon";
    case 4:
        return "joke";
    default:
        return "generic";
    }
}

/* 0x629f4 arm */
wavesynth_mark *wavesynth_mark_new(void)
{
    return cst_safe_alloc(1, sizeof(wavesynth_mark));
}

/* 0x62a00 arm */
void wavesynth_mark_free(wavesynth_mark *m)
{
    cst_free(m);
}

CST_VAL_REGISTER_TYPE_NODEL(wavesynth, wavesynth)

/* 0x62a74 arm */
wavesynth *new_wavesynth(void)
{
    wavesynth *s = cst_safe_calloc(1, sizeof(wavesynth));

    s->wave = new_wave();
    return s;
}

/* 0x62a98 arm */
void delete_wavesynth(wavesynth *s)
{
    const cst_val *l;

    if (s == NULL)
        return;
    if (s->selector)
        unit_selector_free(s->selector);
    if (s->wave)
        delete_wave(s->wave);
    if (s->marks) {
        for (l = s->marks; l; l = val_cdr(l))
            wavesynth_mark_free(val_userdata(val_car(l)));
        delete_val(s->marks);
    }
    cst_free(s);
}

/* 0x62b04 arm */
void val_delete_wavesynth(void *v)
{
    delete_wavesynth(v);
}

/* 0x62b08 arm: appends n samples (silence if samples is NULL). */
int wavesynth_append(wavesynth *s, const short *samples, int n)
{
    if (s->wave->num_samples - s->pos < n)
        cst_wave_resize(s->wave, s->wave->num_samples + n, 1);
    if (samples)
        memcpy(s->wave->samples + s->pos, samples, n * sizeof(short));
    else
        memset(s->wave->samples + s->pos, 0, n * sizeof(short));
    s->pos += n;
    s->total += n;
    s->pending = 0;
    return n;
}

/* 0x62b94 arm */
int wavesynth_flush(wavesynth *s)
{
    int n = s->pending;

    s->pos += n;
    s->total += n;
    s->pending = 0;
    return n;
}

#define WS_MAX(a, b) ((a) < (b) ? (b) : (a))

/* 0x62bc0 arm: appends a track's frames (cross-fading the first one into the pending
 * tail); with keep_last the last frame becomes the new pending tail. */
int wavesynth_add_track(wavesynth *s, const track *t, int keep_last)
{
    int n = 0, i, len, k, last;

    if (s->wave->num_samples - s->pos < track_span(t, 0, t->num_frames - 1))
        cst_wave_resize(s->wave,
                        WS_MAX(track_span(t, 0, t->num_frames - 1), s->wave->sample_rate * 5) +
                        s->wave->num_samples, 1);
    for (i = 0; last = t->num_frames - 1, i < last; i++) {
        len = track_frame_len(t, i);
        if (i == 0 && s->pending > 0)
            k = wave_crossfade(s->wave->samples + s->pos + n, s->pending, t->frames[0], len);
        else {
            memcpy(s->wave->samples + s->pos + n, t->frames[i], len * sizeof(short));
            k = len;
        }
        n += k;
    }
    s->pos += n;
    if (keep_last) {
        len = track_frame_len(t, last);
        memcpy(&s->wave->samples[s->pos], t->frames[t->num_frames - 1],
               (s->pending = len) * sizeof(short));
    } else
        s->pending = 0;
    s->total += n;
    return n;
}

/* 0x62d78 arm */
void wavesynth_add_mark(wavesynth *s, const cst_item *item, int start)
{
    wavesynth_mark *m = wavesynth_mark_new();
    cst_val *l;

    m->a = (cst_val *)item;
    m->b = NULL;
    l = s->marks;
    m->start = start;
    m->end = 0;
    if (l)
        s->marks = val_append(l, cons_val(userdata_val(m), NULL));
    else
        s->marks = cons_val(userdata_val(m), l);
}

/* 0x62de0 arm: marks a unit start..end (and the end of the last unit). */
void wavesynth_add_unit_marks(wavesynth *s, const cst_item *item, int start, int end)
{
    int e = end >= start ? end : start;
    wavesynth_mark *m = wavesynth_mark_new();
    const cst_item *next;

    m->b = (cst_val *)item;
    m->a = NULL;
    m->start = start;
    m->end = e;
    m->unit = 1;
    if (s->marks)
        s->marks = val_append(s->marks, cons_val(userdata_val(m), NULL));
    else
        s->marks = cons_val(userdata_val(m), s->marks);
    next = item_next(item);
    if (next)
        return;
    m = wavesynth_mark_new();
    m->a = (cst_val *)next;
    m->b = (cst_val *)item;
    m->unit = (int)next;
    m->start = e;
    m->end = e;
    if (s->marks)
        s->marks = val_append(s->marks, cons_val(userdata_val(m), next));
    else
        s->marks = cons_val(userdata_val(m), s->marks);
}

/* 0x62ec0 arm: takes up to max (0: all) finished samples as a new wave; marks (optional)
 * gets the marks inside it, relative to its start. */
cst_wave *wavesynth_take(wavesynth *s, int max, cst_val **marks)
{
    int n = s->pos, off;
    cst_wave *w;
    cst_val *out = NULL, *l;
    wavesynth_mark *m;

    if (n == 0)
        return NULL;
    if (max > 0) {
        if (n >= max)
            n = max;
    }
    w = new_wave();
    w->sample_rate = s->wave->sample_rate;
    cst_wave_resize(w, n, 1);
    memcpy(w->samples, s->wave->samples, n * sizeof(short));
    if (s->pos - n > 0 || s->pending > 0)
        memmove(s->wave->samples, s->wave->samples + n,
                (s->wave->num_samples - n) * sizeof(short));
    s->pos -= w->num_samples;
    while (s->marks &&
           (m = val_userdata(val_car(s->marks)))->start < s->total - s->pos) {
        l = s->marks;
        s->marks = (cst_val *)val_cdr(l);
        if (s->marks)
            set_cdr(l, NULL);
        if (marks) {
            off = s->total - s->pos - w->num_samples;
            m->start -= off;
            m->end -= off;
            out = cons_val(val_car(l), out);
        } else
            wavesynth_mark_free(m);
        delete_val(l);
    }
    if (marks)
        *marks = val_reverse(out);
    return w;
}

/* 0x630c4 arm */
void wavesynth_nop(void)
{
}

const unsigned char cache_key_secret[32] = {
    0x48, 0x70, 0x5e, 0xe3, 0xa3, 0x3f, 0xa8, 0x3f, 0x43, 0x21, 0xa1, 0xf5,
    0xbd, 0x09, 0x75, 0x34, 0x61, 0xa9, 0xe0, 0x86, 0xb5, 0x58, 0xde, 0x2d,
    0x91, 0x82, 0x0a, 0x1e, 0x04, 0x3f, 0x31, 0xd4
};
int cache_key_secret_len = 32;

/* 0x637f8 arm: the key that encrypts the voice's unit cache, derived from its
 * "cache_encrypt_key" (NULL if it has none). */
unsigned char *utt_cache_key(const cst_utterance *u)
{
    const unsigned char *k;
    const EVP_MD *md;
    unsigned char *out = NULL;
    unsigned int len;
    HMAC_CTX ctx;

    if (!feat_present(u->features, "cache_encrypt_key") ||
        (k = val_binary_key(feat_val(u->features, "cache_encrypt_key"))) == NULL)
        return out;
    md = EVP_sha256();
    len = EVP_MD_size(md);
    out = cst_safe_alloc(len, 1);
    HMAC_CTX_init(&ctx);
    HMAC_Init_ex(&ctx, cache_key_secret, cache_key_secret_len, md, NULL);
    HMAC_Update(&ctx, k, EVP_CIPHER_key_length(EVP_aes_256_cbc()));
    HMAC_Final(&ctx, out, &len);
    HMAC_CTX_cleanup(&ctx);
    return out;
}

/* 0x630c8 arm: checks the HMAC-SHA256 that ends data (over ad and the ciphertext) and
 * decrypts the AES-256-CBC ciphertext (IV first) with key; *n gets the plaintext size in
 * 8-byte records. NULL (and *n = 0) on failure. */
unsigned char *cache_decrypt(const unsigned char *data, unsigned int size, const void *ad, int adlen,
                             unsigned int *n, const unsigned char *key, cst_features *m)
{
    int maclen = EVP_MD_size(EVP_sha256());
    int t0 = m ? ivona_time_usec() : 0;
    const EVP_CIPHER *cipher;
    EVP_CIPHER_CTX *c;
    unsigned char *out;
    unsigned int len;
    int r, bs, total;

    if (data != NULL && size > (unsigned int)maclen) {
        {
            unsigned char *mac = cst_safe_alloc(maclen, 1);
            int body = size - maclen;
            const EVP_MD *md = EVP_sha256();
            HMAC_CTX ctx;

            HMAC_CTX_init(&ctx);
            HMAC_Init_ex(&ctx, cache_key_secret, cache_key_secret_len, md, NULL);
            HMAC_Update(&ctx, ad, adlen);
            HMAC_Update(&ctx, data, body);
            HMAC_Final(&ctx, mac, &len);
            HMAC_CTX_cleanup(&ctx);
            r = memcmp(mac, data + size - len, len);
            cst_free(mac);
        }
        if (r == 0) {
            int outl;

            cipher = EVP_aes_256_cbc();
            bs = EVP_CIPHER_block_size(cipher);
            size = size - bs - len;
            out = cst_safe_alloc(size, 1);
            c = EVP_CIPHER_CTX_new();
            if (c == NULL) {
                cst_free(out);
                *n = 0;
                return NULL;
            }
            if (EVP_DecryptInit_ex(c, cipher, NULL, key, data) != 1) {
                cst_free(out);
                EVP_CIPHER_CTX_free(c);
                *n = 0;
                return NULL;
            }
            if (EVP_DecryptUpdate(c, out, &outl, data + bs, size) != 1) {
                cst_free(out);
                EVP_CIPHER_CTX_free(c);
                *n = 0;
                return NULL;
            }
            total = outl;
            if (EVP_DecryptFinal_ex(c, out + outl, &outl) != 1) {
                cst_free(out);
                EVP_CIPHER_CTX_free(c);
                *n = 0;
                return NULL;
            }
            total += outl;
            EVP_CIPHER_CTX_free(c);
            *n = (unsigned int)total / 8;
            if (m)
                metrics_add_time(m, "diphone_graph_cache_decrypt_time", ivona_time_usec() - t0);
            return out;
        }
    }
    *n = 0;
    return NULL;
}
