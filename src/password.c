/* Royal Martinot — codes de reprise (C portable, testé sur PC par
 * tests/password_test.c). Les 52 bits de données et 2 bits de version (2 :
 * ancien format à 16 personnages, 3 : format actuel, 1 : format des lieux 5
 * et 6, château et parc, où surface, lieu et niveau sont regroupés)
 * occupent les bits 0 à 53 (version 0 : le Grand Martinot, type 5, qui a ses
 * propres champs : résultats sur 12 bits, sans surface ni options) ; une somme de contrôle de 11 bits suit (bits 54
 * à 64), puis les 65 bits sont écrits en 13 symboles de 5 bits, brouillés en
 * chaîne pour qu'une lettre modifiée en change plusieurs. */
#include "password.h"

static const char alpha[] = PW_ALPHA;
static pu8 buf[9];
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
    for (i = 0; i < 7; ++i) {
        pu8 v = (pu8)(i == 6 ? (buf[6] & 0x3F) : buf[i]);   /* bits 0..53 */
        c = (unsigned int)(((c << 3) | (c >> 9)) & 0xFFF);
        c ^= v;
        c = (c + (unsigned int)v * 37u + i) & 0xFFF;
    }
    return c;
}

static void pack(const PwData *d)
{
    pu8 i;
    for (i = 0; i < 9; ++i) buf[i] = 0;
    bitpos = 0;
    if (d->type == 5) {
        /* version 0 : le Grand Martinot (surfaces imposées, 4 tournois de 16 :
           tours gagnés à chaque tournoi sur 3 bits) */
        put_bits(d->cond, 2); put_bits(d->stage, 4); put_bits(d->seed, 8);
        put_bits(d->chr, 5); put_bits(d->env, 3); put_bits(d->fmt, 2); put_bits(d->lvl, 2);
        put_bits((pu8)(d->gres & 0xFF), 8); put_bits((pu8)(d->gres >> 8), 4);
        put_bits(d->trophies, 4);
        put_bits(d->doubles, 1); put_bits(d->partner, 5); put_bits(d->hum2, 1);
        put_bits(0, 3);
        put_bits(0, 2);   /* version */
        return;
    }
    /* version 3 (jusqu'à 32 personnages, 21 aujourd'hui) : personnage et partenaire sur 5 bits ;
       la place vient du regroupement type x conditions (15 valeurs, 4 bits)
       et surface x lieu (30 valeurs, 5 bits) */
    put_bits((pu8)(d->type * 3 + d->cond), 4); put_bits(d->stage, 4); put_bits(d->seed, 8);
    put_bits(d->chr, 5);
    if (d->env <= 4) {
        put_bits((pu8)(d->surf * 5 + d->env), 5); put_bits(d->fmt, 2);
        put_bits(d->lvl, 2);
    } else {
        /* version 1 (lieux 5 et 6 : château, parc) : surface, lieu et niveau
           regroupés sur 7 bits ((surface x 7 + lieu) x 3 + niveau < 126) ;
           les codes des 5 premiers lieux ne changent pas */
        put_bits((pu8)((d->surf * 7 + d->env) * 3 + d->lvl), 7); put_bits(d->fmt, 2);
    }
    put_bits(d->results, 8); put_bits(d->opts, 3); put_bits(d->trophies, 4);
    put_bits(d->doubles, 1); put_bits(d->partner, 5); put_bits(d->hum2, 1);
    put_bits(d->env <= 4 ? 3 : 1, 2);   /* version */
}

/* v / k et v % k pour de petites valeurs, sans division */
static pu8 divk(pu8 v, pu8 k, pu8 *rem)
{
    pu8 q = 0;
    while (v >= k) { v = (pu8)(v - k); ++q; }
    *rem = v;
    return q;
}

