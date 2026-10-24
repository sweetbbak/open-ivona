/*
 * Pieces of an utterance being synthesised (0x27610..0x27920): an utterance with the input span
 * it covers, and the same as a detached list of token descriptions.
 */
#include "cst_features.h"
#include "cst_item.h"
#include "cst_relation.h"
#include "cst_utterance.h"
#include "ivona_ustr.h"
#include "tts_instance.h"

#define REL_TOKEN 1

tts_lang_desc *tts_language_table(tts_instance *tts);
void *lang_part_data(tts_lang_desc *lang, const char *name);

typedef struct utt_chunk {
    cst_utterance *utt;
    ustr_span span;             /* +0x04: input covered */
    void *src;                  /* +0x0c */
    int phrasing_model;         /* +0x10 */
    int break_dur_model;        /* +0x14 */
} utt_chunk;

/* 0x27610 arm (declared int without a value: the last call is not a sibcall) */
int utt_chunk_init(utt_chunk *c, cst_utterance *utt, void *src, int phrasing_model,
                    int break_dur_model)
{
    cst_relation *t = utt_relation(utt, REL_TOKEN);

    c->utt = utt;
    if (relation_head(t) == NULL) {
        c->span.start = 0;
        c->span.end = 0;
    } else {
        c->span.start = item_feat_int(relation_head(t), "start_pos");
        c->span.end = item_feat_int(relation_tail(t), "end_pos");
    }
    c->src = src;
    c->phrasing_model = phrasing_model;
    c->break_dur_model = break_dur_model;
    feat_set_int(utt->features, "phrasing_model", phrasing_model);
    feat_set_int(utt->features, "break_dur_model", break_dur_model);
}

/* 0x276c8 arm: an empty utterance for the instance *src */
void utt_chunk_init_empty(utt_chunk *c, tts_instance **src, int phrasing_model,
                          int break_dur_model)
{
    cst_utterance *utt = new_utterance(tts_language_table(*src), (void *)lang_part_data);

    utt_relation_create(utt, REL_TOKEN);
    utt_chunk_init(c, utt, src, phrasing_model, break_dur_model);
}

/* 0x27724 arm */
void utt_chunk_free_utt(utt_chunk *c)
{
    if (c)
        delete_utterance(c->utt);
}

/* 0x27734 arm: grows the span to cover s */
void utt_chunk_add_span(utt_chunk *c, const ustr_span *s)
{
    if (c->span.start == 0) {
        if (c->span.end == 0) {
            c->span.start = s->start;
            c->span.end = s->end;
            return;
        }
    } else if (c->span.start > s->start)
        c->span.start = s->start;
    if (s->end > c->span.end)
        c->span.end = s->end;
}

/* a token as a list node (token_info_from_item) */
typedef struct token_node token_node;
struct token_node {
    token_node *prev;
    token_node *next;
};

typedef struct token_chunk {
    token_node *head;
    token_node *tail;
    ustr_span span;             /* +0x08 */
    int phrasing_model;         /* +0x10 */
    int break_dur_model;        /* +0x14 */
    void *src;                  /* +0x18 */
} token_chunk;

token_node *token_info_from_item(cst_item *t, void *ctx);
void token_info_free(token_node *n);
void token_info_to_item(token_node *n, cst_relation *r);

/* 0x2777c arm */
void token_chunk_from_utt(const utt_chunk *c, token_chunk *tc, void *ctx)
{
    cst_item *t;
    token_node *n;

    tc->head = NULL;
    tc->tail = NULL;
    tc->span = c->span;
    tc->src = c->src;
    for (t = relation_head(utt_relation(c->utt, REL_TOKEN)); t; t = item_next(t)) {
        n = token_info_from_item(t, ctx);
        n->prev = tc->tail;
        tc->tail = n;
        if (n->prev == NULL)
            tc->head = n;
        else
            n->prev->next = n;
    }
    tc->phrasing_model = feat_int(c->utt->features, "phrasing_model");
    tc->break_dur_model = feat_int(c->utt->features, "break_dur_model");
}

/* 0x27834 arm */
void token_list_remove(token_chunk *tc, token_node *n)
{
    if (n->prev)
        n->prev->next = n->next;
    else
        tc->head = n->next;
    if (n->next)
        n->next->prev = n->prev;
    else
        tc->tail = n->prev;
    token_info_free(n);
}

/* 0x27860 arm */
void audio_chunk_free(token_chunk *tc)
{
    if (tc == NULL)
        return;
    while (tc->head)
        token_list_remove(tc, tc->head);
}

/* 0x27890 arm: an utterance of the chunk's tokens */
cst_utterance *chunk_utterance(token_chunk *tc)
{
    cst_utterance *utt = new_utterance(tts_language_table(*(tts_instance **)tc->src),
                                       (void *)lang_part_data);
    cst_relation *r = utt_relation_create(utt, REL_TOKEN);
    token_node *n;

    for (n = tc->head; n; n = n->next)
        token_info_to_item(n, r);
    feat_set_int(utt->features, "phrasing_model", tc->phrasing_model);
    feat_set_int(utt->features, "break_dur_model", tc->break_dur_model);
    return utt;
}
