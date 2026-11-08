/* Pixel Open — moteur de match : déplacements, frappes, service,
 * physique de la balle, arbitrage et intelligence artificielle.
 *
 * Unités : positions et vitesses en virgule fixe 1/64 de pixel (FP).
 * La hauteur de balle (z) utilise la même unité ; elle est dessinée en
 * décalant la balle vers le haut par rapport à son ombre. */
#include "game.h"
#include "rules.h"
#include "court.h"
#include "gfx.h"
#include "audio.h"

#pragma static-locals(on)

#define FP 64
#define PX(v) ((s16)(v) >> 6)

MatchCfg cfg;
u8 match_quit;
u8 st_points[2], st_aces[2], st_dfaults[2], st_winners[2], st_errors[2];
u8 st_first_in[2], st_first_tot[2], st_longest;
u8 tr_score, tr_total, tr_target;

/* ---------------- état des joueurs (une entrée par équipe en simple) ---- */
/* non statiques : lisibles par les tests automatiques (fichier .lbl) */
s16 plx[2], ply[2];
static u8 pl_state[2], pl_timer[2], pl_charge[2], pl_btn[2], pl_hit[2];
static u8 pl_walk[2], pl_moving[2], pl_flip[2];
static u8 ai_delay[2], ai_net[2];
static u8 hit_margin;
#define ST_IDLE   0
#define ST_CHARGE 1
#define ST_SWING  2
#define ST_TOSS   3
#define ST_SERVED 4

/* ---------------- balle ----------------------------------------------- */
s16 bx, by, bz, bvx, bvy, bvz;
u8 bgrav, bspin, bounces, hitter, b_serve, b_vis, b_live;
s16 land_x, land_y;
u8 rally_hits;
#define SPIN_FLAT 0
#define SPIN_TOP  1
#define SPIN_SLICE 2

/* ---------------- état du match ----------------------------------------- */
u8 ms, ms_timer, faults, paused;
static u8 end_reason, pending_winner;
static u8 hud_dirty, msg_timer;
static char msg1[12], msg2[12];
#define MS_SERVE  0     /* serveur en place, attend le lancer */
#define MS_TOSS   1     /* balle lancée, attend la frappe */
#define MS_RALLY  2
#define MS_DEAD   3     /* point joué, la balle finit sa course */
#define MS_OVER   4

#define END_FAULT 1
#define END_POINT 2

/* surfaces : restitution et adhérence au rebond (sur 256) */
static const u8 surf_rest[4] = { 196, 150, 176, 160 };
static const u8 surf_fric[4] = { 200, 244, 226, 236 };
/* IA : vitesse (FP/trame), délai de réaction, dispersion, marge visée */
static const u8 ai_speed[3]  = { 78, 98, 116 };
static const u8 ai_react[3]  = { 16, 9, 3 };
static const u8 ai_spread[3] = { 30, 18, 11 };
static const u8 ai_margin[3] = { 26, 15, 9 };
static const u8 ai_power[3]  = { 4, 12, 20 };
#define HUMAN_SPEED 120

static u8 side_of(u8 t) { return (u8)(t ^ sc_swap); }   /* 0 = bas, 1 = haut */
static u8 serving_team(void) { return sc_server; }

static s16 mulfrac(s16 v, u8 k)
{
    return (s16)(((long)v * k) >> 8);
}

static s16 iabs(s16 v) { return v < 0 ? -v : v; }

static s8 rnd_spread(u8 spread)
{
    if (!spread) return 0;
    return (s8)((s16)(rand8() % (u8)(spread * 2 + 1)) - spread);
}

static void set_msg(const char *a, const char *b, u8 t)
{
    u8 i;
    for (i = 0; i < 11; ++i) { msg1[i] = *a ? *a++ : ' '; msg2[i] = *b ? *b++ : ' '; }
    msg_timer = t;
    hud_dirty = 1;
}

/* ====================================================================== */
/* Lancement de la balle : atteindre (tx, ty) en T trames                  */
/* ====================================================================== */
static void launch(s16 tx, s16 ty, u8 T, u8 grav)
{
    long zn;
    s16 tn, tt;
    u8 tries;
    for (tries = 0; tries < 12; ++tries) {
        /* division signée explicite : cc65 divise en non signé un
           entier par un unsigned char */
        tt = (s16)T;
        bvx = (tx * FP - bx) / tt;
        bvy = (ty * FP - by) / tt;
        bvz = (s16)(((u16)grav * T) / 2) - bz / tt;
        /* contrôle de passage au-dessus du filet */
        if (bvy == 0) break;
        tn = (s16)((NET_Y * FP - by) / bvy);
        if (tn <= 0 || tn >= T) break;
        zn = (long)bz + (long)bvz * tn - ((long)grav * tn * tn) / 2;
        if (zn >= (NET_H + 3) * FP) break;
        T += 3;
    }
    bgrav = grav;
    land_x = tx;
    land_y = ty;
    bounces = 0;
    b_live = 1;
}

