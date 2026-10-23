/*
 * User dictionaries (0x37dd0..0x394b0): a voice's NULL-terminated array of rules, each a
 * case-insensitive PCRE pattern and its replacement, applied to input text.
 *
 * Dictionary files hold one rule per line: the pattern and its replacement, each either a bare
 * word (backslash escapes the next character) or a double-quoted string (\" is a quote).
 * '#' starts a comment line. The encoding comes from a BOM or a first line
 * "#& encoding=NAME", else from the caller.
 */
#include <stdio.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_string.h"
#include "ivona_pcre.h"
#include "ivona_util.h"
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
ivona_ustr *ustr_new(unsigned int pos, const char *s, size_t n);
void ustr_clear(ivona_ustr *u);
ivona_ustr *ustr_recode(ivona_ustr *src, ivona_ustr *dst, const char *encoding);

typedef struct ustr_span {
    size_t start;
    size_t end;
} ustr_span;

void ustr_append(ivona_ustr *u, const char *s, size_t n, const ustr_span *span);

/* textstream.c, ustr.c */
typedef struct textstream textstream;
typedef struct linereader linereader;
FILE *ivona_fopen(const char *path, const char *mode);
textstream *textstream_open(const char *path);
textstream *textstream_from_mem(const void *data, size_t len);
void textstream_close(textstream *ts);
const char *textstream_ptr(const textstream *ts);
int textstream_ensure(textstream *ts, size_t n);
linereader *linereader_new(textstream *ts, const char *encoding);
void linereader_free(linereader *lr);
size_t linereader_len(const linereader *lr);
int linereader_lineno(const linereader *lr);
int linereader_next(linereader *lr);
char *linereader_line(const linereader *lr);

int charset_supported(const char *name);
int uni_isspace(unsigned int c);

/* fetch.c */
typedef struct fetcher fetcher;
typedef struct fetch_result {
    void *data;
    size_t size;
} fetch_result;
fetcher *voice_fetcher_new(tts_voice *v);
int fetcher_fetch(fetcher *f, const char *uri, float timeout, void *arg1, void *arg2,
                  cst_features *metrics);
fetch_result *fetcher_result(fetcher *f);
tts_msg *fetcher_messages(fetcher *f);
void fetcher_delete(fetcher *f);

#define FETCH_ABORTED   1
#define FETCH_OK        3

int dict_add(tts_dict_entry ***dictp, const char *rule, const char *change_with, const char *source,
             int line, int index, const char *dict_id, char **err, int keep_rule);

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

/* quoted for a dictionary file: '"' becomes '\"' */
static inline char *dict_quote(const char *s)
{
    char *r;
    int i, j;

    if (s == NULL)
        return NULL;
    r = cst_safe_calloc((strlen(s) + 1) * 2, 1);
    for (i = 0, j = 0; s[i]; i++) {
        if (s[i] == '"')
            r[j++] = '\\';
        r[j++] = s[i];
    }
    return r;
}

/* 0x37dd0 arm: exported. Writes the rules tagged dict_id (all for NULL or "") as UTF-8. */
int tts_dict_save(tts_voice *v, const char *path, const char *dict_id)
{
    tts_dict_entry **p;
    char *rule, *change_with;
    FILE *fp;

    if (v == NULL || path == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return -1;
    }
    fp = ivona_fopen(path, "w");
    if (fp == NULL) {
        tts_set_errorf(TTS_STATUS_SYSTEM_ERROR, "Cannot open file for writing");
        return -1;
    }
    fwrite("#& encoding=UTF-8\n", 1, 18, fp);
    if (dict_id && *dict_id == '\0')
        dict_id = NULL;
    for (p = v->dict; *p; p++) {
        if ((*p)->rule == NULL)
            continue;
        if (dict_id && ((*p)->dict_id == NULL || strcmp(dict_id, (*p)->dict_id) != 0))
            continue;
        rule = dict_quote((*p)->rule);
        change_with = dict_quote((*p)->change_with);
        if (*rule == '\0' || strpbrk(rule, " \t\n"))
            fprintf(fp, "\"%s\" ", rule);
        else
            fprintf(fp, "%s ", rule);
        if (*change_with == '\0' || strpbrk(change_with, " \t\n"))
            fprintf(fp, "\"%s\"\n", change_with);
        else
            fprintf(fp, "%s\n", change_with);
        cst_free(change_with);
        cst_free(rule);
    }
    fclose(fp);
    return 1;
}

