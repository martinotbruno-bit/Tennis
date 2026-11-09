/* Royal Martinot — générique de lancement (banque BANK_INTRO), volontairement
 * très prétentieux : « Bruno Martinot presents », coproduction avec Augustin,
 * critiques de presse dithyrambiques, puis révélation du logo lettre par
 * lettre, avec « boum », étoiles qui scintillent, confettis, applaudissements
 * et fanfare. N'importe quel bouton passe directement à l'écran titre. */
#include "app.h"
#include "gfx.h"
#include "audio.h"
#include "assets.h"
#include "stats.h"

#pragma static-locals(on)

#define NSCENES 5
static const u16 scene_len[NSCENES] = { 160, 180, 180, 260, 440 };
static u8 sc;
static u16 t;

/* attributs : palette pal pour les rangées y0 à y1 (blocs de 4 rangées) */
static void attr_rows(u8 y0, u8 y1, u8 pal)
{
    u8 ay, ax, v = (u8)(pal | (pal << 2) | (pal << 4) | (pal << 6));
    for (ay = (u8)(y0 >> 2); ay <= (u8)(y1 >> 2); ++ay) {
        vram_adr((u16)(0x23C0 + ay * 8));
        for (ax = 0; ax < 8; ++ax) vram_put(v);
    }
}

static void scene(u8 n)
{
    ppu_off();                       /* fondu au noir automatique */
    vbuf_reset();
    clear_screen();
    pal_set_menu();
    spr_begin(); spr_end();
    switch (n) {
    case 0:
        txt_at(9, 11, "BRUNO MARTINOT");
        txt_at(12, 13, "PRESENTS");
        break;
    case 1:
        txt_at(6, 9, "IN ASSOCIATION WITH");
        txt_at(7, 12, "AUGUSTIN MARTINOT");
        txt_at(6, 14, "CHIEF EXECUTIVE SON");
        break;
    case 2:
        txt_at(8, 9, "A ROYAL MARTINOT");
        txt_at(11, 11, "PRODUCTION");
        txt_at(7, 15, "FILMED IN GLORIOUS");
        txt_at(7, 17, "8-BIT TECHNICOLOR");
        break;
    case 3:
        /* les étoiles de la critique apparaissent une à une (intro_tick) */
        txt_at(10, 7, "MAGNIFIQUE.");
        txt_at(8, 8, "- BRUNO MARTINOT");
        txt_at(4, 12, "THE GREATEST TENNIS GAME");
        txt_at(4, 13, "EVER MADE IN THIS HOUSE.");
        txt_at(6, 14, "- AUGUSTIN MARTINOT");
        txt_at(5, 18, "WINNER - BEST GAME 2026");
        txt_at(2, 19, "(CATEGORY: OUR LIVING ROOM)");
        break;
    default:
        attr_rows(4, 11, 2);         /* logo doré */
        /* confettis multicolores (palettes de sprites 0, 1 et 3) */
        pal_buf[17] = 0x16; pal_buf[18] = 0x2A; pal_buf[19] = 0x30;
        pal_buf[21] = 0x12; pal_buf[22] = 0x28; pal_buf[23] = 0x30;
        pal_buf[29] = 0x24; pal_buf[30] = 0x21; pal_buf[31] = 0x28;
        pal_dirty = 1;
        break;
    }
    ppu_on();
}

/* --- révélation du logo ------------------------------------------------ */
static const u8 letter_slot[13] = { 0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13 };
static const char sub1[] = "GRAND SLAM PRESTIGE";
static const char sub2[] = "PLATINUM DIAMOND EDITION";
static const u8 conf_x[10] = { 20, 52, 84, 116, 148, 180, 212, 236, 36, 132 };
static const u8 conf_v[10] = { 1, 2, 1, 3, 2, 1, 2, 3, 2, 1 };
static const u8 shimmer[4] = { 0x30, 0x38, 0x28, 0x38 };
static u8 spk_x[6], spk_y[6], spk_t[6];
static char cell[3];

