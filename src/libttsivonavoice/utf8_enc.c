/*
 * UTF-8 encoder (0x5abb8), alone between vector.c and the unit concatenation code.
 */

static const unsigned int utf8_max[6] = { 0x7f, 0x7ff, 0xffff, 0x1fffff, 0x3ffffff, 0x7fffffff };
static const unsigned char utf8_lead[6] = { 0x00, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc };

/* 0x5abb8 arm: writes c as UTF-8 (up to 7 bytes, PCRE's ord2utf8 scheme) to out; returns the
 * byte count. */
int utf8_encode(unsigned int c, char *out)
{
    int i, j;

    for (i = 0; i < 6; i++)
        if (c <= utf8_max[i])
            break;
    out += i;
    for (j = i; j > 0; j--) {
        *out-- = 0x80 | (c & 0x3f);
        c >>= 6;
    }
    *out = utf8_lead[i] | c;
    return i + 1;
}
