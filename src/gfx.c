/* Royal Martinot — affichage : palettes, sprites, texte. */
#include "gfx.h"
#include <string.h>
#include "assets.h"
#include "app.h"
#include "audio.h"

const char * const char_name[NCHARS] = {
    "ALEX", "MAYA", "LEON", "NINA", "OMAR", "ZOE ", "HUGO", "EMMA",
    "IVAN", "LINA", "TOM ", "JADE", "LUCA", "SARA", "NOAH", "INES", "MARGAUX",
    "MAX ", "LOLA", "PAPI", "TITOU" };
const u8 char_female[NCHARS] = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0 };
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
    { 0x27, 0x35, 0x37 },   /* MARGAUX : rose clair, cheveux blonds ($37 : le $28 de la palette NES tire sur le vert olive, 2.73) */
    { 0x07, 0x27, 0x0F },   /* MAX : orange */
    { 0x37, 0x00, 0x07 },   /* LOLA : anthracite, cheveux châtains */
    { 0x36, 0x30, 0x10 },   /* PAPI : blanc, cheveux gris */
    { 0x27, 0x0C, 0x07 },   /* TITOU : canard foncé, cheveux châtains */
};
const char * const style_name[NSTYLES] = {
    "EQUILIBRE", "FOND DE COURT", "PUISSANCE", "FILET",
    "DEFENSIF", "OFFENSIF", "TACTIQUE", "PLACEMENT", "FANTAISIE" };
const u8 char_style[NCHARS] = { 0, 1, 2, 3, 4, 5, 6, 7, 2, 3, 5, 4, 1, 7, 6, 0, 8, 2, 5, 4, 8 };

const char * const surf_name[NSURF] = {
    "TERRE BATTUE", "GAZON       ", "DUR         ", "MOQUETTE    ", "PLAGE       ", "GLACE       " };
const char * const env_name[NENV] = { "STADE", "CLUB ", "SALLE", "CLUB NUDISTE", "COURT MARGAUX", "CHATEAU", "PARC " };
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
/* couleur de la balle par court (vérifiée par tools/contrast.py) : jaune de
   vraie balle de tennis ($28, la teinte NES la plus proche) partout où elle
   se voit bien ; sur le sable jaune de la plage, couleur approchante la plus
   lisible : vert-jaune vif ($2A) */
static const u8 ball_col[NSURF + 1] = { 0x28, 0x28, 0x28, 0x28, 0x2A, 0x28, 0x28 };

