#ifndef AUDIO_H
#define AUDIO_H
#include "runtime.h"
void audio_update(void);          /* à appeler une fois par trame logique */
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
#define MUS_NONE    0
#define MUS_TITLE   1
#define MUS_POINT   2
#define MUS_GAME    3
#define MUS_VICTORY 4
#define MUS_DEFEAT  5
#endif
