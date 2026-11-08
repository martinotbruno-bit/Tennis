/* Pixel Open — affichage : palettes, sprites, texte. */
#include "gfx.h"
#include "assets.h"
#include "app.h"

const char * const char_name[NCHARS] = {
    "ALEX", "MAYA", "LEON", "NINA", "OMAR", "ZOE ", "HUGO", "EMMA",
    "IVAN", "LINA", "TOM ", "JADE", "LUCA", "SARA", "NOAH", "INES" };
const u8 char_female[NCHARS] = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 };
/* peau, tenue, sombre (cheveux/short/raquette) */
static const u8 char_pal[NCHARS][3] = {
    { 0x27, 0x16, 0x07 },
    { 0x37, 0x12, 0x0F },
    { 0x17, 0x1A, 0x0F },
    { 0x27, 0x14, 0x08 },
    { 0x07, 0x21, 0x0F },
    { 0x36, 0x25, 0x18 },
    { 0x37, 0x19, 0x07 },
    { 0x27, 0x13, 0x0F },
    { 0x36, 0x28, 0x0F },
    { 0x17, 0x2C, 0x08 },
    { 0x27, 0x1C, 0x0F },
    { 0x36, 0x15, 0x07 },
    { 0x37, 0x0B, 0x0F },
    { 0x17, 0x24, 0x0F },
    { 0x07, 0x29, 0x0F },
    { 0x27, 0x11, 0x08 },
};
const char * const style_name[NSTYLES] = {
    "EQUILIBRE", "FOND DE COURT", "PUISSANCE", "FILET",
    "DEFENSIF", "OFFENSIF", "TACTIQUE", "PLACEMENT" };
const u8 char_style[NCHARS] = { 0, 1, 2, 3, 4, 5, 6, 7, 2, 3, 5, 4, 1, 7, 6, 0 };

const char * const surf_name[NSURF] = {
    "TERRE BATTUE", "GAZON       ", "DUR         ", "MOQUETTE    ", "PLAGE       ", "GLACE       " };
const char * const env_name[NENV] = { "STADE", "CLUB ", "SALLE", "CLUB NUDISTE" };
/* fond universel, surface, lignes, filet */
static const u8 surf_pal[NSURF][4] = {
    { 0x17, 0x27, 0x30, 0x0F },
    { 0x09, 0x1A, 0x30, 0x0F },
    { 0x0A, 0x12, 0x30, 0x0F },
    { 0x06, 0x15, 0x30, 0x0F },
    { 0x21, 0x28, 0x16, 0x0F },   /* plage : sable jaune, distinct des teintes de peau */
    { 0x0C, 0x31, 0x12, 0x0F },
};
/* palette 2 (bords du court) selon le lieu */
static const u8 env_pal[NENV][3] = {
    { 0x27, 0x16, 0x37 },     /* stade : public */
    { 0x1A, 0x09, 0x07 },     /* club : haies, arbres, troncs */
    { 0x00, 0x2A, 0x0F },     /* salle : murs, panneaux, projecteurs */
    { 0x1A, 0x27, 0x16 },     /* club nudiste : herbe, peau, serviettes */
};

static u8 darken(u8 c)
{
    if (c == 0x0F || c == 0x30) return c;
    return (c >= 0x10) ? (u8)(c - 0x10) : 0x0F;
}

