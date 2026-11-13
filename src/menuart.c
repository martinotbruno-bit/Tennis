/* Royal Martinot — habillage des menus (banque 8, version 2.57).
 *
 * Tuiles du décor libres pendant les menus (celles des courts, rétablies
 * avant le match par chr_borrow_restore, bg_dirty) :
 *   $60-$9F police surlignée (tools/gen_menu.py), $A0-$A7 ornements dorés,
 *   $A8-$FB lettres doublées des titres (fabriquées à chaque écran).
 * Palettes du décor (pal_set_menu) : 0 texte blanc, 1 libellés bleu pâle,
 * 2 valeurs jaunes, 3 or ; couleur 2 de chaque palette : texte surligné.
 *
 * Fonctions appelées par screens.c (rendu coupé) :
 *   menu_deco_begin  charge police surlignée et ornements ;
 *   deco_title       titre en lettres doublées (deco_str) aux rangées 1-2,
 *                    filet doré rangée 3 ;
 *   deco_frame       attributs de l'écran (deco_kind) et filet du bas
 *                    (deco_row) ;
 *   palmares_build   palmarès (déplacé de la banque 1, pleine). */
#include "app.h"
#include "gfx.h"
#include "game.h"
#include "comp.h"
#include "stats.h"
#include "assets_vs.h"

#pragma static-locals(off)

#define PPU_DATA (*(volatile u8 *)0x2007)
#define T_FILET  0xA0
#define T_DIAM   0xA1
#define T_LEND   0xA2
#define T_REND   0xA3
#define T_STAR   0xA4
#define T_TITLE  0xA8      /* lettres doublées : 4 tuiles chacune, jusqu'à $FB */

extern const u8 menu_font2[], menu_deco[], menu_attr[], menu_big[];
extern const char menu_bigchars[];
extern u8 bg_dirty;
extern u8 pal_page;

#pragma bss-name(push, "SAVEDATA")
char deco_str[24];        /* titre à écrire (copié par screens.c) */
u8 deco_kind, deco_row;   /* type d'écran (attributs) ; rangée du filet du bas */
static u8 dt_map[24];
static u8 vs_row[VS_W], vs_at[64];
static char pb[8];
#pragma bss-name(pop)

void menu_deco_begin(void)
{
    chr_copy(BANK_PODIUM, menu_font2, 0x0600, 1024);
    chr_copy(BANK_PODIUM, menu_deco, 0x0A00, 128);
    if (!bg_dirty) bg_dirty = 2;   /* tuiles des menus seulement : rétablies avant le match (load_court) */
}

/* filet doré, des colonnes x0 à x1, losange au milieu */
static void filet(u8 y, u8 x0, u8 x1)
{
    u8 x, m = (u8)((x0 + x1) >> 1);
    vram_adr(NTADR(x0, y));
    for (x = x0; x <= x1; ++x)
        vram_put(x == x0 ? T_LEND : x == x1 ? T_REND : (x == m || x == m + 1) ? T_DIAM : T_FILET);
}

/* titre : lettres doublées (une lettre = 2 x 2 tuiles, l'espace = 1 colonne),
   centré aux rangées 1 et 2 ; trop long (plus de 30 colonnes) : texte simple
   en or, rangée 2. Les lettres doublées viennent toutes faites de la banque
   12 (menu_big, tools/gen_menu.py) : copie rapide */
void deco_title(void)
{
    u8 k, i, n = 0, w = 0, ch, x, v, r;
    for (k = 0; deco_str[k]; ++k) w = (u8)(w + (deco_str[k] == ' ' ? 1 : 2));
    if (w > 30) {
        txt_at((u8)((32 - k) >> 1), 2, deco_str);
        filet(3, 1, 30);
        return;
    }
    /* une fois chaque lettre distincte */
    for (k = 0; deco_str[k]; ++k) {
        ch = (u8)deco_str[k];
        dt_map[k] = 0xFF;
        if (ch == ' ') continue;
        for (i = 0; i < k; ++i) if ((u8)deco_str[i] == ch) { dt_map[k] = dt_map[i]; break; }
        if (dt_map[k] != 0xFF) continue;
        for (i = 0; menu_bigchars[i] && (u8)menu_bigchars[i] != ch; ++i) ;
        if (!menu_bigchars[i]) continue;                  /* caractère absent : blanc */
        dt_map[k] = n;
        chr_copy(BANK_COURTS, menu_big + (u16)i * 64, (u16)(T_TITLE + n * 4) << 4, 64);
        ++n;
    }
    x = (u8)((32 - w) >> 1);
    for (r = 0; r < 2; ++r) {
        vram_adr(NTADR(x, 1 + r));
        for (k = 0; deco_str[k]; ++k) {
            if (deco_str[k] == ' ') { vram_put(0); continue; }
            if (dt_map[k] == 0xFF) { vram_put(0); vram_put(0); continue; }
            v = (u8)(T_TITLE + dt_map[k] * 4 + r * 2);
            vram_put(v); vram_put((u8)(v + 1));
        }
    }
    filet(3, 1, 30);
    vram_adr(NTADR(1, 1)); vram_put(T_STAR);
    vram_adr(NTADR(30, 1)); vram_put(T_STAR);
}

