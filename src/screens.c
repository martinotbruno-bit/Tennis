/* Royal Martinot — écrans hors match (banque BANK_SCREENS) : titre,
 * configuration, présentation, résultats, compétitions, codes, palmarès. */
#include "app.h"
#include "game.h"
#include "gfx.h"
#include "rules.h"
#include "audio.h"
#include "assets.h"
#include "password.h"
#include "comp.h"
#include "stats.h"

#pragma static-locals(on)

/* sous-états (lus par les tests automatiques) */
#define SS_TITLE     0
#define SS_MENU      1
#define SS_RESULTS   2
#define SS_PRESENT   3
#define SS_COMP      4
#define SS_CODE      5
#define SS_PALMARES  6
#define SS_TRAIN_RES 7
#define SS_COMP_END  8
#define SS_GMI      10   /* annonce d'un tournoi du Grand Martinot */
#define SS_TEASER   11   /* bande-annonce du Grand Martinot et du Martinworld Tour */
void teaser_build(void); void teaser_tick(void);
extern u8 teaser_done;
#define SS_SETBREAK  9
u8 scr_state;

/* ---------------------------------------------------------------------- */
/* Options de configuration                                                */
#define O_START  0
#define O_MODE   1
#define O_TYPE   2
#define O_P1     3
#define O_P2     4
#define O_LEVEL  5
#define O_SURF   6
#define O_ENV    7
#define O_COND   8
#define O_FMT    9
#define O_REPLAY 10
#define O_COMP   11
#define O_CSIZE  12
#define O_CPROG  13
#define O_CVAR   14
#define O_EX     15
#define O_SIDE   16
#define O_SPEED  17
#define O_SPIN   18
#define O_COUNT  19
#define O_CPART  20
#define O_GSPEED 21   /* vitesse du jeu : lent, normal, rapide */
#define O_SOUND  22   /* son : tout, bruitages seuls, aucun */
#define O_FATIG  23   /* fatigue dans les très longs échanges */
#define O_VIEW   24   /* vue : à plat, perspective */
#define NOPT     25

#define MODE_1P      0
#define MODE_2P      1
#define MODE_2P_TEAM 2
#define MODE_COMP    3
#define MODE_TRAIN   4
#define MODE_DEMO    5
#define NMODES       6

u8 opt_val[NOPT] = { 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 2, 1, 0, 1, 0, 1, 0, 1, 0 };
static const u8 opt_max[NOPT] = { 1, NMODES, 2, NCHARS, NCHARS, 3, NSURF, NENV, 3, 4, 2, NCT, 2, 2, 2, NEX, 4, 3, 3, 3, 2, 3, 3, 2, 2 };
static const char * const opt_label[NOPT] = {
    "COMMENCER", "MODE", "JEU", "JOUEUR 1", "JOUEUR 2", "NIVEAU CPU", "SURFACE", "LIEU",
    "CONDITIONS", "FORMAT", "RALENTIS", "COMPETITION", "JOUEURS", "ADVERSAIRES", "SURFACES",
    "EXERCICE", "COTE", "VITESSE", "EFFET", "BALLES", "JOUE PAR", "VITESSE JEU", "SON", "FATIGUE", "VUE" };
static const char * const mode_name[NMODES] = {
    "1 JOUEUR VS CPU", "2 J. ADVERSAIRES", "EN EQUIPE", "COMPETITION", "ENTRAINEMENT", "DEMO CPU" };
static const char * const cond_name[3] = { "JOUR", "NUIT", "PLUIE" };
static const char * const level_name[3] = { "FACILE", "NORMAL", "EXPERT" };
static const char * const fmt_name[4] = { "1 SET", "3 SETS", "5 SETS", "RAPIDE 4 JEUX" };
static const char * const ex_name[NEX] = {
    "FRAPPES/CIBLES", "DEPLACEMENTS", "REGULARITE", "PRECISION", "VOLEE", "SERVICE",
    "JOUEUR FIXE", "PERSONNALISE", "TUTORIEL" };
static const char * const side_name[4] = { "COUP DROIT", "REVERS", "ALTERNE", "ALEATOIRE" };
static const char * const speed_name[3] = { "LENTE", "MOYENNE", "RAPIDE" };
static const char * const gspeed_name[3] = { "LENT", "NORMAL", "RAPIDE" };
static const char * const sound_name[3] = { "MUSIQUE + SONS", "BRUITAGES SEULS", "AUCUN" };
static const char * const spin_name[3] = { "LIFTE", "COUPE", "LOB" };
static const u8 count_val[3] = { 10, 20, 30 };

/* nom de la compétition t (liste en banque 5) : copié dans comp_l2 */
static const char *cname(u8 t) { COMP(comp_cname, t); return comp_l2; }
/* titre du Grand Martinot ou du Martinworld Tour */
static void gs_title(u8 y) { txt_at(3, y, c_type == CT_GRAND ? "LE GRAND MARTINOT" : "MARTINWORLD TOUR"); }
/* écran d'étape ; au premier tour d'un tournoi du Grand Martinot, d'abord
   son annonce (banque 14 : image, nom, fanfare) */
extern u8 gm_ev;
void gm_intro_build(void);
void gm_intro_tick(void);
static void comp_screen(void);
static void screen_begin(void);
static void comp_next(void)
{
    /* l'image du tournoi (premier tour) s'affiche juste avant le début du
       match, après sa présentation (gm_ev : $FF = aucune) */
    gm_ev = COMP(comp_gm_intro, 0);
    comp_screen();
}
static u8 vis[NOPT];
u8 nvis, cursor;     /* non statiques : lus par les tests */
static u8 last_mode, demo_dbl;
static u16 idle;
static char line[32];

/* ajoute une ligne au menu (appel court : la banque 1 est pleine) */
static void __fastcall__ add_opt(u8 o) { vis[nvis++] = o; }
static void build_visible(void)
{
    u8 m = opt_val[O_MODE];
    nvis = 0;
    add_opt(O_START);
    add_opt(O_MODE);
    if (m == MODE_COMP) {
        add_opt(O_COMP);
        if (opt_val[O_COMP] == CT_CUSTOM) { add_opt(O_CSIZE); add_opt(O_CPROG); add_opt(O_CVAR); }
        add_opt(O_TYPE);
        add_opt(O_P1);
        if (opt_val[O_TYPE]) { add_opt(O_P2); add_opt(O_CPART); }   /* partenaire */
        add_opt(O_LEVEL);
        /* Grand Martinot, Martinworld Tour : au stade, de jour, surfaces
           imposées, 3 sets et finale en 5 ; saison : surfaces imposées */
        if (opt_val[O_COMP] < CT_GRAND) {
            if (opt_val[O_COMP] != CT_SEASON) add_opt(O_SURF);
            add_opt(O_ENV); add_opt(O_COND); add_opt(O_FMT);
        }
        add_opt(O_REPLAY);
        add_opt(O_GSPEED);
    } else if (m == MODE_TRAIN) {
        add_opt(O_EX);
        if (opt_val[O_EX] == EX_CUSTOM) { add_opt(O_SIDE); add_opt(O_SPEED); add_opt(O_SPIN); add_opt(O_COUNT); }
        add_opt(O_P1);
        add_opt(O_SURF);
        add_opt(O_ENV);
        add_opt(O_COND);
        add_opt(O_GSPEED);
    } else {
        /* simple ou double : seulement à 2 joueurs adversaires. « 2 J. EQUIPE »
           est toujours en double ; « 1 joueur contre CPU » toujours en simple
           (son double, avec un partenaire CPU, est le mode en équipe) */
        if (m == MODE_2P) add_opt(O_TYPE);
        add_opt(O_P1);
        add_opt(O_P2);
        if (m == MODE_2P_TEAM) add_opt(O_CPART);   /* partenaire : CPU ou manette 2 */
        add_opt(O_LEVEL);
        add_opt(O_SURF);
        add_opt(O_ENV);
        add_opt(O_COND);
        add_opt(O_FMT);
        add_opt(O_REPLAY);
        add_opt(O_GSPEED);
    }
    if (m != MODE_TRAIN) add_opt(O_FATIG);
    add_opt(O_VIEW);
    add_opt(O_SOUND);
    if (cursor >= nvis) cursor = 0;
}

