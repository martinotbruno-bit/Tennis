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

extern const unsigned char prop_tile13, chair_tile13;   /* assets_swap.c (cette banque) : gourde, serviette, raquette ; chaise */

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
#define CER_CHAIRS 0xFD /* fin de l'échange qui amène un changement de côté : les chaises apparaissent */

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
static char nm[5], who[14], who2[14];
#pragma bss-name(pop)

extern u8 pl_walk[4], pl_moving[4], npl, msg_timer;
void set_msg(const char *a, const char *b, u8 t);
void team_name(u8 t, char *out);
void __fastcall__ proj2(u8 x, u8 y);
extern u8 proj_x, proj_y;
#pragma zpsym("proj_x")
#pragma zpsym("proj_y")

static const u8 run4[4] = { P_RUN1, P_RUN3, P_RUN2, P_RUN4 };

extern u8 view_x[4];              /* extras.c : vue (face / dos) inversée par sprp.s */
static void all_free(void)
{
    u8 p;
    for (p = 0; p < 4; ++p) { pose_force[p] = 0xFF; pl_moving[p] = 0; view_x[p] = 0; }
}

/* un pas vers (gx, gy) : 2 px par trame au plus sur chaque axe ; 1 si arrivé */
static u8 step_to(u8 p, s16 gx, s16 gy)
{
    s16 dx = gx - plx[p], dy = gy - ply[p];
    u8 f = 0;
    if (dx > 128) dx = 128; else if (dx < -128) dx = -128;
    if (dy > 128) dy = 128; else if (dy < -128) dy = -128;
    plx[p] += dx; ply[p] += dy;
    if (!dx && !dy) { pl_moving[p] = 0; view_x[p] = 0; return 1; }
    pl_moving[p] = 1;
    /* vu de dos quand il s'éloigne (vers le haut), de face quand il vient
       vers nous (sinon le joueur qui monte au fond marchait à reculons) ;
       en marche latérale, la vue ne change pas */
    if (dy) view_x[p] = (u8)((dy > 0) ^ (((p & 1) ^ sc_swap) != 0));
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
    static const u8 chair_top[4] = { 100, 100, 88, 88 }, chair_bot[4] = { 158, 158, 170, 170 };   /* (comme chair_y) */
    if (cer != CER_PEND) return;
    for (p = 0; p < npl; ++p) {
        tx[p] = plx[p]; ty[p] = ply[p];
        plx[p] = sx[p]; ply[p] = sy[p];
        /* chaise du côté où le joueur se trouve (sans traverser le filet) */
        sx[p] = CHAIR_X * FP;
        sy[p] = (s16)((PX(ply[p]) < NET_Y ? chair_top[p] : chair_bot[p]) * FP);
        wx[p] = CHAIR_X * FP;
        stage[p] = 0;
        pose_force[p] = P_IDLE;     /* (fin de fête : sa pose n'a plus ses tuiles, jeu F chargé) */
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
/* Le point qui vient de finir amène-t-il un changement de côté (sans fin
   de set, qui a son écran) ? Mêmes règles que rules_point. */
extern u8 pending_winner, end_reason;
static u8 change_next(u8 w)
{
    u8 o = (u8)(w ^ 1), pw = (u8)(sc_points[w] + 1), g, n;
    if (sc_over) return 0;
    if (sc_tiebreak) {
        if (pw >= 7 && pw >= sc_points[o] + 2) return 0;
        n = (u8)(sc_points[0] + sc_points[1] + 1);
        while (n >= 6) n -= 6;
        return (u8)(n == 0);
    }
    if (pw < 4 || pw < sc_points[o] + 2) return 0;
    g = (u8)(sc_games[w] + 1);
    if (g >= sc_games_per_set && g >= sc_games[o] + 2) return 0;
    return (u8)((g + sc_games[o]) & 1);
}
/* à chaque trame (swap_tick) : les chaises apparaissent à la fin de
   l'échange qui amène un changement de côté ; s'il y a un ralenti,
   seulement après lui (version 2.59 : ni avant ni pendant le ralenti) */
void cer_watch(void)
{
    if (cer == CER_CHAIRS && ms != MS_DEAD) cer = 0;
    if (cer || ms != MS_DEAD || rp_pending || cfg.mode != GM_MATCH || cfg.demo || end_reason != 2) return;
    if (change_next(pending_winner)) cer = CER_CHAIRS;
}

/* « ALEX : PILE » : nom de l'équipe t et ce que l'arbitre lui attribue */
static void name_side(u8 t, char *out)
{
    u8 i;
    const char *w = (u8)(toss_res ^ t) ? "FACE" : "PILE";
    team_name(t, nm); nm[4] = 0;
    for (i = 0; i < 4 && nm[i]; ++i) out[i] = nm[i];
    out[i++] = ' '; out[i++] = ':'; out[i++] = ' ';
    while (*w) out[i++] = *w++;
    out[i] = 0;
}

void cer_tick(void)
{
    u8 p, done = 1, win, top;
    if (cer >= CER_CHAIRS) return;
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
        /* toss_res : ce que l'arbitre attribue à l'équipe 0 (0 pile, 1 face) */
        break;
    case CER_TOSS:
        /* tirage au sort : l'arbitre attribue pile ou face à chaque joueur
           (au hasard), lance la pièce, et annonce le résultat ; le gagnant
           sert (le premier serveur, tiré au hasard au début du match : la
           pièce tombe du côté qu'on lui a attribué) */
        if (cer_t == 1) {
            name_side(0, who); name_side(1, who2);
            set_msg(who, who2, 64);
        }
        if (cer_t == 104) {
            team_name(sc_server, nm); nm[4] = 0;
            for (p = 0; p < 4 && nm[p]; ++p) who[p] = nm[p];
            who[p++] = ' '; who[p++] = 'S'; who[p++] = 'E'; who[p++] = 'R'; who[p++] = 'T'; who[p] = 0;
            set_msg((u8)(toss_res ^ sc_server) ? "FACE !" : "PILE !", who, 70);
            sfx_clap();
        }
        if (cer_t >= 175) { all_free(); cer = 0; }
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
        /* assis de profil, tourné vers le court (à droite) */
        spr1((u8)(qx - 4), (u8)(qy - 14), (u8)(prop_tile13 + 1), 2);        /* serviette autour du cou */
        if ((u8)(cer_t + p * 40) % 80 < 30)
            spr1((u8)(qx + 2), (u8)(qy - 21), prop_tile13, 2);              /* il boit à la gourde */
    }
}
/* chaises des changements de côté : à côté de l'arbitre, une par joueur
   (dès la fin de l'échange, puis pendant la marche et la pause) */
static const u8 chair_y[4] = { 100, 158, 88, 170 };
void cer_draw(void)
{
    u8 p, h, t;
    if (cer == CER_TOSS && cer_t >= 60 && cer_t < 104) {
        /* pièce lancée par l'arbitre : monte, retombe, tourne */
        t = (u8)(cer_t - 60);
        h = (u8)((t * (u8)(44 - t)) >> 4);
        spr1(44, (u8)(110 - h), (u8)(ball_base + ((t >> 2) & 1 ? 12 : 8)), 2);   /* pile (grande balle) / tranche (balle écrasée) */
    }
    if (cer == CER_CHAIRS || cer == CER_CHAIR || cer == CER_SEAT) {
        for (p = 0; p < (u8)(npl == 4 ? 4 : 2); ++p) {
            qx = CHAIR_X; qy = chair_y[p];
            if (persp) { proj2(qx, qy); qx = proj_x; qy = proj_y; }
            spr1((u8)(qx - 5), (u8)(qy - 8), chair_tile13, 2);              /* chaise (de profil, derrière) */
        }
        if (cer == CER_SEAT)
            for (p = 0; p < npl; ++p)
                if ((u8)(cer_t + p * 40) % 80 >= 30) { at(p); spr1((u8)(qx + 8), (u8)(qy - 7), prop_tile13, 2); }   /* gourde posée */
    }
    if (cer == CER_JOY && cer_t >= 130)
        for (p = 0; p < npl; ++p)
            if ((p & 1) == sc_winner) { at(p); spr1((u8)(qx + 10), (u8)(qy - 6), (u8)(prop_tile13 + 2), 2); }   /* raquette lâchée */
}

/* ---------------------------------------------------------------------- */
/* Bustes « bras caché » pour le joueur vu de dos (version 2.52). Dans les
 * gestes où un bras passe devant le buste (préparations, revers, volée,
 * impact du service), le dessin commun aux deux vues montre le bras : juste
 * de face, mais de dos il barrerait le dos. Pour chaque joueur vu de dos
 * dans un de ces gestes, les 2 tuiles de son buste « bras caché » sont
 * chargées dans une case (2 cases à la fin de la zone partagée, libres
 * pendant l'échange : jeu R complet) ; row1_ovr dit à spr_person (sprp.s)
 * d'utiliser la case pour la rangée du buste. Le chargement passe par le
 * tampon VRAM (swap_put) : tuiles et sprites changent dans la même image.
 * Appelé après le calcul des poses (match_draw) et pendant les ralentis. */
extern const unsigned char xrow1[], xpose_idx[], slot_tile13;
extern u8 pose_now[4], swap_on, swap_nude, row1_ovr[4];
extern u16 swp_dst;

u8 __fastcall__ swap_put(const u8 *src);
#pragma bss-name(push, "SAVEDATA")
static u8 slot_have[2];
#pragma bss-name(pop)
void slot_tick(void)
{
    u8 p, k, s, want, t;
    const u8 *d;
    if (swap_on != 2 || swap_nude) {
        slot_have[0] = slot_have[1] = 0xFF;
        row1_ovr[0] = row1_ovr[1] = row1_ovr[2] = row1_ovr[3] = 0;
        return;
    }
    for (p = 0; p < 4; ++p) {
        row1_ovr[p] = 0;
        if (p >= npl || ((p & 1) ^ sc_swap)) continue;         /* vu de face (en haut) */
        k = xpose_idx[pose_now[p] & 31];
        if (k == 0xFF) continue;
        want = (u8)(k * 2 + (char_female[cfg.chr[p]] ? 1 : 0));
        s = (u8)(p >> 1);
        t = (u8)(slot_tile13 + s * 2);
        if (slot_have[s] != want) {
            d = xrow1 + (u16)want * 32;
            swp_dst = 0x1000 + ((u16)t << 4);
            if (!swap_put(d)) continue;                         /* tampon plein : l'image suivante */
            swp_dst += 16;
            if (!swap_put(d + 16)) { slot_have[s] = 0xFF; continue; }
            slot_have[s] = want;
        }
        row1_ovr[p] = t;
    }
}
