/* Pixel Open — boucle principale, écrans titre, configuration, tournoi,
 * codes de sauvegarde et résultats. */
#include "runtime.h"
#include "gfx.h"
#include "game.h"
#include "rules.h"
#include "audio.h"
#include "assets.h"
#include "password.h"

#pragma static-locals(on)

/* état global du programme (consulté aussi par les tests automatiques) */
u8 app_state;
#define AS_TITLE   0
#define AS_MENU    1
#define AS_MATCH   2
#define AS_RESULTS 3
#define AS_TOURNEY 4
#define AS_CODE    5
u8 is_pal, pal_acc;

/* options du menu */
#define OPT_START 0
#define OPT_MODE  1
#define OPT_P1    2
#define OPT_P2    3
#define OPT_LEVEL 4
#define OPT_SURF  5
#define OPT_TIME  6
#define OPT_FMT   7
#define NOPT      8
#define MODE_1P   0
#define MODE_2P   1
#define MODE_TOUR 2
#define MODE_TR_RALLY 3
#define MODE_TR_SERVE 4
#define MODE_DEMO 5
#define NMODES    6
static const char * const mode_name[NMODES] = {
    "1 JOUEUR VS CPU", "2 JOUEURS      ", "TOURNOI        ",
    "ENTRAINEMENT   ", "SERVICE        ", "DEMO CPU VS CPU" };
static const char * const level_name[3] = { "FACILE", "NORMAL", "EXPERT" };
static const char * const fmt_name[4] = { "1 SET      ", "3 SETS     ", "5 SETS     ", "RAPIDE 4 JX" };
static const char * const opt_label[NOPT] = {
    "COMMENCER", "MODE", "JOUEUR 1", "JOUEUR 2", "NIVEAU CPU", "SURFACE", "MOMENT", "FORMAT" };
u8 opt_val[NOPT] = { 0, 0, 0, 1, 1, 0, 0, 0 };
static u8 opt_max[NOPT] = { 1, NMODES, NCHARS, NCHARS, 3, NSURF, 2, 4 };
static u8 cursor;

/* tournoi */
u8 tour_round, tour_wins;
static u8 tour_opp[3];
static const char * const round_name[3] = { "QUART DE FINALE", "DEMI-FINALE    ", "FINALE         " };
static u8 last_mode;

static void tick_frame(void)
{
    wait_frame();
    pad_poll();
}

/* ---------------------------------------------------------------------- */
static void draw_logo(u8 y)
{
    u8 i;
    vram_adr(NTADR(6, y));
    for (i = 0; i < 10; ++i) { vram_put(logo_tiles[i * 4]); vram_put(logo_tiles[i * 4 + 1]); }
    vram_adr(NTADR(6, y + 1));
    for (i = 0; i < 10; ++i) { vram_put(logo_tiles[i * 4 + 2]); vram_put(logo_tiles[i * 4 + 3]); }
}

static void set_attr_rows(u8 y0, u8 y1, u8 pal)
{
    /* attributs par bandes de 32 pixels (lignes de tuiles y0..y1 incluses) */
    u8 ay, ax, v = (u8)(pal | (pal << 2) | (pal << 4) | (pal << 6));
    for (ay = (u8)(y0 >> 2); ay <= (u8)(y1 >> 2); ++ay) {
        vram_adr((u16)(0x23C0 + ay * 8));
        for (ax = 0; ax < 8; ++ax) vram_put(v);
    }
}

static void screen_begin(void)
{
    ppu_off();
    vbuf_reset();
    clear_screen();
    pal_set_menu();
    spr_begin(); spr_end();
}

