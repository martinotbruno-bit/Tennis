#ifndef GAME_H
#define GAME_H
#include "runtime.h"

/* contrôleurs */
#define CTRL_PAD1 0
#define CTRL_PAD2 1
#define CTRL_CPU  2

/* modes de partie */
#define GM_MATCH       0
#define GM_TRAIN_RALLY 1   /* lanceur de balles */
#define GM_TRAIN_SERVE 2   /* travail du service */

/* conditions */
#define COND_DAY   0
#define COND_NIGHT 1
#define COND_RAIN  2

/* joueurs : index 0..3 ; équipe = index & 1 ; 2 et 3 = partenaires en double */
typedef struct {
    u8 mode;            /* GM_* */
    u8 doubles;         /* 0 simple, 1 double */
    u8 ctrl[4];         /* contrôleur de chaque joueur */
    u8 chr[4];          /* personnage de chaque joueur */
    u8 level[4];        /* niveau IA 0..2 */
    u8 surface;
    u8 cond;            /* COND_* */
    u8 sets_to_win;
    u8 games_per_set;
} MatchCfg;

extern MatchCfg cfg;

/* statistiques par équipe */
extern u8 st_points[2], st_aces[2], st_dfaults[2], st_winners[2], st_errors[2];
extern u8 st_first_in[2], st_first_tot[2], st_longest;
extern u8 tr_score, tr_total, tr_target;   /* entraînement */

void match_start(void);
/* une trame logique ; renvoie 1 quand la partie est terminée (ou quittée) */
u8 match_tick(void);
void match_draw(void);
extern u8 match_quit;

#endif
