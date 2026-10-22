/*
 * Resource fetcher (0x39798..0x3a188): loads documents referenced by URI (e.g. PLS lexicons), either
 * through an application-supplied fetch callback or, deprecated, through the statically linked
 * libcurl. Relative URIs are resolved against a base URI. Diagnostics accumulate in a tts_msg list.
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "cst_features.h"
#include "ivona_util.h"
#include "tts_metrics.h"

/* libcurl (statically linked) */
typedef void CURL;
#define CURLOPT_WRITEDATA       10001
#define CURLOPT_URL             10002
#define CURLOPT_TIMEOUT         13
#define CURLOPT_REFERER         10016
#define CURLOPT_USERAGENT       10018
#define CURLOPT_WRITEFUNCTION   20011
#define CURLOPT_FAILONERROR     45
#define CURLOPT_FOLLOWLOCATION  52
#define CURLOPT_MAXREDIRS       68
#define CURLINFO_CONTENT_TYPE   0x100012
CURL *curl_easy_init(void);
int curl_easy_setopt(CURL *curl, int option, ...);
int curl_easy_perform(CURL *curl);
void curl_easy_cleanup(CURL *curl);
int curl_easy_getinfo(CURL *curl, int info, ...);
const char *curl_easy_strerror(int code);

#define FETCH_FAILED    0
#define FETCH_DEFAULT   2   /* callback asks for the built-in (curl) mechanism */
#define FETCH_OK        3

#define FETCH_MAX_SIZE  0x8000000

typedef struct fetch_result {
    void *data;
    size_t size;
    char *content_type;
    void (*free_fn)(struct fetch_result *r);
    void *unk_10;
} fetch_result;

typedef struct fetch_request {
    void *user;
    void *arg1;
    void *arg2;
    float timeout;
    const char *uri;
    const char *base;
    const char *url;            /* resolved absolute URI */
} fetch_request;

typedef int (*fetch_fn)(fetch_request *req, fetch_result *res);

typedef struct fetcher {
    CURL *curl;
    fetch_fn fetch;             /* +0x04 */
    fetch_result result;        /* +0x08 */
    void *user;                 /* +0x1c */
    char *referer;              /* +0x20 */
    char *base;                 /* +0x24 */
    tts_msg *msgs;              /* +0x28 */
} fetcher;

/* 0x39798 arm: free_fn for results produced by curl. */
static void fetch_result_free(fetch_result *r)
{
    cst_free(r->data);
    r->data = NULL;
    r->size = 0;
    cst_free(r->content_type);
    r->content_type = NULL;
}

/* 0x397c4 arm: CURLOPT_WRITEFUNCTION; appends to a strbuf, aborting past 128 MiB. */
static size_t fetch_write(void *ptr, size_t size, size_t nmemb, strbuf *buf)
{
    size_t n = size * nmemb;
    if (n + buf->len > FETCH_MAX_SIZE)
        return nmemb - 1;
    strbuf_append(buf, ptr, n);
    return nmemb;
}

/* 0x397fc arm */
fetcher *fetcher_new(void)
{
    return cst_safe_calloc(1, sizeof(fetcher));
}

/* 0x39808 arm */
void fetcher_delete(fetcher *f)
{
    if (f == NULL)
        return;
    if (f->curl)
        curl_easy_cleanup(f->curl);
    if (f->result.free_fn)
        f->result.free_fn(&f->result);
    memset(&f->result, 0, sizeof(f->result));
    cst_free(f->referer);
    cst_free(f->base);
    tts_msg_free(f->msgs);
    cst_free(f);
}

/* 0x39870 arm: releases everything and zeroes the fetcher. */
void fetcher_reset(fetcher *f)
{
    if (f->curl)
        curl_easy_cleanup(f->curl);
    if (f->result.free_fn)
        f->result.free_fn(&f->result);
    memset(&f->result, 0, sizeof(f->result));
    cst_free(f->referer);
    cst_free(f->base);
    tts_msg_free(f->msgs);
    memset(f, 0, sizeof(*f));
}

