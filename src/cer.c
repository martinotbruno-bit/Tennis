/* Royal Martinot — séquences animées du match (banque 13, version 2.52).
 *  - entrée des joueurs : ils sortent du tunnel (en bas à droite), longent
 *    le court et gagnent leur place, saluent le public, puis l'arbitre lance
 *    une pièce : « PILE ! » ou « FACE ! », et l'équipe qui sert est annoncée ;
 *  - changement de côté : les joueurs vont s'asseoir sur leur chaise, près
 *    de l'arbitre (serviette sur les épaules, gourde), puis rejoignent leur
 *    nouvelle place en passant à côté du poteau du filet ;
 *  - fin du match : les vainqueurs tombent à genoux, poings levés, puis
 *    s'allongent sur le dos, raquette lâchée ; tous vont au filet se serrer
 *    la main, puis saluent l'arbitre.
 * A ou START passe la séquence. Les poses viennent du jeu de tuiles F (zone
 * partagée, chargé pendant les séquences : extras.c) ; les accessoires
 * (chaise, gourde, serviette, raquette au sol) aussi ; la pièce est la
 * tuile de la balle. cer_skip (mis à 1 par les tests à chaque trame) passe
 * toutes les séquences.
 * Variables : RAM de cartouche libre (bloc SAVEDATA, non sauvegardé ; la RAM
 * interne est pleine). */
#include "game_int.h"
#include "rules.h"
#include "court.h"
#include "gfx.h"
#include "audio.h"
#include "assets.h"

#pragma static-locals(off)

extern const unsigned char prop_tile13;   /* assets_swap.c (cette banque) */

#define PXU(v) ((u8)(((u16)(v) << 2) >> 8))      /* pixels, valeur positive */

#define CER_ENTRY  1    /* marche depuis le tunnel */
#define CER_WAVE   2    /* salut au public */
#define CER_TOSS   3    /* pièce lancée par l'arbitre */
#define CER_CHAIR  4    /* changement de côté : vers les chaises */
#define CER_SEAT   5    /* assis */
#define CER_BACK   6    /* vers la nouvelle place */
#define CER_JOY    7    /* fin du match : à genoux, puis allongé */
#define CER_NET    8    /* vers le filet */
#define CER_SHAKE  9    /* poignée de main */
#define CER_SALUTE 10   /* salut à l'arbitre */
#define CER_PEND   0xFE /* changement de côté annoncé (positions retenues) */

#define P_IDLE 0
#define P_RUN1 1
#define P_RUN2 2
#define P_SERVE 5
#define P_RUN3 19
#define P_RUN4 20
#define FLIP 0x80

#define TUNNEL_X  216   /* tunnel et couloir : à droite du court */
#define CHAIR_X   46    /* chaises : à côté de l'arbitre */

#pragma bss-name(push, "SAVEDATA")
u8 cer, cer_skip, pose_force[4];
u16 cer_t;
static s16 tx[4], ty[4], sx[4], sy[4], wx[4];
static u8 stage[4], toss_res, qx, qy;
static char nm[5], who[12];
#pragma bss-name(pop)

extern u8 pl_walk[4], pl_moving[4], npl, msg_timer;
void set_msg(const char *a, const char *b, u8 t);
void team_name(u8 t, char *out);
void __fastcall__ proj2(u8 x, u8 y);
extern u8 proj_x, proj_y;
#pragma zpsym("proj_x")
#pragma zpsym("proj_y")

static const u8 run4[4] = { P_RUN1, P_RUN3, P_RUN2, P_RUN4 };

static void all_free(void)
{
    u8 p;
    for (p = 0; p < 4; ++p) { pose_force[p] = 0xFF; pl_moving[p] = 0; }
}

/* un pas vers (gx, gy) : 2 px par trame au plus sur chaque axe ; 1 si arrivé */
static u8 step_to(u8 p, s16 gx, s16 gy)
{
    s16 dx = gx - plx[p], dy = gy - ply[p];
    u8 f = 0;
    if (dx > 128) dx = 128; else if (dx < -128) dx = -128;
    if (dy > 128) dy = 128; else if (dy < -128) dy = -128;
    plx[p] += dx; ply[p] += dy;
    if (!dx && !dy) { pl_moving[p] = 0; return 1; }
    pl_moving[p] = 1;
    ++pl_walk[p];
    if (dx < 0) f = FLIP;
    pose_force[p] = (u8)(run4[(pl_walk[p] >> 3) & 3] | f);
    return 0;
}
/* trajet en 2 temps : jusqu'au couloir (wx, ty), puis jusqu'à (tx, ty) */
static u8 walk(u8 p)
{
    if (stage[p] == 0) {
        if (step_to(p, wx[p], ty[p])) stage[p] = 1;
        return 0;
    }
    if (stage[p] == 1 && step_to(p, tx[p], ty[p])) { stage[p] = 2; pose_force[p] = P_IDLE; }
    return (u8)(stage[p] == 2);
}

