/*
 * Vox unit databases (0x5e4f0..0x615d0): the vox file header and section table, the
 * decoder, unit record decoding and unit selection. Compiled without inlining (see config).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_file.h"
#include "cst_string.h"
#include "cst_val.h"
#include "cst_utterance.h"
#include "cst_val_defs.h"
#include "tts_track.h"
#include "tts_voxdb.h"

#define VOX_MAGIC_CDXV 0x56584443
#define VOX_MAGIC_DDXV 0x56584444

int blacklists_contain(const cst_val *l, int id);
int metrics_add_int(cst_features *metrics, const char *name, int v);
int metrics_max_int(cst_features *metrics, const char *name, int v);
float metrics_set_float(cst_features *metrics, const char *name, float v);
float acoustic_stub_1(const void *a, const void *b, int x);

int ivona_is_dir(const char *path);
int ivona_time_usec(void);
float metrics_add_time(cst_features *metrics, const char *name, int usec);

typedef track *(*vox_decode_fn)(vox_decoder *d, const unsigned char *data, unsigned int size);
typedef int (*vox_decode2_fn)(vox_decoder *d, const unsigned char *data, unsigned int size,
                              int n_packets, short *out, int n_samples);
cst_val *zdur_cart_val(const unsigned char **tables);
cst_val *dynamic_voxdbs_val(const dynamic_voxdbs *v);
dynamic_voxdbs *dynamic_voxdbs_new(diphonedb *a);

int pcm_unit_decode();
int vorbis_unit_open(vox_decoder *d);
void vorbis_unit_close(vox_decoder *d);
int vorbis_unit_decode();
int vorbis_ogg_unit_open(vox_decoder *d);
void vorbis_ogg_unit_close(vox_decoder *d);
int vorbis_ogg_unit_decode();
int silk_unit_open(vox_decoder *d);
void silk_unit_close(vox_decoder *d);
int silk_unit_decode();

CST_VAL_USER_TYPE_DCLS(diphonedb, diphonedb)
CST_VAL_USER_TYPE_DCLS(dynamic_voxdbs, dynamic_voxdbs)
CST_VAL_REGISTER_TYPE_NODEL(diphonedb, diphonedb)

/* 0x5e560 arm: index of the unit named name, or -1. */
int diphonedb_unit_index(const diphonedb *d, const char *name)
{
    unsigned int lo = 0, hi = d->n_names, mid;
    int c;

    while (lo < hi) {
        mid = lo + (hi - lo) / 2;
        c = strcmp(name, (const char *)d->names + mid * 20);
        if (c < 0)
            hi = mid;
        else if (c == 0)
            return mid;
        else
            lo = mid + 1;
    }
    return -1;
}

/* 0x5e5c8 arm: decodes a unit record and its data into u. */
vox_unit *diphonedb_unit_decode(const diphonedb *d, const vox_unit_rec *r, vox_unit *u)
{
    const unsigned char *p = d->unit_data + r->data, *q;
    unsigned char c;

    u->f98 = d->sec13 ? d->sec13[r - d->units] : 0;
    memset(u->unk_60, 0, sizeof u->unk_60);
    u->dur = r->dur / 1000.0f;
    u->phones[0] = &d->phones[r->phones[0]];
    u->phones[1] = &d->phones[r->phones[1]];
    u->phones[2] = &d->phones[r->phones[2]];
    u->phones[3] = &d->phones[r->phones[3]];
    u->phones[4] = &d->phones[r->phones[4]];
    u->f18 = r->flags & 0xf;
    u->f19 = r->flags >> 4;
    u->f1a = p[0];
    u->f1c = p[1];
    u->f1d = p[2];
    u->f1f[0] = p[3];
    memcpy(u->f1f, p + 3, 9);
    if (d->sec12)
        memcpy(u->f28, p + 0x11, 5);
    u->f1b = u->f1a >> 4;
    u->f1a &= 0xf;
    u->f1e = u->f1d >> 4;
    u->f1d &= 0xf;
    u->ph30 = &d->phones[(signed char)p[0xc]];
    c = p[0xd];
    u->f35 = c >> 4;
    u->f34 = c & 0xf;
    u->f36 = p[0xe];
    q = p + (unsigned short)(p[0xf] | p[0x10] << 8);
    u->ph38 = &d->phones[(signed char)q[0]];
    u->content38 = (signed char)q[0] == d->content;
    u->f3c = q[1];
    u->ph40 = &d->phones[(signed char)q[2]];
    u->content40 = (signed char)q[2] == d->content;
    u->f44 = q[3];
    u->f45 = q[4];
    u->f46 = q[5];
    u->ph48 = &d->phones[(signed char)q[6]];
    u->content48 = (signed char)q[6] == d->content;
    u->f4c = q[7];
    p = q + (q[8] | q[9] << 8);
    memcpy(u->f4d, p, 6);
    u->ph54 = &d->phones[(signed char)p[6]];
    u->f58 = p[7];
    u->f59 = p[8];
    u->p90 = NULL;
    u->p8c = NULL;
    u->p94 = NULL;
    u->p88 = NULL;
    return u;
}

/* Shared opening/validation prefix in vox_file_name and vox_decoder_new. */
static inline cst_file vox_file_open(const char *path, vox_header *h)
{
    cst_file fp = cst_fopen(path, CST_OPEN_READ | CST_OPEN_BINARY);
    if (fp) {
        if (cst_fread(fp, h, sizeof *h, 1) == 1 &&
            (h->magic == VOX_MAGIC_CDXV ||
             (h->magic == VOX_MAGIC_DDXV && h->u.ddxv.version == 1))) {
            h->name[31] = 0;
            return fp;
        }
        fclose(fp);
    }
    return NULL;
}

