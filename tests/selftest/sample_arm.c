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
