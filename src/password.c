/* Pixel Open — codes de reprise (C portable, testé sur PC par
 * tests/password_test.c). Les 45 bits de données et 3 bits de version
 * forment 6 octets ; une somme de contrôle de 12 bits est ajoutée puis les
 * 60 bits sont écrits en 12 symboles de 5 bits, brouillés en chaîne pour
 * qu'une lettre modifiée en change plusieurs. */
#include "password.h"

static const char alpha[] = PW_ALPHA;
static pu8 buf[8];
static pu8 bitpos;

static void put_bits(pu8 v, pu8 n)
{
    pu8 i;
    for (i = 0; i < n; ++i) {
        if (v & (1 << i)) buf[bitpos >> 3] |= (pu8)(1 << (bitpos & 7));
        ++bitpos;
    }
}

static pu8 get_bits(pu8 n)
{
    pu8 i, v = 0;
    for (i = 0; i < n; ++i) {
        if (buf[bitpos >> 3] & (1 << (bitpos & 7))) v |= (pu8)(1 << i);
        ++bitpos;
    }
    return v;
}

static unsigned int check12(void)
{
    unsigned int c = 0x5A3;
    pu8 i;
    for (i = 0; i < 6; ++i) {
        c = (unsigned int)(((c << 3) | (c >> 9)) & 0xFFF);
        c ^= buf[i];
        c = (c + (unsigned int)buf[i] * 37u + i) & 0xFFF;
    }
    return c;
}

static void pack(const PwData *d)
{
    pu8 i;
    for (i = 0; i < 8; ++i) buf[i] = 0;
    bitpos = 0;
    put_bits(d->type, 3); put_bits(d->stage, 4); put_bits(d->seed, 8);
    put_bits(d->chr, 4); put_bits(d->surf, 3); put_bits(d->fmt, 2);
    put_bits(d->cond, 2); put_bits(d->env, 2); put_bits(d->lvl, 2);
    put_bits(d->results, 8); put_bits(d->opts, 3); put_bits(d->trophies, 4);
    put_bits(5, 3);                     /* version */
}

void pw_encode(char *out, const PwData *d)
{
    unsigned int c;
    pu8 i, prev = 0, n;
    pack(d);
    c = check12();
    bitpos = 48;
    put_bits((pu8)(c & 0xFF), 8);
    put_bits((pu8)(c >> 8), 4);
    bitpos = 0;
    for (i = 0; i < PW_LEN; ++i) {
        n = (pu8)((get_bits(5) ^ (i * 7 + 3) ^ prev) & 31);
        out[i] = alpha[n];
        prev = n;
    }
}

pu8 pw_decode(const char *in, PwData *d)
{
    pu8 i, j, n, prev = 0;
    unsigned int c;
    for (i = 0; i < 8; ++i) buf[i] = 0;
    bitpos = 0;
    for (i = 0; i < PW_LEN; ++i) {
        n = 255;
        for (j = 0; j < 32; ++j) if (alpha[j] == in[i]) n = j;
        if (n == 255) return 0;
        put_bits((pu8)((n ^ (i * 7 + 3) ^ prev) & 31), 5);
        prev = n;
    }
    bitpos = 48;
    c = get_bits(8);
    c |= (unsigned int)get_bits(4) << 8;
    if (c != check12()) return 0;
    bitpos = 0;
    d->type = get_bits(3); d->stage = get_bits(4); d->seed = get_bits(8);
    d->chr = get_bits(4); d->surf = get_bits(3); d->fmt = get_bits(2);
    d->cond = get_bits(2); d->env = get_bits(2); d->lvl = get_bits(2);
    d->results = get_bits(8); d->opts = get_bits(3); d->trophies = get_bits(4);
    if (get_bits(3) != 5) return 0;
    if (d->type > 4 || d->chr > 9 || d->surf > 5 || d->cond > 2 || d->env > 2 || d->lvl > 2) return 0;
    return 1;
}
