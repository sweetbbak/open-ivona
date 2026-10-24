/*
 * Language tag matching (0x2252c..0x2285c): "en-GB-x" style tags, compared part by part.
 */
#include <string.h>
#include <strings.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "ivona_util.h"

/* NUL-terminates the parts of a tag (at most 3; ':' ends it); their number */
static inline unsigned int lang_tag_split(char *s)
{
    unsigned int n = 1;
    char *p;

    for (p = s; *p; p++) {
        if (*p == ':') {
            *p = '\0';
            break;
        }
        if (n <= 2 && *p == '-') {
            *p = '\0';
            n++;
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
            pa = x + la + 1;
            pb = y + lb + 1;
            if (strcasecmp(strcasecmp(pa, "UK") == 0 ? "GB" : pa,
                           strcasecmp(pb, "UK") == 0 ? "GB" : pb) != 0)
                r = 1;
            else if (nb > 2 && na > 2)
                r = strcasecmp(pa + strlen(pa) + 1, pb + strlen(pb) + 1) == 0 ? 4 : 2;
            else
                r = na == nb ? 4 : 2;
        } else
            r = na == nb ? 4 : 1;
    }
    cst_free(x);
    cst_free(y);
    return r;
}

/* 0x227bc arm: the best match of a against a whitespace separated list of patterns (exact
 * matches count 3, unless all are exact) */
int lang_tags_match(const char *a, const char *list)
{
    strlist *l;
    int best, all, m;
    char **p;

    if (list == NULL || list[0] == '\0')
        return 4;
    l = strlist_split_ws(list);
    best = 0;
    all = 1;
    for (p = l->items; p != l->items + l->count; p++) {
        m = lang_tag_match(a, *p);
        if (m == 4)
            m = 3;
        else
            all = 0;
        if (best < m)
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
        if (err) {
            linereader_free(lr);
            textstream_close(ts);
            vector_free(v);
            return NULL;
        }
        line = linereader_line(lr);
        arg = strchr(line, '=');
        if (arg)
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
