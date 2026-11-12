/* Royal Martinot — podium d'une compétition gagnée (banque BANK_PODIUM).
 *
 * Les 3 premiers montent sur un podium centré, posé sur le court de la
 * compétition, en taille double (32 x 48 px). La NES ne sait pas agrandir les
 * sprites : chaque personnage (6 tuiles de 8 x 8) est agrandi à l'écran
 * construit (rendu coupé) en 24 tuiles, chaque pixel devenant 2 x 2.
 *  - joueurs : tuiles du DÉCOR (à partir de $60, à la place des tuiles du
 *    court, du logo, etc., inutiles ici), une palette du décor chacun ;
 *  - coupe agrandie (et, en double, le 3e) : tuiles de SPRITES, à la place des
 *    personnages vus de dos (inutiles ici).
 * Toute la CHR-RAM est rechargée depuis la banque 2 en quittant l'écran
 * (chr_reload).
 *
 * Palettes du décor : 0 = court (fond), haut des marches, marches, blanc ;
 * 1 à 3 = un joueur chacune. En double, il faudrait 6 palettes : les deux
 * vainqueurs et le capitaine finaliste sont dans le décor, le capitaine
 * classé 3e est en sprites (24 sprites, palette de sprites 0) ; le nom des
 * équipes complètes est écrit sous chaque marche. */
#include "app.h"
#include "gfx.h"
#include "audio.h"
#include "assets.h"
#include "game.h"
#include "comp.h"

#pragma static-locals(on)

#define PPU_DATA (*(volatile u8 *)0x2007)

#define T_PLAYER 0x60          /* 3 joueurs x 24 tuiles du décor : $60..$A7 */
#define T_TOP    0xA8          /* haut de marche */
#define T_FACE   0xA9          /* marche */
#define T_DIGIT  0xAA          /* chiffres 1, 2, 3 sur les marches ($AA..$AC) */
#define T_HL     0xAD          /* ligne blanche horizontale */
#define T_VL     0xAE          /* ligne blanche verticale */
#define T_CTL    0xAF          /* coin haut-gauche */
#define T_CTR    0xB0          /* coin haut-droite */
#define S_CUP    0x10          /* coupe agrandie : 16 tuiles de sprites */
#define S_THIRD  0x20          /* 3e (double) : 24 tuiles de sprites */

/* remise des médailles (compétitions classiques, version 2.53) : RAM de
   cartouche libre (bloc SAVEDATA, non sauvegardé) */
#define S_MED    0x38          /* ruban, médailles or, argent, bronze : 4 tuiles de sprites */
#define MED_T0   210           /* trames de la remise, avant la coupe */
#pragma bss-name(push, "SAVEDATA")
u8 pod_busy;                   /* 1 : la cérémonie retient A (screens.c) */
static u8 gm_st, gm_qa, gm_q0;
static u16 pod_t;
static u8 pal_sv[16], med_x[4], med_y[4], med_ty[4], med_c[4], n_med, fl_t, medals;
#pragma bss-name(pop)
#pragma bss-name(push, "WBSS")
u8 pod_new;
static u8 src[16], out[64], at[64];
static u8 cup_t, fw_c, third_c;
#pragma bss-name(pop)

/* un quartet de pixels doublé : chaque bit devient 2 bits */
static const u8 dbl[16] = { 0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F,
                            0xC0, 0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF };
/* variante de bras de la tête de Margaux par pose (comme sprp.s) */
/* couleur de la surface (fond universel) ; Margaux : rose */
static const u8 surf_col[7] = { 0x26, 0x2A, 0x12, 0x13, 0x28, 0x21, 0x25 };

/* une tuile source (adresse PPU sa) -> 4 tuiles agrandies (haut-gauche,
   haut-droite, bas-gauche, bas-droite) à partir de l'adresse PPU da */
static void dbl_tile(u16 sa, u16 da)
{
    u8 i, b, h;
    vram_adr(sa);
    src[0] = PPU_DATA;               /* lecture à vide (tampon du PPU) ; « (void) » serait supprimé par cc65 */
    for (i = 0; i < 16; ++i) src[i] = PPU_DATA;
    for (i = 0; i < 8; ++i) {
        h = (u8)(i >> 1);
        b = src[h];      out[i] = dbl[b >> 4];      out[16 + i] = dbl[b & 15];
        b = src[8 + h];  out[8 + i] = dbl[b >> 4];  out[24 + i] = dbl[b & 15];
        b = src[4 + h];  out[32 + i] = dbl[b >> 4]; out[48 + i] = dbl[b & 15];
        b = src[12 + h]; out[40 + i] = dbl[b >> 4]; out[56 + i] = dbl[b & 15];
    }
    vram_adr(da);
    for (i = 0; i < 64; ++i) vram_put(out[i]);
}

