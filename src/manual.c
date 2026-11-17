/* Royal Martinot — mode d'emploi intégré (banque BANK_MANUAL). */
#include "app.h"
#include "gfx.h"
#include "audio.h"
#include "assets.h"

#pragma static-locals(on)

#define NPAGES 13
#define NLINES 18
/* 17 caractères au plus par ligne (colonnes 2 à 18) : à droite, le
   mini-court des illustrations (colonnes 20 à 29, 2.76) */

static const char * const page_title[NPAGES] = {
    "COMMANDES",
    "LES COUPS",
    "VOLEE ET SMASH",
    "LE SERVICE",
    "FAUTES ET LET",
    "LE SCORE",
    "LE MATCH",
    "LE DOUBLE",
    "MODES DE JEU",
    "CONDITIONS",
    "COMPETITIONS",
    "CODES, PALMARES",
    "CONSEILS" };

static const char * const page_text[NPAGES][NLINES] = {
  { "CROIX : BOUGER", "", "A : COUP LIFTE", "B : COUP COUPE", "", "MAINTENIR LE", "BOUTON ARME LE", "COUP : PLUS FORT", "AU LACHER.", "", "LA FRAPPE PART", "SEULE QUAND LA", "BALLE ARRIVE A", "PORTEE.", "", "START : PAUSE", "RALENTI, QUITTER.", "" },
  { "A + HAUT :", "  FRAPPE A PLAT", "A + BAS :", "  CROISE COURT", "B + HAUT : LOB", "B + BAS : AMORTIE", "", "HAUT = VERS", "L'ADVERSAIRE.", "", "GAUCHE/DROITE A", "LA FRAPPE : LE", "LONG DE LA LIGNE.", "SANS DIRECTION :", "COUP CROISE.", "", "", "" },
  { "AVANT LE REBOND :", "VOLEE.", "BALLE HAUTE :", "SMASH.", "TRES HAUTE, PRES", "DU FILET : SMASH", "SAUTE.", "", "COUP PARFAIT :", "BALLE A COTE DU", "CORPS, BOUTON", "LACHE JUSTE AVANT", "L'IMPACT.", "", "TROP FORT VERS", "LES LIGNES :", "RISQUE DE FAUTE.", "" },
  { "GAUCHE/DROITE :", "SE PLACER.", "A OU B : LANCER.", "", "AU SOMMET :", "A : PUISSANT", "B : COUPE, PLUS", "SUR.", "PILE AU SOMMET :", "SERVICE CANON !", "", "GAUCHE/DROITE A", "L'IMPACT : VISER", "L'EXTERIEUR OU", "LE T.", "", "SANS FRAPPE : LA", "BALLE RETOMBE." },
  { "2 FAUTES DE", "SERVICE : DOUBLE", "FAUTE, LE POINT", "EST PERDU.", "", "SERVICE TOUCHANT", "LE FILET PUIS LE", "CARRE : LET, A", "REMETTRE.", "", "UNE BALLE FAUTE", "PEUT SE RENVOYER", "SI LE COUP EST", "ARME : LE POINT", "RESTE JOUE.", "", "", "" },
  { "0 15 30 40 JEU", "40-40 : EGALITE,", "PUIS AVANTAGE :", "2 POINTS D'ECART.", "", "SET : 6 JEUX,", "2 D'ECART.", "A 6-6 : JEU", "DECISIF EN 7.", "", "JEU BLANC : JEU", "GAGNE SANS", "PERDRE UN POINT.", "", "FORMATS : 1, 3", "OU 5 SETS, OU", "RAPIDE (4 JEUX).", "" },
  { "CHANGEMENT DE", "COTE AUX JEUX", "IMPAIRS : PAUSE", "SUR LES CHAISES", "ET STATISTIQUES", "(A : PAGE", "SUIVANTE).", "", "RALENTIS DES", "BEAUX POINTS :", "NON, OUI OU", "SIMPLE (MENU OU", "PAUSE).", "", "COIN DU HAUT :", "* = SERVEUR", "@ = SET GAGNE", "" },
  { "4 JOUEURS. LES", "COULOIRS COMPTENT", "SAUF AU SERVICE.", "", "LES PARTENAIRES", "SERVENT A TOUR", "DE ROLE.", "", "PARTENAIRE : CPU", "OU MANETTE 2.", "", "LE PARTENAIRE CPU", "COUVRE SA MOITIE", "DU COURT.", "", "EN COMPETITION", "AUSSI.", "" },
  { "1 JOUEUR / CPU", "2 JOUEURS", "EN EQUIPE", "COMPETITIONS", "ENTRAINEMENT :", "9 EXERCICES, DONT", "UN TUTORIEL.", "DEMO : CPU/CPU.", "", "LIEUX : STADE,", "CLUB, SALLE, PARC", "CHATEAU, COURT", "MARGAUX, BONUS.", "", "SURFACES : TERRE,", "GAZON, DUR,", "MOQUETTE, PLAGE,", "GLACE." },
  { "JOUR, NUIT SOUS", "LES PROJECTEURS.", "", "PLUIE : COURT", "GLISSANT.", "", "CREPUSCULE : LE", "SOLEIL SE COUCHE", "AU FIL DU MATCH,", "UN PLAN DU", "SOLEIL A CHAQUE", "CHANGEMENT DE", "LUMIERE.", "", "VITESSE DU JEU,", "FATIGUE ET SON :", "DANS LES MENUS.", "" },
  { "TOURNOI, POULES,", "CHAMPIONNAT,", "SAISON,", "PERSONNALISE,", "GRAND MARTINOT", "(4 TOURNOIS),", "MARTINWORLD (7).", "", "EN DOUBLE AUSSI.", "", "UN TITRE OUVRE", "UN LIEU BONUS.", "", "MATCH EN COURS :", "IL SE REPREND", "AU RETOUR.", "", "" },
  { "AVANT CHAQUE", "MATCH, UN CODE", "DE 13 SIGNES :", "NOTEZ-LE.", "TITRE > CODE DE", "REPRISE.", "HAUT/BAS : SIGNE", "GAUCHE/DROITE :", "POSITION.", "START : VALIDER.", "", "LE PALMARES GARDE", "STATISTIQUES,", "TITRES ET", "RECORDS.", "", "", "" },
  { "PLACEZ-VOUS A", "COTE DE LA BALLE,", "PAS DESSUS.", "ARMEZ TOT SUR LES", "BALLES LENTES.", "", "VOLLEYEUR : LOB.", "JOUEUR AU FOND :", "AMORTIE.", "", "GAZON, GLACE :", "BALLE RAPIDE,", "MONTEZ AU FILET.", "TERRE, PLAGE :", "JEU DE FOND.", "", "L'IA S'ADAPTE A", "VOS HABITUDES." },
};

