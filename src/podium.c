/* Royal Martinot — podium d'une compétition gagnée (banque BANK_PODIUM).
 *
 * Les 3 premiers montent sur un podium centré, posé sur le court de la
 * compétition, en taille double (32 x 48 px). La NES ne sait pas agrandir les
 * sprites : chaque personnage (6 tuiles de 8 x 8) est agrandi à l'écran
 * construit (rendu coupé) en 24 tuiles, chaque pixel devenant 2 x 2.
 *  - joueurs : tuiles du DÉCOR (à partir de $60, à la place des tuiles du
 *    court, du logo, etc., inutiles ici), une palette du décor chacun ;
 *  - coupe agrandie (et, en double, le 3e) : tuiles de SPRITES, à la place des
 *    personnages vus de dos (inutiles ici).
 * Toute la CHR-RAM est rechargée depuis la banque 2 en quittant l'écran
 * (chr_reload).
 *
 * Palettes du décor : 0 = court (fond), haut des marches, marches, blanc ;
 * 1 à 3 = un joueur chacune. En double, il faudrait 6 palettes : les deux
 * vainqueurs et le capitaine finaliste sont dans le décor, le capitaine
 * classé 3e est en sprites (24 sprites, palette de sprites 0) ; le nom des
 * équipes complètes est écrit sous chaque marche. */
#include "app.h"
#include "gfx.h"
#include "audio.h"
#include "assets.h"
#include "game.h"
#include "comp.h"

#pragma static-locals(on)

#define PPU_DATA (*(volatile u8 *)0x2007)

#define T_PLAYER 0x60          /* 3 joueurs x 24 tuiles du décor : $60..$A7 */
#define T_TOP    0xA8          /* haut de marche */
#define T_FACE   0xA9          /* marche */
#define T_DIGIT  0xAA          /* chiffres 1, 2, 3 sur les marches ($AA..$AC) */
#define T_HL     0xAD          /* ligne blanche horizontale */
#define T_VL     0xAE          /* ligne blanche verticale */
#define T_CTL    0xAF          /* coin haut-gauche */
#define T_CTR    0xB0          /* coin haut-droite */
#define S_CUP    0x10          /* coupe agrandie : 16 tuiles de sprites */
#define S_THIRD  0x20          /* 3e (double) : 24 tuiles de sprites */

#pragma bss-name(push, "WBSS")
u8 pod_new;
static u8 src[16], out[64], at[64];
static u8 cup_t, fw_c, third_c;
#pragma bss-name(pop)

/* un quartet de pixels doublé : chaque bit devient 2 bits */
static const u8 dbl[16] = { 0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F,
                            0xC0, 0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF };
/* variante de bras de la tête de Margaux par pose (comme sprp.s) */
/* couleur de la surface (fond universel) ; Margaux : rose */
static const u8 surf_col[7] = { 0x26, 0x2A, 0x12, 0x13, 0x28, 0x21, 0x25 };

/* une tuile source (adresse PPU sa) -> 4 tuiles agrandies (haut-gauche,
   haut-droite, bas-gauche, bas-droite) à partir de l'adresse PPU da */
static void dbl_tile(u16 sa, u16 da)
{
    u8 i, b, h;
    vram_adr(sa);
    src[0] = PPU_DATA;               /* lecture à vide (tampon du PPU) ; « (void) » serait supprimé par cc65 */
    for (i = 0; i < 16; ++i) src[i] = PPU_DATA;
    for (i = 0; i < 8; ++i) {
        h = (u8)(i >> 1);
        b = src[h];      out[i] = dbl[b >> 4];      out[16 + i] = dbl[b & 15];
        b = src[8 + h];  out[8 + i] = dbl[b >> 4];  out[24 + i] = dbl[b & 15];
        b = src[4 + h];  out[32 + i] = dbl[b >> 4]; out[48 + i] = dbl[b & 15];
        b = src[12 + h]; out[40 + i] = dbl[b >> 4]; out[56 + i] = dbl[b & 15];
    }
    vram_adr(da);
    for (i = 0; i < 64; ++i) vram_put(out[i]);
}

/* personnage c, vu de face, dans la pose donnée : 24 tuiles à partir de la
   tuile dst de la table base ($0000 décor, $1000 sprites). Tuile source k
   (rangée k/2, colonne k&1) -> tuiles dst + 4k .. dst + 4k + 3 */
