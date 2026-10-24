/*
 * Language tag matching (0x2252c..0x2285c): "en-GB-x" style tags, compared part by part.
 */
#include <string.h>
#include <strings.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "ivona_util.h"

/* NUL-terminates the parts of a tag (at most 3; ':' ends it); their number */
static inline unsigned int lang_tag_split(char *s)
{
    unsigned int n = 1;
    char *p;

    for (p = s; *p; p++) {
        if (*p == ':') {
            *p = '\0';
            break;
        }
        if (n <= 2 && *p == '-') {
            *p = '\0';
            n++;
        }
    }
    return n;
}

/* 0x2252c arm: how well tag a matches pattern b: 4 exactly (or b empty), 2 by language and
 * region, 1 by language only, 0 not at all. UK counts as GB. */
int lang_tag_match(const char *a, const char *b)
{
    char *x, *y;
    const char *pa, *pb;
    size_t la, lb;
    unsigned int na, nb;
    int r;

    if (b == NULL || b[0] == '\0' || strcasecmp(a, b) == 0)
        return 4;
    x = cst_strdup(a);
    y = cst_strdup(b);
    na = lang_tag_split(x);
    nb = lang_tag_split(y);
    if (strcasecmp(x, y) != 0)
        r = 0;
    else {
        la = strlen(x);
        lb = strlen(y);
        if (nb > 1 && na > 1) {
            pa = x + la + 1;
            pb = y + lb + 1;
            if (strcasecmp(strcasecmp(pa, "UK") == 0 ? "GB" : pa,
                           strcasecmp(pb, "UK") == 0 ? "GB" : pb) != 0)
                r = 1;
            else if (nb > 2 && na > 2)
                r = strcasecmp(pa + strlen(pa) + 1, pb + strlen(pb) + 1) == 0 ? 4 : 2;
            else
                r = na == nb ? 4 : 2;
        } else
            r = na == nb ? 4 : 1;
    }
    cst_free(x);
    cst_free(y);
    return r;
}

/* 0x227bc arm: the best match of a against a whitespace separated list of patterns (exact
 * matches count 3, unless all are exact) */
int lang_tags_match(const char *a, const char *list)
{
    strlist *l;
    int best, all, m;
    char **p;

    if (list == NULL || list[0] == '\0')
        return 4;
    l = strlist_split_ws(list);
    best = 0;
    all = 1;
    for (p = l->items; p != l->items + l->count; p++) {
        m = lang_tag_match(a, *p);
        if (m == 4)
            m = 3;
        else
            all = 0;
        if (best < m)
            best = m;
    }
    strlist_free(l);
    if (all)
        return 4;
    return best;
}