/* ====================================================================== */
/* Fin d'échange                                                           */
/* ====================================================================== */
static void point_over(u8 winner, u8 reason)
{
    /* reason : 0 normal, 1 faute adverse (filet/dehors), 2 coup gagnant */
    if (ms != MS_RALLY && ms != MS_TOSS) return;
    ms = MS_DEAD;
    ms_timer = 80;
    b_live = 0;
    end_reason = END_POINT;
    pending_winner = winner;
    ++st_points[winner];
    if (rally_hits > st_longest) st_longest = rally_hits;
    if (cfg.mode != GM_MATCH) return;
    if (reason == 2) {
        ++st_winners[winner];
        if (rally_hits == 1) { ++st_aces[winner]; set_msg("ACE !", "", 70); }
        else set_msg("COUP", "GAGNANT !", 70);
        sfx_crowd(7);
    } else if (reason == 1) {
        ++st_errors[winner ^ 1];
        sfx_crowd(2);
    }
    if (rally_hits >= 12) { set_msg("QUEL", "ECHANGE !", 70); sfx_crowd(7); }
}

static void serve_fault(const char *why)
{
    ms = MS_DEAD;
    ms_timer = 60;
    b_live = 0;
    sfx_fault();
    if (cfg.mode == GM_TRAIN_SERVE) {
        ++tr_total; end_reason = END_FAULT; set_msg(why, "", 50); return;
    }
    if (faults == 0) {
        end_reason = END_FAULT;
        set_msg("FAUTE", why, 55);
    } else {
        ++st_dfaults[serving_team()];
        set_msg("DOUBLE", "FAUTE", 70);
        end_reason = END_POINT;
        pending_winner = (u8)(serving_team() ^ 1);
        ++st_points[pending_winner];
    }
}

/* ====================================================================== */
/* Rebond et arbitrage                                                     */
/* ====================================================================== */
static u8 in_service_box(void)
{
    u8 srv_side = side_of(hitter);
    u8 ad = rules_ad_side();
    /* bas+égalité -> carré gauche ; bas+avantage -> droit ; inversé en haut */
    u8 left = (u8)((srv_side == 0) == (ad == 0));
    s16 x = PX(bx), y = PX(by);
    if (left) { if (x < SGL_L || x > CENTER_X) return 0; }
    else      { if (x < CENTER_X || x > SGL_R) return 0; }
    if (srv_side == 0) return (u8)(y >= TOP_SERV && y <= NET_Y);
    return (u8)(y >= NET_Y && y <= BOT_SERV);
}

static void ball_bounce(void)
{
    u8 half = (u8)(PX(by) < NET_Y ? 1 : 0);
    u8 recv = (u8)(hitter ^ 1);
    u8 rest = surf_rest[cfg.surface], fric = surf_fric[cfg.surface];
    s16 x = PX(bx), y = PX(by);

    if (bspin == SPIN_TOP)   { rest = (u8)(rest + 24); if (fric < 240) fric += 12; }
    if (bspin == SPIN_SLICE) { rest = (u8)(rest - 46); }
    bvz = mulfrac(-bvz, rest);
    bvx = mulfrac(bvx, fric);
    bvy = mulfrac(bvy, fric);
    bz = 0;
    if (b_vis) sfx_bounce();

    if (!b_live) { ++bounces; return; }

    if (bounces == 0) {
        if (half != side_of(recv)) {
            /* la balle n'a pas passé le filet */
            if (b_serve) { serve_fault("FILET"); return; }
            point_over(recv, 1);
            ++bounces;
            return;
        }
        if (b_serve) {
            if (!in_service_box()) { serve_fault(""); ++bounces; return; }
            if (faults == 0) ++st_first_in[hitter];
            if (cfg.mode == GM_TRAIN_SERVE) {
                ++tr_score; ++tr_total;
                ms = MS_DEAD; ms_timer = 50; b_live = 0; end_reason = END_FAULT;
                set_msg("BON", "SERVICE", 40);
                ++bounces;
                return;
            }
        } else if (x < SGL_L || x > SGL_R || y < TOP_BASE || y > BOT_BASE) {
            set_msg("DEHORS", "", 50);
            point_over(recv, 1);
            ++bounces;
            return;
        } else if (cfg.mode == GM_TRAIN_RALLY && hitter == 0) {
            /* entraînement : balle bonne ; bonus si dans la zone cible */
            ++tr_score;
            if (y < TOP_SERV && ((tr_target == 0 && x < CENTER_X) || (tr_target == 1 && x >= CENTER_X))) {
                ++tr_score;
                set_msg("CIBLE !", "+2", 40);
            } else set_msg("BONNE", "+1", 40);
            hud_dirty = 1;
            ms = MS_DEAD; ms_timer = 40; b_live = 0; end_reason = END_FAULT;
            ++bounces;
            return;
        }
        ++bounces;
        return;
    }
    /* deuxième rebond : le frappeur gagne le point */
    point_over(hitter, (u8)(rally_hits >= 1 ? 2 : 0));
    ++bounces;
}