static void big_person(u8 c, u8 pose, u16 base, u8 dst)
{
    u8 k, s, m = (u8)(person_row[(char_female[c] << 1) | 1] + pose * 3);
    for (k = 0; k < 6; ++k) {
        s = (u8)(person_map[m + (k >> 1)] + (k & 1));
        if (c == 16 && k < 2) s = (u8)(marg_map[11 + pose] + k);   /* Margaux (de face) */
        dbl_tile((u16)(0x1000 + ((u16)s << 4)), (u16)(base + ((u16)(u8)(dst + k * 4) << 4)));
    }
}

/* palette du bloc de 16 x 16 px (colonne bx, rangée by de blocs) */
static void blk(u8 bx, u8 by, u8 pal)
{
    u8 i = (u8)((by >> 1) * 8 + (bx >> 1)), sh = (u8)(((by & 1) << 2) | ((bx & 1) << 1));
    at[i] = (u8)((at[i] & (u8)~(3 << sh)) | (pal << sh));
}

/* joueur agrandi dans le décor : coin (x, y) en tuiles (pairs), palette pal */
static void put_big(u8 x, u8 y, u8 dst, u8 pal)
{
    u8 r, c;
    for (r = 0; r < 6; ++r) {
        vram_adr(NTADR(x, y + r));
        for (c = 0; c < 4; ++c)
            vram_put((u8)(dst + ((r >> 1) * 2 + (c >> 1)) * 4 + (r & 1) * 2 + (c & 1)));
    }
    for (r = 0; r < 3; ++r) { blk((u8)(x >> 1), (u8)((y >> 1) + r), pal); blk((u8)((x >> 1) + 1), (u8)((y >> 1) + r), pal); }
}

/* marche : colonnes x .. x+w-1, de la rangée top à la rangée 24 ; chiffre n */
static void step(u8 x, u8 w, u8 top, u8 n)
{
    u8 r;
    vram_adr(NTADR(x, top));
    vram_fill(T_TOP, w);
    for (r = (u8)(top + 1); r <= 24; ++r) {
        vram_adr(NTADR(x, r));
        vram_fill(T_FACE, w);
    }
    vram_adr(NTADR(x + ((w - 1) >> 1), top + 1));
    vram_put((u8)(T_DIGIT + n - 1));
}

static u8 slen(const char *s) { u8 n = 0; while (s[n]) ++n; return n; }
static void txt_c(u8 y, const char *s) { txt_at((u8)((32 - slen(s)) >> 1), y, s); }
/* nom (simple) ou équipe (double) centré sous une marche */
static void label(u8 x, u8 w, u8 i)
{
    static char b[8];
    u8 n;
    if (c_dbl) team_label(c_pl[i], c_pl2[i], b);
    else {
        /* simple : nom entier (MARGAUX déborde d'une case, toujours centré) */
        const char *p = char_name[c_pl[i]];
        for (n = 0; n < 7 && p[n] > ' '; ++n) b[n] = p[n];
        b[n] = 0;
    }
    txt_at((u8)(x + (w >> 1) - (slen(b) >> 1)), 26, b);
}

/* tuile unie de 8 octets répétés par plan */
static void tile8(u8 t, u8 p0, u8 p1)
{
    u8 i;
    vram_adr((u16)t << 4);
    for (i = 0; i < 8; ++i) vram_put(p0);
    for (i = 0; i < 8; ++i) vram_put(p1);
}
static void make_tiles(void)
{
    u8 i, n;
    /* haut de marche : 2 lignes claires (couleur 1), puis la marche (couleur 2) */
    vram_adr((u16)T_TOP << 4);
    for (i = 0; i < 8; ++i) vram_put((u8)(i < 2 ? 0xFF : 0x00));
    for (i = 0; i < 8; ++i) vram_put((u8)(i < 2 ? 0x00 : 0xFF));
    tile8(T_FACE, 0x00, 0xFF);
    /* chiffres blancs (couleur 3) sur la marche (couleur 2), d'après la police */
    for (n = 0; n < 3; ++n) {
        vram_adr((u16)('1' + n) << 4);
        src[0] = PPU_DATA;               /* lecture à vide (tampon du PPU) ; « (void) » serait supprimé par cc65 */
        for (i = 0; i < 8; ++i) src[i] = PPU_DATA;
        vram_adr((u16)(T_DIGIT + n) << 4);
        for (i = 0; i < 8; ++i) vram_put(src[i]);
        for (i = 0; i < 8; ++i) vram_put(0xFF);
    }
    /* lignes du court (couleur 3) */
    vram_adr((u16)T_HL << 4);
    for (n = 0; n < 2; ++n) for (i = 0; i < 8; ++i) vram_put((u8)(i == 3 || i == 4 ? 0xFF : 0x00));
    tile8(T_VL, 0x18, 0x18);
    vram_adr((u16)T_CTL << 4);
    for (n = 0; n < 2; ++n) for (i = 0; i < 8; ++i) vram_put((u8)(i < 3 ? 0x00 : i < 5 ? 0x1F : 0x18));
    vram_adr((u16)T_CTR << 4);
    for (n = 0; n < 2; ++n) for (i = 0; i < 8; ++i) vram_put((u8)(i < 3 ? 0x00 : i < 5 ? 0xF8 : 0x18));
}

