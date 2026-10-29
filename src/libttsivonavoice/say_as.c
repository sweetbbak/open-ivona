/*
 * SSML say-as interpretations (0x2cda8..0x2d71c): the interpret-as/format/detail table and the
 * date and time forms say-as text is normalized to.
 */
#include <string.h>
#include <strings.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "ssml_doc.h"
#include "ssml_say_as.h"

const char *dom_get_attr(const dom_elem *e, const char *name);
int cst_parse_uint_n(const char *s, size_t n, unsigned long *out);
const char *cst_skip_space(const char *s);

static const say_as_info say_as_none = { NULL, NULL, 0 };
static const char date_fields[] = "ymd";

/* a say-as format: the interpretation for one format/detail value (NULL: the default) */
typedef struct say_as_format {
    const char *format;
    say_as_info info;
} say_as_format;

typedef struct say_as_entry {
    const char *name;
    const say_as_format *formats;
} say_as_entry;

/* (defined in reverse: GCC lays them out in reverse order of definition) */
static const say_as_format say_as_flight[] = { { NULL, { "flight-code", NULL, 0 } } };
static const say_as_format say_as_interjection[] = { { NULL, { "interjection", NULL, 1 } } };
static const say_as_format say_as_social[] = { { NULL, { "social-speech", NULL, 1 } } };
static const say_as_format say_as_expletive[] = { { NULL, { "expletive", NULL, 1 } } };
static const say_as_format say_as_address[] = {
    { "postal", { "characters", NULL, 0 } },
    { "us-state", { "address", "us-state", 0 } },
    { NULL, { "address", NULL, 0 } },
};
static const say_as_format say_as_duration[] = {
    { "hms", { "duration", "hms", 0 } },
    { "hm", { "duration", "hm", 0 } },
    { "ms", { "duration", "ms", 0 } },
    { "h", { "duration", "h", 0 } },
    { "m", { "duration", "m", 0 } },
    { "s", { "duration", "s", 0 } },
    { NULL, { "duration", NULL, 0 } },
};
static const say_as_format say_as_time[] = {
    { "hms24", { "time", "hms24", 0 } },
    { "hms12", { "time", "hms12", 0 } },
    { "hms", { "time", "hms24", 0 } },
    { "hm", { "time", "hms24", 0 } },
    { NULL, { "time", NULL, 0 } },
};
static const say_as_format say_as_date[] = {
    { "mdy", { "date", "mdy", 0 } },
    { "dmy", { "date", "dmy", 0 } },
    { "ymd", { "date", "ymd", 0 } },
    { "md", { "date", "md", 0 } },
    { "dm", { "date", "dm", 0 } },
    { "ym", { "date", "ym", 0 } },
    { "my", { "date", "my", 0 } },
    { "d", { "date", "d", 0 } },
    { "m", { "date", "m", 0 } },
    { "y", { "date", "y", 0 } },
    { "year", { "date", "y", 0 } },
    { NULL, { "date", NULL, 0 } },
};
static const say_as_format say_as_radio[] = { { NULL, { "radiostation", NULL, 0 } } };
static const say_as_format say_as_telephone[] = { { NULL, { "telephone", NULL, 0 } } };
static const say_as_format say_as_unit[] = { { NULL, { "unit", NULL, 0 } } };
static const say_as_format say_as_fraction[] = { { NULL, { "fraction", NULL, 0 } } };
static const say_as_format say_as_digits[] = { { NULL, { "digits", NULL, 0 } } };
static const say_as_format say_as_cardinal[] = { { NULL, { "cardinal", NULL, 0 } } };
static const say_as_format say_as_ordinal[] = { { NULL, { "ordinal", NULL, 0 } } };
static const say_as_format say_as_number[] = {
    { "cardinal", { "cardinal", NULL, 0 } },
    { "cardinals", { "cardinal", NULL, 0 } },
    { "ordinal", { "ordinal", NULL, 0 } },
    { "ordinals", { "ordinal", NULL, 0 } },
    { "telephone", { "telephone", NULL, 0 } },
    { "phone", { "telephone", NULL, 0 } },
    { "digit", { "digits", NULL, 0 } },
    { "digits", { "digits", NULL, 0 } },
    { "fraction", { "fraction", NULL, 0 } },
    { "fractions", { "fraction", NULL, 0 } },
    { NULL, { "cardinal", NULL, 0 } },
};
static const say_as_format say_as_serial[] = { { NULL, { "serial", NULL, 0 } } };
static const say_as_format say_as_characters[] = {
    { "glyphs", { "characters", "glyphs", 0 } },
    { NULL, { "characters", NULL, 0 } },
};
static const say_as_format say_as_plain[] = { { NULL, { "", NULL, 0 } } };

