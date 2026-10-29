/*
 * Amy's target (model) cost (0x51d18..): how well a candidate diphone, units a|b, fits the
 * wanted one, ta|tb. Hand-tuned penalties on the phone context, prosodic features (stress,
 * accent, tone), position, duration and the unit's domain.
 */
#include <math.h>
#include <string.h>
#include "tts_voxdb.h"

int unit_domain_cost(const vox_unit *a, const vox_unit *b);
float acoustic_stub_1(const void *a, const void *b, int x);
unsigned int unit_half_cost(const vox_unit *x, const vox_unit *t, const int *weights);

#define PHONE(u, i) ((u)->phones[i]->name)
#define F(u, off) (((const unsigned char *)(u))[off])   /* a byte feature of a unit */
#define CLASS(s) (*(const char *)(s))                   /* first char of a class string */

static inline int absdiff(int x, int y)
{
    int d = x - y;

    return d < 0 ? -d : d;
}

/* both "1", or one 1 and the other above 1, of a feature (stress/accent levels) */
#define MISMATCH_1(x, y) ((x) == 1 ? (y) > 1 : (x) > 1 && (y) == 1)

/* 0x390828: weights of the side cost; [0] is the penalty for an unstressed candidate where the
 * target is stressed (except for accented vowels) */
static const int unit_cost_weights[2] = { 15, 400 };

/* the cost of unit x standing for target unit t, scaled by 0.4: the phone context, positions,
 * stress, accent and tone, duration and the domain */
static inline unsigned int unit_side_cost(const vox_unit *x, const vox_unit *t, const int *w)
{
    int p0 = strcmp(PHONE(x, 0), PHONE(t, 0)) == 0;
    int ctx, cost, tone;
    char vc = CLASS(t->p88);
    float xd, td, r;

    /* the phone context */
    ctx = p0 ? 0 : 3;
    if (strcmp(PHONE(x, 1), PHONE(t, 1)) != 0)
        ctx = p0 ? 7 : 10;
    cost = (strcmp(PHONE(x, 3), PHONE(t, 3)) ? 7 : 0) + ctx
           + (strcmp(PHONE(x, 4), PHONE(t, 4)) ? 3 : 0);
    if (strcmp(PHONE(t, 2), "r") == 0)
        cost *= 3;
    if (strcmp(PHONE(t, 1), "r") == 0 || strcmp(PHONE(t, 3), "r") == 0)
        cost = vc == '+' ? cost * 6 : cost * 3;
    /* positions and prosody */
    cost += absdiff(F(x, 0x19), F(t, 0x19)) * 3 + absdiff(F(x, 0x18), F(t, 0x18)) * 3
            + absdiff(F(x, 0x1a), F(t, 0x1a)) * 3;
    if (F(t, 0x1d) != F(x, 0x1d))
        cost += vc == '+' ? 1005 : 5;
    cost += (absdiff(F(x, 0x21), F(t, 0x21)) + absdiff(F(x, 0x20), F(t, 0x20))) * 2;
    if (MISMATCH_1(F(x, 0x22), F(t, 0x22)))
        cost += 2;
    if (F(x, 0x23) == 1 && F(t, 0x23) > 1)
        cost += F(x, 0x1d) == 0 || vc != '+' ? 15 : w[0];
    if (F(x, 0x26) != F(t, 0x26))
        cost++;
    if (F(x, 0x27) != F(t, 0x27))
        cost++;
    cost += absdiff(F(x, 0x34), F(t, 0x34)) * 3 + (strcmp(x->ph30->name, t->ph30->name) ? 3 : 0);
    if (F(x, 0x44) != F(t, 0x44))
        cost++;
    if (F(x, 0x45) == 1) {
        if (F(t, 0x45) > 1)
            cost += 2;
    } else if (F(x, 0x45) > 1 && F(t, 0x45) == 1)
        cost++;
    if (F(x, 0x46) == 1 && F(t, 0x46) > 1)
        cost += 3;
    if (F(x, 0x4c) != F(t, 0x4c))
        cost++;
    if (x->content40 == 0) {
        if (t->content40)
            cost += 75;
    } else if (t->content40 == 0)
        cost += 50;
    if (x->content38 != t->content38)
        cost += 3;
    if (x->content48 != t->content48)
        cost += 3;
    if (t->content40 == 0 && strcmp(t->ph40->name, x->ph40->name) != 0)
        cost += 10;
    if (F(x, 0x50) != F(t, 0x50))
        cost++;
    if (MISMATCH_1(F(x, 0x51), F(t, 0x51)))
        cost++;
    if (MISMATCH_1(F(x, 0x52), F(t, 0x52)))
        cost++;
    /* the tone (ph54: ToBI labels such as L, L*, H) where the target is accented */
    if (F(t, 0x46) == 1) {
        tone = strcmp(x->ph54->name, t->ph54->name) ? 10 : 0;
        if (strcmp("L*", t->ph54->name) == 0 && strcmp("L", x->ph54->name) == 0)
            tone = 0;
        else if (strcmp("L", t->ph54->name) == 0 && strcmp("L*", x->ph54->name) == 0)
            tone = 8;
        if (F(x, 0x46) != 1)
            cost += 5;
        if (F(t, 0x23) == 1) {
            int k = F(x, 0x23) == 1 ? 0 : 100, m = F(x, 0x23) == 1 ? 0 : 1500;

            if (vc == '+' || CLASS(t->p8c) != '-')
                cost += tone * 250 + m;
            else
                cost += tone * 20 + k;
        } else if (F(t, 0x1d)) {
            unsigned int u = F(t, 0x1d) == F(x, 0x1d) && F(x, 0x46) < 2 && F(x, 0x23) != 1
                             ? F(x, 0x23) != F(t, 0x23) : 10;

            if (vc == '+')
                cost += tone * 25 + u * 25;
            else if (CLASS(t->p8c) == '-')
                cost += tone * 10 + u * 10;
            else
                cost += tone * 15 + u * 15;
        }
        if (F(t, 0x19) == 1)
            cost += CLASS(t->p8c) == '-' ? tone * 20 : tone * 40;
        cost += tone;
    }
    if ((F(x, 0x46) == 1 || F(x, 0x45) == 1) && F(t, 0x23) > 1 && F(t, 0x22) > 1
        && strcmp(x->ph54->name, "H") == 0)
        cost += 1000;
    /* duration */
    if (t->f98 != 4 && strcmp(PHONE(x, 2), "pau") != 0) {
        xd = x->dur;
        td = t->dur;
        r = fabsf((xd - td) / (xd + td));
        if (xd >= td) {
            float y = r * 150.0f;

            cost += (int)y;
            if (r > 0.15f)
                cost += (int)(y - 22.5f);
            if (r > 0.35f)
                cost += (int)(r * 360.0f - 126.0f);
        } else if (xd < td) {
            cost += (int)(r * 250.0f);
            if (r > 0.2f)
                cost += (int)(r * 120.0f - 24.0f);
            if (r > 0.3f)
                cost += (int)(r * 150.0f - 45.0f);
            if (r > 0.4f)
                cost += (int)(r * 300.0f - 120.0f);
        }
    }
    return (unsigned int)((float)(unsigned int)(cost + unit_domain_cost(x, t)) * 0.4f);
}

