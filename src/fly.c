/* Royal Martinot — survol du stade avant une finale (banque 20, version 2.73).
 *
 * Trois bandes de 80 lignes, chacune large de deux écrans (512 px, miroir
 * vertical) : le ciel (nuages, titre), les tribunes pleines, le court vu de
 * côté (couleur de la surface, lignes, filet, panneaux). La caméra glisse
 * vers la droite pendant 256 trames ; chaque bande défile à sa vitesse
 * (ciel 1/4, tribunes 1/2, court 1 px par trame) : parallaxe. Le défilement
 * de la bande du haut est posé par la NMI (scroll_x, scroll_nt), ceux des
 * deux autres par l'interruption de ligne du MMC3 (crt0.s, split_on 4,
 * lignes 80 et 160). Les tuiles ($C0-$CF du décor) sont dessinées ici ; la
 * CHR-RAM est rechargée à la fin (chr_reload). A ou START passe. */
#include "app.h"
#include "gfx.h"
#include "audio.h"
#include "game.h"
#include "comp.h"

#pragma static-locals(off)          /* (variables locales sur la pile : la RAM de travail est pleine) */


#define T_CLOUD 0xC0            /* 2 x 2 tuiles */
#define T_ROOF  0xC4
#define T_FANA  0xC5
#define T_FANB  0xC6
#define T_RAIL  0xC7
#define T_GRND  0xC8
#define T_LINE  0xC9
#define T_POST  0xCA
#define T_NET   0xCB
#define T_AD    0xCC
#define T_ADT   0xCD
#define T_SKY   0x00

/* tuiles (tools : dessins 8 x 8 en caractères 0 à 3, encodés en deux plans) */
static const u8 fly_chr[224] = {
    0x00,0x07,0x1F,0x3F,0x7F,0xFF,0xFF,0xFF,0x00,0x07,0x1F,0x3F,0x7F,0xFF,0xFF,0xFF,   /* cloud_tl */
    0x00,0xE0,0xF8,0xFC,0xFE,0xFF,0xFF,0xFF,0x00,0xE0,0xF8,0xFC,0xFE,0xFF,0xFF,0xFF,   /* cloud_tr */
    0xFF,0xFF,0x7F,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0x7F,0x00,0x00,0x00,0x00,   /* cloud_bl */
    0xFF,0xFF,0xFE,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFE,0x00,0x00,0x00,0x00,   /* cloud_br */
    0xFF,0xFF,0x00,0xFF,0xFF,0xFF,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00,0xFF,0x00,   /* roof */
    0x99,0x99,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x66,0x66,0xF7,0xF7,0x94,0xFF,0x00,0x00,   /* fanA */
    0x33,0x33,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xCC,0xCC,0xEF,0xEF,0x91,0xFF,0x00,0x00,   /* fanB */
    0xFF,0x00,0x00,0xFF,0xB6,0xB6,0xB6,0xFF,0x00,0xFF,0xFF,0x00,0x49,0x49,0x49,0x00,   /* rail */
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,   /* ground */
    0xFF,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0xFF,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,   /* line */
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,   /* post */
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xFF,   /* net */
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,   /* ad */
    0xFF,0x91,0xAB,0x91,0xAB,0x91,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,   /* adt */
};

