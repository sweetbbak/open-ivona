/*
 * Tokens of an utterance as list nodes (0x356d8..0x36a3c): what token_chunk_from_utt
 * (utt_chunk.c) keeps of each token while it rearranges them.
 */
#include "cst_alloc.h"
#include "cst_val.h"

typedef struct token_node token_node;
struct token_node {
    token_node *prev;
    token_node *next;
};

#define TOKEN_INFO_VAL 5

typedef struct token_info {
    token_node node;
    int unk_8[5];
    int type;                   /* +0x1c: TOKEN_INFO_* */
    int unk_20[2];
    cst_val *val;               /* +0x28: TOKEN_INFO_VAL */
} token_info;

/* 0x36220 arm */
void token_info_free(token_info *t)
{
    if (t->type == TOKEN_INFO_VAL)
        delete_val(t->val);
    cst_free(t);
}
