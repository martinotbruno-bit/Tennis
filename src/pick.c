/* Royal Martinot 2.76 — pages de choix illustrées (banque 29).
 *
 * Après les réglages (mode, compétition, format…), les choix qui se voient
 * se font sur des pages à part, dans l'ordre : le tournoi (compétition
 * « tournoi » : un des 7 tournois Martinot, sur l'image de son annonce),
 * le ou les joueurs (affiche VS : bustes, noms, styles), puis le lieu et la
 * surface (le vrai court en aperçu, avec ses couleurs et la lumière du
 * moment). GAUCHE/DROITE : changer ; A : valider (page suivante) ;
 * B : page précédente (ou retour aux réglages) ; START : tout valider.
 *
 * screens.c appelle pick_begin (0 : aucune page, on continue), puis
 * pick_tick à chaque trame (1 : fini, 2 : retour aux réglages). Les choix
 * sont écrits dans opt_val, comme le faisaient les lignes du menu. */
#include "app.h"
#include "gfx.h"
#include "game.h"
#include "audio.h"
#include "comp.h"

#pragma static-locals(off)          /* (RAM de cartouche presque pleine : variables sur la pile) */

/* options (screens.c) */
#define O_MODE   1
#define O_TYPE   2
#define O_P1     3
#define O_P2     4
#define O_LEVEL  5
#define O_SURF   6
#define O_ENV    7
#define O_COND   8
#define O_COMP   11
#define O_CPART  20
#define MODE_1P      0
#define MODE_2P      1
#define MODE_2P_TEAM 2
#define MODE_COMP    3
#define MODE_TRAIN   4
#define MODE_DEMO    5
extern u8 opt_val[];

/* pages */
#define PK_P1   1
#define PK_P2   2
#define PK_EV   3
#define PK_ENV  4
#define PK_SURF 5

#pragma bss-name(push, "WBSS")
static u8 pk_i, pk_t, pk_ev;
#pragma bss-name(pop)

void vs_build(void); void vs_texts(void); u8 __fastcall__ vs_anim(u8 t);   /* banques 8 et 19 */
void menu_deco_begin(void);
void load_court(void);                                                  /* banque 3 */
extern u8 gm_ev;
void gm_intro_build(void); void gm_intro_tick(void);                     /* banque 14 */
extern u8 persp;

/* les 7 tournois Martinot (ordre des images d'annonce, gmintro.c) : surface */
static const u8 ev_surf[7] = { 2, 0, 1, 2, 5, 4, 3 };

static u8 pk_list(u8 *l)
{
    u8 n = 0, m = opt_val[O_MODE], ct = opt_val[O_COMP];
    if (m == MODE_DEMO) return 0;
    if (m == MODE_COMP && ct == CT_KO) l[n++] = PK_EV;           /* d'abord le tournoi */
    l[n++] = PK_P1;
    if (m == MODE_1P || m == MODE_2P || m == MODE_2P_TEAM || (m == MODE_COMP && (opt_val[O_TYPE] || opt_val[O_CPART]))) l[n++] = PK_P2;   /* (2.76 : compétition à deux) */
    if (m == MODE_COMP) {
        if (ct != CT_KO && ct < CT_GRAND) { l[n++] = PK_ENV; if (ct != CT_SEASON) l[n++] = PK_SURF; }
    } else { l[n++] = PK_ENV; l[n++] = PK_SURF; }
    return n;
}
static u8 pk_stage(void) { u8 l[6]; return pk_i < pk_list(l) ? l[pk_i] : 0; }

/* le partenaire (et non l'adversaire) : en équipe, ou en compétition */
static u8 partner(void) { u8 m = opt_val[O_MODE]; return (u8)(m == MODE_2P_TEAM || (m == MODE_COMP && opt_val[O_TYPE])); }

/* texte centré sur 28 colonnes (colonne 2), le reste effacé */
static void line_at(u8 y, const char *s)
{
    char b[29];
    u8 n = 0, i, x;
    while (s[n]) ++n;
    for (i = 0; i < 28; ++i) b[i] = ' ';
    x = (u8)((28 - n) >> 1);
    for (i = 0; i < n; ++i) b[x + i] = s[i];
    b[28] = 0;
    txt_at(2, y, b);
}
static void with_arrows2(u8 y, const char *t, const char *s)
{
    char b[30];
    u8 n = 0;
    while (*t) b[n++] = *t++;
    b[n++] = '<'; b[n++] = ' ';
    while (*s && n < 26) b[n++] = *s++;
    while (n && b[n - 1] == ' ') --n;                 /* (noms de surface complétés d'espaces) */
    b[n++] = ' '; b[n++] = '>'; b[n] = 0;
    line_at(y, b);
}
#define with_arrows(y, s) with_arrows2(y, "", s)

