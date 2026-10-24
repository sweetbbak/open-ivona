/*
 * Splitting a token stream (0x4962c..0x4983c).
 */
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"

#define REL_TOKEN 1

/* token features carried over, by type */
static const char *const token_int_feats[] = { "start_pos", "end_pos", NULL };
static const char *const token_float_feats[] = {
    "amplify", "duration_stretch", "pitch_stretch", "silence", NULL
};
static const char *const token_string_feats[] = {
    "name", "prepunctuation", "punc", "whitespace", "phones", "xsampa", "interpret-as",
    "interpret-as-format", "role", NULL
};
static const char *const token_val_feats[] = { "wave", NULL };

/* 0x4962c arm: moves the tokens after tok into the Token relation of a new utterance (with the
 * same engine), copying their features. NULL if tok is the last token. */
cst_utterance *utt_split_tokens(cst_item *tok)
{
    cst_utterance *utt, *nu;
    cst_relation *r;
    cst_item *t, *n;
    const char *const *f;

    if (item_next(tok) == NULL)
        return NULL;
    utt = item_utt(tok);
    nu = new_utterance(utt->engine_a, utt->engine_b);
    r = utt_relation_create(nu, REL_TOKEN);
    while ((t = item_next(tok)) != NULL) {
        n = relation_append(r, NULL);
        for (f = token_int_feats; *f; f++)
            if (item_feat_present(t, *f))
                item_set_int(n, *f, item_feat_int(t, *f));
        for (f = token_float_feats; *f; f++)
            if (item_feat_present(t, *f))
                item_set_float(n, *f, item_feat_float(t, *f));
        for (f = token_string_feats; *f; f++)
            if (item_feat_present(t, *f))
                item_set_string(n, *f, item_feat_string(t, *f));
        for (f = token_val_feats; *f; f++)
            if (item_feat_present(t, *f))
                item_set(n, *f, item_feat(t, *f));
        delete_item(t);
    }
    return nu;
}
