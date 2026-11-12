/* Royal Martinot — bande-annonce du Grand Martinot et du Martinworld Tour
 * (banque 7, version 2.56), au lancement de la compétition, sur un riff de
 * rock (MUS_ROCK), environ 6 secondes :
 *  - 4 plans en gros plan (personnages agrandis 4 fois dans le décor :
 *    96 tuiles), séparés par des coupes franches : la frappe (flash à
 *    l'impact, la balle part en traînée), la glissade sur terre battue
 *    (gerbe de poussière, l'écran tremble), les bras levés au ciel (pluie de
 *    confettis), la raquette frappée au sol (choc, l'écran tremble, éclats) ;
 *    au fond, des bandes obliques dont les couleurs tournent (vitesse) ;
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

#pragma static-locals(off)

#define PPU_DATA (*(volatile u8 *)0x2007)
#define T_BIG    0x60          /* personnage agrandi : 96 tuiles $60-$BF */
#define T_STREAK 0xC0          /* bandes obliques (couleurs 0, 1, 2) */
#define T_GROUND 0xC1          /* sol (couleur 3) */
#define T_LINE   0xC2          /* filet doré (couleur 3) */
#define T_TITLE  0x60          /* titre : lettres doublées, 4 tuiles chacune */
#define SHOT_T   40            /* trames par plan */
#define NSHOT    4

#pragma bss-name(push, "SAVEDATA")
u8 teaser_done;
static u8 ts_shot, ts_t, ts_n, ts_sh, ts_fx;
static u8 ts_buf[16], ts_exp[64];
#pragma bss-name(pop)
extern u8 cer_skip;            /* (cer.c) : le banc d'essai passe les séquences */

/* poses et décors des plans : frappe (impact), glissade (fente), bras levés
   (victoire, jeu F), raquette au sol (colère, jeu C) */
static const u8 shot_pose[NSHOT] = { POSE_IMPACT, POSE_REACH, POSE_VICTORY, POSE_ANGRY };
static const u8 shot_col[NSHOT][4] = {
    { 0x01, 0x11, 0x21, 0x12 },    /* nuit bleue, court dur */
    { 0x07, 0x17, 0x27, 0x16 },    /* terre battue */
    { 0x09, 0x19, 0x29, 0x1A },    /* gazon */
    { 0x05, 0x15, 0x25, 0x06 },    /* colère : rouge sombre */
};
static const u8 q4[4] = { 0x00, 0x0F, 0xF0, 0xFF };

/* tuile source (table des sprites) -> 16 tuiles agrandies 4 fois (décor).
   Chaque octet source est d'abord développé en 4 octets (2 pixels chacun,
   doublés 4 fois), sans décalage variable (lent sur 6502) */
static void big4_tile(u8 s, u8 dst)
{
    u8 i, oy, ox, r, b, *e;
    vram_adr(0x1000 + ((u16)s << 4));
    ts_buf[0] = PPU_DATA;                  /* lecture à vide (tampon du PPU) */
    for (i = 0; i < 16; ++i) ts_buf[i] = PPU_DATA;
    for (i = 0; i < 16; ++i) {
        b = ts_buf[i]; e = ts_exp + i * 4;
        e[0] = q4[b >> 6]; e[1] = q4[(b >> 4) & 3]; e[2] = q4[(b >> 2) & 3]; e[3] = q4[b & 3];
    }
    vram_adr((u16)dst << 4);
    for (oy = 0; oy < 4; ++oy)
        for (ox = 0; ox < 4; ++ox) {
            e = ts_exp + oy * 8 + ox;               /* rangée source 2 oy, colonne ox */
            for (r = 0; r < 4; ++r) vram_put(e[0]);
            for (r = 0; r < 4; ++r) vram_put(e[4]);
            e += 32;                                /* plan 2 */
            for (r = 0; r < 4; ++r) vram_put(e[0]);
            for (r = 0; r < 4; ++r) vram_put(e[4]);
        }
}
/* personnage c (de face) dans la pose donnée, agrandi 4 fois : 64 x 96 px,
   colonnes 12 à 19, rangées 8 à 19 */