static void begin_screen(void)
{
    ppu_off();
    vbuf_reset();
    people_tiles(0);
    chr_borrow_restore();
    clear_screen();
    pal_set_menu();
    bank_call(BANK_PODIUM, menu_deco_begin);
    spr_begin(); spr_end();
    scroll_x = 0; scroll_y = 0; scroll_nt = 0;
}

/* l'affiche VS : le joueur 1 à gauche, l'adversaire (ou le partenaire) à
   droite ; le côté choisi entre flèches */
static void draw_players(u8 st)
{
    u8 m = opt_val[O_MODE];
    begin_screen();
    cfg.surface = opt_val[O_SURF] < NSURF ? opt_val[O_SURF] : 0;
    cfg.env = ENV_STADIUM;
    cfg.doubles = 0;
    cfg.chr[0] = opt_val[O_P1];
    cfg.chr[1] = opt_val[O_P2];
    cfg.ctrl[0] = CTRL_PAD1;
    cfg.ctrl[1] = (u8)(m == MODE_2P || ((m == MODE_2P_TEAM || m == MODE_COMP) && opt_val[O_CPART]) ? CTRL_PAD2 : CTRL_CPU);
    cfg.level[1] = opt_val[O_LEVEL];
    pal_set_player(0, cfg.chr[0]);
    pal_set_player(1, cfg.chr[1]);
    bank_call(BANK_PODIUM, vs_build);
    bank_call(BANK_CAR, vs_texts);
    txt_at(3, 1, st == PK_P1 ? "TON JOUEUR" : partner() ? "TON PARTENAIRE" : m != MODE_1P ? "JOUEUR 2" : "TON ADVERSAIRE");
    txt_at(13, 11, partner() ? "  ET  " : "CONTRE");
    /* flèches autour du nom choisi (rangée 19) */
    txt_at((u8)(st == PK_P1 ? 1 : 17), 19, "<");
    {
        const char *nm = chr_nm(cfg.chr[st == PK_P1 ? 0 : 1]);
        u8 n = 0; while (nm[n] && nm[n] != ' ') ++n;
        txt_at((u8)((st == PK_P1 ? 2 : 18) + n), 19, ">");
    }
    line_at(24, "GAUCHE/DROITE : CHOISIR");
    line_at(25, "A : VALIDER    B : RETOUR");
    line_at(27, "START : TOUT VALIDER");
    pk_t = 23;                          /* (affiche déjà en place : recopie du buste droit) */
    ppu_on();
}

/* l'image d'annonce du tournoi Martinot */
static void draw_event(void)
{
    char b[24];
    const char *q;
    u8 i = 0;
    begin_screen();
    gm_ev = pk_ev;
    bank_call(BANK_GMI, gm_intro_build);
    line_at(2, "CHOIX DU TOURNOI");
    b[i++] = (char)('1' + pk_ev); b[i++] = '/'; b[i++] = '7'; b[i++] = ' '; b[i++] = ':'; b[i++] = ' ';
    for (q = surf_name[ev_surf[pk_ev]]; *q; ) b[i++] = *q++;
    b[i] = 0;
    with_arrows(23, b);
    line_at(25, "GAUCHE/DROITE : CHOISIR");
    line_at(27, "A : VALIDER    B : RETOUR");
    ppu_on();
}

/* le court en aperçu (vue à plat), lieu et surface choisis, lumière du
   moment ; le texte dans le haut de l'écran (zone du score) */
static void draw_court(u8 st)
{
    u8 v = cfg.view;
    ppu_off();
    vbuf_reset();
    spr_begin(); spr_end();
    scroll_x = 0; scroll_y = 0; scroll_nt = 0;
    cfg.surface = opt_val[O_SURF];
    cfg.env = opt_val[O_ENV];
    cfg.cond = opt_val[O_COND];
    if (cfg.env == ENV_HALL && cfg.cond >= COND_RAIN) cfg.cond = COND_DAY;
    cfg.chr[0] = opt_val[O_P1];
    cfg.chr[1] = opt_val[O_P2];
    cfg.view = 0;
    vram_adr(0x2000);
    bank_call(BANK_EXTRAS, load_court);
    cfg.view = v;
    persp = 0;
    pal_set_court(cfg.surface, cfg.cond, cfg.env);
    pal_set_player(0, cfg.chr[0]);
    pal_set_player(1, cfg.chr[1]);
    vram_adr(0x2000); vram_fill(0, 96);                  /* (rangées du score : le texte) */
    if (st == PK_ENV) with_arrows2(1, "LIEU : ", env_name[cfg.env]);
    else with_arrows2(1, "SURFACE : ", surf_name[cfg.surface]);
    line_at(2, "A : VALIDER   B : RETOUR");
    ppu_on();
}