/* personnage c, vu de face, dans la pose donnée : 24 tuiles à partir de la
   tuile dst de la table base ($0000 décor, $1000 sprites). Tuile source k
   (rangée k/2, colonne k&1) -> tuiles dst + 4k .. dst + 4k + 3 */
static void big_person(u8 c, u8 pose, u16 base, u8 dst)
{
    u8 k, s, m = (u8)(person_row[1] + pose * 3);
    const u8 *map = char_female[c] ? person_map_f : person_map_m;
    for (k = 0; k < 6; ++k) {
        s = (u8)(map[m + (k >> 1)] + (k & 1));
        if (c == 16 && k < 2) s = (u8)(marg_map[NPOSE + pose] + k);   /* Margaux (de face) */
        dbl_tile((u16)(0x1000 + ((u16)s << 4)), (u16)(base + ((u16)(u8)(dst + k * 4) << 4)));
    }
}

/* palette du bloc de 16 x 16 px (colonne bx, rangée by de blocs) */
static void blk(u8 bx, u8 by, u8 pal)
{
    u8 i = (u8)((by >> 1) * 8 + (bx >> 1)), sh = (u8)(((by & 1) << 2) | ((bx & 1) << 1));
    at[i] = (u8)((at[i] & (u8)~(3 << sh)) | (pal << sh));
}

/* joueur agrandi dans le décor : coin (x, y) en tuiles (pairs), palette pal */
static void put_big(u8 x, u8 y, u8 dst, u8 pal)
{
    u8 r, c;
    for (r = 0; r < 6; ++r) {
        vram_adr(NTADR(x, y + r));
        for (c = 0; c < 4; ++c)
            vram_put((u8)(dst + ((r >> 1) * 2 + (c >> 1)) * 4 + (r & 1) * 2 + (c & 1)));
    }
    for (r = 0; r < 3; ++r) { blk((u8)(x >> 1), (u8)((y >> 1) + r), pal); blk((u8)((x >> 1) + 1), (u8)((y >> 1) + r), pal); }
}

/* marche : colonnes x .. x+w-1, de la rangée top à la rangée 24 ; chiffre n */
static void step(u8 x, u8 w, u8 top, u8 n)
{
    u8 r;
    vram_adr(NTADR(x, top));
    vram_fill(T_TOP, w);
    for (r = (u8)(top + 1); r <= 24; ++r) {
        vram_adr(NTADR(x, r));
        vram_fill(T_FACE, w);
    }
    vram_adr(NTADR(x + ((w - 1) >> 1), top + 1));
    vram_put((u8)(T_DIGIT + n - 1));
}

static u8 slen(const char *s) { u8 n = 0; while (s[n]) ++n; return n; }
static void txt_c(u8 y, const char *s) { txt_at((u8)((32 - slen(s)) >> 1), y, s); }
/* nom (simple) ou équipe (double) centré sous une marche */
static void label(u8 x, u8 w, u8 i)
{
    static char b[8];
    u8 n;
    if (c_dbl) team_label(c_pl[i], c_pl2[i], b);
    else {
        /* simple : nom entier (MARGAUX déborde d'une case, toujours centré) */
        const char *p = char_name[c_pl[i]];
        for (n = 0; n < 7 && p[n] > ' '; ++n) b[n] = p[n];
        b[n] = 0;
    }
    txt_at((u8)(x + (w >> 1) - (slen(b) >> 1)), 26, b);
}