/* one field of a rule line: [*start, *end), quotes excluded; returns the position after it or
 * NULL if malformed */
static inline char *dict_field(char *s, char **start, char **end)
{
    if (*s == '"') {
        *start = ++s;
        for (; *s != '"'; s++) {
            if (*s == '\0')
                return NULL;
            if (*s == '\\' && *++s == '\0')
                return NULL;
        }
        *end = s;
        return s + 1;
    }
    *start = s;
    while (*s && !uni_isspace((unsigned char)*s)) {
        if (*s == '\\') {
            s++;
            if (*s == '\0' || uni_isspace((unsigned char)*s))
                return NULL;
        } else if (*s == '"')
            return NULL;
        s++;
    }
    *end = s;
    return s;
}

/* 0x38084 arm: finds the two fields of a rule line. Returns 1, 0 for a blank or comment line,
 * -1 for a malformed one. */
int dict_split_line(char *s, char **rule, char **rule_end, char **change, char **change_end)
{
    while (uni_isspace((unsigned char)*s))
        s++;
    if (*s == '\0' || *s == '#')
        return 0;
    s = dict_field(s, rule, rule_end);
    if (s == NULL || !uni_isspace((unsigned char)*s))
        return -1;
    while (uni_isspace((unsigned char)*s))
        s++;
    s = dict_field(s, change, change_end);
    if (s == NULL)
        return -1;
    while (uni_isspace((unsigned char)*s))
        s++;
    if (*s != '\0')
        return -1;
    return 1;
}

/* the encoding named by a "#& encoding=NAME" line (allocated), or NULL */
static inline char *dict_header_encoding(const char *line)
{
    const char *s, *tok, *val, *eq;
    char *enc = NULL;
    size_t n;

    for (s = line + 2; *s; ) {
        for (tok = s; *tok && strchr(" \t\n", *tok); tok++)
            ;
        if (*tok == '\0')
            break;
        s = strpbrk(tok, " \t");
        if (s == NULL)
            s = tok + strlen(tok);
        n = s - tok;
        eq = memchr(tok, '=', n);
        if (eq) {
            n = eq - tok;
            val = eq + 1;
        } else
            val = s;
        if (n == 8 && strncmp(tok, "encoding", n) == 0) {
            n = s - val;
            enc = cst_safe_alloc(n + 1, 1);
            memcpy(enc, val, n);
            enc[n] = '\0';
        }
    }
    return enc;
}

/* the file's encoding (allocated): header line, BOM, or encoding; NULL if unsupported */
static inline char *dict_encoding(textstream *ts, const char *encoding)
{
    const char *p;
    linereader *lr;
    char *line, *enc = NULL;

    if (textstream_ensure(ts, 2)) {
        p = textstream_ptr(ts);
        if (p[0] == '#' && p[1] == '&') {
            lr = linereader_new(ts, "ASCII");
            if (linereader_next(lr)) {
                line = linereader_line(lr);
                enc = dict_header_encoding(line);
                cst_free(line);
            }
            if (enc == NULL)
                enc = (char *)cst_strdup((const cst_string *)encoding);
            else if (!charset_supported(enc)) {
                tts_set_errorf(TTS_STATUS_FORMAT_ERROR, "Lexicon encoding not supported: %s", enc);
                cst_free(enc);
                enc = NULL;
            }
            linereader_free(lr);
            return enc;
        }
        if (p[0] == '\xff' && p[1] == '\xfe')
            return (char *)cst_strdup((const cst_string *)"UTF-16LE");
        if (p[0] == '\xfe' && p[1] == '\xff')
            return (char *)cst_strdup((const cst_string *)"UTF-16BE");
        if (textstream_ensure(ts, 3)) {
            p = textstream_ptr(ts);
            if (p[0] == '\xef' && p[1] == '\xbb' && p[2] == '\xbf')
                return (char *)cst_strdup((const cst_string *)"UTF-8");
        }
    }
    return (char *)cst_strdup((const cst_string *)encoding);
}

