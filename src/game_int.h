/* Pixel Open — état interne du match partagé entre le moteur (BANK_GAME)
 * et les fonctions annexes (BANK_EXTRAS). Les fonctions annexes ne peuvent
 * appeler que la banque fixe : elles communiquent par ces variables. */
#ifndef GAME_INT_H
#define GAME_INT_H
#include "game.h"

#define FP 64
#define PX(v) ((s16)(v) >> 6)

extern u8 npl;
extern s16 plx[4], ply[4];
extern s16 bx, by, bz, bvx, bvy, bvz;
extern u8 bgrav, bspin, bounces, hitter, hitter_p, b_serve, b_vis, b_live;
extern s16 land_x, land_y;
extern u8 rally_hits, ms, ms_timer, faults, paused;
extern u8 msg_timer, hud_dirty;
extern char msg1[16], msg2[16];
extern u8 pose_cache[4], flip_cache[4];

#define MS_SERVE  0     /* serveur en place, attend le lancer */
#define MS_TOSS   1     /* balle lancée, attend la frappe */
#define MS_RALLY  2
#define MS_DEAD   3     /* point joué, la balle finit sa course */
#define MS_OVER   4
#define MS_REPLAY 5     /* ralenti du point */

#define SPIN_FLAT 0
#define SPIN_TOP  1
#define SPIN_SLICE 2
#define SPIN_DROP  3   /* amortie */

/* types de coups (tutoriel, statistiques) */
#define SH_DRIVE  0
#define SH_SLICE  1
#define SH_LOB    2
#define SH_DROP   3
#define SH_FLAT   4
#define SH_SHORT  5
#define SH_VOLLEY 6
#define SH_SMASH  7
extern u8 last_shot, last_aim;

/* --- entraînement (événements envoyés à train_event) --- */
#define TEV_LAND_IN     1
#define TEV_LAND_OUT    2
#define TEV_MISS        3
#define TEV_SERVE_IN    4
#define TEV_SERVE_FAULT 5
extern u8 tr_ev, tr_done, tr_human_serves, tr_step, tr_target, tr_streak;
extern s16 ev_x, ev_y;
/* balle du lanceur, calculée par train_feed() puis lancée par le moteur */
extern s16 feed_x, feed_y, feed_z, feed_tx, feed_ty;
extern u8 feed_T, feed_g, feed_spin, feed_px_set;
extern s16 feed_px, feed_py;    /* position imposée au joueur (volée) */
extern u8 tgt_x, tgt_y;         /* cible affichée (0 = aucune) */

/* --- public animé --- */
extern u8 crowd_timer;

/* --- ralentis --- */
extern u8 rp_pending, rp_done;

/* fonctions de la banque BANK_EXTRAS (appel via bank_call) */
void train_feed(void);
void train_event(void);
void train_start(void);
void rp_record(void);           /* banque fixe (assembleur) */
void rp_start(void);
void rp_play(void);
void crowd_tick(void);
void load_court(void);
void hud_update(void);
void rain_tick(void);
void rain_draw(void);
extern u8 pause_cur;
#endif
