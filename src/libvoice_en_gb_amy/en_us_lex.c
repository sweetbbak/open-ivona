/*
 * The en-US lexicons (0x4b020..): the main lexicon over the language file's entries with an
 * addenda and letter-to-sound rules, and a part-of-speech lexicon compiled in; their accessors.
 */
#include <stddef.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_lexicon.h"
#include "cst_lts.h"
#include "cst_string.h"
#include "en_lang.h"
#include "lex_table.h"

extern const int en_us_lts_letter_index[];
extern const unsigned char en_us_lts_model[];
extern const char *const en_us_lts_phone_table[];
extern const int en_us_lex_num_entries;
extern const char *const en_us_lex_phone_table[];
extern const char *const en_us_lex_phone_hufftable[];
extern const char *const en_us_lex_entry_hufftable[];
extern const int en_us_lex_pos_num_entries;
extern const unsigned char en_us_lex_pos_data[];
extern const int en_us_lex_pos_num_bytes;
extern const char *const en_us_lex_pos_phone_table[];
extern const char *const en_us_lex_pos_phone_hufftable[];
extern const char *const en_us_lex_pos_entry_hufftable[];

int lex_word_pos_list(const cst_lexicon *l, const char *word, char *buf, size_t max_len);
int lex_select_pos(const cst_lexicon *l, const char *word, const char *s);

/* 0x4b020 arm: a copy of word without apostrophes */
char *en_strip_apostrophes(const char *word)
{
    char *s = cst_strdup(word);
    char *p, *d;

    for (p = d = s; *p; p++)
        if (*p != '\'')
            *d++ = *p;
    *d = '\0';
    return s;
}

#include "en_us_lex_addenda.inc"

static lex_table en_us_lex_pos_addenda;
static cst_lexicon en_us_lex_pos;
static cst_lexicon en_us_lex;
static cst_lts_rules en_us_lts_rules;

/* 0x4b0d8 arm: sets the lexicons up */
void en_us_setup(en_lang_core *core)
{
    en_us_lts_rules.name = "en_us_lex_lts";
    en_us_lts_rules.letter_index = en_us_lts_letter_index;
    en_us_lts_rules.models = en_us_lts_model;
    en_us_lts_rules.phone_table = en_us_lts_phone_table;
    en_us_lts_rules.context_window_size = 4;
    en_us_lts_rules.context_extra_feats = 1;
    en_us_lts_rules.letter_table = NULL;
    en_us_lts_rules.relative_states = 1;
    en_us_lex.name = "en_us_lex";
    en_us_lex.num_entries = en_us_lex_num_entries;
    en_us_lex.data = core->lex_data;
    en_us_lex.num_bytes = core->lex_num_bytes;
    en_us_lex.phone_table = (char **)en_us_lex_phone_table;
    en_us_lex.lts_rule_set = &en_us_lts_rules;
    en_us_lex.addenda = (void *)&en_us_lex_addenda;
    en_us_lex.phone_hufftable = en_us_lex_phone_hufftable;
    en_us_lex.entry_hufftable = en_us_lex_entry_hufftable;
    en_us_lex.word_norm = NULL;
    en_us_lex.lts_norm = NULL;
    feat_set_string(core->features, "lts_module_type", "CART");
    feat_set_string(core->features, "external_pls", "no");
    en_us_lex_pos.name = "en_us_lex_pos";
    en_us_lex_pos.num_entries = en_us_lex_pos_num_entries;
    en_us_lex_pos.data = (unsigned char *)en_us_lex_pos_data;
    en_us_lex_pos.num_bytes = en_us_lex_pos_num_bytes;
    en_us_lex_pos.phone_table = (char **)en_us_lex_pos_phone_table;
    en_us_lex_pos.lts_rule_set = NULL;
    en_us_lex_pos.addenda = &en_us_lex_pos_addenda;
    en_us_lex_pos.phone_hufftable = en_us_lex_pos_phone_hufftable;
    en_us_lex_pos.entry_hufftable = en_us_lex_pos_entry_hufftable;
    en_us_lex_pos.word_norm = NULL;
    en_us_lex_pos.lts_norm = NULL;
}

/* 0x4b2a4 arm */
void en_us_teardown(void)
{
}

/* 0x4b2a8 arm: whether word (as pos) is in the lexicon or, unless main_only, the
 * part-of-speech lexicon */
int en_us_in_lex(const char *word, const char *pos, int main_only)
{
    if (in_lex(&en_us_lex, word, pos))
        return 1;
    if (main_only)
        return 0;
    return in_lex(&en_us_lex_pos, word, pos) != 0;
}

/* 0x4b308 arm: the letter-to-sound pronunciation of word (*score -1) */
cst_val *en_us_lts(const char *word, float *score)
{
    if (score)
        *score = -1;
    return lts_apply(word, "", &en_us_lts_rules);
}

/* 0x4b334 arm: the pronunciation of word as pos; *score is 0 for a lexicon entry, -1 when it
 * comes from the letter-to-sound rules (apostrophes dropped) */
cst_val *en_us_lex_lookup(const char *word, const char *pos, float *score)
{
    cst_val *phones;
    char *w;

    if ((phones = lex_lookup(&en_us_lex, word, pos)) != NULL) {
        if (score)
            *score = 0;
        return phones;
    }
    if (strchr(word, '\'')) {
        w = en_strip_apostrophes(word);
        phones = en_us_lts(w, score);
        cst_free(w);
        return phones;
    }
    return en_us_lts(word, score);
}

/* 0x4b3c4 arm: the first of the parts of speech in s that word has, or -1 */
int en_us_lex_select_pos(const char *word, const char *s)
{
    return lex_select_pos(&en_us_lex, word, s);
}

/* 0x4b3e0 arm: word's parts of speech in the part-of-speech lexicon */
int en_us_lex_pos_list(const char *word, char *buf, size_t max_len)
{
    return lex_word_pos_list(&en_us_lex_pos, word, buf, max_len);
}