/* 0x5e858 arm: the voice name in a vox file's header, or NULL. */
char *vox_file_name(const char *path)
{
    vox_header h;
    cst_file fp = vox_file_open(path, &h);
    if (fp) {
        char *name = cst_strdup(h.name);
        cst_fclose(fp);
        return name;
    }
    return NULL;
}

/* 0x5e8f0 arm */
void vox_db_free(vox_db *db)
{
    if (db == NULL)
        return;
    if (db->map)
        cst_munmap_file(db->map);
    if (db->path)
        cst_free(db->path);
    if (db->name)
        cst_free(db->name);
    cst_free(db);
}

/* 0x5e938 arm: reads the header and section table of a vox file image and picks its codec. */
vox_db *vox_db_open(const unsigned char *data)
{
    vox_header h;
    vox_section *secs = NULL;
    const vox_section *s1, *s2;
    unsigned int n = 0, i;
    vox_db *db;

    memcpy(&h, data, sizeof h);
    if (h.magic != VOX_MAGIC_CDXV) {
        if (h.magic == VOX_MAGIC_DDXV && h.u.ddxv.version == 1) {
            n = h.u.ddxv.n_sections;
            h.name[31] = 0;
            secs = cst_safe_alloc(n, sizeof(vox_section));
            memcpy(secs, data + sizeof h, n * sizeof(vox_section));
        }
    } else {
        h.name[31] = 0;
        n = 7;
        secs = cst_safe_alloc(n, sizeof(vox_section));
        for (i = 0; i < n; i++) {
            secs[i].type = i + 1;
            secs[i].e.offset = h.u.cdxv[i].offset;
            secs[i].e.size = h.u.cdxv[i].size;
        }
    }
    db = cst_safe_calloc(1, sizeof(vox_db));
    db->name = cst_strdup(h.name);
    db->data = data;
    db->sample_rate = h.sample_rate;
    db->key = h.key;
    if (n) {
        s1 = NULL;
        for (i = 0; i != n; i++)
            if (secs[i].type == 1) {
                s1 = &secs[i];
                break;
            }
        s2 = NULL;
        for (i = 0; i != n; i++)
            if (secs[i].type == 2) {
                s2 = &secs[i];
                break;
            }
        if (s1 && s2) {
            db->sec2_size = s2->e.size;
            db->sec1_size = s1->e.size;
            for (i = 0; i < n; i++) {
                if (secs[i].e.offset == 0)
                    continue;
                switch (secs[i].type) {
                case 0:
                case 1:
                case 2:
                    break;
                case 3:
                    db->n_names = secs[i].e.size / 20;
                    db->names = data + secs[i].e.offset;
                    break;
                case 4:
                    db->sec4 = data + secs[i].e.offset;
                    break;
                case 5:
                    db->units = (const vox_unit_rec *)(data + secs[i].e.offset);
                    break;
                case 6:
                    db->unit_data = data + secs[i].e.offset;
                    break;
                case 7:
                    db->phones = (const vox_phone *)(data + secs[i].e.offset);
                    db->n_phones = secs[i].e.size / 32;
                    break;
                case 8:
                    db->sec8 = data + secs[i].e.offset;
                    break;
                case 9:
                    db->sec9 = data + secs[i].e.offset;
                    break;
                case 10:
                    db->sec10 = data + secs[i].e.offset;
                    break;
                case 11:
                    db->sec11 = data + secs[i].e.offset;
                    break;
                case 12:
                    db->sec12 = (signed char)data[secs[i].e.offset];
                    break;
                case 13:
                    db->sec13 = (const unsigned short *)(data + secs[i].e.offset);
                    break;
                case 14:
                case 15:
                    break;
                default:
                    goto fail;
                }
            }
            db->sec1 = data + s1->e.offset;
            if (db->sec2_size)
                db->sec2 = data + s2->e.offset;
            cst_free(secs);
            switch (h.codec) {
            case 2:
                db->decode = pcm_unit_decode;
                return db;
            case 8:
                db->open = vorbis_unit_open;
                db->close = vorbis_unit_close;
                db->decode = vorbis_unit_decode;
                return db;
            case 12:
                db->open = vorbis_ogg_unit_open;
                db->close = vorbis_ogg_unit_close;
                db->decode2 = vorbis_ogg_unit_decode;
                return db;
            case 14:
                db->open = silk_unit_open;
                db->close = silk_unit_close;
                db->decode2 = silk_unit_decode;
                return db;
            default:
                vox_db_free(db);
                return NULL;
            }
        }
    }
fail:
    vox_db_free(db);
    cst_free(secs);
    return NULL;
}

/* 0x5eeb4 arm: a decoder for db, reading from db->path if it has one. */
vox_decoder *vox_decoder_new(const vox_db *db)
{
    vox_decoder *d;
    vox_header h;
    cst_file fp;

    if (db == NULL)
        return NULL;
    d = cst_safe_calloc(1, sizeof(vox_decoder));
    d->db = db;
    if (db->path) {
        fp = vox_file_open(db->path, &h);
        d->fp = fp;
        if (fp == NULL) {
            cst_free(d);
            return NULL;
        }
        if (h.sample_rate != db->sample_rate || h.key != db->key) {
            cst_fclose(fp);
            cst_free(d);
            return NULL;
        }
    } else
        d->fp = NULL;
    if (db->open && !db->open(d))
        goto fail;
    return d;
fail:
    if (d->fp)
        cst_fclose(d->fp);
    cst_free(d);
    return NULL;
}

