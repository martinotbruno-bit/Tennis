/* Pixel Open — codes de sauvegarde (portable, testé sur PC).
 * Disposition des 24 bits (avant brouillage) :
 *   bits 0-1  manche du tournoi (0..2)     bits 2-4  personnage (0..5)
 *   bits 5-6  surface (0..3)               bits 7-8  format (0..3)
 *   bit  9    nuit                         bits 10-13 trophées (0..15)
 *   bits 14-15 version (=1)                bits 16-23 somme de contrôle */
#include "password.h"

static const char alpha[] = PW_ALPHA;
static const pu8 key[6] = { 0x9, 0x4, 0xE, 0x2, 0xB, 0x6 };

static pu8 checksum(pu8 lo, pu8 hi)
{
    pu8 c = (pu8)(lo * 7u + hi * 13u + 0x5Au);
    c ^= (pu8)((lo << 3) | (hi >> 5));
    return c;
}

void pw_encode(char *out, pu8 round, pu8 chr, pu8 surf, pu8 fmt, pu8 night, pu8 wins)
{
    unsigned int v = (unsigned int)((round & 3) | ((chr & 7) << 2) | ((surf & 3) << 5)
        | ((fmt & 3) << 7) | ((night & 1) << 9) | ((wins & 15) << 10) | (1u << 14));
    pu8 lo = (pu8)v, hi = (pu8)(v >> 8);
    pu8 c = checksum(lo, hi);
    pu8 nib[6];
    pu8 i, prev = 0;
    nib[0] = (pu8)(lo & 15); nib[1] = (pu8)(lo >> 4);
    nib[2] = (pu8)(hi & 15); nib[3] = (pu8)(hi >> 4);
    nib[4] = (pu8)(c & 15);  nib[5] = (pu8)(c >> 4);
    for (i = 0; i < 6; ++i) {
        /* brouillage chaîné : une lettre modifiée en change plusieurs */
        pu8 n = (pu8)((nib[i] ^ key[i] ^ prev) & 15);
        out[i] = alpha[n];
        prev = n;
    }
}

pu8 pw_decode(const char *in, pu8 *round, pu8 *chr, pu8 *surf, pu8 *fmt, pu8 *night, pu8 *wins)
{
    pu8 nib[6], i, j, prev = 0, lo, hi, c;
    unsigned int v;
    for (i = 0; i < 6; ++i) {
        pu8 n = 255;
        for (j = 0; j < 16; ++j) if (alpha[j] == in[i]) n = j;
        if (n == 255) return 0;
        nib[i] = (pu8)((n ^ key[i] ^ prev) & 15);
        prev = n;
    }
    lo = (pu8)(nib[0] | (nib[1] << 4));
    hi = (pu8)(nib[2] | (nib[3] << 4));
    c  = (pu8)(nib[4] | (nib[5] << 4));
    if (checksum(lo, hi) != c) return 0;
    v = (unsigned int)(lo | (hi << 8));
    if (((v >> 14) & 3) != 1) return 0;
    *round = (pu8)(v & 3);
    *chr = (pu8)((v >> 2) & 7);
    *surf = (pu8)((v >> 5) & 3);
    *fmt = (pu8)((v >> 7) & 3);
    *night = (pu8)((v >> 9) & 1);
    *wins = (pu8)((v >> 10) & 15);
    if (*round > 2 || *chr > 5) return 0;
    return 1;
}
