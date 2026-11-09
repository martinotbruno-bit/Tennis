/* Pixel Open — fonctions du moteur de match placées dans la banque fixe
 * (la banque du moteur est pleine) : utilitaires et calcul de trajectoire.
 * Elles ne lisent et n'écrivent que l'état partagé de game_int.h. */
#include "game_int.h"
#include "rules.h"
#include "court.h"

#pragma static-locals(on)

extern u8 b_netcord;

s16 mulfrac(s16 v, u8 k) { return (s16)(((v >> 3) * (s16)k) >> 5); }

s16 iabs(s16 v) { return v < 0 ? -v : v; }

s8 rnd_spread(u8 spread)
{
    if (!spread) return 0;
    return (s8)((s16)(rand8() % (u8)(spread * 2 + 1)) - spread);
}

void set_msg(const char *a, const char *b, u8 t)
{
    u8 i;
    for (i = 0; i < 14; ++i) { msg1[i] = *a ? *a++ : ' '; msg2[i] = *b ? *b++ : ' '; }
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

/* Nom du coup gagnant affiché au centre du bandeau (15 caractères au plus) */
const char *win_label(u8 shot, u8 pass, u8 jump)
{
    if (shot == SH_SMASH) return jump ? "SMASH SAUTE !" : "SMASH !";
    if (shot == SH_DROP) return "AMORTIE !";
    if (shot == SH_LOB) return "LOB GAGNANT !";
    if (shot == SH_VOLLEY) return "VOLEE GAGNANTE";
    if (pass) return "PASSING !";
    return "COUP GAGNANT !";
}

/* ---------------------------------------------------------------------- */
/* Petits effets du court (banque fixe, appelés à la fin de match_draw) :  */
/* ombres des joueurs, poussière ou éclaboussures au rebond, arbitre de    */
/* chaise animé au stade et repère d'atterrissage en vitesse « lent ».     */
/* Les sprites sont ajoutés après les joueurs : ils passent derrière eux.  */
/* ---------------------------------------------------------------------- */
#include "gfx.h"
#define PXU(v) ((u8)(((u16)(v) << 2) >> 8))      /* pixels, valeur positive */
extern u8 pl_jump[4];
static u8 fx_t, fx_x, fx_y, fx_kind, fx_bounces, fx_hits, mk_t;

void match_fx(void)
{
    u8 i, j;
    /* rebond : poussière sur terre et sable, éclaboussure sous la pluie */
    if (bounces != fx_bounces) {
        fx_bounces = bounces;
        if (bounces && bounces < 3 && b_vis) {
            fx_kind = 0;
            if (cfg.cond == COND_RAIN) fx_kind = 8;
            else if ((cfg.surface == 0 || cfg.surface == 4) && cfg.env != ENV_MARGAUX) fx_kind = 6;
            if (fx_kind) { fx_t = 12; fx_x = PXU(bx); fx_y = PXU(by); }
        }
    }
    if (fx_t) {
        --fx_t;
        spr1((u8)(fx_x - 4), (u8)(fx_y - 6), (u8)(fx_kind + (fx_t < 6)), (u8)(fx_kind == 8 ? 3 : 2));
    }
    /* ombres : toujours en simple ; en double, seulement pendant un saut
       (au plus 8 sprites par ligne sur NES) */
    for (i = 0; i < npl; ++i) {
        j = pl_jump[i];
        if (npl == 4 && !j) continue;
        if (cfg.mode == GM_TRAIN && i == 1 && cfg.ex != EX_STATIC) continue;   /* lanceur invisible */
        spr1((u8)(PXU(plx[i]) - 3), PXU(ply[i]), 5, 2);   /* sous les pieds, décalée vers la droite */
    }
    /* repère d'atterrissage (lent) : 30 trames après chaque frappe vers un
       joueur humain */
    if (rally_hits != fx_hits) {
        fx_hits = rally_hits;
        j = (u8)(hitter ^ 1);
        mk_t = 0;
        if (cfg.gspeed == 0 && rally_hits &&
            (cfg.ctrl[j] != CTRL_CPU || (npl == 4 && cfg.ctrl[j + 2] != CTRL_CPU))) mk_t = 30;
    }
    if (mk_t) {
        --mk_t;
        if (b_live && bounces == 0) spr1((u8)(land_x - 4), (u8)(land_y - 4), 10, 3);
    }
    /* arbitre de chaise (stade) : il regarde le côté où se trouve la balle */
    if (cfg.env == ENV_STADIUM) {
        spr1(20, NET_Y - 22, (u8)(by < NET_Y * FP ? 12 : 11), 2);
        spr1(20, NET_Y - 14, 13, 2);
    }
}