void pw_encode(char *out, const PwData *d)
{
    unsigned int c;
    pu8 i, prev = 0, n;
    pack(d);
    c = check12();
    bitpos = 54;
    put_bits((pu8)(c & 0xFF), 8);
    put_bits((pu8)((c >> 8) & 7), 3);
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
    for (i = 0; i < 9; ++i) buf[i] = 0;
    bitpos = 0;
    for (i = 0; i < PW_LEN; ++i) {
        n = 255;
        for (j = 0; j < 32; ++j) if (alpha[j] == in[i]) n = j;
        if (n == 255) return 0;
        put_bits((pu8)((n ^ (i * 7 + 3) ^ prev) & 31), 5);
        prev = n;
    }
    bitpos = 54;
    c = get_bits(8);
    c |= (unsigned int)get_bits(3) << 8;
    if (c != (check12() & 0x7FF)) return 0;
    bitpos = 52;
    n = get_bits(2);                    /* version */
    bitpos = 0;
    d->gres = 0;
    if (n == 0) {
        d->type = 5; d->surf = 0; d->results = 0; d->opts = 0;
        d->cond = get_bits(2); d->stage = get_bits(4); d->seed = get_bits(8);
        d->chr = get_bits(5); d->env = get_bits(3); d->fmt = get_bits(2); d->lvl = get_bits(2);
        d->gres = get_bits(8);
        d->gres |= (unsigned short)get_bits(4) << 8;
        d->trophies = get_bits(4);
        d->doubles = get_bits(1); d->partner = get_bits(5); d->hum2 = get_bits(1);
        if (get_bits(3)) return 0;
        /* 3 bits par tournoi : 0 à 4 tours gagnés (0 à 3 en double) */
        {
            unsigned short v = d->gres;
            for (n = 0; n < 4; ++n) { if ((v & 7) > (d->doubles ? 3 : 4)) return 0; v >>= 3; }
        }
    } else if (n == 2) {
        /* ancien format (16 personnages) : toujours accepté */
        d->type = get_bits(3); d->stage = get_bits(4); d->seed = get_bits(8);
        d->chr = get_bits(4); d->surf = get_bits(3); d->fmt = get_bits(2);
        d->cond = get_bits(2); d->env = get_bits(3); d->lvl = get_bits(2);
        d->results = get_bits(8); d->opts = get_bits(3); d->trophies = get_bits(4);
        d->doubles = get_bits(1); d->partner = get_bits(4); d->hum2 = get_bits(1);
    } else if (n == 1) {
        n = get_bits(4);
        if (n >= 15) return 0;
        d->type = divk(n, 3, &d->cond);
        d->stage = get_bits(4); d->seed = get_bits(8);
        d->chr = get_bits(5);
        n = get_bits(7);
        if (n >= 126) return 0;
        n = divk(n, 3, &d->lvl);
        d->surf = divk(n, 7, &d->env);
        if (d->env <= 4) return 0;
        d->fmt = get_bits(2);
        d->results = get_bits(8); d->opts = get_bits(3); d->trophies = get_bits(4);
        d->doubles = get_bits(1); d->partner = get_bits(5); d->hum2 = get_bits(1);
    } else if (n == 3) {
        n = get_bits(4);
        if (n >= 15) return 0;
        d->type = divk(n, 3, &d->cond);
        d->stage = get_bits(4); d->seed = get_bits(8);
        d->chr = get_bits(5);
        n = get_bits(5);
        if (n >= 30) return 0;
        d->surf = divk(n, 5, &d->env);
        d->fmt = get_bits(2); d->lvl = get_bits(2);
        d->results = get_bits(8); d->opts = get_bits(3); d->trophies = get_bits(4);
        d->doubles = get_bits(1); d->partner = get_bits(5); d->hum2 = get_bits(1);
    } else return 0;
    if (bitpos != 52) return 0;
    if (d->chr >= PW_NCHARS || d->partner >= PW_NCHARS) return 0;
    if (d->env > 6) return 0;
    if (d->hum2 && !d->doubles) return 0;
    if (d->type > 5 || d->surf > 5 || d->cond > 2 || d->lvl > 2) return 0;
    if (d->doubles && d->partner == d->chr) return 0;
    /* Margaux ne joue jamais au club nudiste */
    if (d->env == 3 && (d->chr == 16 || (d->doubles && d->partner == 16))) return 0;
    return 1;
}
