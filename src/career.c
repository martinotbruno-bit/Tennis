/* Royal Martinot — banque 19 (version 2.72) : fiche des joueurs sur l'affiche
 * VS, face-à-face, mode carrière.
 *
 * Fiche façon télévision : sur l'affiche VS d'un match en simple, sous
 * chaque buste, les lignes « manette / niveau / style » alternent avec la
 * fiche du joueur (taille, main forte, classement) ; en bas, les conditions
 * du match alternent avec le face-à-face (vos matchs contre cet adversaire,
 * gardés au palmarès). Les textes d'origine de l'affiche (screens.c, banque
 * 1 pleine) sont écrits ici aussi. */
#include "app.h"
#include "gfx.h"
#include "game.h"
#include "comp.h"
#include "stats.h"

#pragma static-locals(on)

extern u8 opt_val[];
static const char * const lvl_name[3] = { "FACILE", "NORMAL", "EXPERT" };
static const char * const cnd_name[4] = { "JOUR", "NUIT", "PLUIE", "CREPUSCULE" };
static const char * const fmt_nm[4] = { "1 SET", "3 SETS", "5 SETS", "RAPIDE 4 JEUX" };

/* fiche : taille (cm - 100), gaucher, classement mondial (0 : texte à part) */
static const u8 ch_h[NCHARS] = { 85, 72, 90, 68, 83, 70, 96, 75, 88, 66, 79, 74, 81, 69, 87, 71, 28, 92, 65, 76, 40 };
static const u8 ch_left[NCHARS] = { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0 };
static const u8 ch_rank[NCHARS] = { 3, 1, 5, 2, 8, 6, 4, 7, 11, 9, 14, 10, 12, 13, 15, 16, 0, 18, 17, 0, 0 };

extern u8 sw_h2h[NCHARS + 2];          /* stats.c : 'H', vos victoires (4 bits hauts) et défaites contre chaque personnage, contrôle */
extern u16 car_rank(void);             /* classement de la carrière (0 : pas de carrière) */

#define fb comp_l1                     /* (tampon de texte de 29 caractères de comp.c : libre sur l'affiche et les écrans de fin) */
static u8 fch_t, fch_pg, fch_row, fch_on, fch_h2h;