static void draw(void)
{
    u8 st = pk_stage();
    fade_skip = 1;
    if (st == PK_EV) draw_event();
    else if (st == PK_ENV || st == PK_SURF) draw_court(st);
    else draw_players(st);
}

/* fin des pages : motifs d'origine (images, court), décor des menus */
static u16 leave(u8 r)
{
    ppu_off();
    split_stop();
    chr_reload();
    spr_begin(); spr_end();
    music_play(MUS_NONE);
    return r;
}

/* first : 1 depuis les réglages ; 0 : aucune page, le parcours continue */
u16 __fastcall__ pick_begin(u16 first)
{
    u8 l[6], e;
    (void)first;
    pk_i = 0;
    if (!pk_list(l)) return 0;
    /* tournoi : celui de la surface choisie la dernière fois */
    pk_ev = 0;
    for (e = 0; e < 7; ++e) if (ev_surf[e] == opt_val[O_SURF]) { pk_ev = e; break; }
    if (opt_val[O_ENV] == ENV_HALL && opt_val[O_SURF] == 3) pk_ev = 6;
    draw();
    return 1;
}

static u8 bad_env(u8 v)
{
    if (v != ENV_NUDE) return 0;
    return (u8)(!nude_unlocked || opt_val[O_P1] == 16 || opt_val[O_P2] == 16);   /* (Margaux ne va jamais au club nudiste) */
}

u16 __fastcall__ pick_tick(u16 pad)
{
    u8 p = (u8)pad, st = pk_stage(), l[6], n, v, o;
    s8 d;
    if (p & PAD_START) { sfx_menu_ok(); return leave(1); }
    if (p & PAD_A) {
        sfx_menu_ok();
        n = pk_list(l);
        if (++pk_i >= n) return leave(1);
        draw();
        return 0;
    }
    if (p & PAD_B) {
        sfx_menu_back();
        if (!pk_i) return leave(2);
        --pk_i;
        draw();
        return 0;
    }
    if (p & (PAD_LEFT | PAD_RIGHT)) {
        d = (s8)((p & PAD_RIGHT) ? 1 : -1);
        sfx_menu_val((u8)(p & PAD_RIGHT));
        if (st == PK_EV) {
            pk_ev = (u8)((pk_ev + 7 + d) % 7);
            opt_val[O_SURF] = ev_surf[pk_ev];
            opt_val[O_ENV] = (u8)(pk_ev == 6 ? ENV_HALL : ENV_STADIUM);   /* (Deborcy : en salle) */
        } else if (st == PK_ENV) {
            v = opt_val[O_ENV];
            do v = (u8)((v + NENV + d) % NENV); while (bad_env(v));
            opt_val[O_ENV] = v;
        } else if (st == PK_SURF) {
            opt_val[O_SURF] = (u8)((opt_val[O_SURF] + NSURF + d) % NSURF);
        } else {
            o = (u8)(st == PK_P1 ? O_P1 : O_P2);
            v = opt_val[o];
            do v = (u8)((v + NCHARS + d) % NCHARS);
            while (v == opt_val[o == O_P1 ? O_P2 : O_P1] || (v == 16 && opt_val[O_ENV] == ENV_NUDE));
            opt_val[o] = v;
        }
        draw();
        return 0;
    }
    /* animations de la page */
    if (st == PK_EV) bank_call(BANK_GMI, gm_intro_tick);
    else if (st == PK_P1 || st == PK_P2) {
        if (pk_t < 26) far_call(BANK_PODIUM, (void *)vs_anim, ++pk_t);           /* buste droit recopié */
        else if (pk_t == 26) { split_stop(); far_call(BANK_PODIUM, (void *)vs_anim, 40); ++pk_t; }
    } else {
        /* les deux joueurs sur le court, au repos */
        spr_begin();
        spr_person(120, 202, cfg.chr[0], 0, (u8)(((frame_cnt >> 5) & 1) ? POSE_IDLE : POSE_DRIBBLE), 0, 0);
        spr_person(136, 46, cfg.chr[1], 1, POSE_IDLE, 0, 1);
        spr_end();
    }
    return 0;
}
