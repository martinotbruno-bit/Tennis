/* Royal Martinot : dessin du match (sprites des joueurs, de la balle, des
   effets), appelé à chaque trame par la boucle principale. En banque 22
   (la banque 0, celle du moteur, était pleine). */
#include "game_int.h"
void decor_draw(void);   /* extra2.c */
#include "app.h"
#include "rules.h"
#include "court.h"
#include "gfx.h"
#include "game.h"

#define side_of(t) ((u8)(((t) & 1) ^ sc_swap))   /* 0 = bas, 1 = haut */
extern u8 hit_now, hl_mode, cer, pose_force[4], pose_now[4], pl_jump[4];
extern u8 band_hold;
#pragma zpsym("band_hold")
extern u8 pose_cache[4], flip_cache[4];
void poses_all(void); void slot_tick(void); void cer_front(void); void cer_draw(void);
void match_fx(void); void band_draw(void);

/* fête de victoire : quand un humain gagne (ou en démonstration), comme
   pour la musique de victoire (copie de celle de game.c) */
static const u8 jump_h[16] = { 0, 0, 0, 0, 1, 1, 2, 3, 4, 5, 6, 7, 9, 10, 12, 14 };   /* j² / 16 */
static const u8 hop_h[16] = { 0, 1, 3, 5, 6, 7, 7, 7, 8, 7, 7, 7, 6, 5, 3, 1 };
static u8 win_party(void)
{
    return (u8)(ms == MS_OVER && !(cfg.ctrl[sc_winner] == CTRL_CPU && cfg.ctrl[sc_winner ^ 1] != CTRL_CPU));
}

extern u8 dr_ord[4], dr_xs[4], dr_ys[4];
void draw_order(void);   /* fastmove.s */
void match_draw(void)
{
    u8 i, j, k, p, party;   /* j sert aussi au saut */
#define ord dr_ord
#define ys dr_ys
#define xs dr_xs
    s16 sy;
    /* trame de frappe (calcul de trajectoire coûteux) : on garde les sprites
       de la trame précédente, ce qui évite tout ralentissement ; pendant un
       ralenti, c'est rp_play qui dessine */
    if (hit_now || ms == MS_REPLAY || hl_mode) return;   /* (portrait du vainqueur : la banque 15 dessine) */
    party = win_party();
    /* tri par profondeur : le joueur le plus bas à l'écran passe devant ;
       positions à l'écran en octets (draw_order, assembleur) */
    draw_order();
    /* au moins 3 joueurs sur une même bande de 24 px (double) : plus de 8
       sprites par ligne possibles ; on fait tourner l'ordre des sprites pour
       qu'aucun ne disparaisse (rotation seulement quand c'est nécessaire) */
    spr_rot = 0;
    if (npl == 4 && ((u8)(ys[ord[0]] - ys[ord[2]]) < 24 || (u8)(ys[ord[1]] - ys[ord[3]]) < 24))
        spr_rot = (u8)(frame_cnt * 36);
    spr_begin();
    /* balle et ombre en tête (priorité d'affichage maximale) */
    if (b_vis) {
        /* lancer de service : hauteur réelle pour le serveur du bas ; réduite
           (x 9/16) pour celui du haut, afin que la balle reste à l'écran.
           La balle grossit avec sa hauteur (spr_ball, banque fixe). */
        sy = bz < 0 ? 0 : PX(bz);
        k = (u8)sy;
        if (ms == MS_TOSS && by < NET_Y * FP) sy = (sy + (sy << 3)) >> 4;
        spr_ball((u8)PX(bx), PX(by) - sy - 3, (u8)PX(by), k);
    }
    bank_call(BANK_EXTRAS, poses_all);     /* poses et miroirs, balle du dribble, glissades */
    bank_call(BANK_SWAP, slot_tick);            /* bustes « bras caché » (joueur vu de dos) */
    if (cer) bank_call(BANK_SWAP, cer_front);   /* serviette, gourde (devant les joueurs) */
    for (i = 0; i < npl; ++i) {
        p = ord[i];
        /* saut : hauteur (j²/16) au plus haut à l'impact, puis chute ;
           j/4 est gardé dans les bits 6-7 de pose_cache pour les ralentis
           (pose dans les bits 0-4) */
        j = pl_jump[p];
        if (j) --pl_jump[p];
        k = pose_now[p];
        pose_cache[p] = (u8)((k & 31) | ((j >> 2) << 6));
        if (cfg.mode == GM_TRAIN && p == 1 && cfg.ex != EX_STATIC) continue;   /* lanceur invisible */
        j = jump_h[j < 16 ? j : 15];              /* j² / 16 (table : pas de multiplication) */
        /* victoire : le vainqueur sautille de joie (un bond toutes les 16 trames) */
        if (party && (u8)(p & 1) == sc_winner && pose_force[p] == 0xFF) j = hop_h[ms_timer & 15];
        if (POSE_LOW(k & 31)) j -= 4;          /* à genoux, assis : 4 px plus bas */
        pv_x = xs[p];
        pv_y = (s16)ys[p] - (s8)j;
        pv_c = cfg.chr[p]; pv_front = side_of(p); pv_pose = k; pv_flip = flip_cache[p]; pj_sg = p;
        spr_person_v();                           /* palette p & 1 */
    }
    if (cer) bank_call(BANK_SWAP, cer_draw);    /* pièce, chaises, raquette lâchée */
    bank_call(BANK_EXTRAS, match_fx);   /* ombres, poussière, arbitre, repère */
    if (party) { win_fx(); far_call(BANK_TRAIN, (void *)fw_draw, ms_timer); }
    if (cfg.cond == COND_RAIN) bank_call(BANK_EXTRAS, rain_draw);
    else if (cfg.env == ENV_PARK && (ms == MS_DEAD || ms == MS_OVER || cer || band_hold)) bank_call(BANK_X2, decor_draw);   /* feuilles d'automne : fin du point, bandeau du score, pauses (extra2.c) */
    spr_end();
    bank_call(BANK_HL, band_draw);      /* bandeau du score entre les points (highlight.c) */
}
#undef ord
#undef ys
#undef xs
