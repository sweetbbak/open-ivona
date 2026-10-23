/*
 * Voices (0x4029c..0x414c4): loading a voice library (dlopen, register_voice, its language data and
 * vox file), unloading it, and the per-voice PLS lexicons.
 */
#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_string.h"
#include "cst_val.h"
#include "ivona_util.h"
#include "ivona_vector.h"
#include "tts_cert.h"
#include "tts_instance.h"
#include "tts_internal.h"
#include "tts_voice.h"

#define ENGINE_VERSION "1.6.101.3"

typedef struct pls_entry {
    int id;
    void *lex;
} pls_entry;


void *res_cache_acquire(res_cache *c, const char *path, void *arg);
void res_cache_release(res_cache *c, void *data);

/* tts_api.c */
void *tts_get_certificate(tts_instance *tts);
tts_lang_desc *tts_language_table(tts_instance *tts);
void tts_add_module(tts_instance *tts, void *module);
void tts_remove_module(tts_instance *tts, void *module);
int lang_load_part(tts_lang_desc *desc, tts_instance *tts, const char *path);
void lang_desc_free(tts_lang_desc *desc, tts_instance *tts);

int cert_allows_voice(tts_cert_data *cert, const char *lector, const char *lang_codes, int custom);
tts_lang_desc *lang_desc_find(tts_lang_desc *table, const char *code);
void voice_reg_fixup(voice_reg *reg);
void voice_attach_language(voice_reg *reg, tts_lang_desc *lang,
                           void *(*part_data)(tts_lang_desc *lang, const char *name));
int vox_file_check(const char *path);
void voice_params_init(tts_voice *v);
void pls_lexicon_free(void *lex);
void *pls_lexicon_parse(const char *data, size_t size, const char *lang_codes, tts_msg **msgs,
                        int flags);
tts_dict_entry **dict_new(void);
void dict_free(tts_dict_entry **dict);

/* textstream.c */
typedef struct textstream textstream;
textstream *textstream_open(const char *path);
void textstream_close(textstream *ts);
size_t textstream_avail(const textstream *ts);
const char *textstream_ptr(const textstream *ts);
size_t textstream_fill_all(textstream *ts);

/* fetch.c */
typedef struct fetcher fetcher;
typedef struct fetch_result {
    void *data;
    size_t size;
} fetch_result;
typedef int (*fetch_fn)(void *req, void *res);
fetcher *fetcher_new(void);
void fetcher_set_referer(fetcher *f, const char *referer);
void fetcher_set_base(fetcher *f, const char *base);
void fetcher_set_callback(fetcher *f, fetch_fn fetch, void *user);
int fetcher_fetch(fetcher *f, const char *uri, float timeout, void *arg1, void *arg2,
                  cst_features *metrics);
fetch_result *fetcher_result(fetcher *f);
tts_msg *fetcher_messages(fetcher *f);
void fetcher_delete(fetcher *f);

#define FETCH_ABORTED   1
#define FETCH_OK        3

/* 0x4029c arm: the part of lang named name, else the last one named fallback, else the first. */
lang_part *lang_find_part(tts_lang_desc *lang, const char *name, const char *fallback)
{
    lang_part *part = NULL, *found = NULL;
    lang_part **p, **end;

    if (lang) {
        for (p = lang->parts->data, end = p + lang->parts->count; p != end; p++) {
            part = *p;
            if (name && strcmp(part->name, name) == 0)
                goto out;
            if (strcmp(part->name, fallback) == 0)
                found = part;
        }
        part = found;
        if (part == NULL && vector_size(lang->parts) != 0)
            part = *(lang_part **)vector_get(lang->parts, 0);
    }
out:
    return part;
}

/* 0x40388 arm: handed to the voice library to find its language data. */
void *lang_part_data(tts_lang_desc *lang, const char *name)
{
    lang_part *p = lang_find_part(lang, name, name);

    return p ? p->data : NULL;
}

/* 0x403a0 arm: destructor of voice_data.pls elements */
static void pls_entry_free(void *elem, size_t size)
{
    pls_lexicon_free(((pls_entry *)elem)->lex);
}

