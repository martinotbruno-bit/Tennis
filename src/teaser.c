/* Royal Martinot — bande-annonce du Grand Martinot et du Martinworld Tour
 * (banque 7, versions 2.56 et 2.57), au lancement de la compétition, sur un
 * riff de rock (MUS_ROCK), environ 6 secondes :
 *  - 7 plans de personnages dessinés en haute définition (tools/gen_teaser.py :
 *    jusqu'à 225 tuiles par image, visage, ombres, raquette cordée) : la
 *    frappe (gros plan, la balle arrive, flash à l'impact), la glissade sur
 *    terre battue (en pied puis zoom, gerbe de poussière, l'écran tremble),
 *    les bras levés au ciel (en pied puis zoom sur le buste, confettis), la
 *    raquette fracassée au sol (choc, éclats, puis zoom brutal sur le visage
 *    qui crie) ;
 *  - caméra : travellings éclair d'un plan à l'autre (défilement horizontal
 *    sur deux écrans), lente dérive pendant le plan, tremblements ; les zooms
 *    sont des coupes avec un éclair blanc ;
 *  - puis le titre, façon cinéma : sur fond noir, entre deux filets dorés, le
 *    nom de la compétition apparaît lettre par lettre (lettres doublées),
 *    chacune avec un éclair ; un reflet doré le parcourt ; le sous-titre suit.
 * A ou START la passe ; cer_skip (banc d'essai) aussi. Les tuiles du décor
 * sont rechargées ensuite (chr_reload, screens.c). Variables : RAM de
 * cartouche libre (bloc SAVEDATA, non sauvegardé). */
#include "app.h"
#include "gfx.h"
#include "audio.h"
#include "assets.h"
#include "comp.h"
#include "assets_tz.h"

#pragma static-locals(off)

#define PPU_DATA (*(volatile u8 *)0x2007)
#define T_STREAK 0xF8          /* fond uni (couleur 0) */
#define T_GROUND 0xF9          /* sol (couleur 3) */
#define T_LINE   0xFA          /* filet doré (couleur 3) */
#define T_TITLE  0x60          /* titre : lettres doublées, 4 tuiles chacune */
#define NCUT     7

#pragma bss-name(push, "SAVEDATA")
u8 teaser_done;
static u8 ts_shot, ts_t, ts_n, ts_sh, ts_fx, ts_cut, ts_c, ts_bk;
static s16 ts_cam;
static u8 ts_buf[16];
#pragma bss-name(pop)
extern u8 cer_skip;            /* (cer.c) : le banc d'essai passe les séquences */

/* les plans : image, durée (trames), groupe (couleurs, effets, personnage),
   entrée en travelling éclair (1) ou coupe de zoom avec éclair blanc (0) */
static const u8 cut_art[NCUT] = { TZ_IMPACT, TZ_SLIDE, TZ_SLIDE_Z, TZ_VICTORY, TZ_VICTORY_Z, TZ_ANGRY, TZ_ANGRY_Z };
static const u8 cut_len[NCUT] = { 40, 18, 24, 16, 26, 20, 26 };
static const u8 cut_shot[NCUT] = { 0, 1, 1, 2, 2, 3, 3 };
static const u8 cut_whip[NCUT + 1] = { 1, 1, 0, 1, 0, 1, 0, 1 };
static const u8 shot_col[4][4] = {
    /* fond, bandes (sombres, discrètes : le fond autour du personnage, sans
       bandes à cause des blocs de palette 16 x 16, ne fait pas de halo), sol */
    { 0x01, 0x0F, 0x02, 0x11 },    /* nuit bleue, court dur */
    { 0x07, 0x0F, 0x08, 0x17 },    /* terre battue */
    { 0x09, 0x0F, 0x0A, 0x19 },    /* gazon */
    { 0x05, 0x0F, 0x04, 0x15 },    /* colère : rouge sombre */
};
/* travelling éclair : entrée (le personnage arrive de la droite), sortie (il
   part à gauche) ; décalage de la caméra en pixels */
static const u8 whip_in[6] = { 150, 90, 48, 22, 8, 2 };
static const u8 whip_out[5] = { 6, 20, 48, 96, 160 };