/* 0x5efe0 arm */
void vox_decoder_free(vox_decoder *d)
{
    if (d == NULL)
        return;
    if (d->db->close)
        d->db->close(d);
    if (d->fp)
        cst_fclose(d->fp);
    cst_free(d);
}

/* The two identical normalization bodies in voxdb_name_match are inlined. */
static inline char *voxdb_name_normalize(const char *s)
{
    if (cst_startswith(s, "vox@")) {
        size_t digits = strspn(s + 4, "0123456789");
        size_t len = strlen(s);
        char *out = cst_safe_alloc(len - digits, 1);
        memcpy(out, s, 3);
        digits += 4;
        memcpy(out + 3, s + digits, len + 1 - digits);
        return out;
    }
    return cst_strdup(s);
}

/* 0x5f01c arm: whether vox names a and b refer to the same voice (equal after dropping
 * "@<digits>", or one is the other plus "_..."). A NULL a matches anything. */
int voxdb_name_match(char *a, char *b)
{
    size_t la, lb;
    int r;

    if (a == NULL || strcmp(a, b) == 0)
        return 1;
    a = voxdb_name_normalize(a);
    b = voxdb_name_normalize(b);
    la = strlen(a);
    lb = strlen(b);
    if (strcmp(a, b) == 0)
        r = 1;
    else if (cst_startswith(a, b) && a[lb] == '_')
        r = 1;
    else if (cst_startswith(b, a))
        r = b[la] == '_';
    else
        r = 0;
    cst_free(a);
    cst_free(b);
    return r;
}

/* 0x5f1e4 arm: opens the unit database in a vox file image. */
diphonedb *diphonedb_new(const unsigned char *data)
{
    diphonedb *d = cst_safe_calloc(1, sizeof(diphonedb));
    vox_db *db = vox_db_open(data);
    vox_decoder *dec;
    const vox_phone *ph;
    int i, n;

    d->name = db->name;
    d->n_names = db->n_names;
    d->names = db->names;
    d->sec4 = db->sec4;
    d->units = db->units;
    d->unit_data = db->unit_data;
    d->phones = db->phones;
    d->n_phones = db->n_phones;
    d->sec12 = db->sec12;
    d->sec13 = db->sec13;
    d->extra = db->extra;
    d->db = db;
    dec = vox_decoder_new(db);
    d->key = db->key;
    d->dec = dec;
    if (d->names == NULL || d->sec4 == NULL || d->units == NULL || d->unit_data == NULL ||
        d->phones == NULL) {
        vox_decoder_free(dec);
        vox_db_free(db);
        cst_free(d);
        return NULL;
    }
    n = d->n_phones;
    ph = d->phones;
    for (i = 0; i < n; i++, ph++)
        if (strcmp(ph->name, "content") == 0)
            break;
    if (i >= n)
        i = -1;
    d->content = i;
    return d;
}

/* 0x5f334 arm */
void delete_diphonedb(diphonedb *d)
{
    if (d == NULL)
        return;
    if (d->dec)
        vox_decoder_free(d->dec);
    if (d->db)
        vox_db_free(d->db);
    cst_free(d);
}

/* 0x5f36c arm */
void val_delete_diphonedb(void *v)
{
    delete_diphonedb(v);
}

static inline const char *voxdb_checksum_find(int key, const char *checksum)
{
    const char *next;
    for (; checksum; checksum = next ? next + 1 : NULL) {
        long value = atol(checksum);
        next = strchr(checksum, ';');
        if (key == value)
            break;
    }
    return checksum;
}

/* 0x5f370 arm: loads the voice's unit database from dir (a directory holding
 * <voxdbname>/<checksum>/<voxdbname><format>, or the vox file itself; NULL: the voice name)
 * and sets the voice features that come from it. */
diphonedb *voice_load_diphonedb(cst_features *f, const char *dir)
{
    const char *name = get_param_string(f, "name", NULL);
    const char *checksum = get_param_string(f, "checksum", NULL);
    const char *vname, *fmt, *semi, *dir_checksum;
    char *path;
    cst_filemap *map;
    diphonedb *d;
    vox_db *db;
    int key, k;

    path = NULL;
    if (dir == NULL)
        dir = name;
    else if (ivona_is_dir(dir)) {
        vname = get_param_string(f, "voxdbname", get_param_string(f, "name", NULL));
        dir_checksum = get_param_string(f, "checksum", NULL);
        fmt = get_param_string(f, "voxdb_format", "22v");
        if (!dir_checksum || !vname) {
            path = NULL;
        } else {
            semi = strchr(dir_checksum, ';');
            k = semi ? semi - dir_checksum : (int)strlen(dir_checksum);
            path = cst_safe_alloc(strlen(dir) + 2 * strlen(vname) + strlen(fmt) + 4 + k, 1);
            sprintf(path, "%s/%s/%.*s/%s%s", dir, vname, k, dir_checksum, vname, fmt);
        }
        if (path)
            dir = path;
    }
    map = cst_mmap_file(dir);
    if (map == NULL || (d = diphonedb_new(map->mem)) == NULL) {
        cst_free(path);
        return NULL;
    }
    db = d->db;
    db->path = cst_strdup(dir);
    db->map = map;
    if (!voxdb_name_match((char *)name, db->name)) {
        delete_diphonedb(d);
        cst_free(path);
        return NULL;
    }
    key = db->key;
    if (checksum == NULL || voxdb_checksum_find(key, checksum)) {
    if (!feat_present(f, "zdur_cart")) {
        if (db->sec10 == NULL || db->sec8 == NULL || db->sec9 == NULL)
            goto fail;
        feat_set(f, "zdur_cart", zdur_cart_val(&db->sec9));
    }
    if (!feat_present(f, "dynamic_voxdbs")) {
        feat_set(f, "dynamic_voxdbs", dynamic_voxdbs_val(dynamic_voxdbs_new(d)));
        feat_set_int(f, "sample_rate", db->sample_rate);
        feat_set(f, "diphonedb", diphonedb_val(d));
        feat_set_int(f, "pitchmarks_on_fly", db->sec11 ? *(const int *)db->sec11 != 0 : 0);
    }
    cst_free(path);
    return d;
    }
fail:
    delete_diphonedb(d);
    cst_free(path);
    return NULL;
}

