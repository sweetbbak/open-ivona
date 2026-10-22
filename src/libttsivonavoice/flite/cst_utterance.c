/* Imported from CMU Flite 1.4 src/hrg/cst_utterance.c by `dc flite import`; functions with an address comment
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
/*               Date:  December 1999                                    */
/*************************************************************************/
/*                                                                       */
/*    Utterances                                                         */
/*                                                                       */
/*************************************************************************/
#include "cst_alloc.h"
#include "cst_utterance.h"

/* utterance functions are the modules that do the meat in synthesis */
CST_VAL_REGISTER_FUNCPTR(uttfunc,cst_uttfunc)

/* 0x4e368: IVONA: utterances carry two engine pointers */
cst_utterance *new_utterance(void *engine_a, void *engine_b)
{
    cst_utterance *u;

    u = cst_alloc(cst_utterance, 1);
    u->ctx = NULL;
    u->features = new_features_local(u->ctx);
    u->ffunctions = new_features_local(u->ctx);
    u->relations = new_features_local(u->ctx);
    u->engine_b = engine_b;
    u->engine_a = engine_a;
    return u;
}

/* 0x4e3bc */
void delete_utterance(cst_utterance *u)
{
    cst_featvalpair *fp;

    if (u) {
        delete_features(u->features);
        delete_features(u->ffunctions);
        /* delete relations */
        for (fp = u->relations->head; fp; fp = fp->next)
            delete_relation(val_relation(fp->val));
        delete_features(u->relations);
        cst_free(u);
    }
}

/* 0x4e414 */
int utt_relation_delete(cst_utterance *u, int id)
{
    const char *name = cst_relation_id_name(id);
    cst_relation *r;

    if (feat_present(u->relations, name)) {
        r = val_relation(feat_val(u->relations, name));
        delete_relation(r);
    }
    return feat_remove(u->relations, name);
}

/* 0x4e460 */
cst_relation *utt_relation_create(cst_utterance *u, int id)
{
    cst_relation *r;

    utt_relation_delete(u, id);
    r = new_relation(id, u);
    feat_set(u->relations, cst_relation_id_name(id), relation_val(r));
    return r;
}

/* 0x4e4b0 */
int utt_relation_present(cst_utterance *u, int id)
{
    return feat_present(u->relations, cst_relation_id_name(id));
}

/* 0x4e4d0 */
cst_relation *utt_relation(cst_utterance *u, int id)
{
    const cst_val *v = feat_val(u->relations, cst_relation_id_name(id));

    if (v != NULL)
        return val_relation(v);
    cst_dbgmsg("Relation: %s not present in utterance\n", cst_relation_id_name(id));
    cst_error();
}