static inline pls_entry *pls_find(const ivona_vector *pls, int id)
{
    pls_entry *p, *end;

    for (p = pls->data, end = p + pls->count; p != end; p++)
        if (p->id == id)
            return p;
    return NULL;
}

/* 0x403a8 arm: parses a PLS lexicon and adds it to the voice; returns its id or -1. Parser
 * messages become the error (a warning when the lexicon loaded). */
int voice_pls_add(tts_voice *v, const char *data, size_t size)
{
    tts_msg *msgs = NULL;
    pls_entry e;
    char *msg;
    void *lex;
    int id;

    lex = pls_lexicon_parse(data, size, v->voice->lang_codes, &msgs, 0);
    msg = msgs ? tts_msg_join(msgs) : NULL;
    if (lex) {
        for (id = 1; pls_find(VOICE_DATA(v)->pls, id); )
            if (++id == INT_MAX) {
                tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "Too many PLS lexicons");
                id = -1;
                break;
            }
        e.id = id;
        e.lex = lex;
        vector_push(VOICE_DATA(v)->pls, &e);
        id = e.id;
        if (msg)
            tts_set_errorf(TTS_STATUS_FORMAT_ERROR, "%s", msg);
        else
            tts_set_error(TTS_STATUS_OK);
    } else {
        if (msg)
            tts_set_errorf(TTS_STATUS_FORMAT_ERROR, "%s", msg);
        else
            tts_set_errorf(TTS_STATUS_FORMAT_ERROR, "Failed parsing PLS lexicon file");
        id = -1;
    }
    cst_free(msg);
    tts_msg_free(msgs);
    return id;
}

/* 0x40530 arm */
voice_data *voice_data_new(voice_reg *reg, void *vox)
{
    voice_data *vd = cst_safe_alloc(1, sizeof(voice_data));

    vd->reg = reg;
    vd->vox = vox;
    vd->pls = vector_new(sizeof(pls_entry), NULL, pls_entry_free);
    vd->features = new_features();
    vd->lang = NULL;
    return vd;
}

/* 0x40588 arm */
void voice_data_free(voice_data *vd)
{
    vector_free(vd->pls);
    delete_features(vd->features);
    cst_free(vd);
}

/* 0x405ac arm: dlsym, setting the error when the symbol is missing. */
void *ivona_dlsym(void *dl, const char *name)
{
    void *p = dlsym(dl, name);

    if (p == NULL)
        tts_set_errorf(TTS_STATUS_SYSTEM_ERROR, "%s", dlerror());
    return p;
}

/* "<prefix>_<lang>_<name>.dat", lowercased, with separators as '_' */
/* 0x405e8 arm */
void voice_dat_name(strbuf *b, const char *prefix, const char *lang, const char *name)
{
    char *s;

    strbuf_append_str(b, prefix);
    strbuf_append_str(b, "_");
    s = cst_ascii_downcase(lang);
    cst_strsubchar(s, '-', '_');
    cst_strsubchar(s, ' ', '_');
    strbuf_append_str(b, s);
    cst_free(s);
    strbuf_append_str(b, "_");
    s = cst_ascii_downcase(name);
    cst_strsubchar(s, ' ', '_');
    cst_strsubchar(s, '/', '_');
    strbuf_append_str(b, s);
    cst_free(s);
    strbuf_append_str(b, ".dat");
}

/* 0x406b0 arm: "lang_<lang>.dat" */
void lang_dat_name(strbuf *b, const char *lang)
{
    char *s;

    strbuf_append_str(b, "lang_");
    s = cst_ascii_downcase(lang);
    cst_strsubchar(s, '-', '_');
    cst_strsubchar(s, ' ', '_');
    strbuf_append_str(b, s);
    cst_free(s);
    strbuf_append_str(b, ".dat");
}

/* the directory of data files: $IVONA_LANG_DAT_DIR, else that of dir (or of the library) */
static inline void dir_name(strbuf *b, const char *path)
{
    size_t n = strlen(path);

    while (n && !strchr("/\\", path[n - 1]))
        n--;
    strbuf_append(b, path, n);
}

#define DAT_DIR(b, sep, dir, lib_path)                          \
    do {                                                        \
        const char *env_ = getenv("IVONA_LANG_DAT_DIR");        \
        if (env_) {                                             \
            strbuf_append_str(b, env_);                         \
            sep = '/';                                          \
            strbuf_append(b, &sep, 1);                          \
        } else                                                  \
            dir_name(b, dir ? dir : lib_path);                  \
    } while (0)