/* tuile unie de 8 octets répétés par plan */
static void tile8(u8 t, u8 p0, u8 p1)
{
    u8 i;
    vram_adr((u16)t << 4);
    for (i = 0; i < 8; ++i) vram_put(p0);
    for (i = 0; i < 8; ++i) vram_put(p1);
}
static void make_tiles(void)
{
    u8 i, n;
    /* haut de marche : 2 lignes claires (couleur 1), puis la marche (couleur 2) */
    vram_adr((u16)T_TOP << 4);
    for (i = 0; i < 8; ++i) vram_put((u8)(i < 2 ? 0xFF : 0x00));
    for (i = 0; i < 8; ++i) vram_put((u8)(i < 2 ? 0x00 : 0xFF));
    tile8(T_FACE, 0x00, 0xFF);
    /* chiffres blancs (couleur 3) sur la marche (couleur 2), d'après la police */
    for (n = 0; n < 3; ++n) {
        vram_adr((u16)('1' + n) << 4);
        src[0] = PPU_DATA;               /* lecture à vide (tampon du PPU) ; « (void) » serait supprimé par cc65 */
        for (i = 0; i < 8; ++i) src[i] = PPU_DATA;
        vram_adr((u16)(T_DIGIT + n) << 4);
        for (i = 0; i < 8; ++i) vram_put(src[i]);
        for (i = 0; i < 8; ++i) vram_put(0xFF);
    }
    /* lignes du court (couleur 3) */
    vram_adr((u16)T_HL << 4);
    for (n = 0; n < 2; ++n) for (i = 0; i < 8; ++i) vram_put((u8)(i == 3 || i == 4 ? 0xFF : 0x00));
    tile8(T_VL, 0x18, 0x18);
    vram_adr((u16)T_CTL << 4);
    for (n = 0; n < 2; ++n) for (i = 0; i < 8; ++i) vram_put((u8)(i < 3 ? 0x00 : i < 5 ? 0x1F : 0x18));
    vram_adr((u16)T_CTR << 4);
    for (n = 0; n < 2; ++n) for (i = 0; i < 8; ++i) vram_put((u8)(i < 3 ? 0x00 : i < 5 ? 0xF8 : 0x18));
}

/* tuiles de sprites des médailles : ruban en V (couleur 2), disques or (1),
   argent (2), bronze (3) */
static void medal_tiles(void)
{
    static const u8 rib[8] = { 0x81, 0x42, 0x42, 0x24, 0x24, 0x18, 0x18, 0x00 };
    static const u8 disc[8] = { 0x00, 0x3C, 0x7E, 0x7E, 0x7E, 0x7E, 0x3C, 0x00 };
    u8 i, k;
    vram_adr(0x1000 + ((u16)S_MED << 4));
    for (i = 0; i < 8; ++i) vram_put(0);
    for (i = 0; i < 8; ++i) vram_put(rib[i]);
    for (k = 1; k < 4; ++k) {
        for (i = 0; i < 8; ++i) vram_put((u8)(k & 1 ? disc[i] : 0));
        for (i = 0; i < 8; ++i) vram_put((u8)(k & 2 ? disc[i] : 0));
    }
}
static void medal_at(u8 x, u8 row, u8 c)
{
    med_x[n_med] = x; med_ty[n_med] = (u8)(row * 8 + 18); med_y[n_med] = 0; med_c[n_med] = c;
    ++n_med;
}

