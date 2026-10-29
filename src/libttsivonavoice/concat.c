/*
 * Unit concatenation (0x5ac78..0x5cf18): selects units for an utterance, applies the SSML
 * effects (pitch/duration stretch, compression, amplification) to their audio and joins
 * them into the output wave.
 */
#include <stdio.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_item.h"
#include "cst_utterance.h"
#include "cst_val.h"
#include <math.h>
#include "cst_wave.h"
#include "tts_track.h"
#include "tts_voxdb.h"
#include "tts_wavesynth.h"

cst_item *item_traverse(const cst_item *item, ...);
cst_features *get_param_features(const cst_features *f, const char *name, cst_features *def);
dynamic_voxdbs *val_dynamic_voxdbs(const cst_val *v);
diphonedb *dynamic_voxdbs_find(const dynamic_voxdbs *dv, int key);
int dynamic_voxdbs_has_unit(const dynamic_voxdbs *dv, const char *name);
track *vox_decoder_read_unit(vox_decoder *dec, unsigned int i, cst_features *m);
void wavesynth_nop(track **ctx, int from, int to);
int ivona_time_usec(void);
float metrics_add_time(cst_features *metrics, const char *name, int usec);
int metrics_add_int(cst_features *metrics, const char *name, int v);
void *val_void(const cst_val *v);
extern const int cst_val_type_userdata;
unit_selector *unit_selector_new(cst_utterance *utt);
void unit_selector_free(unit_selector *s);
void *val_userdata(const cst_val *v);
const unsigned char *val_binary_key(const cst_val *v);
unsigned char *utt_cache_key(const cst_utterance *u);
int blacklists_contain(const cst_val *l, int id);
void vox_unit_from_segment(const cst_item *seg, vox_unit *u);
unsigned char *cache_graph_key(const diphonedb *db, const vox_unit *units, int n,
                               unsigned int *len, const unsigned char *hkey, cst_features *m,
                               int tb_min_distance);
int *cache_decrypt(const unsigned char *data, unsigned int size, const void *ad, int adlen,
                   unsigned int *n, const unsigned char *key, cst_features *m);
unsigned char *cache_encrypt(const int *path, unsigned int n, const void *ad, int adlen,
                             unsigned int *size, const unsigned char *key, cst_features *m);
int *unit_select_viterbi(unit_selector *s, int beam, const cst_val *blacklist, cst_features *m,
                         unsigned int *n, int threads);

/* a cached graph search result, freed by its owner's callback */
typedef struct cache_blob {
    unsigned char *data;
    unsigned int size;
    void (*free)(struct cache_blob *b);
    int unused;
} cache_blob;

typedef void (*cache_get_fn)(void *user, const unsigned char *key, unsigned int keylen,
                             cache_blob *out);
typedef void (*cache_put_fn)(void *user, const unsigned char *key, unsigned int keylen,
                             const unsigned char *data, unsigned int size);
typedef unit_selector *(*diphone_graph_fn)(cst_utterance *utt, vox_unit *units, int n);

#define ITEM_TRAVERSE_PREV 0x103
#define ITEM_TRAVERSE_DAUGHTER 0x107
#define ITEM_TRAVERSE_PARENT 0x106

/* 0x5ac78 arm: the effect_whisper level of the word over a syllable's first (or, with last,
 * last) segment, falling back to the neighbouring segment's word; 0 if unset. */
int syl_whisper(const cst_item *syl, int last)
{
    const cst_item *s, *w;
    int r = 0;

    if (!last)
        s = item_as(item_daughter(syl), 6);
    else
        s = item_as(item_last_daughter(syl), 6);
    w = item_traverse(s, 4, ITEM_TRAVERSE_PARENT, ITEM_TRAVERSE_PARENT, 1, ITEM_TRAVERSE_PARENT, 0);
    if (w == NULL)
        w = item_traverse(last == 1 ? item_prev(s) : item_next(s), 4, ITEM_TRAVERSE_PARENT,
                          ITEM_TRAVERSE_PARENT, 1, ITEM_TRAVERSE_PARENT, 0);
    if (w && item_feat_present(w, "effect_whisper"))
        r = item_feat_int(w, "effect_whisper");
    return r;
}

/* a word's effect parameter times the utterance's */
/* the word over a segment */
#define SEG_WORD(s) \
    item_traverse(s, 4, ITEM_TRAVERSE_PARENT, ITEM_TRAVERSE_PARENT, 1, ITEM_TRAVERSE_PARENT, 0)