static void title_screen(void)
{
    u8 i;
    screen_begin();
    set_attr_rows(4, 7, 2);
    draw_logo(5);
    for (i = 0; i < 4; ++i) {
        vram_adr(NTADR(14, 10 + i));
        vram_put(bigball_tiles[i * 4]); vram_put(bigball_tiles[i * 4 + 1]);
        vram_put(bigball_tiles[i * 4 + 2]); vram_put(bigball_tiles[i * 4 + 3]);
    }
    set_attr_rows(8, 15, 2);
    txt_at(8, 17, "TENNIS 8 BITS");
    txt_at(7, 20, "START : JOUER");
    txt_at(7, 22, "SELECT : CODE TOURNOI");
    txt_at(6, 26, "(C) 2026 PIXEL OPEN");
    ppu_on();
    music_play(MUS_TITLE);
    app_state = AS_TITLE;
}

/* ---------------------------------------------------------------------- */
static void menu_value(u8 o, char *buf)
{
    const char *s = "";
    u8 v = opt_val[o], i;
    switch (o) {
    case OPT_START: s = ""; break;
    case OPT_MODE:  s = mode_name[v]; break;
    case OPT_P1:
    case OPT_P2:    s = char_name[v]; break;
    case OPT_LEVEL: s = level_name[v]; break;
    case OPT_SURF:  s = surf_name[v]; break;
    case OPT_TIME:  s = v ? "NUIT" : "JOUR"; break;
    case OPT_FMT:   s = fmt_name[v]; break;
    }
    for (i = 0; i < 15; ++i) buf[i] = *s ? *s++ : ' ';
    buf[15] = 0;
}

static void menu_draw_row(u8 o, u8 direct)
{
    static char buf[16];
    menu_value(o, buf);
    if (direct) { txt_at(3, (u8)(5 + o * 2), opt_label[o]); txt_at(15, (u8)(5 + o * 2), buf); }
    else vbuf_text(15, (u8)(5 + o * 2), buf, 15);
}

static void menu_screen(void)
{
    u8 o;
    screen_begin();
    txt_at(9, 2, "CONFIGURATION");
    for (o = 0; o < NOPT; ++o) menu_draw_row(o, 1);
    txt_at(3, 23, "HAUT/BAS : CHOISIR");
    txt_at(3, 24, "GAUCHE/DROITE : MODIFIER");
    txt_at(3, 25, "A OU START : JOUER");
    pal_set_player(0, opt_val[OPT_P1]);
    pal_set_player(1, opt_val[OPT_P2]);
    ppu_on();
    app_state = AS_MENU;
}

static void menu_sprites(void)
{
    spr_begin();
    spr1(8, (u8)(5 * 8 + cursor * 16), 3, 3);
    spr_person(200, 196, opt_val[OPT_P1], 1, (u8)((frame_cnt >> 4) & 1 ? 0 : 6), 0, 0);
    spr_person(232, 196, opt_val[OPT_P2], 1, (u8)((frame_cnt >> 4) & 1 ? 6 : 0), 1, 1);
    spr_end();
}

static void setup_cfg(u8 mode)
{
    u8 f = opt_val[OPT_FMT];
    cfg.surface = opt_val[OPT_SURF];
    cfg.night = opt_val[OPT_TIME];
    cfg.chr[0] = opt_val[OPT_P1];
    cfg.chr[1] = opt_val[OPT_P2];
    cfg.level[0] = cfg.level[1] = opt_val[OPT_LEVEL];
    cfg.sets_to_win = (u8)(f == 1 ? 2 : f == 2 ? 3 : 1);
    cfg.games_per_set = (u8)(f == 3 ? 4 : 6);
    cfg.ctrl[0] = CTRL_PAD1;
    cfg.ctrl[1] = CTRL_CPU;
    cfg.mode = GM_MATCH;
    switch (mode) {
    case MODE_2P: cfg.ctrl[1] = CTRL_PAD2; break;
    case MODE_TR_RALLY: cfg.mode = GM_TRAIN_RALLY; break;
    case MODE_TR_SERVE: cfg.mode = GM_TRAIN_SERVE; break;
    case MODE_DEMO: cfg.ctrl[0] = CTRL_CPU; cfg.level[0] = 2; cfg.level[1] = 1; break;
    case MODE_TOUR:
        cfg.chr[1] = tour_opp[tour_round];
        cfg.level[1] = tour_round;
        break;
    }
    last_mode = mode;
}

