#include <string.h>
#include <stdio.h>
#include "cst_alloc.h"
#include "cst_string.h"
#include "cst_val.h"
#include "cst_lts.h"
#include "cst_endian.h"

typedef struct cst_string_buffer_struct {
    int len;
    int cap;
    char *data;
} cst_string_buffer;

void cst_string_buffer_init(cst_string_buffer *b);
void cst_string_buffer_free(cst_string_buffer *b);
void cst_string_buffer_clear(cst_string_buffer *b);
void cst_string_buffer_append_n(cst_string_buffer *b, const char *str, int len);
void cst_string_buffer_append(cst_string_buffer *b, const char *str);
char *cst_substr(const char *src, int start, int length);

#define cst_substr cst_substr

/* 0x54740: arm */
cst_val *lts_apply(const char *word, const char *feats, const cst_lts_rules *r)
{
    cst_string_buffer sb_str;
    cst_lts_rule state;
    cst_val *phones = NULL;
    cst_lts_letter *full_buff;
    int pos;

    cst_string_buffer_init(&sb_str);
    full_buff = cst_alloc(cst_lts_letter, strlen(word) + (r->context_window_size * 2) + 1);
    sprintf((char *)full_buff, "%.*s#%s#%.*s",
            r->context_window_size - 1, "00000000",
            word,
            r->context_window_size - 1, "00000000");

    for (pos = r->context_window_size + strlen(word) - 1;
         full_buff[pos] != '#';
         pos--)
    {
        int index;
        int start;
        int relative_start;
        const char *p;
        char *dash;
        const unsigned char *vals;

        cst_string_buffer_clear(&sb_str);
        cst_string_buffer_append_n(&sb_str, (char *)full_buff + pos - r->context_window_size, r->context_window_size);
        cst_string_buffer_append_n(&sb_str, (char *)full_buff + pos + 1, r->context_window_size);
        cst_string_buffer_append(&sb_str, feats);

        if (r->letter_table)
        {
            int j;
            for (j = 3; r->letter_table[j]; j++)
            {
                if (r->letter_table[j][0] == full_buff[pos])
                    break;
            }
            if (!r->letter_table[j])
                continue;
            index = j - 3;
        }
        else
        {
            int c = full_buff[pos] - 'a';
            if ((unsigned char)c > 25)
                continue;
            index = c % 26;
        }

        vals = (const unsigned char *)sb_str.data;
        start = r->letter_index[index];
        relative_start = r->relative_states ? start : 0;
        memcpy(&state, &r->models[start * 6], 6);
        while (state.feat != CST_LTS_EOR)
        {
            cst_lts_addr nstate;
            if (vals[state.feat] == state.val)
                nstate = state.qtrue;
            else
                nstate = state.qfalse;
            if (CST_BIG_ENDIAN)
                nstate = SWAPSHORT(nstate);
            memcpy(&state, &r->models[(nstate + relative_start) * 6], 6);
        }

        p = r->phone_table[state.val];
        if (strcmp("epsilon", p) == 0)
            continue;

        if (__builtin_expect((dash = (char *)cst_strchr(p, '-')) != NULL, 1))
        {
            char *left = cst_substr(p, 0, strlen(p) - strlen(dash));
            cst_val *v_left = string_val(left);
            if (left[strlen(left) - 1] == '0')
            {
                char *s = (char *)val_string(v_left);
                s[strlen(s) - 1] = '\0';
            }

            const char *right = dash + 1;
            cst_val *v_right = string_val(right);
            if (right[strlen(right) - 1] == '0')
            {
                char *s = (char *)val_string(v_right);
                s[strlen(s) - 1] = '\0';
            }

            phones = cons_val(v_left, cons_val(v_right, phones));
            cst_free(left);
        }
        else
        {
            cst_val *v = string_val(p);
            if (p[strlen(p) - 1] != '0')
                phones = cons_val(v, phones);
            else
            {
                char *s = (char *)val_string(v);
                s[strlen(s) - 1] = '\0';
                phones = cons_val(v, phones);
            }
        }
    }

done:
    cst_string_buffer_free(&sb_str);
    cst_free(full_buff);
    return phones;
}