/* in place: \" becomes '"', other escapes are kept; returns the new length */
static inline size_t dict_unescape(char *s, size_t n)
{
    size_t i, j = 0;

    for (i = 0; i < n; i++) {
        if (s[i] == '\\') {
            i++;
            if (s[i] == '"')
                s[j++] = s[i];
            else {
                s[j++] = '\\';
                s[j++] = s[i];
            }
        } else
            s[j++] = s[i];
    }
    return j;
}

typedef struct dict_parser {
    textstream *ts;
    linereader *lr;
    char *encoding;
    char *source;
    strlist *errors;    /* the first three messages */
    unsigned int nerrors;
    int count;
    int keep_rules;
    ivona_ustr *line;
    ivona_ustr *utf8;
} dict_parser;

static inline void dict_parse_error(dict_parser *p, const char *msg)
{
    char buf[128];

    p->nerrors++;
    if (p->errors->count < 3) {
        snprintf(buf, sizeof(buf), "line %ld: %s", (long)linereader_lineno(p->lr), msg);
        strlist_push(p->errors, buf);
    }
}

/* 0x382ec arm: adds the rules of a dictionary file to *dictp. Returns the number added, or -1
 * with the error set (the first three messages and the error count). */
int dict_parse(textstream *ts, const char *encoding, tts_dict_entry ***dictp, const char *source,
               const char *dict_id, int keep_rules)
{
    dict_parser p;
    char *rule, *change, *rule_end, *change_end;
    char *err;
    size_t len, n, n2;
    char **s;
    int ret, r;

    memset(&p, 0, sizeof(p));
    p.encoding = dict_encoding(ts, encoding);
    if (p.encoding == NULL)
        return -1;
    p.ts = ts;
    p.lr = linereader_new(ts, p.encoding);
    p.errors = strlist_new();
    p.source = (char *)cst_strdup((const cst_string *)source);
    p.keep_rules = keep_rules;
    p.line = ustr_new(0, NULL, 0);
    p.utf8 = ustr_new(0, NULL, 0);
    while (linereader_next(p.lr)) {
        len = linereader_len(p.lr);
        if (len != 0) {
            ustr_span span;

            span.start = 0;
            span.end = 0;
            ustr_clear(p.line);
            ustr_append(p.line, textstream_ptr(p.ts), len, &span);
            ustr_clear(p.utf8);
            r = dict_split_line(ustr_recode(p.line, p.utf8, p.encoding)->data,
                                &rule, &rule_end, &change, &change_end);
            if (r == 1) {
                n = dict_unescape(rule, rule_end - rule);
                n2 = dict_unescape(change, change_end - change);
                err = NULL;
                rule[n] = '\0';
                change[n2] = '\0';
                if (dict_add(dictp, rule, change, p.source, linereader_lineno(p.lr), -1, dict_id,
                             &err, p.keep_rules) >= 0)
                    p.count++;
                else if (err) {
                    dict_parse_error(&p, err);
                    cst_free(err);
                }
            } else if (r != 0)
                dict_parse_error(&p, "invalid dictionary format");
        }
        if (p.nerrors >= 100) {
            dict_parse_error(&p, "Too many errors. Quitting");
            break;
        }
    }
    if (p.nerrors == 0)
        ret = p.count;
    else {
        strbuf msg;
        char buf[64];

        strbuf_init(&msg);
        n = 0;
        for (s = p.errors->items; s != p.errors->items + p.errors->count; s++) {
            if (*s == NULL)
                continue;
            n++;
            if (msg.len)
                strbuf_append(&msg, "\n", 1);
            strbuf_append_str(&msg, *s);
        }
        if (n == 0) {
            snprintf(buf, sizeof(buf), "%ld errors found", (long)p.nerrors);
            strbuf_append_str(&msg, buf);
        } else if (p.nerrors > n) {
            snprintf(buf, sizeof(buf), "\nAt least %ld more errors follow", (long)(p.nerrors - n));
            strbuf_append_str(&msg, buf);
        }
        tts_set_errorf(TTS_STATUS_FORMAT_ERROR, "%s", msg.data);
        strbuf_release(&msg);
        ret = -1;
    }
    strlist_free(p.errors);
    linereader_free(p.lr);
    ivona_ustr_free(p.line);
    ivona_ustr_free(p.utf8);
    cst_free(p.encoding);
    cst_free(p.source);
    return ret;
}