u8 page;   /* page affichée (lue par les tests) */

/* ---------------------------------------------------------------------- */
/* Illustrations (2.72, refaites en 2.76) : à droite de chaque page, un     */
/* mini-court vu comme en match (colonnes 20 à 29) où se joue la scène dont */
/* parle la page : le joueur du bas (de dos), l'adversaire du haut (de      */
/* face), une balle qui vole en arc avec son ombre (spr_ball, comme en      */
/* match). Deux pistes par scène, qui bouclent ensemble : les gestes        */
/* (poses, positions) et la trajectoire de la balle (points de passage,     */
/* hauteur, bosse de l'arc ; durée 0 : la balle saute au point).            */
/* ---------------------------------------------------------------------- */
#define CX   200                 /* milieu du mini-court */
#define YB   184                 /* pieds du joueur du bas */
#define YT    58                 /* pieds de l'adversaire */
typedef struct { u8 t, pa, pb, ax, ox; } Pose;          /* durée, poses (bas, haut), positions x */
typedef struct { u8 t, x, y, z, h; } Fly;               /* durée, point d'arrivée (x, y au sol, hauteur z), bosse */

/* échange de fond : coup droit du bas, coup droit (de face) du haut */
static const Pose r_pose[] = {
    { 8, POSE_RUN1, POSE_FOLLOW, 202, CX }, { 8, POSE_RUN3, POSE_IDLE, 206, CX }, { 18, POSE_PREP, POSE_IDLE, 206, CX },
    { 6, POSE_IMPACT, POSE_IDLE, 206, CX }, { 14, POSE_FOLLOW, POSE_IDLE, 206, CX }, { 8, POSE_RUN2, POSE_IDLE, 200, CX },
    { 6, POSE_IDLE, POSE_IDLE, 198, CX }, { 8, POSE_IDLE, POSE_PREP, 198, CX }, { 6, POSE_IDLE, POSE_IMPACT, 198, CX } };