/* fin de séquence : chacun à sa place */
static void finish(void)
{
    u8 p;
    if (cer >= CER_ENTRY && cer <= CER_BACK)
        for (p = 0; p < npl; ++p) { plx[p] = tx[p]; ply[p] = ty[p]; }
    all_free();
    cer = 0;
}

/* ---------------------------------------------------------------------- */
/* Entrée des joueurs (début du match) */
void cer_entry(void)
{
    u8 p;
    cer_skip = 0;
    all_free();
    cer = 0;
    if (cfg.mode != GM_MATCH || cfg.demo) return;
    for (p = 0; p < npl; ++p) {
        tx[p] = plx[p]; ty[p] = ply[p];
        wx[p] = TUNNEL_X * FP;
        plx[p] = TUNNEL_X * FP; ply[p] = 236 * FP;
        stage[p] = 0;
        pose_force[p] = P_IDLE;
    }
    cer = CER_ENTRY;
    cer_t = 0;
    msg_timer = 0;
}

/* Changement de côté : positions avant (cer_save) et après (cer_change)
   le placement pour le service */
void cer_save(void)
{
    u8 p;
    if (cfg.mode != GM_MATCH || cfg.demo) return;
    for (p = 0; p < npl; ++p) { sx[p] = plx[p]; sy[p] = ply[p]; }
    cer = CER_PEND;
}
void cer_change(void)
{
    u8 p;
    static const u8 chair_top[4] = { 100, 100, 88, 88 }, chair_bot[4] = { 158, 158, 170, 170 };
    if (cer != CER_PEND) return;
    for (p = 0; p < npl; ++p) {
        tx[p] = plx[p]; ty[p] = ply[p];
        plx[p] = sx[p]; ply[p] = sy[p];
        /* chaise du côté où le joueur se trouve (sans traverser le filet) */
        sx[p] = CHAIR_X * FP;
        sy[p] = (s16)((PX(ply[p]) < NET_Y ? chair_top[p] : chair_bot[p]) * FP);
        wx[p] = CHAIR_X * FP;
        stage[p] = 0;
    }
    cer = CER_CHAIR;
    cer_t = 0;
}

/* Fin du match */
void cer_end(void)
{
    u8 p;
    if (cfg.mode != GM_MATCH || cfg.demo) return;
    for (p = 0; p < npl; ++p) stage[p] = 0;
    cer = CER_JOY;
    cer_t = 0;
}

