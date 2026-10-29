#ifndef TTS_VOXDB_H
#define TTS_VOXDB_H

/* vox unit databases (voxdb.c) */

#include "cst_file.h"
#include "cst_utterance.h"

/* a phone (32 bytes, section 7) */
typedef struct vox_phone {
    char name[32];
} vox_phone;

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
    int pad_samples;            /* +0x58 */
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
    unsigned int cost;          /* target (model) cost */
    int refs;                   /* +0x04: candidates of the next segment whose best path runs here */
    unsigned int total;         /* +0x08: best path cost up to here */
    unsigned int unit;          /* +0x0c */
    struct select_cand *back;   /* +0x10: previous candidate on the best path */
    struct select_cand *next;   /* +0x14: the next segment's candidate continuing this unit */
} select_cand;

/* the units a segment may use: db->sec4[first..first + count) */
typedef struct vox_unit_range {
    int unk_0[3];
    unsigned int first;         /* +0x0c */
    unsigned int count;         /* +0x10 */
} vox_unit_range;

typedef unsigned int (*select_model_cost_fn)(const vox_unit *a, const vox_unit *b,
                                             const vox_unit *ta, const vox_unit *tb);
typedef unsigned int (*select_join_cost_fn)(const vox_unit_rec *a, const vox_unit_rec *b,
                                            unsigned int limit);

/* one segment (diphone) of a unit selection (0x158 bytes) */
typedef struct select_segment {
    const vox_unit_range *range;
    vox_unit target[2];         /* +0x04, +0xa0: the wanted halves */
    select_join_cost_fn join_cost;      /* +0x13c */
    int boundary;               /* +0x140: the search may fix its path here */
    select_cand *cands;         /* +0x144 */
    int n_cands;                /* +0x148 */
    int alive;                  /* +0x14c: candidates still on some path */
    const struct diphonedb *db; /* +0x150 */
    select_model_cost_fn model_cost;    /* +0x154 */
} select_segment;

/* the extra unit databases of a voice */
typedef struct dynamic_voxdbs {
    diphonedb *a;
    diphonedb *b;
} dynamic_voxdbs;

typedef struct unit_selector {
    cst_utterance *utt;
    dynamic_voxdbs *dynamic;    /* +0x04 */
    void (*preselect)(struct unit_selector *s, int i);  /* +0x08 */
    select_model_cost_fn model_cost;    /* +0x0c */
    select_cand *best;          /* +0x10: end of the path fixed so far */
    select_cand *prev_best;     /* +0x14: end of the path returned before */
    select_segment *segs;       /* +0x18 */
    int n_segs;                 /* +0x1c */
    int pos;                    /* +0x20: last segment searched */
    int n_ready;                /* +0x24: segments with candidates */
    int fixed;                  /* +0x28: segment of best */
    int done;                   /* +0x2c: the selection covers the utterance */
} unit_selector;



#endif