/* ====================================================================== */
/* Grand Martinot et Martinworld Tour (version 2.53) : remise du trophée,   */
/* puis conférence de presse.                                               */
/*  - tapis rouge jusqu'à la table des trophées ; ramasseurs de balles en   */
/*    haie d'honneur ; le finaliste monte recevoir sa plaque, salue et se   */
/*    range ; le vainqueur monte, reçoit la coupe, l'embrasse, puis la      */
/*    soulève sous les confettis et les feux d'artifice ;                   */
/*  - conférence de presse : le champion derrière la table, micros, flashs */
/*    des photographes, trois questions et leurs réponses.                  */
/* A avance (pod_busy : screens.c ne quitte l'écran qu'à la fin).           */
/* ====================================================================== */
#pragma static-locals(off)          /* variables locales sur la pile : RAM pleine */
#define G_CARPET 0x60          /* tuiles du décor : tapis, liseré, table, nappe, toile */
#define G_EDGE   0x61
#define G_TABLE  0x62
#define G_CLOTH  0x63
#define G_WALL   0x64
#define G_END    560           /* trophée soulevé : fin de la remise */
#define P_IDLE_  0
#define P_JOY_   6
#define P_SERVE_ 5
static const u8 run4g[4] = { 1, 19, 2, 20 };
static const u8 kid_x[4] = { 100, 156, 100, 156 };
static const u8 kid_y[4] = { 150, 150, 200, 200 };
static const u8 kid_c[4] = { 3, 7, 9, 12 };
static void gm_head(void)
{
    txt_c(2, c_type == CT_GRAND ? "LE GRAND MARTINOT" : "MARTINWORLD TOUR");
    txt_c(4, c_slam ? "GRAND CHELEM !" : "CHAMPION !");
}
static void gm_build(void)
{
    u8 i, r;
    gm_st = 1; pod_busy = 1; pod_t = 0;
    /* tuiles : tapis rouge (1), liseré doré (3 sur 1), table (2), nappe (3) */
    tile8(G_CARPET, 0xFF, 0x00);
    vram_adr((u16)G_EDGE << 4);
    for (i = 0; i < 8; ++i) vram_put(0xFF);
    for (i = 0; i < 8; ++i) vram_put(0x81);
    tile8(G_TABLE, 0x00, 0xFF);
    tile8(G_CLOTH, 0xFF, 0xFF);
    vram_adr((u16)G_WALL << 4);
    for (i = 0; i < 8; ++i) vram_put((u8)(i & 1 ? 0x22 : 0x88));
    for (i = 0; i < 8; ++i) vram_put(0);
    medal_tiles();
    /* tapis : colonnes 14 à 17, des rangées 11 à 29 ; table : rangées 9-10 */
    for (r = 11; r < 30; ++r) {
        vram_adr(NTADR(13, r)); vram_put(G_EDGE);
        vram_fill(G_CARPET, 4); vram_put(G_EDGE);
    }
    vram_adr(NTADR(11, 9)); vram_fill(G_CLOTH, 10);
    vram_adr(NTADR(11, 10)); vram_fill(G_TABLE, 10);
    gm_head();
    if (pod_new) txt_c(6, "MODE NUDISTE DEBLOQUE !");
    /* palettes : fond = surface ; rouge, brun, blanc ; sprites : 0
       vainqueur, 1 finaliste, 2 ramasseurs et officiel, 3 or, argent, blanc */
    pal_buf[0] = pal_buf[16] = surf_col[c_env == ENV_MARGAUX ? 6 : 2];
    pal_buf[1] = 0x16; pal_buf[2] = 0x07; pal_buf[3] = 0x30;
    pal_set_player(0, c_pl[0]);
    pal_set_player(1, c_pl[c_pod[1]]);
    pal_set_player(2, 3);
    pal_buf[16 + 12] = pal_buf[0]; pal_buf[16 + 13] = 0x28; pal_buf[16 + 14] = 0x10; pal_buf[16 + 15] = 0x30;
    pal_dirty = 1;
}
/* conférence de presse : toile de fond, table et micros */
static const char * const gm_q[12] = {
    "UN MOT SUR CETTE FINALE ?",     "J'AI JOUE COMME UN MARTINOT :", "AVEC LE COEUR.",
    "QUI REMERCIEZ-VOUS ?",          "DEBORAH, POUR LES GOUTERS,",    "ET MON REVERS, QUI A TENU.",
    "AUGUSTIN VEUT LA COUPE.",       "OUI, MAIS PAS POUR Y RANGER",   "SES CHAUSSETTES.",
    "PAUL VOUS DEFIE AU PING-PONG.", "J'ACCEPTE, SI MARGAUX",          "FAIT L'ARBITRE.",
};
static void gm_text(void)
{
    u8 k = (u8)(((gm_q0 + gm_qa) & 3) * 3), r;
    ppu_off();                       /* (écriture hors du rendu : une image noire) */
    for (r = 5; r < 13; ++r) { vram_adr(NTADR(1, r)); vram_fill(' ', 30); }
    txt_at(2, 5, "QUESTION :"); txt_c(7, gm_q[k]);
    txt_at(2, 9, "REPONSE :"); txt_c(11, gm_q[k + 1]); txt_c(12, gm_q[k + 2]);
    if (gm_qa == 2) { txt_c(27, "A : CONTINUER"); pod_busy = 0; }   /* A quitte l'écran */
    ppu_on();
}
static void gm_press_build(void)
{
    u8 r;
    ppu_off();
    vram_adr(0x2000);
    for (r = 0; r < 30; ++r) vram_fill(r >= 21 ? (r == 21 ? G_CLOTH : G_TABLE) : (r >= 15 && r < 21 ? G_WALL : 0), 32);
    vram_adr(0x23C0); vram_fill(0, 64);
    pal_buf[0] = 0x01; pal_buf[1] = 0x11; pal_buf[2] = 0x07; pal_buf[3] = 0x30;
    pal_dirty = 1;
    txt_c(2, "CONFERENCE DE PRESSE");
    gm_qa = 0;
    gm_q0 = (u8)(rand8() & 1);
    gm_st = 2; pod_t = 0;
    gm_text();
}
static void gm_tick(void)
{
    u8 i, x, y, f, hy, cupx, cupy;
    u16 t;
    if (pod_t < 0xFFFF) ++pod_t;
    t = pod_t;
    if (gm_st == 1) {
        if ((pad1_new & (PAD_A | PAD_START)) && t > 60) {      /* (pas l'appui qui a mené ici) */
            if (t < G_END) { pod_t = t = G_END; }
            else { gm_press_build(); return; }
        }
        spr_begin();
        cupx = 124; cupy = 66;                      /* coupe sur la table */
        /* le vainqueur : monte le tapis (266-390), reçoit la coupe,
           l'embrasse (420-480), la soulève */
        if (t >= 266) {
            y = (u8)(t < 390 ? 236 - (t - 266) : 112);
            f = (u8)(t < 390 ? run4g[(t >> 3) & 3] : (t < 480 ? P_IDLE_ : P_SERVE_));
            spr_person(128, y, c_pl[0], (u8)(t >= 390), f, 0, 0);
            if (c_dbl) spr_person(150, (u8)(y + 6), c_pl2[0], (u8)(t >= 390), f, 0, 0);
            if (t >= 390) {
                hy = (u8)(t < 420 ? 66 + ((t - 390) * 2 / 3) : t < 480 ? 84 : 66 - ((t >> 3) & 1));
                cupx = 120; cupy = hy;
            }
        }
        /* la coupe (4 tuiles) */
        spr1(cupx, cupy, trophy_tile, 3); spr1((u8)(cupx + 8), cupy, (u8)(trophy_tile + 1), 3);
        spr1(cupx, (u8)(cupy + 8), (u8)(trophy_tile + 2), 3); spr1((u8)(cupx + 8), (u8)(cupy + 8), (u8)(trophy_tile + 3), 3);
        /* le finaliste : monte (40-164), reçoit la plaque (164-214), se range à gauche */
        if (t >= 40) {
            if (t < 164) { x = 128; y = (u8)(236 - (t - 40)); f = run4g[(t >> 3) & 3]; }
            else if (t < 214) { x = 128; y = 112; f = P_IDLE_; }
            else if (t < 266) { x = (u8)(128 - (t - 214)); y = 112; f = run4g[(t >> 3) & 3]; }
            else { x = 76; y = 112; f = (u8)((t >> 4) & 1 ? P_JOY_ : P_IDLE_); }
            spr_person(x, y, c_pl[c_pod[1]], (u8)(t >= 164), f, 0, 1);
            /* la plaque : sur la table, puis dans ses mains */
            spr1((u8)(t < 184 ? 140 : x - 4), (u8)(t < 184 ? 70 : y - 14), (u8)(S_MED + 2), 3);
        } else spr1(140, 70, (u8)(S_MED + 2), 3);
        if (t == 184 || t == 420) sfx_crowd(5);
        if (t == 480) { sfx_crowd(7); music_play(MUS_VICTORY); }
        /* l'officiel, à droite de la table ; les ramasseurs en haie d'honneur */
        spr_person(176, 100, 5, 1, P_IDLE_, 1, 2);
        for (i = 0; i < (u8)(c_dbl ? 2 : 4); ++i) spr_person(kid_x[i], kid_y[i], kid_c[i], 1, P_IDLE_, (u8)(i & 1), 2);
        /* trophée soulevé : confettis et feux d'artifice */
        if (t >= 480) {
            for (i = 0; i < 8; ++i) spr1((u8)(i * 31 + 9), (u8)(((t + i * 29) * (u8)(1 + (i & 1))) & 0xFF), confetti_tile, (u8)(i & 3));
            fw_draw((u8)(130 - ((t - 480) % 130)), 0);
            fw_sound((u8)(130 - ((t - 480) % 130)));
        }
        if (t == G_END) txt_c(28, "A : CONFERENCE DE PRESSE");
        spr_end();
        return;
    }
    /* conférence de presse */
    if ((pad1_new & (PAD_A | PAD_START)) || (gm_qa < 2 && t > 300)) {
        if (gm_qa < 2) { ++gm_qa; pod_t = 0; gm_text(); }
    }
    /* flashs des photographes */
    if ((u8)(pod_t * 23) < 9) { pal_buf[0] = (u8)((pod_t & 1) ? 0x30 : 0x01); pal_dirty = 1; }
    else if (pal_buf[0] != 0x01) { pal_buf[0] = 0x01; pal_dirty = 1; }
    spr_begin();
    /* micros (balles noires sur la table) ; le champion derrière la table */
    spr1(120, 158, (u8)(ball_base + 4), 2); spr1(136, 158, (u8)(ball_base + 4), 2);
    spr_person(128, (u8)(184), c_pl[0], 1, (u8)(gm_qa == 1 && (t >> 4) & 1 ? P_JOY_ : P_IDLE_), 0, 0);
    if (c_dbl) spr_person(152, 184, c_pl2[0], 1, P_IDLE_, 0, 0);
    spr_end();
}
#pragma static-locals(on)

