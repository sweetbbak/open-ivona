#include "cst_features.h"
#include "tts_voice.h"

voice_reg *amy_voice_new(const char *voxdir);
void FUN_000397cc(voice_reg *reg);

/* 0x258b0: arm */
voice_reg *register_voice(const char *voxdir)
{
    voice_reg *reg = amy_voice_new(voxdir);
    if (reg != NULL)
        feat_set_string(reg->features, "eng_version", "1.6.101.3");
    return reg;
}

/* 0x258e8: arm */
void unregister_voice(voice_reg *reg)
{
    FUN_000397cc(reg);
}