static const char *opt_text(u8 o)
{
    u8 v = opt_val[o];
    switch (o) {
    case O_MODE:   return mode_name[v];
    case O_TYPE:   return v ? "DOUBLE" : "SIMPLE";
    case O_P1:
    case O_P2:     return char_name[v];
    case O_LEVEL:  return level_name[v];
    case O_SURF:   return surf_name[v];
    case O_ENV:    return env_name[v];
    case O_COND:
        if (opt_val[O_ENV] == ENV_HALL && v == COND_RAIN) return "PLUIE > JOUR";
        return cond_name[v];
    case O_FMT:    return fmt_name[v];
    case O_REPLAY: return v ? "OUI" : "NON";
    case O_COMP:   return cname(v);
    case O_CSIZE:  return v ? "8" : "4";
    case O_CPROG:  return v ? "PROGRESSIFS" : "NIVEAU FIXE";
    case O_CVAR:   return v ? "VARIEES" : "IDENTIQUES";
    case O_EX:     return ex_name[v];
    case O_SIDE:   return side_name[v];
    case O_SPEED:  return speed_name[v];
    case O_SPIN:   return spin_name[v];
    case O_COUNT:  return v == 0 ? "10" : v == 1 ? "20" : "30";
    case O_CPART:  return v ? "MANETTE 2" : "CPU";
    case O_GSPEED: return gspeed_name[v];
    case O_SOUND:  return sound_name[v];
    case O_FATIG:  return v ? "OUI" : "NON";
    case O_VIEW:   return v ? "PERSPECTIVE" : "A PLAT";
    }
    return "";
}

static void pad_text(const char *s, u8 n)
{
    u8 i;
    for (i = 0; i < n; ++i) line[i] = *s ? *s++ : ' ';
    line[n] = 0;
}

/* ---------------------------------------------------------------------- */
/* Musique des menus : polka du titre (sans reprise depuis le titre), ou valse du court Margaux quand ce
   lieu est choisi ; finale d'une compétition : thème de la finale */
static void menu_music(void)
{
    u8 m = opt_val[O_ENV] == ENV_MARGAUX ? MUS_MARGAUX : MUS_TITLE;
    if (music_cur != m) music_play(m);
}
static u8 is_final(void)
{
    /* nom d'étape (comp_l1, ou comp_l2 en saison) exactement « FINALE » */
    return (u8)((comp_l1[0] == 'F' && comp_l1[6] == 0) || (comp_l2[0] == 'F' && comp_l2[6] == 0));
}

void menu_deco_begin(void); void deco_title(void); void deco_frame(void); void deco_filet(void); void palmares_build(void);
extern char deco_str[24];
extern u8 deco_kind, deco_row;
static void screen_begin(void)
{
    ppu_off();
    vbuf_reset();
    people_tiles(0);
    chr_borrow_restore();            /* arbitre du match : coupe d'origine */
    clear_screen();
    pal_set_menu();
    bank_call(BANK_PODIUM, menu_deco_begin);   /* police surlignée, ornements dorés */
    spr_begin(); spr_end();
}
/* habillage : titre en lettres doublées (rangées 1-2) et filet, attributs
   de l'écran (kind : 0 texte, 1 configuration), filet du bas (rangée row) */
static void deco(const char *title, u8 kind, u8 row)
{
    u8 i = 0;
    while ((deco_str[i] = title[i]) != 0) ++i;
    deco_kind = kind; deco_row = row;
    bank_call(BANK_PODIUM, deco_title);
    bank_call(BANK_PODIUM, deco_frame);
}

static void set_attr_rows(u8 y0, u8 y1, u8 pal)
{
    u8 ay, ax, v = (u8)(pal | (pal << 2) | (pal << 4) | (pal << 6));
    for (ay = (u8)(y0 >> 2); ay <= (u8)(y1 >> 2); ++ay) {
        vram_adr((u16)(0x23C0 + ay * 8));
        for (ax = 0; ax < 8; ++ax) vram_put(v);
    }
}

/* logo géant sur deux lignes : « ROYAL » (lettres 0 à 4) puis « MARTINOT »
   (lettres 6 à 13), centrés ; chaque lettre fait 2 x 2 tuiles */
static void logo_line(u8 x, u8 y, u8 first, u8 n)
{
    u8 i;
    vram_adr(NTADR(x, y));
    for (i = first; i < first + n; ++i) { vram_put(logo_tiles[i * 4]); vram_put(logo_tiles[i * 4 + 1]); }
    vram_adr(NTADR(x, y + 1));
    for (i = first; i < first + n; ++i) { vram_put(logo_tiles[i * 4 + 2]); vram_put(logo_tiles[i * 4 + 3]); }
}
static void draw_logo(u8 y)
{
    logo_line(11, y, 0, 5);
    logo_line(8, (u8)(y + 2), 6, 8);
}

/* ---------------------------------------------------------------------- */
/* Titre                                                                   */
u8 title_cur;
static const char * const title_items[4] = { "JOUER", "MODE D'EMPLOI", "PALMARES", "CODE DE REPRISE" };

static u8 opts_loaded;
static void title_screen(void)
{
    u8 i;
    SV(sv_check);
    if (sv.nude) nude_unlocked = 1;
    if (!opts_loaded) {
        /* derniers réglages du menu (cartouche à pile), bornés par sécurité */
        opts_loaded = 1;
        if (far_call(BANK_COMP, (void *)so_load, 0)) {
            for (i = 0; i < NOPT; ++i) if (opt_val[i] >= opt_max[i]) opt_val[i] = 0;
            if (opt_val[O_ENV] == ENV_NUDE && (!nude_unlocked || opt_val[O_P1] == 16 || opt_val[O_P2] == 16)) opt_val[O_ENV] = 0;
        }
        snd_set(opt_val[O_SOUND]);
    }
    screen_begin();
    set_attr_rows(0, 7, 2);
    draw_logo(2);
    txt_at(6, 7, "GRAND SLAM PRESTIGE");
    for (i = 0; i < 4; ++i) {
        vram_adr(NTADR(14, 8 + i));
        vram_put(bigball_tiles[i * 4]); vram_put(bigball_tiles[i * 4 + 1]);
        vram_put(bigball_tiles[i * 4 + 2]); vram_put(bigball_tiles[i * 4 + 3]);
    }
    set_attr_rows(8, 11, 2);
    txt_at(4, 13, "PLATINUM DIAMOND EDITION");
    set_attr_rows(12, 15, 3);
    set_attr_rows(24, 27, 3);
    deco_row = 15; bank_call(BANK_PODIUM, deco_filet);
    deco_row = 24; bank_call(BANK_PODIUM, deco_filet);
    pal_buf[9] = 0x27; pal_buf[10] = 0x28; pal_buf[11] = 0x30;   /* logo et balle : couleurs d'origine */
    for (i = 0; i < 4; ++i) txt_at(10, (u8)(16 + i * 2), title_items[i]);
    txt_at(4, 25, "HAUT/BAS PUIS A - START");
    txt_at(4, 27, "(C) 2026 BRUNO MARTINOT");
    ppu_on();
    if (music_cur != MUS_TITLE) music_play(MUS_TITLE);   /* même polka qu'au menu, sans reprise */
    scr_state = SS_TITLE;
    idle = 0;
}

