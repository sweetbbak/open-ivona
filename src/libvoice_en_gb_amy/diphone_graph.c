/*
 * Amy's diphone graph (0x4b790..): the unit selection problem for a run of target units. One
 * segment per pair of neighbouring targets, each with the join cost of its phone class, and the
 * places where the search may fix its path ("traverse breaks", every tb_min_distance phones).
 */
#include <string.h>
#include "cst_features.h"
#include "cst_utterance.h"
#include "cst_val.h"
#include "tts_voxdb.h"

typedef struct join_cost_func join_cost_func;
join_cost_func *val_join_cost_func(const cst_val *v);
/* declared with the unit count too, which it ignores (the caller passes it) */
unit_selector *unit_selector_new(cst_utterance *utt, int n);
select_segment *unit_selector_add_diphone(unit_selector *s, const vox_unit *a, const vox_unit *b);
unsigned int unit_model_cost(const vox_unit *a, const vox_unit *b, const vox_unit *ta,
                             const vox_unit *tb);

/* the phone class strings of a target unit (vox_unit.p88..p94) */
#define UNIT_VC(u) ((const char *)(u)->p88)     /* "+" for a vowel */
#define UNIT_P8C(u) ((const char *)(u)->p8c)
#define UNIT_CTYPE(u) ((const char *)(u)->p90)  /* "r" trill, "g" glide, ... */
#define UNIT_P94(u) ((const char *)(u)->p94)
#define UNIT_PHONE(u) ((u)->phones[2]->name)

/* 0x4b790 arm: diphone_graph_func: the selector for the n target units */
unit_selector *diphone_graph_build(cst_utterance *utt, const vox_unit *units, int n)
{
    select_join_cost_fn trill = (select_join_cost_fn)val_join_cost_func(
        feat_val(utt->features, "conc_cost_trill"));
    select_join_cost_fn vowel = (select_join_cost_fn)val_join_cost_func(
        feat_val(utt->features, "conc_cost_vowel"));
    select_join_cost_fn voiced = (select_join_cost_fn)val_join_cost_func(
        feat_val(utt->features, "conc_cost_voiced"));
    select_join_cost_fn glide = (select_join_cost_fn)val_join_cost_func(
        feat_val(utt->features, "conc_cost_glide"));
    select_join_cost_fn unvoiced = (select_join_cost_fn)val_join_cost_func(
        feat_val(utt->features, "conc_cost_unvoiced"));
    int tb = get_param_int(utt->features, "tb_min_distance", -1);
    unit_selector *s = unit_selector_new(utt, n);
    select_segment *seg;
    const vox_unit *b;
    const char *ctype;
    int i, last = 0, dist;

    s->model_cost = unit_model_cost;
    for (i = 0; n > 1; n--, i++, units++) {
        b = units + 1;
        if ((seg = unit_selector_add_diphone(s, units, b)) == NULL)
            continue;
        ctype = UNIT_CTYPE(b);
        if (strcmp(ctype, "r") == 0)
            seg->join_cost = trill;
        else if (strcmp(UNIT_VC(b), "+") == 0)
            seg->join_cost = vowel;
        else if (strcmp(ctype, "g") == 0 || strcmp(UNIT_PHONE(b), "l") == 0)
            seg->join_cost = glide;
        else if (strcmp(UNIT_P8C(b), "-") == 0)
            seg->join_cost = strcmp(UNIT_P94(b), "g") == 0 ? voiced : unvoiced;
        else
            seg->join_cost = voiced;
        /* a traverse break at a consonant, at least tb phones after the last one: preferably
         * not inside a cluster (r p l g) or, within 2 tb, before 1.5 tb */
        if (tb > 0 && (dist = i - last) >= tb && tb <= n && strcmp(UNIT_VC(b), "-") == 0) {
            if (dist <= tb * 2) {
                if (strcmp(UNIT_P8C(b), "-") != 0 && !(dist > tb * 1.5))
                    continue;
                if (strchr("rplg", *ctype))
                    continue;
            }
            seg->boundary = 1;
            last = i;
        }
    }
    return s;
}
