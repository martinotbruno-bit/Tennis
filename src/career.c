/* Royal Martinot — banque 19 (version 2.72) : fiche des joueurs sur l'affiche
 * VS, face-à-face, mode carrière.
 *
 * Fiche façon télévision : sur l'affiche VS d'un match en simple, sous
 * chaque buste, les lignes « manette / niveau / style » alternent avec la
 * fiche du joueur (taille, main forte, classement) ; en bas, les conditions
 * du match alternent avec le face-à-face (vos matchs contre cet adversaire,
 * gardés au palmarès). Les textes d'origine de l'affiche (screens.c, banque
 * 1 pleine) sont écrits ici aussi. */
#include <string.h>
#include "app.h"
#include "gfx.h"
#include "game.h"
#include "comp.h"
#include "stats.h"
#include "game_int.h"
#include "audio.h"
void set_msg(const char *a, const char *b, u8 t);   /* banque fixe */

#pragma static-locals(on)

extern u8 opt_val[];
static const char * const lvl_name[3] = { "FACILE", "NORMAL", "EXPERT" };
static const char * const cnd_name[4] = { "JOUR", "NUIT", "PLUIE", "CREPUSCULE" };
static const char * const fmt_nm[4] = { "1 SET", "3 SETS", "5 SETS", "RAPIDE 4 JEUX" };

/* fiche : taille (cm - 100), gaucher, classement mondial (0 : texte à part) */
static const u8 ch_h[NCHARS] = { 85, 72, 90, 68, 83, 70, 96, 75, 88, 66, 79, 74, 81, 69, 87, 71, 28, 92, 65, 76, 40 };
static const u8 ch_left[NCHARS] = { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0 };
static const u8 ch_rank[NCHARS] = { 3, 1, 5, 2, 8, 6, 4, 7, 11, 9, 14, 10, 12, 13, 15, 16, 0, 18, 17, 0, 0 };

extern u8 sw_h2h[NCHARS + 2], sw_car[12];   /* stats.c (carrière : sw_car) : 'H', vos victoires (4 bits hauts) et défaites contre chaque personnage, contrôle */
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
        if (r == 0) { if (cfg.doubles) { fb[0] = '+'; fb_put(2, chr_nm(cfg.chr[t + 2])); } }
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
    } else { fb_put(0, "VOS MATCHS CONTRE "); fb_put(18, chr_nm(cfg.chr[1])); }
}

