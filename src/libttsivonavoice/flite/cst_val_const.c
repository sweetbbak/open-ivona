/*
 * Flite cst_val_const.c: constant small int / numeric-string vals, shared instead of allocated.
 * IVONA: val_int_n / val_string_n clamp out-of-range (including negative) n to the last entry.
 */
#include "cst_val.h"
#include "cst_features.h"
#include "cst_string.h"

DEF_CONST_VAL_STRING(val_string_0,"0");
DEF_CONST_VAL_STRING(val_string_1,"1");
DEF_CONST_VAL_STRING(val_string_2,"2");
DEF_CONST_VAL_STRING(val_string_3,"3");
DEF_CONST_VAL_STRING(val_string_4,"4");
DEF_CONST_VAL_STRING(val_string_5,"5");
DEF_CONST_VAL_STRING(val_string_6,"6");
DEF_CONST_VAL_STRING(val_string_7,"7");
DEF_CONST_VAL_STRING(val_string_8,"8");
DEF_CONST_VAL_STRING(val_string_9,"9");
DEF_CONST_VAL_STRING(val_string_10,"10");
DEF_CONST_VAL_STRING(val_string_11,"11");
DEF_CONST_VAL_STRING(val_string_12,"12");
DEF_CONST_VAL_STRING(val_string_13,"13");
DEF_CONST_VAL_STRING(val_string_14,"14");
DEF_CONST_VAL_STRING(val_string_15,"15");
DEF_CONST_VAL_STRING(val_string_16,"16");
DEF_CONST_VAL_STRING(val_string_17,"17");
DEF_CONST_VAL_STRING(val_string_18,"18");
DEF_CONST_VAL_STRING(val_string_19,"19");
DEF_CONST_VAL_INT(val_int_0,0);
DEF_CONST_VAL_INT(val_int_1,1);
DEF_CONST_VAL_INT(val_int_2,2);
DEF_CONST_VAL_INT(val_int_3,3);
DEF_CONST_VAL_INT(val_int_4,4);
DEF_CONST_VAL_INT(val_int_5,5);
DEF_CONST_VAL_INT(val_int_6,6);
DEF_CONST_VAL_INT(val_int_7,7);
DEF_CONST_VAL_INT(val_int_8,8);
DEF_CONST_VAL_INT(val_int_9,9);
DEF_CONST_VAL_INT(val_int_10,10);
DEF_CONST_VAL_INT(val_int_11,11);
DEF_CONST_VAL_INT(val_int_12,12);
DEF_CONST_VAL_INT(val_int_13,13);
DEF_CONST_VAL_INT(val_int_14,14);
DEF_CONST_VAL_INT(val_int_15,15);
DEF_CONST_VAL_INT(val_int_16,16);
DEF_CONST_VAL_INT(val_int_17,17);
DEF_CONST_VAL_INT(val_int_18,18);
DEF_CONST_VAL_INT(val_int_19,19);

static const int val_int_const_max = 20;
static const cst_val * const val_int_const[] = {
    VAL_INT_0, VAL_INT_1, VAL_INT_2, VAL_INT_3, VAL_INT_4,
    VAL_INT_5, VAL_INT_6, VAL_INT_7, VAL_INT_8, VAL_INT_9,
    VAL_INT_10, VAL_INT_11, VAL_INT_12, VAL_INT_13, VAL_INT_14,
    VAL_INT_15, VAL_INT_16, VAL_INT_17, VAL_INT_18, VAL_INT_19 };

static const cst_val * const val_string_const[] = {
    VAL_STRING_0, VAL_STRING_1, VAL_STRING_2, VAL_STRING_3, VAL_STRING_4,
    VAL_STRING_5, VAL_STRING_6, VAL_STRING_7, VAL_STRING_8, VAL_STRING_9,
    VAL_STRING_10, VAL_STRING_11, VAL_STRING_12, VAL_STRING_13, VAL_STRING_14,
    VAL_STRING_15, VAL_STRING_16, VAL_STRING_17, VAL_STRING_18, VAL_STRING_19 };

/* 0x55fa8 */
const cst_val *val_int_n(int n)
{
    if (n >= 0 && n < val_int_const_max)
        return val_int_const[n];
    return VAL_INT_19;
}

/* 0x55fd4 */
const cst_val *val_string_n(int n)
{
    if (n >= 0 && n < val_int_const_max)  /* yes, *int*: it is the table size */
        return val_string_const[n];
    return VAL_STRING_19;
}