void podium_build(void)
{
    static char b[32];
    u8 i, sc, w, x1, x2, x3, wc, light;
    COMP(comp_podium, 0);
    for (i = 0; i < 64; ++i) at[i] = 0;
    make_tiles();

    /* palettes : court, marches grises, blanc (noir sur les surfaces claires) */
    sc = c_env == ENV_MARGAUX ? 6 : c_surf;
    light = (u8)(sc == 4 || sc == 5);
    pal_buf[0] = pal_buf[16] = surf_col[sc];
    pal_buf[1] = 0x10; pal_buf[2] = 0x00; pal_buf[3] = light ? 0x0F : 0x30;
    /* joueurs du décor : pal_set_player(61..63) écrit les palettes 1..3 du
       décor (16 + 4 x slot reboucle sur 8 bits) */
    pal_set_player(61, c_pl[0]);
    pal_set_player(62, c_dbl ? c_pl2[0] : c_pl[c_pod[1]]);
    pal_set_player(63, c_dbl ? c_pl[c_pod[1]] : c_pl[c_pod[2]]);
    third_c = c_pl[c_pod[2]];
    /* sprites : 0 le 3e en double (sinon le vainqueur), 1 le 2e ; confettis */
    pal_set_player(0, c_dbl ? third_c : c_pl[0]);
    pal_set_player(1, c_pl[c_pod[1]]);

    /* le court : lignes de fond et de côté */
    vram_adr(NTADR(2, 9)); vram_put(T_CTL); vram_fill(T_HL, 26); vram_put(T_CTR);
    for (i = 10; i < 30; ++i) {
        vram_adr(NTADR(2, i)); vram_put(T_VL);
        vram_adr(NTADR(29, i)); vram_put(T_VL);
    }

    /* podium centré : 2e à gauche, 1er au milieu (plus haut), 3e à droite */
    wc = c_dbl ? 10 : 6;
    w = (u8)(6 + wc + 6);
    x2 = (u8)((32 - w) >> 1); x1 = (u8)(x2 + 6); x3 = (u8)(x1 + wc);
    step(x2, 6, 20, 2);
    step(x1, wc, 18, 1);
    step(x3, 6, 22, 3);

    /* joueurs agrandis : vainqueur(s) bras levés, les autres au repos */
    if (c_dbl) {
        big_person(c_pl[0], POSE_JOY, 0x0000, T_PLAYER);
        big_person(c_pl2[0], POSE_JOY, 0x0000, T_PLAYER + 24);
        big_person(c_pl[c_pod[1]], POSE_IDLE, 0x0000, T_PLAYER + 48);
        big_person(third_c, POSE_IDLE, 0x1000, S_THIRD);
        put_big((u8)(x1 + 1), 12, T_PLAYER, 1);
        put_big((u8)(x1 + 5), 12, T_PLAYER + 24, 2);
        put_big((u8)(x2 + 1), 14, T_PLAYER + 48, 3);
    } else {
        big_person(c_pl[0], POSE_JOY, 0x0000, T_PLAYER);
        big_person(c_pl[c_pod[1]], POSE_IDLE, 0x0000, T_PLAYER + 24);
        big_person(c_pl[c_pod[2]], POSE_IDLE, 0x0000, T_PLAYER + 48);
        put_big((u8)(x1 + 1), 12, T_PLAYER, 1);
        put_big((u8)(x2 + 1), 14, T_PLAYER + 24, 2);
        put_big((u8)(x3 + 1), 16, T_PLAYER + 48, 3);
    }
    /* coupe agrandie (sprites) */
    for (i = 0; i < 4; ++i)
        dbl_tile((u16)(0x1000 + ((u16)(u8)(trophy_tile + i) << 4)), (u16)(0x1000 + ((u16)(u8)(S_CUP + i * 4) << 4)));

    label(x2, 6, c_pod[1]);
    label(x1, wc, 0);
    label(x3, 6, c_pod[2]);

    /* textes centrés */
    for (i = 0; i < 15; ++i) b[i] = "ROYAL MARTINOT "[i];
    {
        const char *n = comp_name[c_type];
        u8 k = 15;
        while (*n && k < 15 + 12) b[k++] = *n++;
        b[k] = 0;
    }
    txt_c(2, b);
    txt_c(4, c_type == CT_LEAGUE || c_type == CT_SEASON ? "CHAMPION !" : "VAINQUEUR !");
    for (i = 0; i < 11; ++i) b[i] = "TROPHEES : "[i];
    num2(b + 11, c_trophies); b[13] = 0;
    txt_c(5, b);
    if (pod_new) txt_c(7, "MODE NUDISTE DEBLOQUE !");
    txt_c(28, "A : RETOUR AU TITRE");

    vram_adr(0x23C0);
    for (i = 0; i < 64; ++i) vram_put(at[i]);
    pal_dirty = 1;
    cup_t = 0;
}

