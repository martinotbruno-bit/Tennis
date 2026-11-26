/* Royal Martinot — (3.02) fonctions de jeu en banque 29 (BANK_PICK, place
 * libre) : messages de timing de l'entraînement, service de l'entraînement,
 * choix des coups de l'ordinateur. */
#include "game_int.h"
#include "app.h"
#include "rules.h"
#include "court.h"
#include "gfx.h"
#include "audio.h"
void set_msg(const char *a, const char *b, u8 t);   /* banque fixe */
#pragma static-locals(push, off)

/* ---------------------------------------------------------------------- */
/* (3.02, déplacés de shots.c, banque 13 pleine) Entraînement : écart au  */
/* bon moment, en images (trames) ; service frappé avec A : écart au      */
/* sommet du lancer (vz = 4 ou -2 au sommet ; -6 par trame)               */
/* ---------------------------------------------------------------------- */
#pragma bss-name(push, "WBSS")
static char timing_b[9];
#pragma bss-name(pop)
u16 __fastcall__ timing_f(u16 a)
{
    char *b = timing_b;
    u8 n = (u8)(a >> 8);
    if (n > 9) n = 9;
    b[0] = (char)('0' + n);
    b[1] = ' '; b[2] = 'I'; b[3] = 'M'; b[4] = 'A'; b[5] = 'G'; b[6] = 'E';
    b[7] = (char)(n > 1 ? 'S' : 0); b[8] = 0;
    set_msg((u8)a ? "TROP TOT DE" : "TROP TARD DE", b, 45);
    return 0;
}
u16 __fastcall__ serve_fb(u16 a)
{
    s16 vz = (s16)a;
    if (vz > 4) timing_f((u16)(1 | ((u16)((u8)(vz - 4) / 6) << 8)));
    else if (vz < -2) timing_f((u16)((u8)((u8)(-2 - vz) / 6) << 8));
    return 0;
}

