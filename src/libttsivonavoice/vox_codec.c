/*
 * Vox database unit decoders (0x6b084..0x6c444). The codec factory (vox_db_open) picks one
 * function set from the vox header's codec id (+0x28): 8/12 Vorbis (Tremor), 14 SILK.
 * See docs/notes/vox-codecs.md.
 */
#include <string.h>
#include "cst_alloc.h"
#include "silk.h"
#include "tts_track.h"

/* codec object built by the factory from the vox header (partly known) */
typedef struct vox_codec_info {
    int unk_0;
    char *name;                 /* +0x04 */
    int unk_8;
    int sample_rate;            /* +0x0c: header +0x24 */
    int key;                    /* +0x10: header +0x2c, obfuscates packet payloads */
} vox_codec_info;

/* a decoder instance */
typedef struct vox_decoder {
    const vox_codec_info *info;
    int unk_4;
    void *priv;                 /* +0x08: codec state */
} vox_decoder;

#define SILK_MAX_BYTES 0x1400
#define SILK_MAX_SAMPLES 9600

/* one packet of a unit */
typedef struct silk_packet {
    short nBytes;
    short unused;
    unsigned char payload[SILK_MAX_BYTES];
} silk_packet;

/* codec 14 state */
typedef struct silk_unit_decoder {
    void *dec;                          /* SILK decoder state */
    SKP_SILK_SDK_DecControlStruct ctrl;
    unsigned char key[8];               /* info->key, then each packet's clear tail bytes */
} silk_unit_decoder;

/* 0x6b1a8 arm: codec 2, raw 16-bit PCM: the number of pitch periods n, n cumulative period end
 * offsets, then the samples. */
track *pcm_unit_decode(vox_decoder *d, const unsigned char *data, unsigned int size)
{
    const unsigned short *p = (const unsigned short *)data;
    track *t = new_track();
    const unsigned short *src;
    short *dst;
    int i;

    (void)d;
    (void)size;
    track_resize(t, p[0]);
    for (i = 0; i < p[0]; i++) {
        t->pms[i] = p[i + 1];
        track_frame_alloc(t, i, track_frame_len(t, i));
        dst = t->frames[i];
        src = p + p[0] + 1 + (i ? t->pms[i - 1] : 0);
        memcpy(dst, src, track_frame_len(t, i) * sizeof(short));
    }
    return t;
}

static inline silk_unit_decoder *silk_decoder_new(int rate, int key)
{
    silk_unit_decoder *s;
    int size;

    s = cst_safe_alloc(1, sizeof(silk_unit_decoder));
    if (rate == 22050)
        rate = 24000;
    s->ctrl.API_sampleRate = rate;
    s->ctrl.framesPerPacket = 1;
    if (SKP_Silk_SDK_Get_Decoder_Size(&size) == 0 && (s->dec = cst_safe_alloc(size, 1)) != NULL) {
        s->key[0] = key & 0xff;
        s->key[1] = (key >> 8) & 0xff;
        s->key[2] = (key >> 16) & 0xff;
        s->key[3] = (key >> 24) & 0xff;
        return s;
    }
    cst_free(s);
    return NULL;
}

/* 0x6b260 arm: codec 14 open. 22050 Hz voices are decoded at SILK's 24000 Hz. */
int silk_unit_open(vox_decoder *d)
{
    if (d == NULL)
        return 0;
    d->priv = silk_decoder_new(d->info->sample_rate, d->info->key);
    return d->priv != NULL;
}

/* 0x6b320 arm: codec 14 close. */
void silk_unit_close(vox_decoder *d)
{
    silk_unit_decoder *s;

    if (d == NULL || (s = d->priv) == NULL)
        return;
    if (s->dec)
        cst_free(s->dec);
    cst_free(s);
}

/* 0x6b354 arm: codec 14: decodes one unit, a run of packets (u16 length + payload), into
 * out_len samples, dropping the first skip samples; the rest of out is zeroed.
 * Payloads are XORed with key[]: the 4 key bytes, the packet's clear tail (n % 4 bytes, for
 * packets over 4 bytes) and zeros. Returns 0, or -1..-5 (init, truncated length, packet too big,
 * truncated packet, decode error). */
int silk_unit_decode(vox_decoder *d, const unsigned char *data, unsigned int data_len, unsigned int skip,
                     short *samples, unsigned int n_samples)
{
    silk_unit_decoder *s = d->priv;
    short nBytes, len;
    silk_packet pkt;
    short out[SILK_MAX_SAMPLES];
    unsigned int off, pos, tot, i, k, tail, first;
    int n, count;

    if (SKP_Silk_SDK_InitDecoder(s->dec) != 0)
        return -1;
    tot = 0;
    for (off = 0; off < data_len; off = pos + n) {
        pos = off + 2;
        if (data_len < pos)
            return -2;
        memcpy(&nBytes, data + off, 2);
        pkt.nBytes = nBytes;
        n = nBytes;
        if (n > SILK_MAX_BYTES)
            return -3;
        if (data_len < pos + n)
            return -4;
        memcpy(pkt.payload, data + pos, n);
        k = 4;
        tail = 0;
        if ((unsigned int)n > 4) {
            for (i = n & ~3; i < (unsigned int)n; i++)
                s->key[k++] = pkt.payload[i];
            tail = n - (n & ~3);
        }
        for (; k < 8; k++)
            s->key[k] = 0;
        for (i = 0; i < n - tail; i++)
            pkt.payload[i] ^= s->key[i % k];
        do {
            if (SKP_Silk_SDK_Decode(s->dec, &s->ctrl, 0, pkt.payload, pkt.nBytes, out, &len) != 0)
                return -5;
            first = 0;
            if (skip) {
                first = len;
                if (skip < first)
                    first = skip;
                skip -= first;
            }
            count = len - first;
            if (count >= (int)(n_samples - tot))
                count = n_samples - tot;
            if (count > 0)
                memcpy(samples + tot, out + first, count * sizeof(short));
            tot += count;
        } while (s->ctrl.moreInternalDecoderFrames);
    }
    if (tot < n_samples)
        memset(samples + tot, 0, (n_samples - tot) * sizeof(short));
    return 0;
}
