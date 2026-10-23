/* Imported from CMU Flite 1.4 src/synth/cst_phoneset.c by `dc flite import`; functions with an address comment
 * are present in the engine (unmodified ones match as-is, others carry IVONA's changes). */
/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 1999                             */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  December 2000                                    */
/*************************************************************************/
/*                                                                       */
/*    Voice definition                                                   */
/*                                                                       */
/*************************************************************************/
#include "cst_val.h"
#include "cst_utterance.h"
#include "cst_item.h"
#include "cst_phoneset.h"
#include "cst_val_const.h"

/* language data (partly known) */
typedef struct lang_ps_data {
    int unk[5];
    cst_features *features;     /* +0x14 */
} lang_ps_data;

void *item_lang_data(const cst_item *i);

CST_VAL_REGISTER_TYPE_NODEL(phoneset,cst_phoneset)

/* 0x70da8 */
cst_phoneset *new_phoneset()
{
    /* These aren't going to be supported dynamically */
    cst_phoneset *v = cst_safe_calloc(1, sizeof(struct cst_phoneset_struct));  /* IVONA */

    return v;
}

/* 0x70db4 */
void delete_phoneset(cst_phoneset *v)
{
    if (v)
    {
	cst_free(v);
    }
}

/* 0x70dc0 */
int phone_id(const cst_phoneset *ps,const char* phonename)
{
    int i, lo, hi, c;

    /* IVONA: -1 when missing; binary search over sorted phone names */
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
    for (i=0; i< ps->num_phones; i++)
        if (cst_streq(ps->phonenames[i],phonename))
            return i;
    return -1;
}

/* 0x70e6c */
int phone_feat_id(const cst_phoneset *ps,const char* featname)
{
    int i;

    for (i=0; ps->featnames[i]; i++)
	if (cst_streq(ps->featnames[i],featname))
	    return i;

    return -1;  /* IVONA */
}

/* 0x70ebc */
const cst_val *phone_feature(const cst_phoneset *ps,
			     const char* phonename,
			     const char *featname)
{
    int p, f;

    /* IVONA: "0" for unknown phones or features */
    if ((p = phone_id(ps,phonename)) == -1 || (f = phone_feat_id(ps,featname)) == -1)
        return &val_string_0;
    return ps->featvals[ps->fvtable[p][f]];
}

/* 0x70f14 */
const char *phone_feature_string(const cst_phoneset *ps,
				 const char* phonename,
				 const char *featname)
{
    return val_string(phone_feature(ps,phonename,featname));
}


/* 0x70f24 */
const cst_phoneset *item_phoneset(const cst_item *p)
{
    const lang_ps_data *ld;

    /* IVONA: the item's own phoneset, else the language's, else the utterance's */
    if (item_feat_present(p,"phoneset"))
        return val_phoneset(item_feat(p,"phoneset"));
    if ((ld = item_lang_data(p)) != NULL)
        return val_phoneset(feat_val(ld->features,"phoneset"));
    return val_phoneset(feat_val(item_utt(p)->features,"phoneset"));
}