/* Coupe brandie au-dessus du ou des vainqueurs, feux d'artifice, confettis ;
   en double, le 3e en sprites agrandis. Pour tenir dans une image, seuls les
   éléments qui bougent sont réécrits à chaque trame, directement dans la
   copie de l'OAM : entrées 0-15 la coupe (hauteur), 16-19 les feux
   d'artifice, 20-29 les confettis (hauteur, par addition), 30-53 le 3e
   (écrit une seule fois). */
extern u8 oam_buf[256];
extern u8 oam_i;
#pragma zpsym("oam_i")
static const u8 conf_x[10] = { 20, 52, 84, 116, 148, 180, 212, 236, 36, 132 };
static const u8 conf_v[10] = { 1, 2, 1, 3, 2, 1, 2, 3, 2, 1 };
static const u8 conf_y0[10] = { 16, 39, 62, 85, 108, 131, 154, 177, 200, 23 };
#pragma bss-name(push, "WBSS")
static u8 conf_y[10];
#pragma bss-name(pop)
/* 4 x 4 sprites (coupe) ou 4 x 6 (joueur) à partir de l'entrée o de l'OAM */
static void big_spr(u8 o, u8 x, u8 y, u8 t, u8 rows, u8 pal)
{
    u8 r, c;
    for (r = 0; r < rows; ++r)
        for (c = 0; c < 4; ++c) {
            oam_buf[o] = (u8)(y + r * 8 - 1);
            oam_buf[o + 1] = (u8)(t + ((r >> 1) * 2 + (c >> 1)) * 4 + (r & 1) * 2 + (c & 1));
            oam_buf[o + 2] = pal;
            oam_buf[o + 3] = (u8)(x + c * 8);
            o += 4;
        }
}
void podium_sprites(void)
{
    u8 i, o, cy, y;
    if (cup_t == 0) {
        /* première trame : tout masqué, puis coupe, confettis et 3e en place */
        for (i = 0; i < 64; ++i) oam_buf[i * 4] = 0xFF;
        big_spr(0, 112, 80, S_CUP, 4, 2);
        for (i = 0; i < 10; ++i) {
            o = (u8)(80 + i * 4);
            conf_y[i] = conf_y0[i];
            oam_buf[o + 1] = confetti_tile; oam_buf[o + 2] = (u8)(i & 3); oam_buf[o + 3] = conf_x[i];
        }
        if (c_dbl) big_spr(120, (u8)((((32 - 22) >> 1) + 16 + 1) * 8), 128, S_THIRD, 6, 0);
    }
    if (cup_t < 255) ++cup_t;
    /* la coupe monte des mains au-dessus de la tête, puis oscille */
    if (cup_t < 24) cy = 80;
    else if (cup_t < 54) cy = (u8)(80 - ((cup_t - 24) >> 1));
    else cy = (u8)(65 + (((frame_cnt >> 3) & 1) ? 1 : 0));
    for (i = 0; i < 16; ++i) oam_buf[i * 4] = (u8)(cy + (i >> 2) * 8 - 1);
    /* feux d'artifice pendant que la coupe est brandie (5 fusées, ~4 s) */
    oam_buf[64] = oam_buf[68] = oam_buf[72] = oam_buf[76] = 0xFF;
    if (cup_t == 24) fw_c = 131;
    if (cup_t >= 24 && cup_t < 233) {             /* arrêt après la 5e gerbe, avant une 6e fusée */
        if (--fw_c == 0) fw_c = 130;
        fw_sound(fw_c);
        oam_i = 64;
        fw_draw(fw_c, 0);
    }
    /* confettis : chute à 3 vitesses, retour en haut */
    for (i = 0; i < 10; ++i) {
        o = (u8)(80 + i * 4);
        if (cup_t < 24) { oam_buf[o] = 0xFF; continue; }
        y = (u8)(conf_y[i] + conf_v[i]);
        if (y > 215) y = (u8)(y - 200);
        conf_y[i] = y;
        oam_buf[o] = (u8)(y - 1);
    }
}