void podium_build(void)
{
    static char b[32];
    u8 i, sc, w, x1, x2, x3, wc, light;
    COMP(comp_podium, 0);
    pod_busy = 0; gm_st = 0;
    if (c_type >= CT_GRAND) { gm_build(); return; }
    for (i = 0; i < 64; ++i) at[i] = 0;
    make_tiles();

    /* palettes : court, marches grises, blanc (noir sur les surfaces claires) */
    sc = c_env == ENV_MARGAUX ? 6 : c_surf;
    light = (u8)(sc == 4 || sc == 5);
    pal_buf[0] = pal_buf[16] = surf_col[sc];
    pal_buf[1] = 0x10; pal_buf[2] = 0x00; pal_buf[3] = light ? 0x0F : 0x30;
    /* joueurs du décor : pal_set_player(61..63) écrit les palettes 1..3 du
       décor (16 + 4 x slot reboucle sur 8 bits) */
    pal_set_player(61, c_pl[0]);
    pal_set_player(62, c_dbl ? c_pl2[0] : c_pl[c_pod[1]]);
    pal_set_player(63, c_dbl ? c_pl[c_pod[1]] : c_pl[c_pod[2]]);
    third_c = c_pl[c_pod[2]];
    /* sprites : 0 le 3e en double (sinon le vainqueur), 1 le 2e ; confettis */
    pal_set_player(0, c_dbl ? third_c : c_pl[0]);
    pal_set_player(1, c_pl[c_pod[1]]);

    /* le court : lignes de fond et de côté */
    vram_adr(NTADR(2, 9)); vram_put(T_CTL); vram_fill(T_HL, 26); vram_put(T_CTR);
    for (i = 10; i < 30; ++i) {
        vram_adr(NTADR(2, i)); vram_put(T_VL);
        vram_adr(NTADR(29, i)); vram_put(T_VL);
    }

    /* podium centré : 2e à gauche, 1er au milieu (plus haut), 3e à droite */
    wc = c_dbl ? 10 : 6;
    w = (u8)(6 + wc + 6);
    x2 = (u8)((32 - w) >> 1); x1 = (u8)(x2 + 6); x3 = (u8)(x1 + wc);
    step(x2, 6, 20, 2);
    step(x1, wc, 18, 1);
    step(x3, 6, 22, 3);

    /* joueurs agrandis : vainqueur(s) bras levés, les autres au repos */
    if (c_dbl) {
        big_person(c_pl[0], POSE_JOY, 0x0000, T_PLAYER);
        big_person(c_pl2[0], POSE_JOY, 0x0000, T_PLAYER + 24);
        big_person(c_pl[c_pod[1]], POSE_IDLE, 0x0000, T_PLAYER + 48);
        big_person(third_c, POSE_IDLE, 0x1000, S_THIRD);
        put_big((u8)(x1 + 1), 12, T_PLAYER, 1);
        put_big((u8)(x1 + 5), 12, T_PLAYER + 24, 2);
        put_big((u8)(x2 + 1), 14, T_PLAYER + 48, 3);
    } else {
        big_person(c_pl[0], POSE_JOY, 0x0000, T_PLAYER);
        big_person(c_pl[c_pod[1]], POSE_IDLE, 0x0000, T_PLAYER + 24);
        big_person(c_pl[c_pod[2]], POSE_IDLE, 0x0000, T_PLAYER + 48);
        put_big((u8)(x1 + 1), 12, T_PLAYER, 1);
        put_big((u8)(x2 + 1), 14, T_PLAYER + 24, 2);
        put_big((u8)(x3 + 1), 16, T_PLAYER + 48, 3);
    }
    /* coupe agrandie (sprites) */
    for (i = 0; i < 4; ++i)
        dbl_tile((u16)(0x1000 + ((u16)(u8)(trophy_tile + i) << 4)), (u16)(0x1000 + ((u16)(u8)(S_CUP + i * 4) << 4)));

    label(x2, 6, c_pod[1]);
    label(x1, wc, 0);
    label(x3, 6, c_pod[2]);

    /* textes centrés */
    for (i = 0; i < 15; ++i) b[i] = "ROYAL MARTINOT "[i];
    {
        static const char * const pod_name[NCT] = { "TOURNOI", "POULES", "CHAMPIONNAT", "SAISON", "PERSONNALISE", "GRAND MARTINOT", "MARTINWORLD" };
        const char *n = pod_name[c_type];
        u8 k = 15;
        while (*n && k < 15 + 12) b[k++] = *n++;
        b[k] = 0;
    }
    txt_c(2, c_type == CT_GRAND ? "LE GRAND MARTINOT" : c_type == CT_WORLD ? "MARTINWORLD TOUR" : b);
    /* Grand Martinot, Martinworld Tour : tous les tournois gagnés, c'est le Grand Chelem */
    txt_c(4, c_type >= CT_GRAND && c_slam ? "GRAND CHELEM !" :
             c_type == CT_LEAGUE || c_type == CT_SEASON || c_type >= CT_GRAND ? "CHAMPION !" : "VAINQUEUR !");
    for (i = 0; i < 11; ++i) b[i] = "TROPHEES : "[i];
    num2(b + 11, c_trophies); b[13] = 0;
    txt_c(5, b);
    if (pod_new) txt_c(7, "MODE NUDISTE DEBLOQUE !");
    txt_c(28, "A : RETOUR AU TITRE");

    vram_adr(0x23C0);
    for (i = 0; i < 64; ++i) vram_put(at[i]);
    /* remise des médailles (compétitions classiques) : les joueurs
       apparaissent un à un (palettes du décor 1 à 3 masquées), puis les
       médailles descendent ; la coupe vient ensuite */
    medals = (u8)(c_type < CT_GRAND);
    pod_t = medals ? 0 : MED_T0;
    n_med = 0; fl_t = 0;
    for (i = 0; i < 16; ++i) pal_sv[i] = pal_buf[i];
    if (medals) {
        medal_tiles();
        for (i = 5; i < 16; ++i) if (i & 3) pal_buf[i] = pal_buf[0];
        pal_buf[16 + 12] = pal_buf[0]; pal_buf[16 + 13] = 0x28; pal_buf[16 + 14] = 0x10; pal_buf[16 + 15] = 0x17;
        if (c_dbl) {
            medal_at((u8)((x1 + 1) * 8 + 12), 12, 1); medal_at((u8)((x1 + 5) * 8 + 12), 12, 1);
            medal_at((u8)((x2 + 1) * 8 + 12), 14, 2); medal_at((u8)((((32 - 22) >> 1) + 16 + 1) * 8 + 12), 16, 3);
        } else {
            medal_at((u8)((x1 + 1) * 8 + 12), 12, 1); medal_at((u8)((x2 + 1) * 8 + 12), 14, 2); medal_at((u8)((x3 + 1) * 8 + 12), 16, 3);
        }
    }
    pal_dirty = 1;
    cup_t = 0;
}

