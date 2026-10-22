/* Self-test TU (arm mode). */
int g_counter;
extern int g_exported;
extern int lookup(void *n, int k);

int arm_helper(int a, int b)
{
    int i, s = 0;
    for (i = 0; i < b; i++)
        s += a * i + g_exported;
    g_counter = s;
    return s;
}

int arm_calls_thumb(void *n)
{
    return lookup(n, 3) * 3;
}

/* Aliases (like C++ C1/C2, D1/D2): one address, two names. Each caller references a different
   name, so whichever name the function table keeps, one call is checked through the alias. */
__attribute__((visibility("hidden"), noinline)) int alias_body(int x)
{
    return x * 7 + g_exported;
}
__attribute__((visibility("hidden"))) int alias_other(int x) __attribute__((alias("alias_body")));

int calls_alias_body(int x)
{
    return alias_body(x) + 1;
}

int calls_alias_other(int x)
{
    return alias_other(x) + 2;
}

extern int xalias_body(int x) __attribute__((visibility("hidden")));
extern int xalias_other(int x) __attribute__((visibility("hidden")));

int calls_xalias_body(int x)
{
    return xalias_body(x) * 3;
}

int calls_xalias_other(int x)
{
    return xalias_other(x) * 5;
}

/* Hand-written mixed-mode stub (like libgcc's _Unwind_* wrappers): ARM code that switches to
   Thumb and makes a Thumb `bl`; the matcher must decode that call as Thumb. */
__asm__(".text\n"
        ".arm\n"
        ".global mixed_stub\n"
        ".type mixed_stub, %function\n"
        "mixed_stub:\n"
        "    push {r4, lr}\n"
        "    add ip, pc, #1\n"
        "    bx ip\n"
        ".thumb\n"
        "    bl arm_helper\n"
        "    pop {r4, pc}\n"
        ".arm\n"
        ".size mixed_stub, .-mixed_stub\n");
