/* Royal Martinot — fonctions du moteur de match placées dans la banque fixe
 * (la banque du moteur est pleine) : utilitaires et calcul de trajectoire.
 * Elles ne lisent et n'écrivent que l'état partagé de game_int.h. */
#include "game_int.h"
#include "rules.h"
#include "court.h"
#include "gfx.h"
#include "audio.h"
#include "assets.h"

#pragma static-locals(on)

u8 ump_arm;              /* 1 : faute ou balle dehors, l'arbitre tend le bras */

extern u8 b_netcord;

s16 mulfrac(s16 v, u8 k) { return (s16)(((v >> 3) * (s16)k) >> 5); }

s16 iabs(s16 v) { return v < 0 ? -v : v; }

s8 rnd_spread(u8 spread)
{
    if (!spread) return 0;
    return (s8)((s16)(rand8() % (u8)(spread * 2 + 1)) - spread);
}

/* Ligne d'annonce de 14 caractères, texte centré */
void center14(char *dst, const char *s)
{
    u8 i, n = 0, o;
    while (s[n] && n < 14) ++n;
    o = (u8)((14 - n) >> 1);
    for (i = 0; i < 14; ++i) dst[i] = (char)((i >= o && i < o + n) ? s[i - o] : ' ');
}

void set_msg(const char *a, const char *b, u8 t)
{
    center14(msg1, a);
    center14(msg2, b);
    msg_timer = t;
    hud_dirty = 1;
}

void launch(s16 tx, s16 ty, u8 T, u8 grav)
{
    s16 tt, dy, zn, n, d;
    u16 frac = 0, tn;
    u8 tries;
    /* fraction du trajet (sur 256) à laquelle la balle passe le filet :
       une seule division 16 bits, hors de la boucle (optimisation CPU) */
    dy = ty * FP - by;
    n = NET_Y - PX(by);
    d = ty - PX(by);
    if ((n < 0) == (d < 0) && d != 0) {
        if (n < 0) { n = -n; d = -d; }
        frac = ((u16)n << 8) / (u16)d;
    }
    /* hauteur au filet sans division : la parabole passant par z(0) = bz et
       z(T) = 0 vaut z(t) = bz.(1 - t/T) + g.t.(T - t)/2 (calcul en FP/4) */
    for (tries = 0; tries < 5 && frac && frac < 256; ++tries) {
        tn = (frac * T) >> 8;
        zn = (s16)((((u16)bz >> 4) * (256 - frac)) >> 6)
           + (s16)((((u16)grav * tn) >> 3) * (u16)(T - tn));
        if (zn >= ((NET_H + 3) * FP) >> 2) break;
        T = (u8)(T + (T >> 3) + 3);
    }
    tt = (s16)T;      /* division signée explicite (cc65 : s16 / u8 = non signé) */
    bvz = (s16)(((u16)grav * T) >> 1) - bz / tt;
    bvx = (tx * FP - bx) / tt;
    bvy = dy / tt;
    bgrav = grav;
    land_x = tx;
    land_y = ty;
    bounces = 0;
    b_live = 1;
    b_netcord = 0;
}

/* (win_label : highlight.c, banque 15) */

/* ---------------------------------------------------------------------- */
/* Petits effets du court (banque fixe, appelés à la fin de match_draw) :  */
/* ombres des joueurs, poussière ou éclaboussures au rebond, arbitre de    */
/* chaise animé au stade et repère d'atterrissage en vitesse « lent ».     */
/* Les sprites sont ajoutés après les joueurs : ils passent derrière eux.  */
/* ---------------------------------------------------------------------- */
#include "gfx.h"
#define PXU(v) ((u8)(((u16)(v) << 2) >> 8))      /* pixels, valeur positive */
extern u8 pl_jump[4], tired;
u8 st_perf[2], st_ptry[2], hit_perf, swing_early;
u8 perf_t, perf_x, perf_y;       /* éclat de la frappe parfaite (dessiné par match_fx, banque 3) */


/* Nom d'équipe sur 4 lettres (panneaux de score, annonces) : « ALEX » en
   simple, « MXAL » en double. Abréviations de team_label : MARGAUX donne
   « MX », pour ne pas se confondre avec MAYA (« MA »). */
