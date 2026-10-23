/* DNN models (0x56324..0x5634c): this build has no DNN support, so the API is stubbed out. */
#include <stddef.h>

/* 0x56324 arm */
void *new_dnn_models(void)
{
    return NULL;
}

/* 0x5632c arm */
void *dnn_stub_2c(void)
{
    return NULL;
}

/* 0x56334 arm */
void dnn_stub_34(void)
{
}

/* 0x56338 arm */
void delete_dnn_models(void *m)
{
}

/* 0x5633c arm */
int dnn_stub_3c(void)
{
    return 0;
}

/* 0x56344 arm */
int dnn_stub_44(void)
{
    return 0;
}