static const Fly r_fly[] = { { 28, 214, 150, 0, 20 }, { 12, 216, YB, 12, 6 }, { 30, 190, 80, 0, 22 }, { 12, 190, YT, 14, 6 } };

/* les coups : à plat (tendu), lob (très haut), amortie (court) */
static const Pose c_pose[] = {
    { 16, POSE_PREP, POSE_IDLE, 206, CX }, { 6, POSE_IMPACT, POSE_IDLE, 206, CX }, { 26, POSE_FOLLOW, POSE_IDLE, 206, CX },
    { 16, POSE_PREP, POSE_IDLE, 206, CX }, { 6, POSE_SRVHIT, POSE_IDLE, 206, CX }, { 50, POSE_IDLE, POSE_RUN1, 206, 196 },
    { 16, POSE_PREP, POSE_IDLE, 206, CX }, { 6, POSE_DROP, POSE_IDLE, 206, CX }, { 40, POSE_IDLE, POSE_RUN2, 206, 204 } };
static const Fly c_fly[] = {
    { 0, 216, YB, 12, 0 }, { 22, 196, 60, 0, 8 }, { 26, 192, 30, 0, 6 },              /* à plat : tendu, au fond */
    { 0, 216, YB, 12, 0 }, { 22, 0, 0, 0, 0 }, { 44, 204, 54, 0, 80 }, { 12, 202, 34, 0, 10 },   /* lob */
    { 0, 216, YB, 12, 0 }, { 22, 0, 0, 0, 0 }, { 26, 206, 100, 0, 16 }, { 14, 206, 92, 0, 4 } };  /* amortie : juste derrière le filet */

/* volée au filet, puis smash sur un lob */
static const Pose v_pose[] = {
    { 20, POSE_IDLE, POSE_PREP, 204, CX }, { 6, POSE_IDLE, POSE_IMPACT, 204, CX }, { 10, POSE_VOLLEY, POSE_FOLLOW, 204, CX },
    { 12, POSE_VOLLEY, POSE_IDLE, 204, CX }, { 22, POSE_IDLE, POSE_PREP, 204, CX }, { 6, POSE_IDLE, POSE_SRVHIT, 204, CX },
    { 24, POSE_SERVE, POSE_IDLE, 204, CX }, { 10, POSE_SRVHIT, POSE_IDLE, 204, CX }, { 20, POSE_FOLLOW, POSE_IDLE, 204, CX } };
static const Fly v_fly[] = {
    { 0, 190, YT, 14, 0 }, { 26, 214, 134, 16, 10 }, { 16, 186, 52, 0, 4 },             /* volée */
    { 12, 186, 40, 0, 2 }, { 0, 190, YT, 14, 0 }, { 28, 0, 0, 0, 0 }, { 30, 206, 134, 44, 50 },   /* lob */
    { 10, 194, 70, 0, 2 }, { 20, 190, 36, 0, 4 } };                                   /* smash */
#define YV 140                   /* joueur au filet */

/* service : rebonds, lancer, frappe ; retour, balle suivante */
static const Pose s_pose[] = {
    { 14, POSE_DRIBBLE, POSE_IDLE, 186, 212 }, { 10, POSE_IDLE, POSE_IDLE, 186, 212 }, { 14, POSE_DRIBBLE, POSE_IDLE, 186, 212 },
    { 24, POSE_SERVE, POSE_IDLE, 186, 212 }, { 8, POSE_SRVHIT, POSE_IDLE, 186, 212 }, { 22, POSE_FOLLOW, POSE_PREP, 186, 212 },
    { 6, POSE_FOLLOW, POSE_IMPACT, 186, 212 }, { 30, POSE_IDLE, POSE_FOLLOW, 186, 212 } };
