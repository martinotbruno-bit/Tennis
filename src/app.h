/* Pixel Open — organisation de l'application en banques MMC1. */
#ifndef APP_H
#define APP_H
#include "runtime.h"

/* banques commutables ($8000-$BFFF) ; la banque 7 est fixe en $C000 */
#define BANK_GAME    0   /* moteur de match */
#define BANK_SCREENS 1   /* titre, menus, compétitions, palmarès */
#define BANK_CHR     2   /* motifs graphiques copiés en CHR-RAM au démarrage */
#define BANK_EXTRAS  3   /* ralentis, exercices, public animé, tutoriel */
#define BANK_MANUAL  4   /* mode d'emploi */
#define BANK_COMP    5   /* compétitions et palmarès */
#define BANK_NUDE    6   /* tuiles des personnages au naturel */

/* état de l'application (lu par la boucle principale, banque fixe) */
#define AS_SCREENS 0
#define AS_MATCH   1
#define AS_MANUAL  2
extern u8 app_state;

/* requêtes de transition entre banques, traitées par la boucle principale */
#define GO_NONE   0
#define GO_MATCH  1      /* lancer le match décrit par cfg */
#define GO_SCREEN 2      /* entrer dans l'écran go_arg (banque des écrans) */
#define GO_MANUAL 3
extern u8 go_req, go_arg;
extern u8 is_pal;

/* écrans (go_arg) */
#define SCR_TITLE       0
#define SCR_MENU        1
#define SCR_AFTER_MATCH 2

/* points d'entrée des banques */
void match_begin(void);            /* BANK_GAME */
u8 match_tick(void);
void match_draw(void);
void screens_enter(u8 scr);        /* BANK_SCREENS */
void screens_tick(void);
void manual_begin(void);           /* BANK_MANUAL */
void manual_tick(void);
#endif
