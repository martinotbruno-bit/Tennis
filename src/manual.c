/* Royal Martinot — mode d'emploi intégré (banque BANK_MANUAL). */
#include "app.h"
#include "gfx.h"
#include "audio.h"
#include "assets.h"

#pragma static-locals(on)

#define NPAGES 8
#define NLINES 18
/* 28 caractères au plus par ligne (colonnes 2 à 29 : hors zone rognée) */

static const char * const page_title[NPAGES] = {
    "COMMANDES DE BASE", "LES COUPS", "LE SERVICE", "LES REGLES",
    "LE DOUBLE", "MODES DE JEU", "COMPETITIONS ET CODES", "CONSEILS" };

static const char * const page_text[NPAGES][NLINES] = {
  { "CROIX : SE DEPLACER", "",
    "A     : FRAPPE LIFTEE", "        MAINTENIR A AVANT",
    "        L'IMPACT = PLUS FORT", "",
    "B     : COUP COUPE (SLICE)", "",
    "START : PAUSE", "        CONTINUER / QUITTER", "",
    "LA FRAPPE PART TOUTE SEULE", "QUAND LA BALLE ARRIVE A",
    "PORTEE PENDANT QUE VOUS", "ARMEZ, OU JUSTE APRES AVOIR",
    "RELACHE LE BOUTON.",
    "MENU : VITESSE DU JEU,", "FATIGUE ET SON." },
  { "A + HAUT : FRAPPE A PLAT", "A + BAS  : CROISE COURT",
    "B + HAUT : LOB", "B + BAS  : AMORTIE", "HAUT = VERS L'ADVERSAIRE",
    "GAUCHE OU DROITE AU MOMENT", "DE LA FRAPPE : VISER LE", "LONG DE LA LIGNE.",
    "SANS DIRECTION : CROISE.",
    "AVANT LE REBOND : VOLEE", "BALLE HAUTE : SMASH",
    "TRES HAUTE : SMASH SAUTE", "",
    "BALLE A COTE DU CORPS ET", "BOUTON RELACHE JUSTE AVANT",
    "L'IMPACT : COUP PARFAIT !",
    "TROP DE PUISSANCE VERS LES", "LIGNES = RISQUE DE FAUTE." },
  { "AVANT LE LANCER :", "GAUCHE/DROITE : SE PLACER", "",
    "A OU B : LANCER LA BALLE", "PUIS, AU SOMMET DU LANCER :",
    "A : SERVICE PUISSANT", "B : SERVICE COUPE, PLUS SUR",
    "A PILE AU SOMMET : CANON !",
    "GAUCHE/DROITE A L'IMPACT :", "VISER L'EXTERIEUR OU LE T.", "",
    "SANS FRAPPE, LA BALLE", "RETOMBE : LANCER A REFAIRE.", "",
    "2 FAUTES = DOUBLE FAUTE.", "BALLE QUI TOUCHE LE FILET", "PUIS TOMBE DANS LE CARRE :",
    "LET, SERVICE A REMETTRE." },
  { "POINTS : 0 15 30 40 JEU", "A 40-40 : EGALITE, PUIS", "AVANTAGE. IL FAUT 2 POINTS",
    "D'ECART POUR GAGNER LE JEU.", "",
    "SET : 6 JEUX, 2 D'ECART.", "A 6-6 : JEU DECISIF EN 7", "POINTS, 2 D'ECART.", "",
    "LE SERVICE CHANGE A CHAQUE", "JEU. CHANGEMENT DE COTE", "APRES CHAQUE JEU IMPAIR.", "",
    "FORMATS : 1, 3 OU 5 SETS,", "OU RAPIDE (SETS DE 4 JEUX).", "",
    "SCORE (COINS) : POINTS,", "* = SERVEUR, @ = SET GAGNE" },
  { "4 JOUEURS SUR LE COURT.", "LES COULOIRS SONT EN JEU,", "SAUF AU SERVICE.", "",
    "LES PARTENAIRES SERVENT A", "TOUR DE ROLE.", "",
    "EN EQUIPE : PARTENAIRE JOUE", "PAR LE CPU OU LA MANETTE 2.", "",
    "2 J. ADVERSAIRES : CHAQUE", "JOUEUR A UN PARTENAIRE CPU.", "",
    "LE PARTENAIRE CPU COUVRE", "SA MOITIE DU COURT ET VOUS", "LAISSE LES BALLES DE LA", "VOTRE.",
    "COMPETITION AUSSI, CPU OU J2" },
  { "1 JOUEUR CONTRE LE CPU", "2 JOUEURS ADVERSAIRES", "EN EQUIPE (CPU OU J2)", "",
    "COMPETITION : TOURNOI,", "POULES, CHAMPIONNAT, SAISON", "(4 SURFACES), PERSONNALISE", "",
    "ENTRAINEMENT : 9 EXERCICES,", "DONT UN TUTORIEL GUIDE.",
    "DEMO : CPU CONTRE CPU.", "UN TITRE OUVRE UN LIEU BONUS",
    "LIEUX : STADE, CLUB, SALLE,", "COURT MARGAUX.", "CONDITIONS : JOUR, NUIT,", "PLUIE, CREPUSCULE.",
    "SURFACES : TERRE, GAZON,", "DUR, MOQUETTE, PLAGE, GLACE" },
  { "AVANT CHAQUE MATCH D'UNE", "COMPETITION, UN CODE DE", "13 SIGNES S'AFFICHE.", "",
    "NOTEZ-LE. POUR REPRENDRE :", "TITRE > CODE DE REPRISE", "HAUT/BAS : CHANGER LE SIGNE",
    "GAUCHE/DROITE : POSITION", "START : VALIDER", "",
    "UN CODE MAL RECOPIE EST", "REFUSE.", "",
    "LE PALMARES GARDE VOS", "STATISTIQUES, TITRES ET", "RECORDS (TANT QUE LA",
    "CONSOLE EST ALLUMEE, OU", "TOUJOURS SI PILE)." },
  { "PLACEZ-VOUS A COTE DE LA", "TRAJECTOIRE, PAS DESSUS.", "",
    "ARMEZ TOT SUR LES BALLES", "LENTES POUR FRAPPER FORT.", "",
    "CONTRE UN VOLLEYEUR : LOB.", "CONTRE UN JOUEUR AU FOND :", "AMORTIE.", "",
    "GAZON, GLACE : BALLE QUI", "FILE, MONTEZ AU FILET.", "TERRE, PLAGE : BALLE LENTE,",
    "CONSTRUISEZ DU FOND.", "",
    "CHAQUE ADVERSAIRE A SON", "STYLE ET L'IA S'ADAPTE A", "VOS HABITUDES." },
};

