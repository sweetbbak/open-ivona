/*
 * Concatenation costs (0x4c038..0x4d954): join cost functions per phone class, registered as
 * "conc_cost_*" features of a voice.
 */
#include "cst_features.h"
#include "cst_val.h"
#include "tts_voice.h"

typedef struct join_cost_func join_cost_func;
cst_val *join_cost_func_val(const join_cost_func *f);
void voice_reg_set_language(voice_reg *reg, tts_lang_desc *table,
                            void *(*part_data)(tts_lang_desc *lang, const char *name));
void voice_reg_init(voice_reg *reg);

int conc_cost_plosive_unvoiced();
int conc_cost_trill();
int conc_cost_unvoiced();
int conc_cost_vowel();
int conc_cost_glide();
int conc_cost_voiced();
int conc_cost_plosive_voiced();

/* 0x4c038 arm */
void voice_attach_language(voice_reg *reg, tts_lang_desc *table,
                           void *(*part_data)(tts_lang_desc *lang, const char *name))
{
    voice_reg_set_language(reg, table, part_data);
    voice_reg_init(reg);
}

static inline void voice_reg_set_conc_cost(voice_reg *reg, const char *name,
                                           int (*f)())
{
    cst_features *feats = reg->features;

    feat_set(feats, name, join_cost_func_val((join_cost_func *)f));
}

/* 0x4d7ec arm */
void voice_reg_init(voice_reg *reg)
{
    voice_reg_set_conc_cost(reg, "conc_cost_vowel", conc_cost_vowel);
    voice_reg_set_conc_cost(reg, "conc_cost_plosive_unvoiced", conc_cost_plosive_unvoiced);
    voice_reg_set_conc_cost(reg, "conc_cost_plosive_voiced", conc_cost_plosive_voiced);
    voice_reg_set_conc_cost(reg, "conc_cost_trill", conc_cost_trill);
    voice_reg_set_conc_cost(reg, "conc_cost_glide", conc_cost_glide);
    voice_reg_set_conc_cost(reg, "conc_cost_unvoiced", conc_cost_unvoiced);
    voice_reg_set_conc_cost(reg, "conc_cost_voiced", conc_cost_voiced);
}