static const Fly s_fly[] = {
    { 0, 190, YB, 10, 0 }, { 7, 190, YB, 0, 0 }, { 7, 190, YB, 10, 0 }, { 10, 190, YB, 10, 0 },
    { 7, 190, YB, 0, 0 }, { 7, 190, YB, 10, 0 }, { 24, 190, YB, 40, 30 }, { 8, 192, YB, 30, 0 },
    { 22, 206, 74, 0, 8 }, { 6, 204, YT, 12, 2 }, { 30, 200, 160, 0, 18 } };

/* faute de service de l'adversaire, renvoyée quand même par le joueur armé */
static const Pose f_pose[] = {
    { 24, POSE_PREP, POSE_SERVE, 196, 186 }, { 8, POSE_PREP, POSE_SRVHIT, 196, 186 }, { 22, POSE_PREP, POSE_FOLLOW, 206, 186 },
    { 6, POSE_IMPACT, POSE_IDLE, 206, 186 }, { 26, POSE_FOLLOW, POSE_IDLE, 206, 186 }, { 14, POSE_IDLE, POSE_IDLE, 200, 186 } };
static const Fly f_fly[] = {
    { 0, 180, YT, 14, 0 }, { 24, 180, YT, 28, 8 }, { 8, 182, YT, 22, 0 },
    { 18, 214, 162, 0, 8 }, { 10, 216, YB, 10, 4 }, { 30, 190, 84, 0, 18 }, { 14, 186, 54, 0, 6 } };

/* point gagné : poing serré, joie */
static const Pose w_pose[] = { { 30, POSE_IDLE, POSE_IDLE, CX, CX }, { 30, POSE_FIST, POSE_TIRED, CX, CX }, { 40, POSE_JOY, POSE_TIRED, CX, CX },
                               { 30, POSE_IDLE, POSE_IDLE, CX, CX } };
/* changement de côté : assis, puis retour sur le court */
static const Pose m_pose[] = { { 60, POSE_SIT, POSE_SIT, CX, CX }, { 8, POSE_RUN1, POSE_RUN1, CX, CX }, { 8, POSE_RUN2, POSE_RUN2, CX, CX },
                               { 30, POSE_IDLE, POSE_IDLE, CX, CX }, { 40, POSE_TIRED, POSE_IDLE, CX, CX } };
/* conseils : plongeon sur une balle lointaine */
static const Pose d_pose[] = { { 22, POSE_IDLE, POSE_PREP, 192, CX }, { 6, POSE_IDLE, POSE_IMPACT, 192, CX }, { 14, POSE_RUN1, POSE_FOLLOW, 196, CX },
                               { 10, POSE_REACH, POSE_IDLE, 200, CX }, { 26, POSE_DIVE, POSE_IDLE, 206, CX }, { 30, POSE_KNEEL, POSE_IDLE, 206, CX },
                               { 20, POSE_IDLE, POSE_IDLE, 196, CX } };
static const Fly d_fly[] = { { 0, 190, YT, 14, 0 }, { 28, 0, 0, 0, 0 }, { 26, 226, 160, 0, 18 }, { 12, 230, YB, 8, 4 },
                             { 16, 206, 90, 0, 22 }, { 18, 204, 60, 0, 6 }, { 30, 204, 60, 0, 0 } };
static const Fly no_fly[] = { { 1, 0, 0, 0, 0 } };

typedef struct { const Pose *p; const Fly *f; u8 np, nf, ya, flags; } Scene;
#define SC(p, f, ya, fl) { p, f, sizeof p / sizeof(Pose), sizeof f / sizeof(Fly), ya, fl }
#define F_DBL 1                  /* double : deux partenaires de plus */
static const Scene scenes[] = {
    SC(r_pose, r_fly, YB, 0), SC(c_pose, c_fly, YB, 0), SC(v_pose, v_fly, YV, 0), SC(s_pose, s_fly, YB, 0),
    SC(f_pose, f_fly, YB, 0), SC(w_pose, no_fly, YB, 0), SC(m_pose, no_fly, YB, 0), SC(v_pose, v_fly, YV, F_DBL),
    SC(r_pose, r_fly, YB, 0), SC(r_pose, r_fly, YB, 0), SC(w_pose, no_fly, YB, 0), SC(w_pose, no_fly, YB, 0),
    SC(d_pose, d_fly, YB, 0) };

