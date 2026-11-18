/* Royal Martinot 2.76 — fonctions de match ajoutées en 2.76, sorties des
   banques 15 et 20 pleines (banque 30) : balle faute renvoyée, annonce de
   l'ace, balle limite (zoom rapide), récapitulatif du score. */
#include <string.h>
#include "game_int.h"
#include "app.h"
#include "rules.h"
#include "court.h"
#include "gfx.h"
#include "audio.h"
#include "assets.h"

#define side_of(t) ((u8)(((t) & 1) ^ sc_swap))
extern u8 sc_swap;
void set_msg(const char *a, const char *b, u8 t);   /* banque fixe */

/* ---------------------------------------------------------------------- */
/* Balle faute renvoyée (2.76) : après un service faux ou une balle dehors */
/* (le point est déjà joué, MS_DEAD), le joueur humain qui a armé son coup */
/* et a la balle à portée la renvoie quand même, mollement, vers l'autre   */
/* camp, comme on le fait sur un vrai court. L'échange reste terminé : la  */
/* balle n'est plus en jeu (b_live = 0), ses rebonds ne comptent pas.      */
/* Appelée par human_update (game.c) ; 1 si la balle part.                 */
/* ---------------------------------------------------------------------- */
void launch(s16 tx, s16 ty, u8 T, u8 grav);   /* gamefix.c */
extern u8 end_reason;
u16 __fastcall__ dead_hit(u16 pp)
{
    u8 p = (u8)pp, top;
    s16 d;
    if (!b_vis || end_reason == END_LET || bz > 40 * FP) return 0;
    top = (u8)(PX(ply[p]) < NET_Y);
    if ((u8)(PX(by) < NET_Y) != top) return 0;            /* balle dans son camp */
    if (top ? bvy >= 0 : bvy <= 0) return 0;              /* et qui vient vers lui (pas celle qu'il vient de renvoyer) */
    d = PX(by) - PX(ply[p]); if (d < -15 || d > 10) return 0;
    d = PX(bx) - PX(plx[p]); if (d < -24 || d > 24) return 0;
    bspin = 0;
    launch(CENTER_X + (s16)(rand8() & 31) - 16, top ? NET_Y + 70 : NET_Y - 70, 44, 7);
    b_live = 0;                                           /* (hors jeu) */
    sfx_hit(0);
    if (ms_timer < 45) ms_timer = 45;                     /* (le temps de la voir partir) */
    return 1;
}

/* annonce « ACE ! » et vitesse du service (2.76 : déplacée de highlight.c,
   banque 15 pleine) */
extern u8 hl_spd;
static char ace_b[10];
void hl_ace_msg(void)
{
    u16 k = (u16)hl_spd + 80;
    ace_b[0] = (char)('0' + k / 100); ace_b[1] = (char)('0' + (k / 10) % 10); ace_b[2] = (char)('0' + k % 10);
    ace_b[3] = ' '; ace_b[4] = 'K'; ace_b[5] = 'M'; ace_b[6] = '/'; ace_b[7] = 'H'; ace_b[8] = 0;
    set_msg("ACE !", ace_b, 90);
}

/* (2.76) déplacés de highlight.c (banque 15 pleine) : balle limite, puis
   récapitulatif du score */
extern u8 hl_bx, hl_by, hl_bin, hl_bok, hl_sok, hl_sbok, hl_sbi, hl_sbx, hl_sby, hl_sbin, hl_bi, hl_quick, rp_pending;
extern u16 hl_bf, hl_sbf;
/* balle dehors de peu (2.73) : le point s'arrête sur une balle d'échange
   tombée à 3 px de la ligne ou moins ; ralenti court (12 trames avant le
   rebond, rp_start), sans gros plan, droit au zoom x 2 sur l'impact
   (« DEHORS ! », 50 trames au lieu de 80), puis fin du ralenti. Appelé par
   point_over (faute adverse). */
extern u8 hit_jump;
extern u8 b_serve;
#pragma static-locals(push, off)        /* (pile : la RAM de travail est pleine) */
/* balle bonne sur la ligne ou à 2 px à l'intérieur du point où elle la
   touche encore (2.74) : distance à la ligne la plus proche, 4 px au plus */
