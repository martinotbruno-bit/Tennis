/* Royal Martinot — générique de lancement (banque BANK_INTRO). Il se prend
 * extrêmement au sérieux, sur une fanfare qui déraille complètement :
 *  - « Bruno Martinot presents », coproduction avec Augustin ;
 *  - une barre de chargement du prestige qui finit à 117 % ;
 *  - des critiques dithyrambiques signées par la famille ;
 *  - des mentions légales absurdes ;
 *  - la révélation du logo, lettre par lettre, avec « boum », or scintillant,
 *    étoiles, confettis et ovation… suivie d'un seul applaudissement timide ;
 *    puis une balle fait tomber une lettre, et le stagiaire vient la remettre.
 * Le bouton « passer » est vexé : au premier appui il demande confirmation,
 * au second il cède (« FINE. »). */
#include "app.h"
#include "gfx.h"
#include "audio.h"
#include "assets.h"
#include "stats.h"

#pragma static-locals(on)

#define PPU_DATA (*(volatile u8 *)0x2007)

#define NSCENES 7
#define SC_LOAD 3
#define SC_PRESS 4
#define SC_LOGO 6
static const u16 scene_len[NSCENES] = { 160, 160, 160, 230, 240, 180, 690 };
static u8 sc;
static u16 t;
static u8 skip_n, skip_t;       /* bouton « passer » vexé */
static char cell[4];

/* attributs : palette pal pour les rangées y0 à y1 (blocs de 4 rangées) */
static void attr_rows(u8 y0, u8 y1, u8 pal)
{
    u8 ay, ax, v = (u8)(pal | (pal << 2) | (pal << 4) | (pal << 6));
    for (ay = (u8)(y0 >> 2); ay <= (u8)(y1 >> 2); ++ay) {
        vram_adr((u16)(0x23C0 + ay * 8));
        for (ax = 0; ax < 8; ++ax) vram_put(v);
    }
}

/* --- lettre « N » du logo en sprites (pour la faire tomber) ------------- */
/* Les 4 tuiles de la lettre sont copiées dans la table des sprites, à la
   place des tuiles de la coupe (inutilisée ici), restaurées à la fin. */
#pragma bss-name(push, "SAVEDATA")    /* (version 2.61 : zone de sauvegarde libre, hors du bloc sauvegardé ; libère la RAM de travail) */
static u8 cup_save[64];
static u8 tilebuf[16];
#pragma bss-name(pop)
#define N_SLOT 11                /* « ROYAL MARTINOT » (espace compris) : le N est en position 11 */
static void letter_to_sprites(void)
{
    u8 i, j;
    for (i = 0; i < 4; ++i) {
        vram_adr((u16)(logo_tiles[N_SLOT * 4 + i] << 4));
        tilebuf[0] = PPU_DATA;       /* lecture à vide (tampon du PPU) ; « (void) » serait supprimé par cc65 */
        for (j = 0; j < 16; ++j) tilebuf[j] = PPU_DATA;
        vram_adr((u16)(0x1000 + ((trophy_tile + i) << 4)));
        for (j = 0; j < 16; ++j) vram_put(tilebuf[j]);
    }
}
static void cup_restore(void)
{
    u8 i;
    vram_adr((u16)(0x1000 + (trophy_tile << 4)));
    for (i = 0; i < 64; ++i) vram_put(cup_save[i]);
}

static void scene(u8 n)
{
    ppu_off();                       /* fondu au noir automatique */
    clear_screen();
    pal_set_menu();
    pal_buf[0] = pal_buf[4] = pal_buf[8] = pal_buf[12] = pal_buf[16] = pal_buf[20] = 0x0F;   /* générique : fond noir */
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
    case SC_LOAD:
        txt_at(6, 9, "LOADING PRESTIGE...");
        txt_at(5, 12, "(                    )");
        txt_at(8, 19, "DO NOT TURN OFF");
        txt_at(9, 20, "YOUR PRESTIGE");
        break;
    case SC_PRESS:
        /* les étoiles de la critique apparaissent une à une */
        txt_at(10, 7, "MAGNIFIQUE.");
        txt_at(8, 8, "- BRUNO MARTINOT");
        txt_at(4, 12, "THE GREATEST TENNIS GAME");
        txt_at(4, 13, "EVER MADE IN THIS HOUSE.");
        txt_at(6, 14, "- AUGUSTIN MARTINOT");
        txt_at(5, 18, "WINNER - BEST GAME 2026");
        txt_at(2, 19, "(CATEGORY: OUR LIVING ROOM)");
        break;
    case 5:
        txt_at(2, 6, "NO TENNIS BALLS WERE HARMED");
        txt_at(7, 7, "DURING PRODUCTION.");
        txt_at(6, 10, "RATED A FOR AUGUSTIN");
        txt_at(6, 13, "POWERED BY 1.79 MHZ");
        txt_at(9, 14, "OF PURE LUXURY");
        txt_at(7, 17, "OFFICIAL SPONSOR :");
        txt_at(7, 18, "THE FAMILY FRIDGE");
        txt_at(10, 21, "0 % CALORIES");
        break;
    default:
        attr_rows(4, 11, 2);         /* logo doré */
        letter_to_sprites();
        /* palettes de sprites : 0 le stagiaire, 1 confettis, 3 la lettre */
        pal_set_player(0, 0);
        pal_buf[21] = 0x12; pal_buf[22] = 0x28; pal_buf[23] = 0x30;
        pal_buf[29] = 0x27; pal_buf[30] = 0x28; pal_buf[31] = 0x30;
        pal_dirty = 1;
        break;
    }
    ppu_on();
}