static void start_match(void)
{
    ppu_off();
    vbuf_reset();
    vram_adr(0x2000);
    vram_unrle(court_nt_rle);
    pal_set_court(cfg.surface, cfg.night);
    pal_set_player(0, cfg.chr[0]);
    pal_set_player(1, cfg.chr[1]);
    music_play(MUS_NONE);
    match_start();
    match_draw();
    ppu_on();
    sfx_whistle();
    app_state = AS_MATCH;
}

/* ---------------------------------------------------------------------- */
static void tour_pick_opponents(void)
{
    u8 i, c = 0, n = 0;
    /* adversaires : les personnages suivants dans la liste */
    for (i = 1; i < NCHARS && n < 3; ++i) {
        c = (u8)((opt_val[OPT_P1] + i * 1) % NCHARS);
        tour_opp[n++] = c;
    }
}

static void tourney_screen(void)
{
    static char code[7];
    screen_begin();
    txt_at(6, 3, "TOURNOI PIXEL OPEN");
    if (tour_round >= 3) {
        txt_at(9, 8, "VAINQUEUR !");
        txt_at(12, 11, char_name[opt_val[OPT_P1]]);
        txt_at(5, 14, "TROPHEES GAGNES :");
        { char b[3]; num2(b, tour_wins); b[2] = 0; txt_at(23, 14, b); }
        txt_at(5, 24, "A : RETOUR AU TITRE");
        music_play(MUS_VICTORY);
    } else {
        txt_at(8, 7, round_name[tour_round]);
        txt_at(8, 10, char_name[opt_val[OPT_P1]]);
        txt_at(13, 10, "VS");
        txt_at(16, 10, char_name[tour_opp[tour_round]]);
        txt_at(8, 12, surf_name[opt_val[OPT_SURF]]);
        txt_at(8, 13, level_name[tour_round]);
        txt_at(6, 17, "CODE DE REPRISE :");
        pw_encode(code, tour_round, opt_val[OPT_P1], opt_val[OPT_SURF], opt_val[OPT_FMT], opt_val[OPT_TIME], tour_wins);
        code[6] = 0;
        txt_at(12, 19, code);
        txt_at(5, 24, "A : JOUER   B : MENU");
    }
    ppu_on();
    app_state = AS_TOURNEY;
}

/* ---------------------------------------------------------------------- */
static char code_buf[7];
static u8 code_pos, code_err;
static void code_screen(void)
{
    u8 i;
    screen_begin();
    txt_at(7, 4, "CODE DE TOURNOI");
    for (i = 0; i < 6; ++i) code_buf[i] = PW_ALPHA[0];
    code_buf[6] = 0;
    code_pos = 0;
    code_err = 0;
    txt_at(12, 10, code_buf);
    txt_at(3, 20, "HAUT/BAS : LETTRE");
    txt_at(3, 21, "GAUCHE/DROITE : POSITION");
    txt_at(3, 22, "START : VALIDER  B : RETOUR");
    ppu_on();
    app_state = AS_CODE;
}

