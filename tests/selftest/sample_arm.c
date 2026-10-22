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