extern u8 bspin;
#pragma static-locals(off)          /* (RAM interne pleine : variables sur la pile) */
#pragma bss-name(push, "WBSS")
static u8 pi, pt, fi, ft;
static u8 fx0, fy0, fz0;          /* départ du vol en cours */
static u8 ax0, ox0;               /* positions au début du geste en cours */
#pragma bss-name(pop)
static void fly_start(const Fly *f)
{
    if (f->x) { fx0 = f->x; fy0 = f->y; fz0 = f->z; }
}
static void illus_reset(void)
{
    const Scene *sc = &scenes[page];
    pi = pt = fi = ft = 0;
    fx0 = sc->f[sc->nf - 1].x; fy0 = sc->f[sc->nf - 1].y; fz0 = sc->f[sc->nf - 1].z;
    ax0 = sc->p[sc->np - 1].ax; ox0 = sc->p[sc->np - 1].ox;
    bspin = 1;
}
static s16 lerp(u8 a, u8 b, u8 k, u8 t) { return (s16)a + (s16)(((s16)b - (s16)a) * (s16)k / (s16)t); }
static void illus_tick(void)
{
    const Scene *sc = &scenes[page];
    const Pose *p = sc->p + pi;
    const Fly *f = sc->f + fi;
    s16 x, y, z, ax, ox;
    /* gestes */
    ax = lerp(ax0, p->ax, pt, p->t); ox = lerp(ox0, p->ox, pt, p->t);
    spr_begin();
    /* balle (en tête : priorité d'affichage), avec son ombre */
    if (f->x && f->t) {
        x = lerp(fx0, f->x, ft, f->t); y = lerp(fy0, f->y, ft, f->t);
        z = lerp(fz0, f->z, ft, f->t) + (s16)(((u16)f->h * 4 * ft * (u16)(f->t - ft)) / ((u16)f->t * f->t));
        if (sc->ya == YV && fi < 2) y = lerp(fy0, f->y, ft, f->t);
        spr_ball((u8)x, y - z - 2, (u8)y, (u8)z);
    } else if (!f->x && fx0 && sc->f != no_fly) {
        spr_ball(fx0, (s16)fy0 - fz0 - 2, fy0, fz0);    /* balle tenue / posée */
    }
    spr_person(ax, sc->ya, 0, 0, p->pa, 0, 0);                   /* le joueur, de dos */
    spr_person(ox, YT, 1, 1, p->pb, 0, 1);                       /* l'adversaire, de face */
    if (sc->flags & F_DBL) {
        spr_person(176, YB, 2, 0, POSE_IDLE, 0, 0);
        spr_person(216, 92, 3, 1, POSE_IDLE, 0, 1);
    }
    spr_end();
    if (++pt >= p->t) { pt = 0; ax0 = p->ax; ox0 = p->ox; if (++pi >= sc->np) pi = 0; }
    if (++ft >= f->t) {
        ft = 0;
        if (f->x) { fx0 = f->x; fy0 = f->y; fz0 = f->z; }
        if (++fi >= sc->nf) fi = 0;
        f = sc->f + fi;
        while (f->t == 0) { fly_start(f); if (++fi >= sc->nf) fi = 0; f = sc->f + fi; }   /* (saut : la balle est replacée) */
    }
}

/* tuiles du mini-court ($A0-$A8, prises à la zone des décors : rétablies
   avant le match, bg_dirty) ; couleurs : 1 abords, 2 court, 3 lignes */
extern u8 bg_dirty;
#define T_OUT 0xA0
static const u8 mini_chr[9][16] = {
    { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 0,0,0,0,0,0,0,0 },                        /* abords */
    { 0,0,0,0,0,0,0,0, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF },                        /* court */
    { 0xFF,0,0,0,0,0,0,0, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF },                     /* ligne en haut */
    { 0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF }, /* ligne à gauche */
    { 0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF }, /* ligne à droite */
    { 0xFF,0x80,0x80,0x80,0x80,0x80,0x80,0x80, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF }, /* coin, croisement */
    { 0xFF,0x01,0x01,0x01,0x01,0x01,0x01,0x01, 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF }, /* coin droit */
    { 0xFF,0xAA,0x55,0xAA,0x55,0xAA,0,0, 0xFF,0x55,0xAA,0x55,0xAA,0x55,0xFF,0xFF },      /* filet */
    { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, 0xFF,0,0,0,0,0,0,0 } };                   /* ligne de fond du bas */