u8 page;   /* page affichée (lue par les tests) */

/* Illustrations (2.72) : en haut à droite de chaque page, un joueur (ALEX,
   de face) enchaîne les gestes dont parle la page ; une balle les
   accompagne. Séquences : pose, durée (trames), position de la balle
   (dx, dy depuis les pieds ; 0, 0 : pas de balle). */
typedef struct { u8 pose, t; s8 bx, by; } Step;
static const Step seq0[] = { { POSE_RUN1, 8, 0, 0 }, { POSE_RUN3, 8, 0, 0 }, { POSE_RUN2, 8, 0, 0 }, { POSE_RUN4, 8, 0, 0 },
                             { POSE_RUN1, 8, 0, 0 }, { POSE_RUN3, 8, 0, 0 }, { POSE_PREP, 20, 14, -14 }, { POSE_IMPACT, 8, 12, -12 },
                             { POSE_FOLLOW, 24, 0, 0 }, { POSE_IDLE, 30, 0, 0 } };
static const Step seq1[] = { { POSE_PREP, 18, 16, -12 }, { POSE_IMPACT, 8, 12, -12 }, { POSE_FOLLOW, 20, 0, 0 }, { POSE_IDLE, 14, 0, 0 },
                             { POSE_BPREP, 18, -10, -12 }, { POSE_BIMPACT, 8, -8, -12 }, { POSE_BFOLLOW, 20, 0, 0 }, { POSE_IDLE, 14, 0, 0 },
                             { POSE_VOLLEY, 24, 12, -18 }, { POSE_IDLE, 14, 0, 0 }, { POSE_SRVHIT, 24, 6, -34 }, { POSE_IDLE, 14, 0, 0 },
                             { POSE_DROP, 24, 12, -10 }, { POSE_IDLE, 20, 0, 0 } };