/* --- barre de chargement du prestige ------------------------------------- */
static u8 load_pct, load_cells;
static void load_anim(void)
{
    u8 p, c;
    if (t < 60) p = (u8)((t * 87) / 60);
    else if (t < 140) p = (u8)(87 + (t - 60) / 7);
    else if (t < 175) p = 99;
    else p = 117;
    if (t == 175) { sfx_canon(); vbuf_text(11, 16, "OVERLOAD !", 10); }
    if (p != load_pct) {
        load_pct = p;
        cell[0] = (char)(p >= 100 ? '1' : ' ');
        cell[1] = (char)('0' + (p / 10) % 10);
        cell[2] = (char)('0' + p % 10);
        cell[3] = 0;
        vbuf_text(13, 14, cell, 3);
        vbuf_text(17, 14, "%", 1);
    }
    /* 20 cases pour 100 % ; à 117 %, la barre déborde du cadre */
    c = (u8)((p * 20) / 100);
    while (load_cells < c) { vbuf_text((u8)(6 + load_cells), 12, "#", 1); ++load_cells; }
}

/* --- révélation du logo -------------------------------------------------- */
static const u8 letter_slot[13] = { 0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13 };
static const char sub1[] = "GRAND SLAM PRESTIGE";
static const char sub2[] = "PLATINUM DIAMOND EDITION";
static const u8 conf_x[10] = { 20, 52, 84, 116, 148, 180, 212, 236, 36, 132 };
static const u8 conf_v[10] = { 1, 2, 1, 3, 2, 1, 2, 3, 2, 1 };
static const u8 shimmer[4] = { 0x30, 0x38, 0x28, 0x38 };
static u8 spk_x[6], spk_y[6], spk_t[6];
/* la lettre qui tombe (sprite), la balle et le stagiaire */
#define N_X 144                   /* colonne 18 */
#define N_Y 64                    /* rangée 8 */
static u8 lx, ly, lvy, lstate;    /* lstate : 0 en place, 1 tombe, 2 au sol, 3 remonte */
static u8 bxp, byp, ix;

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
    /* l'ovation retombe… un seul applaudissement timide */
    if (t == 226) { sfx_clap(); vbuf_text(13, 16, "*CLAP*", 6); }
    if (t == 262) vbuf_text(13, 16, "      ", 6);
    if (t == 238) { vbuf_text(4, 18, "LIMITED EDITION : 1 COPY", 24); sfx_menu_ok(); }
    if (t == 258) vbuf_text(6, 21, "ALL RIGHTS RESERVED", 19);
    if (t == 270) vbuf_text(6, 22, "(MOSTLY BY AUGUSTIN)", 20);
    /* une balle surgit et fait tomber le N de MARTINOT */
    if (t == 320) { bxp = 250; byp = 40; }
    if (t == 345) {
        vbuf_text(18, 8, "  ", 2); vbuf_text(18, 9, "  ", 2);
        lx = N_X; ly = N_Y; lvy = 0; lstate = 1;
        sfx_hit(2);
    }
    if (lstate == 1) {
        if ((t & 3) == 0 && lvy < 6) ++lvy;
        ly = (u8)(ly + lvy);
        if (ly >= 192) { ly = 192; lstate = 2; sfx_net(); }   /* « bonk » */
    }
    /* le stagiaire arrive, s'excuse, remet la lettre */
    if (t == 395) ix = 8;
    if (t > 395 && t < 519) ++ix;
    if (t == 525) vbuf_text(10, 26, "OOPS. SORRY.", 12);
    if (t == 540) lstate = 3;
    if (lstate == 3) {
        ly = (u8)(ly - 6);
        if (ly <= N_Y) {
            ly = N_Y; lstate = 0;
            cell[0] = (char)logo_tiles[N_SLOT * 4]; cell[1] = (char)logo_tiles[N_SLOT * 4 + 1]; cell[2] = 0;
            vbuf_text(18, 8, cell, 2);
            cell[0] = (char)logo_tiles[N_SLOT * 4 + 2]; cell[1] = (char)logo_tiles[N_SLOT * 4 + 3];
            vbuf_text(18, 9, cell, 2);
            vbuf_text(6, 26, "(PRESTIGE RESTORED)", 19);
            sfx_menu_ok();
        }
    }
    if (t >= 600 && ix > 10) ix = (u8)(ix - 2);           /* il repart */
    /* or qui scintille : la couleur claire du logo change toutes les 6 trames */
    if ((t % 6) == 0) {
        pal_buf[11] = shimmer[(t / 6) & 3];
        pal_buf[10] = (u8)(((t / 6) & 1) ? 0x28 : 0x27);
        pal_dirty = 1;
    }
    /* sprites : balle, lettre, stagiaire, étoiles, confettis */
    spr_begin();
    if (t >= 320 && t < 345) {
        bxp = (u8)(bxp - 4); byp = (u8)(byp + 1);
        spr1(bxp, byp, 1, 2);
    } else if (t >= 345 && t < 400 && byp > 8) {
        bxp = (u8)(bxp + 3); byp = (u8)(byp - 2);          /* rebondit et s'en va */
        spr1(bxp, byp, 1, 2);
    }
    if (lstate) {
        spr1(lx, ly, trophy_tile, 3);
        spr1((u8)(lx + 8), ly, (u8)(trophy_tile + 1), 3);
        spr1(lx, (u8)(ly + 8), (u8)(trophy_tile + 2), 3);
        spr1((u8)(lx + 8), (u8)(ly + 8), (u8)(trophy_tile + 3), 3);
    }
    if (t > 395 && ix > 10) {
        /* course de profil à l'aller et au retour, joie quand la lettre est remise */
        k = (u8)((t >> 3) & 1 ? POSE_RUN1 : POSE_RUN2);
        if (t >= 519 && t < 560) k = POSE_IDLE;
        else if (t >= 560 && t < 600) k = POSE_JOY;
        spr_person(ix, 208, 0, 1, k, 0, 0);
    }
    for (i = 0; i < 6; ++i) {
        if (!spk_t[i]) {
            spk_t[i] = (u8)(8 + (rand8() & 15));
            spk_x[i] = (u8)(64 + (rand8() & 127));
            spk_y[i] = (u8)(40 + (rand8() & 31));
        }
        --spk_t[i];
        if (spk_t[i] & 4) spr1(spk_x[i], spk_y[i], 3, 2);
    }
    if (t >= 112 && t < 330)
        for (i = 0; i < 10; ++i)
            spr1(conf_x[i], (u8)(((t - 112) * conf_v[i] + i * 23) % 200 + 16), confetti_tile, (u8)(i & 1));
    spr_end();
}