#define EFFECT(w, name) \
    (get_param_float(item_feats(w), name, 1.0f) * get_param_float(item_utt(w)->features, name, 1.0f))

/* 0x5ad84 arm: runs (or continues) unit selection for the diphones built so far: a new
 * selection builds the phone features of the diphones and asks the cache for the whole path,
 * else the voice's graph function builds the search graph; the Viterbi search then stores
 * each diphone's unit as its checksum and diphone features. A finished selection's path is
 * put into the cache. */
cst_utterance *concat_select(cst_utterance *utt)
{
    cache_blob blob = { 0 };
    void *user;
    cache_get_fn cache_get;
    cache_put_fn cache_put;
    const unsigned char *hkey;
    cst_features *metrics;
    int threads, t0, t1, i, n;
    unsigned char *key, *put;
    diphone_graph_fn graph;
    wavesynth *ws;
    unit_selector *sel;
    cst_item *first, *d;
    vox_unit *units;
    int *path;
    unsigned int npath, size;
    const cst_val *bl;
    const dynamic_voxdbs *dv;

    user = feat_present(utt->features, "cache_user_data")
        ? val_userdata(feat_val(utt->features, "cache_user_data")) : NULL;
    cache_get = feat_present(utt->features, "cache_get_func")
        ? (cache_get_fn)val_userdata(feat_val(utt->features, "cache_get_func")) : NULL;
    cache_put = feat_present(utt->features, "cache_put_func")
        ? (cache_put_fn)val_userdata(feat_val(utt->features, "cache_put_func")) : NULL;
    hkey = feat_present(utt->features, "cache_hash_key")
        ? val_binary_key(feat_val(utt->features, "cache_hash_key")) : NULL;
    metrics = get_param_features(utt->features, "core_metrics", NULL);
    threads = get_param_int(utt->features, "num_parallel_threads", 1);
    key = utt_cache_key(utt);
    t0 = metrics ? ivona_time_usec() : 0;
    graph = (diphone_graph_fn)val_userdata(feat_val(utt->features, "diphone_graph_func"));
    ws = val_wavesynth(feat_val(utt->features, "streamer"));
    sel = ws->selector;
    if (sel == NULL) {
        t1 = metrics ? ivona_time_usec() : 0;
        for (d = item_traverse(ws->word, ITEM_TRAVERSE_DAUGHTER, 4, 0); d; d = item_next(d))
            if (item_daughter(d))
                break;
        first = item_traverse(item_traverse(d, ITEM_TRAVERSE_DAUGHTER, ITEM_TRAVERSE_DAUGHTER, 6, 0),
                              ITEM_TRAVERSE_PREV, 7, ITEM_TRAVERSE_PARENT, 0);
        if (first == NULL) {
            units = cst_safe_alloc(1, sizeof(vox_unit));
            n = 0;
        } else {
            for (n = 0, d = first; (d = item_next(d)); n++)
                ;
            units = cst_safe_alloc(n + 2, sizeof(vox_unit));
            i = 0;
            d = first;
            do {
                vox_unit_from_segment(item_as(item_daughter(d), 6), &units[i]);
                if (item_next(d) == NULL) {
                    vox_unit_from_segment(item_as(item_last_daughter(d), 6), &units[i + 1]);
                    i += 2;
                } else
                    i++;
                d = item_next(d);
            } while (d);
            n = i;
        }
        if (metrics)
            metrics_add_time(metrics, "diphone_graph_build_phone_feats_time", ivona_time_usec() - t1);
        if (cache_get && hkey && key && n > 0) {
            ws->cache_key = cache_graph_key(ws->voxdbs->a, units, n, &ws->cache_key_len, hkey, metrics,
                                            get_param_int(utt->features, "tb_min_distance", -1));
            ws->path = cst_safe_alloc(n - 1, 2 * sizeof(int));
            if (metrics) {
                t1 = ivona_time_usec();
                cache_get(user, ws->cache_key, ws->cache_key_len, &blob);
                metrics_add_time(metrics, "diphone_graph_cache_get_time", ivona_time_usec() - t1);
            } else
                cache_get(user, ws->cache_key, ws->cache_key_len, &blob);
            path = cache_decrypt(blob.data, blob.size, ws->cache_key, ws->cache_key_len, &npath, key,
                                 metrics);
            if (path) {
                bl = feat_present(utt->features, "blacklist") ? feat_val(utt->features, "blacklist") : NULL;
                dv = val_dynamic_voxdbs(feat_val(utt->features, "dynamic_voxdbs"));
                for (i = 0; i < npath; i++) {
                    if (blacklists_contain(bl, path[2 * i + 1]) || !dynamic_voxdbs_find(dv, path[2 * i])) {
                        cst_free(path);
                        goto miss;
                    }
                }
                if (metrics)
                    metrics_add_int(metrics, "diphone_graph_cache_hits", 1);
                ws->selector = unit_selector_new(utt);
                ws->selector->done = 1;
                ws->sel_first = first;
                cst_free(units);
                goto apply;
            }
        }
    miss:
        if (metrics)
            metrics_add_int(metrics, "diphone_graph_cache_misses", 1);
        ws->selector = graph(utt, units, n);
        ws->sel_first = first;
        cst_free(units);
        sel = ws->selector;
    }
    path = unit_select_viterbi(sel, get_param_int(utt->features, "viterbi_beam", -1),
                               feat_present(utt->features, "blacklist") ? feat_val(utt->features, "blacklist") : NULL,
                               get_param_features(utt->features, "core_metrics", NULL), &npath, threads);
    if (cache_put && ws->cache_key && key) {
        if (ws->path == NULL) {
            ws->path = path;
            ws->path_len = npath;
        } else {
            memcpy(ws->path + 2 * ws->path_len, path, npath * 2 * sizeof(int));
            ws->path_len += npath;
        }
        if (ws->selector->done
            && (put = cache_encrypt(ws->path, ws->path_len, ws->cache_key, ws->cache_key_len, &size,
                                    key, metrics))) {
            if (metrics) {
                t1 = ivona_time_usec();
                cache_put(user, ws->cache_key, ws->cache_key_len, put, size);
                metrics_add_time(metrics, "diphone_graph_cache_put_time", ivona_time_usec() - t1);
            } else
                cache_put(user, ws->cache_key, ws->cache_key_len, put, size);
            cst_free(put);
        }
    }
apply:
    cst_free(key);
    if (npath && ws->sel_first) {
        i = 0;
        do {
            if (item_feat_present(ws->sel_first, "unit_included_in_path")) {
                item_set(ws->sel_first, "checksum", int_val(path[2 * i]));
                item_set(ws->sel_first, "diphone", int_val(path[2 * i + 1]));
                i++;
            }
            ws->sel_first = item_next(ws->sel_first);
        } while (i < npath && ws->sel_first);
    }
    cst_free(path);
    if (blob.free)
        blob.free(&blob);
    if (ws->selector->done || path == NULL) {
        unit_selector_free(ws->selector);
        ws->selector = NULL;
        ws->sel_first = NULL;
        cst_free(ws->path);
        cst_free(ws->cache_key);
        ws->cache_key = NULL;
        ws->cache_key_len = 0;
        ws->path = NULL;
        ws->path_len = 0;
    }
    if (metrics)
        metrics_add_time(metrics, "diphone_graph_time", ivona_time_usec() - t0);
    return utt;
}