static void fb_clear(u8 n) { u8 i; for (i = 0; i < n; ++i) fb[i] = ' '; fb[n] = 0; }
static void fb_put(u8 at, const char *s) { while (*s && at < 28) fb[at++] = *s++; }
static u8 fb_num(u8 at, u16 v)          /* nombre à partir de at ; rend la colonne suivante */
{
    char d[6];
    u8 n = 0;
    do { d[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) fb[at++] = d[--n];
    return at;
}

/* ligne r (0 à 2 : rangées 20 à 22) sous le buste t, page 0 (affiche) ou 1 (fiche) */
static void side_text(u8 t, u8 r, u8 pg)
{
    u8 c = cfg.chr[t];
    u16 k;
    fb_clear(14);
    if (!pg) {
        if (r == 0) { if (cfg.doubles) { fb[0] = '+'; fb_put(2, char_name[cfg.chr[t + 2]]); } }
        else if (r == 1) fb_put(0, cfg.ctrl[t] == CTRL_CPU ? lvl_name[cfg.level[t]] : cfg.ctrl[t] == CTRL_PAD1 ? (const char *)"MANETTE 1" : (const char *)"MANETTE 2");
        else if (cfg.doubles && cfg.ctrl[t + 2] == CTRL_PAD2) fb_put(0, "+ MANETTE 2");
        else if (cfg.ctrl[t] == CTRL_CPU) fb_put(0, style_name[char_style[c]]);
        return;
    }
    if (r == 0) { fb[0] = (char)('0' + (ch_h[c] + 100) / 100); fb[1] = ','; fb[2] = (char)('0' + ch_h[c] / 10 % 10); fb[3] = (char)('0' + ch_h[c] % 10); fb_put(5, "M"); }
    else if (r == 1) fb_put(0, ch_left[c] ? (char_female[c] ? (const char *)"GAUCHERE" : (const char *)"GAUCHER") : (char_female[c] ? (const char *)"DROITIERE" : (const char *)"DROITIER"));
    else {
        k = (t == 0 && cfg.ctrl[0] != CTRL_CPU) ? car_rank() : 0;
        if (k) { fb_put(0, "CARRIERE "); fb_put(fb_num(9, k), "E"); }
        else if (ch_rank[c]) { fb_put(0, "CLASSEMENT "); fb_put(fb_num(11, ch_rank[c]), "E"); }
        else fb_put(0, c == 16 ? (const char *)"NO 1 DU COEUR" : c == 19 ? (const char *)"LEGENDE" : (const char *)"JUNIOR");
    }
}
/* rangée 24 + r (28 caractères, colonnes 2 à 29) */
static void foot_text(u8 r, u8 pg)
{
    u8 v;
    fb_clear(28);
    if (!pg || !fch_h2h) {
        if (r == 0) { fb_put(0, surf_name[cfg.surface]); fb_put(16, env_name[cfg.env]); }
        else { fb_put(0, cnd_name[cfg.cond]); fb_put(16, fmt_nm[cfg.games_per_set == 4 ? 3 : cfg.sets_to_win - 1]); }
        return;
    }
    v = sw_h2h[1 + cfg.chr[1]];
    if (r == 0) {
        if (!v) { fb_put(0, "PREMIER FACE A FACE"); return; }
        fb_put(0, "FACE A FACE : ");
        r = fb_num(14, v >> 4); fb_put(r, " V - "); r = fb_num((u8)(r + 5), v & 15); fb_put(r, " D");
    } else { fb_put(0, "VOS MATCHS CONTRE "); fb_put(18, char_name[cfg.chr[1]]); }
}

/* affiche VS : textes (rendu coupé), puis la fiche alterne (fiche_tick) */
void vs_texts(void)
{
    u8 t, r;
    for (t = 0; t < 2; ++t) {
        txt_at((u8)(t ? 18 : 2), 19, char_name[cfg.chr[t]]);
        for (r = 0; r < 3; ++r) { side_text(t, r, 0); txt_at((u8)(t ? 18 : 2), (u8)(20 + r), fb); }
    }
    if (cfg.env == ENV_MARGAUX) txt_at(2, 3, "BIENVENUE AU COURT MARGAUX !");
    for (r = 0; r < 2; ++r) { foot_text(r, 0); txt_at(2, (u8)(24 + r), fb); }
    /* fiche : en simple ; face-à-face : manette 1 contre l'ordinateur */
    fch_on = (u8)(!cfg.doubles);
    fch_h2h = (u8)(fch_on && cfg.ctrl[0] != CTRL_CPU && cfg.ctrl[1] == CTRL_CPU && sw_h2h[0] == 'H');
    fch_t = 0; fch_pg = 0; fch_row = 0xFF;
}
/* à chaque trame de l'affiche VS : page suivante toutes les 150 trames
   (la première après 2,5 s, la glissade finie) ; une rangée par trame */
void fiche_tick(void)
{
    u8 r;
    if (!fch_on) return;
    if (++fch_t >= 150) { fch_t = 0; fch_pg ^= 1; fch_row = 0; }
    if (fch_row >= 8) return;
    r = fch_row;
    if (vram_len + 32 > VBUF_MAX) return;      /* (budget de la trame : la suivante) */
    if (r < 6) { side_text((u8)(r / 3), (u8)(r % 3), fch_pg); vbuf_text((u8)(r < 3 ? 2 : 18), (u8)(20 + r % 3), fb, 14); }
    else { foot_text((u8)(r - 6), fch_pg); vbuf_text(2, (u8)(24 + r - 6), fb, 28); }
    ++fch_row;
}

/* ====================================================================== */
/* Mode carrière (option CARRIERE, compétitions en simple, sauf la         */
/* compétition personnalisée) : chaque compétition jouée rapporte des      */
/* points selon le rang final, le type et le niveau ; le classement va du */
/* 100e (0 point) au 1er ; tous les 400 points, une qualité progresse, à  */
/* tour de rôle : service (+2 de vitesse de balle), vitesse de course     */
/* (+2), précision (dispersion -1), jusqu'à 5 crans chacune. Les qualités */
/* ne jouent que dans les matchs d'une compétition en carrière.            */
/* ====================================================================== */
#define O_CAREER 25
#pragma bss-name(push, "WBSS")
u8 car_srv, car_spd, car_acc;          /* bonus du joueur 1 pour ce match (game.c, shots.c) */
#pragma bss-name(pop)

/* contrôle du bloc de la carrière : même calcul que stats.c (blk_sum), copié
   ici (banque 19 : une fonction de la banque 5 ne s'appelle pas directement) */
static void car_sum(void)
{
    u8 n = 11, c = 0x3B;
    const u8 *b = sw_car;
    while (n--) c = (u8)(((c << 1) | (c >> 7)) ^ *b++);
    sw_car[11] = c;
}
static u16 car_pts(void) { return (u16)(sw_car[1] | (sw_car[2] << 8)); }
/* 1 + 99 x 600 / (600 + points) : 100e à 0 point, 50e à 600, 10e vers 5 400,
   1er au-delà de 58 800 (calcul sur 16 bits : pas de routine 32 bits dans la
   banque fixe) */
static u16 rank_of(u16 p) { return (u16)(1 + 59400U / (600U + (p > 64000U ? 64000U : p))); }
u16 car_rank(void)
{
    if (sw_car[0] != 'C' || !opt_val[O_CAREER]) return 0;
    return rank_of(car_pts());
}
static u8 car_lvl(u8 k)                 /* crans de la qualité k (0 service, 1 vitesse, 2 précision) */
{
    u16 n = car_pts() / 400;
    if (n > 15) n = 15;
    n = (n + 2 - k) / 3;
    return (u8)(n > 5 ? 5 : n);
}
/* début d'un match : bonus si la carrière est en jeu */
void car_match_setup(void)
{
    u8 on = (u8)(opt_val[O_CAREER] && c_active && !c_dbl && c_type != CT_CUSTOM && cfg.mode == GM_MATCH &&
                 cfg.ctrl[0] != CTRL_CPU && sw_car[0] == 'C');
    car_srv = on ? (u8)(car_lvl(0) * 2) : 0;
    car_spd = on ? (u8)(car_lvl(1) * 2) : 0;
    car_acc = on ? car_lvl(2) : 0;
}
/* fin d'une compétition (stats.c, sv_comp_end) */
void car_comp_end(void)
{
    static const u8 base[9] = { 25, 100, 60, 35, 35, 18, 18, 18, 18 };   /* x 4 : 400 au vainqueur */
    static const u8 tm[NCT] = { 4, 4, 6, 8, 4, 12, 12 };
    static const u8 lm[3] = { 2, 4, 6 };
    u16 g, p;
    u8 r;
    sw_car[6] = sw_car[7] = 0;
    if (!opt_val[O_CAREER] || c_dbl || c_type == CT_CUSTOM || sw_car[0] != 'C') { car_sum(); return; }
    g = (u16)(((u16)base[c_rank < 9 ? c_rank : 0] * 4 * tm[c_type] * lm[c_lvl < 3 ? c_lvl : 1]) >> 4);
    p = car_pts();
    p = (u16)(p + g < p ? 0xFFFF : p + g);
    sw_car[1] = (u8)p; sw_car[2] = (u8)(p >> 8);
    sw_car[6] = (u8)g; sw_car[7] = (u8)(g >> 8);
    if (sw_car[3] < 255) ++sw_car[3];
    if (c_rank == 1 && sw_car[4] < 255) ++sw_car[4];
    r = (u8)rank_of(p);
    if (!sw_car[5] || r < sw_car[5]) sw_car[5] = r;
    car_sum();
}
/* écran de fin de compétition : « CARRIERE : +400 PTS, 57E » (rangée y) */
u16 __fastcall__ car_end_text(u16 y)
{
    u16 g = (u16)(sw_car[6] | (sw_car[7] << 8));
    u8 x;
    if (!g) return 0;
    fb_clear(28);
    fb_put(0, "CARRIERE : +");
    x = fb_num(12, g); fb_put(x, " PTS, ");
    x = fb_num((u8)(x + 6), car_rank()); fb_put(x, "E");
    txt_at(2, (u8)y, fb);
    return 0;
}
/* palmarès, page des records (rangées 18 à 22) */
static const char * const sk_name[3] = { "SERVICE", "VITESSE", "PRECISION" };
void car_pal_text(void)
{
    u8 k, x, n;
    if (sw_car[0] != 'C' || !sw_car[3]) { txt_at(3, 19, "CARRIERE : PAS ENCORE"); return; }
    fb_clear(28);
    fb_put(0, "CARRIERE "); x = fb_num(9, rank_of(car_pts())); fb_put(x, "E  ");
    fb_put((u8)(x + 3), "MEILLEUR "); x = fb_num((u8)(x + 12), sw_car[5]); fb_put(x, "E");
    txt_at(3, 19, fb);
    fb_clear(28);
    x = fb_num(0, car_pts()); fb_put(x, " PTS  ");
    x = fb_num((u8)(x + 6), sw_car[3]); fb_put(x, " COMP.  ");
    x = fb_num((u8)(x + 8), sw_car[4]); fb_put(x, " TITRES");
    txt_at(3, 20, fb);
    for (k = 0; k < 3; ++k) {
        fb_clear(16);
        fb_put(0, sk_name[k]);
        for (n = car_lvl(k), x = 10; n; --n) fb[x++] = '@';
        while (x < 15) fb[x++] = '-';
        txt_at(3, (u8)(21 + k), fb);
    }
}