static void code_update(void)
{
    u8 i, idx = 0;
    for (i = 0; i < 16; ++i) if (PW_ALPHA[i] == code_buf[code_pos]) idx = i;
    if (pad1_new & PAD_UP)    { idx = (u8)((idx + 1) & 15); sfx_menu_move(); }
    if (pad1_new & PAD_DOWN)  { idx = (u8)((idx + 15) & 15); sfx_menu_move(); }
    code_buf[code_pos] = PW_ALPHA[idx];
    if ((pad1_new & PAD_RIGHT) && code_pos < 5) ++code_pos;
    if ((pad1_new & PAD_LEFT) && code_pos > 0) --code_pos;
    vbuf_reset();
    vbuf_text(12, 10, code_buf, 6);
    if (pad1_new & PAD_B) { title_screen(); return; }
    if (pad1_new & PAD_START) {
        u8 r, c, s, f, n, w;
        if (pw_decode(code_buf, &r, &c, &s, &f, &n, &w)) {
            opt_val[OPT_P1] = c; opt_val[OPT_SURF] = s; opt_val[OPT_FMT] = f;
            opt_val[OPT_TIME] = n; tour_wins = w; tour_round = r;
            opt_val[OPT_MODE] = MODE_TOUR;
            tour_pick_opponents();
            sfx_menu_ok();
            tourney_screen();
            return;
        }
        sfx_fault();
        vbuf_text(9, 13, "CODE INVALIDE", 13);
    }
    spr_begin();
    spr1((u8)(12 * 8 + code_pos * 8), 88, 3, 3);
    spr_end();
}

/* ---------------------------------------------------------------------- */
static void stat_row(u8 y, const char *label, u8 a, u8 b)
{
    char n[3];
    n[2] = 0;
    txt_at(2, y, label);
    num2(n, a); txt_at(19, y, n);
    num2(n, b); txt_at(26, y, n);
}

static u8 pct(u8 a, u8 b) { return b ? (u8)(((u16)a * 100) / b) : 0; }

static void results_screen(void)
{
    u8 i, w = sc_winner;
    char b[12];
    screen_begin();
    pal_set_player(0, cfg.chr[0]);
    pal_set_player(1, cfg.chr[1]);
    if (cfg.mode == GM_MATCH) {
        txt_at(8, 2, "VICTOIRE DE");
        txt_at(20, 2, char_name[cfg.chr[w]]);
        /* score des sets */
        for (i = 0; i <= sc_set_no && i < 5; ++i) {
            if (sc_hist[i][0] + sc_hist[i][1] == 0) break;
            b[0] = (char)('0' + sc_hist[i][0]); b[1] = '-'; b[2] = (char)('0' + sc_hist[i][1]); b[3] = 0;
            txt_at((u8)(6 + i * 5), 4, b);
        }
        txt_at(18, 7, char_name[cfg.chr[0]]);
        txt_at(25, 7, char_name[cfg.chr[1]]);
        stat_row(9, "POINTS GAGNES", st_points[0], st_points[1]);
        stat_row(11, "ACES", st_aces[0], st_aces[1]);
        stat_row(13, "DOUBLES FAUTES", st_dfaults[0], st_dfaults[1]);
        stat_row(15, "COUPS GAGNANTS", st_winners[0], st_winners[1]);
        stat_row(17, "FAUTES", st_errors[0], st_errors[1]);
        stat_row(19, "1ERES BALLES %", pct(st_first_in[0], st_first_tot[0]), pct(st_first_in[1], st_first_tot[1]));
        num2(b, st_longest); b[2] = 0;
        txt_at(2, 21, "ECHANGE LE PLUS LONG"); txt_at(26, 21, b);
        if (last_mode == MODE_TOUR) txt_at(4, 25, "A : CONTINUER");
        else txt_at(4, 25, "A : REVANCHE   B : MENU");
    }
    ppu_on();
    app_state = AS_RESULTS;
}

static void training_results(void)
{
    char b[3];
    screen_begin();
    txt_at(8, 6, "FIN DE L'EXERCICE");
    txt_at(6, 10, "POINTS :");
    num2(b, tr_score); b[2] = 0; txt_at(16, 10, b);
    txt_at(6, 12, "BALLES :");
    num2(b, tr_total); txt_at(16, 12, b);
    txt_at(4, 25, "A : RECOMMENCER B : MENU");
    ppu_on();
    app_state = AS_RESULTS;
}

/* ---------------------------------------------------------------------- */
static u16 idle_frames;