static void stars_anim(void)
{
    /* critique : cinq étoiles, une toutes les 12 trames */
    if (t >= 12 && t <= 60 && (t % 12) == 0) {
        vbuf_text((u8)(13 + t / 12 - 1), 5, "*", 1);    /* « ***** » centré, colonnes 13 à 17 */
        sfx_menu_ok();
    }
}

static void finish(void)
{
    music_play(MUS_NONE);
    ppu_off();
    cup_restore();                   /* tuiles de la coupe, empruntées par la lettre */
    spr_begin(); spr_end();
    go_req = GO_SCREEN;
    go_arg = SCR_TITLE;
}

void intro_begin(void)
{
    /* réglage SON mémorisé (cartouche à pile) appliqué dès le générique ;
       les autres réglages sont relus et vérifiés par l'écran titre */
    u8 i;
    if ((u8)far_call(BANK_COMP, (void *)so_load, 0) && opt_val[22] < 3) snd_set(opt_val[22]);
    /* coupe d'origine sauvegardée (écran encore coupé au démarrage) */
    vram_adr((u16)(0x1000 + (trophy_tile << 4)));
    cup_save[0] = PPU_DATA;          /* lecture à vide (tampon du PPU) */
    for (i = 0; i < 64; ++i) cup_save[i] = PPU_DATA;
    sc = 0;
    t = 0;
    skip_n = skip_t = 0;
    load_pct = 255; load_cells = 0;
    scene(0);
    music_play(MUS_INTRO);
}

void intro_tick(void)
{
    vbuf_reset();
    /* bouton « passer » vexé : confirmation au premier appui */
    if ((pad1_new | pad2_new) & (PAD_A | PAD_B | PAD_START | PAD_SELECT)) {
        if (!skip_n) {
            skip_n = 1;
            vbuf_text(5, 26, "ARE YOU SURE? THIS IS", 21);
            vbuf_text(7, 27, "VERY PRESTIGIOUS.", 17);
            sfx_fault();
        } else if (!skip_t) {
            skip_t = 10;
            vbuf_text(5, 26, "        FINE.        ", 21);
            vbuf_text(7, 27, "                 ", 17);
        }
    }
    if (skip_t && --skip_t == 0) { finish(); return; }
    if (++t >= scene_len[sc]) {
        t = 0;
        if (++sc == NSCENES) { finish(); return; }
        load_pct = 255; load_cells = 0;
        lstate = 0; ix = 0;
        scene(sc);
        return;
    }
    if (sc == SC_LOAD) load_anim();
    else if (sc == SC_PRESS) stars_anim();
    else if (sc == SC_LOGO) logo_anim();
}
