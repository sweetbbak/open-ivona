/*
 * Language tag matching (0x2252c..0x2285c): "en-GB-x" style tags, compared part by part.
 */
#include <string.h>
#include <strings.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_string.h"
#include "ivona_util.h"

/* NUL-terminates the parts of a tag (at most 3; ':' ends it); their number */
static inline unsigned int lang_tag_split(char *s)
{
    unsigned int n = 1;
    int i;

    for (i = 0; s[i]; i++) {
        if (s[i] == ':') {
            s[i] = '\0';
            break;
        }
        if (n <= 2 && s[i] == '-') {
            s[i++] = '\0';
            n++;
            if (!s[i])
                break;
        }
    }
    return n;
}

/* 0x2252c arm: how well tag a matches pattern b: 4 exactly (or b empty), 2 by language and
 * region, 1 by language only, 0 not at all. UK counts as GB. */
int lang_tag_match(const char *a, const char *b)
{
    char *x, *y;
    const char *pa, *pb;
    size_t la, lb;
    unsigned int na, nb;
    int r;

    if (b == NULL || b[0] == '\0' || strcasecmp(a, b) == 0)
        return 4;
    x = cst_strdup(a);
    y = cst_strdup(b);
    na = lang_tag_split(x);
    nb = lang_tag_split(y);
    if (strcasecmp(x, y) != 0)
        r = 0;
    else {
        la = strlen(x);
        lb = strlen(y);
        if (nb > 1 && na > 1) {
            const char *ra, *rb;
            pa = x + la + 1;
            ra = strcasecmp(pa, "UK") == 0 ? "GB" : pa;
            pb = y + lb + 1;
            rb = strcasecmp(pb, "UK") == 0 ? "GB" : pb;
            if (strcasecmp(ra, rb) != 0)
                r = 1;
            else if (nb > 2 && na > 2)
                r = strcasecmp(pa + strlen(pa) + 1, pb + strlen(pb) + 1) == 0 ? 4 : 2;
            else
                r = na == nb ? 4 : 2;
        } else
            r = na != nb ? 1 : 4;
    }
    cst_free(x);
    cst_free(y);
    return r;
}

#define LANG_MAX(a, b) ((a) < (b) ? (b) : (a))

/* 0x227bc arm: the best match of a against a whitespace separated list of patterns (exact
 * matches count 3, unless all are exact) */
int lang_tags_match(const char *a, const char *list)
{
    strlist *l;
    char **p;
    int best, all;

    if (list == NULL || list[0] == '\0')
        return 4;
    l = strlist_split_ws(list);
    best = 0;
    all = 1;
    for (p = l->items; p != l->items + l->count; p++) {
        int m = lang_tag_match(a, *p);
        int score = m == 4 ? 3 : m;
        if (m == 4)
            m = score;
        else
            all = 0;
        if (best < score)
            best = m;
    }
    strlist_free(l);
    if (all)
        return 4;
    return best;
}

/* ------------------------------------------------------------------------------------------ */
/* lang.dat */

#include "ivona_vector.h"
#include "tts_internal.h"

typedef struct isef_file isef_file;
isef_file *isef_open(const char *path);
void isef_free(isef_file *f);
const char *isef_path(const isef_file *f);
int isef_find(const isef_file *f, const char *name, const char **data, size_t *size);

typedef struct textstream textstream;
textstream *textstream_from_mem(const void *data, size_t len);
void textstream_close(textstream *ts);
typedef struct linereader linereader;
linereader *linereader_new(textstream *ts, const char *encoding);
void linereader_free(linereader *lr);
int linereader_next(linereader *lr);
char *linereader_line(const linereader *lr);
const char *isef_next(const isef_file *f, const char *p, size_t n, const char **data, size_t *size);
int dict_load_mem(const char *source, const void *data, size_t len, tts_dict_entry ***dictp);

typedef struct pls_lexicon pls_lexicon;
void pls_lexicon_free(pls_lexicon *l);
pls_lexicon *pls_lexicon_from_data(const char *data, size_t size, const char *arg, void *ctx);

/* 0x229d4 arm */
static void lexicon_ptr_free(void *p, size_t size)
{
    pls_lexicon_free(*(pls_lexicon **)p);
}

