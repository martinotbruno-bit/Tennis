/* Pixel Open — mode d'emploi intégré (banque BANK_MANUAL). */
#include "app.h"
#include "gfx.h"
#include "audio.h"

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
    "RELACHE LE BOUTON.", "", "" },
  { "A + HAUT : FRAPPE A PLAT", "A + BAS  : CROISE COURT",
    "B + HAUT : LOB", "B + BAS  : AMORTIE", "",
    "GAUCHE OU DROITE AU MOMENT", "DE LA FRAPPE : VISER LE", "LONG DE LA LIGNE.",
    "SANS DIRECTION : CROISE.", "",
    "AVANT LE REBOND : VOLEE", "BALLE HAUTE AU FILET : SMASH", "",
    "BALLE LOIN DU CORPS :", "FRAPPE EN EXTENSION,", "MOINS PRECISE.",
    "TROP DE PUISSANCE VERS LES", "LIGNES = RISQUE DE FAUTE." },
  { "AVANT LE LANCER :", "GAUCHE/DROITE : SE PLACER", "",
    "A OU B : LANCER LA BALLE", "PUIS, AU SOMMET DU LANCER :",
    "A : SERVICE PUISSANT", "B : SERVICE COUPE, PLUS SUR", "",
    "GAUCHE/DROITE A L'IMPACT :", "VISER L'EXTERIEUR OU LE T.", "",
    "SANS FRAPPE, LA BALLE", "RETOMBE : LANCER A REFAIRE.", "",
    "2 FAUTES = DOUBLE FAUTE.", "BALLE QUI TOUCHE LE FILET", "PUIS TOMBE DANS LE CARRE :",
    "LET, SERVICE A REMETTRE." },
  { "POINTS : 0 15 30 40 JEU", "A 40-40 : EGALITE, PUIS", "AVANTAGE. IL FAUT 2 POINTS",
    "D'ECART POUR GAGNER LE JEU.", "",
    "SET : 6 JEUX, 2 D'ECART.", "A 6-6 : JEU DECISIF EN 7", "POINTS, 2 D'ECART.", "",
    "LE SERVICE CHANGE A CHAQUE", "JEU. CHANGEMENT DE COTE", "APRES CHAQUE JEU IMPAIR.", "",
    "FORMATS : 1, 3 OU 5 SETS,", "OU RAPIDE (SETS DE 4 JEUX).", "",
    "SCORE (COINS) : J = JEUX,", "@ = SET GAGNE, * = SERVEUR" },
  { "4 JOUEURS SUR LE COURT.", "LES COULOIRS SONT EN JEU,", "SAUF AU SERVICE.", "",
    "LES PARTENAIRES SERVENT A", "TOUR DE ROLE.", "",
    "2 J. EQUIPE : MANETTES 1 ET", "2 ENSEMBLE CONTRE LE CPU.", "",
    "2 J. ADVERSAIRES : CHAQUE", "JOUEUR A UN PARTENAIRE CPU.", "",
    "LE PARTENAIRE CPU COUVRE", "SA MOITIE DU COURT ET VOUS", "LAISSE LES BALLES DE LA", "VOTRE.",
    "COMPETITION AUSSI, CPU OU J2" },
  { "1 JOUEUR CONTRE LE CPU", "2 JOUEURS (ADVERSAIRES OU", "EQUIPE EN DOUBLE)", "",
    "COMPETITION : TOURNOI,", "POULES, CHAMPIONNAT, SAISON", "(4 SURFACES), PERSONNALISE", "",
    "ENTRAINEMENT : 9 EXERCICES,", "DONT UN TUTORIEL GUIDE.", "",
    "DEMO : CPU CONTRE CPU.", "UN TITRE OUVRE UN LIEU BONUS",
    "LIEUX : STADE, CLUB, SALLE.", "CONDITIONS : JOUR, NUIT,", "PLUIE (PAS EN SALLE).",
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
    ppu_on();
}

void manual_begin(void)
{
    page = 0;
    draw_page();
}

void manual_tick(void)
{
    if (pad1_new & PAD_RIGHT) { page = (u8)(page + 1 == NPAGES ? 0 : page + 1); sfx_menu_move(); draw_page(); }
    else if (pad1_new & PAD_LEFT) { page = (u8)(page ? page - 1 : NPAGES - 1); sfx_menu_move(); draw_page(); }
    else if (pad1_new & (PAD_B | PAD_START)) { sfx_menu_ok(); go_req = GO_SCREEN; go_arg = SCR_TITLE; }
}