void team_name(u8 t, char *out)
{
    team_label(cfg.chr[t], cfg.doubles ? cfg.chr[t + 2] : 255, out);   /* (2.98 : 5 lettres, « AL/MA » en double) */
}

/* lob : au rebond, il garde son élan lifté vers le fond, mais sa
   remontée est plafonnée (environ 56 px au sommet au lieu de 90 à 110) : il
   retombe dans le court, frappable jusqu'à son second rebond (can_hit :
   62 px après le rebond), au lieu de partir hors de l'écran */
#define LOB_VZ 210
void lob_cap(void)
{
    if (last_shot == SH_LOB && bvz > LOB_VZ) bvz = LOB_VZ;
}

/* le joueur p peut-il frapper la balle ? (déplacé du moteur, banque 0) */
u8 can_hit(u8 p)
{
    s16 dx, dy;
    u8 t = (u8)(p & 1);
    if (ms != MS_RALLY || !b_live || hitter == t) return 0;
    if (bounces > 1) return 0;
    if ((PX(by) < NET_Y ? 1 : 0) != (u8)((t & 1) ^ sc_swap)) return 0;
    if (b_serve && bounces == 0) return 0;      /* le retour se joue après rebond */
    /* zone de frappe généreuse : les balles un peu loin partent quand même
       (la profondeur d'abord : le plus souvent, elle suffit à conclure) ;
       (3.03) largeur ±22 px (±24 depuis la 2.4), le joueur faisant un pas
       vers la balle au-delà de 15 px (step_f, play2.c) */
    dy = PX(by) - PX(ply[p]);
    if (dy < -15 || dy > 10) return 0;
    dx = PX(bx) - PX(plx[p]);
    if (dx < -22 || dx > 22) return 0;
    if (bz > 40 * FP) {
        /* balle très haute : avant le rebond, smash sauté (joueurs humains et
           CPU expert, hors du fond du court) ; après le rebond, un lob
           haut se reprend jusqu'à 62 px, sans sauter */
        if (bz > 62 * FP) return 0;
        if (!bounces) {
            if (iabs(PX(ply[p]) - NET_Y) > 70) return 0;
            if (cfg.ctrl[p] == CTRL_CPU && cfg.level[p] != 2) return 0;
        }
    }
    return 1;
}

/* ---------------------------------------------------------------------- */
/* Frappe parfaite : coup lifté, à plat ou coupé après le rebond, balle à  */
/* côté du corps (ni collée, ni en extension) et, pour un humain, bouton   */
/* relâché juste avant l'arrivée de la balle (3 premières trames du geste) */
/* ---------------------------------------------------------------------- */

/* perfect_hit : banque 13 (shots.c) ; timing_msg, serve_fb : banque 29 (play2.c, 3.02) */

/* Récapitulatif du score (score_line, score_recap) : banque 0 (game.c) (place en banque fixe) */

/* Feux d'artifice (fête de victoire sur le court, levée de coupe). t est un
   compte à rebours (ms_timer sur le court) : chaque fusée siffle en montant
   à t = éclatement + 15, puis éclate (« pop ») à t = 115, 75 ou 35 en 4
   étoiles qui s'écartent pendant 14 trames ; dy décale les gerbes vers le
   bas. Sons déclenchés par la logique (fw_sound), gerbes dessinées avec le
   reste (fw_draw). */
static const u8 fw_pop_t[3] = { 115, 75, 35 };
void fw_sound(u8 t)
{
    u8 k;
    for (k = 0; k < 3; ++k) {
        if (t == (u8)(fw_pop_t[k] + 15)) sfx_fw_launch();
        else if (t == fw_pop_t[k]) sfx_fw_pop();
    }
}

/* Fête de victoire : quand un humain gagne (ou en démonstration), comme pour
   la musique de victoire. Hauteur du bond de joie : t·(16−t)/8, en table.
   (Déplacées depuis la banque du moteur, qui était pleine.) */

/* poses des joueurs : poses_all (banque 3, extras.c) */