/* ---------------------------------------------------------------------- */
/* Configuration                                                           */
static u8 rd_row, rd_n;
static u8 hl_cur = 0xFF;      /* ligne affichée surlignée ($FF : aucune) */
static void menu_draw_row(u8 i, u8 direct)
{
    txt_off = (u8)(i == cursor ? 0x40 : 0);   /* ligne choisie : police surlignée */
    pad_text(opt_text(vis[i]), 15);
    if (direct) {
        if (vis[i] == O_P2 && (opt_val[O_MODE] == MODE_COMP || opt_val[O_MODE] == MODE_2P_TEAM)) txt_at(3, (u8)(4 + i), "PARTENAIRE");
        else txt_at(3, (u8)(4 + i), opt_label[vis[i]]);
        txt_at(16, (u8)(4 + i), line);
        if (i == cursor) hl_cur = i;
    }
    else {
        vbuf_text(16, (u8)(4 + i), line, 15);
        if (i == cursor) hl_cur = 0xFF;     /* valeur changée : flèches redessinées */
    }
    txt_off = 0;
}
/* ligne i : libellé et valeur, surlignés (on) avec des flèches autour de la
   valeur, ou normaux ; par le tampon VRAM (0 : pas de place cette trame) */
static u8 menu_hl_row(u8 i, u8 on)
{
    static const char ar[2][2] = { { ' ', ' ' }, { '<', '>' } };
    if (vram_len + 50 > VBUF_MAX) return 0;
    txt_off = (u8)(on ? 0x40 : 0);
    pad_text(vis[i] == O_P2 && (opt_val[O_MODE] == MODE_COMP || opt_val[O_MODE] == MODE_2P_TEAM) ? (const char *)"PARTENAIRE" : (const char *)opt_label[vis[i]], 13);
    vbuf_text(3, (u8)(4 + i), line, 13);
    pad_text(opt_text(vis[i]), 15);
    vbuf_text(16, (u8)(4 + i), line, 15);
    txt_off = 0;
    if (vis[i] != O_START) {
        u8 n = 15;
        while (n && line[n - 1] == ' ') --n;             /* fin de la valeur */
        line[0] = ar[on][0]; vbuf_text(15, (u8)(4 + i), line, 1);
        line[0] = ar[on][1]; vbuf_text((u8)(16 + n), (u8)(4 + i), line, 1);
    }
    return 1;
}
/* le surlignage suit le curseur (après la réécriture des lignes) */
static void menu_hl_tick(void)
{
    if (rd_row < rd_n) return;
    if (hl_cur != cursor && hl_cur != 0xFF) { if (hl_cur < nvis && !menu_hl_row(hl_cur, 0)) return; hl_cur = 0xFF; }
    if (hl_cur == 0xFF && menu_hl_row(cursor, 1)) hl_cur = cursor;
}

/* Changement de mode (ou de compétition, d'exercice) : la liste des lignes
   change. Au lieu de couper l'écran pour tout reconstruire (éclair noir), on
   réécrit les lignes par le tampon VRAM, deux par trame environ. */
static void menu_soft(void)
{
    u8 old = nvis;
    build_visible();
    hl_cur = 0xFF;              /* toutes les lignes sont réécrites : surlignage ensuite */
    if (cursor >= nvis) cursor = 0;
    rd_n = old > nvis ? old : nvis;
    rd_row = 0;
}
static void menu_soft_tick(void)
{
    u8 i;
    while (rd_row < rd_n && vram_len + 40 <= VBUF_MAX) {
        i = rd_row++;
        if (i < nvis) {
            pad_text(vis[i] == O_P2 && (opt_val[O_MODE] == MODE_COMP || opt_val[O_MODE] == MODE_2P_TEAM) ? (const char *)"PARTENAIRE" : (const char *)opt_label[vis[i]], 13);
            vbuf_text(3, (u8)(4 + i), line, 13);
            pad_text(opt_text(vis[i]), 15);
        } else {
            pad_text("", 13);
            vbuf_text(3, (u8)(4 + i), line, 13);
            pad_text("", 15);
        }
        vbuf_text(16, (u8)(4 + i), line, 15);
        line[0] = ' '; vbuf_text(15, (u8)(4 + i), line, 1); vbuf_text(31, (u8)(4 + i), line, 1);
    }
}

/* Bas du menu (rangées 24 à 27) : style de l'adversaire et aide des
   touches ; en mode compétition, curseur sur la ligne COMPETITION :
   explication de la compétition choisie (comp_desc, banque 5). Redessiné
   une ligne par trame (budget VBlank) quand ce qu'il faut montrer change. */
static u8 info_k, info_row;          /* réglés par menu_screen */
static void menu_style_line(u8 direct)
{
    u8 c = opt_val[O_MODE] == MODE_COMP || opt_val[O_MODE] == MODE_TRAIN ? opt_val[O_P1] : opt_val[O_P2];
    if (info_k != 0xFF) return;          /* explication affichée */
    pad_text(style_name[char_style[c]], 14);
    if (direct) { txt_at(3, 24, "STYLE"); txt_at(9, 24, line); }
    else vbuf_text(9, 24, line, 14);
}
static void menu_info_tick(void)
{
    u8 want = (u8)(opt_val[O_MODE] == MODE_COMP && vis[cursor] == O_COMP ? opt_val[O_COMP] : 0xFF);
    if (want != info_k) { info_k = want; info_row = 0; }
    while (info_row < 4 && vram_len + 32 <= VBUF_MAX) {
        if (info_k != 0xFF) {
            COMP(comp_desc, (u8)(info_k * 4 + info_row));
            vbuf_text(2, (u8)(24 + info_row), comp_l1, 28);
        } else if (info_row == 0) {
            pad_text(" STYLE", 28);
            vbuf_text(2, 24, line, 28);
            info_k = 0xFF;
            menu_style_line(0);
        } else {
            COMP(comp_desc, (u8)(NCT * 4 + info_row));      /* aide des touches */
            vbuf_text(2, (u8)(24 + info_row), comp_l1, 28);
        }
        ++info_row;
    }
}

static void menu_screen(void)
{
    u8 i;
    build_visible();
    rd_row = rd_n = 0;
    screen_begin();
    deco("REGLAGES", 1, 22);
    for (i = 0; i < nvis; ++i) menu_draw_row(i, 1);
    info_k = 0xFF; info_row = 4;
    menu_style_line(1);
    for (i = 1; i < 4; ++i) { COMP(comp_desc, (u8)(NCT * 4 + i)); txt_at(2, (u8)(24 + i), comp_l1); }
    pal_set_player(0, opt_val[O_P1]);
    pal_set_player(1, opt_val[O_P2]);
    ppu_on();
    scr_state = SS_MENU;
    menu_music();
}

static void menu_sprites(void)
{
    static const u8 bounce[8] = { 0, 1, 2, 3, 3, 2, 1, 0 };
    menu_hl_tick();
    spr_begin();
    spr1(9, (u8)(4 * 8 + cursor * 8 - 1 - bounce[(frame_cnt >> 2) & 7]), (u8)(ball_base + 8), 3);   /* balle qui rebondit */
    {   /* pastille de la couleur de la surface, devant sa valeur */
        static const u8 sc[6][3] = { { 0x27, 0x17, 0x37 }, { 0x3A, 0x2A, 0x30 }, { 0x21, 0x11, 0x31 },
                                     { 0x24, 0x14, 0x34 }, { 0x38, 0x28, 0x30 }, { 0x31, 0x21, 0x30 } };
        u8 k;
        for (k = 0; k < nvis; ++k) if (vis[k] == O_SURF) {
            const u8 *c = sc[opt_val[O_SURF] < 6 ? opt_val[O_SURF] : 0];
            if (pal_buf[25] != c[0]) { pal_buf[25] = c[0]; pal_buf[26] = c[1]; pal_buf[27] = c[2]; pal_dirty = 1; }
            spr1(110, (u8)(4 * 8 + k * 8 - 1), (u8)(ball_base + 8), 2);
        }
    }
    spr_person(28, 29, opt_val[O_P1], 1, (u8)((frame_cnt >> 5) & 1 ? POSE_IDLE : POSE_JOY), 0, 0);
    if (opt_val[O_MODE] <= MODE_2P_TEAM || opt_val[O_MODE] == MODE_DEMO)
        spr_person(220, 29, opt_val[O_P2], 1, (u8)((frame_cnt >> 5) & 1 ? POSE_JOY : POSE_IDLE), 1, 1);
    spr_end();
}