/* 0x51d18 arm: the side cost of x for t with the given weights */
unsigned int unit_half_cost(const vox_unit *x, const vox_unit *t, const int *weights)
{
    return unit_side_cost(x, t, weights);
}

/* 0x52538 arm: the model cost of candidate a|b for target ta|tb (select_model_cost_fn): both
 * side costs, plus penalties where a phrase boundary (0x18/0x19 with 0x20/0x21) falls between
 * the halves differently */
unsigned int unit_model_cost(const vox_unit *a, const vox_unit *b, const vox_unit *ta,
                             const vox_unit *tb)
{
    unsigned int half = unit_half_cost(a, ta, unit_cost_weights);
    unsigned int total = unit_side_cost(b, tb, unit_cost_weights);
    char vc = CLASS(tb->p88);
    int extra = 0, aflag, taflag;

    if (F(a, 0x19) == 1 && F(a, 0x21) == 1 && F(b, 0x18) == 1 && F(b, 0x20) == 1) {
        if (!(F(ta, 0x19) == 1 && F(ta, 0x21) == 1 && F(tb, 0x18) == 1 && F(tb, 0x20) == 1))
            extra = vc == '+' ? 500 : 350;
    } else if (F(ta, 0x19) == 1 && F(ta, 0x21) == 1 && F(tb, 0x18) == 1)
        extra = F(tb, 0x20) == 1 ? 5 : 0;
    aflag = F(a, 0x18) == 1 && F(a, 0x20) == 1;
    taflag = F(ta, 0x18) == 1 && F(ta, 0x20) == 1;
    if (aflag != taflag)
        extra += CLASS(ta->p88) == '+' ? 10 : 5;
    if ((F(b, 0x19) == 1 && F(b, 0x21) == 1) != (F(tb, 0x19) == 1 && F(tb, 0x21) == 1))
        extra += 5;
    total = total + half + extra;
    if (*(const int *)(ta->unk_60 + 4) != 0 || *(const int *)(tb->unk_60 + 4) != 0)
        total = (unsigned int)((float)total + (acoustic_stub_1(a->unk_60, ta->unk_60, 1)
                                               + acoustic_stub_1(b->unk_60, tb->unk_60, 0)));
    return total;
}