/* 0x398dc arm */
void fetcher_set_referer(fetcher *f, const char *referer)
{
    cst_free(f->referer);
    f->referer = cst_strdup(referer);
    if (f->curl)
        curl_easy_setopt(f->curl, CURLOPT_REFERER, referer);
}

/* 0x39920 arm */
void fetcher_set_base(fetcher *f, const char *base)
{
    cst_free(f->base);
    f->base = cst_strdup(base);
}

/* 0x39944 arm */
void fetcher_set_callback(fetcher *f, fetch_fn fetch, void *user)
{
    f->fetch = fetch;
    f->user = user;
}

/* 0x39950 arm: the last fetched document, or NULL. */
fetch_result *fetcher_result(fetcher *f)
{
    return f->result.data ? &f->result : NULL;
}

/* 0x39964 arm */
tts_msg *fetcher_messages(fetcher *f)
{
    return f->msgs;
}

/* 0x3996c arm: resolves uri against base. Absolute URIs are copied; "/x" replaces the path,
 * "?q" the query, anything else the last path segment ("./" and "../" prefixes are honoured).
 * Spaces are escaped as "%20" before the query and "+" inside it.
 */
char *uri_resolve(const char *base, const char *uri)
{
    char scheme[16];
    char c;
    char *b, *path, *s, *q, *out, *d;
    const char *p;
    int up, n, in_path;
    size_t len;

    if (sscanf(uri, "%15[^?&/:]://%c", scheme, &c) == 2)
        return cst_strdup(uri);
    if (base == NULL || *base == '\0')
        return NULL;
    b = cst_strdup(base);
    if (b == NULL)
        return NULL;
    s = strstr(b, "//");
    path = s ? s + 2 : b;

    if (*uri == '/') {
        s = strchr(path, '/');
        if (s) {
            q = strchr(path, '?');
            if (q && s > q)
                s = q;
            *s = '\0';
        } else {
            q = strchr(path, '?');
            if (q)
                *q = '\0';
        }
    } else {
        q = strchr(path, '?');
        if (q)
            *q = '\0';
        if (*uri != '?') {
            s = strrchr(path, '/');
            if (s)
                *s = '\0';
        }
        path = strchr(path, '/');
        path = path ? path + 1 : NULL;
        if (uri[0] == '.' && uri[1] == '/')
            uri += 2;
        for (up = 0; uri[0] == '.' && uri[1] == '.' && uri[2] == '/'; up++)
            uri += 3;
        if (path) {
            for (; up > 0; up--) {
                s = strrchr(path, '/');
                if (s == NULL) {
                    *path = '\0';
                    break;
                }
                *s = '\0';
            }
        }
    }

    n = 0;
    in_path = 1;
    for (p = uri; *p; p++) {
        if (*p == ' ' && in_path)
            n += 3;
        else {
            if (*p == '?')
                in_path = 0;
            n++;
        }
    }
    len = strlen(b);
    out = cst_safe_alloc(len + n + 2, 1);
    if (out == NULL) {
        cst_free(b);
        return NULL;
    }
    memcpy(out, b, len);
    if (*uri != '/' && (path == NULL || *path) && *uri != '?')
        out[len++] = '/';
    d = out + len;
    in_path = 1;
    for (p = uri; *p; p++) {
        if (*p == ' ') {
            if (in_path) {
                d[0] = '%';
                d[1] = '2';
                d[2] = '0';
                d += 3;
            } else {
                *d++ = '+';
            }
        } else {
            if (*p == '?')
                in_path = 0;
            *d++ = *p;
        }
    }
    *d = '\0';
    cst_free(b);
    return out;
}

