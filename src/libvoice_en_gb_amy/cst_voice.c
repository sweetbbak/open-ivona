#include <stddef.h>
#include "cst_features.h"
#include "cst_voice.h"

typedef struct ivona_voice {
    cst_features *features;
    cst_features *ffunctions;
    void *utt_init;
} ivona_voice;

#include "cst_alloc.h"

struct param {
    const char *key;
    const char *val;
};

/* 0x586b0: arm */
ivona_voice *FUN_000586b0(const struct param **params)
{
    ivona_voice *v = cst_safe_alloc(1, 12);
    v->features = new_features();
    if (params && *params) {
        const struct param *p = *params;
        if (p->key) {
            int i = 0, next = 8;
            const char *key = p->key;
            do {
                const struct param *cur = (const struct param *)((const char *)*params + i);
                feat_set_string(v->features, key, cur->val);
                i = next;
                key = *(const char **)((const char *)*params + next);
                next += 8;
            } while (key);
        }
    }
    return v;
}

/* 0x5872c: arm */
ivona_voice *FUN_0005872c(void)
{
    return FUN_000586b0(NULL);
}

/* 0x58734: arm */
void FUN_00058734(ivona_voice *u)
{
    if (u != NULL) {
        delete_features(u->features);
        cst_free(u);
    }
}

/* 0x58754: arm */
int FUN_00058754(void)
{
    return 1;
}