/* the interpret-as values */
static const say_as_entry say_as_table[] = {
    { "characters", say_as_characters },
    { "characters", say_as_characters },
    { "spell-out", say_as_characters },
    { "spell", say_as_characters },
    { "acronym", say_as_characters },
    { "letter", say_as_characters },
    { "letters", say_as_characters },
    { "glyph", say_as_characters },
    { "glyphs", say_as_characters },
    { "serial", say_as_serial },
    { "address", say_as_address },
    { "currency", say_as_plain },
    { "name", say_as_plain },
    { "net", say_as_plain },
    { "web", say_as_plain },
    { "e-mail", say_as_plain },
    { "number", say_as_number },
    { "numbers", say_as_number },
    { "ordinal", say_as_ordinal },
    { "ordinals", say_as_ordinal },
    { "cardinal", say_as_cardinal },
    { "cardinals", say_as_cardinal },
    { "digit", say_as_digits },
    { "digits", say_as_digits },
    { "fraction", say_as_fraction },
    { "fractions", say_as_fraction },
    { "radiostation", say_as_radio },
    { "radio", say_as_radio },
    { "unit", say_as_unit },
    { "units", say_as_unit },
    { "measure", say_as_unit },
    { "phone", say_as_telephone },
    { "telephone", say_as_telephone },
    { "date", say_as_date },
    { "time", say_as_time },
    { "duration", say_as_duration },
    { "expletive", say_as_expletive },
    { "social-speech", say_as_social },
    { "literal", say_as_plain },
    { "interjection", say_as_interjection },
    { "flight-code", say_as_flight },
    { NULL, NULL },
};

/* namespace prefixes of interpret-as values */
static const char *const say_as_ns[] = {
    "http://www.w3.org/2001/10/synthesis^",
    "https://amazon.com/ssml/2017-01-01/^",
    "http://www.w3.org/2001/vxml^",
    NULL,
};

#define SSML_WS " \t\n\v\f\r\x85\xa0"

/* the interpretation of interpret-as name with format fmt (NULL: the default), NULL if
 * name is unknown */
static inline const say_as_info *say_as_find(const char *name, const char *fmt)
{
    const say_as_entry *t;
    const say_as_format *f;

    for (t = say_as_table; t->name; t++) {
        if (strcasecmp(t->name, name) != 0)
            continue;
        f = t->formats;
        if (f->format == NULL)
            return &f->info;
        for (; f->format; f++)
            if (fmt && strcasecmp(f->format, fmt) == 0)
                break;
        return &f->info;
    }
    return NULL;
}

/* 0x2cda8 arm: an interpret-as value carrying its format ("date:mdy", "date;mdy",
 * "date_mdy") */
static const say_as_info *say_as_split(const char *s)
{
    const char *seps, *p, *e, *f;
    char *name, *fmt;
    const say_as_info *r;

    for (seps = ":;_"; *seps; seps++) {
        p = strchr(s, *seps);
        if (p == NULL)
            continue;
        for (e = p; e != s && strchr(SSML_WS, e[-1]); e--)
            ;
        for (f = p + 1; *f && strchr(SSML_WS, *f); f++)
            ;
        if (e == s)
            continue;
        name = cst_strndup(s, e - s);
        fmt = *f ? cst_strdup(f) : NULL;
        r = name ? say_as_find(name, fmt) : NULL;
        cst_free(name);
        cst_free(fmt);
        if (r)
            return r;
    }
    return NULL;
}

/* an Amazon extension namespace for a say-as interpretation */
struct say_as_ext {
    const char *ns;
    const char *unk_4;
};

static say_as_ext say_as_ext_speechcon = { "https://amazon.com/ssml/2017-01-01/^speechcon", NULL };
static say_as_ext say_as_ext_social = { "https://amazon.com/ssml/2017-01-01/^socialSpeech", NULL };
static say_as_ext say_as_ext_none;

static struct {
    const char *name;
    say_as_ext *ext;
} say_as_exts[] = {
    { "social-speech", &say_as_ext_social },
    { "interjection", &say_as_ext_speechcon },
    { NULL, &say_as_ext_none },
};

/* 0x2cf84 arm: the extension of a say-as interpretation (an empty one if none) */
say_as_ext *say_as_ext_find(const say_as_info *sa)
{
    typeof(say_as_exts[0]) *p;

    if (sa == NULL || sa->name == NULL)
        return &say_as_ext_none;
    for (p = say_as_exts; p; p++)
        if (p->name == NULL || strcmp(p->name, sa->name) == 0)
            break;
    return p->ext;
}