static void ball_update(void)
{
    s16 oldy = by;
    if (!b_vis) return;
    bvz -= bgrav;
    bx += bvx;
    by += bvy;
    bz += bvz;
    /* filet */
    if (((oldy < NET_Y * FP) != (by < NET_Y * FP)) && bz < NET_H * FP
        && bx > (DBL_L - 6) * FP && bx < (DBL_R + 6) * FP) {
        by = oldy;
        bvy = -(bvy >> 2);
        bvx >>= 1;
        if (bvz > 0) bvz = 0;
        sfx_net();
        if (b_live && !b_serve) set_msg("FILET", "", 40);
    }
    if (bz <= 0 && bvz < 0) {
        ball_bounce();
        if (bounces > 6) { bvz = 0; bvx = mulfrac(bvx, 200); bvy = mulfrac(bvy, 200); }
    }
    /* sortie de l'écran */
    if (bx < 8 * FP || bx > 248 * FP || by < 20 * FP || by > 238 * FP) {
        if (b_live) {
            if (bounces == 0) {
                set_msg("DEHORS", "", 50);
                if (b_serve) { serve_fault(""); }
                else point_over((u8)(hitter ^ 1), 1);
            } else point_over(hitter, 2);
        }
        bvx = bvy = 0;
        if (bx < 8 * FP) bx = 8 * FP;
        if (bx > 248 * FP) bx = 248 * FP;
        if (by < 20 * FP) by = 20 * FP;
        if (by > 238 * FP) by = 238 * FP;
    }
}

/* ====================================================================== */
/* Frappes                                                                 */
/* ====================================================================== */
static u8 can_hit(u8 t)
{
    s16 dx, dy;
    if (ms != MS_RALLY || !b_live || hitter == t) return 0;
    if (bounces > 1) return 0;
    if ((PX(by) < NET_Y ? 1 : 0) != side_of(t)) return 0;
    if (b_serve && bounces == 0) return 0;      /* le retour se joue après rebond */
    dx = PX(bx) - PX(plx[t]);
    dy = PX(by) - PX(ply[t]);
    if (dx < -20 || dx > 20 || dy < -12 || dy > 8) return 0;
    if (bz > 36 * FP) return 0;
    return 1;
}

/* pad : directions maintenues ; btn : PAD_A ou PAD_B ; power 0..32 */
static void do_hit(u8 t, u8 pad, u8 btn, u8 power, u8 spread)
{
    u8 side = side_of(t);
    s8 dir = side ? 1 : -1;          /* vers l'adversaire en y */
    s16 depth, tx, ty, dist, dx;
    u8 speed, grav, T;
    u8 smash = (u8)(bounces == 0 && bz > 20 * FP);
    u8 volley = (u8)(bounces == 0);

    dx = PX(bx) - PX(plx[t]);
    if (dx > 14 || dx < -14) spread += 8;          /* frappe en extension */

    bspin = SPIN_FLAT;
    if (smash) {
        depth = 62; speed = 80; grav = 10; spread += 4;
        sfx_crowd(5);
    } else if (btn == PAD_B && (pad & PAD_UP)) {        /* lob */
        depth = 74; speed = 24; grav = 6; bspin = SPIN_TOP;
    } else if (btn == PAD_B && (pad & PAD_DOWN)) {      /* amortie */
        depth = 18; speed = 20; grav = 9; bspin = SPIN_SLICE; spread += 3;
    } else if (btn == PAD_B) {                          /* coupé */
        depth = 66; speed = 36; grav = 4; bspin = SPIN_SLICE;
    } else if (pad & PAD_UP) {                          /* frappe à plat appuyée */
        depth = 78; speed = (u8)(46 + power); grav = 5; spread += 4;
    } else if (pad & PAD_DOWN) {                        /* croisé court */
        depth = 46; speed = (u8)(36 + (power >> 1)); grav = 8; bspin = SPIN_TOP;
    } else {                                            /* lifté */
        depth = 70; speed = (u8)(40 + power); grav = 7; bspin = SPIN_TOP;
    }
    if (volley && !smash) { depth = (s16)(depth - 10); if (speed > 42) speed = 42; }

    /* direction : croix gauche/droite, sinon croisé naturel */
    if (pad & PAD_LEFT) tx = SGL_L + hit_margin;
    else if (pad & PAD_RIGHT) tx = SGL_R - hit_margin;
    else tx = CENTER_X + ((CENTER_X - PX(plx[t])) >> 1);
    if (depth < 30 && !(pad & (PAD_LEFT | PAD_RIGHT))) tx = PX(plx[t]);
    spread = (u8)(spread + (power >> 2));
    if (spread > 100) spread = 100;
    tx += rnd_spread(spread);
    ty = NET_Y + dir * (depth + (rnd_spread((u8)(spread >> 1))));

    dist = iabs(ty - PX(by)) + (iabs(tx - PX(bx)) >> 2);
    dist = (dist * 16) / speed;
    if (dist > 200) dist = 200;
    T = (u8)dist;
    if (T < 14) T = 14;

    hitter = t;
    b_serve = 0;
    ++rally_hits;
    pl_hit[t] = 1;
    pl_flip[t] = (u8)(dx < 0);
    launch(tx, ty, T, grav);
    sfx_hit((u8)(power >> 3));
    /* l'adversaire IA réagit après un délai */
    ai_delay[t ^ 1] = ai_react[cfg.level[t ^ 1]];
}

