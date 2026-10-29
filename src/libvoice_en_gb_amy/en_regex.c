/* IVONA's English regexes (generated: tools/gen_regex.py), each a const cst_regex pointer as in Flite's cst_regex_defs.h; the comments show the compiled patterns. */
#include <stddef.h>
#include "cst_regex.h"

/* BRANCH BOL BRANCH EXACTLY(') BRANCH NOTHING BRANCH OPEN1 BRANCH PLUS ANYOF(abcdefghijklmnopqrstuvwxyzàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿßABCDEFGHIJKLMNOPQRSTUVWXYZÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞ) EXACTLY(') CLOSE1 BACK BRANCH NOTHING PLUS ANYOF(abcdefghijklmnopqrstuvwxyzàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿßABCDEFGHIJKLMNOPQRSTUVWXYZÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞ) BRANCH EXACTLY(') BRANCH NOTHING BRANCH OPEN2 BRANCH EXACTLY(-) BRANCH EXACTLY(') BRANCH NOTHING BRANCH OPEN3 BRANCH PLUS ANYOF(abcdefghijklmnopqrstuvwxyzàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿßABCDEFGHIJKLMNOPQRSTUVWXYZÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞ) EXACTLY(') CLOSE3 BACK BRANCH NOTHING PLUS ANYOF(abcdefghijklmnopqrstuvwxyzàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿßABCDEFGHIJKLMNOPQRSTUVWXYZÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞ) BRANCH EXACTLY(') BRANCH NOTHING CLOSE2 BACK BRANCH NOTHING EOL END */
static const unsigned char en_rx_quoted_rxprog[] = {  /* 0x38cd14 */
    156, 6, 2, 115, 1, 0, 3, 6, 0, 8, 8, 0, 8, 39, 0, 6,
    0, 3, 9, 0, 3, 6, 0, 141, 21, 0, 3, 6, 0, 129, 11, 0,
    121, 4, 0, 0, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108,
    109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 224, 225,
    226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241,
    242, 243, 244, 245, 246, 248, 249, 250, 251, 252, 253, 254, 255, 223, 65, 66,
    67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82,
    83, 84, 85, 86, 87, 88, 89, 90, 192, 193, 194, 195, 196, 197, 198, 199,
    200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 216,
    217, 218, 219, 220, 221, 222, 0, 8, 0, 5, 39, 0, 41, 0, 3, 7,
    0, 138, 6, 0, 3, 9, 0, 3, 11, 0, 121, 4, 0, 0, 97, 98,
    99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114,
    115, 116, 117, 118, 119, 120, 121, 122, 224, 225, 226, 227, 228, 229, 230, 231,
    232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 248,
    249, 250, 251, 252, 253, 254, 255, 223, 65, 66, 67, 68, 69, 70, 71, 72,
    73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88,
    89, 90, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205,
    206, 207, 208, 209, 210, 211, 212, 213, 214, 216, 217, 218, 219, 220, 221, 222,
    0, 6, 0, 8, 8, 0, 8, 39, 0, 6, 0, 3, 9, 0, 3, 6,
    1, 60, 22, 0, 3, 6, 1, 48, 8, 0, 5, 45, 0, 6, 0, 8,
    8, 0, 8, 39, 0, 6, 0, 3, 9, 0, 3, 6, 0, 141, 23, 0,
    3, 6, 0, 129, 11, 0, 121, 4, 0, 0, 97, 98, 99, 100, 101, 102,
    103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118,
    119, 120, 121, 122, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235,
    236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 248, 249, 250, 251, 252,
    253, 254, 255, 223, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
    77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 192, 193,
    194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
    210, 211, 212, 213, 214, 216, 217, 218, 219, 220, 221, 222, 0, 8, 0, 5,
    39, 0, 43, 0, 3, 7, 0, 138, 6, 0, 3, 9, 0, 3, 11, 0,
    121, 4, 0, 0, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108,
    109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 224, 225,
    226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241,
    242, 243, 244, 245, 246, 248, 249, 250, 251, 252, 253, 254, 255, 223, 65, 66,
    67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82,
    83, 84, 85, 86, 87, 88, 89, 90, 192, 193, 194, 195, 196, 197, 198, 199,
    200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 216,
    217, 218, 219, 220, 221, 222, 0, 6, 0, 8, 8, 0, 8, 39, 0, 6,
    0, 3, 9, 0, 3, 42, 0, 3, 7, 1, 57, 6, 0, 3, 9, 0,
    3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_quoted_rx = {  /* 0x3d3110 */
   0, 1, NULL, 0, 631,
   (char *)en_rx_quoted_rxprog
};
const cst_regex * const en_rx_quoted = &en_rx_quoted_rx;  /* 0x3d310c */

/* BRANCH BOL OPEN1 BRANCH EXACTLY(AL) BRANCH EXACTLY(AK) BRANCH EXACTLY(AZ) BRANCH EXACTLY(AR) BRANCH EXACTLY(CA) BRANCH EXACTLY(CO) BRANCH EXACTLY(CT) BRANCH EXACTLY(DE) BRANCH EXACTLY(FL) BRANCH EXACTLY(GA) BRANCH EXACTLY(HI) BRANCH EXACTLY(ID) BRANCH EXACTLY(IL) BRANCH EXACTLY(IN) BRANCH EXACTLY(IA) BRANCH EXACTLY(KS) BRANCH EXACTLY(KY) BRANCH EXACTLY(LA) BRANCH EXACTLY(ME) BRANCH EXACTLY(MD) BRANCH EXACTLY(MA) BRANCH EXACTLY(MI) BRANCH EXACTLY(MN) BRANCH EXACTLY(MS) BRANCH EXACTLY(MO) BRANCH EXACTLY(MT) BRANCH EXACTLY(NE) BRANCH EXACTLY(NV) BRANCH EXACTLY(NH) BRANCH EXACTLY(NJ) BRANCH EXACTLY(NM) BRANCH EXACTLY(NY) BRANCH EXACTLY(NC) BRANCH EXACTLY(ND) BRANCH EXACTLY(OH) BRANCH EXACTLY(OK) BRANCH EXACTLY(OR) BRANCH EXACTLY(PA) BRANCH EXACTLY(RI) BRANCH EXACTLY(SC) BRANCH EXACTLY(SD) BRANCH EXACTLY(TN) BRANCH EXACTLY(TX) BRANCH EXACTLY(UT) BRANCH EXACTLY(VT) BRANCH EXACTLY(VA) BRANCH EXACTLY(WA) BRANCH EXACTLY(WV) BRANCH EXACTLY(WI) BRANCH EXACTLY(WY) CLOSE1 OPEN2 BRANCH EXACTLY(-) PLUS ANYOF(0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞ) CLOSE2 BRANCH BACK BRANCH NOTHING BRANCH OPEN3 BRANCH EXACTLY(-) OPEN4 BRANCH EXACTLY(LOOP) BRANCH EXACTLY(TOLL) BRANCH EXACTLY(W) BRANCH EXACTLY(N) BRANCH EXACTLY(S) BRANCH EXACTLY(E) CLOSE4 CLOSE3 BRANCH NOTHING EOL END */
static const unsigned char en_rx_us_state_rxprog[] = {  /* 0x38cf8c */
    156, 6, 2, 135, 1, 0, 3, 21, 0, 3, 6, 0, 9, 8, 1, 191,
    65, 76, 0, 6, 0, 9, 8, 1, 182, 65, 75, 0, 6, 0, 9, 8,
    1, 173, 65, 90, 0, 6, 0, 9, 8, 1, 164, 65, 82, 0, 6, 0,
    9, 8, 1, 155, 67, 65, 0, 6, 0, 9, 8, 1, 146, 67, 79, 0,
    6, 0, 9, 8, 1, 137, 67, 84, 0, 6, 0, 9, 8, 1, 128, 68,
    69, 0, 6, 0, 9, 8, 1, 119, 70, 76, 0, 6, 0, 9, 8, 1,
    110, 71, 65, 0, 6, 0, 9, 8, 1, 101, 72, 73, 0, 6, 0, 9,
    8, 1, 92, 73, 68, 0, 6, 0, 9, 8, 1, 83, 73, 76, 0, 6,
    0, 9, 8, 1, 74, 73, 78, 0, 6, 0, 9, 8, 1, 65, 73, 65,
    0, 6, 0, 9, 8, 1, 56, 75, 83, 0, 6, 0, 9, 8, 1, 47,
    75, 89, 0, 6, 0, 9, 8, 1, 38, 76, 65, 0, 6, 0, 9, 8,
    1, 29, 77, 69, 0, 6, 0, 9, 8, 1, 20, 77, 68, 0, 6, 0,
    9, 8, 1, 11, 77, 65, 0, 6, 0, 9, 8, 1, 2, 77, 73, 0,
    6, 0, 9, 8, 0, 249, 77, 78, 0, 6, 0, 9, 8, 0, 240, 77,
    83, 0, 6, 0, 9, 8, 0, 231, 77, 79, 0, 6, 0, 9, 8, 0,
    222, 77, 84, 0, 6, 0, 9, 8, 0, 213, 78, 69, 0, 6, 0, 9,
    8, 0, 204, 78, 86, 0, 6, 0, 9, 8, 0, 195, 78, 72, 0, 6,
    0, 9, 8, 0, 186, 78, 74, 0, 6, 0, 9, 8, 0, 177, 78, 77,
    0, 6, 0, 9, 8, 0, 168, 78, 89, 0, 6, 0, 9, 8, 0, 159,
    78, 67, 0, 6, 0, 9, 8, 0, 150, 78, 68, 0, 6, 0, 9, 8,
    0, 141, 79, 72, 0, 6, 0, 9, 8, 0, 132, 79, 75, 0, 6, 0,
    9, 8, 0, 123, 79, 82, 0, 6, 0, 9, 8, 0, 114, 80, 65, 0,
    6, 0, 9, 8, 0, 105, 82, 73, 0, 6, 0, 9, 8, 0, 96, 83,
    67, 0, 6, 0, 9, 8, 0, 87, 83, 68, 0, 6, 0, 9, 8, 0,
    78, 84, 78, 0, 6, 0, 9, 8, 0, 69, 84, 88, 0, 6, 0, 9,
    8, 0, 60, 85, 84, 0, 6, 0, 9, 8, 0, 51, 86, 84, 0, 6,
    0, 9, 8, 0, 42, 86, 65, 0, 6, 0, 9, 8, 0, 33, 87, 65,
    0, 6, 0, 9, 8, 0, 24, 87, 86, 0, 6, 0, 9, 8, 0, 15,
    87, 73, 0, 6, 0, 9, 8, 0, 6, 87, 89, 0, 41, 0, 3, 22,
    0, 3, 6, 0, 81, 8, 0, 5, 45, 0, 11, 0, 73, 4, 0, 0,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 65, 66, 67, 68, 69, 70,
    71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
    87, 88, 89, 90, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203,
    204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 216, 217, 218, 219, 220,
    221, 222, 0, 42, 0, 3, 6, 0, 6, 7, 0, 90, 6, 0, 3, 9,
    0, 3, 6, 0, 77, 23, 0, 3, 6, 0, 68, 8, 0, 5, 45, 0,
    24, 0, 3, 6, 0, 11, 8, 0, 51, 76, 79, 79, 80, 0, 6, 0,
    11, 8, 0, 40, 84, 79, 76, 76, 0, 6, 0, 8, 8, 0, 29, 87,
    0, 6, 0, 8, 8, 0, 21, 78, 0, 6, 0, 8, 8, 0, 13, 83,
    0, 6, 0, 8, 8, 0, 5, 69, 0, 44, 0, 3, 43, 0, 6, 6,
    0, 3, 9, 0, 3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_us_state_rx = {  /* 0x3d3128 */
   0, 1, NULL, 0, 651,
   (char *)en_rx_us_state_rxprog
};
const cst_regex * const en_rx_us_state = &en_rx_us_state_rx;  /* 0x3d3124 */

/* BRANCH BOL ANYOF(ABCDEFGHIJKLMNOPQRSTUVWXYZÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞ) EXACTLY(-) PLUS ANYOF(0123456789) EOL END */
static const unsigned char en_rx_letter_number_rxprog[] = {  /* 0x38d218 */
    156, 6, 0, 91, 1, 0, 3, 4, 0, 60, 65, 66, 67, 68, 69, 70,
    71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
    87, 88, 89, 90, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203,
    204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 216, 217, 218, 219, 220,
    221, 222, 0, 8, 0, 5, 45, 0, 11, 0, 17, 4, 0, 0, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 0, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_letter_number_rx = {  /* 0x3d3140 */
   0, 1, NULL, 0, 95,
   (char *)en_rx_letter_number_rxprog
};
const cst_regex * const en_rx_letter_number = &en_rx_letter_number_rx;  /* 0x3d313c */

/* BRANCH BOL OPEN1 BRANCH EXACTLY(GMT) BRANCH EXACTLY(gmt) BRANCH EXACTLY(UTC) BRANCH EXACTLY(utc) CLOSE1 ANYOF(+-) OPEN2 BRANCH BRANCH EXACTLY(0) BRANCH NOTHING ANYOF(0123456789) BRANCH EXACTLY(1) ANYOF(01234) CLOSE2 BRANCH OPEN3 BRANCH BRANCH EXACTLY(:) BRANCH NOTHING OPEN4 BRANCH EXACTLY(00) BRANCH EXACTLY(30) BRANCH EXACTLY(45) CLOSE4 CLOSE3 BRANCH NOTHING EOL END */
static const unsigned char en_rx_utc_offset_rxprog[] = {  /* 0x38d278 */
    156, 6, 0, 180, 1, 0, 3, 21, 0, 3, 6, 0, 10, 8, 0, 37,
    71, 77, 84, 0, 6, 0, 10, 8, 0, 27, 103, 109, 116, 0, 6, 0,
    10, 8, 0, 17, 85, 84, 67, 0, 6, 0, 10, 8, 0, 7, 117, 116,
    99, 0, 41, 0, 3, 4, 0, 6, 43, 45, 0, 22, 0, 3, 6, 0,
    31, 6, 0, 8, 8, 0, 8, 48, 0, 6, 0, 3, 9, 0, 3, 4,
    0, 31, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 17,
    8, 0, 5, 49, 0, 4, 0, 9, 48, 49, 50, 51, 52, 0, 42, 0,
    3, 6, 0, 59, 23, 0, 3, 6, 0, 50, 6, 0, 8, 8, 0, 8,
    58, 0, 6, 0, 3, 9, 0, 3, 24, 0, 3, 6, 0, 9, 8, 0,
    24, 48, 48, 0, 6, 0, 9, 8, 0, 15, 51, 48, 0, 6, 0, 9,
    8, 0, 6, 52, 53, 0, 44, 0, 3, 43, 0, 6, 6, 0, 3, 9,
    0, 3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_utc_offset_rx = {  /* 0x3d3158 */
   0, 1, NULL, 0, 184,
   (char *)en_rx_utc_offset_rxprog
};
const cst_regex * const en_rx_utc_offset = &en_rx_utc_offset_rx;  /* 0x3d3154 */

/* BRANCH BOL OPEN1 BRANCH BRANCH ANYOF(0123456789) BRANCH NOTHING ANYOF(0123456789) CLOSE1 EOL END */
static const unsigned char en_rx_one_two_digits_rxprog[] = {  /* 0x38d330 */
    156, 6, 0, 55, 1, 0, 3, 21, 0, 3, 6, 0, 40, 6, 0, 17,
    4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0,
    3, 9, 0, 3, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56,
    57, 0, 41, 0, 3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_one_two_digits_rx = {  /* 0x3d3170 */
   0, 1, NULL, 0, 59,
   (char *)en_rx_one_two_digits_rxprog
};
const cst_regex * const en_rx_one_two_digits = &en_rx_one_two_digits_rx;  /* 0x3d316c */

/* BRANCH BOL OPEN1 BRANCH BRANCH ANYOF(0123456789) BRANCH NOTHING ANYOF(0123456789) CLOSE1 BRANCH OPEN2 BRANCH EXACTLY(:) ANYOF(0123456789) ANYOF(0123456789) BRANCH OPEN3 BRANCH EXACTLY(:) ANYOF(0123456789) ANYOF(0123456789) CLOSE3 BRANCH NOTHING CLOSE2 BRANCH NOTHING ANYOF(AaPp) BRANCH EXACTLY(.) BRANCH NOTHING ANYOF(Mm) BRANCH EXACTLY(.) BRANCH NOTHING EOL END */
static const unsigned char en_rx_time_ampm_rxprog[] = {  /* 0x38d36c */
    156, 6, 0, 199, 1, 0, 3, 21, 0, 3, 6, 0, 40, 6, 0, 17,
    4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0,
    3, 9, 0, 3, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56,
    57, 0, 41, 0, 3, 6, 0, 96, 22, 0, 3, 6, 0, 87, 8, 0,
    5, 58, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6,
    0, 45, 23, 0, 3, 6, 0, 36, 8, 0, 5, 58, 0, 4, 0, 14,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 4, 0, 14, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 0, 43, 0, 6, 6, 0, 3, 9,
    0, 3, 42, 0, 6, 6, 0, 3, 9, 0, 3, 4, 0, 8, 65, 97,
    80, 112, 0, 6, 0, 8, 8, 0, 8, 46, 0, 6, 0, 3, 9, 0,
    3, 4, 0, 6, 77, 109, 0, 6, 0, 8, 8, 0, 8, 46, 0, 6,
    0, 3, 9, 0, 3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_time_ampm_rx = {  /* 0x3d3188 */
   0, 1, NULL, 0, 203,
   (char *)en_rx_time_ampm_rxprog
};
const cst_regex * const en_rx_time_ampm = &en_rx_time_ampm_rx;  /* 0x3d3184 */

/* BRANCH BOL OPEN1 BRANCH BRANCH ANYOF(0123456789) BRANCH NOTHING ANYOF(0123456789) CLOSE1 EXACTLY(:) ANYOF(0123456789) ANYOF(0123456789) BRANCH OPEN2 BRANCH EXACTLY(:) ANYOF(0123456789) ANYOF(0123456789) CLOSE2 BRANCH NOTHING EOL END */
static const unsigned char en_rx_time_colon_rxprog[] = {  /* 0x38d438 */
    156, 6, 0, 139, 1, 0, 3, 21, 0, 3, 6, 0, 40, 6, 0, 17,
    4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0,
    3, 9, 0, 3, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56,
    57, 0, 41, 0, 3, 8, 0, 5, 58, 0, 4, 0, 14, 48, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 0, 4, 0, 14, 48, 49, 50, 51, 52,
    53, 54, 55, 56, 57, 0, 6, 0, 45, 22, 0, 3, 6, 0, 36, 8,
    0, 5, 58, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56,
    57, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0,
    42, 0, 6, 6, 0, 3, 9, 0, 3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_time_colon_rx = {  /* 0x3d31a0 */
   0, 1, NULL, 0, 143,
   (char *)en_rx_time_colon_rxprog
};
const cst_regex * const en_rx_time_colon = &en_rx_time_colon_rx;  /* 0x3d319c */

/* BRANCH BOL ANYOF(0123456789) ANYOF(0123456789) ANYOF(0123456789) ANYOF(0123456789) ANYOF(0123456789) BRANCH OPEN1 BRANCH EXACTLY(-) ANYOF(0123456789) ANYOF(0123456789) ANYOF(0123456789) ANYOF(0123456789) CLOSE1 BRANCH NOTHING EOL END */
static const unsigned char en_rx_zip_code_rxprog[] = {  /* 0x38d4c8 */
    156, 6, 0, 158, 1, 0, 3, 4, 0, 14, 48, 49, 50, 51, 52, 53,
    54, 55, 56, 57, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 4,
    0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 73,
    21, 0, 3, 6, 0, 64, 8, 0, 5, 45, 0, 4, 0, 14, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 0, 4, 0, 14, 48, 49, 50, 51,
    52, 53, 54, 55, 56, 57, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53,
    54, 55, 56, 57, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 0, 41, 0, 6, 6, 0, 3, 9, 0, 3, 2, 0, 3, 0,
    0, 0,
};
static const cst_regex en_rx_zip_code_rx = {  /* 0x3d31b8 */
   0, 1, NULL, 0, 162,
   (char *)en_rx_zip_code_rxprog
};
const cst_regex * const en_rx_zip_code = &en_rx_zip_code_rx;  /* 0x3d31b4 */

/* BRANCH BOL ANYOF(KW) ANYOF(ABCDEFGHIJKLMNOPQRSTUVWXYZ) ANYOF(ABCDEFGHIJKLMNOPQRSTUVWXYZ) BRANCH ANYOF(ABCDEFGHIJKLMNOPQRSTUVWXYZ) BRANCH NOTHING BRANCH OPEN1 BRANCH EXACTLY(-FM) CLOSE1 BRANCH NOTHING EOL END */
static const unsigned char en_rx_call_sign_rxprog[] = {  /* 0x38d56c */
    156, 6, 0, 139, 1, 0, 3, 4, 0, 6, 75, 87, 0, 4, 0, 30,
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 0, 4, 0, 30, 65, 66,
    67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82,
    83, 84, 85, 86, 87, 88, 89, 90, 0, 6, 0, 33, 4, 0, 33, 65,
    66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
    82, 83, 84, 85, 86, 87, 88, 89, 90, 0, 6, 0, 3, 9, 0, 3,
    6, 0, 19, 21, 0, 3, 6, 0, 10, 8, 0, 7, 45, 70, 77, 0,
    41, 0, 6, 6, 0, 3, 9, 0, 3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_call_sign_rx = {  /* 0x3d31d0 */
   0, 1, NULL, 0, 143,
   (char *)en_rx_call_sign_rxprog
};
const cst_regex * const en_rx_call_sign = &en_rx_call_sign_rx;  /* 0x3d31cc */

/* BRANCH BOL OPEN1 BRANCH EXACTLY(m) BRANCH EXACTLY(d) BRANCH NOTHING BRANCH EXACTLY(y) BRANCH NOTHING BRANCH EXACTLY(d) BRANCH EXACTLY(m) BRANCH NOTHING BRANCH EXACTLY(y) BRANCH NOTHING BRANCH EXACTLY(y) BRANCH EXACTLY(m) BRANCH NOTHING BRANCH EXACTLY(d) BRANCH NOTHING CLOSE1 EOL END */
static const unsigned char en_rx_date_order_rxprog[] = {  /* 0x38d5fc */
    156, 6, 0, 123, 1, 0, 3, 21, 0, 3, 6, 0, 36, 8, 0, 5,
    109, 0, 6, 0, 8, 8, 0, 8, 100, 0, 6, 0, 3, 9, 0, 3,
    6, 0, 8, 8, 0, 8, 121, 0, 6, 0, 3, 9, 0, 75, 6, 0,
    36, 8, 0, 5, 100, 0, 6, 0, 8, 8, 0, 8, 109, 0, 6, 0,
    3, 9, 0, 3, 6, 0, 8, 8, 0, 8, 121, 0, 6, 0, 3, 9,
    0, 39, 6, 0, 36, 8, 0, 5, 121, 0, 6, 0, 8, 8, 0, 8,
    109, 0, 6, 0, 3, 9, 0, 3, 6, 0, 8, 8, 0, 8, 100, 0,
    6, 0, 3, 9, 0, 3, 41, 0, 3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_date_order_rx = {  /* 0x3d31e8 */
   0, 1, NULL, 0, 127,
   (char *)en_rx_date_order_rxprog
};
const cst_regex * const en_rx_date_order = &en_rx_date_order_rx;  /* 0x3d31e4 */

/* BRANCH BOL OPEN1 BRANCH BRANCH OPEN2 BRANCH BRANCH OPEN3 BRANCH ANYOF(0123456789) BRANCH ANYOF(0123456789) BRANCH NOTHING ANYOF(./-) CLOSE3 BRANCH NOTHING ANYOF(0123456789) BRANCH ANYOF(0123456789) BRANCH NOTHING ANYOF(./-) CLOSE2 BRANCH NOTHING ANYOF(0123456789) BRANCH ANYOF(0123456789) BRANCH NOTHING BRANCH ANYOF(0123456789) BRANCH NOTHING BRANCH ANYOF(0123456789) BRANCH NOTHING BRANCH ANYOF(0123456789) BRANCH ANYOF(0123456789) BRANCH NOTHING BRANCH OPEN4 BRANCH ANYOF(./-) ANYOF(0123456789) BRANCH ANYOF(0123456789) BRANCH NOTHING CLOSE4 BRANCH NOTHING BRANCH ANYOF(0123456789) BRANCH ANYOF(0123456789) BRANCH NOTHING BRANCH ANYOF(0123456789) BRANCH NOTHING BRANCH ANYOF(0123456789) BRANCH NOTHING ANYOF(./-) ANYOF(0123456789) BRANCH ANYOF(0123456789) BRANCH NOTHING BRANCH OPEN5 BRANCH ANYOF(./-) ANYOF(0123456789) BRANCH ANYOF(0123456789) BRANCH NOTHING CLOSE5 BRANCH NOTHING CLOSE1 EOL END */
static const unsigned char en_rx_date_rxprog[] = {  /* 0x38d67c */
    156, 6, 2, 7, 1, 0, 3, 21, 0, 3, 6, 0, 210, 6, 0, 118,
    22, 0, 3, 6, 0, 109, 6, 0, 56, 23, 0, 3, 6, 0, 47, 4,
    0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 17,
    4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0,
    3, 9, 0, 3, 4, 0, 7, 46, 47, 45, 0, 43, 0, 6, 6, 0,
    3, 9, 0, 3, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56,
    57, 0, 6, 0, 17, 4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 0, 6, 0, 3, 9, 0, 3, 4, 0, 7, 46, 47, 45, 0,
    42, 0, 6, 6, 0, 3, 9, 0, 3, 4, 0, 14, 48, 49, 50, 51,
    52, 53, 54, 55, 56, 57, 0, 6, 0, 17, 4, 0, 17, 48, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 3, 9, 0, 3, 6, 0,
    17, 4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6,
    0, 3, 9, 0, 3, 6, 0, 17, 4, 0, 17, 48, 49, 50, 51, 52,
    53, 54, 55, 56, 57, 0, 6, 0, 3, 9, 1, 41, 6, 0, 102, 4,
    0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 17,
    4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0,
    3, 9, 0, 3, 6, 0, 56, 24, 0, 3, 6, 0, 47, 4, 0, 7,
    46, 47, 45, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56,
    57, 0, 6, 0, 17, 4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 0, 6, 0, 3, 9, 0, 3, 44, 0, 6, 6, 0, 3, 9,
    0, 195, 6, 0, 192, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 0, 6, 0, 17, 4, 0, 17, 48, 49, 50, 51, 52, 53, 54,
    55, 56, 57, 0, 6, 0, 3, 9, 0, 3, 6, 0, 17, 4, 0, 17,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 3, 9, 0,
    3, 6, 0, 17, 4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55, 56,
    57, 0, 6, 0, 3, 9, 0, 3, 4, 0, 7, 46, 47, 45, 0, 4,
    0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 17,
    4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0,
    3, 9, 0, 3, 6, 0, 56, 25, 0, 3, 6, 0, 47, 4, 0, 7,
    46, 47, 45, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56,
    57, 0, 6, 0, 17, 4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 0, 6, 0, 3, 9, 0, 3, 45, 0, 6, 6, 0, 3, 9,
    0, 3, 41, 0, 3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_date_rx = {  /* 0x3d3200 */
   0, 1, NULL, 0, 523,
   (char *)en_rx_date_rxprog
};
const cst_regex * const en_rx_date = &en_rx_date_rx;  /* 0x3d31fc */

/* BRANCH BOL OPEN1 BRANCH OPEN2 BRANCH OPEN3 BRANCH ANYOF(123456789) BRANCH ANYOF(0123456789) BRANCH NOTHING BRANCH ANYOF(0123456789) BRANCH NOTHING OPEN4 BRANCH EXACTLY(,) ANYOF(0123456789) ANYOF(0123456789) ANYOF(0123456789) CLOSE4 BRANCH BACK BRANCH NOTHING BRANCH ANYOF(123456789) STAR ANYOF(0123456789) BRANCH EXACTLY(0) CLOSE3 OPEN5 BRANCH EXACTLY(th) BRANCH EXACTLY(TH) BRANCH EXACTLY(Th) CLOSE5 BRANCH BRANCH OPEN6 BRANCH ANYOF(23456789) BRANCH ANYOF(123456789) STAR ANYOF(0123456789) ANYOF(023456789) BRANCH OPEN7 BRANCH ANYOF(123456789) BRANCH ANYOF(0123456789) BRANCH NOTHING BRANCH ANYOF(0123456789) BRANCH NOTHING OPEN8 BRANCH EXACTLY(,) ANYOF(0123456789) ANYOF(0123456789) ANYOF(0123456789) CLOSE8 BRANCH BACK BRANCH NOTHING BRANCH ANYOF(123456789) BRANCH ANYOF(0123456789) BRANCH NOTHING BRANCH ANYOF(0123456789) BRANCH NOTHING CLOSE7 EXACTLY(,) ANYOF(0123456789) ANYOF(023456789) CLOSE6 BRANCH NOTHING OPEN9 BRANCH EXACTLY(1st) BRANCH EXACTLY(1ST) BRANCH EXACTLY(1St) BRANCH EXACTLY(2nd) BRANCH EXACTLY(2ND) BRANCH EXACTLY(2Nd) BRANCH EXACTLY(3rd) BRANCH EXACTLY(3RD) BRANCH EXACTLY(3Rd) CLOSE9 CLOSE2 CLOSE1 BRANCH CLOSE-10 BRANCH BRANCH EXACTLY(') BRANCH NOTHING ANYOF(Ss) CLOSE10 BRANCH NOTHING EOL END */
static const unsigned char en_rx_number_rxprog[] = {  /* 0x38d888 */
    156, 6, 2, 171, 1, 0, 3, 21, 0, 3, 6, 2, 118, 22, 0, 3,
    6, 0, 213, 23, 0, 3, 6, 0, 130, 4, 0, 13, 49, 50, 51, 52,
    53, 54, 55, 56, 57, 0, 6, 0, 17, 4, 0, 17, 48, 49, 50, 51,
    52, 53, 54, 55, 56, 57, 0, 6, 0, 3, 9, 0, 3, 6, 0, 17,
    4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0,
    3, 9, 0, 3, 24, 0, 3, 6, 0, 50, 8, 0, 5, 44, 0, 4,
    0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 4, 0, 14,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 4, 0, 14, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 0, 44, 0, 3, 6, 0, 6, 7,
    0, 59, 6, 0, 3, 9, 0, 44, 6, 0, 33, 4, 0, 13, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 0, 10, 0, 25, 4, 0, 0, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 8, 8, 0, 5, 48,
    0, 43, 0, 3, 25, 0, 3, 6, 0, 9, 8, 0, 24, 116, 104, 0,
    6, 0, 9, 8, 0, 15, 84, 72, 0, 6, 0, 9, 8, 0, 6, 84,
    104, 0, 45, 1, 155, 6, 1, 152, 6, 1, 47, 26, 0, 3, 6, 0,
    15, 4, 1, 35, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 46,
    4, 0, 13, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 10, 0, 17,
    4, 0, 0, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 4, 0,
    246, 48, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 233, 27, 0,
    3, 6, 0, 130, 4, 0, 13, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    0, 6, 0, 17, 4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55, 56,
    57, 0, 6, 0, 3, 9, 0, 3, 6, 0, 17, 4, 0, 17, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 3, 9, 0, 3, 28,
    0, 3, 6, 0, 50, 8, 0, 5, 44, 0, 4, 0, 14, 48, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 0, 4, 0, 14, 48, 49, 50, 51, 52,
    53, 54, 55, 56, 57, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54,
    55, 56, 57, 0, 48, 0, 3, 6, 0, 6, 7, 0, 59, 6, 0, 3,
    9, 0, 65, 6, 0, 62, 4, 0, 13, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 0, 6, 0, 17, 4, 0, 17, 48, 49, 50, 51, 52, 53, 54,
    55, 56, 57, 0, 6, 0, 3, 9, 0, 3, 6, 0, 17, 4, 0, 17,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 3, 9, 0,
    3, 47, 0, 3, 8, 0, 5, 44, 0, 4, 0, 14, 48, 49, 50, 51,
    52, 53, 54, 55, 56, 57, 0, 4, 0, 13, 48, 50, 51, 52, 53, 54,
    55, 56, 57, 0, 46, 0, 6, 6, 0, 3, 9, 0, 3, 29, 0, 3,
    6, 0, 10, 8, 0, 87, 49, 115, 116, 0, 6, 0, 10, 8, 0, 77,
    49, 83, 84, 0, 6, 0, 10, 8, 0, 67, 49, 83, 116, 0, 6, 0,
    10, 8, 0, 57, 50, 110, 100, 0, 6, 0, 10, 8, 0, 47, 50, 78,
    68, 0, 6, 0, 10, 8, 0, 37, 50, 78, 100, 0, 6, 0, 10, 8,
    0, 27, 51, 114, 100, 0, 6, 0, 10, 8, 0, 17, 51, 82, 68, 0,
    6, 0, 10, 8, 0, 7, 51, 82, 100, 0, 49, 0, 3, 42, 0, 3,
    41, 0, 3, 6, 0, 32, 30, 0, 3, 6, 0, 23, 6, 0, 8, 8,
    0, 8, 39, 0, 6, 0, 3, 9, 0, 3, 4, 0, 6, 83, 115, 0,
    50, 0, 6, 6, 0, 3, 9, 0, 3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_number_rx = {  /* 0x3d3218 */
   0, 1, NULL, 0, 687,
   (char *)en_rx_number_rxprog
};
const cst_regex * const en_rx_number = &en_rx_number_rx;  /* 0x3d3214 */

/* BRANCH BOL OPEN1 BRANCH ANYOF(123456789) BRANCH ANYOF(0123456789) BRANCH NOTHING BRANCH ANYOF(0123456789) BRANCH NOTHING OPEN2 BRANCH EXACTLY(,) ANYOF(0123456789) ANYOF(0123456789) ANYOF(0123456789) CLOSE2 BRANCH BACK BRANCH NOTHING BRANCH ANYOF(123456789) STAR ANYOF(0123456789) BRANCH EXACTLY(0) CLOSE1 EXACTLY(s) EOL END */
static const unsigned char en_rx_commaint_rxprog[] = {  /* 0x38db38 */
    156, 6, 0, 191, 1, 0, 3, 21, 0, 3, 6, 0, 130, 4, 0, 13,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 17, 4, 0, 17,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 3, 9, 0,
    3, 6, 0, 17, 4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55, 56,
    57, 0, 6, 0, 3, 9, 0, 3, 22, 0, 3, 6, 0, 50, 8, 0,
    5, 44, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 4,
    0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 42, 0, 3,
    6, 0, 6, 7, 0, 59, 6, 0, 3, 9, 0, 44, 6, 0, 33, 4,
    0, 13, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 10, 0, 25, 4,
    0, 0, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 8,
    8, 0, 5, 48, 0, 41, 0, 3, 8, 0, 5, 115, 0, 2, 0, 3,
    0, 0, 0,
};
static const cst_regex en_rx_commaint_rx = {  /* 0x3d3230 */
   0, 1, NULL, 0, 195,
   (char *)en_rx_commaint_rxprog
};
const cst_regex * const en_rx_commaint = &en_rx_commaint_rx;  /* 0x3d322c */

/* BRANCH BOL PLUS ANYOF(0123456789) BRANCH OPEN1 BRANCH EXACTLY(TH) BRANCH EXACTLY(th) BRANCH EXACTLY(ND) BRANCH EXACTLY(nd) BRANCH EXACTLY(RD) BRANCH EXACTLY(rd) CLOSE1 BRANCH NOTHING EOL END */
static const unsigned char en_rx_ordinal_rxprog[] = {  /* 0x38dbfc */
    156, 6, 0, 95, 1, 0, 3, 11, 0, 17, 4, 0, 0, 48, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 63, 21, 0, 3, 6, 0,
    9, 8, 0, 51, 84, 72, 0, 6, 0, 9, 8, 0, 42, 116, 104, 0,
    6, 0, 9, 8, 0, 33, 78, 68, 0, 6, 0, 9, 8, 0, 24, 110,
    100, 0, 6, 0, 9, 8, 0, 15, 82, 68, 0, 6, 0, 9, 8, 0,
    6, 114, 100, 0, 41, 0, 6, 6, 0, 3, 9, 0, 3, 2, 0, 3,
    0, 0, 0,
};
static const cst_regex en_rx_ordinal_rx = {  /* 0x3d3248 */
   0, 1, NULL, 0, 99,
   (char *)en_rx_ordinal_rxprog
};
const cst_regex * const en_rx_ordinal = &en_rx_ordinal_rx;  /* 0x3d3244 */

/* BRANCH BOL BRANCH ANYOF(-+±) BRANCH NOTHING OPEN1 BRANCH ANYOF($Â£Â¥) BRANCH EXACTLY(Rs) BRANCH EXACTLY(kr) BRANCH EXACTLY(AUD) BRANCH EXACTLY(BBD) BRANCH EXACTLY(BGN) BRANCH EXACTLY(BMD) BRANCH EXACTLY(BRL) BRANCH EXACTLY(BSD) BRANCH EXACTLY(BZD) BRANCH EXACTLY(CAD) BRANCH EXACTLY(CHF) BRANCH EXACTLY(CLP) BRANCH EXACTLY(CNY) BRANCH EXACTLY(COP) BRANCH EXACTLY(CUC) BRANCH EXACTLY(CUP) BRANCH EXACTLY(CZK) BRANCH EXACTLY(DEM) BRANCH EXACTLY(DKK) BRANCH EXACTLY(DOP) BRANCH EXACTLY(EEK) BRANCH EXACTLY(EUR) BRANCH EXACTLY(FJD) BRANCH EXACTLY(FRF) BRANCH EXACTLY(GBP) BRANCH EXACTLY(GYD) BRANCH EXACTLY(HKD) BRANCH EXACTLY(HUF) BRANCH EXACTLY(IDR) BRANCH EXACTLY(INR) BRANCH EXACTLY(ISK) BRANCH EXACTLY(JMD) BRANCH EXACTLY(JPY) BRANCH EXACTLY(KRW) BRANCH EXACTLY(KYD) BRANCH EXACTLY(LTL) BRANCH EXACTLY(LVL) BRANCH EXACTLY(MXN) BRANCH EXACTLY(NAD) BRANCH EXACTLY(NIO) BRANCH EXACTLY(NOK) BRANCH EXACTLY(NZD) BRANCH EXACTLY(PKR) BRANCH EXACTLY(PLN) BRANCH EXACTLY(RON) BRANCH EXACTLY(RUB) BRANCH EXACTLY(SEK) BRANCH EXACTLY(SBD) BRANCH EXACTLY(SGD) BRANCH EXACTLY(TRY) BRANCH EXACTLY(TTD) BRANCH EXACTLY(TWD) BRANCH EXACTLY(UAH) BRANCH EXACTLY(USD) BRANCH EXACTLY(UYU) BRANCH EXACTLY(ZAR) BRANCH EXACTLY(ZWL) BRANCH EXACTLY(XCD) CLOSE1 BRANCH ANYOF(-+±) BRANCH NOTHING OPEN2 BRANCH ANYOF(123456789) BRANCH ANYOF(0123456789) BRANCH NOTHING BRANCH ANYOF(0123456789) BRANCH NOTHING OPEN3 BRANCH EXACTLY(,) ANYOF(0123456789) ANYOF(0123456789) ANYOF(0123456789) CLOSE3 BRANCH BACK BRANCH NOTHING BRANCH ANYOF(123456789) STAR ANYOF(0123456789) BRANCH EXACTLY(0) CLOSE2 BRANCH OPEN4 BRANCH EXACTLY(.) PLUS ANYOF(0123456789) CLOSE4 BRANCH NOTHING BRANCH ANYOF(MBTKk) BRANCH NOTHING EOL END */
static const unsigned char en_rx_money_rxprog[] = {  /* 0x38dc60 */
    156, 6, 3, 104, 1, 0, 3, 6, 0, 10, 4, 0, 10, 45, 43, 177,
    0, 6, 0, 3, 9, 0, 3, 21, 0, 3, 6, 0, 12, 4, 2, 75,
    36, 194, 163, 194, 165, 0, 6, 0, 9, 8, 2, 63, 82, 115, 0, 6,
    0, 9, 8, 2, 54, 107, 114, 0, 6, 0, 10, 8, 2, 45, 65, 85,
    68, 0, 6, 0, 10, 8, 2, 35, 66, 66, 68, 0, 6, 0, 10, 8,
    2, 25, 66, 71, 78, 0, 6, 0, 10, 8, 2, 15, 66, 77, 68, 0,
    6, 0, 10, 8, 2, 5, 66, 82, 76, 0, 6, 0, 10, 8, 1, 251,
    66, 83, 68, 0, 6, 0, 10, 8, 1, 241, 66, 90, 68, 0, 6, 0,
    10, 8, 1, 231, 67, 65, 68, 0, 6, 0, 10, 8, 1, 221, 67, 72,
    70, 0, 6, 0, 10, 8, 1, 211, 67, 76, 80, 0, 6, 0, 10, 8,
    1, 201, 67, 78, 89, 0, 6, 0, 10, 8, 1, 191, 67, 79, 80, 0,
    6, 0, 10, 8, 1, 181, 67, 85, 67, 0, 6, 0, 10, 8, 1, 171,
    67, 85, 80, 0, 6, 0, 10, 8, 1, 161, 67, 90, 75, 0, 6, 0,
    10, 8, 1, 151, 68, 69, 77, 0, 6, 0, 10, 8, 1, 141, 68, 75,
    75, 0, 6, 0, 10, 8, 1, 131, 68, 79, 80, 0, 6, 0, 10, 8,
    1, 121, 69, 69, 75, 0, 6, 0, 10, 8, 1, 111, 69, 85, 82, 0,
    6, 0, 10, 8, 1, 101, 70, 74, 68, 0, 6, 0, 10, 8, 1, 91,
    70, 82, 70, 0, 6, 0, 10, 8, 1, 81, 71, 66, 80, 0, 6, 0,
    10, 8, 1, 71, 71, 89, 68, 0, 6, 0, 10, 8, 1, 61, 72, 75,
    68, 0, 6, 0, 10, 8, 1, 51, 72, 85, 70, 0, 6, 0, 10, 8,
    1, 41, 73, 68, 82, 0, 6, 0, 10, 8, 1, 31, 73, 78, 82, 0,
    6, 0, 10, 8, 1, 21, 73, 83, 75, 0, 6, 0, 10, 8, 1, 11,
    74, 77, 68, 0, 6, 0, 10, 8, 1, 1, 74, 80, 89, 0, 6, 0,
    10, 8, 0, 247, 75, 82, 87, 0, 6, 0, 10, 8, 0, 237, 75, 89,
    68, 0, 6, 0, 10, 8, 0, 227, 76, 84, 76, 0, 6, 0, 10, 8,
    0, 217, 76, 86, 76, 0, 6, 0, 10, 8, 0, 207, 77, 88, 78, 0,
    6, 0, 10, 8, 0, 197, 78, 65, 68, 0, 6, 0, 10, 8, 0, 187,
    78, 73, 79, 0, 6, 0, 10, 8, 0, 177, 78, 79, 75, 0, 6, 0,
    10, 8, 0, 167, 78, 90, 68, 0, 6, 0, 10, 8, 0, 157, 80, 75,
    82, 0, 6, 0, 10, 8, 0, 147, 80, 76, 78, 0, 6, 0, 10, 8,
    0, 137, 82, 79, 78, 0, 6, 0, 10, 8, 0, 127, 82, 85, 66, 0,
    6, 0, 10, 8, 0, 117, 83, 69, 75, 0, 6, 0, 10, 8, 0, 107,
    83, 66, 68, 0, 6, 0, 10, 8, 0, 97, 83, 71, 68, 0, 6, 0,
    10, 8, 0, 87, 84, 82, 89, 0, 6, 0, 10, 8, 0, 77, 84, 84,
    68, 0, 6, 0, 10, 8, 0, 67, 84, 87, 68, 0, 6, 0, 10, 8,
    0, 57, 85, 65, 72, 0, 6, 0, 10, 8, 0, 47, 85, 83, 68, 0,
    6, 0, 10, 8, 0, 37, 85, 89, 85, 0, 6, 0, 10, 8, 0, 27,
    90, 65, 82, 0, 6, 0, 10, 8, 0, 17, 90, 87, 76, 0, 6, 0,
    10, 8, 0, 7, 88, 67, 68, 0, 41, 0, 3, 6, 0, 10, 4, 0,
    10, 45, 43, 177, 0, 6, 0, 3, 9, 0, 3, 22, 0, 3, 6, 0,
    130, 4, 0, 13, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0,
    17, 4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6,
    0, 3, 9, 0, 3, 6, 0, 17, 4, 0, 17, 48, 49, 50, 51, 52,
    53, 54, 55, 56, 57, 0, 6, 0, 3, 9, 0, 3, 23, 0, 3, 6,
    0, 50, 8, 0, 5, 44, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53,
    54, 55, 56, 57, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    0, 43, 0, 3, 6, 0, 6, 7, 0, 59, 6, 0, 3, 9, 0, 44,
    6, 0, 33, 4, 0, 13, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0,
    10, 0, 25, 4, 0, 0, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    0, 6, 0, 8, 8, 0, 5, 48, 0, 42, 0, 3, 6, 0, 34, 24,
    0, 3, 6, 0, 25, 8, 0, 5, 46, 0, 11, 0, 17, 4, 0, 0,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 44, 0, 6, 6, 0,
    3, 9, 0, 3, 6, 0, 12, 4, 0, 12, 77, 66, 84, 75, 107, 0,
    6, 0, 3, 9, 0, 3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_money_rx = {  /* 0x3d3260 */
   0, 1, NULL, 0, 876,
   (char *)en_rx_money_rxprog
};
const cst_regex * const en_rx_money = &en_rx_money_rx;  /* 0x3d325c */

/* BRANCH BOL OPEN1 BRANCH ANYOF(123456789) BRANCH ANYOF(0123456789) BRANCH NOTHING BRANCH ANYOF(0123456789) BRANCH NOTHING OPEN2 BRANCH EXACTLY(,) ANYOF(0123456789) ANYOF(0123456789) ANYOF(0123456789) CLOSE2 BRANCH BACK BRANCH NOTHING BRANCH ANYOF(123456789) STAR ANYOF(0123456789) BRANCH EXACTLY(0) CLOSE1 EOL END */
static const unsigned char en_rx_decimal_rxprog[] = {  /* 0x38dfcc */
    156, 6, 0, 186, 1, 0, 3, 21, 0, 3, 6, 0, 130, 4, 0, 13,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 17, 4, 0, 17,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 3, 9, 0,
    3, 6, 0, 17, 4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55, 56,
    57, 0, 6, 0, 3, 9, 0, 3, 22, 0, 3, 6, 0, 50, 8, 0,
    5, 44, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 4,
    0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 42, 0, 3,
    6, 0, 6, 7, 0, 59, 6, 0, 3, 9, 0, 44, 6, 0, 33, 4,
    0, 13, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 10, 0, 25, 4,
    0, 0, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 8,
    8, 0, 5, 48, 0, 41, 0, 3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_decimal_rx = {  /* 0x3d3278 */
   0, 1, NULL, 0, 190,
   (char *)en_rx_decimal_rxprog
};
const cst_regex * const en_rx_decimal = &en_rx_decimal_rx;  /* 0x3d3274 */

/* BRANCH BOL OPEN1 BRANCH STAR ANY EXACTLY(illion) BRANCH STAR ANY EXACTLY(ILLION) BRANCH EXACTLY(thousand) BRANCH EXACTLY(Thousand) BRANCH EXACTLY(THOUSAND) CLOSE1 EOL END */
static const unsigned char en_rx_illion_rxprog[] = {  /* 0x38e08c */
    156, 6, 0, 98, 1, 0, 3, 21, 0, 3, 6, 0, 19, 10, 0, 6,
    3, 0, 0, 8, 0, 74, 105, 108, 108, 105, 111, 110, 0, 6, 0, 19,
    10, 0, 6, 3, 0, 0, 8, 0, 55, 73, 76, 76, 73, 79, 78, 0,
    6, 0, 15, 8, 0, 42, 116, 104, 111, 117, 115, 97, 110, 100, 0, 6,
    0, 15, 8, 0, 27, 84, 104, 111, 117, 115, 97, 110, 100, 0, 6, 0,
    15, 8, 0, 12, 84, 72, 79, 85, 83, 65, 78, 68, 0, 41, 0, 3,
    2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_illion_rx = {  /* 0x3d3290 */
   0, 1, NULL, 0, 102,
   (char *)en_rx_illion_rxprog
};
const cst_regex * const en_rx_illion = &en_rx_illion_rx;  /* 0x3d328c */

/* BRANCH BOL OPEN1 BRANCH ANYOF(ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz) EXACTLY(-) CLOSE1 BRANCH BACK BRANCH NOTHING ANYOF(ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz) BRANCH OPEN2 BRANCH EXACTLY(') PLUS ANYOF(abcdefghijklmnopqrstuvwxyzàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿßABCDEFGHIJKLMNOPQRSTUVWXYZÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞ) CLOSE2 BRANCH NOTHING EOL END */
static const unsigned char en_rx_spelled_rxprog[] = {  /* 0x38e0f4 */
    156, 6, 1, 35, 1, 0, 3, 21, 0, 3, 6, 0, 64, 4, 0, 56,
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 97, 98, 99, 100, 101, 102,
    103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118,
    119, 120, 121, 122, 0, 8, 0, 5, 45, 0, 41, 0, 3, 6, 0, 6,
    7, 0, 73, 6, 0, 3, 9, 0, 3, 4, 0, 56, 65, 66, 67, 68,
    69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
    85, 86, 87, 88, 89, 90, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106,
    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
    0, 6, 0, 138, 22, 0, 3, 6, 0, 129, 8, 0, 5, 39, 0, 11,
    0, 121, 4, 0, 0, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
    108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
    241, 242, 243, 244, 245, 246, 248, 249, 250, 251, 252, 253, 254, 255, 223, 65,
    66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
    82, 83, 84, 85, 86, 87, 88, 89, 90, 192, 193, 194, 195, 196, 197, 198,
    199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214,
    216, 217, 218, 219, 220, 221, 222, 0, 42, 0, 6, 6, 0, 3, 9, 0,
    3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_spelled_rx = {  /* 0x3d32a8 */
   0, 1, NULL, 0, 295,
   (char *)en_rx_spelled_rxprog
};
const cst_regex * const en_rx_spelled = &en_rx_spelled_rx;  /* 0x3d32a4 */

/* BRANCH BOL STAR ANY ANYOF(aeiouyàáâãäåæèéêëìíîïòóôõöøùúûüýÿAEIOUYÀÁÂÃÄÅÆÈÉÊËÌÍÎÏÒÓÔÕÖØÙÚÛÜÝ) STAR ANY EOL END */
static const unsigned char en_rx_has_vowel_rxprog[] = {  /* 0x38e21c */
    156, 6, 0, 90, 1, 0, 3, 10, 0, 6, 3, 0, 0, 4, 0, 69,
    97, 101, 105, 111, 117, 121, 224, 225, 226, 227, 228, 229, 230, 232, 233, 234,
    235, 236, 237, 238, 239, 242, 243, 244, 245, 246, 248, 249, 250, 251, 252, 253,
    255, 65, 69, 73, 79, 85, 89, 192, 193, 194, 195, 196, 197, 198, 200, 201,
    202, 203, 204, 205, 206, 207, 210, 211, 212, 213, 214, 216, 217, 218, 219, 220,
    221, 0, 10, 0, 6, 3, 0, 0, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_has_vowel_rx = {  /* 0x3d32c0 */
   0, 1, NULL, 0, 94,
   (char *)en_rx_has_vowel_rxprog
};
const cst_regex * const en_rx_has_vowel = &en_rx_has_vowel_rx;  /* 0x3d32bc */

/* BRANCH BOL BRANCH ANYOF(-+±) BRANCH NOTHING OPEN1 BRANCH ANYOF(123456789) BRANCH ANYOF(0123456789) BRANCH NOTHING BRANCH ANYOF(0123456789) BRANCH NOTHING OPEN2 BRANCH EXACTLY(,) ANYOF(0123456789) ANYOF(0123456789) ANYOF(0123456789) CLOSE2 BRANCH BACK BRANCH NOTHING BRANCH ANYOF(123456789) STAR ANYOF(0123456789) BRANCH EXACTLY(0) CLOSE1 BRANCH OPEN3 BRANCH EXACTLY(.) PLUS ANYOF(0123456789) CLOSE3 BRANCH NOTHING BRANCH OPEN4 BRANCH ANYOF(eE) BRANCH ANYOF(-+±) BRANCH NOTHING PLUS ANYOF(0123456789) CLOSE4 BRANCH NOTHING EOL END */
static const unsigned char en_rx_signed_number_rxprog[] = {  /* 0x38e27c */
    156, 6, 1, 43, 1, 0, 3, 6, 0, 10, 4, 0, 10, 45, 43, 177,
    0, 6, 0, 3, 9, 0, 3, 21, 0, 3, 6, 0, 130, 4, 0, 13,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 17, 4, 0, 17,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 3, 9, 0,
    3, 6, 0, 17, 4, 0, 17, 48, 49, 50, 51, 52, 53, 54, 55, 56,
    57, 0, 6, 0, 3, 9, 0, 3, 22, 0, 3, 6, 0, 50, 8, 0,
    5, 44, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 4,
    0, 14, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 42, 0, 3,
    6, 0, 6, 7, 0, 59, 6, 0, 3, 9, 0, 44, 6, 0, 33, 4,
    0, 13, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 10, 0, 25, 4,
    0, 0, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 8,
    8, 0, 5, 48, 0, 41, 0, 3, 6, 0, 34, 23, 0, 3, 6, 0,
    25, 8, 0, 5, 46, 0, 11, 0, 17, 4, 0, 0, 48, 49, 50, 51,
    52, 53, 54, 55, 56, 57, 0, 43, 0, 6, 6, 0, 3, 9, 0, 3,
    6, 0, 51, 24, 0, 3, 6, 0, 42, 4, 0, 6, 101, 69, 0, 6,
    0, 10, 4, 0, 10, 45, 43, 177, 0, 6, 0, 3, 9, 0, 3, 11,
    0, 17, 4, 0, 0, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0,
    44, 0, 6, 6, 0, 3, 9, 0, 3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_signed_number_rx = {  /* 0x3d32d8 */
   0, 1, NULL, 0, 303,
   (char *)en_rx_signed_number_rxprog
};
const cst_regex * const en_rx_signed_number = &en_rx_signed_number_rx;  /* 0x3d32d4 */

/* BRANCH BOL OPEN1 BRANCH ANYOF(ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz) EXACTLY(.) CLOSE1 BRANCH BACK BRANCH NOTHING ANYOF(ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz) BRANCH EXACTLY(.) BRANCH NOTHING EOL END */
static const unsigned char en_rx_dotted_letters_rxprog[] = {  /* 0x38e3ac */
    156, 6, 0, 161, 1, 0, 3, 21, 0, 3, 6, 0, 64, 4, 0, 56,
    65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 97, 98, 99, 100, 101, 102,
    103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118,
    119, 120, 121, 122, 0, 8, 0, 5, 46, 0, 41, 0, 3, 6, 0, 6,
    7, 0, 73, 6, 0, 3, 9, 0, 3, 4, 0, 56, 65, 66, 67, 68,
    69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
    85, 86, 87, 88, 89, 90, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106,
    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
    0, 6, 0, 8, 8, 0, 8, 46, 0, 6, 0, 3, 9, 0, 3, 2,
    0, 3, 0, 0, 0,
};
static const cst_regex en_rx_dotted_letters_rx = {  /* 0x3d32f0 */
   0, 1, NULL, 0, 165,
   (char *)en_rx_dotted_letters_rxprog
};
const cst_regex * const en_rx_dotted_letters = &en_rx_dotted_letters_rx;  /* 0x3d32ec */

/* BRANCH BOL OPEN1 BRANCH BRANCH EXACTLY(0) BRANCH NOTHING ANYOF(123456789) BRANCH EXACTLY(1) ANYOF(012) BRANCH EXACTLY(jan) BRANCH EXACTLY(january) BRANCH EXACTLY(Jan) BRANCH EXACTLY(January) BRANCH EXACTLY(JAN) BRANCH EXACTLY(JANUARY) BRANCH EXACTLY(feb) BRANCH EXACTLY(february) BRANCH EXACTLY(Feb) BRANCH EXACTLY(February) BRANCH EXACTLY(FEB) BRANCH EXACTLY(FEBRUARY) BRANCH EXACTLY(mar) BRANCH EXACTLY(march) BRANCH EXACTLY(Mar) BRANCH EXACTLY(March) BRANCH EXACTLY(MAR) BRANCH EXACTLY(MARCH) BRANCH EXACTLY(apr) BRANCH EXACTLY(april) BRANCH EXACTLY(Apr) BRANCH EXACTLY(April) BRANCH EXACTLY(APR) BRANCH EXACTLY(APRIL) BRANCH EXACTLY(may) BRANCH EXACTLY(May) BRANCH EXACTLY(MAY) BRANCH EXACTLY(jun) BRANCH EXACTLY(june) BRANCH EXACTLY(Jun) BRANCH EXACTLY(June) BRANCH EXACTLY(JUN) BRANCH EXACTLY(JUNE) BRANCH EXACTLY(jul) BRANCH EXACTLY(july) BRANCH EXACTLY(Jul) BRANCH EXACTLY(July) BRANCH EXACTLY(JUL) BRANCH EXACTLY(JULY) BRANCH EXACTLY(aug) BRANCH EXACTLY(august) BRANCH EXACTLY(Aug) BRANCH EXACTLY(August) BRANCH EXACTLY(AUG) BRANCH EXACTLY(AUGUST) BRANCH EXACTLY(sep) BRANCH EXACTLY(sept) BRANCH EXACTLY(september) BRANCH EXACTLY(Sep) BRANCH EXACTLY(Sept) BRANCH EXACTLY(September) BRANCH EXACTLY(SEP) BRANCH EXACTLY(SEPT) BRANCH EXACTLY(SEPTEMBER) BRANCH EXACTLY(oct) BRANCH EXACTLY(october) BRANCH EXACTLY(Oct) BRANCH EXACTLY(October) BRANCH EXACTLY(OCT) BRANCH EXACTLY(OCTOBER) BRANCH EXACTLY(nov) BRANCH EXACTLY(november) BRANCH EXACTLY(Nov) BRANCH EXACTLY(November) BRANCH EXACTLY(NOV) BRANCH EXACTLY(NOVEMBER) BRANCH EXACTLY(dec) BRANCH EXACTLY(december) BRANCH EXACTLY(Dec) BRANCH EXACTLY(December) BRANCH EXACTLY(DEC) BRANCH EXACTLY(DECEMBER) CLOSE1 ANYOF(./-) OPEN2 BRANCH BRANCH EXACTLY(0) BRANCH NOTHING ANYOF(123456789) BRANCH ANYOF(12) ANYOF(0123456789) BRANCH EXACTLY(3) ANYOF(01) CLOSE2 ANYOF(./-) BRANCH OPEN3 BRANCH ANYOF(12) ANYOF(0123456789) CLOSE3 BRANCH NOTHING ANYOF(0123456789) ANYOF(0123456789) EOL END */
static const unsigned char en_rx_date_month_day_rxprog[] = {  /* 0x38e454 */
    156, 6, 4, 26, 1, 0, 3, 21, 0, 3, 6, 0, 30, 6, 0, 8,
    8, 0, 8, 48, 0, 6, 0, 3, 9, 0, 3, 4, 3, 97, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 15, 8, 0, 5, 49, 0,
    4, 3, 76, 48, 49, 50, 0, 6, 0, 10, 8, 3, 66, 106, 97, 110,
    0, 6, 0, 14, 8, 3, 56, 106, 97, 110, 117, 97, 114, 121, 0, 6,
    0, 10, 8, 3, 42, 74, 97, 110, 0, 6, 0, 14, 8, 3, 32, 74,
    97, 110, 117, 97, 114, 121, 0, 6, 0, 10, 8, 3, 18, 74, 65, 78,
    0, 6, 0, 14, 8, 3, 8, 74, 65, 78, 85, 65, 82, 89, 0, 6,
    0, 10, 8, 2, 250, 102, 101, 98, 0, 6, 0, 15, 8, 2, 240, 102,
    101, 98, 114, 117, 97, 114, 121, 0, 6, 0, 10, 8, 2, 225, 70, 101,
    98, 0, 6, 0, 15, 8, 2, 215, 70, 101, 98, 114, 117, 97, 114, 121,
    0, 6, 0, 10, 8, 2, 200, 70, 69, 66, 0, 6, 0, 15, 8, 2,
    190, 70, 69, 66, 82, 85, 65, 82, 89, 0, 6, 0, 10, 8, 2, 175,
    109, 97, 114, 0, 6, 0, 12, 8, 2, 165, 109, 97, 114, 99, 104, 0,
    6, 0, 10, 8, 2, 153, 77, 97, 114, 0, 6, 0, 12, 8, 2, 143,
    77, 97, 114, 99, 104, 0, 6, 0, 10, 8, 2, 131, 77, 65, 82, 0,
    6, 0, 12, 8, 2, 121, 77, 65, 82, 67, 72, 0, 6, 0, 10, 8,
    2, 109, 97, 112, 114, 0, 6, 0, 12, 8, 2, 99, 97, 112, 114, 105,
    108, 0, 6, 0, 10, 8, 2, 87, 65, 112, 114, 0, 6, 0, 12, 8,
    2, 77, 65, 112, 114, 105, 108, 0, 6, 0, 10, 8, 2, 65, 65, 80,
    82, 0, 6, 0, 12, 8, 2, 55, 65, 80, 82, 73, 76, 0, 6, 0,
    10, 8, 2, 43, 109, 97, 121, 0, 6, 0, 10, 8, 2, 33, 77, 97,
    121, 0, 6, 0, 10, 8, 2, 23, 77, 65, 89, 0, 6, 0, 10, 8,
    2, 13, 106, 117, 110, 0, 6, 0, 11, 8, 2, 3, 106, 117, 110, 101,
    0, 6, 0, 10, 8, 1, 248, 74, 117, 110, 0, 6, 0, 11, 8, 1,
    238, 74, 117, 110, 101, 0, 6, 0, 10, 8, 1, 227, 74, 85, 78, 0,
    6, 0, 11, 8, 1, 217, 74, 85, 78, 69, 0, 6, 0, 10, 8, 1,
    206, 106, 117, 108, 0, 6, 0, 11, 8, 1, 196, 106, 117, 108, 121, 0,
    6, 0, 10, 8, 1, 185, 74, 117, 108, 0, 6, 0, 11, 8, 1, 175,
    74, 117, 108, 121, 0, 6, 0, 10, 8, 1, 164, 74, 85, 76, 0, 6,
    0, 11, 8, 1, 154, 74, 85, 76, 89, 0, 6, 0, 10, 8, 1, 143,
    97, 117, 103, 0, 6, 0, 13, 8, 1, 133, 97, 117, 103, 117, 115, 116,
    0, 6, 0, 10, 8, 1, 120, 65, 117, 103, 0, 6, 0, 13, 8, 1,
    110, 65, 117, 103, 117, 115, 116, 0, 6, 0, 10, 8, 1, 97, 65, 85,
    71, 0, 6, 0, 13, 8, 1, 87, 65, 85, 71, 85, 83, 84, 0, 6,
    0, 10, 8, 1, 74, 115, 101, 112, 0, 6, 0, 11, 8, 1, 64, 115,
    101, 112, 116, 0, 6, 0, 16, 8, 1, 53, 115, 101, 112, 116, 101, 109,
    98, 101, 114, 0, 6, 0, 10, 8, 1, 37, 83, 101, 112, 0, 6, 0,
    11, 8, 1, 27, 83, 101, 112, 116, 0, 6, 0, 16, 8, 1, 16, 83,
    101, 112, 116, 101, 109, 98, 101, 114, 0, 6, 0, 10, 8, 1, 0, 83,
    69, 80, 0, 6, 0, 11, 8, 0, 246, 83, 69, 80, 84, 0, 6, 0,
    16, 8, 0, 235, 83, 69, 80, 84, 69, 77, 66, 69, 82, 0, 6, 0,
    10, 8, 0, 219, 111, 99, 116, 0, 6, 0, 14, 8, 0, 209, 111, 99,
    116, 111, 98, 101, 114, 0, 6, 0, 10, 8, 0, 195, 79, 99, 116, 0,
    6, 0, 14, 8, 0, 185, 79, 99, 116, 111, 98, 101, 114, 0, 6, 0,
    10, 8, 0, 171, 79, 67, 84, 0, 6, 0, 14, 8, 0, 161, 79, 67,
    84, 79, 66, 69, 82, 0, 6, 0, 10, 8, 0, 147, 110, 111, 118, 0,
    6, 0, 15, 8, 0, 137, 110, 111, 118, 101, 109, 98, 101, 114, 0, 6,
    0, 10, 8, 0, 122, 78, 111, 118, 0, 6, 0, 15, 8, 0, 112, 78,
    111, 118, 101, 109, 98, 101, 114, 0, 6, 0, 10, 8, 0, 97, 78, 79,
    86, 0, 6, 0, 15, 8, 0, 87, 78, 79, 86, 69, 77, 66, 69, 82,
    0, 6, 0, 10, 8, 0, 72, 100, 101, 99, 0, 6, 0, 15, 8, 0,
    62, 100, 101, 99, 101, 109, 98, 101, 114, 0, 6, 0, 10, 8, 0, 47,
    68, 101, 99, 0, 6, 0, 15, 8, 0, 37, 68, 101, 99, 101, 109, 98,
    101, 114, 0, 6, 0, 10, 8, 0, 22, 68, 69, 67, 0, 6, 0, 15,
    8, 0, 12, 68, 69, 67, 69, 77, 66, 69, 82, 0, 41, 0, 3, 4,
    0, 7, 46, 47, 45, 0, 22, 0, 3, 6, 0, 30, 6, 0, 8, 8,
    0, 8, 48, 0, 6, 0, 3, 9, 0, 3, 4, 0, 50, 49, 50, 51,
    52, 53, 54, 55, 56, 57, 0, 6, 0, 23, 4, 0, 6, 49, 50, 0,
    4, 0, 28, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0,
    14, 8, 0, 5, 51, 0, 4, 0, 6, 48, 49, 0, 42, 0, 3, 4,
    0, 7, 46, 47, 45, 0, 6, 0, 32, 23, 0, 3, 6, 0, 23, 4,
    0, 6, 49, 50, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 0, 43, 0, 6, 6, 0, 3, 9, 0, 3, 4, 0, 14, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 4, 0, 14, 48, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 0, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_date_month_day_rx = {  /* 0x3d3308 */
   0, 1, NULL, 0, 1054,
   (char *)en_rx_date_month_day_rxprog
};
const cst_regex * const en_rx_date_month_day = &en_rx_date_month_day_rx;  /* 0x3d3304 */

/* BRANCH BOL ANYOF(12) ANYOF(0123456789) ANYOF(0123456789) ANYOF(0123456789) ANYOF(./-) OPEN1 BRANCH BRANCH EXACTLY(0) BRANCH NOTHING ANYOF(123456789) BRANCH EXACTLY(1) ANYOF(012) BRANCH EXACTLY(jan) BRANCH EXACTLY(january) BRANCH EXACTLY(Jan) BRANCH EXACTLY(January) BRANCH EXACTLY(JAN) BRANCH EXACTLY(JANUARY) BRANCH EXACTLY(feb) BRANCH EXACTLY(february) BRANCH EXACTLY(Feb) BRANCH EXACTLY(February) BRANCH EXACTLY(FEB) BRANCH EXACTLY(FEBRUARY) BRANCH EXACTLY(mar) BRANCH EXACTLY(march) BRANCH EXACTLY(Mar) BRANCH EXACTLY(March) BRANCH EXACTLY(MAR) BRANCH EXACTLY(MARCH) BRANCH EXACTLY(apr) BRANCH EXACTLY(april) BRANCH EXACTLY(Apr) BRANCH EXACTLY(April) BRANCH EXACTLY(APR) BRANCH EXACTLY(APRIL) BRANCH EXACTLY(may) BRANCH EXACTLY(May) BRANCH EXACTLY(MAY) BRANCH EXACTLY(jun) BRANCH EXACTLY(june) BRANCH EXACTLY(Jun) BRANCH EXACTLY(June) BRANCH EXACTLY(JUN) BRANCH EXACTLY(JUNE) BRANCH EXACTLY(jul) BRANCH EXACTLY(july) BRANCH EXACTLY(Jul) BRANCH EXACTLY(July) BRANCH EXACTLY(JUL) BRANCH EXACTLY(JULY) BRANCH EXACTLY(aug) BRANCH EXACTLY(august) BRANCH EXACTLY(Aug) BRANCH EXACTLY(August) BRANCH EXACTLY(AUG) BRANCH EXACTLY(AUGUST) BRANCH EXACTLY(sep) BRANCH EXACTLY(sept) BRANCH EXACTLY(september) BRANCH EXACTLY(Sep) BRANCH EXACTLY(Sept) BRANCH EXACTLY(September) BRANCH EXACTLY(SEP) BRANCH EXACTLY(SEPT) BRANCH EXACTLY(SEPTEMBER) BRANCH EXACTLY(oct) BRANCH EXACTLY(october) BRANCH EXACTLY(Oct) BRANCH EXACTLY(October) BRANCH EXACTLY(OCT) BRANCH EXACTLY(OCTOBER) BRANCH EXACTLY(nov) BRANCH EXACTLY(november) BRANCH EXACTLY(Nov) BRANCH EXACTLY(November) BRANCH EXACTLY(NOV) BRANCH EXACTLY(NOVEMBER) BRANCH EXACTLY(dec) BRANCH EXACTLY(december) BRANCH EXACTLY(Dec) BRANCH EXACTLY(December) BRANCH EXACTLY(DEC) BRANCH EXACTLY(DECEMBER) CLOSE1 ANYOF(./-) OPEN2 BRANCH BRANCH EXACTLY(0) BRANCH NOTHING ANYOF(123456789) BRANCH ANYOF(12) ANYOF(0123456789) BRANCH EXACTLY(3) ANYOF(01) CLOSE2 EOL END */
static const unsigned char en_rx_date_ymd_rxprog[] = {  /* 0x38e874 */
    156, 6, 4, 8, 1, 0, 3, 4, 0, 6, 49, 50, 0, 4, 0, 14,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 4, 0, 14, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 0, 4, 0, 14, 48, 49, 50, 51,
    52, 53, 54, 55, 56, 57, 0, 4, 0, 7, 46, 47, 45, 0, 21, 0,
    3, 6, 0, 30, 6, 0, 8, 8, 0, 8, 48, 0, 6, 0, 3, 9,
    0, 3, 4, 3, 97, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6,
    0, 15, 8, 0, 5, 49, 0, 4, 3, 76, 48, 49, 50, 0, 6, 0,
    10, 8, 3, 66, 106, 97, 110, 0, 6, 0, 14, 8, 3, 56, 106, 97,
    110, 117, 97, 114, 121, 0, 6, 0, 10, 8, 3, 42, 74, 97, 110, 0,
    6, 0, 14, 8, 3, 32, 74, 97, 110, 117, 97, 114, 121, 0, 6, 0,
    10, 8, 3, 18, 74, 65, 78, 0, 6, 0, 14, 8, 3, 8, 74, 65,
    78, 85, 65, 82, 89, 0, 6, 0, 10, 8, 2, 250, 102, 101, 98, 0,
    6, 0, 15, 8, 2, 240, 102, 101, 98, 114, 117, 97, 114, 121, 0, 6,
    0, 10, 8, 2, 225, 70, 101, 98, 0, 6, 0, 15, 8, 2, 215, 70,
    101, 98, 114, 117, 97, 114, 121, 0, 6, 0, 10, 8, 2, 200, 70, 69,
    66, 0, 6, 0, 15, 8, 2, 190, 70, 69, 66, 82, 85, 65, 82, 89,
    0, 6, 0, 10, 8, 2, 175, 109, 97, 114, 0, 6, 0, 12, 8, 2,
    165, 109, 97, 114, 99, 104, 0, 6, 0, 10, 8, 2, 153, 77, 97, 114,
    0, 6, 0, 12, 8, 2, 143, 77, 97, 114, 99, 104, 0, 6, 0, 10,
    8, 2, 131, 77, 65, 82, 0, 6, 0, 12, 8, 2, 121, 77, 65, 82,
    67, 72, 0, 6, 0, 10, 8, 2, 109, 97, 112, 114, 0, 6, 0, 12,
    8, 2, 99, 97, 112, 114, 105, 108, 0, 6, 0, 10, 8, 2, 87, 65,
    112, 114, 0, 6, 0, 12, 8, 2, 77, 65, 112, 114, 105, 108, 0, 6,
    0, 10, 8, 2, 65, 65, 80, 82, 0, 6, 0, 12, 8, 2, 55, 65,
    80, 82, 73, 76, 0, 6, 0, 10, 8, 2, 43, 109, 97, 121, 0, 6,
    0, 10, 8, 2, 33, 77, 97, 121, 0, 6, 0, 10, 8, 2, 23, 77,
    65, 89, 0, 6, 0, 10, 8, 2, 13, 106, 117, 110, 0, 6, 0, 11,
    8, 2, 3, 106, 117, 110, 101, 0, 6, 0, 10, 8, 1, 248, 74, 117,
    110, 0, 6, 0, 11, 8, 1, 238, 74, 117, 110, 101, 0, 6, 0, 10,
    8, 1, 227, 74, 85, 78, 0, 6, 0, 11, 8, 1, 217, 74, 85, 78,
    69, 0, 6, 0, 10, 8, 1, 206, 106, 117, 108, 0, 6, 0, 11, 8,
    1, 196, 106, 117, 108, 121, 0, 6, 0, 10, 8, 1, 185, 74, 117, 108,
    0, 6, 0, 11, 8, 1, 175, 74, 117, 108, 121, 0, 6, 0, 10, 8,
    1, 164, 74, 85, 76, 0, 6, 0, 11, 8, 1, 154, 74, 85, 76, 89,
    0, 6, 0, 10, 8, 1, 143, 97, 117, 103, 0, 6, 0, 13, 8, 1,
    133, 97, 117, 103, 117, 115, 116, 0, 6, 0, 10, 8, 1, 120, 65, 117,
    103, 0, 6, 0, 13, 8, 1, 110, 65, 117, 103, 117, 115, 116, 0, 6,
    0, 10, 8, 1, 97, 65, 85, 71, 0, 6, 0, 13, 8, 1, 87, 65,
    85, 71, 85, 83, 84, 0, 6, 0, 10, 8, 1, 74, 115, 101, 112, 0,
    6, 0, 11, 8, 1, 64, 115, 101, 112, 116, 0, 6, 0, 16, 8, 1,
    53, 115, 101, 112, 116, 101, 109, 98, 101, 114, 0, 6, 0, 10, 8, 1,
    37, 83, 101, 112, 0, 6, 0, 11, 8, 1, 27, 83, 101, 112, 116, 0,
    6, 0, 16, 8, 1, 16, 83, 101, 112, 116, 101, 109, 98, 101, 114, 0,
    6, 0, 10, 8, 1, 0, 83, 69, 80, 0, 6, 0, 11, 8, 0, 246,
    83, 69, 80, 84, 0, 6, 0, 16, 8, 0, 235, 83, 69, 80, 84, 69,
    77, 66, 69, 82, 0, 6, 0, 10, 8, 0, 219, 111, 99, 116, 0, 6,
    0, 14, 8, 0, 209, 111, 99, 116, 111, 98, 101, 114, 0, 6, 0, 10,
    8, 0, 195, 79, 99, 116, 0, 6, 0, 14, 8, 0, 185, 79, 99, 116,
    111, 98, 101, 114, 0, 6, 0, 10, 8, 0, 171, 79, 67, 84, 0, 6,
    0, 14, 8, 0, 161, 79, 67, 84, 79, 66, 69, 82, 0, 6, 0, 10,
    8, 0, 147, 110, 111, 118, 0, 6, 0, 15, 8, 0, 137, 110, 111, 118,
    101, 109, 98, 101, 114, 0, 6, 0, 10, 8, 0, 122, 78, 111, 118, 0,
    6, 0, 15, 8, 0, 112, 78, 111, 118, 101, 109, 98, 101, 114, 0, 6,
    0, 10, 8, 0, 97, 78, 79, 86, 0, 6, 0, 15, 8, 0, 87, 78,
    79, 86, 69, 77, 66, 69, 82, 0, 6, 0, 10, 8, 0, 72, 100, 101,
    99, 0, 6, 0, 15, 8, 0, 62, 100, 101, 99, 101, 109, 98, 101, 114,
    0, 6, 0, 10, 8, 0, 47, 68, 101, 99, 0, 6, 0, 15, 8, 0,
    37, 68, 101, 99, 101, 109, 98, 101, 114, 0, 6, 0, 10, 8, 0, 22,
    68, 69, 67, 0, 6, 0, 15, 8, 0, 12, 68, 69, 67, 69, 77, 66,
    69, 82, 0, 41, 0, 3, 4, 0, 7, 46, 47, 45, 0, 22, 0, 3,
    6, 0, 30, 6, 0, 8, 8, 0, 8, 48, 0, 6, 0, 3, 9, 0,
    3, 4, 0, 50, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6, 0,
    23, 4, 0, 6, 49, 50, 0, 4, 0, 28, 48, 49, 50, 51, 52, 53,
    54, 55, 56, 57, 0, 6, 0, 14, 8, 0, 5, 51, 0, 4, 0, 6,
    48, 49, 0, 42, 0, 3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_date_ymd_rx = {  /* 0x3d3320 */
   0, 1, NULL, 0, 1036,
   (char *)en_rx_date_ymd_rxprog
};
const cst_regex * const en_rx_date_ymd = &en_rx_date_ymd_rx;  /* 0x3d331c */

/* BRANCH BOL OPEN1 BRANCH BRANCH EXACTLY(0) BRANCH NOTHING ANYOF(123456789) BRANCH ANYOF(12) ANYOF(0123456789) BRANCH EXACTLY(3) ANYOF(01) CLOSE1 ANYOF(./-) OPEN2 BRANCH BRANCH EXACTLY(0) BRANCH NOTHING ANYOF(123456789) BRANCH EXACTLY(1) ANYOF(012) BRANCH EXACTLY(jan) BRANCH EXACTLY(january) BRANCH EXACTLY(Jan) BRANCH EXACTLY(January) BRANCH EXACTLY(JAN) BRANCH EXACTLY(JANUARY) BRANCH EXACTLY(feb) BRANCH EXACTLY(february) BRANCH EXACTLY(Feb) BRANCH EXACTLY(February) BRANCH EXACTLY(FEB) BRANCH EXACTLY(FEBRUARY) BRANCH EXACTLY(mar) BRANCH EXACTLY(march) BRANCH EXACTLY(Mar) BRANCH EXACTLY(March) BRANCH EXACTLY(MAR) BRANCH EXACTLY(MARCH) BRANCH EXACTLY(apr) BRANCH EXACTLY(april) BRANCH EXACTLY(Apr) BRANCH EXACTLY(April) BRANCH EXACTLY(APR) BRANCH EXACTLY(APRIL) BRANCH EXACTLY(may) BRANCH EXACTLY(May) BRANCH EXACTLY(MAY) BRANCH EXACTLY(jun) BRANCH EXACTLY(june) BRANCH EXACTLY(Jun) BRANCH EXACTLY(June) BRANCH EXACTLY(JUN) BRANCH EXACTLY(JUNE) BRANCH EXACTLY(jul) BRANCH EXACTLY(july) BRANCH EXACTLY(Jul) BRANCH EXACTLY(July) BRANCH EXACTLY(JUL) BRANCH EXACTLY(JULY) BRANCH EXACTLY(aug) BRANCH EXACTLY(august) BRANCH EXACTLY(Aug) BRANCH EXACTLY(August) BRANCH EXACTLY(AUG) BRANCH EXACTLY(AUGUST) BRANCH EXACTLY(sep) BRANCH EXACTLY(sept) BRANCH EXACTLY(september) BRANCH EXACTLY(Sep) BRANCH EXACTLY(Sept) BRANCH EXACTLY(September) BRANCH EXACTLY(SEP) BRANCH EXACTLY(SEPT) BRANCH EXACTLY(SEPTEMBER) BRANCH EXACTLY(oct) BRANCH EXACTLY(october) BRANCH EXACTLY(Oct) BRANCH EXACTLY(October) BRANCH EXACTLY(OCT) BRANCH EXACTLY(OCTOBER) BRANCH EXACTLY(nov) BRANCH EXACTLY(november) BRANCH EXACTLY(Nov) BRANCH EXACTLY(November) BRANCH EXACTLY(NOV) BRANCH EXACTLY(NOVEMBER) BRANCH EXACTLY(dec) BRANCH EXACTLY(december) BRANCH EXACTLY(Dec) BRANCH EXACTLY(December) BRANCH EXACTLY(DEC) BRANCH EXACTLY(DECEMBER) CLOSE2 ANYOF(./-) BRANCH OPEN3 BRANCH ANYOF(12) ANYOF(0123456789) CLOSE3 BRANCH NOTHING ANYOF(0123456789) ANYOF(0123456789) EOL END */
static const unsigned char en_rx_date_dmy_rxprog[] = {  /* 0x38ec80 */
    156, 6, 4, 26, 1, 0, 3, 21, 0, 3, 6, 0, 30, 6, 0, 8,
    8, 0, 8, 48, 0, 6, 0, 3, 9, 0, 3, 4, 0, 50, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 23, 4, 0, 6, 49, 50,
    0, 4, 0, 28, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 6,
    0, 14, 8, 0, 5, 51, 0, 4, 0, 6, 48, 49, 0, 41, 0, 3,
    4, 0, 7, 46, 47, 45, 0, 22, 0, 3, 6, 0, 30, 6, 0, 8,
    8, 0, 8, 48, 0, 6, 0, 3, 9, 0, 3, 4, 3, 97, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 0, 6, 0, 15, 8, 0, 5, 49, 0,
    4, 3, 76, 48, 49, 50, 0, 6, 0, 10, 8, 3, 66, 106, 97, 110,
    0, 6, 0, 14, 8, 3, 56, 106, 97, 110, 117, 97, 114, 121, 0, 6,
    0, 10, 8, 3, 42, 74, 97, 110, 0, 6, 0, 14, 8, 3, 32, 74,
    97, 110, 117, 97, 114, 121, 0, 6, 0, 10, 8, 3, 18, 74, 65, 78,
    0, 6, 0, 14, 8, 3, 8, 74, 65, 78, 85, 65, 82, 89, 0, 6,
    0, 10, 8, 2, 250, 102, 101, 98, 0, 6, 0, 15, 8, 2, 240, 102,
    101, 98, 114, 117, 97, 114, 121, 0, 6, 0, 10, 8, 2, 225, 70, 101,
    98, 0, 6, 0, 15, 8, 2, 215, 70, 101, 98, 114, 117, 97, 114, 121,
    0, 6, 0, 10, 8, 2, 200, 70, 69, 66, 0, 6, 0, 15, 8, 2,
    190, 70, 69, 66, 82, 85, 65, 82, 89, 0, 6, 0, 10, 8, 2, 175,
    109, 97, 114, 0, 6, 0, 12, 8, 2, 165, 109, 97, 114, 99, 104, 0,
    6, 0, 10, 8, 2, 153, 77, 97, 114, 0, 6, 0, 12, 8, 2, 143,
    77, 97, 114, 99, 104, 0, 6, 0, 10, 8, 2, 131, 77, 65, 82, 0,
    6, 0, 12, 8, 2, 121, 77, 65, 82, 67, 72, 0, 6, 0, 10, 8,
    2, 109, 97, 112, 114, 0, 6, 0, 12, 8, 2, 99, 97, 112, 114, 105,
    108, 0, 6, 0, 10, 8, 2, 87, 65, 112, 114, 0, 6, 0, 12, 8,
    2, 77, 65, 112, 114, 105, 108, 0, 6, 0, 10, 8, 2, 65, 65, 80,
    82, 0, 6, 0, 12, 8, 2, 55, 65, 80, 82, 73, 76, 0, 6, 0,
    10, 8, 2, 43, 109, 97, 121, 0, 6, 0, 10, 8, 2, 33, 77, 97,
    121, 0, 6, 0, 10, 8, 2, 23, 77, 65, 89, 0, 6, 0, 10, 8,
    2, 13, 106, 117, 110, 0, 6, 0, 11, 8, 2, 3, 106, 117, 110, 101,
    0, 6, 0, 10, 8, 1, 248, 74, 117, 110, 0, 6, 0, 11, 8, 1,
    238, 74, 117, 110, 101, 0, 6, 0, 10, 8, 1, 227, 74, 85, 78, 0,
    6, 0, 11, 8, 1, 217, 74, 85, 78, 69, 0, 6, 0, 10, 8, 1,
    206, 106, 117, 108, 0, 6, 0, 11, 8, 1, 196, 106, 117, 108, 121, 0,
    6, 0, 10, 8, 1, 185, 74, 117, 108, 0, 6, 0, 11, 8, 1, 175,
    74, 117, 108, 121, 0, 6, 0, 10, 8, 1, 164, 74, 85, 76, 0, 6,
    0, 11, 8, 1, 154, 74, 85, 76, 89, 0, 6, 0, 10, 8, 1, 143,
    97, 117, 103, 0, 6, 0, 13, 8, 1, 133, 97, 117, 103, 117, 115, 116,
    0, 6, 0, 10, 8, 1, 120, 65, 117, 103, 0, 6, 0, 13, 8, 1,
    110, 65, 117, 103, 117, 115, 116, 0, 6, 0, 10, 8, 1, 97, 65, 85,
    71, 0, 6, 0, 13, 8, 1, 87, 65, 85, 71, 85, 83, 84, 0, 6,
    0, 10, 8, 1, 74, 115, 101, 112, 0, 6, 0, 11, 8, 1, 64, 115,
    101, 112, 116, 0, 6, 0, 16, 8, 1, 53, 115, 101, 112, 116, 101, 109,
    98, 101, 114, 0, 6, 0, 10, 8, 1, 37, 83, 101, 112, 0, 6, 0,
    11, 8, 1, 27, 83, 101, 112, 116, 0, 6, 0, 16, 8, 1, 16, 83,
    101, 112, 116, 101, 109, 98, 101, 114, 0, 6, 0, 10, 8, 1, 0, 83,
    69, 80, 0, 6, 0, 11, 8, 0, 246, 83, 69, 80, 84, 0, 6, 0,
    16, 8, 0, 235, 83, 69, 80, 84, 69, 77, 66, 69, 82, 0, 6, 0,
    10, 8, 0, 219, 111, 99, 116, 0, 6, 0, 14, 8, 0, 209, 111, 99,
    116, 111, 98, 101, 114, 0, 6, 0, 10, 8, 0, 195, 79, 99, 116, 0,
    6, 0, 14, 8, 0, 185, 79, 99, 116, 111, 98, 101, 114, 0, 6, 0,
    10, 8, 0, 171, 79, 67, 84, 0, 6, 0, 14, 8, 0, 161, 79, 67,
    84, 79, 66, 69, 82, 0, 6, 0, 10, 8, 0, 147, 110, 111, 118, 0,
    6, 0, 15, 8, 0, 137, 110, 111, 118, 101, 109, 98, 101, 114, 0, 6,
    0, 10, 8, 0, 122, 78, 111, 118, 0, 6, 0, 15, 8, 0, 112, 78,
    111, 118, 101, 109, 98, 101, 114, 0, 6, 0, 10, 8, 0, 97, 78, 79,
    86, 0, 6, 0, 15, 8, 0, 87, 78, 79, 86, 69, 77, 66, 69, 82,
    0, 6, 0, 10, 8, 0, 72, 100, 101, 99, 0, 6, 0, 15, 8, 0,
    62, 100, 101, 99, 101, 109, 98, 101, 114, 0, 6, 0, 10, 8, 0, 47,
    68, 101, 99, 0, 6, 0, 15, 8, 0, 37, 68, 101, 99, 101, 109, 98,
    101, 114, 0, 6, 0, 10, 8, 0, 22, 68, 69, 67, 0, 6, 0, 15,
    8, 0, 12, 68, 69, 67, 69, 77, 66, 69, 82, 0, 42, 0, 3, 4,
    0, 7, 46, 47, 45, 0, 6, 0, 32, 23, 0, 3, 6, 0, 23, 4,
    0, 6, 49, 50, 0, 4, 0, 14, 48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 0, 43, 0, 6, 6, 0, 3, 9, 0, 3, 4, 0, 14, 48,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 0, 4, 0, 14, 48, 49, 50,
    51, 52, 53, 54, 55, 56, 57, 0, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_date_dmy_rx = {  /* 0x3d3338 */
   0, 1, NULL, 0, 1054,
   (char *)en_rx_date_dmy_rxprog
};
const cst_regex * const en_rx_date_dmy = &en_rx_date_dmy_rx;  /* 0x3d3334 */

/* BRANCH BOL OPEN1 BRANCH EXACTLY(be) BRANCH EXACTLY(been) BRANCH EXACTLY(being) BRANCH EXACTLY(is) BRANCH EXACTLY(am) BRANCH EXACTLY(i'm) BRANCH STAR ANY EXACTLY('s) BRANCH EXACTLY(was) BRANCH EXACTLY(were) BRANCH EXACTLY(are) BRANCH STAR ANY EXACTLY('re) CLOSE1 EOL END */
static const unsigned char en_rx_be_rxprog[] = {  /* 0x38f0a0 */
    156, 6, 0, 137, 1, 0, 3, 21, 0, 3, 6, 0, 9, 8, 0, 119,
    98, 101, 0, 6, 0, 11, 8, 0, 110, 98, 101, 101, 110, 0, 6, 0,
    12, 8, 0, 99, 98, 101, 105, 110, 103, 0, 6, 0, 9, 8, 0, 87,
    105, 115, 0, 6, 0, 9, 8, 0, 78, 97, 109, 0, 6, 0, 10, 8,
    0, 69, 105, 39, 109, 0, 6, 0, 15, 10, 0, 6, 3, 0, 0, 8,
    0, 53, 39, 115, 0, 6, 0, 10, 8, 0, 44, 119, 97, 115, 0, 6,
    0, 11, 8, 0, 34, 119, 101, 114, 101, 0, 6, 0, 10, 8, 0, 23,
    97, 114, 101, 0, 6, 0, 16, 10, 0, 6, 3, 0, 0, 8, 0, 7,
    39, 114, 101, 0, 41, 0, 3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_be_rx = {  /* 0x3d3350 */
   0, 1, NULL, 0, 141,
   (char *)en_rx_be_rxprog
};
const cst_regex * const en_rx_be = &en_rx_be_rx;  /* 0x3d334c */

/* BRANCH BOL OPEN1 BRANCH EXACTLY(have) BRANCH EXACTLY(haven't) BRANCH EXACTLY(has) BRANCH EXACTLY(hasn't) BRANCH EXACTLY(had) BRANCH EXACTLY(hadn't) BRANCH STAR ANY EXACTLY('ve) CLOSE1 EOL END */
static const unsigned char en_rx_have_rxprog[] = {  /* 0x38f130 */
    156, 6, 0, 102, 1, 0, 3, 21, 0, 3, 6, 0, 11, 8, 0, 84,
    104, 97, 118, 101, 0, 6, 0, 14, 8, 0, 73, 104, 97, 118, 101, 110,
    39, 116, 0, 6, 0, 10, 8, 0, 59, 104, 97, 115, 0, 6, 0, 13,
    8, 0, 49, 104, 97, 115, 110, 39, 116, 0, 6, 0, 10, 8, 0, 36,
    104, 97, 100, 0, 6, 0, 13, 8, 0, 26, 104, 97, 100, 110, 39, 116,
    0, 6, 0, 16, 10, 0, 6, 3, 0, 0, 8, 0, 7, 39, 118, 101,
    0, 41, 0, 3, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_have_rx = {  /* 0x3d3368 */
   0, 1, NULL, 0, 106,
   (char *)en_rx_have_rxprog
};
const cst_regex * const en_rx_have = &en_rx_have_rx;  /* 0x3d3364 */

/* BRANCH BOL PLUS ANYOF(abcdefghijklmnopqrstuvwxyzàáâãäåæçèéêëìíîïðñòóôõöøùúûüýþÿßABCDEFGHIJKLMNOPQRSTUVWXYZÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖØÙÚÛÜÝÞ) EOL END */
static const unsigned char en_rx_word_rxprog[] = {  /* 0x38f19c */
    156, 6, 0, 130, 1, 0, 3, 11, 0, 121, 4, 0, 0, 97, 98, 99,
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
    116, 117, 118, 119, 120, 121, 122, 224, 225, 226, 227, 228, 229, 230, 231, 232,
    233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 248, 249,
    250, 251, 252, 253, 254, 255, 223, 65, 66, 67, 68, 69, 70, 71, 72, 73,
    74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
    90, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206,
    207, 208, 209, 210, 211, 212, 213, 214, 216, 217, 218, 219, 220, 221, 222, 0,
    2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_word_rx = {  /* 0x3d3380 */
   0, 1, NULL, 0, 134,
   (char *)en_rx_word_rxprog
};
const cst_regex * const en_rx_word = &en_rx_word_rx;  /* 0x3d337c */

/* BRANCH BOL ANYOF(123456789) STAR ANYOF(0123456789) EOL END */
static const unsigned char en_rx_nonzero_int_rxprog[] = {  /* 0x390b1c */
    156, 6, 0, 39, 1, 0, 3, 4, 0, 13, 49, 50, 51, 52, 53, 54,
    55, 56, 57, 0, 10, 0, 17, 4, 0, 0, 48, 49, 50, 51, 52, 53,
    54, 55, 56, 57, 0, 2, 0, 3, 0, 0, 0,
};
static const cst_regex en_rx_nonzero_int_rx = {  /* 0x3d6cc8 */
   0, 1, NULL, 0, 43,
   (char *)en_rx_nonzero_int_rxprog
};
const cst_regex * const en_rx_nonzero_int = &en_rx_nonzero_int_rx;  /* 0x3d6c18 */

