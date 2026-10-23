/*
 * User dictionaries (0x37dd0..0x394b0): a voice's NULL-terminated array of rules, each a
 * case-insensitive PCRE pattern and its replacement, applied to input text.
 */
#include <string.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_string.h"
#include "ivona_pcre.h"
#include "tts_internal.h"

/* the voice's feature set (voice_feats -> engine voice -> features) */
#define VOICE_FEATURES(v) (**(cst_features ***)(v)->voice_feats)

/* PCRE has no POSIX classes in UTF mode: rules use these Unicode-property spellings */
#define PCRE_DICT_OPTIONS (PCRE_CASELESS | PCRE_UTF8 | PCRE_NO_UTF8_CHECK | PCRE_UCP)

/* a string converted from an input encoding (charset.c) */
typedef struct ivona_ustr {
    char *data;
    int len;
    int unk_8;
    char *unk_c;
} ivona_ustr;

ivona_ustr *ivona_ustr_decode(const char *s, const char *encoding);
void ivona_ustr_free(ivona_ustr *u);

static inline int dict_count(tts_dict_entry **dict)
{
    int n;

    for (n = 0; dict[n]; n++)
        ;
    return n;
}

static inline void dict_entry_free(tts_dict_entry *e)
{
    pcre_free(e->rule_data);
    pcre_free_study(e->extra);
    cst_free(e->dict_id);
    cst_free(e->rule);
    cst_free(e->change_with);
    cst_free(e);
}

/* 0x38dec arm: an empty dictionary (room for 256 rules). */
tts_dict_entry **dict_new(void)
{
    return cst_safe_calloc(256, sizeof(tts_dict_entry *));
}

/* 0x38df8 arm */
void dict_free(tts_dict_entry **dict)
{
    tts_dict_entry **p;

    if (dict == NULL)
        return;
    for (p = dict; *p; p++)
        dict_entry_free(*p);
    cst_free(dict);
}

/* 0x38e6c arm: compiles rule and inserts it before index (appends for an index out of range);
 * returns the new rule count, or -1 with *err set (allocated). */
int dict_add(tts_dict_entry ***dictp, const char *rule, const char *change_with, int unused_3,
             int unused_4, int index, const char *dict_id, char **err, int keep_rule)
{
    tts_dict_entry **dict;
    tts_dict_entry *e;
    char *pattern, *tmp;
    const char *errptr;
    int erroffset;
    pcre *re;
    pcre_extra *extra;
    int n, cap, newcap;

    n = dict_count(*dictp);
    if (rule == NULL || change_with == NULL)
        return n;
    tmp = cst_strreplace("[:graph:]", "\\p{L}\\p{N}\\p{P}\\{S}", rule);
    pattern = cst_strreplace("[:lower:]", "\\p{Ll}", tmp);
    cst_free(tmp);
    tmp = cst_strreplace("[:print:]", "\\P{C}", pattern);
    cst_free(pattern);
    pattern = cst_strreplace("[:punct:]", "\\p{P}\\p{S}", tmp);
    cst_free(tmp);
    tmp = cst_strreplace("[:upper:]", "\\p{Lu}", pattern);
    cst_free(pattern);
    pattern = cst_strreplace("[:space:]", "[:space:]\n", tmp);
    cst_free(tmp);

    re = pcre_compile(pattern, PCRE_DICT_OPTIONS, &errptr, &erroffset, NULL);
    if (re == NULL) {
        cst_free(pattern);
        *err = cst_asprintf("Regex compilation error at offset %d: %s", erroffset, errptr);
        return -1;
    }
    extra = pcre_study(re, PCRE_STUDY_JIT_COMPILE, &errptr);
    if (extra == NULL && errptr != NULL) {
        cst_free(pattern);
        *err = cst_asprintf("Regex analysis error: %s", errptr);
        return -1;
    }
    e = cst_safe_alloc(1, sizeof(tts_dict_entry));
    e->rule_data = re;
    e->rule = keep_rule ? (char *)cst_strdup((const cst_string *)rule) : NULL;
    e->change_with = (char *)cst_strdup((const cst_string *)change_with);
    e->dict_id = dict_id && *dict_id ? (char *)cst_strdup((const cst_string *)dict_id) : NULL;
    e->extra = extra;
    cst_free(pattern);

    if (index > n || index < 0)
        index = n;
    dict = *dictp;
    cap = (dict_count(dict) & ~0xff) + 0x100;
    newcap = ((n + 2) & ~0xff) + 0x100;
    if (newcap != cap) {
        tts_dict_entry **d = cst_safe_calloc(newcap, sizeof(tts_dict_entry *));
        d = memmove(d, dict, (cap < newcap ? cap : newcap) * sizeof(tts_dict_entry *));
        cst_free(dict);
        dict = d;
    }
    *dictp = dict;
    if (index < n) {
        memmove(&dict[index + 1], &dict[index], (n - index) * sizeof(tts_dict_entry *));
        dict = *dictp;
    }
    dict[index] = e;
    return n + 1;
}

/* 0x391d4 arm: exported. */
int tts_dict_num_entries(const tts_voice *v)
{
    if (v == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return -1;
    }
    return dict_count(v->dict);
}

/* 0x39218 arm: exported. rule and change_with are in the voice's text encoding; new rules are
 * tagged with the voice's "eng_dict_insert_id". Returns the rule count or -1. */
int tts_dict_insert(tts_voice *v, const char *rule, const char *change_with, int index)
{
    const char *dict_id, *enc;
    ivona_ustr *r, *c;
    char *err;
    int n;

    if (v == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return -1;
    }
    if (rule == NULL || change_with == NULL)
        return dict_count(v->dict);
    dict_id = get_param_string(VOICE_FEATURES(v), "eng_dict_insert_id", NULL);
    r = ivona_ustr_decode(rule, get_param_string(VOICE_FEATURES(v), "eng_text_encoding", "UTF-8"));
    enc = get_param_string(VOICE_FEATURES(v), "eng_text_encoding", "UTF-8");
    c = ivona_ustr_decode(change_with, enc);
    n = dict_add(&v->dict, r->len ? r->data : "", c->len ? c->data : "", 0, -1, index, dict_id, &err, 1);
    ivona_ustr_free(r);
    ivona_ustr_free(c);
    if (n == -1) {
        tts_set_errorf(TTS_STATUS_FORMAT_ERROR, "%s", err);
        cst_free(err);
    }
    return n;
}

/* 0x393ac arm: exported; returns the remaining rule count or -1. */
int tts_dict_remove(tts_voice *v, int index)
{
    tts_dict_entry **dict;
    tts_dict_entry *e;
    int n;

    if (v == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return -1;
    }
    dict = v->dict;
    n = dict_count(dict);
    if (index < 0 || index >= n) {
        tts_set_error(TTS_STATUS_RANGE_ERROR);
        return -1;
    }
    e = dict[index];
    if (e) {
        dict_entry_free(e);
        dict = v->dict;
    }
    memmove(&dict[index], &dict[index + 1], (n - index) * sizeof(tts_dict_entry *));
    return n - 1;
}

/* 0x39490 arm: exported; frees every rule and starts an empty dictionary. */
void tts_dict_cleanup(tts_voice *v)
{
    if (v == NULL)
        return;
    dict_free(v->dict);
    v->dict = dict_new();
}
