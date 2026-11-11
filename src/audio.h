#ifndef AUDIO_H
#define AUDIO_H
#include "runtime.h"
void music_play(u8 id);           /* 0 = arrêt */
u8 music_none(void);              /* 1 si aucune musique */
void sfx_hit(u8 power);           /* impact raquette, power 0..3 */
void sfx_bounce(void);
void sfx_net(void);
void sfx_fault(void);
void sfx_whistle(void);
void sfx_crowd(u8 strength);      /* réaction du public */
void sfx_menu_move(void);
void sfx_menu_ok(void);
void sfx_toss(void);
void sfx_step(void);               /* bruit de pas discret */
void sfx_perfect(void);            /* « tchak » sec de la frappe parfaite */
void sfx_canon(void);               /* « boum » du service canon */
void sfx_clap(void);                /* un seul « clap » timide */
void sfx_ooh(void);                 /* public : balle dehors de peu */
void sfx_groan(void);               /* public : double faute */
void sfx_hush(void);                /* public : silence au lancer */
void sfx_line(void);                /* bip du juge électronique (balle dehors) */
void sfx_swing(void);               /* souffle du geste de raquette */
void sfx_fw_launch(void);           /* feu d'artifice : la fusée monte */
void sfx_fw_pop(void);              /* feu d'artifice : « pop » */
void sfx_winner(void);             /* carillon bref d'un coup gagnant */
#define MUS_NONE    0
#define MUS_TITLE   1   /* polka de l'écran titre et du menu */
#define MUS_POINT   2
#define MUS_GAME    3
#define MUS_VICTORY 4
#define MUS_DEFEAT  5
#define MUS_FINAL   6   /* avant la finale d'une compétition */
#define MUS_MARGAUX 7   /* court Margaux (menus et présentation) */
#define MUS_BREAK   8   /* interlude de l'écran de fin de set */
#define MUS_INTRO   9   /* fanfare du générique de lancement */
#define MUS_CREDITS 10  /* générique de fin : hymne solennel au kazoo */
extern u8 music_cur;
extern u8 snd_mode;
void snd_set(u8 m);                /* 0 tout, 1 bruitages seuls, 2 aucun son */
#endif
