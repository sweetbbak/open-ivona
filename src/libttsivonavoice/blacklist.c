/*
 * Unit blacklists (0x5df48..0x5e4f0): sorted lists of unit ids read from an
 * "IVONA_Blacklist" text file, kept in a voice's "blacklist" feature list.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_tokenstream.h"
#include "cst_val.h"
#include "cst_val_defs.h"

typedef struct blacklist {
    unsigned int n;
    unsigned int ids[1];
} blacklist;

/* the diphone database a blacklist plugin applies to */
typedef struct diphonedb_id {
    const char *name;
    int id;
} diphonedb_id;

typedef struct blacklist_plugin {
    const char *db_name;
    int db_id;
    blacklist *bl;
} blacklist_plugin;

CST_VAL_USER_TYPE_DCLS(blacklist, blacklist)
CST_VAL_USER_TYPE_DCLS(diphonedb, diphonedb_id)
int cst_bsearch(const void *key, const void *base, int n, int size,
                int (*cmp)(const void *key, const void *elem, void *arg), void *arg);

/* 0x5df48 arm */
static int blacklist_cmp_sort(const void *a, const void *b)
{
    unsigned int x = *(const unsigned int *)a, y = *(const unsigned int *)b;

    if (x < y)
        return -1;
    return x > y;
}

/* 0x5df6c arm */
static int blacklist_cmp(const void *key, const void *elem, void *arg)
{
    return *(const int *)key - *(const int *)elem;
}

CST_VAL_REGISTER_TYPE_NODEL(blacklist, blacklist)

/* 0x5dfec arm: reads a blacklist; on error writes a message to err and returns NULL. */
blacklist *blacklist_load_ts(cst_tokenstream *ts, char *err, size_t errlen)
{
    blacklist *bl;
    unsigned int cap = 0;
    const char *tok;
    char *end;
    long id;

    ts->p_whitespacesymbols = "\n\r";
    ts->p_singlecharsymbols = "";
    ts->p_prepunctuationsymbols = "";
    ts->p_postpunctuationsymbols = "";
    if (strcmp("IVONA_Blacklist", ts_get(ts)) != 0) {
        snprintf(err, errlen, "Missing IVONA_Blacklist header");
        return NULL;
    }
    bl = cst_safe_calloc(1, sizeof(unsigned int));
    while (!ts_eof(ts)) {
        if (ts_get(ts) == NULL)
            continue;
        tok = ts->token;
        if (*tok == 0)
            continue;
        id = strtol(tok, &end, 10);
        if (*end) {
            snprintf(err, errlen, "Wrong type of value at line %d", ts->line_number);
            cst_free(bl);
            return NULL;
        }
        if (bl->n >= cap) {
            cap += 1024;
            bl = cst_safe_realloc(bl, cap + 1, sizeof(unsigned int));
        }
        /* ids follow the count: word n of the block is the n-th id */
        ((unsigned int *)bl)[++bl->n] = id;
    }
    qsort(bl->ids, bl->n, sizeof(unsigned int), blacklist_cmp_sort);
    return bl;
}

/* 0x5e164 arm */
blacklist *blacklist_load_file(const char *path, char *err, size_t errlen)
{
    cst_tokenstream *ts = ts_open(path);
    blacklist *bl;

    if (ts == NULL) {
        snprintf(err, errlen, "Can't open blacklist");
        return NULL;
    }
    bl = blacklist_load_ts(ts, err, errlen);
    ts_close(ts);
    return bl;
}

/* 0x5e1bc arm */
blacklist *blacklist_load_string(const char *s, char *err, size_t errlen)
{
    cst_tokenstream *ts = ts_open_string(s);
    blacklist *bl = blacklist_load_ts(ts, err, errlen);

    ts_close(ts);
    return bl;
}

/* 0x5e1f0 arm: appends bl to the "blacklist" list of f. */
cst_val *blacklist_add(blacklist *bl, cst_features *f)
{
    cst_val *v = cons_val(blacklist_val(bl), NULL);
    const cst_val *l = get_param_val(f, "blacklist", NULL);

    if (l)
        return val_append((cst_val *)l, v);
    feat_set(f, "blacklist", v);
}

/* 0x5e24c arm: removes bl from the "blacklist" list of f. */
void blacklist_remove(blacklist *bl, cst_features *f)
{
    cst_val *l, *prev, *next;
    const cst_val *tmp;

    if (!feat_present(f, "blacklist"))
        return;
    l = (cst_val *)feat_val(f, "blacklist");
    if (l == NULL)
        return;
    for (prev = NULL; l; prev = l, l = (cst_val *)val_cdr(l))
        if (val_blacklist(val_car(l)) == bl)
            break;
    if (l == NULL)
        return;
    next = (cst_val *)val_cdr(l);
    if (next == NULL && prev == NULL) {
        feat_remove(f, "blacklist");
        return;
    }
    if (prev == NULL) {
        /* the list head stays: move the next element into it and drop that */
        tmp = val_car(l);
        set_car(l, val_car(next));
        set_car(next, tmp);
        prev = l;
        l = next;
        next = (cst_val *)val_cdr(next);
    }
    set_cdr(l, NULL);
    set_cdr(prev, next);
    delete_val(l);
}

/* 0x5e368 arm: whether any blacklist in l contains id. */
int blacklists_contain(const cst_val *l, int id)
{
    const blacklist *bl;

    for (; l; l = val_cdr(l)) {
        bl = val_blacklist(val_car(l));
        if (cst_bsearch(&id, bl->ids, bl->n, sizeof(unsigned int), blacklist_cmp, NULL) != -1)
            return 1;
    }
    return 0;
}

/* 0x5e3ec arm */
void delete_blacklist(blacklist *bl)
{
    cst_free(bl);
}

/* 0x5e3f0 arm: adds the plugin's blacklist if f belongs to its diphone database. */
int blacklist_plugin_add(const blacklist_plugin *p, cst_features *f)
{
    const diphonedb_id *db;

    if (!feat_present(f, "diphonedb"))
        return 0;
    db = val_diphonedb(feat_val(f, "diphonedb"));
    if (strcmp(db->name, p->db_name) != 0 || db->id != p->db_id)
        return 0;
    blacklist_add(p->bl, f);
    return 1;
}

/* 0x5e470 arm */
int blacklist_plugin_remove(const blacklist_plugin *p, cst_features *f)
{
    const diphonedb_id *db;

    if (!feat_present(f, "diphonedb"))
        return 0;
    db = val_diphonedb(feat_val(f, "diphonedb"));
    if (strcmp(db->name, p->db_name) != 0 || db->id != p->db_id)
        return 0;
    blacklist_remove(p->bl, f);
    return 1;
}
