/* Royal Martinot — écrans d'annonce des tournois du Grand Martinot (banque
 * 14, version 2.44) : au premier tour de chaque tournoi, une image (images de
 * tools/gen_gm.py), le nom du tournoi, sa surface, le tour, et sa fanfare.
 *  - Open d'Augustralie : l'Opéra de Sydney, l'eau de la baie scintille ;
 *  - Debogarros : la tour Eiffel qui scintille, au-dessus d'un court de terre ;
 *  - Martinbledon : Big Ben, dont la grande aiguille tourne (sprite) ;
 *  - US Opaul : la statue de la Liberté, raquette au poing ; les fenêtres
 *    de New York s'allument et s'éteignent.
 * Les tuiles de l'image prennent la place des tuiles de décor $60 à $FF
 * (rétablies par chr_borrow_restore, bg_dirty) ; l'aiguille, celle des 3
 * premières tuiles de la coupe (rétablies elles aussi). Les animations de
 * l'eau, de la tour et des fenêtres font tourner des couleurs de palette. */
#include "app.h"
#include "gfx.h"
#include "audio.h"
#include "game.h"
#include "comp.h"
#include "assets.h"

#pragma static-locals(off)   /* variables locales sur la pile : la RAM interne est pleine */

extern const unsigned char gm_ntiles[4], gm_hand[48];
extern const unsigned char * const gm_chr[4];
extern const unsigned char * const gm_nt[4];
extern const unsigned char * const gm_at[4];
extern const unsigned char * const gm_pal[4];
extern const unsigned char * const gm_anim[4];
extern u8 bg_dirty;

#pragma bss-name(push, "WBSS")
u8 gm_ev;                       /* tournoi annoncé (0 à 3), réglé par screens.c */
static u8 gm_t;
#pragma bss-name(pop)

static const char * const gmi_name[4] = { "OPEN D'AUGUSTRALIE", "DEBOGARROS", "MARTINBLEDON", "US OPAUL" };
static const char * const gmi_surf[4] = { "TOURNOI 1/4 : DUR", "TOURNOI 2/4 : TERRE BATTUE", "TOURNOI 3/4 : GAZON", "TOURNOI 4/4 : DUR" };

static void center(u8 y, const char *s)
{
    u8 n = 0;
    while (s[n]) ++n;
    txt_at((u8)((32 - n) >> 1), y, s);
}

void gm_intro_build(void)
{
    u8 k = (u8)(gm_ev & 3), i, j;
    const u8 *p;
    chr_copy(BANK_GMI, gm_chr[k], 0x0600, (u16)gm_ntiles[k] << 4);   /* tuiles $60 et suivantes */
    bg_dirty = 1;
    p = gm_nt[k];
    for (i = 0; i < 16; ++i) {
        vram_adr(NTADR(2, 6 + i));
        for (j = 0; j < 28; ++j) vram_put(*p++);
    }
    vram_adr(0x23C0);
    p = gm_at[k];
    for (i = 0; i < 64; ++i) vram_put(p[i]);
    p = gm_pal[k];
    for (i = 0; i < 16; ++i) pal_buf[i] = p[i];
    pal_buf[16] = p[0]; pal_buf[17] = 0x0F;          /* sprites : aiguille noire */
    pal_dirty = 1;
    center(2, "LE GRAND MARTINOT");
    center(4, gmi_name[k]);
    center(23, gmi_surf[k]);
    COMP(comp_stage_name, 0);
    center(25, comp_l2);
    center(27, "A : CONTINUER");
    if (k == 2) chr_copy(BANK_GMI, gm_hand, 0x1000 + ((u16)trophy_tile << 4), 48);
    music_play((u8)(MUS_GM1 + k));
    gm_t = 0;
}

/* à chaque trame : couleurs qui tournent, aiguille de Big Ben */
void gm_intro_tick(void)
{
    static const u8 dx[8] = { 128, 132, 132, 132, 128, 124, 124, 124 };
    static const u8 dy[8] = { 80, 80, 84, 88, 88, 88, 84, 80 };
    static const u8 tl[8] = { 0, 1, 2, 1, 0, 1, 2, 1 };
    static const u8 fl[8] = { 0, 0, 0, 0x80, 0x80, 0xC0, 0x40, 0x40 };
    u8 k = (u8)(gm_ev & 3), pal, n, i, j, s, q, d;
    const u8 *a = gm_anim[k], *p = gm_pal[k];
    ++gm_t;
    pal = a[0]; n = a[1];
    if (n && (gm_t & 7) == 0) {
        s = (u8)(gm_t >> 3);
        for (j = 1; j < 4; ++j)
            for (i = 0; i < n; ++i)
                if (p[pal * 4 + j] == a[2 + i]) {
                    q = (u8)(i + s);
                    while (q >= n) q -= n;
                    pal_buf[pal * 4 + j] = a[2 + q];
                }
        pal_dirty = 1;
    }
    spr_begin();
    if (k == 2) {
        /* centre du cadran : image (116, 40), soit l'écran (132, 88) ; une
           direction toutes les 16 trames (8 directions, tuiles retournées) */
        d = (u8)((gm_t >> 4) & 7);
        spr1(dx[d], dy[d], (u8)(trophy_tile + tl[d]), fl[d]);
    }
    spr_end();
}
