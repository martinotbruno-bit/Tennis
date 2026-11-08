#ifndef GFX_H
#define GFX_H
#include "runtime.h"

#define NCHARS 6
extern const char * const char_name[NCHARS];
extern const u8 char_female[NCHARS];

#define NSURF 4
extern const char * const surf_name[NSURF];

extern u8 oam_i;

void pal_set_court(u8 surface, u8 night);
void pal_set_menu(void);
void pal_set_player(u8 slot, u8 character);
void spr_begin(void);
void spr_end(void);
void __fastcall__ spr1(u8 x, u8 y, u8 tile, u8 attr);
void spr_person(s16 x, s16 y, u8 character, u8 front, u8 pose, u8 flip, u8 pal);

/* texte direct (rendu coupé) */
void txt_at(u8 x, u8 y, const char *s);
/* texte via le tampon NMI (rendu actif) ; len octets, complétés par des espaces */
void vbuf_text(u8 x, u8 y, const char *s, u8 len);
void vbuf_reset(void);
void num2(char *out, u8 v);
void clear_screen(void);
#endif
