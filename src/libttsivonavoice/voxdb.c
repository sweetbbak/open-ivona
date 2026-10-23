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

#define VOX_MAGIC_CDXV 0x56584443
#define VOX_MAGIC_DDXV 0x56584444

typedef struct vox_extent {
    unsigned int offset;
    unsigned int size;
} vox_extent;

/* file header (0x68 bytes) */
typedef struct vox_header {
    unsigned int magic;
    char name[32];
    int sample_rate;            /* +0x24 */
    int codec;                  /* +0x28 */
    int key;                    /* +0x2c */
    union {
        struct {
            int version;        /* +0x30: DDXV: 1 */
            unsigned int n_sections;
        } ddxv;
        vox_extent cdxv[7];     /* CDXV: sections 1..7 */
    } u;
} vox_header;

typedef struct vox_section {
    unsigned int type;
    vox_extent e;
} vox_section;

/* a phone (32 bytes, section 7) */
typedef struct vox_phone {
    char name[32];
} vox_phone;

/* a unit record (36 bytes, section 5) */
typedef struct vox_unit_rec {
    unsigned short dur;         /* ms */
    unsigned char phones[5];
    unsigned char flags;
    unsigned char unk_8[24];
    unsigned int data;          /* +0x20: offset of the unit's data in section 6 */
} vox_unit_rec;

typedef struct vox_decoder vox_decoder;

/* copied from the vox db into the diphonedb (unknown) */
typedef struct vox_db_extra {
    int unk[5];
} vox_db_extra;

/* an opened vox file (0x8c bytes) */
typedef struct vox_db {
    char *path;                 /* +0x00: file the decoder reads, if any */
    char *name;                 /* +0x04 */
    cst_filemap *map;           /* +0x08 */
    int sample_rate;            /* +0x0c */
    int key;                    /* +0x10 */
    const unsigned char *sec11; /* +0x14 */
    const unsigned char *sec1;  /* +0x18 */
    unsigned int sec1_size;     /* +0x1c */
    const unsigned char *unit_data;     /* +0x20: section 6 */
    const vox_unit_rec *units;  /* +0x24: section 5 */
    const unsigned char *sec4;  /* +0x28 */
    const unsigned char *names; /* +0x2c: section 3, 20-byte entries */
    unsigned int n_names;       /* +0x30 */
    const vox_phone *phones;    /* +0x34: section 7 */
    unsigned int n_phones;      /* +0x38 */
    const unsigned char *sec9;  /* +0x3c */
    const unsigned char *sec8;  /* +0x40 */
    const unsigned char *sec10; /* +0x44 */
    int (*open)(vox_decoder *d);        /* +0x48 */
    void (*close)(vox_decoder *d);      /* +0x4c */
    void *decode;               /* +0x50 */
    void *decode2;              /* +0x54 */
    int unk_58;
    const unsigned char *sec2;  /* +0x5c */
    unsigned int sec2_size;     /* +0x60 */
    int sec12;                  /* +0x64 */
    const unsigned short *sec13;        /* +0x68 */
    vox_db_extra extra;         /* +0x6c */
    int unk_80[2];
    const unsigned char *data;  /* +0x88 */
} vox_db;

struct vox_decoder {
    const vox_db *db;
    cst_file fp;
    void *priv;
};

/* a voice's unit database (0x4c bytes) */
typedef struct diphonedb {
    char *name;
    int key;                    /* +0x04 */
    unsigned int n_names;       /* +0x08 */
    const unsigned char *names; /* +0x0c */
    const unsigned char *sec4;  /* +0x10 */
    const vox_unit_rec *units;  /* +0x14 */
    const unsigned char *unit_data;     /* +0x18 */
    const vox_phone *phones;    /* +0x1c */
    vox_db *db;                 /* +0x20 */
    unsigned int n_phones;      /* +0x24 */
    int content;                /* +0x28: index of the "content" phone, or -1 */
    int sec12;                  /* +0x2c */
    const unsigned short *sec13;        /* +0x30 */
    vox_db_extra extra;         /* +0x34 */
    vox_decoder *dec;           /* +0x48 */
} diphonedb;