static u8 near_in(s16 x, s16 y)
{
    s16 d, l = cfg.doubles ? IN_DL : IN_SL, r = cfg.doubles ? IN_DR : IN_SR;
    d = x - l; if (r - x < d) d = r - x;
    if (y - IN_TOP < d) d = y - IN_TOP;
    if (IN_BOT - y < d) d = IN_BOT - y;
    return (u8)(d <= 4);
}
void hl_out(void)
{
    s16 x = hl_bx, y = hl_by, d = 0, l, r;
    if (!hl_bok || b_serve || !cfg.replays || cfg.env == ENV_NUDE || cfg.demo) return;   /* (2.74 : prioritaire sur le ralenti complet) */
    l = cfg.doubles ? IN_DL : IN_SL; r = cfg.doubles ? IN_DR : IN_SR;   /* (ligne comprise) */
    if (hl_bin) {
        if (!near_in(x, y)) return;
    } else {
        if (x < l) d = l - x; else if (x > r) d = x - r;
        if (y < IN_TOP) d += IN_TOP - y; else if (y > IN_BOT) d += y - IN_BOT;
        if (d == 0 || d > 3) return;
    }
    hl_sok = 0;                         /* pas de gros plan */
    hl_sbok = 1; hl_sbi = hl_bi; hl_sbx = hl_bx; hl_sby = hl_by; hl_sbin = hl_bin; hl_sbf = hl_bf;
    hl_quick = 1; rp_pending = 1;
}
#pragma static-locals(pop)

extern u8 recap_pend;
void team_name(u8 t, char *out);   /* gamefix.c */
void score_line(u8 t, char *out)
{
    u8 i, k = 4;
    team_name(t, out);
    for (i = 0; i < sc_set_no && i < MAX_SETS; ++i) { out[k++] = ' '; out[k++] = (char)('0' + sc_hist[i][t]); }
    if (!sc_over && k < 14) { out[k++] = ' '; out[k++] = (char)('0' + sc_games[t]); }
    out[k] = 0;                     /* les deux lignes ont la même longueur : centrées ensemble */
}

/* bandeau : le score set par set pendant 2 secondes */
static char recap_a[16], recap_b[16];
void score_recap(void)
{
    score_line(0, recap_a);
    score_line(1, recap_b);
    set_msg(recap_a, recap_b, 120);
    recap_pend = 0;
}


u16 __fastcall__ near_in_f(u16 xy) { return near_in((u8)xy, (u8)(xy >> 8)); }

/* (2.77) trace de la balle des zooms (highlight.c) : l'empreinte exacte de
   la balle jugée, disque de 2 px du court ; voir le commentaire de
   highlight.c */
#pragma static-locals(push, off)
/* tuiles précalculées (contour sombre de 1 px, intérieur plein de la
   couleur de la balle, 2.77 : la trace évidée semblait une balle vide ; disque de rayon 4 z px, 16 x 16 px, haut gauche,
   haut droit, bas gauche, bas droit) : x 2 puis x 4 */
static const u8 mk_t2[64] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x03, 0x04, 0x08, 0x08,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xE0, 0xE0, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x20, 0x10, 0x10,
    0x07, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x04, 0x03, 0x00, 0x00, 0x00, 0x00,
    0xE0, 0xE0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x20, 0xC0, 0x00, 0x00, 0x00, 0x00 };
static const u8 mk_t4[64] = {
    0x00, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0x7F, 0x7F, 0x07, 0x18, 0x30, 0x60, 0x40, 0x80, 0x80, 0x80,
    0x00, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFE, 0xFE, 0xE0, 0x18, 0x0C, 0x06, 0x02, 0x01, 0x01, 0x01,
    0x7F, 0x7F, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x00, 0x80, 0x80, 0x80, 0x40, 0x60, 0x30, 0x18, 0x07,
    0xFE, 0xFE, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0x00, 0x01, 0x01, 0x01, 0x02, 0x06, 0x0C, 0x18, 0xE0 };
u16 __fastcall__ mark_build_f(u16 adz)
{
    const u8 *p = (adz & 15) == 4 ? mk_t4 : mk_t2;
    u8 i;
    vram_adr((u16)(adz & 0xFFF0));
    for (i = 0; i < 64; ++i) vram_put(p[i]);
    return 0;
}
#pragma static-locals(pop)