static u8 unused_char(u8 a, u8 b, u8 c)
{
    u8 i;
    for (i = 0; i < NCHARS; ++i) if (i != a && i != b && i != c) return i;
    return 0;
}

static void base_cfg(void)
{
    u8 f = opt_val[O_FMT], i;
    cfg.surface = opt_val[O_SURF];
    cfg.env = opt_val[O_ENV];
    cfg.cond = opt_val[O_COND];
    /* cohérence météo / lieu (CDC 6.3) : pas de pluie en salle */
    if (cfg.env == ENV_HALL && cfg.cond == COND_RAIN) cfg.cond = COND_DAY;
    cfg.sets_to_win = (u8)(f == 1 ? 2 : f == 2 ? 3 : 1);
    cfg.games_per_set = (u8)(f == 3 ? 4 : 6);
    cfg.replays = opt_val[O_REPLAY];
    cfg.gspeed = opt_val[O_GSPEED];
    cfg.fatigue = opt_val[O_FATIG];
    cfg.view = opt_val[O_VIEW];
    cfg.demo = 0;
    cfg.mode = GM_MATCH;
    cfg.doubles = 0;
    for (i = 0; i < 4; ++i) { cfg.level[i] = opt_val[O_LEVEL]; cfg.ctrl[i] = CTRL_CPU; }
    cfg.ctrl[0] = CTRL_PAD1;
}

static void setup_cfg(u8 mode)
{
    base_cfg();
    cfg.doubles = opt_val[O_TYPE];
    cfg.chr[0] = opt_val[O_P1];
    cfg.chr[1] = opt_val[O_P2];
    if (cfg.chr[1] == cfg.chr[0]) cfg.chr[1] = unused_char(cfg.chr[0], 255, 255);
    cfg.chr[2] = unused_char(cfg.chr[0], cfg.chr[1], 255);
    cfg.chr[3] = unused_char(cfg.chr[0], cfg.chr[1], cfg.chr[2]);
    switch (mode) {
    case MODE_1P: cfg.doubles = 0; break;     /* le double avec partenaire CPU : mode en équipe */
    case MODE_2P: cfg.ctrl[1] = CTRL_PAD2; break;
    case MODE_2P_TEAM:
        /* deux joueurs dans la même équipe : toujours un double ; JOUEUR 2 est
           le personnage de la manette 2, les adversaires CPU sont tirés à part */
        cfg.doubles = 1;
        /* partenaire joué par la manette 2 ou par l'ordinateur */
        cfg.ctrl[2] = opt_val[O_CPART] ? CTRL_PAD2 : CTRL_CPU;
        cfg.chr[2] = opt_val[O_P2] == cfg.chr[0] ? unused_char(cfg.chr[0], 255, 255) : opt_val[O_P2];
        cfg.chr[1] = unused_char(cfg.chr[0], cfg.chr[2], 255);
        cfg.chr[3] = unused_char(cfg.chr[0], cfg.chr[1], cfg.chr[2]);
        break;
    case MODE_TRAIN:
        cfg.mode = GM_TRAIN;
        cfg.doubles = 0;
        cfg.fatigue = 0;               /* pas de fatigue à l'entraînement */
        cfg.ex = opt_val[O_EX];
        cfg.ex_side = opt_val[O_SIDE];
        cfg.ex_speed = opt_val[O_SPEED];
        cfg.ex_spin = opt_val[O_SPIN];
        cfg.ex_count = count_val[opt_val[O_COUNT]];
        cfg.chr[1] = unused_char(cfg.chr[0], 255, 255);
        break;
    case MODE_DEMO:
        cfg.ctrl[0] = CTRL_CPU;
        cfg.demo = 1;
        cfg.gspeed = 1;
        cfg.level[0] = cfg.level[2] = 2; cfg.level[1] = cfg.level[3] = 1;
        break;
    }
    last_mode = mode;
}

static void comp_cfg(void)
{
    u8 o = COMP(comp_opponent, 0);
    base_cfg();
    cfg.chr[0] = c_pl[0];
    cfg.chr[1] = c_pl[o];
    cfg.level[1] = COMP(comp_level, 0);
    if (c_dbl) {
        /* double : partenaire du joueur (CPU) et partenaire de l'adversaire */
        cfg.doubles = 1;
        cfg.chr[2] = c_pl2[0];
        cfg.chr[3] = c_pl2[o];
        cfg.level[3] = cfg.level[1];
        cfg.level[2] = (u8)(c_lvl ? c_lvl : 1);
        if (c_hum2) cfg.ctrl[2] = CTRL_PAD2;    /* partenaire joué par la manette 2 */
    }
    cfg.surface = COMP(comp_surface, 0);
    cfg.env = c_env;
    cfg.view = opt_val[O_VIEW];
    cfg.cond = c_cond;
    if (cfg.env == ENV_HALL && cfg.cond == COND_RAIN) cfg.cond = COND_DAY;
    cfg.sets_to_win = (u8)(c_fmt == 1 ? 2 : c_fmt == 2 ? 3 : 1);
    cfg.games_per_set = (u8)(c_fmt == 3 ? 4 : 6);
    last_mode = MODE_COMP;
}

/* ---------------------------------------------------------------------- */
/* Présentation avant le match (CDC 7.5)                                   */
static void team_line(u8 t, u8 y)
{
    /* nom complet (jusqu'à 7 lettres : MARGAUX) ; en double, le partenaire
       sur la ligne suivante */
    txt_at(6, y, char_name[cfg.chr[t]]);
    if (cfg.doubles) { txt_at(6, (u8)(y + 1), "+"); txt_at(8, (u8)(y + 1), char_name[cfg.chr[t + 2]]); }
    if (cfg.ctrl[t] == CTRL_CPU) txt_at(18, y, level_name[cfg.level[t]]);
    else txt_at(18, y, cfg.ctrl[t] == CTRL_PAD1 ? "MANETTE 1" : "MANETTE 2");
    if (cfg.doubles && cfg.ctrl[t + 2] == CTRL_PAD2) txt_at(18, (u8)(y + 1), "+ MANETTE 2");
    else if (cfg.ctrl[t] == CTRL_CPU) txt_at(18, (u8)(y + 1), style_name[char_style[cfg.chr[t]]]);
}

