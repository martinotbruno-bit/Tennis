#ifndef GFX_H
#define GFX_H
#include "runtime.h"

#define NCHARS 21     /* 16 + Margaux (index 16) + MAX, LOLA, PAPI, TITOU (17 à 20) */
extern const char * const char_name[NCHARS];
extern const u8 char_female[NCHARS];
extern const u8 char_style[NCHARS];

/* styles de jeu des personnages contrôlés par l'IA (CDC 10.2) */
#define NSTYLES 9
extern const char * const style_name[NSTYLES];

#define NSURF 6         /* 4 surfaces réelles + 2 fantaisie */
extern const char * const surf_name[NSURF];
#define NENV 7          /* stade, club, salle, club nudiste (à débloquer), court Margaux, château, parc */
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
void __fastcall__ spr_ball(u8 x, s16 sy, u8 shy, u8 zpx);   /* sprp.s */

/* poses des personnages */
#define POSE_IDLE   0
#define POSE_RUN1   1
#define POSE_RUN2   2
#define POSE_SWING  3
#define POSE_FOLLOW 4
#define POSE_SERVE  5
#define POSE_JOY    6
#define POSE_SAD    7
#define POSE_BACK   8    /* revers à deux mains (armé) */
#define POSE_REACH  9    /* extension vers une balle lointaine */
#define POSE_SRVHIT 10   /* impact du service (et fin du lob) */
#define POSE_PREP   11   /* préparation du coup droit */
#define POSE_IMPACT 12
#define POSE_VOLLEY 13
#define POSE_DROP   14   /* amortie */
#define POSE_DRIBBLE 15  /* rebond de la balle avant le service */
#define POSE_FIST   16   /* poing serré */
#define POSE_ANGRY  17   /* colère : raquette au sol */
#define POSE_TIRED  18   /* mains sur les genoux */
#define POSE_RUN3   19   /* course, temps de passage */
#define POSE_RUN4   20
#define POSE_DIVE   21   /* plongeon */
#define POSE_BPREP  22   /* préparation du revers */
#define POSE_BIMPACT 23  /* impact du revers à deux mains */
#define POSE_BFOLLOW 24  /* fin du revers à deux mains */
#define POSE_BSLICE 25   /* revers coupé à une main (armé) */
#define POSE_BVOLLEY 26  /* volée de revers */
#define POSE_KNEEL  27   /* un genou à terre (relevé du plongeon) : dessinée 4 px plus bas */
#define POSE_LIFT   32   /* drapeau : personnage relevé d'1 px (rebond, foulée) */
#define POSE_DOWN   64   /* drapeau : personnage abaissé d'1 px (genoux fléchis) */

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
void chr_copy(u8 bank, const u8 *src, u16 dst, u16 len);   /* rendu coupé */
void bank_unrle(u8 bank, const u8 *rle);                   /* rendu coupé, adresse VRAM déjà fixée */
void bank_read(u8 bank, void *dst, const void *src, u16 n); /* copie depuis une banque vers la RAM */
extern u8 proj_ram[512 + 640];                                    /* tables de projection (sprp.s) */
extern u8 bg_dirty;
extern u8 persp;
#pragma zpsym("persp")
void chr_reload(void);
void chr_borrow_restore(void);     /* tuiles prêtées au match (arbitre) : rendu coupé */             /* toute la CHR-RAM d'origine (rendu coupé) */
extern u8 nude_unlocked;
void team_label(u8 a, u8 b, char *out);
void score_line(u8 t, char *out);   /* « ALEX 6 3 2 » (gamefix.c, 15 octets) */
extern const u8 chr_data[];
#define VBUF_MAX 96     /* octets envoyés au PPU par trame (budget VBlank) */
#endif
