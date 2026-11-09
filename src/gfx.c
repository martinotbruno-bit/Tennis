/* Royal Martinot — affichage : palettes, sprites, texte. */
#include "gfx.h"
#include "assets.h"
#include "app.h"
#include "audio.h"

const char * const char_name[NCHARS] = {
    "ALEX", "MAYA", "LEON", "NINA", "OMAR", "ZOE ", "HUGO", "EMMA",
    "IVAN", "LINA", "TOM ", "JADE", "LUCA", "SARA", "NOAH", "INES", "MARGAUX" };
const u8 char_female[NCHARS] = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1 };
/* peau, tenue, sombre (cheveux/short/raquette) */
static const u8 char_pal[NCHARS][3] = {
    /* tenues choisies pour ressortir sur toutes les surfaces et se distinguer
       de la peau ; cheveux / short / raquette en noir ou brun foncé */
    { 0x27, 0x30, 0x0F },   /* ALEX : blanc (tenue classique) */
    { 0x37, 0x1C, 0x0F },   /* MAYA : bleu canard */
    { 0x17, 0x3B, 0x0F },   /* LEON : vert menthe */
    { 0x27, 0x24, 0x0F },   /* NINA : magenta */
    { 0x07, 0x3C, 0x0F },   /* OMAR : cyan clair */
    { 0x36, 0x0C, 0x0F },   /* ZOE  : canard foncé */
    { 0x37, 0x1C, 0x07 },   /* HUGO : bleu canard */
    { 0x27, 0x2C, 0x0F },   /* EMMA : cyan */
    { 0x36, 0x10, 0x0F },   /* IVAN : gris (le lilas était trop proche du rose de Margaux) */
    { 0x17, 0x3A, 0x0F },   /* LINA : vert pâle */
    { 0x27, 0x37, 0x0F },   /* TOM  : crème (le lavande était trop proche du rose de Margaux) */
    { 0x36, 0x2B, 0x0F },   /* JADE : vert */
    { 0x37, 0x39, 0x0F },   /* LUCA : vert anis */
    { 0x17, 0x31, 0x0F },   /* SARA : bleu clair */
    { 0x07, 0x2C, 0x0F },   /* NOAH : cyan */
    { 0x27, 0x07, 0x0F },   /* INES : chocolat (idem) */
    { 0x27, 0x35, 0x28 },   /* MARGAUX : rose clair, cheveux blonds (blond clair $38 sur la plage) */
};
const char * const style_name[NSTYLES] = {
    "EQUILIBRE", "FOND DE COURT", "PUISSANCE", "FILET",
    "DEFENSIF", "OFFENSIF", "TACTIQUE", "PLACEMENT", "FANTAISIE" };
const u8 char_style[NCHARS] = { 0, 1, 2, 3, 4, 5, 6, 7, 2, 3, 5, 4, 1, 7, 6, 0, 8 };

const char * const surf_name[NSURF] = {
    "TERRE BATTUE", "GAZON       ", "DUR         ", "MOQUETTE    ", "PLAGE       ", "GLACE       " };
const char * const env_name[NENV] = { "STADE", "CLUB ", "SALLE", "CLUB NUDISTE", "COURT MARGAUX" };
/* fond universel, surface, lignes, filet */
static const u8 surf_pal[NSURF + 1][4] = {
    /* couleurs choisies pour se distinguer des 5 teintes de peau, des tenues
       et des cheveux, de jour, de nuit et sous la pluie (tools/contrast.py) */
    { 0x05, 0x26, 0x30, 0x0F },   /* terre battue : brique clair, abords lie-de-vin */
    { 0x1A, 0x2A, 0x30, 0x0F },   /* gazon : vert clair, abords vert */
    { 0x02, 0x12, 0x30, 0x0F },   /* dur : bleu, abords bleu nuit */
    { 0x03, 0x13, 0x30, 0x0F },   /* moquette : violet, abords violet sombre */
    { 0x22, 0x28, 0x16, 0x0F },   /* plage : sable jaune, mer */
    { 0x12, 0x21, 0x30, 0x0F },   /* glace : bleu ciel, abords bleu */
    { 0x14, 0x25, 0x30, 0x0F },   /* court Margaux : rose bonbon, abords prune */
};
/* palette 2 (bords du court) selon le lieu */
static const u8 env_pal[NENV][3] = {
    { 0x27, 0x16, 0x37 },     /* stade : public */
    { 0x1A, 0x09, 0x07 },     /* club : haies, arbres, troncs */
    { 0x00, 0x2A, 0x0F },     /* salle : murs, panneaux, projecteurs */
    { 0x1A, 0x27, 0x07 },     /* club nudiste : herbe, peau, cheveux/chapeaux/serviettes */
    { 0x35, 0x24, 0x30 },     /* court Margaux : rose pâle, coeurs, fleurs et étoiles */
};

static u8 darken(u8 c)
{
    /* un cran plus sombre, sans jamais tomber au noir : le noir est la couleur
       des cheveux et des raquettes, qui doivent rester visibles */
    if (c == 0x0F || c == 0x30) return c;
    return (c >= 0x10) ? (u8)(c - 0x10) : c;
}