extern u8 oam_buf[256];
extern u8 oam_i;
#pragma zpsym("oam_i")
static const u8 conf_x[10] = { 20, 52, 84, 116, 148, 180, 212, 236, 36, 132 };
static const u8 conf_v[10] = { 1, 2, 1, 3, 2, 1, 2, 3, 2, 1 };
static const u8 conf_y0[10] = { 16, 39, 62, 85, 108, 131, 154, 177, 200, 23 };
#pragma bss-name(push, "SAVEDATA")    /* (RAM de cartouche libre, non sauvegardée : remis à zéro à chaque levée de coupe) */
static u8 conf_y[10];
#pragma bss-name(pop)
/* 4 x 4 sprites (coupe) ou 4 x 6 (joueur) à partir de l'entrée o de l'OAM */
static void big_spr(u8 o, u8 x, u8 y, u8 t, u8 rows, u8 pal)
{
    u8 r, c;
    for (r = 0; r < rows; ++r)
        for (c = 0; c < 4; ++c) {
            oam_buf[o] = (u8)(y + r * 8 - 1);
            oam_buf[o + 1] = (u8)(t + ((r >> 1) * 2 + (c >> 1)) * 4 + (r & 1) * 2 + (c & 1));
            oam_buf[o + 2] = pal;
            oam_buf[o + 3] = (u8)(x + c * 8);
            o += 4;
        }
}
/* remise des médailles : apparitions, flashs des photographes, médailles
   (entrées 54 à 61 de l'OAM, réécrites à chaque trame) */