/* a decoded unit (0x9c bytes) */
typedef struct vox_unit {
    float dur;                          /* +0x00: seconds */
    const vox_phone *phones[5];         /* +0x04 */
    unsigned char f18, f19, f1a, f1b, f1c, f1d, f1e;
    unsigned char f1f[9];               /* +0x1f */
    unsigned char f28[5];               /* +0x28 */
    const vox_phone *ph30;              /* +0x30 */
    unsigned char f34, f35, f36;
    const vox_phone *ph38;              /* +0x38 */
    unsigned char f3c;
    const vox_phone *ph40;              /* +0x40 */
    unsigned char f44, f45, f46;
    const vox_phone *ph48;              /* +0x48 */
    unsigned char f4c;
    unsigned char f4d[6];               /* +0x4d */
    const vox_phone *ph54;              /* +0x54 */
    unsigned char f58, f59;
    unsigned char content38, content40, content48;      /* +0x5a */
    unsigned char unk_5d[3];
    unsigned char unk_60[0x28];         /* +0x60 */
    void *p88, *p8c, *p90, *p94;        /* +0x88 */
    unsigned int f98;                   /* +0x98 */
} vox_unit;

/* a unit candidate for one segment (24 bytes) */
typedef struct select_cand {
    int unk_0[2];
    unsigned int unk_8;
    unsigned int cost;          /* +0x0c */
    int unk_10[2];
} select_cand;

/* one segment of a unit selection (0x158 bytes) */
typedef struct select_segment {
    unsigned char unk_0[0x144];
    select_cand *cands;         /* +0x144 */
    int n_cands;                /* +0x148 */
    unsigned char unk_14c[0xc];
} select_segment;

/* the extra unit databases of a voice */
typedef struct dynamic_voxdbs {
    diphonedb *a;
    diphonedb *b;
} dynamic_voxdbs;

typedef struct unit_selector {
    cst_utterance *utt;
    dynamic_voxdbs *dynamic;    /* +0x04 */
    int unk_8[4];
    select_segment *segs;       /* +0x18 */
    int n_segs;                 /* +0x1c */
    int unk_20[4];
} unit_selector;

float acoustic_stub_1(const void *a, const void *b, int x);

int ivona_is_dir(const char *path);
cst_val *FUN_00067fec(const unsigned char **tables);
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