/* tuile unie (ou motif) de 8 octets par plan */
static void tile_bits(u8 t, const u8 *p0, const u8 *p1)
{
    u8 i;
    vram_adr((u16)t << 4);
    for (i = 0; i < 8; ++i) vram_put(p0[i]);
    for (i = 0; i < 8; ++i) vram_put(p1[i]);
}

/* fond uni : la NES colore l'écran par blocs de 16 x 16 ; ceux qui touchent
   le personnage prennent ses palettes, des bandes de couleur y feraient une
   démarcation autour de lui */
static const u8 z8[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const u8 ff[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static const u8 ln[8] = { 0, 0, 0, 0xFF, 0xFF, 0, 0, 0 };

/* fond d'un écran : bandes et sol (en pied) ou noir (zoom : bandes noires) */
static void ts_back(u16 nt, u8 zoom)
{
    u8 i;
    vram_adr(nt);
    for (i = 0; i < 30; ++i)
        vram_fill(zoom ? 0 : (i >= TZ_GROUND_ROW ? T_GROUND : T_STREAK), 32);
}

static u8 ts_zoom(void)
{
    return tz_art[cut_art[ts_cut]][0].zoom;
}

/* un plan : image en haute définition (rendu coupé, environ 3 trames ; noir,
   ou blanc pour une coupe de zoom) */
static void cut_build(void)
{
    u8 i, S, O, D, z;
    const tz_art_t *a;
    const u8 *col;
    ts_shot = cut_shot[ts_cut];
    col = shot_col[ts_shot];
    ts_c = ts_shot == 0 || ts_shot == 2 ? c_pl[0] : c_pl[ts_shot == 1 ? 1 : 2];
    a = &tz_art[cut_art[ts_cut]][char_female[ts_c] ? 1 : 0];
    z = a->zoom;
    pal_buf[0] = pal_buf[16] = cut_whip[ts_cut] ? col[0] : 0x30;   /* éclair blanc du zoom */
    pal_dirty = 1;
    fade_skip = 1; fade_lvl = 0;     /* coupe franche (cinéma), sans fondu */
    ppu_off();
    /* tuiles : 0 vide, puis l'image (décompressée), bandes et sol */
    vram_adr(0x0000);
    vram_fill(0, 16);
    tz_bank = a->bank; tz_n = a->ntile;
    tz_unpack(a->chr);
    tile_bits(T_STREAK, z8, z8);
    tile_bits(T_GROUND, ff, ff);
    ts_back(0x2000, z);
    ts_back(0x2400, z);
    {
        const u8 *m = a->map;
        u8 w = a->w, h = a->h;
        u16 ad = NTADR(a->x0, a->y0);
        for (i = 0; i < h; ++i) {
            vram_adr(ad); ad += 32;
            tz_n = w; tz_copy(m); m += w;
        }
    }
    vram_adr(0x23C0);
    tz_n = 64; tz_copy(a->at);
    vram_adr(0x27C0); vram_fill(0, 64);
    /* palettes : P0 bandes (ou peau, sombre, blanc en zoom), P1 peau et son
       ombre, P2 tenue et son ombre, P3 peau, tenue, blanc (en zoom : peau,
       son ombre, tenue) */
    pal_set_player(61, ts_c);                 /* (P1 : peau, tenue, sombre) */
    S = pal_buf[5]; O = pal_buf[6]; D = pal_buf[7];
    /* zoom sur la joie : fond rouge sombre (la bouche ouverte, de la couleur du fond, reste naturelle) */
    ts_bk = cut_art[ts_cut] == TZ_VICTORY_Z ? 0x06 : col[0];
    pal_buf[0] = pal_buf[4] = pal_buf[8] = pal_buf[12] = pal_buf[16] = ts_bk;
    if (S >= 0x10) { pal_buf[5] = S; pal_buf[6] = (u8)(S - 0x10); } else { pal_buf[5] = (u8)(S + 0x10); pal_buf[6] = S; }
    if (O >= 0x10) { pal_buf[9] = O; pal_buf[10] = (u8)(O - 0x10); } else { pal_buf[9] = (u8)(O + 0x10); pal_buf[10] = O; }
    if (O == 0x30) pal_buf[10] = 0x10;
    pal_buf[7] = pal_buf[11] = D;
    pal_buf[13] = pal_buf[5]; pal_buf[14] = pal_buf[9]; pal_buf[15] = 0x30;
    if (z) { pal_buf[1] = pal_buf[5]; pal_buf[2] = D; pal_buf[3] = 0x30; pal_buf[14] = pal_buf[6]; pal_buf[15] = pal_buf[9]; }
    else { pal_buf[1] = col[1]; pal_buf[2] = col[2]; pal_buf[3] = col[3]; }
    pal_buf[16 + 8] = col[0]; pal_buf[16 + 9] = 0x37; pal_buf[16 + 10] = 0x0F; pal_buf[16 + 11] = 0x30;   /* poussière, éclats */
    pal_buf[16 + 12] = col[0]; pal_buf[16 + 13] = 0x38; pal_buf[16 + 14] = 0x27; pal_buf[16 + 15] = 0x30;   /* balle, confettis */
    pal_dirty = 1;
    ts_t = 0; ts_sh = 0; ts_fx = 0;
    ts_cam = cut_whip[ts_cut] ? -(s16)whip_in[0] : 0;
    scroll_x = (u8)ts_cam; scroll_nt = (u8)((ts_cam >> 8) & 1); scroll_y = 0;
    ppu_on();
}

static const char * const ts_name[2] = { "LE GRAND MARTINOT", "MARTINWORLD TOUR" };
static const char * const ts_sub[2] = { "4 TOURNOIS - LE GRAND CHELEM", "7 TOURNOIS - TOUTES LES SURFACES" };
static void title_build(void)
{
    u8 i, k, ch, b;
    const char *n = ts_name[c_type == CT_WORLD];
    fade_skip = 1; fade_lvl = 0;     /* coupe franche (cinéma), sans fondu */
    ppu_off();
    /* la police ($20-$5F : l'image avait pris toutes les tuiles du décor) */
    vram_adr(0x0200); tz_bank = BANK_CHR;
    for (i = 0; i < 4; ++i) { tz_n = 0; tz_copy(chr_data + 0x200 + ((u16)i << 8)); }
    scroll_y = 0; scroll_x = 0; scroll_nt = 0;
    tile_bits(T_LINE, ln, ln);
    vram_adr(0x2000);
    for (i = 0; i < 30; ++i) vram_fill(0, 32);
    vram_adr(0x23C0); vram_fill(0, 64);
    pal_buf[4] = pal_buf[8] = pal_buf[12] = 0x0F;
    /* lettres doublées : 4 tuiles par lettre ($60 et suivantes) */
    for (k = 0; n[k]; ++k) {
        ch = (u8)n[k];
        vram_adr((u16)ch << 4);
        ts_buf[0] = PPU_DATA;
        for (i = 0; i < 16; ++i) ts_buf[i] = PPU_DATA;
        vram_adr((u16)(T_TITLE + k * 4) << 4);
        for (b = 0; b < 4; ++b) {
            /* tuile b : haut-gauche, haut-droite, bas-gauche, bas-droite */
            u8 r, half = (u8)((b & 1) ? 0 : 4), top = (u8)((b & 2) ? 4 : 0), v, o;
            for (i = 0; i < 2; ++i)
                for (r = 0; r < 8; ++r) {
                    v = ts_buf[i * 8 + top + (r >> 1)];
                    v = (u8)(half ? v >> 4 : v & 15);
                    o = 0;
                    if (v & 8) o |= 0xC0;
                    if (v & 4) o |= 0x30;
                    if (v & 2) o |= 0x0C;
                    if (v & 1) o |= 0x03;
                    vram_put(o);
                }
        }
    }
    ts_n = k;
    vram_adr(NTADR(2, 10)); vram_fill(T_LINE, 28);
    vram_adr(NTADR(2, 18)); vram_fill(T_LINE, 28);
    pal_buf[0] = pal_buf[16] = 0x0F; pal_buf[1] = 0x0F; pal_buf[2] = 0x0F; pal_buf[3] = 0x28;
    pal_dirty = 1;
    ts_t = 0; ts_cut = NCUT;
    ppu_on();
}

void teaser_build(void)
{
    teaser_done = 0;
    ts_cut = 0;
    music_play(MUS_ROCK);
    cut_build();
}

/* à chaque trame */
void teaser_tick(void)
{
    u8 i, x, y, k;
    const u8 *col;
    if (cer_skip || (ts_t > 4 && (pad1_new & (PAD_A | PAD_START)))) {
        scroll_y = 0; scroll_x = 0; scroll_nt = 0; teaser_done = 1; music_play(MUS_NONE); return;
    }
    ++ts_t;
    spr_begin();
    if (ts_cut < NCUT) {
        u8 len = cut_len[ts_cut], z = ts_zoom();
        s16 sx;
        col = shot_col[ts_shot];
        /* caméra : entrée en travelling éclair, dérive lente, sortie éclair
           si le plan suivant arrive aussi en travelling */
        if (cut_whip[ts_cut] && ts_t < 6) ts_cam = -(s16)whip_in[ts_t];
        else if (cut_whip[ts_cut + 1] && ts_t + 5 >= len) ts_cam = (s16)whip_out[ts_t + 5 - len] + (len >> 2);
        else ts_cam = (s16)(ts_t >> 2);
        scroll_x = (u8)ts_cam; scroll_nt = (u8)((ts_cam >> 8) & 1);
        /* après un flash, le fond revient */
        if ((ts_t & 3) == 0) {
            pal_buf[0] = pal_buf[4] = pal_buf[8] = pal_buf[12] = pal_buf[16] = ts_bk;
            if (!z) { k = (u8)((ts_t >> 2) & 1); pal_buf[1] = col[1 + k]; pal_buf[2] = col[2 - k]; }
            pal_dirty = 1;
        }
#define SPR(px, py, t, at) { sx = (s16)(px) - ts_cam; if (sx >= 0 && sx < 248) spr1((u8)sx, (u8)(py), (u8)(t), (u8)(at)); }
        switch (ts_cut) {
        case 0:
            /* la frappe : la balle arrive en diagonale sur le tamis, impact
               (flash blanc), puis elle part en traînée vers la gauche */
            if (ts_t >= 8 && ts_t < 22) {
                k = (u8)(22 - ts_t);
                SPR(TZ_IMPACT_RACKET_X + k * 9, TZ_IMPACT_RACKET_Y - 4 + k * 5, ball_base + 8, 3);
            } else if (ts_t == 22) {
                pal_buf[0] = pal_buf[4] = pal_buf[8] = pal_buf[12] = pal_buf[16] = 0x30; pal_dirty = 1; sfx_hit(3);
            } else if (ts_t > 22 && ts_t < 32) {
                for (i = 0; i < 3; ++i) {
                    k = (u8)((ts_t - 22) * 10 - i * 6);
                    if (k < 40) SPR(TZ_IMPACT_RACKET_X + 4 - k, TZ_IMPACT_RACKET_Y - 8 - (k >> 1), ball_base + 8, 3);
                }
            }
            break;
        case 1: case 2:
            /* glissade : gerbe de terre battue depuis le pied, l'écran tremble au zoom */
            for (i = 0; i < 7; ++i) {
                x = (u8)((ts_t * (i + 2) + i * 9) & 31);
                y = (u8)(((ts_t * (i + 1)) & 15) + i * 3);
                if (ts_cut == 1) { SPR(TZ_SLIDE_FOOTR_X + x - 4, TZ_SLIDE_FOOTR_Y - 6 - y, 6 + (i & 1), 2); }
                else { SPR(TZ_SLIDE_Z_FOOTR_X + x * 2 - 8, TZ_SLIDE_Z_FOOTR_Y - 10 - y * 2, 6 + (i & 1), 2); }
            }
            if (ts_cut == 2) scroll_y = (u8)((ts_t & 2) ? 2 : 0);
            if (ts_cut == 1 && ts_t == 2) sfx_crowd(4);
            break;
        case 3: case 4:
            /* bras levés : pluie de confettis, ovation */
            for (i = 0; i < 12; ++i)
                spr1((u8)(i * 21 + 6), (u8)(((ts_t * (1 + (i & 1)) + i * 23 + (ts_cut == 4 ? 40 : 0)) & 0xBF) + 8), confetti_tile, (u8)(i & 3));
            if (ts_cut == 3 && ts_t == 2) sfx_crowd(7);
            break;
        case 5:
            /* raquette fracassée au sol : choc (flash), éclats, tremblement */
            if (ts_t == 8) { pal_buf[0] = pal_buf[4] = pal_buf[8] = pal_buf[12] = pal_buf[16] = 0x30; pal_dirty = 1; sfx_canon(); ts_sh = 6; }
            if (ts_t > 8)
                for (i = 0; i < 6; ++i) {
                    k = (u8)(ts_t - 8);
                    SPR(TZ_ANGRY_RACKET_TIP_X - 15 + i * 6 + k * (i - 2) * 2, TZ_ANGRY_RACKET_TIP_Y - 4 - k * 3 + (u8)(k * k / 4), ice_tile, 2);
                }
            break;
        default:
            /* zoom brutal sur le visage qui crie : l'écran tremble, s'amortit */
            if (ts_t == 1) { ts_sh = 7; sfx_crowd(2); }
            break;
        }
        if (ts_cut >= 5) {
            if (ts_sh) { scroll_y = (u8)((ts_t & 1) ? ts_sh : 0); if ((ts_t & 3) == 0) --ts_sh; } else scroll_y = 0;
        }
        spr_end();
        if (ts_t >= len) {
            scroll_y = 0;
            if (++ts_cut < NCUT) cut_build(); else title_build();
        }
        return;
    }
    /* titre : une lettre toutes les 5 trames, avec un éclair ; deux lignes
       (« LE GRAND » / « MARTINOT », « MARTINWORLD » / « TOUR ») centrées */
    if (ts_t < 90 && (ts_t % 5) == 1) {
        k = (u8)(ts_t / 5);
        if (k < ts_n) {
            const char *n = ts_name[c_type == CT_WORLD];
            u8 sp = ts_n, len, row;
            while (sp && n[sp - 1] != ' ') --sp;        /* début du dernier mot */
            if (k < sp) { len = (u8)(sp - 1); x = (u8)(((32 - len * 2) >> 1) + k * 2); row = 12; }
            else { len = (u8)(ts_n - sp); x = (u8)(((32 - len * 2) >> 1) + (k - sp) * 2); row = 15; }
            if (n[k] != ' ') {
                ts_buf[0] = (u8)(T_TITLE + k * 4); ts_buf[1] = (u8)(T_TITLE + k * 4 + 1);
                ts_buf[2] = (u8)(T_TITLE + k * 4 + 2); ts_buf[3] = (u8)(T_TITLE + k * 4 + 3);
                vbuf_raw(NTADR(x, row), ts_buf, 2);
                vbuf_raw(NTADR(x, row + 1), ts_buf + 2, 2);
                pal_buf[3] = 0x30; pal_dirty = 1;
                sfx_hit(1);
            }
        }
    } else if (pal_buf[3] == 0x30 && ts_t < 90) { pal_buf[3] = 0x28; pal_dirty = 1; }
    /* reflet doré qui parcourt le titre */
    if (ts_t >= 90 && ts_t < 115) { pal_buf[3] = (u8)((ts_t & 4) ? 0x38 : 0x30); pal_dirty = 1; }
    if (ts_t == 115) {
        const char *sb = ts_sub[c_type == CT_WORLD];
        for (k = 0; sb[k]; ++k) ;
        vbuf_text((u8)((32 - k) >> 1), 21, sb, k);
        pal_buf[3] = 0x28; pal_dirty = 1;
        sfx_crowd(7);
    }
    spr_end();
    if (ts_t >= 210) teaser_done = 1;      /* le titre reste affiché une seconde après le sous-titre complet */
}