/* attributs de l'écran (tables de tools/gen_menu.py) : deco_kind 0 (texte,
   filet du bas rangée 24) ou 1 (configuration, filet rangée 22) */
void deco_filet(void) { filet(deco_row, 1, 30); }
void deco_frame(void)
{
    if (deco_row) filet(deco_row, 1, 30);
    chr_copy(BANK_PODIUM, menu_attr + (deco_kind ? 64 : 0), 0x23C0, 64);
}

/* ---------------------------------------------------------------------- */
/* Palmarès (3 pages)                                                      */
static const char * const ex_name[NEX] = {
    "FRAPPES/CIBLES", "DEPLACEMENTS", "REGULARITE", "PRECISION", "VOLEE", "SERVICE",
    "JOUEUR FIXE", "PERSONNALISE", "TUTORIEL" };
static const char * const rank_txt[6] = { "", "VAINQUEUR !", "FINALISTE", "DEMI-FINALISTE", "4E", "ELIMINE" };

static void u16_at(u8 x, u8 y, u16 v)
{
    u8 i = 5;
    pb[5] = 0;
    do { pb[--i] = (char)('0' + v % 10); v /= 10; } while (v && i);
    while (i) pb[--i] = ' ';
    txt_at(x, y, pb);
}
static u16 pct16(u16 a, u16 b) { return b ? (u16)(((unsigned long)a * 100) / b) : 0; }

void palmares_build(void)
{
    u8 i;
    u16 t;
    char *ln = deco_str;
    if (pal_page == 0) {
        txt_at(3, 5, "MATCHS        SIMPLE DOUBLE");
        txt_at(3, 6, "JOUES"); u16_at(15, 6, sv.played[0]); u16_at(22, 6, sv.played[1]);
        txt_at(3, 7, "GAGNES"); u16_at(15, 7, sv.won[0]); u16_at(22, 7, sv.won[1]);
        txt_at(3, 8, "PERDUS"); u16_at(15, 8, (u16)(sv.played[0] - sv.won[0])); u16_at(22, 8, (u16)(sv.played[1] - sv.won[1]));
        txt_at(3, 9, "POINTS GAGNES"); u16_at(22, 9, sv.pts_w);
        txt_at(3, 10, "POINTS PERDUS"); u16_at(22, 10, sv.pts_l);
        txt_at(3, 11, "ACES"); u16_at(22, 11, sv.aces);
        txt_at(3, 12, "DOUBLES FAUTES"); u16_at(22, 12, sv.dfaults);
        txt_at(3, 13, "COUPS GAGNANTS"); u16_at(22, 13, sv.winners);
        txt_at(3, 14, "FAUTES"); u16_at(22, 14, sv.errors);
        txt_at(3, 15, "1ERES BALLES %"); u16_at(22, 15, pct16(sv.fs_in, sv.fs_tot));
        txt_at(3, 16, "TITRES"); u16_at(22, 16, sv.titles);
        txt_at(3, 17, "COMPETITIONS"); u16_at(22, 17, sv.comps);
        txt_at(3, 18, "RECORD ECHANGE"); u16_at(22, 18, sv.best_rally);
        txt_at(3, 19, "RECORD ACES/MATCH"); u16_at(22, 19, sv.best_aces);
        txt_at(20, 20, "NB     %");
        txt_at(3, 21, "SERVICES CANON"); u16_at(17, 21, sx.ok); u16_at(23, 21, pct16(sx.ok, sx.tried));
        txt_at(3, 22, "COUPS PARFAITS"); u16_at(17, 22, sx.pok); u16_at(23, 22, pct16(sx.pok, sx.ptry));
    } else if (pal_page == 1) {
        txt_at(3, 5, "MEILLEURS RESULTATS");
        for (i = 0; i < NCT; ++i) {
            COMP(comp_cname, i); txt_at(3, (u8)(6 + i), comp_l2);
            t = i < 5 ? sv.comp_best[i] : sw_best[i - 4];
            txt_at(17, (u8)(6 + i), t ? rank_txt[t > 5 ? 5 : t] : (const char *)"-");
        }
        txt_at(3, 14, "HISTORIQUE");
        for (i = 0; i < sv.hist_n && i < 8; ++i) {
            COMP(comp_cname, sv.hist[i][0]); txt_at(3, (u8)(15 + i), comp_l2);
            team_label(sv.hist[i][2], 255, ln); txt_at(16, (u8)(15 + i), ln);
            ln[0] = (char)('0' + sv.hist[i][1]); ln[1] = 'E'; ln[2] = 0;
            txt_at(22, (u8)(15 + i), sv.hist[i][1] == 1 ? "1ER" : ln);
        }
    } else {
        txt_at(3, 5, "RECORDS D'ENTRAINEMENT");
        for (i = 0; i < NEX; ++i) {
            txt_at(3, (u8)(7 + i), ex_name[i]);
            u16_at(20, (u8)(7 + i), sv.best_train[i]);
        }
    }
    txt_at(2, 25, "GAUCHE/DROITE : PAGE");
    txt_at(2, 26, "B : RETOUR");
    txt_at(2, 27, "SELECT X2 : TOUT EFFACER");
}