/* ====================================================================== */
/* Service                                                                 */
/* ====================================================================== */
static void place_for_serve(void)
{
    u8 srv = serving_team(), rcv = (u8)(srv ^ 1);
    u8 ss = side_of(srv);
    u8 ad = rules_ad_side();
    u8 left = (u8)((ss == 0) == (ad == 0));     /* carré visé à gauche ? */
    plx[srv] = (left ? CENTER_X + 22 : CENTER_X - 22) * FP;
    ply[srv] = (ss == 0 ? BOT_BASE + 8 : TOP_BASE - 4) * FP;
    plx[rcv] = (left ? CENTER_X - 46 : CENTER_X + 46) * FP;
    ply[rcv] = (ss == 0 ? TOP_BASE - 2 : BOT_BASE + 6) * FP;
    pl_state[0] = pl_state[1] = ST_IDLE;
    ai_net[0] = ai_net[1] = 0;
    pl_charge[0] = pl_charge[1] = 0;
    b_vis = 0;
    b_live = 0;
    rally_hits = 0;
    ms = MS_SERVE;
    ms_timer = (u8)(40 + (rand8() & 31));
    if (cfg.mode == GM_TRAIN_RALLY) {
        /* le lanceur sert toujours depuis le haut vers le joueur (équipe 0 en bas) */
        ms_timer = 50;
    }
    hud_dirty = 1;
}

static void toss(u8 t)
{
    bx = plx[t] + 5 * FP;
    by = ply[t];
    bz = 18 * FP;
    bvx = bvy = 0;
    bvz = 150;
    bgrav = 6;
    b_vis = 1;
    b_live = 0;
    pl_state[t] = ST_TOSS;
    ms = MS_TOSS;
    sfx_toss();
}

/* frappe de service ; quality : 0 parfait .. grand = mauvais */
static void serve_hit(u8 t, u8 pad, u8 btn, u8 quality, u8 spread)
{
    u8 ss = side_of(t);
    u8 ad = rules_ad_side();
    u8 left = (u8)((ss == 0) == (ad == 0));
    s16 tx, ty, dist;
    u8 speed, grav, T;
    s8 dir = ss ? 1 : -1;

    if (btn == PAD_B) { speed = 44; grav = 7; bspin = SPIN_SLICE; }
    else { speed = 64; grav = 9; bspin = SPIN_FLAT; spread += 4; }
    spread = (u8)(spread + quality);
    /* croix : extérieur / au T */
    if (left) tx = (pad & PAD_LEFT) ? SGL_L + 8 : (pad & PAD_RIGHT) ? CENTER_X - 6 : CENTER_X - 26;
    else      tx = (pad & PAD_RIGHT) ? SGL_R - 8 : (pad & PAD_LEFT) ? CENTER_X + 6 : CENTER_X + 26;
    tx += rnd_spread(spread);
    ty = NET_Y + dir * (40 + rnd_spread((u8)(spread >> 1)) + (quality >> 1));
    dist = iabs(ty - PX(by)) + (iabs(tx - PX(bx)) >> 2);
    T = (u8)((dist * 16) / speed);
    if (T < 14) T = 14;
    hitter = t;
    b_serve = 1;
    rally_hits = 1;
    if (faults == 0) ++st_first_tot[t];
    launch(tx, ty, T, grav);
    ms = MS_RALLY;
    pl_state[t] = ST_SWING;
    pl_timer[t] = 12;
    sfx_hit(btn == PAD_B ? 1 : 3);
    ai_delay[t ^ 1] = ai_react[cfg.level[t ^ 1]];
}

