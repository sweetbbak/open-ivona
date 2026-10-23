/*
 * Certificates (0x42a50..0x441c0): parsing a license certificate, its validity and the voices it
 * allows.
 */
#include <errno.h>
#include <string.h>
#include <time.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_string.h"
#include "cst_val.h"
#include "ivona_util.h"
#include "tts_cert.h"
#include "tts_internal.h"

/* 0x42a50 arm */
int cert_free(tts_cert_data *c)
{
    cst_featvalpair *p;

    if (c) {
        for (p = c->attrs->head; p; p = p->next)
            cst_free((void *)p->name);
        delete_features(c->attrs);
        strlist_free(c->voices);
        memset(c, 0, sizeof(*c));
        cst_free(c);
    }
    return 1;
}

/* 0x439ac arm */
tts_cert_data *cert_copy(const tts_cert_data *c)
{
    cst_featvalpair *p;
    tts_cert_data *n;

    if (c == NULL)
        return NULL;
    n = cst_safe_calloc(1, sizeof(tts_cert_data));
    n->attrs = new_features();
    for (p = c->attrs->head; p; p = p->next)
        feat_set_string(n->attrs, (const char *)cst_strdup((const cst_string *)p->name),
                        val_string(p->val));
    n->expires = c->expires;
    n->issued = c->issued;
    n->max_speed = c->max_speed;
    n->license_id = c->license_id;
    n->id = c->id;
    n->voices = strlist_copy(c->voices);
    n->ssml = c->ssml;
    n->key = c->key;
    return n;
}

/* 0x43a6c arm: 0 if valid, 1 if expired, -1 if the time is unknown. */
int cert_status(const tts_cert_data *c)
{
    long long now;

    if (c->expires == -1)
        return 0;
    now = time(NULL);
    if (now == -1) {
        tts_set_error_errno(errno, "time");
        return -1;
    }
    return c->expires < now;
}

/* whether every language code (separated by white space) is allowed as "all:<code>" */
static inline int cert_allows_langs(const tts_cert_data *c, const char *lang_codes)
{
    char buf[32];
    size_t pos, len;
    int count = 0;

    for (pos = 0; pos < strlen(lang_codes); ) {
        len = strcspn(lang_codes + pos, " \t\n\v\f\r\x85\xa0");
        if (len == 0) {
            pos++;
            continue;
        }
        if (len > 26)
            continue;
        memcpy(buf, "aLl:", 4);
        memcpy(buf + 4, lang_codes + pos, len);
        buf[4 + len] = '\0';
        if (!strlist_contains_nocase(c->voices, buf))
            return 0;
        pos += len + 1;
        count++;
    }
    return count > 0;
}

/* 0x43ae4 arm: whether the certificate allows the voice lector speaking one of lang_codes
 * (custom voices need "all:custom" or "*:custom"). */
int cert_allows_voice(const tts_cert_data *c, const char *lector, const char *lang_codes, int custom)
{
    if (c->voices == NULL)
        return 0;
    if (strlist_contains_nocase(c->voices, lector))
        return 1;
    if (custom)
        return strlist_contains_nocase(c->voices, "all:custom")
            || strlist_contains_nocase(c->voices, "*:custom");
    if (strlist_contains(c->voices, "*") || strlist_contains_nocase(c->voices, "all"))
        return 1;
    return cert_allows_langs(c, lang_codes);
}
