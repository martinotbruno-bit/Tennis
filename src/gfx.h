#ifndef GFX_H
#define GFX_H
#include "runtime.h"

#define NCHARS 17     /* 16 + Margaux (index 16) */
extern const char * const char_name[NCHARS];
extern const u8 char_female[NCHARS];
extern const u8 char_style[NCHARS];

/* styles de jeu des personnages contrôlés par l'IA (CDC 10.2) */
#define NSTYLES 9
extern const char * const style_name[NSTYLES];

#define NSURF 6         /* 4 surfaces réelles + 2 fantaisie */
extern const char * const surf_name[NSURF];
#define NENV 5          /* stade, club, salle, club nudiste (à débloquer), court Margaux */
#define ENV_NUDE 3
#define ENV_MARGAUX 4
extern const char * const env_name[NENV];

void pal_set_court(u8 surface, u8 cond, u8 env);   /* cond : 0 jour, 1 nuit, 2 pluie */
void pal_set_menu(void);
void pal_set_player(u8 slot, u8 character);
void spr_begin(void);
void spr_end(void);
void __fastcall__ spr1(u8 x, u8 y, u8 tile, u8 attr);
void __fastcall__ spr_person(s16 x, s16 y, u8 character, u8 front, u8 pose, u8 flip, u8 pal);   /* sprp.s */
void spr_ball(u8 x, s16 sy, u8 shy, u8 zpx);

/* poses des personnages */
#define POSE_IDLE   0
#define POSE_RUN1   1
#define POSE_RUN2   2
#define POSE_SWING  3
#define POSE_FOLLOW 4
#define POSE_SERVE  5
#define POSE_JOY    6
#define POSE_SAD    7

/* texte direct (rendu coupé) */
void txt_at(u8 x, u8 y, const char *s);
/* texte via le tampon NMI (rendu actif) ; len octets, complétés par des espaces */
void vbuf_text(u8 x, u8 y, const char *s, u8 len);
/* écriture brute via le tampon NMI (motifs de tuiles, etc.) ; 0 si plein */
u8 vbuf_raw(u16 adr, const u8 *data, u8 len);
void vbuf_reset(void);
void num2(char *out, u8 v);
void num3(char *out, u16 v);
void clear_screen(void);
/* tuiles des personnages : 0 tenue normale, 1 au naturel (rendu coupé) */
void people_tiles(u8 nude);
extern u8 nude_unlocked;
void team_label(u8 a, u8 b, char *out);
void score_line(u8 t, char *out);   /* « ALEX 6 3 2 » (gamefix.c, 15 octets) */
extern const u8 chr_data[];
#define VBUF_MAX 96     /* octets envoyés au PPU par trame (budget VBlank) */
#endif