/* ====================================================================== */
/* Contrôle humain                                                         */
/* ====================================================================== */
static void move_toward(u8 t, s16 tx, s16 ty, u8 speed)
{
    s16 dx = tx - plx[t], dy = ty - ply[t];
    s16 sx = speed, sy = speed;
    pl_moving[t] = 0;
    if (iabs(dx) > 2 * FP && iabs(dy) > 2 * FP) { sx = (s16)((speed * 3) >> 2); sy = sx; }
    if (dx > sx) { plx[t] += sx; pl_moving[t] = 1; }
    else if (dx < -sx) { plx[t] -= sx; pl_moving[t] = 1; }
    else plx[t] = tx;
    if (dy > sy) { ply[t] += sy; pl_moving[t] = 1; }
    else if (dy < -sy) { ply[t] -= sy; pl_moving[t] = 1; }
    else ply[t] = ty;
}

static void clamp_player(u8 t)
{
    s16 ymin, ymax;
    if (side_of(t) == 0) { ymin = (NET_Y + 10) * FP; ymax = 228 * FP; }
    else { ymin = 44 * FP; ymax = (NET_Y - 6) * FP; }
    if (ms == MS_SERVE && t == serving_team()) {
        /* le serveur reste derrière sa ligne de fond, dans sa moitié */
        if (side_of(t) == 0) ymin = (BOT_BASE + 6) * FP; else ymax = (TOP_BASE - 3) * FP;
    }
    if (plx[t] < 36 * FP) plx[t] = 36 * FP;
    if (plx[t] > 220 * FP) plx[t] = 220 * FP;
    if (ply[t] < ymin) ply[t] = ymin;
    if (ply[t] > ymax) ply[t] = ymax;
}

static void human_update(u8 t, u8 pad, u8 pressed)
{
    s16 sp = HUMAN_SPEED;
    s16 nx = plx[t], ny = ply[t];
    u8 st = pl_state[t];

    if (ms == MS_SERVE && t == serving_team()) {
        if (pressed & (PAD_A | PAD_B)) { toss(t); return; }
        if (pad & PAD_LEFT) plx[t] -= 64;
        if (pad & PAD_RIGHT) plx[t] += 64;
        /* rester du bon côté du centre */
        {
            u8 left = (u8)((side_of(t) == 0) == (rules_ad_side() == 0));
            if (left && plx[t] < (CENTER_X + 4) * FP) plx[t] = (CENTER_X + 4) * FP;
            if (left && plx[t] > (SGL_R + 10) * FP) plx[t] = (SGL_R + 10) * FP;
            if (!left && plx[t] > (CENTER_X - 4) * FP) plx[t] = (CENTER_X - 4) * FP;
            if (!left && plx[t] < (SGL_L - 10) * FP) plx[t] = (SGL_L - 10) * FP;
        }
        pl_moving[t] = (u8)((pad & (PAD_LEFT | PAD_RIGHT)) != 0);
        return;
    }
    if (ms == MS_TOSS && t == serving_team()) {
        if (pressed & (PAD_A | PAD_B)) {
            s16 q = iabs(bvz) >> 3;          /* 0 au sommet du lancer */
            serve_hit(t, pad, (u8)(pressed & PAD_A ? PAD_A : PAD_B), (u8)(q > 20 ? 20 : q), 3);
        }
        return;
    }
    if (ms == MS_SERVE || ms == MS_TOSS) {
        /* receveur : petits ajustements latéraux avant le service */
        if (pad & PAD_LEFT) plx[t] -= 64;
        if (pad & PAD_RIGHT) plx[t] += 64;
        clamp_player(t);
        return;
    }

    /* frappe : appui = armer (la puissance monte), relâcher = frapper */
    if (st == ST_IDLE && (pressed & (PAD_A | PAD_B))) {
        pl_state[t] = st = ST_CHARGE;
        pl_btn[t] = (u8)(pressed & PAD_A ? PAD_A : PAD_B);
        pl_charge[t] = 0;
        pl_hit[t] = 0;
    }
    if (st == ST_CHARGE) {
        if (pad & pl_btn[t]) { if (pl_charge[t] < 32) ++pl_charge[t]; }
        else { pl_state[t] = st = ST_SWING; pl_timer[t] = 14; }
        sp = HUMAN_SPEED / 2;
    }
    if ((st == ST_CHARGE || (st == ST_SWING && pl_timer[t] > 4)) && !pl_hit[t] && can_hit(t)) {
        do_hit(t, pad, pl_btn[t], pl_charge[t], 3);
        pl_state[t] = ST_SWING;
        pl_timer[t] = 10;
    }
    if (st == ST_SWING) {
        sp = HUMAN_SPEED / 3;
        if (--pl_timer[t] == 0) pl_state[t] = ST_IDLE;
    }
    if (ms == MS_DEAD || ms == MS_OVER) sp = HUMAN_SPEED / 2;

    if (pad & PAD_LEFT) nx -= sp;
    if (pad & PAD_RIGHT) nx += sp;
    if (pad & PAD_UP) ny -= sp;
    if (pad & PAD_DOWN) ny += sp;
    pl_moving[t] = (u8)(nx != plx[t] || ny != ply[t]);
    plx[t] = nx; ply[t] = ny;
    clamp_player(t);
}

