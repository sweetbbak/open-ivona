/*
 * SSML <prosody> attribute values (0x26d38..0x273c4), as factors relative to the current ones.
 */
#include <math.h>
#include <strings.h>

int cst_parse_float_suffix(const char *s, const char *suffix, float *out);
int cst_parse_unsigned_suffix(const char *s, const char *suffix, float *out);
int cst_parse_signed_suffix(const char *s, const char *suffix, float *out);
int cst_parse_number(const char *s, float *out);
int cst_parse_signed_number(const char *s, float *out);

#define SSML_1_0 10
#define SSML_1_1 11

typedef struct prosody {
    float volume;
    float pitch;
    float rate;
} prosody;

typedef struct prosody_level {
    const char *name;
    float value;
} prosody_level;

static const prosody_level pitch_levels[] = {
    { "x-low", 0.666666687f }, { "low", 0.800000012f }, { "medium", 1.0f }, { "high", 1.25f }, { "x-high", 1.5f }, { 0, 0 },
};

static const prosody_level rate_levels[] = {
    { "x-slow", 0.666666687f }, { "slow", 0.816526532f }, { "medium", 1.0f }, { "fast", 1.22469997f }, { "x-fast", 1.5f }, { 0, 0 },
};

static const prosody_level volume_levels[] = {
    { "silent", 0.0f }, { "x-soft", 0.625f }, { "soft", 0.790576339f }, { "medium", 1.0f }, { "loud", 1.26489997f }, { "x-loud", 1.60000002f }, { 0, 0 },
};

/* 0x26d38 arm */
void prosody_reset(prosody *p)
{
    p->volume = 1.0f;
    p->pitch = 1.0f;
    p->rate = 1.0f;
}

/* 0x26d4c arm */
int prosody_parse_pitch(prosody *p, const char *s, int version, const char **warn)
{
    const prosody_level *l;
    float v;

    if (strcasecmp("default", s) == 0) {
        p->pitch = 1.0f;
        if (warn)
            *warn = NULL;
        return 1;
    }
    for (l = pitch_levels; l->name; l++)
        if (strcasecmp(l->name, s) == 0) {
            p->pitch = l->value;
            if (warn)
                *warn = NULL;
            return 1;
        }
    if (cst_parse_unsigned_suffix(s, "%", &v) && v >= -100.0f) {
        p->pitch *= v * 0.01f + 1.0f;
        if (version != SSML_1_0 && warn) {
            *warn = "Unsigned percentage relative change of prosody pitch is deprecated in SSML 1.1";
            return 1;
        }
        if (warn)
            *warn = NULL;
        return 1;
    }
    if (cst_parse_signed_suffix(s, "%", &v) && v >= -100.0f) {
        p->pitch *= v * 0.01f + 1.0f;
        if (warn)
            *warn = NULL;
        return 1;
    }
    if (cst_parse_unsigned_suffix(s, "Hz", &v)) {
        if (warn)
            *warn = "Absolute pitch value in Hertz is not supported";
        return 0;
    }
    if (cst_parse_signed_suffix(s, "Hz", &v)) {
        if (warn)
            *warn = "Relative pitch change in Hertz is not supported";
        return 0;
    }
    if (cst_parse_signed_suffix(s, "st", &v)) {
        if (warn)
            *warn = "Relative pitch change in semitones is not supported";
        return 0;
    }
    return 0;
}

/* 0x26fbc arm */
int prosody_parse_rate(prosody *p, const char *s, int version, const char **warn)
{
    const prosody_level *l;
    float v;

    if (strcasecmp("default", s) == 0) {
        p->rate = 1.0f;
        if (warn)
            *warn = NULL;
        return 1;
    }
    for (l = rate_levels; l->name; l++)
        if (strcasecmp(l->name, s) == 0) {
            p->rate = l->value;
            if (warn)
                *warn = NULL;
            return 1;
        }
    if (version != SSML_1_0 && cst_parse_unsigned_suffix(s, "%", &v)) {
        p->rate = v * 0.01f;
        if (warn)
            *warn = NULL;
        return 1;
    }
    if (cst_parse_float_suffix(s, "%", &v)) {
        if (v >= -100.0f) {
            p->rate *= (v + 100.0f) * 0.01f;
            if (warn && version == SSML_1_1)
                *warn = "Percentage relative change of prosody rate is deprecated in SSML 1.1";
            return 1;
        }
    } else if (cst_parse_number(s, &v)) {
        if (v >= 0.0f) {
            p->rate = v;
            if (warn && version == SSML_1_1)
                *warn = "Multiplier relative change of prosody rate is deprecated in SSML 1.1";
            return 1;
        }
    }
    return 0;
}

/* 0x271b0 arm: which form the value had (1 default, 2 level, 3 dB, 4 percentage, 5 signed
 * number, 6 number), 0 if invalid */
int prosody_parse_volume(float *volume, const char *s)
{
    const prosody_level *l;
    float v, t;

    if (strcasecmp("default", s) == 0) {
        *volume = 1.0f;
        return 1;
    }
    for (l = volume_levels; l->name; l++)
        if (strcasecmp(l->name, s) == 0) {
            *volume = l->value;
            return 2;
        }
    if (cst_parse_signed_suffix(s, "dB", &v)) {
        *volume *= (float)pow(10.0, v / 20.0);
        return 3;
    }
    if (cst_parse_float_suffix(s, "%", &v)) {
        if (v >= -100.0f) {
            *volume *= v * 0.01f + 1.0f;
            return 4;
        }
    } else if (cst_parse_signed_number(s, &v)) {
        t = v * 0.01f + *volume;
        *volume = t < 0.0f ? 0.0f : t;
        return 5;
    } else if (cst_parse_number(s, &v) && v >= 0.0f) {
        *volume = v * 0.01f;
        return 6;
    }
    return 0;
}
