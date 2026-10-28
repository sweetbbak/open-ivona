#include "cst_item.h"
#include "cst_utterance.h"
#include "cst_features.h"
#include "cst_string.h"
#include "cst_relation.h"

typedef struct postlex_rule_result_struct {
    int action;
    int flags;
    const char *replacement;
} postlex_rule_result;

typedef void (*postlex_rule_cb)(postlex_rule_result *res, void *arg0, const char *p1, void *arg2, const char *p2);

cst_val *val_last_n(cst_val *v, int n);
cst_val *val_car_mut(const cst_val *v);
cst_val *val_cdr_mut(const cst_val *v);

/* 0x54b58: arm */
cst_val *postlex_rewrite_phones(void *arg0, cst_val *phones_in, void *arg2_in, cst_val *next_phones,
                                postlex_rule_cb rule_cb, int mask1, int mask2)
{
    register cst_val *phones asm("r6") = phones_in;
    register void *arg2 asm("r7") = arg2_in;
    asm("" : "+r"(phones), "+r"(arg2));
    postlex_rule_result out1;
    cst_val *tail;
    cst_val *r5;
    const char *p1;
    const cst_val *fp;

    tail = val_last_n(phones, 2);
    r5 = val_cdr(tail) ? (cst_val *)val_cdr(tail) : tail;

    p1 = val_string(val_car(r5));
    fp = val_car(next_phones);
    rule_cb(&out1, arg0, p1, arg2, val_string(fp));

    if (!(out1.flags & mask1))
        return phones;

    set_car(next_phones, (cst_val *)string_val(out1.replacement));
    delete_val((cst_val *)fp);

    if (out1.action == 1)
    {
        if (r5 == phones)
        {
            delete_val(r5);
            return NULL;
        }
        set_cdr(tail, NULL);
        delete_val(r5);
    }

    if (mask2)
    {
        cst_val *cur = next_phones;
        r5 = val_cdr_mut(next_phones);
        if (r5)
        {
            cst_val *prev = NULL;
            postlex_rule_result out2;
            cst_val *v_cur;
            do
            {
                v_cur = val_car_mut(cur);
                tail = val_car_mut(r5);
                p1 = val_string(v_cur);
                rule_cb(&out2, NULL, p1, arg2, val_string(tail));
                if (!(out2.flags & mask2))
                    return phones;
                set_car(r5, (cst_val *)string_val(out2.replacement));
                delete_val(tail);
                if (out2.action == 1)
                {
                    if (prev)
                        set_cdr(prev, r5);
                    set_cdr(cur, NULL);
                    delete_val(cur);
                    cur = prev;
                }
                prev = cur;
                cur = r5;
                r5 = val_cdr_mut(r5);
            } while (r5);
        }
    }
    return phones;
}

/* 0x54d3c: arm */
void postlex_assimilation(cst_utterance *u, postlex_rule_cb cb, int def_assim, int flag2)
{
    cst_item *phrase;
    cst_item *w1;
    cst_item *w2;
    postlex_rule_result res;
    postlex_rule_result res2;

    for (phrase = relation_head(utt_relation(u, 3)); phrase != NULL; phrase = item_next(phrase))
    {
        w1 = item_daughter(phrase);
        while (w1 && !item_daughter(item_as(w1, 4)))
            w1 = item_next(w1);

        w2 = item_next(w1);
        while (w2 && !item_daughter(item_as(w2, 4)))
            w2 = item_next(w2);

        while (w2 != NULL)
        {
            cst_item *last_phone = item_last_daughter(item_last_daughter(item_as(w1, 4)));
            cst_item *first_phone = item_daughter(item_daughter(item_as(w2, 4)));
            int assim = get_param_int(item_feats(w1), "assimilation", def_assim);

            cb(&res, (void *)item_feat_string(w1, "name"), item_feat_string(last_phone, "name"), NULL, item_feat_string(first_phone, "name"));
            if (assim & res.flags)
            {
                item_set_string(first_phone, "name", res.replacement);
                if (res.action == 1)
                {
                    cst_item *seg = item_as(last_phone, 6);
                    cst_item *syl = item_parent(last_phone);
                    delete_item(seg);
                    delete_item(last_phone);
                    if (!item_daughter(syl))
                    {
                        delete_item(item_as(syl, 5));
                        delete_item(syl);
                    }
                }
                if (flag2)
                {
                    cst_item *p1 = item_daughter(item_daughter(item_as(w2, 4)));
                    cst_item *p2 = item_next(p1);
                    if (!p2)
                    {
                        p2 = item_daughter(item_next(item_parent(p1)));
                        if (!p2)
                            goto next_word;
                    }
                    goto enter_inner_loop;
                    while (1)
                    {
                        p1 = p2;
                        p2 = item_next(p1);
                        if (!p2)
                            p2 = item_daughter(item_next(item_parent(p1)));
                        if (!p2)
                            break;
                    enter_inner_loop:
                        cb(&res2, NULL, item_feat_string(p1, "name"), (void *)item_feat_string(w2, "name"), item_feat_string(p2, "name"));
                        if (!(flag2 & res2.flags))
                            break;
                        item_set_string(p2, "name", res2.replacement);
                        if (res2.action == 1)
                        {
                            cst_item *syl = item_parent(p1);
                            cst_item *seg = item_as(p1, 6);
                            delete_item(seg);
                            delete_item(p1);
                            if (!item_daughter(syl))
                            {
                                delete_item(item_as(syl, 5));
                                delete_item(syl);
                            }
                        }
                    }
                next_word:;
                }
            }
            cst_item *next_w = item_next(w2);
            while (next_w && !item_daughter(item_as(next_w, 4)))
                next_w = item_next(next_w);
            if (!next_w)
                break;
            w1 = w2;
            w2 = next_w;
        }
    }
}

