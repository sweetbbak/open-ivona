/*
 * Byte blobs (0x47098..0x476a0): owned data with its length.
 */
#include <string.h>
#include "cst_alloc.h"

typedef struct blob {
    void *data;
    size_t len;
} blob;

/* 0x47098 arm */
blob *blob_new(void)
{
    return cst_safe_calloc(1, sizeof(blob));
}

/* 0x470a4 arm */
void blob_free(blob *b)
{
    if (b == NULL)
        return;
    if (b->data) {
        cst_free(b->data);
        b->data = NULL;
    }
    cst_free(b);
}

/* 0x470d4 arm */
blob *blob_copy(const blob *b)
{
    blob *n;

    if (b == NULL)
        return NULL;
    n = blob_new();
    if (b->data) {
        n->data = cst_safe_alloc(b->len, 1);
        memcpy(n->data, b->data, b->len);
        n->len = b->len;
    }
    return n;
}

/* 0x4712c arm */
int blob_equal(const blob *a, const blob *b)
{
    if (a && b) {
        if (a->len != b->len)
            return 0;
        return memcmp(a->data, b->data, a->len) == 0;
    }
    return a == b;
}
