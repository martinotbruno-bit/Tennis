/* Royal Martinot — boucle principale (banque fixe).
 * Chaque trame : attente de la NMI, lecture des manettes, son, puis la
 * logique de l'écran courant dans sa banque. Les changements d'écran entre
 * banques passent par go_req, traité ici. */
#include "app.h"
#include "audio.h"
#include "game.h"

#pragma static-locals(on)

u8 app_state, go_req, go_arg;
u8 is_pal, pal_acc;
static u8 spd_acc, keep1, keep2, no_draw;

static void handle_go(void)
{
    u8 r;
    while (go_req) {
        r = go_req;
        go_req = GO_NONE;
        switch (r) {
        case GO_MATCH:
            set_bank(BANK_GAME);
            match_begin();
            app_state = AS_MATCH;
            break;
        case GO_SCREEN:
            set_bank(BANK_SCREENS);
            app_state = AS_SCREENS;
            screens_enter(go_arg);
            break;
        case GO_RESUME:
            set_bank(BANK_GAME);
            match_resume();
            app_state = AS_MATCH;
            break;
        case GO_INTRO:
            set_bank(BANK_INTRO);
            app_state = AS_INTRO;
            intro_begin();
            break;
        case GO_CREDITS:
            set_bank(BANK_CREDITS);
            app_state = AS_CREDITS;
            credits_begin();
            break;
        case GO_MANUAL:
            set_bank(BANK_MANUAL);
            app_state = AS_MANUAL;
            manual_begin();
            break;
        }
    }
}

static void logic_tick(void)
{
    u8 r;
    /* fondu d'ouverture : un cran plus clair toutes les 2 trames */
    if (fade_lvl && ppu_mask && !(frame_cnt & 1)) {
        fade_lvl = (u8)(fade_lvl - 16); pal_dirty = 1;
        /* menus : l'écran glisse vers sa place pendant qu'il s'éclaire */
        if (app_state == AS_SCREENS) scroll_y = (u8)(fade_lvl >> 2);
    }
    /* le son avance dans la NMI (audio_tick), à chaque trame */
    switch (app_state) {
    case AS_MATCH:
        set_bank(BANK_GAME);
        r = match_tick();
        if (r) { go_req = GO_SCREEN; go_arg = (u8)(r == 2 ? SCR_SETBREAK : SCR_AFTER_MATCH); }
        else if (!no_draw) match_draw();   /* trame logique ajoutée : pas de dessin */
        break;
    case AS_INTRO:
        set_bank(BANK_INTRO);
        intro_tick();
        break;
    case AS_MANUAL:
        set_bank(BANK_MANUAL);
        manual_tick();
        break;
    case AS_CREDITS:
        set_bank(BANK_CREDITS);
        credits_tick();
        break;
    default:
        set_bank(BANK_SCREENS);
        screens_tick();
        break;
    }
    handle_go();
}

void main(void)
{
    /* NTSC ~1860 boucles entre deux VBlank, PAL/Dendy > 2050 */
    is_pal = (u8)(region_cnt > 1970);
    go_req = GO_INTRO;                 /* générique, puis écran titre */
    handle_go();
    for (;;) {
        wait_frame();
        pad_poll();
        /* hasard : hors du match, le générateur avance à chaque trame et
           chaque appui y mêle l'instant où il a lieu ; le temps passé dans
           les menus, jamais le même, rend le tirage au sort, le premier
           serveur et les échanges imprévisibles d'une partie à l'autre */
        if (app_state != AS_MATCH) {
            rand8();
            if (pad1_new | pad2_new) rand_seed ^= frame_cnt;
        }
        /* vitesse du jeu (match) : « lent » saute une trame logique sur 4,
           « rapide » en ajoute une toutes les 4 trames ; les appuis de la
           trame sautée sont gardés pour la suivante */
        if (app_state == AS_MATCH && cfg.gspeed != 1 && ++spd_acc >= 4) {
            spd_acc = 0;
            if (cfg.gspeed == 0) {
                keep1 |= pad1_new; keep2 |= pad2_new;
                continue;
            }
            no_draw = 1;
            logic_tick();
            no_draw = 0;
            pad1_new = pad2_new = 0;
        }
        pad1_new |= keep1; pad2_new |= keep2;
        keep1 = keep2 = 0;
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
