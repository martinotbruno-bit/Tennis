/* Royal Martinot — pages de choix illustrées (banque 29).
 *
 * Après les réglages (mode, compétition, format…), les choix qui se voient
 * se font sur des pages à part, dans l'ordre : le tournoi (compétition
 * « tournoi » : un des 7 tournois Martinot, sur l'image de son annonce),
 * le ou les joueurs (affiche VS : bustes, noms, styles), puis le court :
 * lieu, surface et météo sur la même page, le vrai court en aperçu
 * avec ses couleurs et la lumière choisie (nuit, pluie qui tombe, couchant).
 * HAUT/BAS : ligne ; GAUCHE/DROITE : changer ; A : valider (page suivante) ;
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
u16 __fastcall__ venue_pal_f(u16 cond);   /* extra2.c : court Margaux selon la surface, soirée en salle */

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
#define O_EX_      15                  /* (exercice d'entraînement) */
/* (3.02) entraînement sans adversaire (tous les exercices sauf le joueur fixe) */
#define SOLO() (opt_val[O_MODE] == MODE_TRAIN && opt_val[O_EX_] != EX_STATIC)
#define MODE_DEMO    5
extern u8 opt_val[];
extern u8 cu_c[4];                      /* (gfx.c, page zéro : personnages des pages TON JOUEUR) */
#pragma zpsym("cu_c")

/* pages */
#define PK_P1   1
#define PK_P2   2
#define PK_EV   3
#define PK_CRT  4                       /* lieu, surface et météo */
/* lignes de la page du court */
#define R_ENV   1
#define R_SURF  2
#define R_COND  4

#pragma bss-name(push, "WBSS")
static u8 pk_i, pk_t, pk_ev, pk_row;
u8 pk_off;                              /* (banc d'essai : pages sautées, parcours d'avant) */
#pragma bss-name(pop)

u16 __fastcall__ cust_preview_f(u16 unused); void cust_reset(void);   /* career.c (BANK_CAR) */
void vs_build(void); void vs_texts(void); u8 __fastcall__ vs_anim(u8 t);   /* banques 8 et 19 */
void menu_deco_begin(void);
void load_court(void);                                                  /* banque 3 */
extern u8 gm_ev;
void gm_intro_build(void); void gm_intro_tick(void);                     /* banque 14 */
extern u8 persp;
extern u8 ppu_emph;
#pragma zpsym("ppu_emph")

/* les 7 tournois Martinot (ordre des images d'annonce, gmintro.c) : surface */
static const u8 ev_surf[7] = { 2, 0, 1, 2, 5, 4, 3 };

/* lignes de la page du court selon le mode : en compétition, le lieu et
   la surface du tournoi Martinot (ou de la saison) sont imposés */
