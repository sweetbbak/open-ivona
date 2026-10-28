#include <string.h>
#include "cst_val.h"
#include "cst_val_defs.h"
#include "cst_val_const.h"
#include "cst_utterance.h"
#include "cst_item.h"
#include "cst_phoneset.h"

typedef struct lang_ps_data {
    int unk[5];
    cst_features *features;     /* +0x14 */
} lang_ps_data;

void *item_lang_data(const cst_item *i);

/* 0x58450: arm */
cst_phoneset *val_phoneset(const cst_val *v)
{
    if (v && CST_VAL_TYPE(v) == cst_val_type_phoneset)
        return (cst_phoneset *)CST_VAL_VOID(v);
    cst_errmsg("VAL: tried to access %s in %d type val\n", "phoneset",
               (v ? CST_VAL_TYPE(v) : -1));
    cst_error();
}

/* 0x584a8: arm */
cst_val *phoneset_val(const cst_phoneset *v)
{
    return val_new_typed(cst_val_type_phoneset, (void *)v);
}

/* 0x584c0: arm */
cst_phoneset *new_phoneset(void)
{
    cst_phoneset *v = cst_safe_calloc(1, sizeof(struct cst_phoneset_struct));
    return v;
}

/* 0x584cc: arm */
void delete_phoneset(cst_phoneset *v)
{
    if (v)
    {
        cst_free(v);
    }
}

/* 0x584d8: arm */
int phone_id(const cst_phoneset *ps, const char *phonename)
{
    int i, lo, hi, c;

    if (ps->sorted) {
        lo = 0;
        hi = ps->num_phones;
        while (hi > lo) {
            i = (lo + hi) / 2;
            c = strcmp(ps->phonenames[i], phonename);
            if (c == 0)
                return i;
            if (c > 0)
                hi = i;
            else
                lo = i + 1;
        }
        return -1;
    }
    for (i = 0; i < ps->num_phones; i++)
        if (cst_streq(ps->phonenames[i], phonename))
            return i;
    return -1;
}

/* 0x58584: arm */
int phone_feat_id(const cst_phoneset *ps, const char *featname)
{
    int i;

    for (i = 0; ps->featnames[i]; i++)
        if (cst_streq(ps->featnames[i], featname))
            return i;

    return -1;
}

/* 0x585d4: arm */
const cst_val *phone_feature(const cst_phoneset *ps,
                             const char *phonename,
                             const char *featname)
{
    int p, f;

    if ((p = phone_id(ps, phonename)) == -1 || (f = phone_feat_id(ps, featname)) == -1)
        return &val_string_0;
    return ps->featvals[ps->fvtable[p][f]];
}

/* 0x5862c: arm */
const char *phone_feature_string(const cst_phoneset *ps,
                                 const char *phonename,
                                 const char *featname)
{
    return val_string(phone_feature(ps, phonename, featname));
}

/* 0x5863c: arm */
const cst_phoneset *item_phoneset(const cst_item *p)
{
    const lang_ps_data *ld;

    if (item_feat_present(p, "phoneset"))
        return val_phoneset(item_feat(p, "phoneset"));
    if ((ld = item_lang_data(p)) != NULL)
        return val_phoneset(feat_val(ld->features, "phoneset"));
    return val_phoneset(feat_val(item_utt(p)->features, "phoneset"));
}
