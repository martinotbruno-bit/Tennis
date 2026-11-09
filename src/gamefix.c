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