static inline void voice_unregister(void *dl, voice_reg *reg)
{
    void (*unreg)(voice_reg *) = ivona_dlsym(dl, "unregister_voice");

    if (unreg)
        unreg(reg);
}

/* undoes a partly loaded voice */
static inline void voice_load_abort(tts_instance *tts, void *dl, voice_reg *reg, tts_voice_lib *lib,
                                    tts_lang_desc *own)
{
    voice_unregister(dl, reg);
    if (lib) {
        cst_free((void *)lib->name);
        cst_free(lib);
    }
    lang_desc_free(own, tts);
    dlclose(dl);
}

/* 0x40720 arm: exported. Loads the voice library lib_path; its vox file is found by the library
 * from vox_path, and data files next to it (or $IVONA_LANG_DAT_DIR). */
tts_voice *tts_load_voice(tts_instance *tts, const char *lib_path, const char *vox_path)
{
    tts_cert_data *cert;
    void *dl;
    voice_reg *(*reg_fn)(const char *);
    voice_reg *reg;
    const char *lang_code, *name_ascii;
    tts_lang_desc *lang, *own, *lang_data;
    tts_voice_lib *lib;
    tts_voice *v;
    strbuf path;
    res_cache *cache;
    void *vox;
    int status, ok, err;
    char sep;

    if (tts == NULL || lib_path == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return NULL;
    }
    cert = tts_get_certificate(tts);
    if (cert == NULL)
        return NULL;
    status = cert_status(cert);
    if (status != 0) {
        if (status == 1)
            tts_set_error(TTS_STATUS_CERTIFICATE_EXPIRED);
        return NULL;
    }
    dl = dlopen(lib_path, RTLD_NOW);
    if (dl == NULL) {
        tts_set_errorf(TTS_STATUS_SYSTEM_ERROR, "%s", dlerror());
        return NULL;
    }
    reg_fn = ivona_dlsym(dl, "register_voice");
    if (reg_fn == NULL) {
        dlclose(dl);
        return NULL;
    }
    errno = 0;
    reg = reg_fn(vox_path);
    if (reg == NULL) {
        err = errno;
        dlclose(dl);
        if (err == ENOENT)
            tts_set_errorf(TTS_STATUS_LOAD_VOICE_ERROR, "The vox file does not exist");
        else if (err != 0)
            tts_set_error_errno(errno, NULL);
        else
            tts_set_error(TTS_STATUS_LOAD_VOICE_ERROR);
        return NULL;
    }
    if (!feat_present(reg->features, "language_code")
        || !feat_present(reg->features, "voice_name_ascii")) {
        dlclose(dl);
        return NULL;
    }
    lang_code = feat_string(reg->features, "language_code");
    name_ascii = feat_string(reg->features, "voice_name_ascii");
    voice_reg_fixup(reg);

    /* language data: the instance's, else the voice library's own */
    lang = tts_language_table(tts);
    own = NULL;
    if (lang_desc_find(lang, lang_code))
        ok = vector_size(lang->parts) != 0;
    else {
        lang_data = ivona_dlsym(dl, "language_data");
        lang = lang_desc_find(lang_data, lang_code);
        if (lang == NULL) {
            tts_set_errorf(TTS_STATUS_INVALID_ARGUMENT, "The voice library does not have language data");
            dlclose(dl);
            return NULL;
        }
        own = cst_safe_alloc(1, sizeof(tts_lang_desc));
        *own = *lang;
        own->parts = vector_new(sizeof(lang_part *), NULL, NULL);
        strbuf_init(&path);
        DAT_DIR(&path, sep, vox_path, lib_path);
        voice_dat_name(&path, "lang", lang_code, name_ascii);
        ok = lang_load_part(own, tts, path.data);
        if (!ok) {
            strbuf_clear(&path);
            DAT_DIR(&path, sep, vox_path, lib_path);
            lang_dat_name(&path, lang_code);
            ok = lang_load_part(own, tts, path.data);
        }
        strbuf_release(&path);
        lang = own;
    }
    if (!ok) {
        lang_desc_free(own, tts);
        dlclose(dl);
        return NULL;
    }
    voice_attach_language(reg, lang, lang_part_data);

    lib = NULL;
    if (!feat_present(reg->features, "voice_name") || !feat_present(reg->features, "voice_gender")
        || !feat_present(reg->features, "eng_version")
        || !feat_present(reg->features, "language_name")
        || !feat_present(reg->features, "language_code"))
        goto invalid;
    lib = cst_safe_alloc(1, sizeof(tts_voice_lib));
    lib->lector = feat_string(reg->features, "voice_name");
    lib->language = feat_string(reg->features, "language_name");
    lib->lang_codes = feat_string(reg->features, "language_code");
    lib->gender = feat_string(reg->features, "voice_gender");
    lib->version = feat_string(reg->features, "eng_version");
    lib->vox_internals = NULL;
    lib->vox_data = NULL;
    lib->lector_ascii = feat_string(reg->features, "voice_name_ascii");
    lib->name = cst_asprintf("IVONA 2 - %s - %s", lib->language, lib->lector);
    if (lib->version == NULL)
        goto invalid;
    if (strcmp(ENGINE_VERSION, lib->version) != 0) {
        tts_set_errorf(TTS_STATUS_LOAD_LANGUGE_ERROR + 1,
                       "Voice library version mismatch: got %s, required %s", lib->version,
                       ENGINE_VERSION);
        goto fail;
    }
    if (!cert_allows_voice(cert, lib->lector, lib->lang_codes,
                           get_param_int(reg->features, "custom_voice", 0))) {
        voice_load_abort(tts, dl, reg, lib, own);
        tts_set_errorf(TTS_STATUS_CERTIFICATE_LIMIT, "Voice not allowed by the certificate");
        return NULL;
    }

    cache = tts->lang_cache;
    strbuf_init(&path);
    DAT_DIR(&path, sep, vox_path, lib_path);
    voice_dat_name(&path, "voice", lib->lang_codes, lib->lector_ascii);
    vox = res_cache_acquire(cache, path.data, (void *)lib->lang_codes);
    status = vox_file_check(path.data);
    strbuf_release(&path);
    if ((unsigned int)status > 1)
        goto fail;

    v = cst_safe_alloc(1, sizeof(tts_voice));
    v->voice_feats = voice_data_new(reg, vox);
    v->voice = lib;
    v->sample_rate = get_param_int(reg->features, "sample_rate", 0);
    v->voice_specific = dl;
    v->tts = tts;
    v->vox_name = get_param_string(reg->features, "name", "anonymous");
    VOICE_DATA(v)->lang = own;
    voice_params_init(v);
    tts_add_module(tts, v);
    v->dict = dict_new();
    return v;

invalid:
    tts_set_errorf(TTS_STATUS_LOAD_VOICE_ERROR, "Invalid voice library data");
fail:
    voice_load_abort(tts, dl, reg, lib, own);
    return NULL;
}