static u8 pk_rows(void)
{
    u8 m = opt_val[O_MODE], ct = opt_val[O_COMP];
    if (m != MODE_COMP) return R_ENV | R_SURF | R_COND;
    if (ct == CT_KO) return R_COND;
    if (ct >= CT_GRAND) return 0;
    return (u8)(ct == CT_SEASON ? R_ENV | R_COND : R_ENV | R_SURF | R_COND);
}
static u8 pk_list(u8 *l)
{
    u8 n = 0, m = opt_val[O_MODE], ct = opt_val[O_COMP];
    if (m == MODE_DEMO) return 0;
    if (m == MODE_COMP && ct == CT_KO) l[n++] = PK_EV;           /* d'abord le tournoi */
    /* en compétition, les joueurs sont choisis sur TON JOUEUR : pas
       de page de choix du joueur (on pouvait en rechoisir un avec A) */
    if (m != MODE_COMP) {
        l[n++] = PK_P1;
        if (m == MODE_1P || m == MODE_2P || m == MODE_2P_TEAM || (m == MODE_TRAIN && !SOLO())) l[n++] = PK_P2;   /* (3.02 : entraînement contre le joueur fixe : on le choisit) */
    }
    (void)ct;
    if (pk_rows()) l[n++] = PK_CRT;
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
    if (m == MODE_COMP) far_call(BANK_CAR, (void *)cust_preview_f, 0);   /* (le joueur conçu sur TON JOUEUR) */
    pal_set_player(0, cfg.chr[0]);
    pal_set_player(1, cfg.chr[1]);
    bank_call(BANK_PODIUM, vs_build);
    bank_call(BANK_CAR, vs_texts);
    txt_at(3, 1, st == PK_P1 ? "TON JOUEUR" : partner() ? "TON PARTENAIRE" : m != MODE_1P && m != MODE_TRAIN ? "JOUEUR 2" : "TON ADVERSAIRE");
    /* compétition à deux joueurs humains : inscrits ensemble, pas
       encore adversaires : « ET » plutôt que « CONTRE » */
    txt_at(13, 11, partner() || (m == MODE_COMP && opt_val[O_CPART]) ? "  ET  " : "CONTRE");
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
    if (SOLO()) {                       /* (3.02) entraînement sans adversaire : la moitié droite vide */
        u8 y;
        for (y = 3; y < 23; ++y) { vram_adr(NTADR(12, y)); vram_fill(0, 20); }
        pk_t = 27;
    }
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

/* météo possible : pas de pluie ni de crépuscule en salle ; crépuscule
   en match seulement (ni entraînement, ni compétition) */
static u8 cond_ok(u8 v)
{
    u8 m = opt_val[O_MODE];
    if (opt_val[O_ENV] == ENV_HALL && v >= COND_RAIN) return 0;
    return (u8)(v != COND_SUNSET || (m != MODE_TRAIN && m != MODE_COMP));
}
static const char * const cond_nm[4] = { "JOUR", "NUIT", "PLUIE", "CREPUSCULE" };

/* les lignes du haut (zone du score) : curseur, nom, valeur entre flèches */
static void rows_text(u8 now)
{
    static const char * const lbl[3] = { "LIEU", "SURFACE", "METEO" };
    char b[29];
    const char *q, *t;
    u8 r, i, n, y = 1, rs = pk_rows();
    for (r = 0; r < 3; ++r) {
        if (!(rs & (1 << r))) continue;
        for (i = 0; i < 28; ++i) b[i] = ' ';
        b[28] = 0;
        if (pk_row == r) b[1] = '>';
        for (t = lbl[r], i = 3; *t; ) b[i++] = *t++;
        q = r == 0 ? env_name[opt_val[O_ENV]] : r == 1 ? surf_name[opt_val[O_SURF]] : (opt_val[O_COND] == COND_NIGHT && opt_val[O_ENV] == ENV_HALL ? (const char *)"SOIREE" : cond_nm[opt_val[O_COND]]);   /* (en salle : soirée) */
        for (n = 0; q[n]; ++n) ;
        while (n && q[n - 1] == ' ') --n;               /* (noms de surface complétés d'espaces) */
        i = 11; b[i++] = '<'; b[i++] = ' ';               /* (13 caractères : COURT MARGAUX) */
        while (n-- && i < 26) b[i++] = *q++;
        b[i + 1] = '>';
        if (now) txt_at(2, y, b); else vbuf_text(2, y, b, 28);
        ++y;
    }
}

/* le court en aperçu (vue à plat), lieu, surface et lumière choisis ; le
   texte dans le haut de l'écran (zone du score) */
static void draw_court(void)
{
    u8 v = cfg.view;
    ppu_off();
    vbuf_reset();
    spr_begin(); spr_end();
    scroll_x = 0; scroll_y = 0; scroll_nt = 0;
    if (!cond_ok(opt_val[O_COND])) opt_val[O_COND] = COND_DAY;
    while (!(pk_rows() & (1 << pk_row))) pk_row = (u8)((pk_row + 1) % 3);
    cfg.surface = opt_val[O_SURF];
    cfg.env = opt_val[O_ENV];
    cfg.cond = opt_val[O_COND];
    cfg.chr[0] = opt_val[O_P1];
    cfg.chr[1] = opt_val[O_P2];
    cfg.view = 0;
    vram_adr(0x2000);
    bank_call(BANK_EXTRAS, load_court);
    cfg.view = v;
    persp = 0;
    /* crépuscule : la lumière du couchant (accentuation rouge) */
    pal_set_court(cfg.surface, cfg.cond == COND_SUNSET ? COND_DAY : cfg.cond, cfg.env);
    far_call(BANK_X2, (void *)venue_pal_f, cfg.cond == COND_SUNSET ? COND_DAY : cfg.cond);   /* (court Margaux selon la surface, soirée en salle) */
    ppu_emph = (u8)(cfg.cond == COND_SUNSET ? (is_pal ? 0x40 : 0x20) : 0);
    if (cfg.cond == COND_SUNSET && cfg.env == ENV_CASTLE) { pal_buf[9] = 0x37; pal_buf[10] = 0x27; }   /* (pierre dorée, comme en match) */
    pal_players_match(cfg.chr[0], cfg.chr[1]);                                 /* (comme en match) */
    vram_adr(0x2000); vram_fill(0, 128);                 /* (rangées du score : le texte) */
    vram_adr(0x23C0); vram_fill(0x55, 8);                /* palette du texte sur les 4 rangées : au court Margaux, la palette rose du décor rendait les lettres baveuses */
    rows_text(1);
    ppu_on();
}

static void pk_music(void);
static void draw(void)
{
    u8 st = pk_stage();
    fade_skip = 1;
    if (st == PK_EV) draw_event();
    else if (st == PK_CRT) { draw_court(); pk_music(); }
    else { draw_players(st); pk_music(); }
}

/* fin des pages : motifs d'origine (images, court), décor des menus */
/* musique des menus : la polka, ou la valse du court Margaux */
static void pk_music(void)
{
    u8 m = opt_val[O_ENV] == ENV_MARGAUX ? MUS_MARGAUX : MUS_TITLE;
    if (music_cur != m) music_play(m);
}
static u16 leave(u8 r)
{
    ppu_off();
    split_stop();
    ppu_emph = 0;
    bank_call(BANK_CAR, cust_reset);    /* (profils : appliqués par la compétition elle-même) */
    chr_reload();
    spr_begin(); spr_end();
    pk_music();                         /* (après la fanfare d'un tournoi) */
    return r;
}

/* first : 1 depuis les réglages ; 0 : aucune page, le parcours continue */
u16 __fastcall__ pick_begin(u16 first)
{
    u8 l[6], e;
    (void)first;
    pk_i = 0;
    pk_row = 0;
    if (!cond_ok(opt_val[O_COND])) opt_val[O_COND] = COND_DAY;   /* (salle : ni pluie ni crépuscule, même d'un ancien réglage) */
    if (pk_off || !pk_list(l)) return 0;
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
    /* page du court : HAUT/BAS change de ligne (sans redessiner le court) */
    if (st == PK_CRT && (p & (PAD_UP | PAD_DOWN))) {
        n = pk_rows();
        do pk_row = (u8)((pk_row + ((p & PAD_DOWN) ? 1 : 2)) % 3); while (!(n & (1 << pk_row)));
        sfx_menu_move();
        rows_text(0);
        return 0;
    }
    if (p & (PAD_LEFT | PAD_RIGHT)) {
        d = (s8)((p & PAD_RIGHT) ? 1 : -1);
        sfx_menu_val((u8)(p & PAD_RIGHT));
        if (st == PK_EV) {
            pk_ev = (u8)((pk_ev + 7 + d) % 7);
            opt_val[O_SURF] = ev_surf[pk_ev];
            opt_val[O_ENV] = (u8)(pk_ev == 6 ? ENV_HALL : ENV_STADIUM);   /* (Deborcy : en salle) */
        } else if (st == PK_CRT) {
            if (pk_row == 0) {
                v = opt_val[O_ENV];
                do v = (u8)((v + NENV + d) % NENV); while (bad_env(v));
                opt_val[O_ENV] = v;
                if (!cond_ok(opt_val[O_COND])) opt_val[O_COND] = COND_DAY;
            } else if (pk_row == 1) {
                opt_val[O_SURF] = (u8)((opt_val[O_SURF] + NSURF + d) % NSURF);
            } else {
                v = opt_val[O_COND];
                do v = (u8)((v + 4 + d) & 3); while (!cond_ok(v));
                opt_val[O_COND] = v;
            }
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
        if (!SOLO()) spr_person(136, 62, cfg.chr[1], 1, POSE_IDLE, 0, 1);   /* (sous les 3 lignes du haut ; 3.02 : pas d'adversaire à l'entraînement sans adversaire) */
        if (cfg.cond == COND_RAIN) {
            /* pluie : 10 gouttes qui tombent en biais */
            for (n = 0; n < 10; ++n) spr1((u8)(n * 25 + 7 + (frame_cnt >> 1)), (u8)(40 + (u8)(n * 53 + (frame_cnt << 2)) % 190), 4, 3);
        }
        spr_end();
    }
    return 0;
}

/* (déplacé de screens.c, banque 1 pleine) réglages -> compétition */
#define O_FMT    9
#define O_CSIZE  12
#define O_CPROG  13
#define O_CVAR   14
u16 __fastcall__ comp_opts_f(u16 unused)
{
    (void)unused;
    c_type = opt_val[O_COMP];
    c_opts = (u8)((opt_val[O_CSIZE] ? CO_SIZE8 : 0) | (opt_val[O_CPROG] ? CO_PROG : 0) | (opt_val[O_CVAR] ? CO_VARIED : 0));
    c_lvl = opt_val[O_LEVEL];
    c_surf = (u8)(opt_val[O_SURF] > 3 && (c_type == CT_SEASON || c_type >= CT_GRAND) ? 0 : opt_val[O_SURF]);
    c_env = opt_val[O_ENV];
    c_cond = opt_val[O_COND];
    if (c_cond == COND_SUNSET) c_cond = COND_DAY;   /* (le mot de passe ne code que 3 conditions) */
    if (c_type >= CT_GRAND) { c_env = ENV_STADIUM; c_cond = 0; }   /* Grand Martinot, Martinworld Tour : toujours au stade, de jour */
    c_fmt = opt_val[O_FMT];
    c_pl[0] = opt_val[O_P1];
    c_dbl = opt_val[O_TYPE];
    c_hum2 = (u8)(opt_val[O_TYPE] && opt_val[O_CPART]);   /* (double : partenaire à la manette 2) */
    /* en simple : 1 à 4 joueurs humains (O_CPART : joueurs en plus),
       personnages : les deux choisis, puis ceux des pages TON JOUEUR 3 et 4 */
    c_nh = (u8)(opt_val[O_TYPE] ? 1 : 1 + opt_val[O_CPART]);
    c_hc[0] = opt_val[O_P1]; c_hc[1] = opt_val[O_P2]; c_hc[2] = cu_c[2]; c_hc[3] = cu_c[3];
    c_pl2[0] = opt_val[O_P2];
    if (c_dbl && c_pl2[0] == c_pl[0]) c_pl2[0] = (u8)((c_pl[0] + 1) % NCHARS);
    c_seed = (u8)(rand8() ^ frame_cnt);
    return 0;
}

/* ---------------------------------------------------------------------- */
/* Menu des réglages (déplacé de screens.c, banque 1 pleine) : lignes
   visibles, modes affichés, valeur expliquée par l'aide. menu_f(op << 8 | v) :
   op 0 lignes visibles, 1 mode affiché, 2 choisir le mode affiché v,
   3 valeur de la ligne v pour l'aide */
#define O_START  0
#define O_REPLAY 10
#define O_EX     15
#define O_SIDE   16
#define O_SPEED  17
#define O_SPIN   18
#define O_COUNT  19
#define O_GSPEED 21
#define O_SOUND  22
#define O_FATIG  23
#define O_VIEW   24
#define O_CAREER 25
#define NOPT     26
#define O_VERSUS 26
#define O_OPTS   27
#define O_BACK   28
#define NMODES   6
extern u8 vis[], nvis, cursor, menu_pg;
static void __fastcall__ add_opt(u8 o) { vis[nvis++] = o; }
/* Modes affichés (le mode interne, opt_val[O_MODE], ne change pas : moteur
   et sauvegardes) : 1 JOUEUR (simple ; en double, avec un partenaire CPU :
   mode interne en équipe), 2 JOUEURS (CONTRE : mode 2 joueurs ; ENSEMBLE :
   en équipe, partenaire à la manette 2), COMPETITION, CARRIERE (compétition
   qui compte pour la carrière : opt_val[O_CAREER], toujours en simple, à un
   joueur), ENTRAINEMENT, DEMO */
#define D_1P    0
#define D_2P    1
#define D_COMP  2
#define D_CAR   3
static u8 dmode(void)
{
    u8 m = opt_val[O_MODE];
    if (m == MODE_2P_TEAM) return (u8)(opt_val[O_CPART] ? D_2P : D_1P);
    if (m == MODE_COMP) return (u8)(opt_val[O_CAREER] ? D_CAR : D_COMP);
    return m;
}
static void __fastcall__ set_dmode(u8 d)
{
    static const u8 im[NMODES] = { MODE_1P, MODE_2P, MODE_COMP, MODE_COMP, MODE_TRAIN, MODE_DEMO };
    opt_val[O_MODE] = im[d];
    opt_val[O_CAREER] = (u8)(d == D_CAR);
    if (d == D_CAR) { opt_val[O_TYPE] = 0; opt_val[O_CPART] = 0; if (opt_val[O_COMP] == CT_CUSTOM) opt_val[O_COMP] = 0; }
}
static void build_visible(void)
{
    u8 m = opt_val[O_MODE], d = dmode();
    if (d == D_CAR) set_dmode(D_CAR);   /* (réglages d'avant : la carrière, toujours en simple, à un joueur) */
    nvis = 0;
    add_opt(O_START);
    if (menu_pg) {
        /* options : ce qui se règle une fois pour toutes */
        if (m != MODE_TRAIN) add_opt(O_REPLAY);
        add_opt(O_GSPEED);
        if (m != MODE_TRAIN) add_opt(O_FATIG);
        add_opt(O_VIEW);
        add_opt(O_SOUND);
        add_opt(O_BACK);
        if (cursor >= nvis) cursor = 0;
        return;
    }
    add_opt(O_MODE);
    if (d == D_COMP || d == D_CAR) {
        add_opt(O_COMP);
        if (opt_val[O_COMP] == CT_CUSTOM) { add_opt(O_CSIZE); add_opt(O_CPROG); add_opt(O_CVAR); }
        if (d == D_COMP) {
            add_opt(O_TYPE);
            add_opt(O_CPART);   /* HUMAINS : 1 à 4 en simple, 1 ou 2 en double */
        }
        add_opt(O_LEVEL);
        /* Grand Martinot, Martinworld Tour : au stade, de jour, surfaces
           imposées, 3 sets et finale en 5. Joueurs, tournoi, lieu et
           surface : pages illustrées après les réglages (pick.c) */
        if (opt_val[O_COMP] < CT_GRAND) add_opt(O_FMT);   /* (la météo, sur la page du court) */
    } else if (m == MODE_TRAIN) {
        add_opt(O_EX);
        if (opt_val[O_EX] == EX_CUSTOM) { add_opt(O_SIDE); add_opt(O_SPEED); add_opt(O_SPIN); add_opt(O_COUNT); }
    } else {
        if (d == D_2P) add_opt(O_VERSUS);
        if (d == D_1P || m == MODE_2P) add_opt(O_TYPE);
        add_opt(O_LEVEL);
        if (m == MODE_DEMO) add_opt(O_COND);   /* (sinon sur la page du court, pick.c) */
        add_opt(O_FMT);
    }
    add_opt(O_OPTS);
    if (cursor >= nvis) cursor = 0;
}
/* valeur expliquée par l'aide (lignes sans valeur rangée, modes affichés) */
static u8 __fastcall__ help_val(u8 o)
{
    if (o == O_MODE) return dmode();
    if (o == O_VERSUS || (o == O_TYPE && dmode() == D_1P)) return (u8)(opt_val[O_MODE] == MODE_2P_TEAM);
    if (o >= NOPT) return 0;
    return opt_val[o];
}
/* touches sur les lignes propres au menu (p : touches pressées) : 0 rien,
   1 lignes à refaire, 2 lignes à refaire, curseur sur OPTIONS */
static u8 __fastcall__ menu_keys(u8 p)
{
    u8 o = vis[cursor], d = dmode();
    /* page des options : l'ouvrir (A ou DROITE), en revenir (A, GAUCHE ou B) */
    if ((o == O_OPTS && (p & (PAD_A | PAD_RIGHT))) || (menu_pg && ((p & PAD_B) || (o == O_BACK && (p & (PAD_A | PAD_LEFT)))))) {
        sfx_menu_ok();
        menu_pg ^= 1;
        cursor = 0;
        return (u8)(menu_pg ? 1 : 2);
    }
    /* lignes dont la valeur est le mode interne */
    if ((o == O_MODE || o == O_VERSUS || (o == O_TYPE && d == D_1P)) && (p & (PAD_LEFT | PAD_RIGHT))) {
        if (o == O_MODE) set_dmode((u8)((p & PAD_RIGHT) ? (d + 1 == NMODES ? 0 : d + 1) : (d ? d - 1 : NMODES - 1)));
        else if (opt_val[O_MODE] == MODE_2P_TEAM) opt_val[O_MODE] = (u8)(d == D_2P ? MODE_2P : MODE_1P);
        else { opt_val[O_MODE] = MODE_2P_TEAM; opt_val[O_CPART] = (u8)(d == D_2P); }
        sfx_menu_val((u8)(p & PAD_RIGHT));
        return 1;
    }
    return 0;
}
u16 __fastcall__ menu_f(u16 a)
{
    u8 v = (u8)a;
    switch ((u8)(a >> 8)) {
    case 0: build_visible(); return 0;
    case 1: return dmode();
    case 2: set_dmode(v); return 0;
    case 3: return help_val(v);
    default: return menu_keys(v);
    }
}
