/* ARM-mode calibration functions (engine). Struct layouts are placeholders until identified. */
#include <stddef.h>

struct RefObj { int unk0; int unk4; int refcount; int unkc; };
struct Handle { struct RefObj *obj; };

/* 0x34104 arm: increment the reference count of the object behind a handle. */
void FUN_00034104(struct Handle *h)
{
    h->obj->refcount++;
}

/* 0x34118 arm: decrement the reference count of the object behind a handle. */
void FUN_00034118(struct Handle *h)
{
    h->obj->refcount--;
}

/* 0x355dc arm: true when the object's field at +0xc is zero. */
int FUN_000355dc(struct Handle *h)
{
    return h->obj->unkc == 0;
}

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
