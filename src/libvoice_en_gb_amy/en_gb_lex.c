/*
 * The en-GB lexicon (0x3b4e8..): the lexicon over the language file's entries, with an addenda
 * and letter-to-sound rules (compare Flite's cmu_lex_init), and its accessors.
 */
#include <stddef.h>
#include <string.h>
#include "cst_alloc.h"
#include "cst_features.h"
#include "cst_lexicon.h"
#include "cst_lts.h"
#include "en_lang.h"
#include "lex_table.h"

extern const int en_gb_lts_letter_index[];
extern const unsigned char en_gb_lts_model[];
extern const char *const en_gb_lts_phone_table[];
extern const int en_gb_lex_num_entries;
extern const char *const en_gb_lex_phone_table[];
extern const char *const en_gb_lex_phone_hufftable[];
extern const char *const en_gb_lex_entry_hufftable[];

int lex_word_pos_list(const cst_lexicon *l, const char *word, char *buf, size_t max_len);
int lex_select_pos(const cst_lexicon *l, const char *word, const char *s);
char *en_strip_apostrophes(const char *word);

#include "en_gb_lex_addenda.inc"

static cst_lexicon en_gb_lex;
static cst_lts_rules en_gb_lts_rules;

/* 0x3b4e8 arm: sets the lexicon up over the language file's entries */
void en_gb_setup(en_lang_core *core)
{
    en_gb_lts_rules.name = "en_gb_lex_lts";
    en_gb_lts_rules.letter_index = en_gb_lts_letter_index;
    en_gb_lts_rules.models = en_gb_lts_model;
    en_gb_lts_rules.phone_table = en_gb_lts_phone_table;
    en_gb_lts_rules.context_window_size = 4;
    en_gb_lts_rules.context_extra_feats = 1;
    en_gb_lts_rules.letter_table = NULL;
    en_gb_lts_rules.relative_states = 1;
    en_gb_lex.name = "en_gb_lex";
    en_gb_lex.num_entries = en_gb_lex_num_entries;
    en_gb_lex.data = core->lex_data;
    en_gb_lex.num_bytes = core->lex_num_bytes;
    en_gb_lex.phone_table = (char **)en_gb_lex_phone_table;
    en_gb_lex.lts_rule_set = &en_gb_lts_rules;
    en_gb_lex.addenda = (void *)&en_gb_lex_addenda;
    en_gb_lex.phone_hufftable = en_gb_lex_phone_hufftable;
    en_gb_lex.entry_hufftable = en_gb_lex_entry_hufftable;
    en_gb_lex.word_norm = NULL;
    en_gb_lex.lts_norm = NULL;
    feat_set_string(core->features, "lts_module_type", "CART");
    feat_set_string(core->features, "external_pls", "no");
}

/* 0x3b628 arm */
void en_gb_teardown(void)
{
}

/* 0x3b62c arm: the pronunciation of word as pos; *score is 0 for a lexicon entry, -1 when it
 * comes from the letter-to-sound rules (apostrophes dropped) */
cst_val *en_gb_lex_lookup(const char *word, const char *pos, float *score)
{
    cst_val *phones;
    char *w;

    if ((phones = lex_lookup(&en_gb_lex, word, pos)) != NULL) {
        if (score)
            *score = 0;
        return phones;
    }
    if (strchr(word, '\'')) {
        w = en_strip_apostrophes(word);
        if (score)
            *score = -1;
        phones = lts_apply(w, "", &en_gb_lts_rules);
        cst_free(w);
        return phones;
    }
    if (score)
        *score = -1;
    return lts_apply(word, "", &en_gb_lts_rules);
}

/* 0x3b704 arm (main_only: the en-US lexicons' flag, unused) */
int en_gb_in_lex(const char *word, const char *pos, int main_only)
{
    return in_lex(&en_gb_lex, word, pos);
}

/* 0x3b720 arm: the first of the parts of speech in s that word has, or -1 */
int en_gb_lex_select_pos(const char *word, const char *s)
{
    return lex_select_pos(&en_gb_lex, word, s);
}

/* 0x3b73c arm: word's parts of speech */
int en_gb_lex_pos_list(const char *word, char *buf, size_t max_len)
{
    return lex_word_pos_list(&en_gb_lex, word, buf, max_len);
}
