/* Hash tables (0x56960..0x56cac): chained buckets keyed by byte strings, MurmurHash2. */
#include <string.h>
#include "cst_alloc.h"

typedef struct hash_entry {
    void *key;
    size_t len;
    void *value;
    struct hash_entry *next;
} hash_entry;

typedef struct hash {
    hash_entry **buckets;
    unsigned int size;
    void (*free_value)(void *value);
} hash;

void *cst_memdup(const void *p, int n);

/* 0x56960 arm: MurmurHash2 */
static unsigned int murmur_hash2(const void *key, size_t len)
{
    const unsigned int m = 0x5bd1e995;
    unsigned int h = 0xfa7de9fc ^ len, k;
    const unsigned char *data = key;

    while (len >= 4) {
        k = *(const unsigned int *)data;
        k *= m;
        k ^= k >> 24;
        k *= m;
        h *= m;
        h ^= k;
        data += 4;
        len -= 4;
    }
    switch (len) {
    case 3:
        h ^= data[2] << 16;
    case 2:
        h ^= data[1] << 8;
    case 1:
        h ^= data[0];
        h *= m;
    }
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    return h;
}

/* 0x56a14 arm */
hash *hash_new(unsigned int size, void (*free_value)(void *value))
{
    hash *h;

    if (size == 0)
        return NULL;
    h = cst_safe_alloc(1, sizeof(hash));
    h->buckets = cst_safe_alloc(size, sizeof(hash_entry *));
    h->size = size;
    h->free_value = free_value;
    return h;
}

/* 0x56a54 arm */
void hash_free(hash *h)
{
    unsigned int i;
    hash_entry *e, *next;

    if (!h)
        return;
    if (__builtin_expect(h->size != 0, 1)) {
        i = 0;
        do {
            e = h->buckets[i];
            while (e) {
                next = e->next;
                cst_free(e->key);
                if (h->free_value)
                    h->free_value(e->value);
                cst_free(e);
                e = next;
            }
        } while (++i < h->size);
    }
    cst_free(h->buckets);
    cst_free(h);
}

/* 0x56ad0 arm */
void *hash_get(hash *h, const void *key, size_t len)
{
    const size_t l = len;
    hash_entry *e;

    for (e = h->buckets[murmur_hash2(key, len) % h->size]; e; e = e->next)
        if (l == e->len && memcmp(e->key, key, len) == 0)
            return e->value;
    return NULL;
}

/* 0x56b48 arm: adds or replaces (the old value is not freed). */
void hash_put(hash *h, const void *key, size_t len, void *value)
{
    const size_t l = len;
    unsigned int hval = murmur_hash2(key, len);
    unsigned int idx = hval % h->size;
    register hash_entry **buckets asm("r2") = h->buckets;
    asm("" : "+r"(buckets));
    hash_entry **pp = &buckets[idx];
    hash_entry *e;

    for (; (e = *pp) != NULL; pp = &e->next)
        if (l == e->len && memcmp(e->key, key, len) == 0) {
            e->value = value;
            return;
        }
    e = cst_safe_alloc(1, sizeof(hash_entry));
    e->key = cst_memdup(key, len);
    e->len = len;
    e->next = NULL;
    *pp = e;
    e->value = value;
}

/* 0x56c00 arm */
int hash_remove(hash *h, const void *key, size_t len)
{
    const size_t l = len;
    hash_entry **pp, *e;

    for (pp = &h->buckets[murmur_hash2(key, len) % h->size]; (e = *pp) != NULL; pp = &e->next)
        if (l == e->len && memcmp(e->key, key, len) == 0) {
            *pp = e->next;
            cst_free(e->key);
            if (h->free_value)
                h->free_value(e->value);
            cst_free(e);
            return 1;
        }
    return 0;
}
