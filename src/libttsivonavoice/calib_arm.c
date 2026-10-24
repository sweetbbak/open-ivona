/* ARM-mode calibration functions (engine). Struct layouts are placeholders until identified. */
#include <stddef.h>

struct Module {
    void *unk0;
    void *unk4;
    void (*release)(void *obj);
    void (*fn_c)(void *a, void *b);
    void (*fn_10)(void *a, void *b);
};

extern struct Module *g_modules[4];

/* 0x36a3c arm: broadcast to every registered module. */
void FUN_00036a3c(void *a, void *b)
{
    int i;
    for (i = 0; i < 4; i++)
        g_modules[i]->fn_c(a, b);
}