/* 0x5b6d4 arm: fits a pause unit's audio (frames from..to) to the pause's duration, or
 * applies the word's pitch/duration stretch to a phone's frames (keeping the last keep
 * frames when retiming) and then compression and amplification to samples start..end. */
track *unit_apply_effects(const cst_item *seg, track *t, int from, int to, int start, int end,
                          int keep)
{
    const cst_item *s = item_as(seg, 6), *w;
    float dur, span, pitch, stretch, comp, amp, max, s1, s2, r;
    int span0, n, d, m, pitched;

    if (strcmp("pau", item_feat_string(s, "name")) == 0) {
        if (item_prev(s))
            dur = item_feat_float(s, "end") - item_feat_float(item_prev(s), "end");
        else
            dur = item_feat_float(s, "end");
        if (item_prev(s) && item_next(s))
            dur *= 0.5f;
        span = (float)track_span(t, from, to) / (float)t->sample_rate;
        if (span < dur)
            track_insert_silence(t, (from + to) / 2, (dur - span) * t->sample_rate);
        else
            track_retime_to(t, from, to, t->sample_rate * dur);
        return t;
    }
    span0 = track_span(t, from, to);
    w = item_traverse(s, 4, ITEM_TRAVERSE_PARENT, ITEM_TRAVERSE_PARENT, 1, ITEM_TRAVERSE_PARENT, 0);
    pitch = EFFECT(w, "pitch_stretch");
    stretch = EFFECT(w, "duration_stretch");
    comp = EFFECT(w, "compress");
    amp = EFFECT(w, "amplify");
    pitched = pitch != 1.0f;
    if (pitched)
        stretch /= pitch;
    if (stretch != 1.0f) {
        n = t->num_frames;
        d = 0;
        if (keep == 0) {
            track_retime(t, from, to, stretch);
            d = t->num_frames - n;
        } else {
            m = to - keep;
            if (from <= m) {
                s1 = (float)track_span(t, from, to) / t->sample_rate;
                s2 = (float)track_span(t, m + 1, to) / t->sample_rate;
                r = (stretch * s1 - s2) / (s1 - s2);
                if (r < 0)
                    r = stretch;
                track_retime(t, from, m, r);
                d = t->num_frames - n;
            }
        }
        to += d;
        if (to < from)
            to = from;
    }
    if (pitched)
        track_stretch(t, from, to, pitch);
    d = track_span(t, from, to) - span0 + end;
    if (d > start)
        end = d;
    else
        end = start;
    if (comp != 1.0f)
        track_compress(t, start, end, comp);
    if (amp != 1.0f) {
        max = feat_float(item_utt(w)->features, "param_amplify_max");
        track_gain(t, start, end, amp > max ? max : amp);
    }
    return t;
}

