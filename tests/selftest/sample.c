/* Self-test TU (thumb): exercises every relocation kind the matcher must verify. */
#include <stdio.h>
#include <string.h>

#ifndef MUTATE
#define MUTATE 0
#endif
#ifndef GREETING
#define GREETING "hello %s"
#endif
#ifndef TABLE_LAST
#define TABLE_LAST 6
#endif
#ifndef SUB_OP
#define SUB_OP op_sub
#endif

struct node { int key; int val; struct node *next; };

extern int g_counter;                 /* defined in the other TU -> GOT_PREL */
int g_exported = 5;                   /* global defined here */
static int s_hidden;                  /* .bss static -> GOTOFF/PC-rel to section */
static const short k_table[8] = { 3, 1, 4, 1, 5, 9, 2, TABLE_LAST };
extern int arm_helper(int a, int b);  /* ARM-mode function in the other TU -> BLX */

static int add_key(struct node *n, int k)
{
    int s = 0;
    for (; n; n = n->next)
        if (n->key == k)
            s += n->val;
    return s;
}

int lookup(struct node *n, int k)
{
    return add_key(n, k) + g_counter + MUTATE;
}

int table_get(unsigned i)
{
    s_hidden++;
    return k_table[i & 7] + g_exported;
}

int classify(int x)
{
    switch (x) {
    case 0: return arm_helper(1, 2);
    case 1: return 17;
    case 2: return g_counter;
    case 3: return 99;
    case 4: return s_hidden;
    case 5: return -4;
    case 7: return 1234;
    default: return 0;
    }
}

int greet(const char *who)
{
    char buf[64];
    snprintf(buf, sizeof buf, GREETING, who);
    return (int)strlen(buf);
}

typedef int (*op_fn)(int, int);
static int op_add(int a, int b) { return a + b; }
static int op_sub(int a, int b) { return a - b; }
static const op_fn k_ops[2] = { op_add, SUB_OP };

int apply(int which, int a, int b)
{
    return k_ops[which & 1](a, b);
}