/* 0x38b1c arm */
int dict_load_file(tts_dict_entry ***dictp, const char *path, const char *encoding,
                   const char *dict_id)
{
    textstream *ts = textstream_open(path);
    int ret;

    if (ts == NULL)
        return -1;
    ret = dict_parse(ts, encoding, dictp, path, dict_id, 1);
    textstream_close(ts);
    return ret;
}

static inline int dict_fetch(tts_voice *v, const char *uri)
{
    fetcher *f = voice_fetcher_new(v);
    fetch_result *r;
    textstream *ts;
    const char *dict_id;
    tts_msg *m;
    char *msg;
    int ret = -1, status;

    status = fetcher_fetch(f, uri, 120.0f, (void *)-1, (void *)-1, NULL);
    if (status == FETCH_OK) {
        r = fetcher_result(f);
        ts = textstream_from_mem(r->data, r->size);
        dict_id = get_param_string(VOICE_FEATURES(v), "eng_dict_insert_id", NULL);
        ret = dict_parse(ts, get_param_string(VOICE_FEATURES(v), "eng_text_encoding", "UTF-8"),
                         &v->dict, uri, dict_id, 1);
        textstream_close(ts);
    } else if (status == FETCH_ABORTED)
        tts_set_error(TTS_STATUS_USER_ABORTED);
    else if ((m = fetcher_messages(f)) != NULL) {
        msg = tts_msg_join(m);
        tts_set_errorf(TTS_STATUS_SYSTEM_ERROR, "%s", msg);
        cst_free(msg);
    } else
        tts_set_errorf(TTS_STATUS_SYSTEM_ERROR, "Failed fetching lexicon file");
    fetcher_delete(f);
    return ret;
}

/* 0x38b80 arm: exported. Loads a dictionary file, fetching file:// and http:// URIs through the
 * voice's fetcher; returns the number of rules added or -1. */
int tts_dict_load(tts_voice *v, const char *uri)
{
    const char *dict_id;

    if (v == NULL || uri == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return -1;
    }
    if (cst_startswith(uri, "file://") || cst_startswith(uri, "http://"))
        return dict_fetch(v, uri);
    dict_id = get_param_string(VOICE_FEATURES(v), "eng_dict_insert_id", NULL);
    return dict_load_file(&v->dict, uri,
                          get_param_string(VOICE_FEATURES(v), "eng_text_encoding", "UTF-8"),
                          dict_id);
}

/* 0x38d88 arm: adds the rules of an in-memory UTF-8 dictionary (rules not kept for saving). */
int dict_load_mem(const char *source, const void *data, size_t len, tts_dict_entry ***dictp)
{
    textstream *ts = textstream_from_mem(data, len);
    int ret = 0;

    if (ts) {
        ret = dict_parse(ts, "UTF-8", dictp, source, NULL, 0);
        textstream_close(ts);
    }
    return ret;
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
int dict_add(tts_dict_entry ***dictp, const char *rule, const char *change_with, const char *source,
             int line, int index, const char *dict_id, char **err, int keep_rule)
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
    n = dict_add(&v->dict, r->len ? r->data : "", c->len ? c->data : "", NULL, -1, index, dict_id, &err, 1);
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