void vs_build(void);
/* affiche VS : nom, partenaire, manette ou niveau et style, sous chaque buste */
static void vs_team(u8 t, u8 x)
{
    txt_at(x, 19, char_name[cfg.chr[t]]);
    if (cfg.doubles) { txt_at(x, 20, "+"); txt_at((u8)(x + 2), 20, char_name[cfg.chr[t + 2]]); }
    if (cfg.ctrl[t] == CTRL_CPU) txt_at(x, 21, level_name[cfg.level[t]]);
    else txt_at(x, 21, cfg.ctrl[t] == CTRL_PAD1 ? "MANETTE 1" : "MANETTE 2");
    if (cfg.doubles && cfg.ctrl[t + 2] == CTRL_PAD2) txt_at(x, 22, "+ MANETTE 2");
    else if (cfg.ctrl[t] == CTRL_CPU) txt_at(x, 22, style_name[char_style[cfg.chr[t]]]);
}
static void present_screen(void)
{
    u8 vs = (u8)(cfg.env != ENV_NUDE);       /* (au club nudiste : la présentation d'origine) */
    screen_begin();
    people_tiles((u8)(cfg.env == ENV_NUDE));
    pal_set_player(0, cfg.chr[0]);
    pal_set_player(1, cfg.chr[1]);
    if (vs) bank_call(BANK_PODIUM, vs_build);
    if (cfg.env == ENV_NUDE) {
        txt_at(4, 24, "CLUB NUDISTE LES BRONZES");
        txt_at(4, 25, "TENUE EXIGEE : AUCUNE");
    }
    if (last_mode == MODE_COMP) {
        COMP(comp_cname, c_type); txt_at(3, 1, comp_l2);
        COMP(comp_stage_name, 0);
        txt_at(3, 2, comp_l1);
        txt_at(3, 3, comp_l2);
    } else txt_at(3, 1, cfg.doubles ? "MATCH DE DOUBLE" : "MATCH AMICAL");
    if (vs) {
        vs_team(0, 2);
        txt_at(13, 11, "CONTRE");
        vs_team(1, 18);
        if (cfg.env == ENV_MARGAUX) txt_at(2, 3, "BIENVENUE AU COURT MARGAUX !");
        txt_at(2, 24, surf_name[cfg.surface]);
        txt_at(18, 24, env_name[cfg.env]);
        txt_at(2, 25, cond_name[cfg.cond]);
        txt_at(18, 25, fmt_name[cfg.games_per_set == 4 ? 3 : cfg.sets_to_win - 1]);   /* format réel du match */
    } else {
        team_line(0, 10);
        txt_at(14, 13, "CONTRE");
        team_line(1, 16);
        txt_at(4, 21, surf_name[cfg.surface]);
        txt_at(17, 21, env_name[cfg.env]);
        txt_at(4, 22, cond_name[cfg.cond]);
        txt_at(17, 22, fmt_name[cfg.games_per_set == 4 ? 3 : cfg.sets_to_win - 1]);
    }
    txt_at(4, 27, "A : JOUER   B : RETOUR");
    ppu_on();
    if (last_mode == MODE_COMP && is_final()) music_play(MUS_FINAL);
    else if (cfg.env == ENV_MARGAUX && music_cur != MUS_MARGAUX) music_play(MUS_MARGAUX);
    scr_state = SS_PRESENT;
}

static void present_sprites(void)
{
    /* double : les partenaires à côté, sans recouvrir les noms (colonne 6) */
    u8 x = (u8)(cfg.doubles ? 22 : 30);
    if (cfg.env != ENV_NUDE) return;          /* affiche VS : les bustes, sans sprites */
    spr_begin();
    spr_person(x, 98, cfg.chr[0], 1, POSE_JOY, 0, 0);
    spr_person(x, 146, cfg.chr[1], 1, POSE_JOY, 1, 1);
    if (cfg.doubles) {
        spr_person(39, 98, cfg.chr[2], 1, POSE_IDLE, 0, 0);
        spr_person(39, 146, cfg.chr[3], 1, POSE_IDLE, 1, 1);
    }
    spr_end();
}

/* ---------------------------------------------------------------------- */
/* Compétitions : écran d'étape avec tableau ou classement et code         */
static PwData pw;
static char code_buf[PW_LEN + 1];

static void make_code(void)
{
    pw.type = c_type; pw.stage = c_stage; pw.seed = c_seed; pw.chr = c_pl[0];
    pw.surf = c_surf; pw.fmt = c_fmt; pw.cond = c_cond; pw.env = c_env; pw.lvl = c_lvl;
    pw.results = c_results; pw.opts = c_opts; pw.trophies = c_trophies; pw.gres = c_gres;
    pw.doubles = c_dbl; pw.partner = (u8)(c_dbl ? c_pl2[0] : 0); pw.hum2 = c_hum2;
    pw_s = code_buf; pw_p = &pw;
    (void)far_call(BANK_COMP, (void *)pw_enc_far, 0);
    code_buf[PW_LEN] = 0;
}

static void comp_screen(void)
{
    u8 o;
    screen_begin();
    deco(c_type == CT_GRAND ? (const char *)"GRAND MARTINOT" : c_type == CT_WORLD ? (const char *)"MARTINWORLD" : cname(c_type), 0, 24);
    COMP(comp_stage_name, 0);
    txt_off = 0x40; txt_at(3, 5, comp_l1); txt_off = 0;   /* en or */
    txt_at(3, 6, comp_l2);
    COMP(comp_path, 7);
    o = COMP(comp_opponent, 0);
    team_label(c_pl[0], c_pl2[0], line);
    txt_off = 0x40; txt_at(3, 9, line); txt_off = 0;
    txt_at(9, 9, "CONTRE");
    team_label(c_pl[o], c_pl2[o], line);
    txt_at(16, 9, line);
    txt_at(3, 10, "ADVERSAIRE :");
    txt_at(16, 10, level_name[COMP(comp_level, 0)]);
    txt_at(3, 11, surf_name[COMP(comp_surface, 0)]);
    txt_at(17, 11, env_name[c_env]);
    COMP(comp_draw_table, 13);
    make_code();
    txt_at(3, 22, "CODE :");
    vram_adr(NTADR(10, 22));
    for (o = 0; o < PW_LEN; ++o) { vram_put((u8)code_buf[o]); if (o == 3 || o == 7) vram_put(' '); }
    if (c_dbl) txt_at(3, 23, c_hum2 ? "DOUBLE A 2 JOUEURS" : "COMPETITION EN DOUBLE");
    txt_at(3, 25, "A : JOUER   B : MENU");
    {   /* médailles des 3 premiers (classements) : or, argent, bronze */
        static const u8 med[9] = { 0x38, 0x28, 0x30,  0x30, 0x10, 0x20,  0x37, 0x17, 0x27 };
        for (o = 0; o < 9; ++o) pal_buf[17 + (o / 3) * 4 + o % 3] = med[o];
    }
    ppu_on();
    if (is_final()) { if (music_cur != MUS_FINAL) music_play(MUS_FINAL); }
    else if (music_cur == MUS_FINAL) music_play(MUS_NONE);
    scr_state = SS_COMP;
}

static const char * const rank_txt[6] = { "", "VAINQUEUR !", "FINALISTE", "DEMI-FINALISTE", "4E", "ELIMINE" };
static u8 was_unlocked;
extern u8 pod_busy;             /* podium.c : cérémonie du Grand Martinot en cours */
/* Fin de compétition : victoire = podium (banque BANK_PODIUM) ; sinon,
   classement et joueur déçu */