/* ---------------------------------------------------------------------- */
/* (3.02) Coup de l'ordinateur : rend la croix (octet bas) et le bouton    */
/* (octet haut). a : joueur, et en octet haut sa probabilité de lob contre */
/* un volleyeur (style, IA adaptative).                                    */
/*  - direction : loin de l'adversaire (ou du trou, en double) ; parfois   */
/*    sans direction, au centre ou dans le corps (Normal, Expert) ;        */
/*  - smash : en angle court, vers la ligne de fond, ou (Normal, Expert)   */
/*    coupé, voire amorti contre un adversaire resté au fond ;             */
/*  - volée : amortie de volée contre un adversaire au fond, volée croisée */
/*    courte, ou volée à plat ;                                            */
/*  - fond de court : lob sur le volleyeur, amortie de l'intérieur du      */
/*    court, croisé court, à plat sur une balle courte (de l'intérieur),   */
/*    coupé de variation (armé : approche), à plat des styles puissants.   */
/* ---------------------------------------------------------------------- */
extern const u8 sty_lob[9], sty_drop[9], sty_slice[9];
extern const s8 sty_pow[9];
s16 iabs(s16 v);
u16 __fastcall__ ai_choose_f(u16 a)
{
    u8 p = (u8)a, lobp = (u8)(a >> 8), t = (u8)(p & 1), o = (u8)(t ^ 1);
    u8 lv = cfg.level[p], sty = char_style[cfg.chr[p]], pad, btn = PAD_A, r;
    s16 ox = PX(plx[o]), oy = PX(ply[o]), od, md;
    if (npl == 4) {
        /* viser l'adversaire du fond ou le trou entre les deux */
        ox = (PX(plx[o]) + PX(plx[o + 2])) >> 1;
        if (iabs(PX(ply[o + 2]) - NET_Y) > iabs(oy - NET_Y)) oy = PX(ply[o + 2]);
    }
    od = iabs(oy - NET_Y);                          /* adversaire : 0 au filet */
    md = iabs(PX(ply[p]) - NET_Y);                  /* ordinateur */
    if (ox > CENTER_X + 10) pad = PAD_LEFT;
    else if (ox < CENTER_X - 10) pad = PAD_RIGHT;
    else pad = (u8)(rand8() & 1 ? PAD_LEFT : PAD_RIGHT);
    if (lv && rand8() < 32) pad = 0;                /* au centre, dans le corps : une fois sur huit */
    r = rand8();
    if (!bounces && bz > 24 * FP) {                 /* smash */
        if (lv && r < 40) { btn = PAD_B; if (od > 90 && r < 16) pad |= PAD_DOWN; }   /* coupé, ou amorti */
        else if (r < 110) pad |= PAD_DOWN;          /* en angle court */
        else if (r < 170) pad |= PAD_UP;            /* vers la ligne de fond */
        return (u16)(pad | (btn << 8));
    }
    if (!bounces && md < 50) {                      /* volée */
        if (lv && od > 90 && r < (u8)((sty_drop[sty] >> 1) + 24)) { btn = PAD_B; pad |= PAD_DOWN; }   /* amortie de volée */
        else if (r < 100) pad |= PAD_DOWN;          /* volée croisée courte */
        else if (lv && r < 180) pad |= PAD_UP;      /* volée à plat */
        return (u16)(pad | (btn << 8));
    }
    if (lv && od < 50 && r < lobp) {
        btn = PAD_B; pad |= PAD_UP;                 /* lob sur le volleyeur */
    } else if (lv && od > 90 && md < 70 && rand8() < (u8)(sty_drop[sty] >> (2 - lv))) {
        btn = PAD_B; pad |= PAD_DOWN;               /* amortie (de l'intérieur du court) */
    } else if (lv && rand8() < (u8)(od < 50 ? 90 : 20)) {
        pad |= PAD_DOWN;                            /* croisé court : passing, ou sortir l'adversaire */
    } else if (lv && md < 70 && rand8() < 80) {
        pad |= PAD_UP;                              /* à plat sur une balle courte */
    } else if (rand8() < sty_slice[sty]) {
        btn = PAD_B;                                /* coupé de variation (armé : approche) */
    } else if (sty_pow[sty] > 4 && (rand8() & 3) == 0) {
        pad |= PAD_UP;                              /* frappe à plat */
    }
    return (u16)(pad | (btn << 8));
}

/* ---------------------------------------------------------------------- */
/* (3.03) Pas vers la balle : la frappe part jusqu'à 22 px du centre du    */
/* joueur, mais la raquette n'atteint qu'environ 14 px. Quand la balle     */
/* arrive vers lui (moins de 40 px en profondeur) et passera entre 16 et   */
/* 26 px sur le côté, le joueur glisse de 2 px par trame vers elle, en    */
/* s'arrêtant à 15 ou 16 px d'écart (la frappe en extension garde son imprécision) : */
/* la raquette touche vraiment la balle. Ni pendant un plongeon ni en      */
/* l'air.                                                                  */
/* ---------------------------------------------------------------------- */
extern u8 dive_t[4], pl_jump[4], pl_moving[4], sc_swap;
u16 __fastcall__ step_f(u16 a)
{
    u8 p = (u8)a, t = (u8)(p & 1);
    s16 dx, dy, s;
    if (!b_vis || bounces > 1 || dive_t[p] || pl_jump[p]) return 0;
    if ((u8)(PX(by) < NET_Y ? 1 : 0) != (u8)(t ^ sc_swap)) return 0;   /* balle dans son camp */
    dy = PX(by) - PX(ply[p]);
    if (dy < -40 || dy > 30) return 0;
    if ((dy < 0 && bvy < 0) || (dy > 0 && bvy > 0)) return 0;           /* elle s'éloigne */
    dx = PX(bx) - PX(plx[p]);
    s = dx < 0 ? -dx : dx;
    if (s <= 16 || s > 26) return 0;                                   /* (arrêt à 15 ou 16 px : la frappe reste en extension) */
    plx[p] += dx < 0 ? -128 : 128;
    pl_moving[p] = 1;
    return 0;
}
#pragma static-locals(pop)