/* 0x550f8: arm */
int postlex_voice_is(const char **names, cst_utterance *u)
{
    const cst_val *v = feat_val(u->features, "postlex_voice_name");
    if (!v) {
        v = feat_val(u->features, "name");
        if (__builtin_expect(!v, 1))
            return 0;
    }
    return cst_member_string(val_string(v), names);
}

/* 0x55154: arm */
int postlex_voice_is_name(const char *name, cst_utterance *u)
{
    const char *names[2];
    names[0] = name;
    names[1] = NULL;
    return postlex_voice_is(names, u);
}

/* 0x55178: arm */
void postlex_remove_seg_syl(cst_item *item)
{
    delete_item(item_as(item, 4));
    delete_item(item);
}

cst_item *item_append(cst_item *i, cst_item *new_item);

/* 0x55198: arm */
void postlex_create_syl(cst_item *item)
{
    cst_item *syl_parent = item_parent(item_as(item, 4));
    cst_item *new_syl = item_append(syl_parent, NULL);
    item_set_int(new_syl, "stress", 0);
    item_set_int(new_syl, "glottal_stop_pos", 0);
    item_append(item_as(syl_parent, 5), new_syl);
    if (!item_feat_present(syl_parent, "endtone"))
        return;
    item_set_string(new_syl, "endtone", item_feat_string(syl_parent, "endtone"));
    item_set_string(syl_parent, "endtone", NULL);
    asm volatile("");
}
cst_item *item_traverse(const cst_item *item, ...);

/* 0x55248: arm */
int postlex_is_consonant(cst_item *item)
{
    return strcmp("-", ffeature_string(item_traverse(item, 0), "ph_vc")) == 0;
}

/* 0x55284: arm */
int postlex_is_final_syl(cst_item *item)
{
    cst_item *syl = item_as(item, 4);
    if (!syl || item_next(syl))
        return 0;
    return item_next(item_parent(syl)) == NULL;
}

cst_item *item_prepend(cst_item *i, cst_item *new_item);
cst_item *item_add_daughter(cst_item *i, cst_item *new_item);
cst_val *phoneset_val(const void *val);

/* 0x552c4: arm */
cst_item *postlex_insert_phone(cst_item *syl, const char *phone_name, const void *ps)
{
    cst_item *seg = item_as(item_last_daughter(item_prev(syl)), 6);
    cst_item *new_seg = item_append(seg, NULL);
    cst_item *d = item_daughter(syl);

    register const char *ps_name asm("r8") = "phoneset";
    item_set_string(new_seg, "name", phone_name);
    asm("" : "+r"(ps_name));
    item_set(new_seg, ps_name, phoneset_val(ps));
    if (d)
        return item_prepend(d, new_seg);
    else
        return item_add_daughter(syl, new_seg);
}

/* 0x55360: arm */
void postlex_resyllabify(cst_item *item, const void *ps)
{
    cst_item *syl = item_parent(item_as(item, 4));
    cst_item *next_syl;
    cst_item *d;

    postlex_create_syl(item_as(item_last_daughter(syl), 6));
    next_syl = item_next(syl);
    for (d = item_last_daughter(syl); item_as(d, 6) != item; d = item_last_daughter(syl))
    {
        cst_item *seg = item_as(d, 6);
        postlex_insert_phone(next_syl, item_feat_string(d, "name"), ps);
        postlex_remove_seg_syl(seg);
    }
}

/* 0x55b84: arm */
void postlex_noop(void)
{
}
