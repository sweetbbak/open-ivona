/* Flite cst_endian.c (only swap_bytes_short survived --gc-sections). */
#include "cst_endian.h"

/* its first byte is 1 on a little-endian machine (CST_LITTLE_ENDIAN) */
const int cst_endian_loc = 1;

/* 0x519d0: byte-swap n shorts in place. */
void swap_bytes_short(short *b, int n)
{
    int i;

    for (i = 0; i < n; i++)
        b[i] = SWAPSHORT_U(b[i]);
}