static void reveal(u8 pal)
{
    u8 i;
    for (i = 1; i < 4; ++i) pal_buf[pal * 4 + i] = pal_sv[pal * 4 + i];
    pal_dirty = 1;
    fl_t = 3;
    sfx_crowd(4);
}
static void medal_tick(void)
{
    u8 i, o, k;
    if (pod_t == 20) { if (c_dbl) big_spr(120, (u8)((((32 - 22) >> 1) + 16 + 1) * 8), 128, S_THIRD, 6, 0); else reveal(3); }
    if (pod_t == 60) reveal(c_dbl ? 3 : 2);
    if (pod_t == 100) { reveal(1); if (c_dbl) reveal(2); music_play(MUS_MEDAL); }
    /* flashs des photographes : le fond blanchit 2 trames, de temps en temps */
    if (pod_t > 100 && pod_t < 420 && (u8)(pod_t * 37) < 6) fl_t = 2;
    if (fl_t) { --fl_t; pal_buf[0] = fl_t ? 0x30 : pal_sv[0]; pal_dirty = 1; }
    /* médailles : bronze, argent, puis or, du haut de l'écran au cou */
    for (i = 0; i < n_med; ++i) {
        k = (u8)(med_c[i] == 3 ? 120 : med_c[i] == 2 ? 145 : 170);
        if (pod_t >= k && med_y[i] < med_ty[i]) { med_y[i] += 3; if (med_y[i] > med_ty[i]) med_y[i] = med_ty[i]; }
        o = (u8)(216 + i * 8);
        if (pod_t < k) { oam_buf[o] = oam_buf[o + 4] = 0xFF; continue; }
        oam_buf[o] = (u8)(med_y[i] - 9); oam_buf[o + 1] = S_MED; oam_buf[o + 2] = 3; oam_buf[o + 3] = med_x[i];
        oam_buf[o + 4] = (u8)(med_y[i] - 1); oam_buf[o + 5] = (u8)(S_MED + med_c[i]); oam_buf[o + 6] = 3; oam_buf[o + 7] = med_x[i];
    }
}