void pal_set_court(u8 s, u8 cond, u8 env)
{
    const u8 *p = surf_pal[s];
    const u8 *e = env_pal[env];
    u8 i;
    pal_buf[0] = p[0]; pal_buf[1] = p[1]; pal_buf[2] = p[2]; pal_buf[3] = p[3];
    pal_buf[4] = p[0]; pal_buf[5] = 0x0F; pal_buf[6] = 0x00; pal_buf[7] = 0x30;  /* score */
    pal_buf[8] = p[0]; pal_buf[9] = e[0]; pal_buf[10] = e[1]; pal_buf[11] = e[2]; /* bords */
    pal_buf[12] = p[0]; pal_buf[13] = 0x0F; pal_buf[14] = 0x28; pal_buf[15] = 0x30;
    /* sprites : 0/1 joueurs (définis à part), 2 balle, 3 effets */
    pal_buf[16 + 8] = p[0]; pal_buf[16 + 9] = 0x38; pal_buf[16 + 10] = 0x0F; pal_buf[16 + 11] = 0x30;
    pal_buf[16 + 12] = p[0]; pal_buf[16 + 13] = 0x30; pal_buf[16 + 14] = 0x21; pal_buf[16 + 15] = 0x28;
    if (cond == 2) {
        /* pluie : ciel couvert, couleurs ternies */
        pal_buf[0] = darken(pal_buf[0]);
        for (i = 4; i < 16; i += 4) pal_buf[i] = pal_buf[0];
        pal_buf[1] = darken(pal_buf[1]);
        pal_buf[16 + 8] = pal_buf[16 + 12] = pal_buf[0];
        pal_buf[16 + 13] = 0x21; pal_buf[16 + 15] = 0x31;   /* gouttes */
    }
    if (cond == 1) {
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

/* (x, y) = position des pieds ; sprite 16x24 dessiné par spr_meta (asm) */
void spr_person(s16 x, s16 y, u8 c, u8 front, u8 pose, u8 flip, u8 pal)
{
    s16 l = x - 8, top = y - 24;
    if (l < 0) l = 0;
    if (l > 240) l = 240;
    if (top < 0) top = 0;
    if (top > 216) top = 216;
    meta_x = (u8)l;
    meta_y = (u8)top;
    meta_tile = (u8)(person_base[(char_female[c] << 1) | front] + pose * 6);
    meta_attr = (u8)(flip ? (pal | 0x40) : pal);
    spr_meta();
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
    if ((u16)i + len + 4 > VBUF_MAX) return;     /* budget VBlank atteint : ignoré */
    vram_buf[i++] = (u8)(a >> 8);
    vram_buf[i++] = (u8)a;
    vram_buf[i++] = len;
    while (len--) {
        vram_buf[i++] = (u8)(*s ? *s++ : ' ');
    }
    vram_buf[i] = 0xFF;
    vram_len = i;
}

u8 vbuf_raw(u16 adr, const u8 *data, u8 len)
{
    u8 i = vram_len;
    if ((u16)i + len + 4 > VBUF_MAX) return 0;
    vram_buf[i++] = (u8)(adr >> 8);
    vram_buf[i++] = (u8)adr;
    vram_buf[i++] = len;
    while (len--) vram_buf[i++] = *data++;
    vram_buf[i] = 0xFF;
    vram_len = i;
    return 1;
}

void num3(char *out, u16 v)
{
    out[0] = (char)(v >= 100 ? '0' + (v / 100) % 10 : ' ');
    out[1] = (char)(v >= 10 ? '0' + (v / 10) % 10 : ' ');
    out[2] = (char)('0' + v % 10);
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

/* ---------------------------------------------------------------------- */
/* Tuiles des personnages en CHR-RAM : tenue normale (copie de la banque 2)
 * ou « au naturel » (banque 6). Rendu coupé uniquement. */
u8 nude_unlocked;       /* mode nudiste débloqué (victoire en compétition) */
static u8 chr_nude;

static void chr_copy(u8 bank, const u8 *src, u16 dst, u16 len)
{
    u8 old = cur_bank;
    set_bank(bank);
    vram_adr(dst);
    while (len--) vram_put(*src++);
    set_bank(old);
}

void people_tiles(u8 nude)
{
    if (nude == chr_nude) return;
    if (nude) chr_copy(BANK_NUDE, nude_people, 0x1100, 3072);
    else chr_copy(BANK_CHR, chr_data + 0x1100, 0x1100, 3072);
    chr_nude = nude;
}

/* Nom d'équipe : « ALEX » en simple, « AL/MA » en double (b = 255 : simple) */
void team_label(u8 a, u8 b, char *out)
{
    const char *n = char_name[a];
    if (b == 255) { out[0] = n[0]; out[1] = n[1]; out[2] = n[2]; out[3] = n[3]; out[4] = 0; return; }
    out[0] = n[0]; out[1] = n[1]; out[2] = '/';
    n = char_name[b];
    out[3] = n[0]; out[4] = n[1]; out[5] = 0;
}

/* Noms des compétitions (banque fixe : lus par les écrans et le palmarès) */
const char * const comp_name[5] = { "TOURNOI", "POULES", "CHAMPIONNAT", "SAISON", "PERSONNALISE" };