/* ---------------------------------------------------------------------- */
/* Affiche VS (présentation du match) : bustes face à face, bras croisés,
   joueur 1 (ou équipe 1) à gauche, joueur 2 à droite (en miroir), tournés
   l'un vers l'autre. Chaque buste a 2 palettes : A (sombre, peau, tenue :
   visage, bras croisés sur la tenue), B (sombre, tenue, blanc) ; le texte
   blanc, ombré de sombre, s'écrit dans les blocs B. Tuiles : gauche $60-$9F puis $01-$1F, droite $A0-$FF. Rendu coupé. */
static const u8 surf_bk[6] = { 0x07, 0x0A, 0x02, 0x05, 0x08, 0x0C };   /* fond : couleur de la surface */
static void vs_side(u8 side)
{
    u8 c = cfg.chr[side], f = char_female[c], r, i, v, n;
    const u8 *chr, *map, *ab;
    if (f && c == 16) f = 2;                            /* Margaux : silhouette de fillette */
    if (c == 19) f = 3;                                 /* Papi : cheveux gris (palettes à part) */
    if (side == 0) {
        if (f == 3) { chr = vs_pl_chr; map = vs_pl_map; ab = vs_pl_ab; n = VS_PL_N; }
        else if (f == 2) { chr = vs_gl_chr; map = vs_gl_map; ab = vs_gl_ab; n = VS_GL_N; }
        else { chr = f ? vs_fl_chr : vs_ml_chr; map = f ? vs_fl_map : vs_ml_map; ab = f ? vs_fl_ab : vs_ml_ab; n = f ? VS_FL_N : VS_ML_N; }
    } else {
        if (f == 3) { chr = vs_pr_chr; map = vs_pr_map; ab = vs_pr_ab; n = VS_PR_N; }
        else if (f == 2) { chr = vs_gr_chr; map = vs_gr_map; ab = vs_gr_ab; n = VS_GR_N; }
        else { chr = f ? vs_fr_chr : vs_mr_chr; map = f ? vs_fr_map : vs_mr_map; ab = f ? vs_fr_ab : vs_mr_ab; n = f ? VS_FR_N : VS_MR_N; }
    }
    tz_bank = VS_BANK;
    if (side == 0) {
        vram_adr(0x0600); tz_n = (u8)(n > 64 ? 64 : n); tz_unpack(chr);
        if (n > 64) { vram_adr(0x0010); tz_n = (u8)(n - 64); tz_unpack(tz_src); }
    } else { vram_adr(0x0A00); tz_n = n; tz_unpack(chr); }
    for (r = 0; r < VS_H; ++r) {
        bank_read(VS_BANK, vs_row, map + r * VS_W, VS_W);
        vram_adr(NTADR(side ? 20 : 0, 4 + r));
        for (i = 0; i < VS_W; ++i) {
            v = vs_row[i];
            if (v) { --v; v = (u8)(side ? 0xA0 + v : v < 64 ? 0x60 + v : 0x01 + v - 64); }
            vram_put(v);
        }
    }
    /* attributs des blocs du buste (6 x 7 blocs, rangées de blocs 2 à 8) */
    for (r = 0; r < VS_H / 2; ++r) {
        bank_read(VS_BANK, vs_row, ab + r * 6, 6);
        for (i = 0; i < 6; ++i) {
            u8 bx = (u8)(side ? 10 + i : i), by = (u8)(2 + r), q = (u8)(((by & 1) << 1) | (bx & 1)), a = (u8)((by >> 1) * 8 + (bx >> 1));
            u8 p = (u8)((side ? 2 : 0) + (vs_row[i] ? 1 : 0));
            vs_at[a] = (u8)((vs_at[a] & (u8)~(3 << (q * 2))) | (p << (q * 2)));
        }
    }
}
/* palettes du joueur (couleurs de char_pal) : A et B */
static void vs_pal(u8 side)
{
    u8 b = (u8)(side * 8), S, O, D;
    pal_set_player(61, cfg.chr[side]);       /* (palette 1 du décor : peau, tenue, sombre) */
    S = pal_buf[5]; O = pal_buf[6]; D = pal_buf[7];
    pal_buf[b + 1] = D;
    pal_buf[b + 2] = (u8)(S >= 0x10 ? S : S + 0x10);                 /* peau (éclairée) */
    pal_buf[b + 6] = (u8)(O >= 0x10 ? O : O + 0x10);                  /* tenue */
    pal_buf[b + 5] = (u8)((D & 0x30) >= 0x20 ? 0x07 : D);             /* B : sombre (ombre du texte : noir si cheveux clairs), tenue, blanc */
    pal_buf[b + 3] = pal_buf[b + 6];                                  /* tenue (aussi dans A : bras croisés) */
    pal_buf[b + 7] = 0x30;
    if (cfg.chr[side] == 16 || cfg.chr[side] == 19) {   /* Margaux, Papi : A (noir, peau, cheveux clairs), B (noir, peau, tenue) */
        pal_buf[b + 1] = pal_buf[b + 5] = 0x0F;
        pal_buf[b + 3] = D;
        pal_buf[b + 6] = pal_buf[b + 2];
        pal_buf[b + 7] = (u8)(O >= 0x10 ? O : O + 0x10);
    }
}
void vs_build(void)
{
    u8 i, k = surf_bk[cfg.surface < 6 ? cfg.surface : 0];
    /* texte : moitié gauche palette 1, moitié droite palette 3 (blanc) */
    /* (Margaux n'a pas de blanc : son texte prend la palette blanche de l'autre côté) */
    u8 l0 = (u8)(cfg.chr[0] == 16 || cfg.chr[0] == 19), l1 = (u8)(cfg.chr[1] == 16 || cfg.chr[1] == 19);
    u8 tl = (u8)(l0 && !l1 ? 0xFF : 0x55);
    u8 tr = (u8)(l1 && !l0 ? 0x55 : 0xFF);
    if (l0 && l1) tl = tr = (u8)(cfg.chr[0] == 19 ? 0x55 : 0xFF);     /* Papi contre Margaux : la tenue blanche de Papi */
    for (i = 0; i < 64; ++i) vs_at[i] = (i & 7) < 4 ? tl : tr;
    vs_side(0);
    vs_side(1);
    vram_adr(0x23C0);
    for (i = 0; i < 64; ++i) vram_put(vs_at[i]);
    vs_pal(1);                  /* (le côté droit d'abord : vs_pal passe par la palette 1) */
    vs_pal(0);
    pal_buf[0] = pal_buf[4] = pal_buf[8] = pal_buf[12] = pal_buf[16] = k;
    pal_dirty = 1;
    bg_dirty = 1;               /* tuiles du décor remplacées : rétablies ensuite */
}