static void big4_person(u8 c, u8 pose)
{
    u8 k, s, m = (u8)(person_row[1] + pose * 3), oy, ox;
    const u8 *map = char_female[c] ? person_map_f : person_map_m;
    for (k = 0; k < 6; ++k) {
        s = (u8)(map[m + (k >> 1)] + (k & 1));
        if (c == 16 && k < 2) s = (u8)(marg_map[NPOSE + pose] + k);   /* Margaux */
        big4_tile(s, (u8)(T_BIG + k * 16));
        for (oy = 0; oy < 4; ++oy) {
            vram_adr(NTADR(12 + (k & 1) * 4, 8 + (k >> 1) * 4 + oy));
            for (ox = 0; ox < 4; ++ox) vram_put((u8)(T_BIG + k * 16 + oy * 4 + ox));
        }
    }
}
/* tuile unie (ou motif) de 8 octets par plan */
static void tile_bits(u8 t, const u8 *p0, const u8 *p1)
{
    u8 i;
    vram_adr((u16)t << 4);
    for (i = 0; i < 8; ++i) vram_put(p0[i]);
    for (i = 0; i < 8; ++i) vram_put(p1[i]);
}

/* bandes obliques fines et espacées (couleurs 1 et 2) : le fond est surtout
   de la couleur 0, comme autour du personnage agrandi */