/* ====================================================================== */
/* Intelligence artificielle                                               */
/* ====================================================================== */
static void ai_update(u8 t)
{
    u8 lv = cfg.level[t];
    u8 side = side_of(t);
    u8 opp = (u8)(t ^ 1);
    s16 tx, ty;
    u8 pad = 0, btn = PAD_A;

    if (ms == MS_SERVE) {
        if (t == serving_team()) {
            if (cfg.mode == GM_TRAIN_RALLY && --ms_timer == 0) {
                /* lanceur de balles : envoie une balle vers le joueur */
                bx = (CENTER_X + rnd_spread(30)) * FP; by = (TOP_BASE - 4) * FP; bz = 22 * FP;
                b_vis = 1; hitter = t; b_serve = 0; rally_hits = 0;
                ++tr_total; hud_dirty = 1;
                tx = CENTER_X + rnd_spread(40);
                ty = BOT_BASE - 30 + rnd_spread(12);
                bspin = SPIN_TOP;
                launch(tx, ty, 50, 9);
                ms = MS_RALLY;
                sfx_hit(1);
                tr_target = (u8)(rand8() & 1);
                return;
            }
            if (cfg.mode != GM_TRAIN_RALLY && --ms_timer == 0) toss(t);
        }
        pl_moving[t] = 0;
        return;
    }
    if (ms == MS_TOSS) {
        if (t == serving_team() && bvz <= 0) {
            u8 q = (u8)(rand8() % (u8)(10 - lv * 3));
            if (faults == 0) serve_hit(t, (u8)(rand8() & 1 ? PAD_LEFT : PAD_RIGHT), PAD_A, q, (u8)(ai_spread[lv] >> 2));
            else serve_hit(t, 0, PAD_B, (u8)(q >> 1), (u8)(ai_spread[lv] >> 3));
        }
        return;
    }
    if (cfg.mode == GM_TRAIN_RALLY || cfg.mode == GM_TRAIN_SERVE) {
        /* adversaire d'entraînement immobile */
        pl_moving[t] = 0;
        return;
    }

    if (ms == MS_RALLY && b_live && hitter != t) {
        if (ai_delay[t]) { --ai_delay[t]; pl_moving[t] = 0; return; }
        if (bounces == 0) {
            /* point de frappe prévu : un peu après le rebond annoncé */
            tx = land_x + PX(bvx * 12);
            ty = land_y + PX(bvy * 12);
            /* joueur au filet : interception de volée */
            if (iabs(PX(ply[t]) - NET_Y) < 46 && !b_serve) {
                ty = PX(ply[t]);
                if (bvy != 0) tx = PX(bx) + (s16)(((long)(ty - PX(by)) * bvx) / bvy);
            }
        } else {
            tx = PX(bx + bvx * 6);
            ty = PX(by + bvy * 6);
        }
        /* se placer pour avoir la balle sur le côté */
        tx += (PX(plx[t]) < tx) ? -10 : 10;
        if (side == 0 && ty < NET_Y + 12) ty = NET_Y + 12;
        if (side == 1 && ty > NET_Y - 8) ty = NET_Y - 8;
        move_toward(t, tx * FP, ty * FP, ai_speed[lv]);
        clamp_player(t);

        if (can_hit(t) && (bounces == 1 || bz > 14 * FP || iabs(PX(ply[t]) - NET_Y) < 50)) {
            u8 spread = ai_spread[lv];
            s16 ox = PX(plx[opp]);
            /* viser loin de l'adversaire */
            if (ox > CENTER_X + 10) pad = PAD_LEFT;
            else if (ox < CENTER_X - 10) pad = PAD_RIGHT;
            else pad = (u8)(rand8() & 1 ? PAD_LEFT : PAD_RIGHT);
            /* choix tactiques selon le niveau */
            if (lv >= 1 && iabs(PX(ply[opp]) - NET_Y) < 50 && (rand8() & 3) == 0) {
                btn = PAD_B; pad |= PAD_UP;                     /* lob sur le volleyeur */
            } else if (lv == 2 && iabs(PX(ply[opp]) - NET_Y) > 90 && (rand8() & 7) == 0) {
                btn = PAD_B; pad |= PAD_DOWN;                   /* amortie */
            } else if ((rand8() & 3) == 0) {
                btn = PAD_B;                                    /* coupé de variation */
            }
            if (lv == 0 && (rand8() & 7) == 0) spread += 20;    /* faute directe */
            /* la marge visée (distance aux lignes) dépend du niveau */
            hit_margin = ai_margin[lv];
            do_hit(t, pad, btn, ai_power[lv], spread);
            pl_state[t] = ST_SWING;
            pl_timer[t] = 12;
            /* montée au filet après une frappe à l'intérieur du court */
            if (lv >= 1 && iabs(PX(ply[t]) - NET_Y) < 66 && (rand8() & (lv == 2 ? 1 : 3)) == 0) ai_net[t] = 1;
        }
    } else {
        /* replacement : centre, derrière la ligne de fond (ou au filet) */
        tx = CENTER_X + ((PX(bx) - CENTER_X) >> 2);
        if (ai_net[t]) ty = side ? NET_Y - 34 : NET_Y + 36;
        else ty = side ? TOP_BASE - 6 : BOT_BASE + 8;
        if (ms == MS_DEAD) ty = PX(ply[t]);
        move_toward(t, tx * FP, ty * FP, (u8)(ai_speed[lv] - 20));
        clamp_player(t);
    }
    if (pl_state[t] == ST_SWING && --pl_timer[t] == 0) pl_state[t] = ST_IDLE;
}