/* affiche VS : textes (rendu coupé), puis la fiche alterne (fiche_tick) */
void vs_texts(void)
{
    u8 t, r;
    for (t = 0; t < 2; ++t) {
        txt_at((u8)(t ? 18 : 2), 19, chr_nm(cfg.chr[t]));
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

/* ====================================================================== */
/* Sauvegarde du match en cours (2.72, cartouche à pile) : à la fin de     */
/* chaque point d'un match hors compétition (hl_point_ev), réglages,      */
/* score et statistiques sont copiés dans pod_buf (RAM sauvegardée ; le    */
/* podium, seul autre utilisateur, ne vient qu'en fin de compétition).    */
/* Au démarrage, l'écran titre propose « B : REPRENDRE LE MATCH » : le     */
/* match repart au même score, au service (cer_entry -> msave_restore).   */
/* La sauvegarde est effacée au début d'un autre match et à la fin du     */
/* match (gagné, perdu ou abandonné).                                     */
/* ====================================================================== */
extern u8 pod_buf[144], sc_crit;
extern u8 st_points[2], st_aces[2], st_dfaults[2], st_winners[2], st_errors[2], st_first_in[2], st_first_tot[2];
extern u8 st_longest, st_canon[2], st_ctry[2], st_perf[2], st_ptry[2], st_vmax[2], st_vmaxp[2], srv_slot[2];
#include "rules.h"
#pragma bss-name(push, "WBSS")
u8 mresume;                             /* 1 : le prochain match reprend la sauvegarde */
#pragma bss-name(pop)
typedef struct { u8 *p; u8 n; } MsvPart;
static const MsvPart msv_tab[] = {
    { (u8 *)&cfg, sizeof(MatchCfg) },
    { sc_points, 2 }, { sc_games, 2 }, { sc_sets, 2 }, { (u8 *)sc_hist, MAX_SETS * 2 },
    { &sc_set_no, 1 }, { &sc_tiebreak, 1 }, { &sc_server, 1 }, { &sc_swap, 1 }, { &sc_tb_first, 1 },
    { &sc_set_first_server, 1 }, { &sc_sets_to_win, 1 }, { &sc_games_per_set, 1 },
    { st_points, 2 }, { st_aces, 2 }, { st_dfaults, 2 }, { st_winners, 2 }, { st_errors, 2 },
    { st_first_in, 2 }, { st_first_tot, 2 }, { &st_longest, 1 }, { st_canon, 2 }, { st_ctry, 2 },
    { st_perf, 2 }, { st_ptry, 2 }, { st_vmax, 2 }, { st_vmaxp, 2 }, { srv_slot, 2 } };
#define MSV_N (sizeof(msv_tab) / sizeof(msv_tab[0]))
static u8 msv_len, msv_sum;
static void msv_check(void)             /* longueur et contrôle de pod_buf[0 .. msv_len - 1] */
{
    u8 i = 0, c = 0x4D;
    msv_len = 2;
    for (msv_sum = 0; msv_sum < MSV_N; ++msv_sum) msv_len = (u8)(msv_len + msv_tab[msv_sum].n);
    while (i < msv_len) { c = (u8)(((c << 1) | (c >> 7)) ^ pod_buf[i]); ++i; }
    msv_sum = c;
}
/* fin d'un point (hl_point_ev) : last_mode connu de screens.c, rangé par msave_mode */
static u8 msv_mode;
void __fastcall__ msave_mode(u8 m) { msv_mode = m; }
void msave_write(void)
{
    u8 k, i, n = 2;
    const u8 *q;
    if (cfg.mode != GM_MATCH || cfg.demo || c_active || sc_over) return;
    pod_buf[0] = 'M'; pod_buf[1] = msv_mode;
    for (k = 0; k < MSV_N; ++k)
        for (q = msv_tab[k].p, i = msv_tab[k].n; i; --i) pod_buf[n++] = *q++;
    msv_check();
    pod_buf[msv_len] = msv_sum;
}
void msave_clear(void) { pod_buf[0] = 0; }
/* écran titre : 1 + le mode du menu si une sauvegarde est valable, 0 sinon */
u16 __fastcall__ msave_ok(u16 unused)
{
    (void)unused;
    if (pod_buf[0] != 'M') return 0;
    msv_check();
    if (pod_buf[msv_len] != msv_sum) return 0;
    return (u16)(1 + pod_buf[1]);
}
/* reprise demandée : les réglages tout de suite (écran de présentation) */
static void cust_resume(void);
void msave_load_cfg(void)
{
    u8 i;
    u8 *d = (u8 *)&cfg;
    for (i = 0; i < sizeof(MatchCfg); ++i) d[i] = pod_buf[2 + i];
    mresume = 1;
    cust_resume();                      /* joueur personnalisé d'un match de compétition (2.75) */
}
/* début du match (cer_entry, après match_start) : score et statistiques ;
   un « let » d'une trame replace les joueurs pour le service (after_point) */
void sun_apply(void);                   /* highlight.c, banque 15 */
void msave_restore(void)
{
    u8 k, i, n = 2;
    u8 *q;
    mresume = 0;
    /* (seulement si le match lancé est bien celui de la sauvegarde) */
    for (q = (u8 *)&cfg, i = 0; i < sizeof(MatchCfg); ++i) if (q[i] != pod_buf[2 + i]) { pod_buf[0] = 0; return; }
    for (k = 0; k < MSV_N; ++k)
        for (q = msv_tab[k].p, i = msv_tab[k].n; i; --i) *q++ = pod_buf[n++];
    /* balles de set et de match (lues par l'IA) : comme à la fin d'un point */
    sc_crit = 0;
    for (k = 0; k < 2; ++k) {
        if (rules_is_match_point(k)) sc_crit |= (u8)(4 | (1 << k));
        else if (rules_is_set_point(k)) sc_crit |= (u8)(1 << k);
    }
    ms = MS_DEAD; ms_timer = 1; end_reason = 3;     /* END_LET : after_point replace les joueurs */
    set_msg("REPRISE", "DU MATCH", 90);
    bank_call(BANK_HL, sun_apply);                  /* crépuscule : la lumière du moment */
}

/* ---------------------------------------------------------------------- */
/* Joueur personnalisé en compétition (2.75) : les joueurs humains peuvent  */
/* habiller le personnage choisi (nom, sexe, peau, cheveux, tenue), sans    */
/* obligation. Le profil est gardé dans la sauvegarde (cu_sav, gfx.c) et   */
/* appliqué pendant la compétition seulement (cu_map, char_female).        */
/* Les cheveux partagent leur couleur avec le short et la raquette          */
/* (« sombre » des personnages).                                            */
#pragma static-locals(off)              /* (variables locales sur la pile : WBSS est pleine) */
extern u8 cu_st[6];
#define cu_row cu_st[0]
#define cu_ed  cu_st[1]
#define cu_pos cu_st[2]
#define cu_k   cu_st[3]
#define cu_n   cu_st[4]
extern u8 cu_c[2];
#define CU_ROWS 7
#define CU_VX   13
#define CU_Y(r) (u8)(7 + (r) * 2)
/* (2.76) PALE ($36) et CLAIRE ($37) se confondaient : CLAIRE ($36), HALEE ($26, hâlée) */
static const u8 cu_skin[5] = { 0x36, 0x26, 0x27, 0x17, 0x07 };
static const char * const cu_skin_n[5] = { "CLAIRE", "HALEE", "MATE", "BRUNE", "EBENE" };
static const u8 cu_hair[6] = { 0x0F, 0x07, 0x17, 0x37, 0x10, 0x30 };
static const char * const cu_hair_n[6] = { "NOIRS", "BRUNS", "ROUX", "BLONDS", "GRIS", "BLANCS" };
static const u8 cu_out[15] = { 0x30, 0x10, 0x00, 0x37, 0x35, 0x24, 0x27, 0x07, 0x2B, 0x3B, 0x39, 0x2C, 0x31, 0x1C, 0x0C };
static const char * const cu_out_n[15] = { "BLANC", "GRIS", "ANTHRACITE", "CREME", "ROSE", "MAGENTA", "ORANGE",
                                           "CHOCOLAT", "VERT", "MENTHE", "ANIS", "CYAN", "CIEL", "CANARD", "PETROLE" };
static const char * const cu_lbl[CU_ROWS] = { "PROFIL", "NOM", "GENRE", "PEAU", "CHEVEUX", "TENUE", "VALIDER" };
static const char cu_abc[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ-";
extern const u8 char_pal_rom[NCHARS][3];        /* (gfx.c) */
/* (2.76, déplacés de la banque fixe) sexe d'origine des personnages ;
   personnages d'origine (au démarrage, aux réglages, hors compétition) */
const u8 char_female0[NCHARS] = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0 };
void cust_reset(void)
{
    memcpy(char_female, char_female0, NCHARS);
    cu_map[0] = cu_map[1] = 0;
}

static u8 cu_find(const u8 *t, u8 n, u8 v) { u8 i; for (i = 0; i < n; ++i) if (t[i] == v) return i; return 0; }

/* profil k d'après le personnage c (nom, sexe, couleurs d'origine) ; inactif */
static void cu_from_char(u8 k, u8 c)
{
    u8 *s = cu_sav + CU_SLOT(k), i;
    const char *n = char_name[c];
    for (i = 0; i < 7; ++i) s[i] = (u8)(*n > ' ' ? *n++ : ' ');
    s[7] = 0;
    s[8] = (u8)(char_female0[c] << 1);
    s[9] = char_pal_rom[c][0]; s[10] = char_pal_rom[c][1]; s[11] = char_pal_rom[c][2];
}

/* appliqué au début de la compétition (et à sa reprise) */
void cust_apply(void)
{
    u8 k, c, *s;
    cust_reset();
    if (cu_sav[0] != CU_MAGIC) return;
    for (k = 0; k < 2; ++k) {
        c = k ? (c_hum2 ? c_pl2[0] : 255) : c_pl[0];
        s = cu_sav + CU_SLOT(k);
        if (c < NCHARS && (s[8] & 1)) { cu_map[k] = (u8)(c + 1); char_female[c] = (u8)((s[8] >> 1) & 1); }
    }
}

static void cu_live(void)                       /* aperçu : profil en cours appliqué */
{
    u8 *s = cu_sav + CU_SLOT(cu_k), c = cu_c[cu_k];
    cust_reset();
    if (s[8] & 1) { cu_map[cu_k] = (u8)(c + 1); char_female[c] = (u8)((s[8] >> 1) & 1); }
    pal_set_player(0, c);
}

static void cu_value(u8 r, u8 now)              /* valeur de la rangée r (now : pendant l'affichage) */
{
    u8 *s = cu_sav + CU_SLOT(cu_k), i;
    char b[11];
    const char *v = "";
    for (i = 0; i < 10; ++i) b[i] = ' ';
    b[10] = 0;
    switch (r) {
    case 0: v = (s[8] & 1) ? "PERSONNEL" : "D'ORIGINE"; break;
    case 1:
        for (i = 0; i < 7; ++i) b[i] = (char)s[i];
        if (cu_ed) b[cu_pos] = (char)(b[cu_pos] == ' ' ? '_' : b[cu_pos] + 0x40);   /* lettre en cours : en or */
        v = 0; break;
    case 2: v = (s[8] & 2) ? "FEMME" : "HOMME"; break;
    case 3: v = cu_skin_n[cu_find(cu_skin, 5, s[9])]; break;
    case 4: v = cu_hair_n[cu_find(cu_hair, 6, s[11])]; break;
    case 5: v = cu_out_n[cu_find(cu_out, 15, s[10])]; break;
    default: return;
    }
    if (v) for (i = 0; *v; ++i) b[i] = *v++;
    if (now) vbuf_text(CU_VX, CU_Y(r), b, 10); else txt_at(CU_VX, CU_Y(r), b);
}

/* page du joueur cu_k (écran coupé : appelée après screen_begin, screens.c) */
void cust_page(void)
{
    u8 r;
    txt_off = 0x40;
    txt_at(4, 4, cu_k ? "PARTENAIRE : MANETTE 2" : (cu_n > 1 ? "JOUEUR 1 : MANETTE 1" : "NOM ET TENUE, FACULTATIF"));
    txt_off = 0;
    for (r = 0; r < CU_ROWS; ++r) { txt_at(3, CU_Y(r), cu_lbl[r]); cu_value(r, 0); }
    txt_at(1, CU_Y(cu_row), ">");
    txt_at(3, 21, "GAUCHE/DROITE : CHANGER");
    txt_at(3, 22, "A SUR LE NOM : L'ECRIRE");
    txt_at(2, 25, "START : JOUER   B : RETOUR");
    cu_live();
}

/* first : 1 au début (réglages lus dans opt_val : joueur 1, partenaire
   humain en double), 0 : page du partenaire */
extern u8 opt_val[];
u16 __fastcall__ cust_build(u16 first)
{
    u8 k;
    if (!first) { cust_page(); return 0; }
    cu_c[0] = opt_val[3];                       /* (O_P1 ; O_P2 4, O_TYPE 2, O_CPART 20 : screens.c) */
    cu_c[1] = opt_val[4] == cu_c[0] ? (u8)((cu_c[0] + 1) % NCHARS) : opt_val[4];
    cu_n = (u8)(opt_val[2] && opt_val[20] ? 2 : 1);
    if (cu_sav[0] != CU_MAGIC) { cu_from_char(0, cu_c[0]); cu_from_char(1, cu_n > 1 ? cu_c[1] : 0); cu_sav[0] = CU_MAGIC; }
    /* profil inactif : repart du personnage choisi */
    for (k = 0; k < cu_n; ++k) if (!(cu_sav[CU_SLOT(k) + 8] & 1)) cu_from_char(k, cu_c[k]);
    cu_k = 0; cu_row = 0; cu_ed = 0;
    cust_page();
    return 0;
}

/* à chaque trame : 0 continuer, 1 terminé, 2 page suivante (partenaire),
   3 retour au menu */
u16 __fastcall__ cust_tick(u16 pad)
{
    u8 *s = cu_sav + CU_SLOT(cu_k), r = cu_row, i, ch;
    spr_begin();
    spr_person(212, 150, cu_c[cu_k], 1, (frame_cnt & 64) ? POSE_JOY : POSE_IDLE, 0, 0);
    spr_end();
    if (cu_ed) {                                /* saisie du nom */
        if (pad & (PAD_A | PAD_B | PAD_START)) { cu_ed = 0; sfx_menu_ok(); }
        else if ((pad & PAD_LEFT) && cu_pos) { --cu_pos; sfx_menu_move(); }
        else if ((pad & PAD_RIGHT) && cu_pos < 6) { ++cu_pos; sfx_menu_move(); }
        else if (pad & (PAD_UP | PAD_DOWN)) {
            ch = s[cu_pos];
            for (i = 0; cu_abc[i] && cu_abc[i] != (char)ch; ++i) ;
            if (!cu_abc[i]) i = 0;
            if (pad & PAD_UP) { if (!cu_abc[++i]) i = 0; } else i = (u8)(i ? i - 1 : sizeof(cu_abc) - 2);
            s[cu_pos] = (u8)cu_abc[i];
            s[8] |= 1; sfx_menu_val(1);
            cu_value(0, 1);
        } else return 0;
        cu_value(1, 1);
        return 0;
    }
    if (pad & PAD_B) { sfx_menu_back(); cust_reset(); return 3; }
    if ((pad & PAD_START) || ((pad & PAD_A) && r == CU_ROWS - 1)) {
        sfx_menu_ok();
        /* nom vide : celui du personnage */
        for (i = 0; i < 7 && s[i] == ' '; ++i) ;
        if (i == 7) { ch = s[8]; cu_from_char(cu_k, cu_c[cu_k]); s[8] = ch; }
        cust_reset();
        if (cu_k + 1 < cu_n) { ++cu_k; cu_row = 0; return 2; }
        return 1;
    }
    if (pad & (PAD_UP | PAD_DOWN)) {
        vbuf_text(1, CU_Y(r), " ", 1);
        r = (u8)((pad & PAD_UP) ? (r ? r - 1 : CU_ROWS - 1) : (r + 1 < CU_ROWS ? r + 1 : 0));
        cu_row = r;
        vbuf_text(1, CU_Y(r), ">", 1);
        sfx_menu_move();
        return 0;
    }
    if ((pad & PAD_A) && r == 1) { cu_ed = 1; cu_pos = 0; s[8] |= 1; sfx_menu_ok(); cu_value(0, 1); cu_value(1, 1); cu_live(); return 0; }
    if (pad & (PAD_LEFT | PAD_RIGHT | PAD_A)) {
        u8 d = (u8)((pad & PAD_LEFT) ? 0 : 1);
        switch (r) {
        case 0: s[8] ^= 1; break;
        case 2: s[8] ^= 2; s[8] |= 1; break;
        case 3: i = cu_find(cu_skin, 5, s[9]); s[9] = cu_skin[d ? (i + 1) % 5 : (i + 4) % 5]; s[8] |= 1; break;
        case 4: i = cu_find(cu_hair, 6, s[11]); s[11] = cu_hair[d ? (i + 1) % 6 : (i + 5) % 6]; s[8] |= 1; break;
        case 5: i = cu_find(cu_out, 15, s[10]); s[10] = cu_out[d ? (i + 1) % 15 : (i + 14) % 15]; s[8] |= 1; break;
        default: return 0;
        }
        sfx_menu_val(d);
        cu_value(0, 1); cu_value(r, 1);
        cu_live();
    }
    return 0;
}

/* reprise d'un match de compétition sauvegardé : profil d'après cfg */
static void cust_resume(void)
{
    u8 k, c, *t;
    cust_reset();
    if (pod_buf[1] != 3 || cu_sav[0] != CU_MAGIC) return;   /* (3 : MODE_COMP, screens.c) */
    for (k = 0; k < 2; ++k) {
        c = k ? (cfg.doubles && cfg.ctrl[2] == CTRL_PAD2 ? cfg.chr[2] : 255) : cfg.chr[0];
        t = cu_sav + CU_SLOT(k);
        if (c < NCHARS && (t[8] & 1)) { cu_map[k] = (u8)(c + 1); char_female[c] = (u8)((t[8] >> 1) & 1); }
    }
}