/* 0x2cff4 arm: the interpretation of a <say-as> (or amazon:literal) element from its
 * interpret-as, format, detail and type attributes */
const say_as_info *say_as_lookup(dom_elem *e)
{
    const char *ia = dom_get_attr(e, "interpret-as");
    const char *fmt = dom_get_attr(e, "format");
    const char *type = dom_get_attr(e, "type");
    const char *detail = dom_get_attr(e, "detail");
    const char *const *ns;
    const say_as_info *r;

    if (e->n.type == AMAZON_LITERAL)
        return say_as_find("literal", NULL);
    if (ia) {
        for (ns = say_as_ns; *ns; ns++) {
            if (cst_startswith(ia, *ns)) {
                ia += strlen(*ns);
                break;
            }
        }
        if (fmt && ia && (r = say_as_find(ia, fmt)))
            return r;
        if (detail && ia && (r = say_as_find(ia, detail)))
            return r;
        if (ia) {
            if ((r = say_as_find(ia, NULL)))
                return r;
            if ((r = say_as_split(ia)))
                return r;
        }
    }
    if (type == NULL)
        return &say_as_none;
    if ((r = say_as_find(type, NULL)))
        return r;
    if ((r = say_as_split(type)))
        return r;
    return &say_as_none;
}

/* frees the first n strings of p */
static inline void strs_free(char **p, int n)
{
    int i;

    for (i = 0; i < n; i++) {
        cst_free(p[i]);
        p[i] = NULL;
    }
}

/* 0x2d320 arm: splits a "yyyymmdd" date ("????" or "??" for unknown fields) into year,
 * month and day strings (NULL if unknown); 0 if it is not one. */
int ssml_date_parse(const char *s, char **ymd)
{
    int unknown, n;

    s = cst_skip_space(s);
    if (strlen(s) < 8)
        return 0;
    if (strncmp(s, "????", 4) == 0) {
        unknown = 1;
        ymd[0] = NULL;
    } else {
        if (!cst_parse_uint_n(s, 4, NULL))
            return 0;
        ymd[0] = cst_substr(s, 0, 4);
        unknown = 0;
    }
    if (strncmp(s + 4, "????", 2) == 0)
        ymd[1] = NULL;
    else {
        if (!cst_parse_uint_n(s + 4, 2, NULL)) {
            n = 1;
            goto partial;
        }
        ymd[1] = cst_substr(s + 4, 0, 2);
        unknown = 0;
    }
    if (strncmp(s + 6, "????", 2) == 0) {
        ymd[2] = NULL;
        if (*cst_skip_space(s + 8) == 0 && !unknown)
            return 1;
    } else {
        if (!cst_parse_uint_n(s + 6, 2, NULL)) {
            n = 2;
            goto partial;
        }
        ymd[2] = cst_substr(s + 6, 0, 2);
        if (*cst_skip_space(s + 8) == 0)
            return 1;
    }
    strs_free(ymd, 3);
    return 0;
partial:
    strs_free(ymd, n);
    return 0;
}

/* 0x2d4d8 arm: joins the known date fields with '-' into out; order gets their letters
 * ("ymd", "md", ...). */
void ssml_date_join(const char *y, const char *m, const char *d, char *out, char *order)
{
    const char *f[3];
    int i, len, pos = 0, n = 0;

    f[0] = y;
    f[1] = m;
    f[2] = d;
    for (i = 0; i < 3; i++) {
        if (f[i] == NULL)
            continue;
        len = strlen(f[i]);
        if (pos != 0) {
            out[pos] = '-';
            pos++;
        }
        memcpy(out + pos, f[i], len);
        order[n++] = date_fields[i];
        pos += len;
    }
    out[pos] = 0;
    order[n] = 0;
}

/* 0x2d574 arm: expands a compact time of len characters ("730p", "0730a", "1415h": digits
 * and a suffix a, p, h or ?) into out ("7:30pm"); 0 if it is not one. */
int ssml_time_expand(const char *s, size_t len, char *out)
{
    size_t i, n = len - 1;
    int k = 0;
    unsigned char c;

    if (len - 2 > 3)
        return 0;
    for (i = 0; i < n; i++) {
        c = s[i];
        if (strchr("0123456789", c) == NULL)
            return 0;
        out[k++] = c;
        if (i == len - 4)
            out[k++] = ':';
    }
    switch (s[i]) {
    case 'p':
        out[k++] = 'p';
        out[k++] = 'm';
        break;
    case 'a':
        out[k++] = 'a';
        out[k++] = 'm';
        break;
    case 'h':
    case '?':
        break;
    default:
        return 0;
    }
    out[k] = 0;
    return 1;
}

