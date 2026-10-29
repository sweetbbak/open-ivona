/* IVONA's syllabification trees (postlex_apply_rules, 0x5540c in the voice library). */
#ifndef SYL_CART_H
#define SYL_CART_H

/* a question: the feature of a phone around the consonant cluster being split */
typedef struct syl_cart_feat {
    int kind;                   /* 0: offset-th non-"-" phone before the cluster's vowel, counted
                                   back from the word start; 1: in the cluster; 2: after it;
                                   3: the vowel (offset 1) or the next one (2) */
    unsigned char offset;
    const char *feat;           /* a phone feature, or "stress" */
} syl_cart_feat;

typedef struct syl_cart_node {
    unsigned char feat;         /* question */
    unsigned char val;          /* value asked for; at a leaf the onset size */
    unsigned short yes, no;     /* both 0 at a leaf */
} syl_cart_node;

typedef struct syl_cart {
    const syl_cart_feat *feats;
    const char *const *vals;
    const syl_cart_node *nodes;
} syl_cart;

#endif