#pragma bss-name(push, "SAVEDATA")   /* (zone libre, après les blocs sauvegardés : fly.c est lié en dernier) */
static u16 fly_t;
#pragma bss-name(pop)
static u8 surf_col(void)
{
    static const u8 sc[7] = { 0x17, 0x1A, 0x12, 0x14, 0x28, 0x31, 0x24 };   /* terre, gazon, dur, moquette, plage, glace, Margaux */
    return sc[cfg.env == ENV_MARGAUX ? 6 : (cfg.surface < 6 ? cfg.surface : 2)];
}
static void row_fill(u16 nt, u8 y, u8 a, u8 b)   /* rangée y des deux écrans : tuiles a, b en alternance */
{
    u8 i;
    vram_adr((u16)(nt + y * 32));
    for (i = 0; i < 32; ++i) vram_put((u8)((i & 1) ? b : a));
}
static void cloud(u16 nt, u8 x, u8 y)
{
    vram_adr((u16)(nt + y * 32 + x)); vram_put(T_CLOUD); vram_put(T_CLOUD + 1);
    vram_adr((u16)(nt + (y + 1) * 32 + x)); vram_put(T_CLOUD + 2); vram_put(T_CLOUD + 3);
}
void fly_build(void)
{
    u8 k, y, i;
    u16 nt;
    ppu_off();
    vbuf_reset();
    vram_adr((u16)T_CLOUD << 4);
    for (i = 0; i < sizeof fly_chr; ++i) vram_put(fly_chr[i]);
    for (k = 0; k < 2; ++k) {
        nt = k ? 0x2400 : 0x2000;
        vram_adr(nt); vram_fill(T_SKY, 32 * 10);                 /* ciel */
        row_fill(nt, 10, T_ROOF, T_ROOF);                        /* toit */
        for (y = 11; y < 19; ++y) row_fill(nt, y, y == 14 ? T_RAIL : T_FANA, y == 14 ? T_RAIL : T_FANB);
        row_fill(nt, 19, T_RAIL, T_RAIL);
        row_fill(nt, 20, T_AD, T_ADT); row_fill(nt, 21, T_ADT, T_AD);   /* panneaux */
        for (y = 22; y < 30; ++y) row_fill(nt, y, y == 23 || y == 28 ? T_LINE : T_GRND, y == 23 || y == 28 ? T_LINE : T_GRND);
        /* attributs : ciel 0, tribunes 1 et 2 en damier, court 3 */
        vram_adr((u16)(nt + 0x3C0));
        /* (un octet couvre 4 rangées : celui des rangées 8-11 mêle le ciel en
           haut et les tribunes en bas) */
        for (i = 0; i < 64; ++i) vram_put((u8)(i < 16 ? 0x00 : i < 24 ? ((i & 1) ? 0x90 : 0x60) : i < 40 ? ((i & 1) ? 0x99 : 0x66) : 0xFF));
    }
    /* nuages */
    cloud(0x2000, 3, 1); cloud(0x2000, 12, 7); cloud(0x2000, 24, 2); cloud(0x2000, 29, 6);
    cloud(0x2400, 6, 3); cloud(0x2400, 18, 1); cloud(0x2400, 27, 5);
    /* filet du court (écran 1, colonne 4 : arrive au milieu de l'image) */
    for (y = 22; y < 28; ++y) { vram_adr((u16)(0x2400 + y * 32 + 4)); vram_put(T_POST); vram_put(T_NET); vram_put(T_NET); vram_put(T_POST); }
    /* titre dans le ciel */
    COMP(comp_cname, c_type);
    if (c_type < CT_GRAND) txt_at(3, 3, "ROYAL MARTINOT");
    txt_at(c_type < CT_GRAND ? 18 : 3, 3, c_type == CT_GRAND ? "LE GRAND MARTINOT" : c_type == CT_WORLD ? "MARTINWORLD TOUR" : comp_l2);
    txt_at(3, 5, "LA FINALE");
    /* palettes : ciel, tribunes (deux couleurs de maillots), court */
    pal_buf[0] = 0x21;
    pal_buf[1] = 0x0F; pal_buf[2] = 0x10; pal_buf[3] = 0x30;
    pal_buf[5] = 0x00; pal_buf[6] = 0x37; pal_buf[7] = 0x16;
    pal_buf[9] = 0x00; pal_buf[10] = 0x37; pal_buf[11] = 0x12;
    pal_buf[13] = surf_col(); pal_buf[14] = 0x30; pal_buf[15] = 0x0F;
    for (i = 4; i < 32; i += 4) pal_buf[i] = 0x21;
    pal_buf[16] = 0x21;
    spr_begin(); spr_end();
    fly_t = 0;
    scroll_x = 0; scroll_nt = 0; fly_x[0] = fly_x[1] = 0; fly_n[0] = fly_n[1] = 0;
    ppu_on();
    fly_on(79);                         /* bandes : lignes 80 et 160 */
    sfx_crowd(7);
}
/* à chaque trame ; 1 quand le survol est fini (A ou START, ou 256 trames) */
u16 __fastcall__ fly_tick(u16 pad)
{
    u16 c = fly_t + (fly_t >> 2);       /* court : un peu plus vite que 1 px par trame (320 px) */
    if ((pad & (PAD_A | PAD_START)) || fly_t >= 255) {
        split_stop();
        scroll_x = 0; scroll_nt = 0;
        ppu_off();
        chr_reload();
        return 1;
    }
    ++fly_t;
    scroll_x = (u8)(fly_t >> 2);                                    /* ciel */
    fly_x[0] = (u8)(fly_t >> 1);                                    /* tribunes */
    fly_x[1] = (u8)c; fly_n[1] = (u8)(c >> 8);                      /* court */
    if ((fly_t & 63) == 0) sfx_crowd(5);
    return 0;
}