/* 0x5e858 arm: the voice name in a vox file's header, or NULL. */
char *vox_file_name(const char *path)
{
    vox_header h;
    cst_file fp = cst_fopen(path, CST_OPEN_READ | CST_OPEN_BINARY);
    char *name = NULL;

    if (fp == NULL)
        return name;
    if (cst_fread(fp, &h, sizeof h, 1) == 1 &&
        (h.magic == VOX_MAGIC_CDXV || (h.magic == VOX_MAGIC_DDXV && h.u.ddxv.version == 1))) {
        h.name[31] = 0;
        name = cst_strdup(h.name);
        cst_fclose(fp);
        return name;
    }
    fclose(fp);
    return name;
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
    if (h.magic == VOX_MAGIC_CDXV) {
        h.name[31] = 0;
        n = 7;
        secs = cst_safe_alloc(n, sizeof(vox_section));
        secs[0].type = 1;
        secs[0].e = h.u.cdxv[0];
        secs[1].type = 2;
        secs[1].e = h.u.cdxv[1];
        secs[2].type = 3;
        secs[2].e = h.u.cdxv[2];
        secs[3].type = 4;
        secs[3].e = h.u.cdxv[3];
        secs[4].type = 5;
        secs[4].e = h.u.cdxv[4];
        secs[5].type = 6;
        secs[5].e = h.u.cdxv[5];
        secs[6].type = 7;
        secs[6].e = h.u.cdxv[6];
    } else if (h.magic != VOX_MAGIC_DDXV || h.u.ddxv.version != 1) {
        /* not a vox file */
    } else {
        n = h.u.ddxv.n_sections;
        h.name[31] = 0;
        secs = cst_safe_alloc(n, sizeof(vox_section));
        memcpy(secs, data + sizeof h, n * sizeof(vox_section));
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
                db->decode = vorbis_ogg_unit_decode;
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
        fp = cst_fopen(db->path, CST_OPEN_READ | CST_OPEN_BINARY);
        if (fp == NULL)
            goto nofile;
        if (cst_fread(fp, &h, sizeof h, 1) != 1 ||
            !(h.magic == VOX_MAGIC_CDXV || (h.magic == VOX_MAGIC_DDXV && h.u.ddxv.version == 1))) {
            fclose(fp);
nofile:
            d->fp = NULL;
            cst_free(d);
            return NULL;
        }
        h.name[31] = 0;
        d->fp = fp;
        if (h.sample_rate != db->sample_rate || h.key != db->key)
            goto fail;
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

/* a new copy of s without the digits of a "vox@<digits>..." prefix ("vox...") */
#define VOX_NAME_NORMALIZE(s)                                                         \
    do {                                                                              \
        if (!cst_startswith(s, "vox@"))                                               \
            s = cst_strdup(s);                                                        \
        else {                                                                        \
            size_t k_ = strspn(s + 4, "0123456789");                                  \
            size_t l_ = strlen(s);                                                    \
            char *t_ = cst_safe_alloc(l_ - k_, 1);                                    \
            memcpy(t_, s, 3);                                                         \
            k_ += 4;                                                                  \
            memcpy(t_ + 3, s + k_, l_ + 1 - k_);                                      \
            s = t_;                                                                   \
        }                                                                             \
    } while (0)

#define VOX_NAME_NORMALIZE2(s)                                                        \
    do {                                                                              \
        if (cst_startswith(s, "vox@")) {                                              \
            size_t k_ = strspn(s + 4, "0123456789");                                  \
            size_t l_ = strlen(s);                                                    \
            char *t_ = cst_safe_alloc(l_ - k_, 1);                                    \
            memcpy(t_, s, 3);                                                         \
            k_ += 4;                                                                  \
            memcpy(t_ + 3, s + k_, l_ + 1 - k_);                                      \
            s = t_;                                                                   \
        } else                                                                        \
            s = cst_strdup(s);                                                        \
    } while (0)

/* 0x5f01c arm: whether vox names a and b refer to the same voice (equal after dropping
 * "@<digits>", or one is the other plus "_..."). A NULL a matches anything. */
int voxdb_name_match(char *a, char *b)
{
    size_t la, lb;
    int r;

    if (a == NULL || strcmp(a, b) == 0)
        return 1;
    VOX_NAME_NORMALIZE2(a);
    VOX_NAME_NORMALIZE(b);
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

/* 0x5f370 arm: loads the voice's unit database from dir (a directory holding
 * <voxdbname>/<checksum>/<voxdbname><format>, or the vox file itself; NULL: the voice name)
 * and sets the voice features that come from it. */
diphonedb *voice_load_diphonedb(cst_features *f, const char *dir)
{
    const char *name = get_param_string(f, "name", NULL);
    const char *checksum = get_param_string(f, "checksum", NULL);
    const char *vname, *fmt, *p, *semi;
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
        checksum = get_param_string(f, "checksum", NULL);
        fmt = get_param_string(f, "voxdb_format", "22v");
        if (checksum && vname) {
            semi = strchr(checksum, ';');
            k = semi ? semi - checksum : (int)strlen(checksum);
            path = cst_safe_alloc(strlen(dir) + 2 * strlen(vname) + strlen(fmt) + 4 + k, 1);
            sprintf(path, "%s/%s/%.*s/%s%s", dir, vname, k, checksum, vname, fmt);
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
    if (checksum) {
        for (p = checksum; p;) {
            long v = atol(p);

            semi = strchr(p, ';');
            if (v == key)
                goto ok;
            if (semi == NULL)
                break;
            p = semi + 1;
        }
        goto fail;
    }
ok:
    if (!feat_present(f, "zdur_cart")) {
        if (db->sec10 == NULL || db->sec8 == NULL || db->sec9 == NULL)
            goto fail;
        feat_set(f, "zdur_cart", FUN_00067fec(&db->sec9));
    }
    if (!feat_present(f, "dynamic_voxdbs")) {
        feat_set(f, "dynamic_voxdbs", dynamic_voxdbs_val(dynamic_voxdbs_new(d)));
        feat_set_int(f, "sample_rate", db->sample_rate);
        feat_set(f, "diphonedb", diphonedb_val(d));
        feat_set_int(f, "pitchmarks_on_fly", db->sec11 ? *(const int *)db->sec11 != 0 : 0);
    }
    cst_free(path);
    return d;
fail:
    delete_diphonedb(d);
    cst_free(path);
    return NULL;
}

/* 0x5f6dc arm */
int diphonedb_unit_flag(const dynamic_voxdbs *dv, int x)
{
    const unsigned int *u = (const unsigned int *)dv->a->units;

    if (u == NULL || *u == 0)
        return 0;
    return x == 3;
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
    unsigned int x = ((const select_cand *)a)->unk_8, y = ((const select_cand *)b)->unk_8;

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