/* ====================================================================== */
/* Score affiché                                                           */
/* ====================================================================== */
static void hud_update(void)
{
    static char line[16];
    u8 t;
    vbuf_reset();
    if (cfg.mode == GM_MATCH) {
        for (t = 0; t < 2; ++t) {
            const char *n = char_name[cfg.chr[t]];
            line[0] = n[0]; line[1] = n[1]; line[2] = n[2]; line[3] = n[3];
            line[4] = ' ';
            num2(line + 5, sc_sets[t]);
            line[7] = ' ';
            num2(line + 8, sc_games[t]);
            line[10] = ' ';
            rules_point_label(t, line + 11);
            line[13] = ' ';
            line[14] = (char)(serving_team() == t ? '*' : ' ');
            line[15] = 0;
            vbuf_text(2, (u8)(1 + t), line, 15);
        }
    } else {
        vbuf_text(2, 1, cfg.mode == GM_TRAIN_RALLY ? "FRAPPES" : "SERVICE", 15);
        line[0] = 'P'; line[1] = 'T'; line[2] = 'S'; line[3] = ' ';
        num2(line + 4, tr_score); line[6] = '/';
        num2(line + 7, tr_total); line[9] = 0;
        vbuf_text(2, 2, line, 15);
    }
    if (paused) {
        vbuf_text(19, 1, "  PAUSE", 11);
        vbuf_text(19, 2, "SEL:QUITTER", 11);
    } else if (msg_timer) {
        vbuf_text(19, 1, msg1, 11);
        vbuf_text(19, 2, msg2, 11);
    } else {
        vbuf_text(19, 1, "", 11);
        vbuf_text(19, 2, sc_tiebreak ? "JEU DECISIF" : "", 11);
    }
    hud_dirty = 0;
}

/* ====================================================================== */
void match_start(void)
{
    u8 t;
    rules_init(cfg.sets_to_win, cfg.games_per_set, (u8)(cfg.mode == GM_MATCH ? (rand8() & 1) : 1));
    if (cfg.mode == GM_TRAIN_SERVE) sc_server = 0;
    for (t = 0; t < 2; ++t) {
        st_points[t] = st_aces[t] = st_dfaults[t] = st_winners[t] = st_errors[t] = 0;
        st_first_in[t] = st_first_tot[t] = 0;
        ai_net[t] = 0;
        pl_flip[t] = 0;
    }
    st_longest = 0;
    tr_score = tr_total = 0;
    faults = 0;
    paused = 0;
    match_quit = 0;
    msg_timer = 0;
    place_for_serve();
    set_msg(cfg.mode == GM_MATCH ? "JEU !" : "A VOUS !", "", 60);
}

