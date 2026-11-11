/* Royal Martinot — configuration et état publics du moteur de match. */
#ifndef GAME_H
#define GAME_H
#include "runtime.h"

/* contrôleurs */
#define CTRL_PAD1 0
#define CTRL_PAD2 1
#define CTRL_CPU  2

/* modes de partie */
#define GM_MATCH  0
#define GM_TRAIN  1

/* exercices d'entraînement (CDC 9) */
#define EX_TARGETS   0   /* frappes avec cibles */
#define EX_MOVE      1   /* déplacements : balles alternées dans les coins */
#define EX_STREAK    2   /* régularité : série sans faute */
#define EX_PRECISION 3   /* précision : seule la zone désignée compte */
#define EX_VOLLEY    4   /* volée : au filet */
#define EX_SERVE     5   /* service avec zone cible */
#define EX_STATIC    6   /* échanges contre un adversaire immobile */
#define EX_CUSTOM    7   /* exercice personnalisé */
#define EX_TUTORIAL  8   /* tutoriel guidé des commandes */
#define NEX          9

/* conditions */
#define COND_DAY   0
#define COND_NIGHT 1
#define COND_RAIN  2

/* lieux */
#define ENV_STADIUM 0
#define ENV_CLUB    1
#define ENV_HALL    2
#define ENV_NUDE    3   /* club nudiste (débloqué par une victoire en compétition) */
#define ENV_MARGAUX 4   /* court Margaux : rose, décor coeurs, fleurs, étoiles */
#define ENV_CASTLE  5   /* château : murs de pierre, bannières, torches, balcons */
#define ENV_PARK    6   /* parc : décor du club en couleurs d'automne */

/* joueurs : index 0..3 ; équipe = index & 1 ; 2 et 3 = partenaires en double */
typedef struct {
    u8 mode;            /* GM_* */
    u8 doubles;
    u8 ctrl[4];
    u8 chr[4];
    u8 level[4];        /* niveau IA 0..2 */
    u8 surface;
    u8 cond;
    u8 sets_to_win;
    u8 games_per_set;
    u8 env;
    u8 replays;         /* ralentis des beaux points */
    u8 demo;            /* démonstration : un bouton rend la main */
    u8 ex;              /* exercice (GM_TRAIN) */
    u8 ex_side;         /* personnalisé : 0 coup droit, 1 revers, 2 alterné, 3 aléatoire */
    u8 ex_speed;        /* 0 lente, 1 moyenne, 2 rapide */
    u8 ex_spin;         /* 0 lifté, 1 coupé, 2 lob */
    u8 ex_count;        /* nombre de balles */
    u8 gspeed;          /* vitesse du jeu : 0 lent (75 %), 1 normal, 2 rapide (125 %) */
    u8 fatigue;         /* 1 : fatigue dans les très longs échanges */
} MatchCfg;

extern MatchCfg cfg;
void fw_sound(u8 t);            /* feux d'artifice (banque fixe, gamefix.c) */
void fw_draw(u8 t, u8 dy);

/* statistiques du dernier match, par équipe */
extern u8 st_points[2], st_aces[2], st_dfaults[2], st_winners[2], st_errors[2];
extern u8 st_first_in[2], st_first_tot[2], st_longest;
extern u8 st_canon[2], st_ctry[2], srv_canon;
extern u8 st_perf[2], st_ptry[2], hit_perf, swing_early;   /* frappes parfaites */
/* entraînement */
extern u8 tr_score, tr_total, tr_best;
extern u8 match_quit;

#endif