/* 0x2285c arm: the lexicons named in list (one "section[=arg]" per line), from the sections of
 * lang.dat f; NULL if one is missing or broken. */
ivona_vector *lang_dat_lexicons(const isef_file *f, const char *list, size_t len, void *ctx)
{
    ivona_vector *v = vector_new(sizeof(pls_lexicon *), NULL, lexicon_ptr_free);
    textstream *ts = textstream_from_mem(list, len);
    linereader *lr;
    const char *data;
    size_t size;
    pls_lexicon *lex;
    char *line, *arg;
    int err = 0;

    if (ts == NULL)
        return v;
    lr = linereader_new(ts, "ASCII");
    while (linereader_next(lr)) {
        if (err)
            break;
        line = linereader_line(lr);
        if ((arg = strchr(line, '=')))
            *arg++ = '\0';
        if (!isef_find(f, line, &data, &size)) {
            if (line[0]) {
                tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "Section \"%s\" not found in lang.dat", line);
                err = -1;
            }
        } else if ((lex = pls_lexicon_from_data(data, size, arg, ctx)) != NULL)
            vector_push(v, &lex);
        else
            err = -1;
        cst_free(line);
    }
    linereader_free(lr);
    textstream_close(ts);
    if (err) {
        vector_free(v);
        return NULL;
    }
    return v;
}

#define IVONA_VERSION "1.6.101.3"

/* 0x229dc arm: 0 if lang.dat f is for this engine version, 3 if it has no version, 2 if it is
 * another (1 for no file). */
int lang_dat_check_version(const isef_file *f)
{
    const char *version = NULL, *config = NULL;
    size_t vlen, clen;

    if (f == NULL)
        return 1;
    if (!isef_find(f, "ivona_version", &version, &vlen)
        || !isef_find(f, "lex_config", &config, &clen)) {
        tts_set_errorf(TTS_STATUS_LOAD_VOICE_ERROR,
                       "wrong file structure: ivona_version or lex_config keys were not present in %s file",
                       isef_path(f));
        return 3;
    }
    if (vlen == 9 && clen == 0 && strncmp(version, IVONA_VERSION, 9) == 0)
        return 0;
    tts_set_errorf((enum tts_status)12, "%s version mismatch: Expected %s (%s); Got %.*s (%.*s)",
                   isef_path(f), IVONA_VERSION, "", vlen, version, clen, config);
    return 2;
}

/* 0x22b08 arm */
int vox_file_check(const char *path)
{
    isef_file *f = isef_open(path);
    int r = lang_dat_check_version(f);

    isef_free(f);
    return r;
}

/* ------------------------------------------------------------------------------------------ */
/* loaded language data */

tts_dict_entry **dict_new(void);
void dict_free(tts_dict_entry **dict);

typedef struct lang_core {
    const char *code;
    int unk_4[4];
    cst_features *features;     /* +0x14 */
    cst_features *params;       /* +0x18 */
} lang_core;
void lang_core_init(lang_core *l);
void lang_core_free(lang_core *l);

typedef struct lang_data {
    char *code;
    tts_dict_entry **dict;      /* +0x04 */
    ivona_vector *unk_8;
    ivona_vector *unk_c;
    lang_core core;             /* +0x10: handed to the language library */
    ivona_vector *lexicons;     /* +0x2c: of lang_section, the sections not read at load */
    isef_file *file;            /* +0x30: lang.dat */
} lang_data;

/* a lang.dat section not read by lang_data_load (lang_data.lexicons) */
typedef struct lang_section {
    const char *name;
    const char *data;
    size_t size;
} lang_section;

/* whether name (n bytes) is a line of the newline-separated list */
static inline int lang_dat_listed(const char *name, size_t n, const char *list)
{
    const char *p;

    for (p = list; *p; ) {
        if (strncmp(name, p, n) == 0 && (p[n] == '\n' || p[n] == 0))
            return 1;
        if ((p = strchr(p, '\n')) == NULL || ++p == NULL)
            break;
    }
    return 0;
}