static void comp_end_screen(void)
{
    char b[6];
    screen_begin();
    was_unlocked = nude_unlocked;
    if (c_rank == 1) {
        nude_unlocked = 1;
        pod_new = (u8)!was_unlocked;
        bank_call(BANK_PODIUM, podium_build);
    } else {
        pal_set_player(0, c_pl[0]);
        if (c_type >= CT_GRAND) gs_title(2);
        else { txt_at(3, 2, "ROYAL MARTINOT"); txt_at(18, 2, cname(c_type)); }
        if (c_dbl) { team_label(c_pl[0], c_pl2[0], b); txt_at(3, 3, b); txt_at(9, 3, "(DOUBLE)"); }
        if (c_type == CT_LEAGUE || c_type == CT_SEASON || c_type >= CT_GRAND) {
            txt_at(5, 6, "CLASSEMENT FINAL :");
            num2(b, c_rank); b[2] = 'E'; b[3] = 0;
            txt_at(23, 6, b);
            c_stage = (u8)(c_type == CT_LEAGUE ? 5 : 99);   /* toutes les étapes (tableau limité au dernier tournoi) */
            COMP(comp_draw_table, 11);
        } else {
            txt_at(5, 6, rank_txt[c_rank > 5 ? 5 : c_rank]);
            if (c_type == CT_GROUPS && c_rank == 5) txt_at(5, 8, "EN PHASE DE POULES");
        }
        txt_at(5, 21, "TROPHEES :");
        num2(b, c_trophies); b[2] = 0;
        txt_at(16, 21, b);
        txt_at(5, 26, "A : RETOUR AU TITRE");
    }
    ppu_on();
    SV(sv_comp_end);
    music_play(c_rank == 1 ? MUS_VICTORY : MUS_DEFEAT);
    if (c_rank == 1) sfx_crowd(7);
    c_active = 0;
    scr_state = SS_COMP_END;
}

static void comp_end_sprites(void)
{
    if (c_rank == 1) { bank_call(BANK_PODIUM, podium_sprites); return; }
    spr_begin();
    spr_person(200, 206, c_pl[0], 1, POSE_SAD, 0, 0);
    spr_end();
}

static void start_comp(void)
{
    c_type = opt_val[O_COMP];
    c_opts = (u8)((opt_val[O_CSIZE] ? CO_SIZE8 : 0) | (opt_val[O_CPROG] ? CO_PROG : 0) | (opt_val[O_CVAR] ? CO_VARIED : 0));
    c_lvl = opt_val[O_LEVEL];
    c_surf = (u8)(opt_val[O_SURF] > 3 && (c_type == CT_SEASON || c_type >= CT_GRAND) ? 0 : opt_val[O_SURF]);
    c_env = opt_val[O_ENV];
    c_cond = opt_val[O_COND];
    if (c_type >= CT_GRAND) { c_env = ENV_STADIUM; c_cond = 0; }   /* Grand Martinot, Martinworld Tour : toujours au stade, de jour */
    c_fmt = opt_val[O_FMT];
    c_pl[0] = opt_val[O_P1];
    c_dbl = opt_val[O_TYPE];
    c_hum2 = (u8)(c_dbl && opt_val[O_CPART]);
    c_pl2[0] = opt_val[O_P2];
    if (c_dbl && c_pl2[0] == c_pl[0]) c_pl2[0] = (u8)((c_pl[0] + 1) % NCHARS);
    c_seed = (u8)(rand8() ^ frame_cnt);
    COMP(comp_new, 0);
    if (c_type >= CT_GRAND) {
        /* Grand Martinot, Martinworld Tour : bande-annonce (banque 7) */
        screen_begin();
        bank_call(BANK_INTRO, teaser_build);
        scr_state = SS_TEASER;
        return;
    }
    comp_next();
}

/* ---------------------------------------------------------------------- */
/* Saisie d'un code                                                        */
static u8 code_pos;
static void code_screen(void)
{
    u8 i;
    screen_begin();
    deco("REPRISE", 0, 24);
    txt_at(7, 4, "CODE DE REPRISE");
    for (i = 0; i < PW_LEN; ++i) code_buf[i] = 'A';
    code_buf[PW_LEN] = 0;
    code_pos = 0;
    txt_at(9, 10, code_buf);
    txt_at(3, 19, "HAUT/BAS : CARACTERE");
    txt_at(3, 20, "GAUCHE/DROITE : POSITION");
    txt_at(3, 21, "START : VALIDER");
    txt_at(3, 22, "B : RETOUR");
    ppu_on();
    scr_state = SS_CODE;
}

static void code_tick(void)
{
    static const char al[] = PW_ALPHA;
    u8 i, idx = 0;
    for (i = 0; i < 32; ++i) if (al[i] == code_buf[code_pos]) idx = i;
    if (pad1_new & PAD_UP)    { idx = (u8)((idx + 1) & 31); sfx_menu_move(); }
    if (pad1_new & PAD_DOWN)  { idx = (u8)((idx + 31) & 31); sfx_menu_move(); }
    code_buf[code_pos] = al[idx];
    if ((pad1_new & PAD_RIGHT) && code_pos < PW_LEN - 1) ++code_pos;
    if ((pad1_new & PAD_LEFT) && code_pos > 0) --code_pos;
    vbuf_reset();
    vbuf_text(9, 10, code_buf, PW_LEN);
    if (pad1_new & PAD_B) { title_screen(); return; }
    if (pad1_new & PAD_START) {
        pw_s = code_buf; pw_p = &pw;
        if ((u8)far_call(BANK_COMP, (void *)pw_dec_far, 0)) {
            c_type = pw.type; c_stage = pw.stage; c_seed = pw.seed; c_pl[0] = pw.chr;
            c_surf = pw.surf; c_fmt = pw.fmt; c_cond = pw.cond; c_env = pw.env; c_lvl = pw.lvl;
            c_results = pw.results; c_opts = pw.opts; c_trophies = pw.trophies; c_gres = pw.gres; c_slam = 0;
            c_dbl = pw.doubles; c_pl2[0] = pw.partner; c_hum2 = pw.hum2;
            if (c_trophies) nude_unlocked = 1;     /* un trophée débloque le club nudiste */
            COMP(comp_build, 0);
            opt_val[O_P1] = c_pl[0];
            sfx_menu_ok();
            comp_next();
            return;
        }
        sfx_fault();
        vbuf_text(9, 13, "CODE INVALIDE", 13);
    }
    spr_begin();
    spr1((u8)(9 * 8 + code_pos * 8), 90, 3, 3);
    spr_end();
}

/* ---------------------------------------------------------------------- */
/* Résultats                                                               */
static void stat_row(u8 y, const char *label, u8 a, u8 b)
{
    char n[3];
    n[2] = 0;
    txt_at(2, y, label);
    num2(n, a); txt_at(20, y, n);
    num2(n, b); txt_at(26, y, n);
}

static u8 pct(u8 a, u8 b) { return b ? (u8)(((u16)a * 100) / b) : 0; }

/* Tableau des sets terminés : en-tête puis une ligne par équipe (rangées
   y à y+2), noms au format des panneaux (« ALEX », « MXAL » en double) */
static u8 sets_done(void)
{
    u8 n = 0;
    while (n < 5 && (sc_hist[n][0] | sc_hist[n][1])) ++n;
    return n;
}
static void set_table(u8 y)
{
    u8 i, t, n = sets_done();
    txt_at(8, y, "SETS");
    for (i = 0; i < n; ++i) { line[0] = (char)('1' + i); line[1] = 0; txt_at((u8)(15 + i * 3), y, line); }
    for (t = 0; t < 2; ++t) {
        if (cfg.doubles) team_label(cfg.chr[t], cfg.chr[t + 2], line);
        else team_label(cfg.chr[t], 255, line);
        txt_at(8, (u8)(y + 1 + t), line);
        for (i = 0; i < n; ++i) {
            line[0] = (char)('0' + sc_hist[i][t]); line[1] = 0;
            txt_at((u8)(15 + i * 3), (u8)(y + 1 + t), line);
        }
    }
}

