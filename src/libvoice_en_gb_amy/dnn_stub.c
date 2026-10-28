/* DNN models (0x26e1c..0x26e44): this build has no DNN support, so the API is stubbed out. */
#include <stddef.h>

/* 0x26e1c arm */
void *new_dnn_models(void)
{
    return NULL;
}

/* 0x26e24 arm */
void *dnn_stub_2c(void)
{
    return NULL;
}

/* 0x26e2c arm */
void dnn_stub_34(void)
{
}

/* 0x26e30 arm */
void delete_dnn_models(void *m)
{
}

/* 0x26e34 arm */
int dnn_stub_3c(void)
{
    return 0;
}

/* 0x26e3c arm */
int dnn_stub_44(void)
{
    return 0;
}
