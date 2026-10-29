/* Phone pair rules (apply_phone_rules, phones.c in the voice library): a pair of adjacent
 * segments with given names (and syllable stresses) becomes another pair. */
#ifndef PHONE_RULE_H
#define PHONE_RULE_H

typedef struct phone_rule {
    const char *from1;      /* NULL ends a table */
    int stress1;            /* -1: any */
    const char *from2;
    int stress2;
    int syllables;          /* 0: any, 1: in one syllable, 2: across a syllable boundary */
    const char *to1;        /* NULL: kept; "": deleted */
    int new_stress1;        /* -1: kept */
    const char *to2;
    int new_stress2;
} phone_rule;

#endif
