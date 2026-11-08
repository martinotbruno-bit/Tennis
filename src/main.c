/* Pixel Open — boucle principale (banque fixe).
 * Chaque trame : attente de la NMI, lecture des manettes, son, puis la
 * logique de l'écran courant dans sa banque. Les changements d'écran entre
 * banques passent par go_req, traité ici. */
#include "app.h"
#include "audio.h"

#pragma static-locals(on)

u8 app_state, go_req, go_arg;
u8 is_pal, pal_acc;

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
    audio_update();
    switch (app_state) {
    case AS_MATCH:
        set_bank(BANK_GAME);
        if (match_tick()) { go_req = GO_SCREEN; go_arg = SCR_AFTER_MATCH; }
        else match_draw();
        break;
    case AS_MANUAL:
        set_bank(BANK_MANUAL);
        manual_tick();
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
    go_req = GO_SCREEN;
    go_arg = SCR_TITLE;
    handle_go();
    for (;;) {
        wait_frame();
        pad_poll();
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