/* Coupe brandie au-dessus du ou des vainqueurs, feux d'artifice, confettis ;
   en double, le 3e en sprites agrandis. Pour tenir dans une image, seuls les
   éléments qui bougent sont réécrits à chaque trame, directement dans la
   copie de l'OAM : entrées 0-15 la coupe (hauteur), 16-19 les feux
   d'artifice, 20-29 les confettis (hauteur, par addition), 30-53 le 3e
   (écrit une seule fois). */
void podium_sprites(void)
{
    u8 i, o, cy, y;
    if (gm_st) { gm_tick(); return; }
    if (pod_t < 0xFFFF) ++pod_t;
    if (medals) {
        if (pod_t == 1) for (i = 0; i < 64; ++i) oam_buf[i * 4] = 0xFF;
        medal_tick();
        if (pod_t < MED_T0) return;
    }
    if (cup_t == 0) {
        /* première trame : tout masqué, puis coupe, confettis et 3e en place */
        for (i = 0; i < (u8)(medals ? 30 : 64); ++i) oam_buf[i * 4] = 0xFF;   /* (le 3e et les médailles restent) */
        big_spr(0, 112, 80, S_CUP, 4, 2);
        for (i = 0; i < 10; ++i) {
            o = (u8)(80 + i * 4);
            conf_y[i] = conf_y0[i];
            oam_buf[o + 1] = confetti_tile; oam_buf[o + 2] = (u8)(i & 3); oam_buf[o + 3] = conf_x[i];
        }
        if (c_dbl && !medals) big_spr(120, (u8)((((32 - 22) >> 1) + 16 + 1) * 8), 128, S_THIRD, 6, 0);
    }
    if (cup_t < 255) ++cup_t;
    /* la coupe monte des mains au-dessus de la tête, puis oscille */
    if (cup_t < 24) cy = 80;
    else if (cup_t < 54) cy = (u8)(80 - ((cup_t - 24) >> 1));
    else cy = (u8)(65 + (((frame_cnt >> 3) & 1) ? 1 : 0));
    for (i = 0; i < 16; ++i) oam_buf[i * 4] = (u8)(cy + (i >> 2) * 8 - 1);
    /* feux d'artifice pendant que la coupe est brandie (5 fusées, ~4 s) */
    oam_buf[64] = oam_buf[68] = oam_buf[72] = oam_buf[76] = 0xFF;
    if (cup_t == 24) fw_c = 131;
    if (cup_t >= 24 && cup_t < 233) {             /* arrêt après la 5e gerbe, avant une 6e fusée */
        if (--fw_c == 0) fw_c = 130;
        fw_sound(fw_c);
        oam_i = 64;
        fw_draw(fw_c, 0);
    }
    /* confettis : chute à 3 vitesses, retour en haut */
    for (i = 0; i < 10; ++i) {
        o = (u8)(80 + i * 4);
        if (cup_t < 24) { oam_buf[o] = 0xFF; continue; }
        y = (u8)(conf_y[i] + conf_v[i]);
        if (y > 215) y = (u8)(y - 200);
        conf_y[i] = y;
        oam_buf[o] = (u8)(y - 1);
    }
}
