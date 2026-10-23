/* Imported from CMU Flite 1.4 src/utils/cst_val_user.c by `dc flite import`; functions with an address comment
 * are present in the engine (unmodified ones match as-is, others carry IVONA's changes).
 * IVONA replaced Flite's type list (0x56004..0x56128, table 0x271ab4): lpcres, clunit_db,
 * vit_cand, sts_list, breakfunc, cg_db, voice and audio_streaming_info are gone and types up to
 * 91 were added. The table has no terminating entry. */
/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2001                             */
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
/*               Date:  January 1999                                     */
/*************************************************************************/
/*                                                                       */
/*  User defined type registration                                       */
/*                                                                       */
/*  I'd like to make this file be automatically generated                */
/*                                                                       */
/*************************************************************************/
#include "cst_val.h"
#include "cst_alloc.h"
#include "cst_string.h"

CST_VAL_REG_TD_TYPE(utterance,void,7)
CST_VAL_REG_TD_TYPE(wave,void,9)
CST_VAL_REG_TD_TYPE(track,void,11)
CST_VAL_REG_TD_FUNCPTR(uttfunc,void *,15)
CST_VAL_REG_TD_FUNCPTR(ffunc,void *,17)
CST_VAL_REG_TD_TYPE_NODEL(relation,void,19)
CST_VAL_REG_TD_TYPE_NODEL(item,void,21)
CST_VAL_REG_TD_TYPE_NODEL(cart,void,23)
CST_VAL_REG_TD_TYPE_NODEL(phoneset,void,25)
CST_VAL_REG_TD_TYPE_NODEL(lexicon,void,27)
CST_VAL_REG_TD_TYPE_NODEL(dur_stats,void,29)
CST_VAL_REG_TD_TYPE(diphonedb,void,31)
CST_VAL_REG_TD_TYPE(wavesynth,void,35)
CST_VAL_REG_TD_TYPE(audiotag,void,37)
CST_VAL_REG_TD_TYPE_NODEL(userdata,void,41)
CST_VAL_REG_TD_FUNCPTR(itemfunc,void *,43)
CST_VAL_REG_TD_TYPE(features,void,45)
CST_VAL_REG_TD_FUNCPTR(phonefunc,void *,47)
CST_VAL_REG_TD_FUNCPTR(xsampafunc,void *,49)
CST_VAL_REG_TD_FUNCPTR(allwordposfunc,void *,51)
CST_VAL_REG_TD_TYPE_NODEL(zdur_cart,void,53)
CST_VAL_REG_TD_FUNCPTR(valfunc,void *,55)
CST_VAL_REG_TD_FUNCPTR(string_tokenizer_new_func,void *,57)
CST_VAL_REG_TD_FUNCPTR(join_cost_func,void *,59)
CST_VAL_REG_TD_TYPE_NODEL(blacklist,void,61)
CST_VAL_REG_TD_FUNCPTR(tokenizer_expand_str_func,void *,63)
CST_VAL_REG_TD_TYPE_NODEL(lang_dat_core_data,void,65)
CST_VAL_REG_TD_TYPE(metric,void,67)
CST_VAL_REG_TD_FUNCPTR(synth_module_callback,void *,69)
CST_VAL_REG_TD_TYPE(float_ptr,void,71)
CST_VAL_REG_TD_TYPE(phone_acoustic_data,void,73)
CST_VAL_REG_TD_TYPE(vector,void,75)
CST_VAL_REG_TD_TYPE(word_embeddings,void,77)
CST_VAL_REG_TD_FUNCPTR(ffunc2,void *,79)
CST_VAL_REG_TD_TYPE(dnn_models,void,81)
CST_VAL_REG_TD_TYPE_NODEL(acoustic_covariances,void,85)
CST_VAL_REG_TD_TYPE_NODEL(breaths_stats,void,87)
CST_VAL_REG_TD_TYPE(binary_key,void,89)
/* 0x56064 arm */
void val_delete_binary_key(void *v)
{
    if (v)
        cst_free(v);
}
CST_VAL_REG_TD_TYPE(dynamic_voxdbs,void,91)
CST_VAL_REGISTER_TYPE_NODEL(userdata,cst_userdata)
CST_VAL_REGISTER_TYPE_NODEL(binary_key,void)

const cst_val_def cst_val_defs[] = {
    /* These ones are never called */
    { "int", NULL },  /* 1 */
    { "float", NULL },  /* 3 */
    { "string", NULL },  /* 5 */
    /* These are indexed (type/2) at print and delete time */
    { "utterance", val_delete_utterance },  /* 7 */
    { "wave", val_delete_wave },  /* 9 */
    { "track", val_delete_track },  /* 11 */
    { "", NULL },  /* 13 (was lpcres) */
    { "uttfunc", val_delete_uttfunc },  /* 15 */
    { "ffunc", val_delete_ffunc },  /* 17 */
    { "relation", val_delete_relation },  /* 19 */
    { "item", val_delete_item },  /* 21 */
    { "cart", val_delete_cart },  /* 23 */
    { "phoneset", val_delete_phoneset },  /* 25 */
    { "lexicon", val_delete_lexicon },  /* 27 */
    { "dur_stats", val_delete_dur_stats },  /* 29 */
    { "diphonedb", val_delete_diphonedb },  /* 31 */
    { NULL, NULL },  /* 33 */
    { "wavesynth", val_delete_wavesynth },  /* 35 */
    { "audiotag", val_delete_audiotag },  /* 37 */
    { NULL, NULL },  /* 39 */
    { "userdata", val_delete_userdata },  /* 41 */
    { "itemfunc", val_delete_itemfunc },  /* 43 */
    { "features", val_delete_features },  /* 45 */
    { "phonefunc", val_delete_phonefunc },  /* 47 */
    { "xsampafunc", val_delete_xsampafunc },  /* 49 */
    { "allwordposfunc", val_delete_allwordposfunc },  /* 51 */
    { "zdur_cart", val_delete_zdur_cart },  /* 53 */
    { "valfunc", val_delete_valfunc },  /* 55 */
    { "string_tokenizer_new_func", val_delete_string_tokenizer_new_func },  /* 57 */
    { "join_cost_func", val_delete_join_cost_func },  /* 59 */
    { "blacklist", val_delete_blacklist },  /* 61 */
    { "tokenizer_expand_str_func", val_delete_tokenizer_expand_str_func },  /* 63 */
    { "lang_dat_core_data", val_delete_lang_dat_core_data },  /* 65 */
    { "metric", val_delete_metric },  /* 67 */
    { "synth_module_callback", val_delete_synth_module_callback },  /* 69 */
    { "float_ptr", val_delete_float_ptr },  /* 71 */
    { "phone_acoustic_data", val_delete_phone_acoustic_data },  /* 73 */
    { "vector", val_delete_vector },  /* 75 */
    { "word_embeddings", val_delete_word_embeddings },  /* 77 */
    { "ffunc2", val_delete_ffunc2 },  /* 79 */
    { "dnn_models", val_delete_dnn_models },  /* 81 */
    { NULL, NULL },  /* 83 */
    { "acoustic_covariances", val_delete_acoustic_covariances },  /* 85 */
    { "breaths_stats", val_delete_breaths_stats },  /* 87 */
    { "binary_key", val_delete_binary_key },  /* 89 */
    { "dynamic_voxdbs", val_delete_dynamic_voxdbs },  /* 91 */
};