/* 0x41064 arm: exported. Returns 1, or 0 if the library could not be unregistered or closed. */
int tts_unload_voice(tts_voice *v)
{
    void (*unreg)(voice_reg *);
    voice_reg *reg;
    tts_voice_lib *lib;
    int ret = 1;

    if (v) {
        tts_remove_module(v->tts, v);
        reg = VOICE_DATA(v)->reg;
        unreg = ivona_dlsym(v->voice_specific, "unregister_voice");
        ret = 0;
        if (unreg) {
            unreg(reg);
            ret = 1;
        }
        if (VOICE_DATA(v)->lang)
            lang_desc_free(VOICE_DATA(v)->lang, v->tts);
        if (dlclose(v->voice_specific) != 0) {
            tts_set_errorf(TTS_STATUS_SYSTEM_ERROR, "%s", dlerror());
            ret = 0;
        }
        if (VOICE_DATA(v)->vox)
            res_cache_release(v->tts->lang_cache, VOICE_DATA(v)->vox);
        voice_data_free(VOICE_DATA(v));
        dict_free(v->dict);
        lib = (tts_voice_lib *)v->voice;
        if (lib) {
            cst_free((void *)lib->name);
            cst_free(lib);
        }
        cst_free(v);
    }
    return ret;
}

/* 0x41154 arm: exported; returns 1, or 0 for an unknown id. */
int tts_pls_unload(tts_voice *v, int id)
{
    ivona_vector *pls;
    pls_entry *p, *end;

    if (v == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return -1;
    }
    pls = VOICE_DATA(v)->pls;
    for (p = pls->data, end = p + pls->count; p != end; p++)
        if (p->id == id) {
            vector_erase(pls, p - (pls_entry *)vector_at(pls, 0), 1);
            return 1;
        }
    return 0;
}

