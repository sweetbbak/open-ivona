#include <stddef.h>
#include "cst_features.h"
#include "cst_lexicon.h"
#include "cst_utterance.h"
#include "tts_voice.h"

typedef struct voice_param {
    const char *key;
    const char *value;
} voice_param;

static const voice_param amy_defaults[] = {
    { "name", "vox_en_gb_amy" },
    { "voxdbname", "vox_en_gb_amy" },
    { "checksum", "1427195991;1427205298" },
    { "voxdb_format", "22v" },
    { "voice_name", "Amy" },
    { "voice_name_ascii", "Amy" },
    { "voice_gender", "Female" },
    { "voice_age", "37" },
    { "language_code", "en-GB" },
    { "language_name", "British English" },
    { NULL, NULL }
};
const voice_param * const amy_voice_params = amy_defaults;

extern const cst_lexicon amy_lexicon;
voice_reg *FUN_000586b0(const voice_param * const *params);
void *FUN_00029dd0(cst_features *features, const char *voxdir);
cst_utterance *duration_select(cst_utterance *u);
cst_val *userdata_val(const void *p);

void FUN_00058734(void *v);
void *FUN_0004b790(void *a, void *b, void *c);
void *FUN_000397d0(void *a, void *b, void *c);

static inline void set_lexicon(cst_features *f, const char *name, const cst_lexicon *lex)
{
    feat_set(f, name, lexicon_val(lex));
}

static inline void set_uttfunc(cst_features *f, const char *name, cst_uttfunc func)
{
    feat_set(f, name, uttfunc_val(func));
}

/* 0x396b8 arm: Construct Amy's voice, bind its models and load the vox database.
 * Descriptive name; the original symbol is unknown. */
voice_reg *amy_voice_new(const char *voxdir)
{
    voice_reg *v = FUN_000586b0(&amy_voice_params);
    /* Keep the string and feature-pointer lifetimes observed in the shipped code.
     * These empty constraints emit no instructions. */
    register const char *lex_name asm("r8") = "vox_lexicon";
    register const char *prediction_name asm("r7") = "acoustic_prediction_func";
    asm("" : "+r"(lex_name), "+r"(prediction_name));
    register cst_features *first_features asm("r9") = v->features;
    asm("" : "+r"(first_features));
    set_lexicon(first_features, lex_name, &amy_lexicon);
    feat_set_float(v->features, "duration_stretch", 1.0f);
    set_uttfunc(v->features, prediction_name, duration_select);
    if (FUN_00029dd0(v->features, voxdir)) {
        cst_features *features = v->features;
        cst_val *graph_func = userdata_val(FUN_000397d0);
        register const char *graph_name asm("r5") = "diphone_graph_func";
        asm("" : "+r"(graph_name));
        feat_set(features, graph_name, graph_func);
        feat_set_int(v->features, "viterbi_beam", 50);
        return v;
    }
    FUN_00058734(v);
    return NULL;
}

/* 0x397cc: arm */
void FUN_000397cc(void *v)
{
    FUN_00058734(v);
}

/* 0x397d0: arm */
void *FUN_000397d0(void *a, void *b, void *c)
{
    return FUN_0004b790(a, b, c);
}