/* 0x5f6dc arm */
int diphonedb_unit_flag(const vox_decoder *dec, int x)
{
    const unsigned int *u = (const unsigned int *)dec->db->sec11;

    if (u == NULL || *u == 0)
        return 0;
    return x == 3;
}

/* 0x5f708 arm: reads and decodes unit i; m (optional) collects read and decode times. */
track *vox_decoder_read_unit(vox_decoder *dec, unsigned int i, cst_features *m)
{
    const vox_db *db = dec->db;
    const unsigned int *ext;
    const unsigned char *buf, *data, *p;
    unsigned char *fbuf, *dbuf = NULL;
    unsigned int off, size, hdr, data_off, data_size;
    int n, n_samples, n_packets, t0, t1, rt, r, k;
    short *out;
    track *w;

    if (i >= db->sec1_size / 8)
        return NULL;
    ext = (const unsigned int *)db->sec1;
    off = ext[2 * i];
    size = ext[2 * i + 1];
    if (off == 0 && size == 0)
        return NULL;
    if (dec->fp) {
        fbuf = cst_safe_calloc(size, 1);
        t0 = m ? ivona_time_usec() : 0;
        cst_fseek(dec->fp, off, CST_SEEK_ABSOLUTE);
        if (cst_fread(dec->fp, fbuf, 1, size) != (long)size) {
            cst_free(fbuf);
            return NULL;
        }
        buf = fbuf;
    } else {
        buf = db->data + off;
        t0 = 0;
    }
    if (m)
        metrics_add_time(m, "diphonedb_read_time", ivona_time_usec() - t0);
    if (db->decode2 == NULL) {
        if (m) {
            t1 = ivona_time_usec();
            w = ((vox_decode_fn)db->decode)(dec, buf, size);
            metrics_add_time(m, "diphonedb_decode_time", ivona_time_usec() - t1);
        } else
            w = ((vox_decode_fn)db->decode)(dec, buf, size);
    } else {
        t1 = m ? ivona_time_usec() : 0;
        w = NULL;
        if (size >= 2 && (n = buf[0] | buf[1] << 8, hdr = (n + 6) * 2, size >= hdr)) {
            p = buf + 2;
            n_samples = p[hdr - 14] | p[hdr - 13] << 8;
            n_packets = p[hdr - 12] | p[hdr - 11] << 8;
            data_off = p[hdr - 10] | p[hdr - 9] << 8 | p[hdr - 8] << 16 | p[hdr - 7] << 24;
            data_size = p[hdr - 6] | p[hdr - 5] << 8 | p[hdr - 4] << 16 | p[hdr - 3] << 24;
            out = cst_safe_alloc(n_samples + db->pad_samples, sizeof(short));
            rt = m ? ivona_time_usec() : 0;
            if (dec->fp) {
                dbuf = cst_safe_alloc(data_size, 1);
                cst_fseek(dec->fp, data_off, CST_SEEK_ABSOLUTE);
                if (cst_fread(dec->fp, dbuf, 1, data_size) != (long)data_size) {
                    cst_errmsg("**Error: voxdb read\n");
                    goto cleanup;
                }
                data = dbuf;
            } else
                data = db->data + data_off;
            if (m) {
                rt = ivona_time_usec() - rt;
                metrics_add_time(m, "diphonedb_read_time", rt);
            }
            r = ((vox_decode2_fn)db->decode2)(dec, data, data_size, n_packets, out, n_samples);
            if (r == 0) {
                w = new_track();
                track_resize(w, n);
                for (k = 0; k < n; k++) {
                    w->pms[k] = p[0] | p[1] << 8;
                    p += 2;
                    track_frame_alloc(w, k, track_frame_len(w, k));
                    memcpy(w->frames[k], out + track_frame_start(w, k),
                           track_frame_len(w, k) * sizeof(short));
                }
            }
cleanup:
            if (dec->fp)
                cst_free(dbuf);
            cst_free(out);
            if (m)
                metrics_add_time(m, "diphonedb_decode_time", ivona_time_usec() - t1 - rt);
        }
    }
    if (w) {
        w->sample_rate = db->sample_rate;
        if (diphonedb_unit_flag(dec, w->num_frames))
            w->flag = 1;
    }
    if (dec->fp)
        cst_free((void *)buf);
    return w;
}

