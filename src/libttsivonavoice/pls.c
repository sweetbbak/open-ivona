/*
 * PLS pronunciation lexicons (0x248a0..0x26d38): parsed with expat into entries by grapheme.
 */
#include <string.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "ivona_vector.h"

/* 0x248a0 arm: orders entries by name */
int pls_name_cmp(const void *entry, const void *name)
{
    return strcmp(*(const char *const *)entry, name);
}

/* a lexicon item: a lexeme (0) or a pronunciation part (1..3) */
typedef struct pls_item {
    unsigned int type;
    char *text;
    void *data;                 /* vector (lexeme) or string */
    int unk_c;
} pls_item;

/* 0x248a8 arm */
void pls_item_clear(pls_item *it)
{
    if (it->type == 0) {
        cst_free(it->text);
        vector_free(it->data);
    } else if (it->type <= 3) {
        cst_free(it->text);
        cst_free(it->data);
    }
    memset(it, 0, sizeof(pls_item));
}

typedef struct pls_entry {
    char *name;
    ivona_vector *items;
} pls_entry;

/* 0x24900 arm */
static void pls_entry_destroy(void *p, size_t size)
{
    pls_entry *e = p;

    cst_free(e->name);
    vector_free(e->items);
}

typedef struct pls_lexicon {
    ivona_vector *entries;      /* of pls_entry, by name */
    char *unk_4;
    char *alphabet;             /* +0x08 */
    char *lang;                 /* +0x0c */
} pls_lexicon;

/* 0x25450 arm */
pls_lexicon *pls_lexicon_new(void)
{
    pls_lexicon *l = cst_safe_calloc(1, sizeof(pls_lexicon));

    l->entries = vector_new(sizeof(pls_entry), NULL, pls_entry_destroy);
    return l;
}

/* 0x25488 arm */
void pls_lexicon_free(pls_lexicon *l)
{
    if (l == NULL)
        return;
    vector_free(l->entries);
    cst_free(l->lang);
    cst_free(l->unk_4);
    cst_free(l->alphabet);
    cst_free(l);
}

/* 0x254c0 arm */
void pls_lexicon_set_lang(pls_lexicon *l, const char *lang)
{
    cst_free(l->lang);
    l->lang = cst_strdup(lang);
}

/* 0x254e4 arm */
const char *pls_lexicon_lang(const pls_lexicon *l)
{
    return l->lang;
}

/* 0x254ec arm */
void pls_lexicon_set_alphabet(pls_lexicon *l, const char *alphabet)
{
    cst_free(l->alphabet);
    l->alphabet = cst_strdup(alphabet);
}

/* 0x25510 arm */
const char *pls_lexicon_alphabet(const pls_lexicon *l)
{
    return l->alphabet;
}

typedef struct pls_text {
    int unk_0;
    char *text;
    int unk_8;
} pls_text;

/* 0x25760 arm */
void pls_text_clear(pls_text *t)
{
    cst_free(t->text);
    memset(t, 0, sizeof(pls_text));
}

#include "ivona_util.h"
#include "tts_internal.h"

pls_lexicon *pls_lexicon_parse(const char *data, size_t size, void *ctx, tts_msg **msgs, int level);

/* 0x26cb4 arm */
pls_lexicon *pls_lexicon_from_data(const char *data, size_t size, const char *lang, void *ctx)
{
    tts_msg *msgs;
    pls_lexicon *l = pls_lexicon_parse(data, size, ctx, &msgs, 3);
    char *s;

    if (l) {
        if (lang)
            pls_lexicon_set_lang(l, lang);
    } else {
        s = tts_msg_join(msgs);
        tts_set_errorf(TTS_STATUS_INTERNAL_ERROR, "%s", s);
        cst_free(s);
    }
    if (msgs)
        tts_msg_free(msgs);
    return l;
}