static const u8 st0[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
static const u8 st1[8] = { 0x08, 0x04, 0x02, 0x01, 0x80, 0x40, 0x20, 0x10 };
static const u8 ff[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static const u8 ln[8] = { 0, 0, 0, 0xFF, 0xFF, 0, 0, 0 };

static void shot_build(void)
{
    u8 i, c;
    const u8 *col = shot_col[ts_shot];
    fade_skip = 1; fade_lvl = 0;     /* coupe franche (cinéma), sans fondu */
    ppu_off();
    tile_bits(T_STREAK, st0, st1);
    tile_bits(T_GROUND, ff, ff);
    vram_adr(0x2000);
    for (i = 0; i < 30; ++i) vram_fill(i >= 20 ? T_GROUND : T_STREAK, 32);
    vram_adr(0x23C0);
    for (i = 0; i < 64; ++i) {
        /* blocs du personnage (colonnes 12-19, rangées 8-19) : palette 1 */
        u8 bx = (u8)((i & 7) * 4), by = (u8)((i >> 3) * 4), a = 0;
        if (bx >= 12 && bx < 20 && by >= 8 && by < 20) a = 0x55;
        vram_put(a);
    }
    c = ts_shot == 0 || ts_shot == 2 ? c_pl[0] : c_pl[ts_shot == 1 ? 1 : 2];
    if (ts_shot == 2) {
        /* bras levés : pose du jeu F (zone partagée), chargée pour l'agrandir */
        u8 base, n;
        bank_read(BANK_EXTRAS, &base, &swap_base, 1);
        bank_read(BANK_EXTRAS, &n, &swap_ntile[2], 1);
        chr_copy(BANK_SWAP, swap_chrF, 0x1000 + ((u16)base << 4), (u16)n << 4);
    }
    big4_person(c, shot_pose[ts_shot]);
    pal_buf[0] = col[0]; pal_buf[1] = col[1]; pal_buf[2] = col[2]; pal_buf[3] = col[3];
    pal_set_player(61, c);                   /* palette 1 du décor : le joueur */
    pal_buf[4] = pal_buf[16] = pal_buf[0];   /* ($3F10 recopie $3F00) */
    pal_buf[16 + 8] = col[0]; pal_buf[16 + 9] = 0x37; pal_buf[16 + 10] = 0x0F; pal_buf[16 + 11] = 0x30;   /* poussière, éclats */
    pal_buf[16 + 12] = col[0]; pal_buf[16 + 13] = 0x38; pal_buf[16 + 14] = 0x27; pal_buf[16 + 15] = 0x30;   /* balle, confettis */
    pal_dirty = 1;
    ts_t = 0; ts_sh = 0; ts_fx = 0;
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
    scroll_y = 0;
    tile_bits(T_LINE, ln, ln);
    vram_adr(0x2000);
    for (i = 0; i < 30; ++i) vram_fill(0, 32);
    vram_adr(0x23C0); vram_fill(0, 64);
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
    ts_t = 0; ts_shot = NSHOT;
    ppu_on();
}

void teaser_build(void)
{
    teaser_done = 0;
    ts_shot = 0;
    music_play(MUS_ROCK);
    shot_build();
}

/* à chaque trame */
void teaser_tick(void)
{
    u8 i, x, y, k;
    const u8 *col;
    if (cer_skip || (ts_t > 4 && (pad1_new & (PAD_A | PAD_START)))) {
        scroll_y = 0; teaser_done = 1; music_play(MUS_NONE); return;
    }
    ++ts_t;
    spr_begin();
    if (ts_shot < NSHOT) {
        col = shot_col[ts_shot];
        /* bandes obliques qui scintillent (leurs 2 couleurs alternent) sur
           un fond fixe ; après un flash, le fond revient */
        if ((ts_t & 3) == 0) {
            k = (u8)((ts_t >> 2) & 1);
            pal_buf[0] = pal_buf[4] = pal_buf[16] = col[0];   /* ($3F10 recopie $3F00 : même couleur de fond) */
            pal_buf[1] = col[1 + k]; pal_buf[2] = col[2 - k];
            pal_dirty = 1;
        }
        switch (ts_shot) {
        case 0:
            /* la balle arrive, impact (flash), puis part en traînée */
            if (ts_t < 18) spr1((u8)(250 - ts_t * 4), 100, (u8)(ball_base + 8), 3);
            else if (ts_t == 18) { pal_buf[0] = pal_buf[4] = pal_buf[16] = 0x30; pal_dirty = 1; sfx_hit(3); }
            else for (i = 0; i < 3; ++i) {
                x = (u8)(180 + (ts_t - 18) * 7 - i * 8);
                if (x > 180) spr1(x, (u8)(100 - (ts_t - 18) * 2 + i * 2), (u8)(ball_base + 8), 3);
            }
            break;
        case 1:
            /* glissade : gerbe de poussière depuis le pied, l'écran tremble */
            for (i = 0; i < 6; ++i) {
                x = (u8)(196 + ((ts_t * (i + 2)) & 31));
                y = (u8)(152 - ((ts_t * (i + 1)) & 15) - i * 3);
                spr1(x, y, (u8)(6 + (i & 1)), 2);
            }
            scroll_y = (u8)((ts_t & 2) ? 1 : 0);
            if (ts_t == 4) sfx_crowd(4);
            break;
        case 2:
            /* bras levés : confettis */
            for (i = 0; i < 10; ++i)
                spr1((u8)(i * 25 + 6), (u8)(((ts_t * (1 + (i & 1)) + i * 23) & 0x7F) + 8), confetti_tile, (u8)(i & 3));
            if (ts_t == 2) sfx_crowd(7);
            break;
        default:
            /* raquette au sol : choc à la trame 16, tremblement qui s'amortit, éclats */
            if (ts_t == 16) { pal_buf[0] = pal_buf[4] = pal_buf[16] = 0x30; pal_dirty = 1; sfx_canon(); ts_sh = 6; }
            if (ts_sh) { scroll_y = (u8)((ts_t & 1) ? ts_sh : 0); if ((ts_t & 3) == 0) --ts_sh; } else scroll_y = 0;
            if (ts_t > 16 && ts_t < 44)
                for (i = 0; i < 6; ++i)
                    spr1((u8)(150 + i * 6 + (ts_t - 16) * (i - 2)), (u8)(158 - (ts_t - 16) * 2 + (u8)((ts_t - 16) * (ts_t - 16) / 12)), ice_tile, 2);
            break;
        }
        spr_end();
        if (ts_t >= SHOT_T) {
            scroll_y = 0;
            if (++ts_shot < NSHOT) shot_build(); else title_build();
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
    if (ts_t >= 150) teaser_done = 1;
}
