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

#include <string.h>
#include "syl_cart.h"

typedef struct cst_phoneset_struct cst_phoneset;
int ph_is_syllabic(const cst_phoneset *ps, const cst_val *p);
const char *phone_feature_string(const cst_phoneset *ps, const char *phonename,
                                 const char *featname);

#define PHONE(v) val_string(val_car(v))

/* the phone at p, skipping "-" boundaries, then n further phones */
static inline const cst_val *phone_skip(const cst_val *p, int n)
{
    while (p && strcmp(PHONE(p), "-") == 0)
        p = val_cdr(p);
    while (p && n > 0) {
        p = val_cdr(p);
        n--;
        while (p && strcmp(PHONE(p), "-") == 0)
            p = val_cdr(p);
    }
    return p;
}

/* 0x5540c: arm: inserts a "-" syllable boundary into every consonant cluster between two
 * syllabic phones of the list (clusters that already hold a "-" or a "pau" are left alone),
 * the size of the next syllable's onset given by the tree */
cst_val *postlex_apply_rules(const syl_cart *cart, const cst_phoneset *ps, cst_val *phones)
{
    const cst_val *p, *q, *r;
    const syl_cart_node *nd;
    const syl_cart_feat *f;
    const char *name, *value;
    char buf[12];
    int node, n, k, cnt, len;

    for (p = phones; p; p = val_cdr(p))
        if (ph_is_syllabic(ps, p))
            break;
    if (p == NULL)
        return phones;
    for (;;) {
        /* the next boundary, pause or syllabic phone */
        for (q = val_cdr(p); q; q = val_cdr(q)) {
            name = PHONE(q);
            if (strcmp(name, "-") == 0 || strcmp(name, "pau") == 0 || ph_is_syllabic(ps, q))
                break;
        }
        if (q == NULL)
            return phones;
        name = PHONE(q);
        if (strcmp(name, "-") == 0 || strcmp(name, "pau") == 0) {
            for (p = val_cdr(q); p; p = val_cdr(p))
                if (ph_is_syllabic(ps, p))
                    break;
            if (p == NULL)
                return phones;
            continue;
        }
        /* the phones of the cluster with its vowel */
        n = 0;
        k = 0;
        r = q;
        if (p != q) {
            for (r = p; r; ) {
                if (strcmp(PHONE(r), "-") != 0)
                    n++;
                r = val_cdr(r);
                if (r == q)
                    break;
            }
        }
        if (r == q && n > 1) {
            node = 0;
            for (;;) {
                nd = &cart->nodes[node];
                if (nd->yes == 0 && nd->no == 0) {
                    k = nd->val;
                    if (k >= n)
                        k = n - 1;
                    break;
                }
                f = &cart->feats[nd->feat];
                r = NULL;
                if (f->offset != 0) {
                    switch (f->kind) {
                    case 0:
                        if (phones != p) {
                            cnt = 0;
                            for (r = phones; r != p && r; r = val_cdr(r))
                                if (strcmp(PHONE(r), "-") != 0)
                                    cnt++;
                            r = r && f->offset <= cnt ? phone_skip(phones, cnt - f->offset) : NULL;
                        }
                        break;
                    case 1:
                        cnt = 0;
                        for (r = p; r != q && r; r = val_cdr(r))
                            if (strcmp(PHONE(r), "-") != 0)
                                cnt++;
                        r = r && f->offset <= cnt ? phone_skip(p, cnt - f->offset) : NULL;
                        break;
                    case 2:
                        r = phone_skip(q, f->offset);
                        break;
                    case 3:
                        r = f->offset == 1 ? p : f->offset == 2 ? q : NULL;
                        break;
                    }
                }
                if (r) {
                    name = PHONE(r);
                    len = strlen(name);
                } else {
                    name = "pau";
                    len = 3;
                }
                if (len == 0)
                    value = "";
                else if (strcmp(f->feat, "stress") == 0) {
                    if (strchr("0123", (unsigned char)name[len - 1]))
                        value = &name[len - 1];
                    else
                        value = strcmp(phone_feature_string(ps, name, "vc"), "+") == 0 ? "0" : "-";
                } else {
                    if (len >= 10)
                        len = 10;
                    len--;
                    if (strchr("0123", (unsigned char)name[len])) {
                        memcpy(buf, name, len);
                        buf[len] = '\0';
                        name = buf;
                    }
                    value = phone_feature_string(ps, name, f->feat);
                }
                node = strcmp(value, cart->vals[nd->val]) == 0 ? nd->yes : nd->no;
            }
        }
        /* the boundary goes k phones before q */
        r = p;
        for (len = val_length(p) - val_length(q) - 1 - k; len > 0; len--)
            r = val_cdr(r);
        set_cdr((cst_val *)r, cons_val(string_val("-"), val_cdr(r)));
        p = q;
    }
}

/* 0x55b84: arm */
void postlex_noop(void)
{
}