/* Fin d'un set : récapitulatif, puis reprise du match (A, ou après 10 s) */
static u16 brk_t;
static void setbreak_screen(void)
{
    u8 n;
    char b[5];
    screen_begin();
    n = sets_done();
    b[0] = (char)('0' + n); b[1] = 0;
    txt_at(8, 2, "FIN DU");
    txt_at(15, 2, b);
    txt_at(16, 2, n == 1 ? "ER SET" : "E SET");
    /* vainqueur du set : celui qui a le plus de jeux dans le dernier set */
    n = (u8)(sc_hist[n - 1][1] > sc_hist[n - 1][0]);
    txt_at(8, 3, "POUR");
    if (cfg.doubles) team_label(cfg.chr[n], cfg.chr[n + 2], line); else team_label(cfg.chr[n], 255, line);
    txt_at(13, 3, line);
    set_table(6);
    team_label(cfg.chr[0], 255, b); txt_at(19, 11, b);
    team_label(cfg.chr[1], 255, b); txt_at(25, 11, b);
    stat_row(13, "POINTS GAGNES", st_points[0], st_points[1]);
    stat_row(14, "ACES", st_aces[0], st_aces[1]);
    stat_row(15, "DOUBLES FAUTES", st_dfaults[0], st_dfaults[1]);
    stat_row(16, "COUPS GAGNANTS", st_winners[0], st_winners[1]);
    stat_row(17, "FAUTES", st_errors[0], st_errors[1]);
    stat_row(18, "SERVICES CANON", st_canon[0], st_canon[1]);
    stat_row(19, "COUPS PARFAITS", st_perf[0], st_perf[1]);
    txt_at(2, 21, "AU SERVICE :");
    if (cfg.doubles) team_label(cfg.chr[sc_server], cfg.chr[sc_server + 2], line); else team_label(cfg.chr[sc_server], 255, line);
    txt_at(15, 21, line);
    txt_at(4, 25, "UN BOUTON : REPRENDRE");
    ppu_on();
    music_play(MUS_BREAK);              /* petit interlude, coupé à la reprise */
    brk_t = 0;
    scr_state = SS_SETBREAK;
}

static void results_screen(void)
{
    u8 w = sc_winner;
    char b[5];
    screen_begin();
    people_tiles((u8)(cfg.env == ENV_NUDE));
    pal_set_player(0, cfg.chr[0]);
    pal_set_player(1, cfg.chr[1]);
    deco("RESULTAT", 0, 24);
    txt_off = 0x40;                              /* en or */
    txt_at(8, 5, "VICTOIRE DE");
    txt_at(20, 5, char_name[cfg.chr[w]]);
    txt_off = 0;
    set_table(2);
    /* colonnes de 4 lettres (MARGAUX : « MARG ») */
    team_label(cfg.chr[0], 255, b); txt_at(19, 9, b);
    team_label(cfg.chr[1], 255, b); txt_at(25, 9, b);
    stat_row(11, "POINTS GAGNES", st_points[0], st_points[1]);
    stat_row(12, "ACES", st_aces[0], st_aces[1]);
    stat_row(13, "DOUBLES FAUTES", st_dfaults[0], st_dfaults[1]);
    stat_row(14, "COUPS GAGNANTS", st_winners[0], st_winners[1]);
    stat_row(15, "FAUTES", st_errors[0], st_errors[1]);
    stat_row(16, "1ERES BALLES %", pct(st_first_in[0], st_first_tot[0]), pct(st_first_in[1], st_first_tot[1]));
    num2(b, st_longest); b[2] = 0;
    txt_at(2, 17, "ECHANGE LE PLUS LONG"); txt_at(26, 17, b);
    stat_row(18, "SERVICES CANON", st_canon[0], st_canon[1]);
    stat_row(19, "COUPS PARFAITS", st_perf[0], st_perf[1]);
    if (last_mode == MODE_COMP) txt_at(4, 25, "A : CONTINUER");
    else txt_at(4, 25, "A : REVANCHE   B : MENU");
    ppu_on();
    scr_state = SS_RESULTS;
}

/* vainqueur(s) en joie, perdant(s) déçu(s) (CDC 7.5) */
static void results_sprites(void)
{
    u8 w = sc_winner, joy = (u8)((frame_cnt >> 4) & 1 ? POSE_JOY : POSE_SERVE);
    spr_begin();
    spr_person(60, 64, cfg.chr[w], 1, joy, 0, w);
    spr_person(196, 64, cfg.chr[w ^ 1], 1, POSE_SAD, 0, (u8)(w ^ 1));
    if (cfg.doubles) {
        spr_person(80, 64, cfg.chr[w + 2], 1, joy, 1, w);
        spr_person(216, 64, cfg.chr[(w ^ 1) + 2], 1, POSE_SAD, 1, (u8)(w ^ 1));
    }
    spr_end();
}

static void training_results(void)
{
    char b[4];
    screen_begin();
    deco("ENTRAINEMENT", 0, 24);
    txt_at(8, 4, "FIN DE L'EXERCICE");
    txt_at(6, 7, ex_name[cfg.ex]);
    txt_at(6, 10, cfg.ex == EX_STREAK ? "MEILLEURE SERIE :" : "POINTS :");
    num2(b, tr_score); b[2] = 0; txt_at(24, 10, b);
    if (cfg.ex != EX_STREAK) { txt_at(6, 12, "BALLES :"); num2(b, tr_total); txt_at(24, 12, b); }
    SV(sv_after_train);
    txt_at(6, 14, "RECORD :");
    num2(b, sv.best_train[cfg.ex]); txt_at(24, 14, b);
    txt_at(3, 25, "A : RECOMMENCER  B : MENU");
    ppu_on();
    scr_state = SS_TRAIN_RES;
}

/* ---------------------------------------------------------------------- */
/* Palmarès (CDC 12.1-12.2)                                                */
u8 pal_page;                 /* (lu par menuart.c) */
static u8 pal_confirm;
static void palmares_screen(void)
{
    SV(sv_check);
    screen_begin();
    deco("PALMARES", 0, 24);
    bank_call(BANK_PODIUM, palmares_build);
    ppu_on();
    scr_state = SS_PALMARES;
}

/* ---------------------------------------------------------------------- */
static void after_match(void)
{
    if (match_quit == 2) { title_screen(); return; }        /* démonstration */
    if (match_quit) {
        music_play(MUS_NONE);
        if (last_mode == MODE_COMP) comp_screen(); else menu_screen();
        return;
    }
    if (cfg.mode == GM_TRAIN) { training_results(); return; }
    SV(sv_after_match);
    if (last_mode == MODE_COMP) COMP(comp_record, sc_winner == 0);
    results_screen();
}

void screens_enter(u8 scr)
{
    switch (scr) {
    case SCR_MENU: menu_screen(); break;
    case SCR_AFTER_MATCH: after_match(); break;
    case SCR_SETBREAK: setbreak_screen(); break;
    default: title_screen(); break;
    }
}

static void go_match(void) { go_req = GO_MATCH; }