static const u8 env_pal[NENV][3] = {
    { 0x37, 0x16, 0x0F },     /* stade : peau, gradins rouges, cheveux et rambarde noirs */
    { 0x1A, 0x09, 0x07 },     /* club : haies, arbres, troncs */
    { 0x00, 0x2A, 0x0F },     /* salle : murs, panneaux, projecteurs */
    { 0x1A, 0x27, 0x07 },     /* club nudiste : herbe, peau, cheveux/chapeaux/serviettes */
    { 0x35, 0x24, 0x30 },     /* court Margaux : rose pâle, coeurs, fleurs et étoiles */
    { 0x10, 0x00, 0x16 },     /* château : pierre, joints, bannières et flammes */
    { 0x19, 0x27, 0x07 },     /* parc (décor du club) : herbe, feuilles d'automne, troncs */
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
    /* palette 3 du décor : chaise d'arbitre et spectateurs du club, de la
       salle, du château et du parc ; peau beige $37 (le jaune $28 tirait sur
       le vert, surtout avec certaines palettes d'émulateur) */
    pal_buf[12] = p[0]; pal_buf[13] = 0x0F; pal_buf[14] = 0x37; pal_buf[15] = 0x30;
    /* sprites : 0/1 joueurs (définis à part), 2 balle, 3 effets */
    pal_buf[16 + 8] = p[0];
    /* balle jaune pâle, sauf sur le sable jaune de la plage : blanche */
    /* sprites, palette 2 : arbitre (peau beige $37 : le jaune pâle $38 tirait
       sur le vert), ombres, poussière */
    pal_buf[16 + 9] = 0x37; pal_buf[16 + 10] = 0x0F; pal_buf[16 + 11] = 0x30;
    /* effets : 1 couleur de la balle (selon le court), 2 bleu (pluie,
       sueur), 3 blanc (couture de la balle, étoiles) */
    pal_buf[16 + 12] = p[0]; pal_buf[16 + 13] = ball_col[env == ENV_MARGAUX ? NSURF : s]; pal_buf[16 + 14] = 0x21; pal_buf[16 + 15] = 0x30;
    if (env == ENV_MARGAUX) pal_buf[16 + 11] = 0x34;   /* robe rose de la princesse arbitre (et reflet de la balle) */
    if (cond == 2) {
        /* pluie : ciel couvert, couleurs ternies */
        pal_buf[0] = darken(pal_buf[0]);
        for (i = 4; i < 16; i += 4) pal_buf[i] = pal_buf[0];
        pal_buf[1] = darken(pal_buf[1]);
        pal_buf[16 + 8] = pal_buf[16 + 12] = pal_buf[0];
        pal_buf[16 + 14] = 0x31;   /* gouttes et éclaboussures bleu clair */
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

/* fond des menus (version 2.59) : tiré au hasard à l'allumage parmi les
   couleurs des surfaces (terre battue, gazon, dur, moquette), comme le fond
   de l'affiche VS */
#pragma bss-name (push, "WBSS")
u8 menu_bk;
#pragma bss-name (pop)
/* (menu_bk_pick : screens.c, version 2.73) */
void pal_set_menu(void)
{
    /* fond vert sombre (gazon) ; ombre du texte noire (couleur 1), texte surligné
       (couleur 2), texte (couleur 3) : 0 blanc, 1 libellés bleu pâle (surlignés
       en or), 2 valeurs jaunes (surlignées en blanc), 3 or (titres, filets) */
    static const u8 mp[16] = { 0x0A, 0x0F, 0x28, 0x30,  0x0A, 0x0F, 0x28, 0x3C,
                               0x0A, 0x0F, 0x30, 0x28,  0x0A, 0x07, 0x38, 0x28 };
    u8 i;
    for (i = 0; i < 16; ++i) pal_buf[i] = (i & 3) ? mp[i] : menu_bk;
    if (menu_bk == 0x07) pal_buf[13] = 0x0F;          /* fond terre battue : ombre de l'or en noir */
    pal_buf[16 + 8] = 0x0F; pal_buf[16 + 9] = 0x38; pal_buf[16 + 10] = 0x00; pal_buf[16 + 11] = 0x30;
    pal_buf[16 + 12] = 0x0F; pal_buf[16 + 13] = 0x30; pal_buf[16 + 14] = 0x21; pal_buf[16 + 15] = 0x28;
    pal_buf[16] = pal_buf[20] = menu_bk;
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

void spr_begin(void) { oam_i = spr_rot; }

/* (x, y) = position des pieds ; sprite 16x24 dessiné par spr_meta (asm) */
/* Direction de course mémorisée par personnage (16 personnages distincts
 * dans un match) : 0 face/dos, 1 profil vers la droite, 2 profil vers la
 * gauche. Elle n'est mise à jour que si le joueur s'est déplacé, pour éviter
 * le clignotement quand l'IA ne bouge qu'une trame sur deux. */
/* spr_person : en assembleur (src/sprp.s), c'était la fonction la plus
   coûteuse de chaque trame. Les tableaux de course y sont définis. */
extern u8 run_dir[NCHARS];

u8 txt_off;     /* 0, ou $40 : variante surlignée de la police ($60-$9F, menus) */
void txt_at(u8 x, u8 y, const char *s)
{
    u8 c;
    vram_adr(NTADR(x, y));
    while ((c = (u8)*s++)) vram_put((u8)(c > ' ' && c < 0x60 ? c + txt_off : c));
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
        u8 c = (u8)(*s ? *s++ : ' ');
        vram_buf[i++] = (u8)(c > ' ' && c < 0x60 ? c + txt_off : c);
    }
    vram_buf[i] = 0xFF;
    vram_len = i;
}

/* vbuf_raw : en assembleur (fastmove.s, version 2.66) */

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

void chr_copy(u8 bank, const u8 *src, u16 dst, u16 len)
{
    /* copie rapide en assembleur (tz_copy, crt0.s), par blocs de 256 octets */
    vram_adr(dst);
    tz_bank = bank;
    while (len >= 256) { tz_n = 0; tz_copy(src); src += 256; len -= 256; }
    if (len) { tz_n = (u8)len; tz_copy(src); }
}

/* vue en perspective : décompression d'un court et copie des tables de
   projection depuis la banque 11 (appelées par load_court, banque 3) */
extern u8 persp;
#pragma zpsym("persp")
/* proj_ram : déclaré dans gfx.h */
u8 bg_dirty;                    /* tuiles de décor remplacées pour le match */
void bank_unrle(u8 bank, const u8 *rle)
{
    u8 old = cur_bank;
    set_bank(bank);
    vram_unrle(rle);
    set_bank(old);
}
void bank_read(u8 bank, void *dst, const void *src, u16 n)
{
    u8 old = cur_bank;
    set_bank(bank);
    memcpy(dst, src, n);
    set_bank(old);
}

/* tuiles prêtées au match (arbitre : tuiles de la coupe ; en perspective,
   tuiles du court) : dessin d'origine, rendu coupé */
void swap_restore(void);           /* banque 3 (extras.c) */
void chr_borrow_restore(void)
{
    persp = 0;
    bank_call(BANK_EXTRAS, swap_restore);   /* célébrations, tenues normales (si besoin) */
    /* 2 : seules les tuiles des menus ont changé ($60-$FB) : il suffit de
       rétablir le logo ($CE-$FB), le reste est réécrit par chaque menu */
    if (bg_dirty == 2) chr_copy(BANK_CHR, chr_data + 0xCE0, 0x0CE0, 0x320);
    else if (bg_dirty) chr_copy(BANK_CHR, chr_data, 0x0000, 4096);
    bg_dirty = 0;
    chr_copy(BANK_CHR, chr_data + 0x1000 + ((u16)trophy_tile << 4), 0x1000 + ((u16)trophy_tile << 4), 64);
    chr_copy(BANK_CHR, chr_data + 0x1000 + (11 << 4), 0x1000 + (11 << 4), 48);   /* arbitre (princesse au court Margaux) */
}

/* motifs d'origine (banque 2) dans toute la CHR-RAM, rendu coupé : après le
   podium, qui fabrique ses tuiles agrandies à la place d'autres */
void chr_reload(void)
{
    chr_copy(BANK_CHR, chr_data, 0x0000, 8192);
    chr_nude = 0;
}

/* les tuiles des personnages ont été remplacées (portraits des réglages) :
   rechargées au prochain people_tiles() */
void people_dirty(void) { chr_nude = 0xFE; }

/* (spr_port : screens.c, version 2.73) */

void people_tiles(u8 nude)
{
    if (nude == chr_nude) return;
    if (!nude && chr_nude == 0xFE) {          /* seulement les tuiles des portraits et vignettes */
        chr_copy(BANK_CHR, chr_data + 0x1100, 0x1100, 992);
    } else if (nude) {
        chr_copy(BANK_NUDE, nude_people, 0x1100, people_bytes);
    } else {
        chr_copy(BANK_CHR, chr_data + 0x1100, 0x1100, people_bytes);
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
/* comp_name : banque des écrans (screens.c) ; le podium a sa copie */

/* spr_ball (balle et ombre) : en assembleur, src/sprp.s */

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
