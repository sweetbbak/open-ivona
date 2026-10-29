/* Language data setup (0x3c068..): the en-US lexicons for "en-US", the en-GB ones otherwise. */
#include <string.h>
#include "en_lang.h"

void en_us_setup(en_lang_core *core);
void en_gb_setup(en_lang_core *core);
void en_us_teardown(void);
void en_gb_teardown(void);

/* 0x3c068 arm */
void en_lang_setup(en_lang_core *core)
{
    if (strcmp(core->code, "en-US") == 0)
        en_us_setup(core);
    else
        en_gb_setup(core);
}

/* 0x3c0a0 arm */
void en_lang_teardown(en_lang_core *core)
{
    if (strcmp(core->code, "en-US") == 0)
        en_us_teardown();
    else
        en_gb_teardown();
}