static void logic_tick(void)
{
    u8 o, v;
    audio_update();
    switch (app_state) {
    case AS_TITLE:
        if (pad1_new & PAD_START) { sfx_menu_ok(); menu_screen(); break; }
        if (pad1_new & PAD_SELECT) { sfx_menu_ok(); code_screen(); break; }
        if (++idle_frames > 1200) {     /* démo automatique après 20 s */
            idle_frames = 0;
            setup_cfg(MODE_DEMO);
            start_match();
        }
        break;
    case AS_MENU:
        if (pad1_new & PAD_UP)   { cursor = (u8)(cursor ? cursor - 1 : NOPT - 1); sfx_menu_move(); }
        if (pad1_new & PAD_DOWN) { cursor = (u8)(cursor + 1 == NOPT ? 0 : cursor + 1); sfx_menu_move(); }
        o = cursor;
        if (o != OPT_START && (pad1_new & (PAD_LEFT | PAD_RIGHT))) {
            v = opt_val[o];
            if (pad1_new & PAD_RIGHT) v = (u8)(v + 1 == opt_max[o] ? 0 : v + 1);
            else v = (u8)(v ? v - 1 : opt_max[o] - 1);
            opt_val[o] = v;
            vbuf_reset();
            menu_draw_row(o, 0);
            if (o == OPT_P1) pal_set_player(0, v);
            if (o == OPT_P2) pal_set_player(1, v);
            sfx_menu_move();
        }
        if (pad1_new & PAD_B) { title_screen(); break; }
        if ((pad1_new & PAD_START) || ((pad1_new & PAD_A) && o == OPT_START)) {
            sfx_menu_ok();
            if (opt_val[OPT_MODE] == MODE_TOUR) {
                tour_round = 0;
                tour_pick_opponents();
                tourney_screen();
            } else {
                setup_cfg(opt_val[OPT_MODE]);
                start_match();
            }
            break;
        }
        menu_sprites();
        break;
    case AS_MATCH:
        if (last_mode == MODE_DEMO && (pad1_new & (PAD_START | PAD_A))) { title_screen(); break; }
        if (match_tick()) {
            if (match_quit) { music_play(MUS_NONE); menu_screen(); break; }
            if (last_mode == MODE_DEMO) { title_screen(); break; }
            if (cfg.mode == GM_MATCH) results_screen(); else training_results();
            break;
        }
        /* entraînement : 20 balles par exercice */
        if (cfg.mode != GM_MATCH && tr_total >= 20 && pad1_new == 0) {
            static u8 wait;
            if (++wait > 120) { wait = 0; training_results(); break; }
        }
        match_draw();
        break;
    case AS_RESULTS:
        if (pad1_new & PAD_A) {
            if (last_mode == MODE_TOUR) {
                if (sc_winner == 0) {
                    ++tour_round;
                    if (tour_round >= 3 && tour_wins < 15) ++tour_wins;
                    tourney_screen();
                } else {
                    /* élimination : retour au titre */
                    title_screen();
                }
            } else { setup_cfg(last_mode); start_match(); }
        }
        if (pad1_new & PAD_B) menu_screen();
        break;
    case AS_TOURNEY:
        if (pad1_new & PAD_A) {
            if (tour_round >= 3) { title_screen(); break; }
            setup_cfg(MODE_TOUR);
            start_match();
        }
        if ((pad1_new & PAD_B) && tour_round < 3) menu_screen();
        break;
    case AS_CODE:
        code_update();
        break;
    }
}

void main(void)
{
    /* NTSC ~1860 boucles entre deux VBlank, PAL/Dendy > 2050 */
    is_pal = (u8)(region_cnt > 1970);
    title_screen();
    for (;;) {
        tick_frame();
        logic_tick();
        if (is_pal) {
            /* 50 Hz : une trame logique supplémentaire toutes les 5 trames
               pour conserver la vitesse de jeu du 60 Hz */
            if (++pal_acc == 5) {
                pal_acc = 0;
                pad1_new = pad2_new = 0;
                logic_tick();
            }
        }
    }
}