static void mini_court(void)
{
    u8 r, c, t;
    vram_adr((u16)T_OUT << 4);
    for (r = 0; r < 9; ++r) for (c = 0; c < 16; ++c) vram_put(mini_chr[r][c]);
    if (!bg_dirty) bg_dirty = 2;
    for (r = 4; r < 24; ++r) {
        vram_adr((u16)(0x2000 + r * 32 + 20));
        for (c = 20; c < 30; ++c) {
            t = 0;
            if (r == 14) t = 7;                                   /* filet */
            else if (r == 22) t = (c >= 21 && c <= 28) ? 8 : 0;   /* ligne de fond du bas */
            else if (r >= 6 && r <= 21 && c >= 21 && c <= 28) {
                if (r == 6 || r == 10 || r == 18) t = c == 21 ? 5 : c == 28 ? 6 : (c == 25 && r != 6) ? 5 : 2;
                else t = c == 21 ? 3 : c == 28 ? 4 : (c == 25 && r > 10 && r < 18) ? 3 : 1;
            }
            vram_put((u8)(T_OUT + t));
        }
    }
    /* attributs : palette 3 (court) pour les colonnes 20-29, rangées 4-23 */
    for (r = 1; r < 6; ++r) {
        vram_adr((u16)(0x23C0 + r * 8 + 5));
        vram_put(0xFF); vram_put(0xFF); vram_put(0x33);
    }
    pal_buf[13] = 0x07; pal_buf[14] = 0x17; pal_buf[15] = 0x30;  /* terre battue, lignes blanches */
    pal_buf[16 + 9] = 0x37; pal_buf[16 + 10] = 0x0F; pal_buf[16 + 11] = 0x30;   /* ombre de la balle */
    pal_buf[16 + 13] = 0x28; pal_buf[16 + 14] = 0x21; pal_buf[16 + 15] = 0x30;  /* balle */
    pal_dirty = 1;
}

static void draw_page(void)
{
    u8 i;
    char n[6];
    ppu_off();
    vbuf_reset();
    clear_screen();
    pal_set_menu();
    spr_begin(); spr_end();
    txt_at(2, 1, "MODE D'EMPLOI");
    i = (u8)(page + 1);
    n[0] = (char)(i >= 10 ? '1' : ' '); n[1] = (char)('0' + i % 10); n[2] = '/'; n[3] = (char)('0' + NPAGES / 10); n[4] = (char)('0' + NPAGES % 10); n[5] = 0;
    txt_at(25, 1, n);
    txt_at(2, 3, page_title[page]);
    for (i = 0; i < NLINES; ++i) txt_at(2, (u8)(5 + i), page_text[page][i]);
    txt_at(2, 25, "GAUCHE/DROITE : PAGE");
    txt_at(2, 26, "B : RETOUR");
    people_tiles(0);                    /* poses des joueurs (illustrations, 2.72) */
    pal_set_player(0, 0); pal_set_player(1, 1);
    mini_court();
    illus_reset();
    ppu_on();
}

void manual_begin(void)
{
    page = 0;
    draw_page();
}

void manual_tick(void)
{
    if (pad1_new & PAD_RIGHT) { page = (u8)(page + 1 == NPAGES ? 0 : page + 1); sfx_menu_move(); fade_skip = 1; draw_page(); }
    else if (pad1_new & PAD_LEFT) { page = (u8)(page ? page - 1 : NPAGES - 1); sfx_menu_move(); fade_skip = 1; draw_page(); }
    else if (pad1_new & (PAD_B | PAD_START)) { sfx_menu_ok(); go_req = GO_SCREEN; go_arg = SCR_TITLE; }
    else illus_tick();
}