static const Step seq2[] = { { POSE_DRIBBLE, 12, 4, -4 }, { POSE_IDLE, 10, 4, -12 }, { POSE_DRIBBLE, 12, 4, -4 }, { POSE_SERVE, 10, 4, -30 },
                             { POSE_SERVE, 10, 4, -38 }, { POSE_SERVE, 10, 4, -42 }, { POSE_SRVHIT, 10, 6, -36 }, { POSE_FOLLOW, 24, 0, 0 },
                             { POSE_IDLE, 30, 0, 0 } };
static const Step seq3[] = { { POSE_IDLE, 40, 0, 0 }, { POSE_FIST, 30, 0, 0 }, { POSE_IDLE, 30, 0, 0 }, { POSE_TIRED, 40, 0, 0 } };
static const Step seq4[] = { { POSE_VOLLEY, 30, 12, -18 }, { POSE_IDLE, 30, 0, 0 }, { POSE_BVOLLEY, 30, -10, -18 }, { POSE_IDLE, 30, 0, 0 } };
static const Step seq5[] = { { POSE_JOY, 30, 0, 0 }, { POSE_IDLE, 30, 0, 0 } };
static const Step seq6[] = { { POSE_FIST, 30, 0, 0 }, { POSE_JOY, 40, 0, 0 }, { POSE_IDLE, 30, 0, 0 } };
static const Step seq7[] = { { POSE_IDLE, 20, 0, 0 }, { POSE_REACH, 16, 22, -10 }, { POSE_DIVE, 30, 26, -4 }, { POSE_KNEEL, 30, 0, 0 },
                             { POSE_IDLE, 30, 0, 0 } };
static const Step * const seqs[NPAGES] = { seq0, seq1, seq2, seq3, seq4, seq5, seq6, seq7 };
static const u8 seq_n[NPAGES] = { sizeof seq0 / 4, sizeof seq1 / 4, sizeof seq2 / 4, sizeof seq3 / 4,
                                  sizeof seq4 / 4, sizeof seq5 / 4, sizeof seq6 / 4, sizeof seq7 / 4 };
static u8 an_i, an_t;
#define ILL_X 220
#define ILL_Y 40
static void illus_tick(void)
{
    const Step *st = seqs[page] + an_i;
    u8 dy = (u8)(st->pose == POSE_KNEEL ? 4 : 0);
    spr_begin();
    spr_person(ILL_X, (s16)(ILL_Y + dy), 0, 1, st->pose, 0, 0);
    if (page == 4) spr_person(ILL_X - 24, ILL_Y, 1, 1, st->pose == POSE_IDLE ? POSE_IDLE : POSE_PREP, 1, 1);   /* le double : un partenaire */
    if (st->bx || st->by) spr1((u8)(ILL_X + st->bx), (u8)(ILL_Y + st->by), ball_base, 3);
    spr_end();
    if (++an_t >= st->t) { an_t = 0; if (++an_i >= seq_n[page]) an_i = 0; }
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
    n[0] = (char)('1' + page); n[1] = '/'; n[2] = (char)('0' + NPAGES); n[3] = 0;
    txt_at(26, 1, n);
    txt_at(2, 3, page_title[page]);
    for (i = 0; i < NLINES; ++i) txt_at(2, (u8)(5 + i), page_text[page][i]);
    txt_at(2, 25, "GAUCHE/DROITE : PAGE");
    txt_at(2, 26, "B : RETOUR");
    people_tiles(0);                    /* poses des joueurs (illustrations, 2.72) */
    pal_set_player(0, 0); pal_set_player(1, 1);
    an_i = an_t = 0;
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