/* 0x22b2c arm: loads lang.dat at path: its user dictionaries (prm_lex_list), PLS lexicons
 * (pls_lex_list, named_pls_list), lexicon data and the other sections. NULL if it is for another
 * engine version or a list entry fails. */
lang_data *lang_data_load(const char *path, void *ctx)
{
    static const char known[] =
        "named_pls_list\npls_lex_list\nprm_lex_list\nlang_code\nlex_data\nlex_nopsa_data\n";
    isef_file *f = isef_open(path);
    tts_dict_entry **dict = NULL;
    const char *data = NULL, *list = NULL, *lex = NULL, *nopsa = NULL, *name;
    size_t size, list_len, lex_size = 0, nopsa_size = 0, n;
    char *code = NULL, *line, *src;
    ivona_vector *pls = NULL, *named = NULL;
    textstream *ts;
    linereader *lr;
    const char *d;
    size_t ds;
    lang_section sec;
    lang_data *l;
    int r;

    if (lang_dat_check_version(f) != 0)
        goto make;
    if (isef_find(f, "lex_data", &data, &size)) {
        lex = data;
        lex_size = size;
    }
    if (isef_find(f, "lex_nopsa_data", &data, &size)) {
        nopsa = data;
        nopsa_size = size;
    }
    if (isef_find(f, "lang_code", &data, &size))
        code = cst_strndup(data, size);
    if (!isef_find(f, "prm_lex_list", &data, &size)) {
        data = "prm_lexicon\n";
        size = 12;
    }
    dict = dict_new();
    if ((ts = textstream_from_mem(data, size)) != NULL) {
        r = 0;
        lr = linereader_new(ts, "ASCII");
        while (linereader_next(lr)) {
            if (r < 0)
                break;
            line = linereader_line(lr);
            src = cst_nsprintf(strlen(path) + strlen(line) + 2, "%s:%s", path, line);
            if (isef_find(f, line, &d, &ds))
                r = dict_load_mem(src, d, ds, &dict);
            cst_free(src);
            cst_free(line);
        }
        linereader_free(lr);
        textstream_close(ts);
        if (r < 0)
            goto fail;
    }
    if (isef_find(f, "pls_lex_list", &list, &list_len)
        && (pls = lang_dat_lexicons(f, list, list_len, ctx)) == NULL)
        goto fail;
    if (isef_find(f, "named_pls_list", &list, &list_len)
        && (named = lang_dat_lexicons(f, list, list_len, ctx)) == NULL)
        goto fail;
    goto make;
fail:
    dict_free(dict);
    dict = NULL;
    vector_free(pls);
    pls = named = NULL;
    cst_free(code);
    code = NULL;
    lex = nopsa = NULL;
make:
    if (dict == NULL)
        return NULL;
    l = cst_safe_calloc(1, sizeof(lang_data));
    l->dict = dict;
    l->unk_8 = pls;
    l->unk_c = named;
    l->code = code;
    l->file = f;
    l->core.code = code;
    lang_core_init(&l->core);
    l->core.unk_4[0] = (int)lex;
    l->core.unk_4[1] = lex_size;
    l->core.unk_4[2] = (int)nopsa;
    l->core.unk_4[3] = nopsa_size;
    l->lexicons = vector_new(sizeof(lang_section), NULL, NULL);
    for (name = isef_next(f, NULL, 0, &d, &ds); name; name = isef_next(f, d, ds, &d, &ds)) {
        n = strlen(name);
        if (lang_dat_listed(name, n, known) || (data && lang_dat_listed(name, n, data))
            || (list && lang_dat_listed(name, n, list)))
            continue;
        sec.name = name;
        sec.data = d;
        sec.size = ds;
        vector_push(l->lexicons, &sec);
    }
    return l;
}

/* 0x230a8 arm */
lang_data *lang_data_new(void)
{
    lang_data *l = cst_safe_calloc(1, sizeof(lang_data));

    l->dict = dict_new();
    return l;
}

/* 0x230cc arm */
void lang_data_free(lang_data *l)
{
    dict_free(l->dict);
    vector_free(l->unk_8);
    vector_free(l->unk_c);
    cst_free(l->code);
    isef_free(l->file);
    lang_core_free(&l->core);
    vector_free(l->lexicons);
    cst_free(l);
}
