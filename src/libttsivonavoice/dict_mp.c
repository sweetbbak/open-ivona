/*
 * User dictionaries stored as MessagePack (0x2746c..0x27610): an array of {name, contents}.
 */
#include <string.h>
#include "ivona_tts.h"

typedef struct cmp_ctx cmp_ctx;
int cmp_read_map(cmp_ctx *ctx, unsigned int *size);
int cmp_read_array(cmp_ctx *ctx, unsigned int *size);
void *mp_read_bin(cmp_ctx *ctx, unsigned int *size, int tag);

tts_dict_entry **dict_new(void);
void dict_free(tts_dict_entry **dict);
int dict_load_mem(const char *source, const void *data, size_t len, tts_dict_entry ***dictp);

/* 0x2746c arm */
int dict_read_entry(cmp_ctx *c, tts_dict_entry ***dictp)
{
    unsigned int n, i;
    unsigned int len = 0;
    char *key, *name = NULL, *contents = NULL;
    int ok;

    ok = cmp_read_map(c, &n);
    if (!ok || n == 0)
        return 0;
    for (i = 0; i < n; i++) {
        key = mp_read_bin(c, NULL, 1);
        if (key == NULL)
            return 0;
        if (strcmp(key, "name") == 0)
            name = mp_read_bin(c, NULL, 1);
        else if (strcmp(key, "contents") == 0)
            contents = mp_read_bin(c, &len, 2);
        else
            return 0;
    }
    if (name == NULL || contents == NULL)
        return 0;
    if (dict_load_mem(name, contents, len, dictp) < 0)
        return 0;
    return ok;
}

/* 0x27590 arm */
tts_dict_entry **dict_read(cmp_ctx *c)
{
    tts_dict_entry **dict = dict_new();
    unsigned int n, i;

    if (!cmp_read_array(c, &n))
        goto fail;
    for (i = 0; i < n; i++)
        if (!dict_read_entry(c, &dict))
            goto fail;
    return dict;
fail:
    dict_free(dict);
    return NULL;
}