static void after_point(void)
{
    u8 ev, w = pending_winner;
    if (end_reason == END_FAULT) {
        if (cfg.mode == GM_MATCH) faults = 1;
        if (cfg.mode == GM_TRAIN_SERVE) sc_points[0] ^= 1;   /* alterne les côtés */
        place_for_serve();
        return;
    }
    faults = 0;
    if (cfg.mode != GM_MATCH) { place_for_serve(); return; }
    ev = rules_point(w);
    if (ev & EV_MATCH) {
        ms = MS_OVER;
        ms_timer = 160;
        set_msg("JEU, SET", "ET MATCH", 160);
        sfx_whistle();
        music_play(cfg.ctrl[w] == CTRL_CPU && cfg.ctrl[w ^ 1] != CTRL_CPU ? MUS_DEFEAT : MUS_VICTORY);
        sfx_crowd(7);
        return;
    }
    if (ev & EV_SET) { set_msg("JEU ET SET", char_name[cfg.chr[w]], 90); music_play(MUS_GAME); }
    else if (ev & EV_TIEBREAK) { set_msg("6 PARTOUT", "JEU DECISIF", 90); music_play(MUS_GAME); }
    else if (ev & EV_GAME) { set_msg("JEU", char_name[cfg.chr[w]], 70); music_play(MUS_GAME); }
    else {
        if (rules_is_match_point(0) || rules_is_match_point(1)) set_msg("BALLE DE", "MATCH", 70);
        else if (rules_is_set_point(0) || rules_is_set_point(1)) set_msg("BALLE DE", "SET", 70);
        else if (!sc_tiebreak && sc_points[0] >= 3 && sc_points[0] == sc_points[1]) set_msg("EGALITE", "", 60);
        else if (!sc_tiebreak && sc_points[0] >= 3 && sc_points[1] >= 3) set_msg("AVANTAGE", char_name[cfg.chr[sc_points[0] > sc_points[1] ? 0 : 1]], 60);
        music_play(MUS_POINT);
    }
    if (ev & EV_CHANGE) set_msg("CHANGEMENT", "DE COTE", 90);
    place_for_serve();
}

u8 match_tick(void)
{
    u8 t, ctrl, pad, pressed;

    /* pause : START ; SELECT pendant la pause = quitter */
    if ((pad1_new & PAD_START) && ms != MS_OVER) { paused ^= 1; hud_dirty = 1; }
    if (paused) {
        if (pad1_new & PAD_SELECT) { match_quit = 1; return 1; }
        if (hud_dirty) hud_update();
        return 0;
    }

    for (t = 0; t < 2; ++t) {
        ctrl = cfg.ctrl[t];
        if (ctrl == CTRL_CPU) ai_update(t);
        else {
            pad = ctrl == CTRL_PAD1 ? pad1 : pad2;
            pressed = ctrl == CTRL_PAD1 ? pad1_new : pad2_new;
            hit_margin = 8;
            human_update(t, pad, pressed);
        }
        if (pl_moving[t]) ++pl_walk[t];
    }

    if (ms == MS_TOSS) {
        bvz -= bgrav;
        bz += bvz;
        if (bz < 14 * FP && bvz < 0) {
            /* lancer raté : on recommence sans pénalité */
            b_vis = 0;
            ms = MS_SERVE;
            ms_timer = 30;
            pl_state[serving_team()] = ST_IDLE;
        }
    } else {
        ball_update();
    }

    if (ms == MS_DEAD) {
        if (--ms_timer == 0) after_point();
    } else if (ms == MS_OVER) {
        if (--ms_timer == 0) return 1;
    }

    if (msg_timer) { if (--msg_timer == 0) hud_dirty = 1; }
    if (hud_dirty) hud_update();
    return 0;
}

static u8 pose_of(u8 t)
{
    u8 st = pl_state[t];
    if (ms == MS_OVER) return (u8)((ms_timer & 16) ? 5 : 6);
    if (st == ST_TOSS) return 5;
    if (st == ST_SWING) return (u8)(pl_timer[t] > 6 ? 3 : 4);
    if (st == ST_CHARGE) return 6;
    if (pl_moving[t]) return (u8)((pl_walk[t] & 8) ? 1 : 2);
    return 0;
}

void match_draw(void)
{
    u8 order0, t, i;
    s16 sy;
    spr_begin();
    /* balle et ombre en tête (priorité d'affichage maximale) */
    if (b_vis) {
        sy = PX(by) - PX(bz) - 3;
        if (sy < 8) sy = 8;
        spr1((u8)(PX(bx) - 3), (u8)sy, 1, 2);
        spr1((u8)(PX(bx) - 3), (u8)(PX(by) - 3), 2, 2);
    }
    /* entraînement : cible */
    if (cfg.mode == GM_TRAIN_RALLY) {
        spr1(tr_target ? 158 : 94, 64, 3, 3);
    }
    /* le joueur le plus bas à l'écran est dessiné devant */
    order0 = (u8)(ply[0] > ply[1] ? 0 : 1);
    for (i = 0; i < 2; ++i) {
        t = (u8)(i ? order0 ^ 1 : order0);
        if (cfg.mode == GM_TRAIN_RALLY && t == 1) continue;   /* lanceur invisible */
        spr_person(PX(plx[t]), PX(ply[t]), cfg.chr[t], side_of(t), pose_of(t),
                   (u8)(pl_state[t] == ST_SWING ? pl_flip[t] : 0), t);
    }
    spr_end();
}
