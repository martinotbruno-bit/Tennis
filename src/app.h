/* Royal Martinot — organisation de l'application en banques MMC1. */
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
#define BANK_INTRO   7   /* générique de lancement */
#define BANK_PODIUM  8   /* podium d'une compétition gagnée */
#define BANK_CREDITS 9   /* générique de fin */
#define BANK_MUSIC  10   /* musiques (copiées en RAM de cartouche par music_play) */
#define BANK_PERSP  11   /* vue en perspective : courts déformés, tuiles, projection */
#define BANK_SWAP 13     /* jeux de tuiles par phase (célébrations, revers) */
#define BANK_COURTS 12   /* tables de noms des courts (vue à plat) */

/* état de l'application (lu par la boucle principale, banque fixe) */
#define AS_SCREENS 0
#define AS_MATCH   1
#define AS_MANUAL  2
#define AS_INTRO   3
#define AS_CREDITS 4
extern u8 app_state;

/* requêtes de transition entre banques, traitées par la boucle principale */
#define GO_NONE   0
#define GO_MATCH  1      /* lancer le match décrit par cfg */
#define GO_SCREEN 2      /* entrer dans l'écran go_arg (banque des écrans) */
#define GO_MANUAL 3
#define GO_RESUME 4      /* reprendre le match après l'écran de fin de set */
#define GO_INTRO  5      /* générique de lancement */
#define GO_CREDITS 6     /* générique de fin (saison gagnée) */
extern u8 go_req, go_arg;
extern u8 is_pal;

/* écrans (go_arg) */
#define SCR_TITLE       0
#define SCR_MENU        1
#define SCR_AFTER_MATCH 2
#define SCR_SETBREAK    3   /* récapitulatif de fin de set */

/* points d'entrée des banques */
void match_begin(void);            /* BANK_GAME */
u8 match_tick(void);               /* 1 : fin du match ; 2 : fin d'un set */
void match_resume(void);
void match_draw(void);
void screens_enter(u8 scr);        /* BANK_SCREENS */
void screens_tick(void);
void manual_begin(void);           /* BANK_MANUAL */
void manual_tick(void);
void intro_begin(void);            /* BANK_INTRO */
void podium_build(void);           /* BANK_PODIUM : écran (rendu coupé) */
void podium_sprites(void);         /* BANK_PODIUM : coupe, feux d'artifice, confettis */
extern u8 pod_new;                 /* club nudiste débloqué à l'instant */
void credits_begin(void);          /* BANK_CREDITS */
void credits_tick(void);
void intro_tick(void);
#endif
