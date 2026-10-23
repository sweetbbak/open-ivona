/*
 * Waveform synthesis state (0x62174..0x638e0): the output wave of an utterance, the
 * unit selector and the marks collected while concatenating. Compiled without inlining.
 */
#include <stdio.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_val.h"
#include "cst_val_defs.h"
#include "cst_file.h"
#include "cst_wave.h"
#include "tts_voxdb.h"

typedef struct unit_selector unit_selector;
void unit_selector_free(unit_selector *s);
void *val_userdata(const cst_val *v);

/* a mark (0x14 bytes) */
typedef struct wavesynth_mark {
    int unk[5];
} wavesynth_mark;

typedef struct wavesynth {
    int unk_0;
    void *seg;                  /* +0x04 */
    cst_wave *wave;             /* +0x08 */
    int pos;                    /* +0x0c: samples written */
    int pending;                /* +0x10 */
    int total;                  /* +0x14 */
    cst_val *marks;             /* +0x18: userdata wavesynth_mark */
    unit_selector *selector;    /* +0x1c */
    int unk_20[6];
} wavesynth;

CST_VAL_USER_TYPE_DCLS(wavesynth, wavesynth)

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
