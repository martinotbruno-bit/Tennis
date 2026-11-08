/* Pixel Open — affichage : palettes, sprites, texte. */
#include "gfx.h"
#include "assets.h"

const char * const char_name[NCHARS] = { "ALEX", "MAYA", "LEON", "NINA", "OMAR", "ZOE " };
const u8 char_female[NCHARS]         = { 0, 1, 0, 1, 0, 1 };
/* peau, tenue, sombre (cheveux/short/raquette) */
static const u8 char_pal[NCHARS][3] = {
    { 0x27, 0x16, 0x07 },
    { 0x37, 0x12, 0x0F },
    { 0x17, 0x1A, 0x0F },
    { 0x27, 0x14, 0x08 },
    { 0x07, 0x21, 0x0F },
    { 0x36, 0x25, 0x18 },
};

const char * const surf_name[NSURF] = { "TERRE BATTUE", "GAZON       ", "DUR         ", "MOQUETTE    " };
/* fond universel, surface, lignes, filet ; tribunes (3 couleurs) */
static const u8 surf_pal[NSURF][7] = {
    { 0x17, 0x27, 0x30, 0x0F, 0x0F, 0x16, 0x37 },
    { 0x09, 0x1A, 0x30, 0x0F, 0x0F, 0x12, 0x37 },
    { 0x0A, 0x12, 0x30, 0x0F, 0x0F, 0x16, 0x38 },
    { 0x06, 0x15, 0x30, 0x0F, 0x0F, 0x11, 0x36 },
};

u8 oam_i;

static u8 darken(u8 c)
{
    if (c == 0x0F || c == 0x30) return c;
    return (c >= 0x10) ? (u8)(c - 0x10) : 0x0F;
}

void pal_set_court(u8 s, u8 night)
{
    const u8 *p = surf_pal[s];
    u8 i;
    pal_buf[0] = p[0]; pal_buf[1] = p[1]; pal_buf[2] = p[2]; pal_buf[3] = p[3];
    pal_buf[4] = p[0]; pal_buf[5] = 0x0F; pal_buf[6] = 0x00; pal_buf[7] = 0x30;  /* score */
    pal_buf[8] = p[0]; pal_buf[9] = 0x27; pal_buf[10] = p[5]; pal_buf[11] = p[6]; /* public */
    pal_buf[12] = p[0]; pal_buf[13] = 0x0F; pal_buf[14] = 0x28; pal_buf[15] = 0x30;
    /* sprites : 0/1 joueurs (définis à part), 2 balle, 3 effets */
    pal_buf[16 + 8] = p[0]; pal_buf[16 + 9] = 0x38; pal_buf[16 + 10] = 0x0F; pal_buf[16 + 11] = 0x30;
    pal_buf[16 + 12] = p[0]; pal_buf[16 + 13] = 0x30; pal_buf[16 + 14] = 0x21; pal_buf[16 + 15] = 0x28;
    if (night) {
        /* match de nuit : décors assombris, court éclairé */
        pal_buf[0] = 0x0F;
        for (i = 4; i < 16; i += 4) pal_buf[i] = 0x0F;
        pal_buf[10] = darken(pal_buf[10]); pal_buf[11] = darken(pal_buf[11]); pal_buf[9] = 0x07;
        pal_buf[16 + 8] = 0x0F; pal_buf[16 + 12] = 0x0F;
    }
    pal_buf[16] = pal_buf[20] = pal_buf[0];
    pal_dirty = 1;
}

void pal_set_menu(void)
{
    static const u8 mp[16] = { 0x0F, 0x01, 0x11, 0x30,  0x0F, 0x00, 0x10, 0x30,
                               0x0F, 0x27, 0x28, 0x30,  0x0F, 0x0F, 0x38, 0x30 };
    u8 i;
    for (i = 0; i < 16; ++i) pal_buf[i] = mp[i];
    pal_buf[16 + 8] = 0x0F; pal_buf[16 + 9] = 0x38; pal_buf[16 + 10] = 0x00; pal_buf[16 + 11] = 0x30;
    pal_buf[16 + 12] = 0x0F; pal_buf[16 + 13] = 0x30; pal_buf[16 + 14] = 0x21; pal_buf[16 + 15] = 0x28;
    pal_buf[16] = pal_buf[20] = 0x0F;
    pal_dirty = 1;
}

void pal_set_player(u8 slot, u8 c)
{
    u8 b = (u8)(16 + slot * 4);
    pal_buf[b + 1] = char_pal[c][0];
    pal_buf[b + 2] = char_pal[c][1];
    pal_buf[b + 3] = char_pal[c][2];
    pal_dirty = 1;
}

void spr_begin(void) { oam_i = 0; }

void spr_end(void)
{
    u8 i = oam_i;
    do { oam_buf[i] = 0xFF; i += 4; } while (i != 0);
}

void __fastcall__ spr1(u8 x, u8 y, u8 tile, u8 attr)
{
    u8 i = oam_i;
    oam_buf[i] = (u8)(y - 1);
    oam_buf[i + 1] = tile;
    oam_buf[i + 2] = attr;
    oam_buf[i + 3] = x;
    oam_i = (u8)(i + 4);
}

/* (x, y) = position des pieds ; sprite 16x24 */
void spr_person(s16 x, s16 y, u8 c, u8 front, u8 pose, u8 flip, u8 pal)
{
    u8 base, t, sx, sy, row;
    s16 l = x - 8, top = y - 24;
    if (l < 0) l = 0;
    if (l > 240) l = 240;
    if (top < 0) top = 0;
    if (top > 216) top = 216;
    sx = (u8)l; sy = (u8)top;
    base = (u8)(person_base[(char_female[c] << 1) | front] + pose * 6);
    for (row = 0; row < 3; ++row) {
        t = (u8)(base + row * 2);
        if (flip) {
            spr1(sx, sy, (u8)(t + 1), (u8)(pal | 0x40));
            spr1((u8)(sx + 8), sy, t, (u8)(pal | 0x40));
        } else {
            spr1(sx, sy, t, pal);
            spr1((u8)(sx + 8), sy, (u8)(t + 1), pal);
        }
        sy += 8;
    }
}

void txt_at(u8 x, u8 y, const char *s)
{
    vram_adr(NTADR(x, y));
    while (*s) vram_put((u8)*s++);
}

void vbuf_reset(void)
{
    vram_len = 0;
    vram_buf[0] = 0xFF;
}

void vbuf_text(u8 x, u8 y, const char *s, u8 len)
{
    u8 i = vram_len;
    u16 a = NTADR(x, y);
    if ((u16)i + len + 4 > 126) return;          /* tampon plein : ignoré */
    vram_buf[i++] = (u8)(a >> 8);
    vram_buf[i++] = (u8)a;
    vram_buf[i++] = len;
    while (len--) {
        vram_buf[i++] = (u8)(*s ? *s++ : ' ');
    }
    vram_buf[i] = 0xFF;
    vram_len = i;
}

void num2(char *out, u8 v)
{
    out[0] = (char)(v >= 10 ? '0' + (v / 10) % 10 : ' ');
    out[1] = (char)('0' + v % 10);
}

void clear_screen(void)
{
    vram_adr(0x2000);
    vram_fill(0, 1024);
}