/* 0x5fb6c arm */
vox_db_extra *utt_diphonedb_extra(const cst_utterance *utt)
{
    const cst_val *v = feat_val(utt->features, "diphonedb");
    diphonedb *d = v ? val_diphonedb(v) : NULL;

    return &d->extra;
}

/* 0x5fb9c arm */
static int select_cand_cmp_cost(const void *a, const void *b)
{
    unsigned int x = ((const select_cand *)a)->cost, y = ((const select_cand *)b)->cost;

    if (x < y)
        return -1;
    return x > y;
}

/* 0x5fbc0 arm */
static int select_cand_cmp_8(const void *a, const void *b)
{
    unsigned int x = ((const select_cand *)a)->total, y = ((const select_cand *)b)->total;

    if (x < y)
        return -1;
    return x > y;
}

/* 0x5fbe4 arm: acoustic join cost between units a|b and c|d. */
unsigned int vox_join_cost(const vox_unit *a, const vox_unit *b, const vox_unit *c,
                           const vox_unit *d)
{
    if (*(const int *)(c->unk_60 + 4) != 0 || *(const int *)(d->unk_60 + 4) != 0)
        return acoustic_stub_1(a->unk_60, c->unk_60, 1) + acoustic_stub_1(b->unk_60, d->unk_60, 0);
    return 0;
}

/* 0x5fc44 arm */
unit_selector *unit_selector_new(cst_utterance *utt)
{
    unit_selector *s = cst_safe_calloc(1, sizeof(unit_selector));

    if (utt) {
        s->utt = utt;
        s->dynamic = val_dynamic_voxdbs(feat_val(utt->features, "dynamic_voxdbs"));
    }
    return s;
}

/* 0x5fc8c arm */
void unit_selector_free(unit_selector *s)
{
    int i;

    if (s == NULL)
        return;
    for (i = 0; i < s->n_segs; i++) {
        select_segment *seg = &s->segs[i];

        if (seg->cands)
            cst_free(seg->cands);
    }
    cst_free(s->segs);
    cst_free(s);
}

/* 0x5fcf4 arm: appends a cleared segment. */
select_segment *unit_selector_add_segment(unit_selector *s)
{
    if (s == NULL)
        return NULL;
    s->n_segs++;
    s->segs = cst_safe_realloc(s->segs, s->n_segs, sizeof(select_segment));
    memset(&s->segs[s->n_segs - 1], 0, sizeof(select_segment));
    return &s->segs[s->n_segs - 1];
}

/* 0x5feb8 arm */
void select_segment_resize(select_segment *seg, int n)
{
    seg->cands = cst_safe_realloc(seg->cands, n, sizeof(select_cand));
    seg->n_cands = n;
}

/* 0x5fedc arm: sorts the candidates by cost. */
void select_segment_sort(select_segment *seg)
{
    qsort(seg->cands, seg->n_cands, sizeof(select_cand), select_cand_cmp_cost);
}

#define VITERBI_INF 1000000000
#define PRESELECT_MAX 50

/* inserts v into the ascending k best path costs */
#define BEST_INSERT(best, k, v)                                                      \
    do {                                                                             \
        int lo_ = 0, hi_ = (int)(k) - 1, mid_ = 0;                                   \
        unsigned int *p_;                                                            \
        while (lo_ <= hi_) {                                                         \
            mid_ = lo_ + (hi_ - lo_) / 2;                                            \
            if ((v) < (best)[mid_])                                                  \
                hi_ = mid_ - 1;                                                      \
            else if ((v) == (best)[mid_])                                            \
                break;                                                               \
            else                                                                     \
                lo_ = mid_ + 1;                                                      \
        }                                                                            \
        if (lo_ > hi_ && (best)[mid_] < (v))                                         \
            mid_++;                                                                  \
        p_ = &(best)[mid_];                                                          \
        if ((unsigned int)(mid_ + 1) < (unsigned int)(k))                            \
            memmove(p_ + 1, p_, ((k) - 1 - mid_) * sizeof(unsigned int));           \
        *p_ = (v);                                                                   \
    } while (0)

/* the unit's first (half 0) or second (half 1) context byte */
#define UNIT_CTX(db, u) ((db)->unit_data[(db)->units[u].data + 2] & 0xf)

/* 0x5fefc arm: advances the unit selection: computes (and preselects) the candidates of the
 * segments up to the next boundary, then runs the beam-limited Viterbi search over them
 * until the best path is fixed at some segment; returns the (checksum, unit) pairs of the
 * newly fixed path (*n of them), NULL when there is nothing more to select. */