void pal_set_court(u8 s, u8 cond, u8 env)
{
    /* court Margaux : toujours rose, quelle que soit la surface (qui garde
       son rebond) */
    const u8 *p = surf_pal[env == ENV_MARGAUX ? NSURF : s];
    const u8 *e = env_pal[env];
    u8 i;
    pal_buf[0] = p[0]; pal_buf[1] = p[1]; pal_buf[2] = p[2]; pal_buf[3] = p[3];
    pal_buf[4] = p[0]; pal_buf[5] = 0x0F; pal_buf[6] = 0x00; pal_buf[7] = 0x30;  /* score */
    pal_buf[8] = p[0]; pal_buf[9] = e[0]; pal_buf[10] = e[1]; pal_buf[11] = e[2]; /* bords */
    pal_buf[12] = p[0]; pal_buf[13] = 0x0F; pal_buf[14] = 0x28; pal_buf[15] = 0x30;
    /* sprites : 0/1 joueurs (définis à part), 2 balle, 3 effets */
    pal_buf[16 + 8] = p[0];
    /* balle jaune pâle, sauf sur le sable jaune de la plage : blanche */
    pal_buf[16 + 9] = (u8)(p == surf_pal[4] ? 0x30 : 0x38); pal_buf[16 + 10] = 0x0F; pal_buf[16 + 11] = 0x30;
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
        /* match de nuit : décors assombris (sans noir), court éclairé */
        pal_buf[0] = darken(pal_buf[0]);
        for (i = 4; i < 16; i += 4) pal_buf[i] = pal_buf[0];
        pal_buf[10] = darken(pal_buf[10]); pal_buf[11] = darken(pal_buf[11]); pal_buf[9] = darken(pal_buf[9]);
        pal_buf[16 + 8] = pal_buf[16 + 12] = pal_buf[0];
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
    /* Margaux sur le sable jaune ($28, $18 sous la pluie) : cheveux blond clair */
    if (c == 16 && (pal_buf[1] == 0x28 || pal_buf[1] == 0x18)) pal_buf[b + 3] = 0x38;
    pal_dirty = 1;
}

void spr_begin(void) { oam_i = spr_rot; }

/* (x, y) = position des pieds ; sprite 16x24 dessiné par spr_meta (asm) */
/* Direction de course mémorisée par personnage (16 personnages distincts
 * dans un match) : 0 face/dos, 1 profil vers la droite, 2 profil vers la
 * gauche. Elle n'est mise à jour que si le joueur s'est déplacé, pour éviter
 * le clignotement quand l'IA ne bouge qu'une trame sur deux. */
/* spr_person : en assembleur (src/sprp.s), c'était la fonction la plus
   coûteuse de chaque trame. Les tableaux de course y sont définis. */
extern u8 run_dir[NCHARS];

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
    if (nude) {
        chr_copy(BANK_NUDE, nude_people, 0x1100, 3072);
        chr_copy(BANK_NUDE, nude_profile, 0x1000 + ((u16)profile_base << 4), 384);
    } else {
        chr_copy(BANK_CHR, chr_data + 0x1100, 0x1100, 3072);
        chr_copy(BANK_CHR, chr_data + 0x1000 + ((u16)profile_base << 4), 0x1000 + ((u16)profile_base << 4), 384);
    }
    chr_nude = nude;
}

/* Nom d'équipe : « ALEX » en simple, « AL/MA » en double (b = 255 : simple) */
void team_label(u8 a, u8 b, char *out)
{
    const char *n = char_name[a];
    if (b == 255) { out[0] = n[0]; out[1] = n[1]; out[2] = n[2]; out[3] = n[3]; out[4] = 0; return; }
    /* 2 lettres par joueur ; nom long (MARGAUX) : première et dernière
       lettres (« MX »), pour ne pas se confondre avec MAYA (« MA ») */
    out[0] = n[0]; out[1] = n[4] > ' ' ? n[6] : n[1]; out[2] = '/';
    n = char_name[b];
    out[3] = n[0]; out[4] = n[4] > ' ' ? n[6] : n[1]; out[5] = 0;
}

/* Noms des compétitions (banque fixe : lus par les écrans et le palmarès) */
const char * const comp_name[5] = { "TOURNOI", "POULES", "CHAMPIONNAT", "SAISON", "PERSONNALISE" };

/* Balle et ombre. La balle grossit avec la hauteur (zpx, en pixels) : effet
 * de perspective qui aide à lire les lobs et les rebonds. (x, sy) = position
 * affichée de la balle, shy = position de l'ombre au sol. */
void spr_ball(u8 x, s16 sy, u8 shy, u8 zpx)
{
    u8 t = 1;
    if (zpx >= 30) { t = ball_big_tile; --x; --sy; }
    else if (zpx >= 12) t = ball_mid_tile;
    /* balle au-dessus du haut de l'écran : seule l'ombre reste visible */
    if (sy >= 0) spr1((u8)(x - 3), (u8)sy, t, 2);
    spr1((u8)(x - 3), (u8)(shy - 3), 2, 2);
}

/* Styles de jeu de l'IA (CDC 10.2), indexés par char_style[] : équilibré,
 * fond de court, puissance, filet, défensif, offensif, tactique, placement.
 * Utilisés par le moteur de match (banque 0). */
/* 9e style, FANTAISIE (Margaux) : varie tout, adore l'amortie et le lob,
   monte volontiers au filet */
const s8 sty_pow[9]    = { 0, -2, 10, 0, -4, 6, 0, -2, 2 };
const s8 sty_margin[9] = { 0, 4, 0, 0, 8, -3, 0, -4, 0 };
const s8 sty_spread[9] = { 0, -3, 4, 0, -4, 2, 0, -5, 0 };
const u8 sty_net[9]    = { 40, 0, 30, 220, 0, 110, 60, 40, 120 };   /* montée au filet /256 */
const u8 sty_lob[9]    = { 64, 48, 16, 32, 128, 32, 144, 48, 160 }; /* lob contre volleyeur */
const u8 sty_drop[9]   = { 24, 16, 0, 16, 0, 40, 96, 40, 128 };     /* amortie */
const u8 sty_slice[9]  = { 64, 48, 16, 64, 96, 16, 96, 64, 112 };   /* coupé */
const u8 sty_deep[9]   = { 8, 14, 10, 8, 18, 4, 8, 8, 6 };          /* recul au replacement */
