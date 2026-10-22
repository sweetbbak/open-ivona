/* Shared helpers for the matching decompilation. */
#ifndef DC_COMMON_H
#define DC_COMMON_H

/* Data that is not decompiled yet is referenced as a hidden extern: with -fpic GCC then emits
 * the same PC-relative literal it would for a TU-local definition, and the harness checks the
 * name against symbols/<bin>.data.tsv. Replace with the real definition once the data is done. */
#define HIDDEN __attribute__((visibility("hidden")))

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#endif
