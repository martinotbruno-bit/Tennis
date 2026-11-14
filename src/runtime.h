/* Royal Martinot — interface C des primitives matérielles (crt0.s). */
#ifndef RUNTIME_H
#define RUNTIME_H

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned int u16;
typedef signed int s16;

/* Boutons (ordre de lecture de la manette NES) */
#define PAD_A      0x80
#define PAD_B      0x40
#define PAD_SELECT 0x20
#define PAD_START  0x10
#define PAD_UP     0x08
#define PAD_DOWN   0x04
#define PAD_LEFT   0x02
#define PAD_RIGHT  0x01

#ifdef __CC65__
extern u8 oam_buf[256];
extern u8 fade_lvl;           /* fondu de la palette (0..64, par 16) */
#pragma zpsym("fade_lvl")
extern u8 fade_skip;          /* 1 : prochaine coupure de l'écran sans fondu */
#pragma zpsym("fade_skip")
extern u8 spr_rot;            /* rotation de l'ordre des sprites (0 : aucune) */
#pragma zpsym("spr_rot")
extern u8 vram_buf[128];
extern u8 pal_buf[32];
extern u16 region_cnt;
#pragma zpsym("region_cnt")
extern u8 frame_cnt;
#pragma zpsym("frame_cnt")
extern u8 rand_seed;               /* générateur (poids faible) */
#pragma zpsym("rand_seed")
extern u8 pal_dirty;
#pragma zpsym("pal_dirty")
extern u8 ppu_mask;
#pragma zpsym("ppu_mask")
extern u8 pad1;
#pragma zpsym("pad1")
extern u8 pad2;
#pragma zpsym("pad2")
extern u8 pad1_new;
#pragma zpsym("pad1_new")
extern u8 pad2_new;
#pragma zpsym("pad2_new")
extern u8 vram_len;
#pragma zpsym("vram_len")
extern u8 scroll_y, scroll_nt;      /* défilement vertical (générique de fin) */
#pragma zpsym("scroll_y")
#pragma zpsym("scroll_nt")
extern u8 scroll_x, tz_bank, tz_n;  /* défilement horizontal ; tz_unpack (bande-annonce) */
#pragma zpsym("scroll_x")
#pragma zpsym("tz_bank")
#pragma zpsym("tz_n")
extern const u8 *tz_src;            /* tz_unpack : suite des données (pour continuer) */
#pragma zpsym("tz_src")
void __fastcall__ tz_unpack(const u8 *src);   /* tuiles compressées -> PPU (rendu coupé) */
void __fastcall__ tz_copy(const u8 *src);     /* tz_n octets (0 : 256) d'une banque -> PPU */
void __fastcall__ mirror_h(u8 h);   /* 1 : écrans empilés (MMC1 horizontal) ; rendu coupé */

extern u8 oam_i, meta_x, meta_y, meta_tile, meta_attr;
#pragma zpsym("oam_i")
#pragma zpsym("meta_x")
#pragma zpsym("meta_y")
#pragma zpsym("meta_tile")
#pragma zpsym("meta_attr")
void __fastcall__ spr1(u8 x, u8 y, u8 tile, u8 attr);
void spr_meta(void);
extern u8 cur_bank;
#pragma zpsym("cur_bank")
void __fastcall__ set_bank(u8 bank);
void __fastcall__ bank_call(u8 bank, void (*fn)(void));
u16 __fastcall__ far_call(u8 bank, void *fn, u16 arg);
void spr_end(void);
void wait_frame(void);
void pad_poll(void);
void ppu_off(void);
void __fastcall__ split_start(u8 o);   /* MMC3 : glissade de l'affiche VS (lignes 32-143), décalage o */
void split_stop(void);
void __fastcall__ band_on(u8 l);         /* MMC3 : bandeau du score en bas de l'écran (écran 1 dès la ligne l, 2.71) */
void ppu_on(void);
void __fastcall__ vram_adr(u16 adr);
void __fastcall__ vram_put(u8 v);
void __fastcall__ vram_fill(u8 v, u16 n);
void __fastcall__ vram_unrle(const u8 *data);
u8 rand8(void);

#define APU(reg) (*(volatile u8 *)(reg))
#endif

#define NTADR(x, y) (0x2000 + ((y) << 5) + (x))

#endif