/* 0x411d8 arm: a fetcher set up from the voice's SSML base URI and fetch callback. */
fetcher *voice_fetcher_new(tts_voice *v)
{
    fetcher *f = fetcher_new();
    const char *base = get_param_string(VOICE_FEATURES(v), "eng_ssml_base_uri", NULL);
    void *user;

    if (base && *base) {
        fetcher_set_base(f, base);
        fetcher_set_referer(f, base);
    }
    if (feat_present(VOICE_FEATURES(v), "eng_fetch_function")) {
        user = feat_present(VOICE_FEATURES(v), "eng_fetch_user_data")
             ? val_userdata(feat_val(VOICE_FEATURES(v), "eng_fetch_user_data")) : NULL;
        fetcher_set_callback(f, (fetch_fn)val_userdata(feat_val(VOICE_FEATURES(v), "eng_fetch_function")),
                             user);
    }
    return f;
}

static inline int voice_pls_fetch(tts_voice *v, const char *uri)
{
    fetcher *f = voice_fetcher_new(v);
    fetch_result *r;
    tts_msg *m;
    char *msg;
    int ret = -1, status;

    status = fetcher_fetch(f, uri, 120.0f, (void *)-1, (void *)-1, NULL);
    if (status == FETCH_OK) {
        r = fetcher_result(f);
        ret = voice_pls_add(v, r->data, r->size);
    } else if (status == FETCH_ABORTED)
        tts_set_error(TTS_STATUS_USER_ABORTED);
    else if ((m = fetcher_messages(f)) != NULL) {
        msg = tts_msg_join(m);
        tts_set_errorf(TTS_STATUS_SYSTEM_ERROR, "%s", msg);
        cst_free(msg);
    } else
        tts_set_errorf(TTS_STATUS_SYSTEM_ERROR, "Failed fetching PLS lexicon file");
    fetcher_delete(f);
    return ret;
}

/* 0x412e8 arm: exported. Loads a PLS lexicon from a file or a file:// or http:// URI (SSML must
 * be licensed); returns its id or -1. */
int tts_pls_load(tts_voice *v, const char *uri)
{
    tts_cert_data *cert;
    textstream *ts;
    int ret;

    if (v == NULL || uri == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return -1;
    }
    cert = tts_get_certificate(v->tts);
    if (cert == NULL || !cert->ssml) {
        tts_set_error(TTS_STATUS_CERTIFICATE_LIMIT);
        return -1;
    }
    if (cst_startswith(uri, "file://") || cst_startswith(uri, "http://"))
        return voice_pls_fetch(v, uri);
    ts = textstream_open(uri);
    if (ts == NULL)
        return -1;
    textstream_fill_all(ts);
    ret = voice_pls_add(v, textstream_ptr(ts), textstream_avail(ts));
    textstream_close(ts);
    return ret;
}

/* 0x414a0 arm: exported */
int tts_voice_sample_rate(const tts_voice *v)
{
    if (v == NULL) {
        tts_set_error(TTS_STATUS_INVALID_ARGUMENT);
        return -1;
    }
    return v->sample_rate;
}

/* 0x414c4 arm: the voice's language data part name (default: its language code's). */
lang_part *voice_lang_part(tts_voice *v, const char *name)
{
    const char *code;
    tts_lang_desc *lang;

    if (v == NULL)
        return NULL;
    code = feat_string(VOICE_FEATURES(v), "language_code");
    lang = VOICE_DATA(v)->lang;
    if (lang == NULL)
        lang = lang_desc_find(tts_language_table(v->tts), name);
    if (lang == NULL)
        lang = lang_desc_find(tts_language_table(v->tts), code);
    return lang_find_part(lang, name, code);
}