int *unit_select_viterbi(unit_selector *s, unsigned int beam, const cst_val *bl, cst_features *m,
                         unsigned int *n_out, int threads)
{
    int nsegs = s->n_segs, i, end, j, k, n, calls, fb, fixed, q, p;
    int t0, t, fallback = 0, model_calls = 0, pre_time = 0, sort_time = 0;
    unsigned int u, cnt, lo, hi, total, jc, keep, na, nb;
    select_segment *seg, *prev, *cur;
    select_cand *c, *pc, *nc, *b;
    const diphonedb *db;
    const vox_unit_range *r;
    select_model_cost_fn cost;
    int *path;
    union {
        struct {
            vox_unit a;
            vox_unit b;
        } u;
        struct {
            unsigned char pad[0x9c];
            unsigned int best[2002];
        } s;
    } w;
    unsigned int *best = w.s.best;
    int msum, jsum;

    t0 = m ? ivona_time_usec() : 0;
    if (s->n_segs == 0)
        return NULL;
    if (s->done)
        return NULL;
    i = s->n_ready;
    if (s->pos + 1 >= i) {
        end = nsegs;
        if (i < nsegs - 2) {
            for (k = i; k != nsegs - 2; k++) {
                if (s->segs[k].boundary) {
                    end = k + 2;
                    break;
                }
            }
        }
        if (i < end) {
            for (; i != end; i++) {
                seg = &s->segs[i];
                r = seg->range;
                select_segment_resize(seg, r->count);
                calls = 0;
                fb = 0;
                n = 0;
                if (r->count == 0)
                    seg->n_cands = 0;
                else {
                    for (j = 0; j < r->count; j++) {
                        u = ((const unsigned int *)seg->db->sec4)[r->first + j];
                        if (blacklists_contain(bl, u))
                            continue;
                        db = seg->db;
                        if (strcmp((const char *)seg->target[0].p88, "+") == 0 && seg->target[0].f1d != UNIT_CTX(db, u))
                            continue;
                        if (strcmp((const char *)seg->target[1].p88, "+") == 0 && seg->target[1].f1d != UNIT_CTX(db, u + 1))
                            continue;
                        diphonedb_unit_decode(db, &db->units[u], &w.u.a);
                        diphonedb_unit_decode(seg->db, &seg->db->units[u + 1], &w.u.b);
                        cost = seg->model_cost;
                        seg->cands[n].unit = u;
                        if (cost == NULL)
                            seg->model_cost = cost = s->model_cost;
                        seg->cands[n].cost = cost(&w.u.a, &w.u.b, &seg->target[0], &seg->target[1]);
                        n++;
                        calls++;
                    }
                    seg->n_cands = n;
                    if (n <= PRESELECT_MAX) {
                        /* too few: add the units of the wrong context too */
                        for (j = 0; j < r->count; j++) {
                            u = ((const unsigned int *)seg->db->sec4)[r->first + j];
                            if (blacklists_contain(bl, u))
                                continue;
                            db = seg->db;
                            if (strcmp((const char *)seg->target[0].p88, "+") != 0
                                || seg->target[0].f1d == UNIT_CTX(db, u)) {
                                if (strcmp((const char *)seg->target[1].p88, "+") != 0
                                    || seg->target[1].f1d == UNIT_CTX(db, u + 1))
                                    continue;
                            }
                            diphonedb_unit_decode(db, &db->units[u], &w.u.a);
                            diphonedb_unit_decode(seg->db, &seg->db->units[u + 1], &w.u.b);
                            seg->cands[n].unit = u;
                            seg->cands[n].cost = s->model_cost(&w.u.a, &w.u.b, &seg->target[0],
                                                               &seg->target[1]);
                            fb++;
                            n++;
                        }
                        seg->n_cands = n;
                    }
                }
                t = ivona_time_usec();
                if (s->preselect)
                    s->preselect(s, i);
                else {
                    seg = &s->segs[i];
                    n = seg->n_cands;
                    if (n > PRESELECT_MAX + 1) {
                        /* keep about PRESELECT_MAX of the cheapest, and those whose
                         * neighbouring phones match */
                        hi = 0;
                        lo = 0xffffffff;
                        for (k = 0; k < n; k++) {
                            if (hi < seg->cands[k].cost)
                                hi = seg->cands[k].cost;
                            if (lo >= seg->cands[k].cost)
                                lo = seg->cands[k].cost;
                        }
                        if (lo < hi) {
                            do {
                                cnt = 0;
                                for (k = 0; k < n; k++)
                                    if (seg->cands[k].cost <= lo)
                                        cnt++;
                                lo++;
                            } while (cnt <= PRESELECT_MAX && lo < hi);
                        }
                        na = strcmp("pau", seg->target[0].phones[2]->name);
                        nb = strcmp("pau", seg->target[1].phones[2]->name);
                        keep = 0;
                        for (k = 0; k < n; k++) {
                            c = &seg->cands[k];
                            u = c->unit;
                            if (c->cost <= lo
                                || (na && strcmp(seg->db->phones[seg->db->units[u].phones[1]].name,
                                                 seg->target[0].phones[1]->name) == 0)
                                || (nb && strcmp(seg->db->phones[seg->db->units[u + 1].phones[3]].name,
                                                 seg->target[1].phones[3]->name) == 0)) {
                                if (k != keep)
                                    memcpy(&seg->cands[keep], c, sizeof(select_cand));
                                n = seg->n_cands;
                                keep++;
                            }
                        }
                        seg->n_cands = keep;
                    }
                }
                pre_time += ivona_time_usec() - t;
                model_calls += calls;
                fallback += fb;
                if (m) {
                    t = ivona_time_usec();
                    select_segment_sort(&s->segs[i]);
                    sort_time += ivona_time_usec() - t;
                } else
                    select_segment_sort(&s->segs[i]);
                if (i == 0) {
                    seg = &s->segs[0];
                    for (k = 0; k < seg->n_cands; k++)
                        seg->cands[k].total = seg->cands[k].cost;
                }
            }
            s->n_ready = i;
            if (s->pos + 1 < s->n_segs) {
                prev = &s->segs[s->pos];
                cur = prev + 1;
                if (prev->n_cands && cur->n_cands) {
                    j = k = 0;
                    do {
                        pc = &prev->cands[j];
                        c = &cur->cands[k];
                        if (pc->unit + 1 == c->unit) {
                            pc->next = c;
                            j++;
                            k++;
                        } else if (pc->unit < c->unit)
                            j++;
                        else
                            k++;
                    } while (j < prev->n_cands && k < cur->n_cands);
                }
            }
        }
    }
    if (m) {
        metrics_add_time(m, "diphone_graph_get_diphones_time", ivona_time_usec() - t0);
        metrics_add_time(m, "diphone_graph_sort_mc_time", sort_time);
        metrics_add_time(m, "diphone_graph_preselect_time", pre_time);
        metrics_add_int(m, "diphone_graph_model_cost_calls", model_calls);
        metrics_add_int(m, "diphone_graph_model_cost_fallback_calls", fallback);
    }

    fixed = -1;
    for (;;) {
        if (s->pos >= s->n_segs - 1) {
            if (fixed == -1) {
                /* the end: the cheapest last candidate */
                s->prev_best = s->best;
                cur = &s->segs[s->n_segs - 1];
                k = 0;
                if (cur->n_cands) {
                    lo = 0xffffffff;
                    for (j = 0; j < cur->n_cands; j++) {
                        if (cur->cands[j].total < lo)
                            k = j;
                        if (lo >= cur->cands[j].total)
                            lo = cur->cands[j].total;
                    }
                }
                s->best = &cur->cands[k];
                s->fixed = s->n_segs - 1;
                s->done = 1;
            }
            break;
        }
        prev = &s->segs[s->pos];
        cur = prev + 1;
        s->pos++;
        for (j = 0; j < (int)beam; j++)
            best[j] = VITERBI_INF;
        for (k = 0; k < cur->n_cands; k++)
            cur->cands[k].total = VITERBI_INF;
        if (m) {
            t = ivona_time_usec();
            qsort(prev->cands, prev->n_cands, sizeof(select_cand), select_cand_cmp_8);
            metrics_add_time(m, "diphone_graph_sort_dist_time", ivona_time_usec() - t);
        } else
            qsort(prev->cands, prev->n_cands, sizeof(select_cand), select_cand_cmp_8);
        /* continuing a unit costs no join */
        for (k = 0; k < prev->n_cands; k++) {
            pc = &prev->cands[k];
            nc = pc->next;
            if (nc == NULL)
                continue;
            nc->back = pc;
            nc->total = pc->total + nc->cost;
            if (beam != 0xffffffff && nc->total < best[beam - 1])
                BEST_INSERT(best, beam, nc->total);
        }
        if (s->pos + 1 < s->n_segs && !prev->boundary) {
            seg = &s->segs[s->pos + 1];
            if (cur->n_cands && seg->n_cands) {
                j = k = 0;
                do {
                    c = &cur->cands[j];
                    nc = &seg->cands[k];
                    if (c->unit + 1 == nc->unit) {
                        c->next = nc;
                        j++;
                        k++;
                    } else if (c->unit < nc->unit)
                        j++;
                    else
                        k++;
                } while (j < cur->n_cands && k < seg->n_cands);
            }
        }
        for (k = 0; k < cur->n_cands; k++) {
            c = &cur->cands[k];
            u = c->unit;
            for (j = 0; j < prev->n_cands; j++) {
                pc = &prev->cands[j];
                total = pc->total + c->cost;
                if (j >= (int)beam && (pc->total != pc[-1].total || j > 2 * (int)beam))
                    break;
                if (total >= c->total)
                    break;
                if (c->next == NULL && beam != 0xffffffff && total >= best[beam - 1])
                    break;
                if (pc->unit + 1 == u)
                    continue;
                jc = prev->join_cost(&prev->db->units[pc->unit + 1], &cur->db->units[u], c->total - total);
                if (jc + total < c->total) {
                    c->total = jc + total;
                    c->back = pc;
                }
            }
            if (c->back)
                c->back->refs++;
            if (beam != 0xffffffff && c->total < best[beam - 1])
                BEST_INSERT(best, beam, c->total);
        }
        /* drop the paths through unreferenced candidates; a segment left with one live
         * candidate fixes the path up to it */
        fixed = -1;
        for (k = 0; k < prev->n_cands; k++) {
            pc = &prev->cands[k];
            if (pc->refs) {
                prev->alive++;
                continue;
            }
            p = s->pos - 1;
            if (p <= 0 || (b = pc->back) == NULL || --b->refs != 0)
                continue;
            if (--s->segs[p - 1].alive == 1) {
                if (fixed < p - 1)
                    fixed = p - 1;
                continue;
            }
            for (q = p - 1; q != 0; q--) {
                b = b->back;
                if (b == NULL || --b->refs != 0)
                    break;
                if (--s->segs[q - 1].alive == 1) {
                    if (fixed < q - 1)
                        fixed = q - 1;
                    break;
                }
            }
        }
        if (prev->alive == 1)
            fixed = s->pos - 1;
        if (prev->boundary && s->pos + 1 < s->n_segs) {
            /* a boundary: fix the path at the cheapest candidate */
            s->prev_best = s->best;
            if (cur->n_cands == 0) {
                s->best = cur->cands->back;
                s->fixed = s->pos - 1;
                n = 0;
            } else {
                k = 0;
                lo = 0xffffffff;
                for (j = 0; j < cur->n_cands; j++) {
                    if (cur->cands[j].total < lo)
                        k = j;
                    if (lo >= cur->cands[j].total)
                        lo = cur->cands[j].total;
                }
                b = cur->cands[k].back;
                total = b->total;
                s->best = b;
                s->fixed = s->pos - 1;
                for (n = 0, j = 0; j < cur->n_cands; j++) {
                    if (cur->cands[j].back != s->best)
                        continue;
                    memmove(&cur->cands[n], &cur->cands[j], sizeof(select_cand));
                    cur->cands[n].back = NULL;
                    cur->cands[n].total -= total;
                    n++;
                }
            }
            cur->n_cands = n;
            break;
        }
        if (fixed >= 0) {
            for (c = s->segs[fixed].cands; c->refs == 0; c++)
                ;
            s->fixed = fixed;
            s->prev_best = s->best;
            s->best = c;
            break;
        }
    }

    if (m) {
        n = 0;
        msum = jsum = 0;
        for (c = s->best; c && c != s->prev_best; c = c->back) {
            jc = c->total - c->cost - (c->back ? c->back->total : 0);
            msum = metrics_add_int(m, "diphone_graph_dijkstra_model_cost", c->cost);
            metrics_max_int(m, "diphone_graph_dijkstra_max_model_cost", c->cost);
            jsum = metrics_add_int(m, "diphone_graph_dijkstra_conc_cost", jc);
            metrics_max_int(m, "diphone_graph_dijkstra_max_conc_cost", jc);
            metrics_max_int(m, "diphone_graph_dijkstra_max_total_cost", jc + c->cost);
            n++;
        }
        if ((k = metrics_add_int(m, "diphone_graph_layers", n)) != 0) {
            metrics_set_float(m, "diphone_graph_dijkstra_model_cost_per_layer", (float)msum / (unsigned int)k);
            metrics_set_float(m, "diphone_graph_dijkstra_conc_cost_per_layer", (float)jsum / (unsigned int)k);
        }
        metrics_add_time(m, "diphone_graph_dijkstra_time", ivona_time_usec() - t0);
    }
    c = s->best;
    if (c == NULL || c == s->prev_best) {
        n = 0;
        path = cst_safe_alloc(0, 2 * sizeof(int));
    } else {
        for (n = 0, b = c; b != s->prev_best && b; b = b->back)
            n++;
        path = cst_safe_alloc(n, 2 * sizeof(int));
        for (k = n; c != s->prev_best && c; c = c->back) {
            k--;
            path[2 * k] = s->segs[k].db->key;
            path[2 * k + 1] = c->unit;
        }
    }
    if (n_out)
        *n_out = n;
    return path;
}