/* built-in (curl) fetch of req->url into f->result; inlined into fetcher_fetch. */
static int fetch_curl(fetcher *f, fetch_request *req)
{
    strbuf *buf;
    char *content_type;
    int rc;

    if (req->url == NULL)
        return FETCH_DEFAULT;
    if (f->result.free_fn)
        f->result.free_fn(&f->result);
    memset(&f->result, 0, sizeof(f->result));
    TTS_MSG_ADD(f->msgs, 15,
                "The built-in fetching mechanism is deprecated. Please consult the IVONA SDK documentation.");
    if (f->curl == NULL) {
        f->curl = curl_easy_init();
        if (f->curl == NULL) {
            TTS_MSG_ADD(f->msgs, 10, "Cannot initialize CURL");
            return FETCH_FAILED;
        }
        curl_easy_setopt(f->curl, CURLOPT_WRITEFUNCTION, fetch_write);
        curl_easy_setopt(f->curl, CURLOPT_FAILONERROR, 1);
        curl_easy_setopt(f->curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(f->curl, CURLOPT_MAXREDIRS, 10);
        curl_easy_setopt(f->curl, CURLOPT_USERAGENT,
                         "ivona/1.6.101.3 (IVONA Text-To-Speech; +http://www.ivona.com)");
        if (f->referer)
            curl_easy_setopt(f->curl, CURLOPT_REFERER, f->referer);
    }
    buf = strbuf_new();
    curl_easy_setopt(f->curl, CURLOPT_URL, req->url);
    curl_easy_setopt(f->curl, CURLOPT_WRITEDATA, buf);
    curl_easy_setopt(f->curl, CURLOPT_TIMEOUT, (int)req->timeout + (req->timeout > (int)req->timeout));
    rc = curl_easy_perform(f->curl);
    if (rc) {
        TTS_MSG_ADD(f->msgs, 10, "%s", curl_easy_strerror(rc));
        strbuf_free(buf);
        return FETCH_FAILED;
    }
    /* sic: the content type is only kept when curl_easy_getinfo fails */
    if (curl_easy_getinfo(f->curl, CURLINFO_CONTENT_TYPE, &content_type))
        f->result.content_type = cst_strdup(content_type);
    f->result.size = buf->len;
    f->result.data = buf->data;
    buf->data = NULL;
    buf->len = 0;
    buf->cap = 0;
    strbuf_free(buf);
    f->result.free_fn = fetch_result_free;
    return FETCH_OK;
}

/* 0x39d04 arm: fetches uri into f->result. Returns FETCH_OK, FETCH_FAILED, or FETCH_DEFAULT when
 * the URI could not be resolved. Time spent is added to the "fetch_time" metric. */
int fetcher_fetch(fetcher *f, const char *uri, float timeout, void *arg1, void *arg2,
                  cst_features *metrics)
{
    fetch_request req;
    char *url;
    int t0, ret;

    url = uri_resolve(f->base, uri);
    req.user = f->user;
    req.arg1 = arg1;
    req.arg2 = arg2;
    req.timeout = timeout;
    req.uri = uri;
    req.base = f->base;
    req.url = url;
    if (url == NULL && (req.base == NULL || *req.base == '\0'))
        TTS_MSG_ADD(f->msgs, 11, "Relative URI given, but base URI is not set");

    t0 = ivona_time_usec();
    ret = FETCH_DEFAULT;
    if (f->fetch) {
        if (f->result.free_fn)
            f->result.free_fn(&f->result);
        memset(&f->result, 0, sizeof(f->result));
        ret = f->fetch(&req, &f->result);
        if (ret == FETCH_FAILED)
            TTS_MSG_ADD(f->msgs, 10, "External fetch function failed");
    }
    if (ret == FETCH_DEFAULT)
        ret = fetch_curl(f, &req);
    metrics_add_time(metrics, "fetch_time", ivona_time_usec() - t0);
    if (ret == FETCH_OK && f->result.size == 0) {
        TTS_MSG_ADD(f->msgs, 10, "Fetched empty file");
        ret = FETCH_FAILED;
    }
    cst_free(url);
    return ret;
}