void screens_tick(void)
{
    u8 o, v, i;
    switch (scr_state) {
    case SS_TITLE:
        if (pad1_new & PAD_UP)   { title_cur = (u8)((title_cur + 3) & 3); sfx_menu_move(); idle = 0; }
        if (pad1_new & PAD_DOWN) { title_cur = (u8)((title_cur + 1) & 3); sfx_menu_move(); idle = 0; }
        if (pad1_new & PAD_SELECT) { sfx_menu_ok(); code_screen(); break; }
        if (pad1_new & (PAD_START | PAD_A)) {
            sfx_menu_ok();
            if ((pad1_new & PAD_START) || title_cur == 0) menu_screen();
            else if (title_cur == 1) go_req = GO_MANUAL;
            else if (title_cur == 2) { pal_page = 0; pal_confirm = 0; palmares_screen(); }
            else code_screen();
            break;
        }
        spr_begin();
        spr1(64, (u8)((16 + title_cur * 2) * 8 - 1 - ((frame_cnt >> 3) & 1)), (u8)(ball_base + 8), 3);   /* balle qui sautille */
        spr_end();
        {   /* reflet qui passe sur le logo et la balle ; scintillement de l'édition */
            static const u8 glint[8] = { 0x28, 0x38, 0x30, 0x30, 0x38, 0x28, 0x28, 0x28 };
            u8 t = (u8)(frame_cnt & 127);
            if ((t & 3) == 0) {
                pal_buf[10] = t < 32 ? glint[t >> 2] : 0x28;
                pal_buf[15] = (u8)((frame_cnt & 32) ? 0x38 : 0x28);
                pal_dirty = 1;
            }
        }
        if ((frame_cnt & 3) == 0 && ++idle > 300) {    /* démo automatique après 20 s */
            idle = 0;
            setup_cfg(MODE_DEMO);
            cfg.doubles = demo_dbl;    /* alterne simple et double d'une démo à l'autre */
            demo_dbl ^= 1;
            go_match();
        }
        break;
    case SS_MENU:
        if (pad1_new & PAD_UP)   { cursor = (u8)(cursor ? cursor - 1 : nvis - 1); sfx_menu_move(); }
        if (pad1_new & PAD_DOWN) { cursor = (u8)(cursor + 1 == nvis ? 0 : cursor + 1); sfx_menu_move(); }
        o = vis[cursor];
        if (o != O_START && (pad1_new & (PAD_LEFT | PAD_RIGHT))) {
            v = opt_val[o];
            i = opt_max[o];
            if (pad1_new & PAD_RIGHT) v = (u8)(v + 1 == i ? 0 : v + 1);
            else v = (u8)(v ? v - 1 : i - 1);
            /* le club nudiste n'apparaît qu'une fois débloqué */
            /* Margaux ne va jamais au club nudiste : ni ce lieu avec elle, ni
               elle dans ce lieu */
            if (o == O_ENV && v == ENV_NUDE && (!nude_unlocked || opt_val[O_P1] == 16 || opt_val[O_P2] == 16))
                v = (u8)((pad1_new & PAD_RIGHT) ? ENV_NUDE + 1 : ENV_NUDE - 1);
            if ((o == O_P1 || o == O_P2) && v == 16 && opt_val[O_ENV] == ENV_NUDE)
                v = (u8)((pad1_new & PAD_RIGHT) ? 0 : 15);
            opt_val[o] = v;
            sfx_menu_move();
            if (o == O_MODE || o == O_COMP || o == O_EX || (o == O_TYPE && opt_val[O_MODE] == MODE_COMP)) {
                vbuf_reset();
                menu_soft();
                menu_style_line(0);
                break;
            }
            vbuf_reset();
            menu_draw_row(cursor, 0);
            if (o == O_ENV) menu_music();
            if (o == O_SOUND) { snd_set(v); if (!v) { music_cur = 0xFF; menu_music(); } }
            if (o == O_ENV || o == O_COND) {
                for (i = 0; i < nvis; ++i) if (vis[i] == O_COND) menu_draw_row(i, 0);
            }
            menu_style_line(0);
            if (o == O_P1) pal_set_player(0, v);
            if (o == O_P2) pal_set_player(1, v);
        }
        if (pad1_new & PAD_B) { title_screen(); break; }
        if ((pad1_new & PAD_START) || ((pad1_new & PAD_A) && o == O_START)) {
            sfx_menu_ok();
            (void)far_call(BANK_COMP, (void *)so_save, 0);   /* mémorise les réglages */
            v = opt_val[O_MODE];
            if (v == MODE_COMP) { start_comp(); break; }
            setup_cfg(v);
            if (v == MODE_TRAIN || v == MODE_DEMO) go_match();
            else present_screen();
            break;
        }
        menu_soft_tick();
        menu_info_tick();
        menu_sprites();
        break;
    case SS_TEASER:
        bank_call(BANK_INTRO, teaser_tick);
        if (teaser_done) { ppu_off(); chr_reload(); comp_next(); }   /* tuiles d'origine, puis l'écran de la compétition */
        break;
    case SS_GMI:
        bank_call(BANK_GMI, gm_intro_tick);
        if (pad1_new & (PAD_A | PAD_START)) { sfx_menu_ok(); gm_ev = 0xFF; ppu_off(); chr_reload(); go_match(); }
        break;
    case SS_PRESENT:
        if (pad1_new & (PAD_A | PAD_START)) {
            if (last_mode == MODE_COMP && gm_ev != 0xFF) {   /* l'image du tournoi, puis le match */
                screen_begin(); bank_call(BANK_GMI, gm_intro_build); ppu_on(); scr_state = SS_GMI; break;
            }
            go_match(); break;
        }
        if (pad1_new & PAD_B) { if (last_mode == MODE_COMP) comp_screen(); else menu_screen(); break; }
        present_sprites();
        break;
    case SS_COMP:
        if (c_type == CT_LEAGUE || c_type == CT_SEASON || c_type >= CT_GRAND) {
            spr_begin();
            for (i = 0; i < 3; ++i) spr1(c_type == CT_LEAGUE ? 12 : 8, (u8)((14 + i) * 8 - 1), (u8)(ball_base + 8), i);
            spr_end();
        }
        if (pad1_new & (PAD_A | PAD_START)) { comp_cfg(); present_screen(); break; }
        if (pad1_new & PAD_B) menu_screen();
        break;
    case SS_SETBREAK:
        /* n'importe quel bouton (manette 1 ou 2) ferme l'écran aussitôt,
           sans fondu ; sinon reprise automatique au bout de 10 s */
        if (((pad1_new | pad2_new) & (PAD_A | PAD_B | PAD_START | PAD_SELECT)) || ++brk_t > 600) {
            music_play(MUS_NONE); sfx_menu_ok(); fade_skip = 1; go_req = GO_RESUME;
        }
        break;
    case SS_COMP_END:
        if ((pad1_new & (PAD_A | PAD_START)) && !pod_busy) {   /* (cérémonie en cours : A la fait avancer) */
            if (c_rank == 1) { ppu_off(); chr_reload(); }   /* tuiles du podium : motifs d'origine */
            /* saison gagnée : générique de fin, puis l'écran titre */
            if (c_rank == 1 && (c_type == CT_SEASON || c_type >= CT_GRAND)) { go_req = GO_CREDITS; break; }
            title_screen();
            break;
        }
        comp_end_sprites();
        break;
    case SS_RESULTS:
        if (pad1_new & PAD_A) {
            if (last_mode == MODE_COMP) { if (c_over) comp_end_screen(); else comp_next(); }
            else { setup_cfg(last_mode); go_match(); }
            break;
        }
        if ((pad1_new & PAD_B) && last_mode != MODE_COMP) { menu_screen(); break; }
        results_sprites();
        break;
    case SS_TRAIN_RES:
        if (pad1_new & PAD_A) { setup_cfg(MODE_TRAIN); go_match(); }
        if (pad1_new & PAD_B) menu_screen();
        break;
    case SS_CODE:
        code_tick();
        break;
    case SS_PALMARES:
        if (pad1_new & PAD_RIGHT) { pal_page = (u8)(pal_page == 2 ? 0 : pal_page + 1); fade_skip = 1; palmares_screen(); break; }
        if (pad1_new & PAD_LEFT) { pal_page = (u8)(pal_page == 0 ? 2 : pal_page - 1); fade_skip = 1; palmares_screen(); break; }
        if (pad1_new & PAD_SELECT) {
            if (++pal_confirm == 2) { SV(sv_clear); pal_confirm = 0; fade_skip = 1; palmares_screen(); sfx_fault(); }
            else { vbuf_reset(); vbuf_text(2, 22, "SELECT ENCORE : EFFACER", 24); }
        }
        if (pad1_new & PAD_B) title_screen();
        break;
    }
}
