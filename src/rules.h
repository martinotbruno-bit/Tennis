/* Royal Martinot — règles et score du tennis (code portable, testé sur PC). */
#ifndef RULES_H
#define RULES_H

typedef unsigned char ru8;

#define MAX_SETS 5

/* Événements renvoyés par rules_point() (bits combinables) */
#define EV_POINT      0x00
#define EV_GAME       0x01
#define EV_SET        0x02
#define EV_MATCH      0x04
#define EV_CHANGE     0x08   /* changement de côté après ce point */
#define EV_TIEBREAK   0x10   /* le jeu décisif commence */

extern ru8 sc_points[2];      /* points du jeu en cours (ou du jeu décisif) */
extern ru8 sc_games[2];       /* jeux du set en cours */
extern ru8 sc_sets[2];        /* sets gagnés */
extern ru8 sc_hist[MAX_SETS][2]; /* score des sets terminés */
extern ru8 sc_set_no;         /* numéro du set en cours (0..) */
extern ru8 sc_tiebreak;       /* 1 si jeu décisif en cours */
extern ru8 sc_server;         /* équipe au service */
extern ru8 sc_swap;           /* 0 : équipe 0 en bas ; 1 : équipe 0 en haut */
extern ru8 sc_over, sc_winner;
extern ru8 sc_sets_to_win, sc_games_per_set;
extern ru8 sc_tb_first;       /* serveur du premier point du jeu décisif */
extern ru8 sc_set_first_server;

void rules_init(ru8 sets_to_win, ru8 games_per_set, ru8 first_server);
ru8 rules_point(ru8 team);
/* 0 = côté égalité (droite du serveur), 1 = côté avantage */
ru8 rules_ad_side(void);
/* Libellé des points de l'équipe (2 caractères) dans out[0..1] */
void rules_point_label(ru8 team, char *out);
/* 1 si le prochain point est une balle de jeu/set/match pour l'équipe */
ru8 rules_is_set_point(ru8 team);
ru8 rules_is_match_point(ru8 team);

#endif