/* 0x61408 arm */
dynamic_voxdbs *dynamic_voxdbs_new(diphonedb *a)
{
    dynamic_voxdbs *dv = cst_safe_calloc(1, sizeof(dynamic_voxdbs));

    dv->a = a;
    return dv;
}

/* 0x61424 arm: the database with the given key, or NULL. */
diphonedb *dynamic_voxdbs_find(const dynamic_voxdbs *dv, int key)
{
    if (dv == NULL)
        return NULL;
    if (dv->a && dv->a->key == key)
        return dv->a;
    if (dv->b == NULL)
        return NULL;
    return dv->b->key == key ? dv->b : NULL;
}

/* 0x61468 arm */
int phone_is_breaths(const char *name)
{
    return strcmp(name, "breaths") == 0;
}

/* 0x61488 arm: whether either database has a unit named name. */
int dynamic_voxdbs_has_unit(const dynamic_voxdbs *dv, const char *name)
{
    if (dv->a && diphonedb_unit_index(dv->a, name) != -1)
        return 1;
    if (dv->b == NULL)
        return 0;
    return diphonedb_unit_index(dv->b, name) != -1;
}

/* 0x614d8 arm */
dynamic_voxdbs *val_dynamic_voxdbs(const cst_val *v)
{
    if (v && CST_VAL_TYPE(v) == cst_val_type_dynamic_voxdbs)
        return (dynamic_voxdbs *)CST_VAL_VOID(v);
    cst_errmsg("VAL: tried to access %s in %d type val\n", "dynamic_voxdbs",
               (v ? CST_VAL_TYPE(v) : -1));
    cst_error();
}

/* 0x61530 arm: whether the voice's databases have breath units. */
int feats_have_breath_units(const cst_features *f)
{
    const dynamic_voxdbs *dv;

    if (f == NULL || !feat_present(f, "dynamic_voxdbs"))
        return 0;
    dv = val_dynamic_voxdbs(feat_val(f, "dynamic_voxdbs"));
    if (dv == NULL)
        return 0;
    if (dv->a && diphonedb_unit_index(dv->a, "pau-brth") != -1)
        return 1;
    if (dv->b == NULL)
        return 0;
    return diphonedb_unit_index(dv->b, "pau-brth") != -1;
}

/* 0x615cc arm */
void val_delete_dynamic_voxdbs(void *v)
{
    cst_free(v);
}

/* 0x615d0 arm */
cst_val *dynamic_voxdbs_val(const dynamic_voxdbs *v)
{
    return val_new_typed(cst_val_type_dynamic_voxdbs, (void *)v);
}

/* a join cost function (vox_join_cost by default) */
typedef struct join_cost_func join_cost_func;
extern const int cst_val_type_join_cost_func;
/* 0x615e8 arm, 0x61640 arm */
CST_VAL_REGISTER_TYPE_NODEL(join_cost_func, join_cost_func)
