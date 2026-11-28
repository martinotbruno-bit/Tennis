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

/* ---------------------------------------------------------------------- */
/* (3.05) Marque au sol (option MARQUE) : à la fin d'un point dont le     */
/* rebond décisif (premier rebond de la dernière balle, dans le camp du   */
/* receveur) est près d'une ligne - bonne à 10 px ou moins de la ligne la  */
/* plus proche, dehors à 6 px ou moins -, sa marque reste au sol (dessinée */
/* par match_fx, extras.c) jusqu'à ce que le bandeau du score disparaisse */
/* (3.06 ; au plus tard au lancer du service suivant).                    */
/* ---------------------------------------------------------------------- */
extern u8 opt_val[];
extern u8 hl_bx, hl_by, hl_bin, hl_bok;
u8 mk_on, mk_x, mk_y;                     /* (mk_on : 1 marque montrée, 2 et le bandeau du score déjà vu) */
u16 __fastcall__ mk_set_f(u16 a)
{
    s16 x = hl_bx, y = hl_by, d, l = cfg.doubles ? IN_DL : IN_SL, r = cfg.doubles ? IN_DR : IN_SR;
    u8 recv = (u8)(hitter ^ 1);
    (void)a;
    mk_on = 0;
    if (!opt_val[26] || !hl_bok || cfg.demo) return 0;
    if ((u8)(y < NET_Y ? 1 : 0) != (u8)((recv ^ sc_swap) & 1)) return 0;   /* (balle restée dans le camp du frappeur : filet) */
    if (hl_bin) {
        d = x - l; if (r - x < d) d = r - x;
        if (y - IN_TOP < d) d = y - IN_TOP;
        if (IN_BOT - y < d) d = IN_BOT - y;
        if (b_serve) return 0;                      /* (service bon : les lignes du carré ne sont pas vérifiées ici) */
        if (d > 10) return 0;                       /* (3.07 : 10 px au lieu de 4 pour une balle bonne) */
    } else {
        d = 0;
        if (x < l) d = l - x; else if (x > r) d = x - r;
        if (y < IN_TOP) d += IN_TOP - y; else if (y > IN_BOT) d += y - IN_BOT;
        if (b_serve || d == 0 || d > 6) return 0;
    }
    /* (3.08) marque recalée sur le jugement : elle couvre x - 2 à x + 1 et
       y - 1 à y (extras.c) ; l'empreinte de la balle (2 px de rayon) n'en
       diffère que d'un demi-pixel, de quoi qu'une balle dehors semble
       toucher la ligne de droite, ou qu'une balle bonne du haut semble
       ne pas toucher sa ligne de fond. Balle dehors : au moins un pixel
       d'écart avec la ligne ; balle bonne qui touche la ligne : la marque
       mord sur la ligne */
    l += BALL_R; r -= BALL_R;                       /* (pixels des lignes de côté) */
    if (hl_bin) {
        /* balle qui touche une ligne (son disque la recouvre, d'un côté ou
           de l'autre) : marque ramenée sur la ligne */
        a = (u8)(persp ? 1 : 0);                    /* (perspective : arrondis de la projection, un pixel de plus) */
        if (x <= l + 2) { if (x < l - 1 + (s16)a) x = l - 1 + a; else if (x > l + 2 - (s16)a) x = l + 2 - a; }
        if (x >= r - 2) { if (x < r - 1 + (s16)a) x = r - 1 + a; else if (x > r + 2 - (s16)a) x = r + 2 - a; }
        if (y <= TOP_BASE + 2) { if (y < TOP_BASE - 1) y = TOP_BASE - 1; else if (y > TOP_BASE + 1) y = TOP_BASE + 1; }
        if (y >= BOT_BASE - 2) { if (y < BOT_BASE + 1) y = BOT_BASE + 1; else if (y > BOT_BASE + 2) y = BOT_BASE + 2; }
    } else {
        a = (u8)(persp ? 2 : 0);                    /* (perspective : arrondis de la projection, un écart de plus) */
        if (x > r + 2 && x < r + 4 + (s16)a) x = r + 4 + a;
        if (x < l - 2 && x > l - 3 - (s16)a) x = l - 3 - a;
        if (y > BOT_BASE + 3 && y < BOT_BASE + 4 + (s16)a) y = BOT_BASE + 4 + a;
        if (y < TOP_BASE - 3 && y > TOP_BASE - 4 - (s16)a) y = TOP_BASE - 4 - a;
    }
    mk_x = (u8)x; mk_y = (u8)y; mk_on = 1;
    return 0;
}
#pragma static-locals(pop)