/* 0x5bc3c arm: synthesizes up to n samples (n < 0: all) of utt: builds diphone items for the
 * next words (relation 7), lets the unit selector pick their units, then concatenates the
 * units, silences and inline waves; marks gets the marks of the returned wave. */
cst_wave *concat_synth(cst_utterance *utt, int n, cst_val **marks)
{
    cst_features *metrics = get_param_features(utt->features, "core_metrics", NULL);
    int keep, units, joins, t0, checksum, idx, pchecksum, pidx, k, m, nf, mid, span, from, last,
        seg_ok, wl, wr, len, start, i, flen, nfr, sr;
    const dynamic_voxdbs *dv;
    wavesynth *ws;
    cst_relation *rel;
    cst_item *d, *s, *prev, *cur, *seg, *first, *lastseg, *w1, *w2;
    const diphonedb *db;
    const vox_unit_rec *rec;
    vox_decoder *dec;
    track *t;
    track *ctx[3];
    float d0, d1, half, a, b, x, p1, s1, p2, s2, amp, rate;
    char name[10];
    const cst_val *v;
    cst_wave *wave;
    short *f;

    if (feat_present(utt->features, "pitchmarks_on_fly") && feat_int(utt->features, "pitchmarks_on_fly"))
        keep = 3;
    else
        keep = 0;
    if (!feat_present(utt->features, "diphonedb"))
        return NULL;
    dv = val_dynamic_voxdbs(feat_val(utt->features, "dynamic_voxdbs"));
    if (!utt_relation_present(utt, 7))
        utt_relation_create(utt, 7);
    if (!feat_present(utt->features, "streamer")) {
        ws = new_wavesynth();
        ws->voxdbs = val_dynamic_voxdbs(feat_val(utt->features, "dynamic_voxdbs"));
        ws->word = relation_head(utt_relation(utt, 1));
        ws->seg = NULL;
        ws->wave->sample_rate = ws->voxdbs->a->db->sample_rate;
        feat_set(utt->features, "streamer", wavesynth_val(ws));
    }
    ws = val_wavesynth(feat_val(utt->features, "streamer"));
    if (ws == NULL)
        return NULL;
    rel = utt_relation(utt, 7);

    /* build diphones until there is something to synthesize */
    for (;;) {
        if ((ws->pos != 0
             || (ws->seg
                 && (item_feat_present(ws->seg, "diphone")
                     || item_feat_present(item_daughter(ws->seg), "silence")
                     || item_feat_present(item_daughter(ws->seg), "wave"))))
            && n > 0)
            break;
        if (ws->selector && !ws->selector->done) {
            concat_select(utt);
            continue;
        }
        if (ws->word == NULL)
            break;
        if (item_feat_present(ws->word, "silence") || item_feat_present(ws->word, "wave")) {
            d = relation_append(rel, NULL);
            item_add_daughter(d, ws->word);
            if (ws->seg == NULL)
                ws->seg = d;
        } else if (item_daughter(ws->word)) {
            for (s = item_traverse(ws->word, ITEM_TRAVERSE_DAUGHTER, 4, 0); s; s = item_next(s))
                if (item_daughter(s))
                    break;
            prev = item_prev(item_traverse(s, ITEM_TRAVERSE_DAUGHTER, ITEM_TRAVERSE_DAUGHTER, 6, 0));
            for (cur = item_next(prev); cur; cur = item_next(cur)) {
                snprintf(name, sizeof(name), "%s-%s", item_feat_string(prev, "name"),
                         item_feat_string(cur, "name"));
                d = relation_append(rel, NULL);
                item_set_string(d, "name", name);
                if (dynamic_voxdbs_has_unit(dv, name))
                    item_set_int(d, "unit_included_in_path", 1);
                item_add_daughter(d, prev);
                item_add_daughter(d, cur);
                if (ws->seg == NULL)
                    ws->seg = d;
                if (strcmp("pau", item_feat_string(cur, "name")) == 0)
                    break;
                prev = cur;
            }
            concat_select(utt);
            if (prev)
                ws->word = SEG_WORD(prev);
        }
        ws->word = item_next(ws->word);
    }

    units = 0;
    joins = 0;
    t0 = ivona_time_usec();
    while ((n < 0 || n > ws->pos) && ws->seg
           && (item_feat_present(ws->seg, "diphone")
               || !item_feat_present(ws->seg, "unit_included_in_path"))) {
        seg = ws->seg;
        if (item_feat_present(seg, "unit_included_in_path")) {
            checksum = val_int(item_feat(seg, "checksum"));
            idx = val_int(item_feat(seg, "diphone"));
        } else
            checksum = idx = 0;
        db = dynamic_voxdbs_find(ws->voxdbs, checksum);
        if (metrics && checksum) {
            prev = item_prev(ws->seg);
            if (item_feat_present(prev, "unit_included_in_path")) {
                pchecksum = val_int(item_feat(prev, "checksum"));
                pidx = val_int(item_feat(prev, "diphone"));
                units++;
                if (checksum == pchecksum ? pidx + 1 != idx : pchecksum != 0)
                    joins++;
            } else
                units++;
        }
        if (checksum) {
            rec = &db->units[idx];
            d0 = rec[0].dur / 1000.0f;
            d1 = rec[1].dur / 1000.0f;
            dec = db->dec;
            s = item_as(item_daughter(ws->seg), 6);
            if (s && (w1 = SEG_WORD(s))
                && strcmp(get_param_string(item_feats(w1), "interpret-as", ""), "expletive") == 0
                && (s = item_next(s)) && (w1 = SEG_WORD(s))
                && strcmp(get_param_string(item_feats(w1), "interpret-as", ""), "expletive") == 0) {
                /* bleep: 352 Hz half-sine periods over the diphone */
                sr = dec->db->sample_rate;
                x = (d0 + d1) * 0.5f * 352.0f;
                if (x - (int)x < 0.5f)
                    nfr = floorf(x);
                else
                    nfr = ceilf(x);
                flen = floorf(sr / 352);
                t = new_track();
                track_resize(t, nfr);
                for (i = 0, len = flen; i < nfr; i++, len += flen) {
                    t->pms[i] = len;
                    track_frame_alloc(t, i, flen);
                }
                if (flen > 0) {
                    f = t->frames[0];
                    for (i = 0; i < flen; i++)
                        f[i] = sinf(i * 3.14159265f / flen) * 2187.5f;
                }
                for (i = 1; i < nfr; i++)
                    memcpy(t->frames[i], t->frames[0], flen * sizeof(short));
                t->sample_rate = sr;
            } else if ((t = vox_decoder_read_unit(db->dec, idx, metrics)) == NULL)
                goto next;
            first = item_as(item_daughter(ws->seg), 6);
            lastseg = item_as(item_last_daughter(ws->seg), 6);
            if (strcmp("pau", item_feat_string(first, "name")) == 0
                || strcmp("pau", item_feat_string(lastseg, "name")) == 0)
                seg_ok = 1;
            else {
                w1 = SEG_WORD(first);
                p1 = EFFECT(w1, "pitch_stretch");
                s1 = EFFECT(w1, "duration_stretch");
                w2 = SEG_WORD(lastseg);
                p2 = EFFECT(w2, "pitch_stretch");
                s2 = EFFECT(w2, "duration_stretch");
                seg_ok = s1 != 1.0f || p1 != 1.0f || s2 != 1.0f || p2 != 1.0f;
            }
            if (seg_ok)
                t = track_segment(t);
            half = d0 * 0.5f;
            nf = t->num_frames;
            k = track_frame_at(t, half * t->sample_rate);
            if (k <= 0)
                mid = -1;
            else {
                a = (float)track_span(t, 0, k - 1) / t->sample_rate;
                b = (float)track_span(t, 0, k) / t->sample_rate;
                m = half <= (a + b) * 0.5 ? k - 1 : k;
                span = track_span(t, 0, m);
                if (seg_ok)
                    mid = (float)span;
                else
                    mid = half * t->sample_rate;
                unit_apply_effects(item_daughter(ws->seg), t, 0, m, 0, mid, 0);
                k = m + t->num_frames - nf;
                if (k < 0)
                    k = 0;
                mid += track_span(t, 0, k) - span;
                if (mid < 0)
                    mid = 0;
                last = t->num_frames - 1;
                from = last > k ? k + 1 : last;
                unit_apply_effects(item_last_daughter(ws->seg), t, from, last, mid,
                                   track_span(t, 0, last), keep);
            }
            seg = ws->seg;
            dec = db->dec;
            wl = syl_whisper(seg, 0);
            wr = syl_whisper(seg, 1);
            if (wl || wr) {
                ctx[1] = vox_decoder_read_unit(dec, idx - 1, metrics);
                ctx[2] = vox_decoder_read_unit(dec, idx + 1, metrics);
                ctx[0] = t;
                if (wl && wr)
                    wavesynth_nop(ctx, 0, t->num_frames - 1);
                else if (wl)
                    wavesynth_nop(ctx, 0, k);
                else if (wr)
                    wavesynth_nop(ctx, k < t->num_frames - 1 ? k + 1 : t->num_frames - 1, 0);
                delete_track(ctx[1]);
                delete_track(ctx[2]);
            }
            len = wavesynth_add_track(ws, t, 1);
            delete_track(t);
            if (mid == -1)
                mid = len / (d1 / d0 + 1.0f);
            s = item_as(item_daughter(ws->seg), 6);
            if (item_prev(s) == NULL)
                wavesynth_add_mark(ws, s, 0);
            wavesynth_add_mark(ws, item_next(s), mid - len + ws->total);
        } else if (item_feat_present(item_daughter(ws->seg), "silence")) {
            wavesynth_flush(ws);
            start = ws->total;
            rate = ws->wave ? ws->wave->sample_rate : 0;
            wavesynth_append(ws, NULL, item_feat_float(item_daughter(ws->seg), "silence") * rate);
            wavesynth_add_unit_marks(ws, item_as(item_daughter(ws->seg), 1), start, ws->total - 1);
        } else if (item_feat_present(item_daughter(ws->seg), "wave")) {
            d = item_daughter(ws->seg);
            amp = EFFECT(d, "amplify");
            wavesynth_flush(ws);
            v = item_feat(d, "wave");
            if (CST_VAL_TYPE(v) == cst_val_type_userdata)
                wave = copy_wave(val_void(v));
            else
                wave = val_wave(v);
            if (ws->wave && ws->wave->sample_rate && ws->wave->sample_rate != wave->sample_rate)
                cst_wave_resample(wave, ws->wave->sample_rate);
            amp = EFFECT(d, "amplify");
            if (amp != 1.0f)
                cst_wave_rescale(wave, amp);
            start = ws->total;
            wavesynth_append(ws, wave->samples, wave->num_samples);
            wavesynth_add_unit_marks(ws, item_as(d, 1), start, ws->total - 1);
            if (CST_VAL_TYPE(v) == cst_val_type_userdata)
                delete_wave(wave);
        } else {
            s = item_as(item_daughter(ws->seg), 6);
            if (item_prev(s) == NULL)
                wavesynth_add_mark(ws, s, 0);
            wavesynth_add_mark(ws, item_next(s), ws->total);
        }
    next:
        ws->seg = item_next(ws->seg);
    }
    metrics_add_time(metrics, "unit_concatenation_time", ivona_time_usec() - t0);
    metrics_add_int(metrics, "diphone_synth_units", units);
    metrics_add_int(metrics, "diphone_synth_joins", joins);
    wave = wavesynth_take(ws, n, marks);
    if (feat_present(utt->features, "resample_wave"))
        cst_wave_resample(wave, feat_int(utt->features, "resample_wave"));
    return wave;
}
