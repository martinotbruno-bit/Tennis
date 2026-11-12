/* Royal Martinot — écrans d'annonce des tournois du Grand Martinot et du
 * Tour du monde (banque 14, versions 2.44 et 2.47) : au premier tour de chaque tournoi, une image (images de
 * tools/gen_gm.py), le nom du tournoi, sa surface, le tour, et sa fanfare.
 *  - Open d'Augustralie : l'Opéra de Sydney, l'eau de la baie scintille ;
 *  - Debogarros : la tour Eiffel qui scintille, au-dessus d'un court de terre ;
 *  - Martinbledon : Big Ben, dont la grande aiguille tourne (sprite) ;
 *  - US Opaul : la statue de la Liberté, raquette au poing ; les fenêtres
 *    de New York s'allument et s'éteignent ;
 *  - Saint-Martinotz (glace) : les Alpes, un chalet, une patinoire ; il neige ;
 *  - Copamargaux (plage) : Rio, le Pain de Sucre, la mer qui scintille ;
 *  - Bruno-Bercy (moquette) : Bercy la nuit, les projecteurs balaient le ciel.
 * Les images 4 à 6 sont dans la banque 6 (gm_bank).
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

extern const unsigned char gm_ntiles[7], gm_hand[64], gm_bank[7];
extern const unsigned char * const gm_chr[7];
extern const unsigned char * const gm_nt[7];
extern const unsigned char * const gm_at[7];
extern const unsigned char * const gm_pal[7];
extern const unsigned char * const gm_anim[7];
extern u8 bg_dirty;

#pragma bss-name(push, "WBSS")
u8 gm_ev;                       /* image du tournoi annoncé (0 à 6), réglé par screens.c */
static u8 gm_t;
#pragma bss-name(pop)

static const char * const gmi_name[7] = { "OPEN D'AUGUSTRALIE", "DEBOGARROS", "MARTINBLEDON", "US OPAUL",
                                          "SAINT-MARTINOTZ", "COPAMARGAUX", "BRUNO-BERCY" };
/* surface de chaque tournoi (image), et son rang dans le Tour du monde */
static const u8 gmi_surf[7] = { 2, 0, 1, 2, 5, 4, 3 };
static const u8 gmi_rank[7] = { 0, 2, 3, 5, 1, 4, 6 };
static const char * const sname[6] = { "TERRE BATTUE", "GAZON", "DUR", "MOQUETTE", "PLAGE", "GLACE" };

static void center(u8 y, const char *s)
{
    u8 n = 0;
    while (s[n]) ++n;
    txt_at((u8)((32 - n) >> 1), y, s);
}

void gm_intro_build(void)
{
    u8 k = gm_ev, i, b = gm_bank[gm_ev];
    const u8 *p;
    char line[28];
    const char *q;
    chr_copy(b, gm_chr[k], 0x0600, (u16)gm_ntiles[k] << 4);   /* tuiles $60 et suivantes */
    bg_dirty = 1;
    p = gm_nt[k];
    for (i = 0; i < 16; ++i) { chr_copy(b, p, NTADR(2, 6 + i), 28); p += 28; }
    chr_copy(b, gm_at[k], 0x23C0, 64);
    p = gm_pal[k];
    for (i = 0; i < 16; ++i) pal_buf[i] = p[i];
    pal_buf[16] = p[0]; pal_buf[17] = (u8)(k == 4 ? 0x30 : 0x0F);   /* sprites : aiguille noire, flocons blancs */
    pal_dirty = 1;
    center(2, c_type == CT_WORLD ? "LE TOUR DU MONDE" : "LE GRAND MARTINOT");
    center(4, gmi_name[k]);
    /* « TOURNOI 2/7 : GLACE » */
    q = "TOURNOI";
    for (i = 0; *q; ++i) line[i] = *q++;
    line[i++] = ' ';
    line[i++] = (char)('1' + (c_type == CT_WORLD ? gmi_rank[k] : k));
    line[i++] = '/';
    line[i++] = (char)(c_type == CT_WORLD ? '7' : '4');
    line[i++] = ' '; line[i++] = ':'; line[i++] = ' ';
    for (q = sname[gmi_surf[k]]; *q; ) line[i++] = *q++;
    line[i] = 0;
    center(23, line);
    COMP(comp_stage_name, 0);
    center(25, comp_l2);
    center(27, "A : CONTINUER");
    if (k == 2 || k == 4) chr_copy(BANK_GMI, gm_hand, 0x1000 + ((u16)trophy_tile << 4), 64);
    music_play((u8)(MUS_GM1 + k));
    gm_t = 0;
}

/* à chaque trame : couleurs qui tournent, aiguille de Big Ben, neige des Alpes */
void gm_intro_tick(void)
{
    static const u8 dx[8] = { 126, 130, 130, 130, 126, 122, 122, 122 };
    static const u8 dy[8] = { 82, 82, 86, 90, 90, 90, 86, 82 };
    static const u8 tl[8] = { 0, 1, 2, 1, 0, 1, 2, 1 };
    static const u8 fl[8] = { 0, 0, 0, 0x80, 0x80, 0xC0, 0x40, 0x40 };
    /* flocons : colonne, hauteur de départ, vitesse (1 ou 2 px toutes les 2 trames) */
    static const u8 sx[12] = { 22, 48, 71, 95, 118, 140, 163, 186, 210, 35, 130, 200 };
    static const u8 sy[12] = { 0, 70, 30, 100, 15, 85, 45, 110, 60, 20, 95, 5 };
    static const u8 sw[8] = { 0, 1, 2, 2, 2, 1, 0, 0 };
    u8 k = gm_ev, pal, n, i, j, s, q, d;
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
        /* centre du cadran : image (114, 42), soit l'écran (130, 90) ; une
           direction toutes les 16 trames (8 directions, tuiles retournées) */
        d = (u8)((gm_t >> 4) & 7);
        spr1(dx[d], dy[d], (u8)(trophy_tile + tl[d]), fl[d]);
    }
    if (k == 4) {
        /* neige sur l'image (écran : y 48 à 175), balancement de gauche à droite */
        for (i = 0; i < 12; ++i) {
            d = (u8)(gm_t >> 1);
            if (i & 1) d = (u8)(d + (d >> 1));          /* flocons plus rapides */
            q = (u8)((sy[i] + d) & 127);
            spr1((u8)(16 + sx[i] + sw[(u8)((gm_t >> 4) + i) & 7]), (u8)(48 + q), (u8)(trophy_tile + 3), 0);
        }
    }
    spr_end();
}