static void draw_letter(u8 k)
{
    /* lettre k (0..12) du logo : ROYAL rangées 6-7, MARTINOT rangées 8-9 */
    u8 s = letter_slot[k], x, y;
    if (k < 5) { x = (u8)(11 + k * 2); y = 6; }
    else { x = (u8)(8 + (k - 5) * 2); y = 8; }
    cell[0] = (char)logo_tiles[s * 4]; cell[1] = (char)logo_tiles[s * 4 + 1]; cell[2] = 0;
    vbuf_text(x, y, cell, 2);
    cell[0] = (char)logo_tiles[s * 4 + 2]; cell[1] = (char)logo_tiles[s * 4 + 3];
    vbuf_text(x, (u8)(y + 1), cell, 2);
}

static void logo_anim(void)
{
    u8 i, k;
    vbuf_reset();
    /* une lettre géante toutes les 8 trames, chacune avec son « boum » */
    if (t >= 8 && t <= 104 && (t & 7) == 0) { draw_letter((u8)((t >> 3) - 1)); sfx_canon(); }
    if (t == 112) sfx_crowd(7);                                  /* ovation */
    /* sous-titres tapés lettre à lettre */
    if (t >= 120 && t < 120 + 2 * 19 && !(t & 1)) {
        k = (u8)((t - 120) >> 1);
        cell[0] = sub1[k]; cell[1] = 0;
        vbuf_text((u8)(6 + k), 12, cell, 1);
        sfx_menu_move();
    }
    if (t >= 168 && t < 168 + 2 * 24 && !(t & 1)) {
        k = (u8)((t - 168) >> 1);
        cell[0] = sub2[k]; cell[1] = 0;
        vbuf_text((u8)(4 + k), 14, cell, 1);
        sfx_menu_move();
    }
    if (t == 238) { vbuf_text(4, 18, "LIMITED EDITION : 1 COPY", 24); sfx_menu_ok(); }
    if (t == 258) vbuf_text(6, 21, "ALL RIGHTS RESERVED", 19);
    if (t == 270) vbuf_text(5, 22, "(MOSTLY BY AUGUSTIN)", 20);
    /* or qui scintille : la couleur claire du logo change toutes les 6 trames */
    if ((t % 6) == 0) {
        pal_buf[11] = shimmer[(t / 6) & 3];
        pal_buf[10] = (u8)(((t / 6) & 1) ? 0x28 : 0x27);
        pal_dirty = 1;
    }
    /* étoiles qui brillent autour du logo, confettis après la dernière lettre */
    spr_begin();
    for (i = 0; i < 6; ++i) {
        if (!spk_t[i]) {
            spk_t[i] = (u8)(8 + (rand8() & 15));
            spk_x[i] = (u8)(64 + (rand8() & 127));
            spk_y[i] = (u8)(40 + (rand8() & 31));
        }
        --spk_t[i];
        if (spk_t[i] & 4) spr1(spk_x[i], spk_y[i], 3, 2);
    }
    if (t >= 112)
        for (i = 0; i < 10; ++i)
            spr1(conf_x[i], (u8)(((t - 112) * conf_v[i] + i * 23) % 200 + 16), confetti_tile, (u8)(i & 3));
    spr_end();
}

static void stars_anim(void)
{
    /* critique : cinq étoiles, une toutes les 12 trames */
    if (t >= 12 && t <= 60 && (t % 12) == 0) {
        vbuf_reset();
        vbuf_text((u8)(12 + t / 12 - 1), 5, "*", 1);
        sfx_menu_ok();
    }
}

static void finish(void)
{
    music_play(MUS_NONE);
    spr_begin(); spr_end();
    go_req = GO_SCREEN;
    go_arg = SCR_TITLE;
}

void intro_begin(void)
{
    /* réglage SON mémorisé (cartouche à pile) appliqué dès le générique ;
       les autres réglages sont relus et vérifiés par l'écran titre */
    if ((u8)far_call(BANK_COMP, (void *)so_load, 0) && opt_val[22] < 3) snd_set(opt_val[22]);
    sc = 0;
    t = 0;
    scene(0);
    music_play(MUS_INTRO);
}

void intro_tick(void)
{
    /* n'importe quel bouton : directement à l'écran titre */
    if ((pad1_new | pad2_new) & (PAD_A | PAD_B | PAD_START | PAD_SELECT)) { finish(); return; }
    if (++t >= scene_len[sc]) {
        t = 0;
        if (++sc == NSCENES) { finish(); return; }
        scene(sc);
        return;
    }
    if (sc == 3) stars_anim();
    else if (sc == 4) logo_anim();
}