/* ---------------------------------------------------------------------- */
void cer_tick(void)
{
    u8 p, done = 1, win, top;
    if (cer == CER_PEND) return;
    ++cer_t;
    if (cer_skip || ((pad1_new | pad2_new) & (PAD_A | PAD_START))) {
        /* passer : fin du match, il reste peu de temps (sauf pour les
           tests, qui gardent la durée d'avant) */
        if (cer >= CER_JOY && !cer_skip && ms_timer > 20) ms_timer = 20;
        finish();
        return;
    }
    switch (cer) {
    case CER_ENTRY:
        for (p = 0; p < npl; ++p)
            if (cer_t < (u16)(p * 16)) { pose_force[p] = P_IDLE; done = 0; }   /* sorties échelonnées */
            else if (!walk(p)) done = 0;
        if (done) { cer = CER_WAVE; cer_t = 0; sfx_crowd(6); }
        break;
    case CER_WAVE:
        /* salut : raquette levée, agitée */
        for (p = 0; p < npl; ++p) pose_force[p] = (u8)((cer_t >> 3) & 1 ? P_SERVE : P_IDLE);
        if (cer_t >= 48) { cer = CER_TOSS; cer_t = 0; all_free(); for (p = 0; p < npl; ++p) pose_force[p] = P_IDLE; toss_res = (u8)(rand8() & 1); }
        break;
    case CER_TOSS:
        if (cer_t == 44) {
            team_name(sc_server, nm); nm[4] = 0;
            for (p = 0; p < 4 && nm[p]; ++p) who[p] = nm[p];
            who[p++] = ' '; who[p++] = 'S'; who[p++] = 'E'; who[p++] = 'R'; who[p++] = 'T'; who[p] = 0;
            set_msg(toss_res ? "PILE !" : "FACE !", who, 70);
            sfx_clap();
        }
        if (cer_t >= 110) { all_free(); cer = 0; }
        break;
    case CER_CHAIR:
        for (p = 0; p < npl; ++p)
            if (stage[p] < 2) {
                if (step_to(p, sx[p], sy[p])) { stage[p] = 2; pose_force[p] = POSE_SIT; }
                else done = 0;
            }
        if (done) { cer = CER_SEAT; cer_t = 0; }
        break;
    case CER_SEAT:
        for (p = 0; p < npl; ++p) pose_force[p] = POSE_SIT;
        if (cer_t >= 130) {
            cer = CER_BACK; cer_t = 0;
            for (p = 0; p < npl; ++p) stage[p] = 0;
        }
        break;
    case CER_BACK:
        for (p = 0; p < npl; ++p) if (!walk(p)) done = 0;
        if (done) finish();
        break;
    case CER_JOY:
        /* 0-59 : fête habituelle ; 60-129 : vainqueurs à genoux, poings
           levés ; 130-199 : allongés sur le dos, raquette lâchée */
        for (p = 0; p < npl; ++p) {
            win = (u8)((p & 1) == sc_winner);
            if (cer_t < 60) pose_force[p] = 0xFF;
            else if (!win) pose_force[p] = P_IDLE;
            else pose_force[p] = cer_t < 130 ? POSE_VICTORY : POSE_SUPINE;
        }
        if (cer_t >= 200) {
            /* au filet : bas à gauche du centre, haut à droite (les mains se
               rejoignent au-dessus du filet) ; en double, par paires */
            for (p = 0; p < npl; ++p) {
                top = (u8)(((p & 1) ^ sc_swap) != 0);
                tx[p] = (s16)((CENTER_X + (top ? 7 : -7) - (p >= 2 ? 26 : 0)) * FP);
                ty[p] = (s16)((top ? NET_Y - 4 : NET_Y + 9) * FP);
                stage[p] = 1;
            }
            cer = CER_NET; cer_t = 0;
        }
        break;
    case CER_NET:
        for (p = 0; p < npl; ++p) if (!walk(p)) done = 0;
        if (done) { cer = CER_SHAKE; cer_t = 0; sfx_crowd(4); }
        break;
    case CER_SHAKE:
        for (p = 0; p < npl; ++p) {
            top = (u8)(((p & 1) ^ sc_swap) != 0);
            pose_force[p] = (u8)(POSE_SHAKE | (top ? FLIP : 0));
        }
        if (cer_t >= 50) { cer = CER_SALUTE; cer_t = 0; }
        break;
    case CER_SALUTE:
        /* tournés vers l'arbitre (à gauche), raquette levée */
        for (p = 0; p < npl; ++p) pose_force[p] = (u8)(((cer_t >> 3) & 1 ? P_SERVE : P_IDLE) | FLIP);
        if (cer_t >= 60) { all_free(); cer = 0; }
        break;
    }
}

/* ---------------------------------------------------------------------- */
/* Accessoires : derrière les joueurs (cer_draw, après eux dans l'OAM) ou
   devant (cer_front, avant eux) */
static void at(u8 p)
{
    qx = PXU(plx[p]); qy = PXU(ply[p]);
    if (persp) { proj2(qx, qy); qx = proj_x; qy = proj_y; }
}
void cer_front(void)
{
    u8 p;
    if (cer != CER_SEAT) return;
    for (p = 0; p < npl; ++p) {
        at(p);
        spr1((u8)(qx - 4), (u8)(qy - 14), (u8)(prop_tile13 + 2), 2);          /* serviette sur les épaules */
        if ((u8)(cer_t + p * 40) % 80 < 30)
            spr1((u8)(qx + 4), (u8)(qy - 20), (u8)(prop_tile13 + 1), 2);      /* elle boit à la gourde */
    }
}
void cer_draw(void)
{
    u8 p, h, t;
    if (cer == CER_TOSS && cer_t < 44) {
        /* pièce lancée par l'arbitre : monte, retombe, tourne */
        t = (u8)cer_t;
        h = (u8)((t * (u8)(44 - t)) >> 4);
        spr1(44, (u8)(110 - h), (u8)(ball_base + ((t >> 2) & 1 ? 12 : 8)), 2);   /* pile (grande balle) / tranche (balle écrasée) */
    }
    if (cer == CER_SEAT || cer == CER_CHAIR) {
        for (p = 0; p < npl; ++p) {
            at(p);
            if (cer == CER_SEAT) {
                spr1((u8)(qx - 12), (u8)(qy - 10), prop_tile13, 2);             /* chaise (dossier, derrière) */
                if ((u8)(cer_t + p * 40) % 80 >= 30)
                    spr1((u8)(qx + 7), (u8)(qy - 7), (u8)(prop_tile13 + 1), 2);   /* gourde posée */
            }
        }
    }
    if (cer == CER_JOY && cer_t >= 130)
        for (p = 0; p < npl; ++p)
            if ((p & 1) == sc_winner) { at(p); spr1((u8)(qx + 10), (u8)(qy - 6), (u8)(prop_tile13 + 3), 2); }   /* raquette lâchée */
}
