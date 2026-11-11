/* Royal Martinot — moteur de match : déplacements, frappes, service,
 * physique de la balle, arbitrage et intelligence artificielle.
 * Gère le simple (joueurs 0 et 1) et le double (joueurs 0..3,
 * équipe = index & 1).
 *
 * Unités : positions et vitesses en virgule fixe 1/64 de pixel (FP).
 * La hauteur de balle (z) utilise la même unité ; elle est dessinée en
 * décalant la balle vers le haut par rapport à son ombre. */
#include "game_int.h"
#include "app.h"
#include "rules.h"
#include "court.h"
#include "gfx.h"
#include "audio.h"

#pragma static-locals(on)

MatchCfg cfg;
u8 match_quit;
u8 st_points[2], st_aces[2], st_dfaults[2], st_winners[2], st_errors[2];
u8 st_first_in[2], st_first_tot[2], st_longest;
u8 st_canon[2], st_ctry[2];      /* services canon réussis, services A tentés (humains) */
u8 srv_canon;                    /* le dernier service est un service canon */
u8 tr_score, tr_total, tr_best;
u8 tr_ev, tr_done, tr_human_serves, tr_step, tr_target, tr_streak;
s16 ev_x, ev_y;
s16 feed_x, feed_y, feed_z, feed_tx, feed_ty, feed_px, feed_py;
u8 feed_T, feed_g, feed_spin, feed_px_set;
u8 crowd_timer, rp_pending, rp_done;
u8 last_shot, last_aim;
u8 pose_cache[4], flip_cache[4];
static u8 tr_wait;
u8 b_netcord;
u8 pause_cur;
u8 tgt_x, tgt_y;
/* IA adaptative : tendances observées des joueurs humains, par équipe */
static u8 ad_l[2], ad_r[2], ad_net[2], ad_n[2];

/* ---------------- joueurs (non statiques : lus par les tests) ---------- */
u8 npl;                         /* 2 en simple, 4 en double */
s16 plx[4], ply[4];
u8 pl_state[4], pl_timer[4];        /* lus par poses_all (banque 3) */
static u8 pl_charge[4], pl_btn[4], pl_hit[4];
u8 pl_walk[4], pl_moving[4], pl_flip[4];
u8 ai_delay[4];                  /* lu par les tests */
static u8 ai_net[4];
/* Fonctions utilitaires et calcul de trajectoire : banque fixe (gamefix.c),
   pour libérer la banque du moteur */
s16 mulfrac(s16 v, u8 k);
s16 iabs(s16 v);
s8 rnd_spread(u8 spread);
void set_msg(const char *a, const char *b, u8 t);
void launch(s16 tx, s16 ty, u8 T, u8 grav);
const char *win_label(u8 shot, u8 pass, u8 jump);
void match_fx(void);
static u8 hit_margin;
static s16 pcache_x[4], pcache_y[4];   /* prédictions IA en cache */
s8 ai_err[4];                          /* erreur d'anticipation tirée à chaque frappe */
static s16 ai_gx[4], ai_gy[4];         /* destination courante de l'IA (FP) */
static u8 ai_x2;                       /* 1 : pas de déplacement doublé */
static u8 hit_now;                     /* une frappe a eu lieu dans cette trame */
u8 pl_jump[4];                         /* smash sauté : trames de saut restantes */
static u8 win_party(void);
static u8 rel_pad(u8 p, u8 pad);
extern u8 pose_now[4];                 /* poses de la trame (poses_all, banque 3) */
void poses_all(void);
u8 hit_pass, hit_jump;                 /* dernière frappe : adversaire au filet, smash sauté */
u8 tired;                              /* fatigue de l'échange en cours, 0..10 */
u8 srv_slot[2];                 /* double : quel partenaire sert pour l'équipe */
#define ST_IDLE   0
#define ST_CHARGE 1
#define ST_SWING  2
#define ST_TOSS   3

/* ---------------- balle ----------------------------------------------- */
s16 bx, by, bz, bvx, bvy, bvz;
u8 bgrav, bspin, bounces, hitter, hitter_p, b_serve, b_vis, b_live;
s16 land_x, land_y;
u8 rally_hits;

/* ---------------- état du match ----------------------------------------- */
u8 ms, ms_timer, faults, paused;
u8 end_reason, pending_winner;
u8 hud_dirty, msg_timer;
char msg1[16], msg2[16];


/* surfaces : restitution et adhérence au rebond (sur 256) */
static const u8 surf_rest[6] = { 196, 150, 176, 160, 214, 116 };
static const u8 surf_fric[6] = { 200, 244, 226, 236, 170, 253 };
/* IA : vitesse (FP/trame), délai de réaction, dispersion, marge visée */
/* vitesses (FP/trame) ; ≤ 127 car doublées en double (ai_x2) */
/* réglages v2.3 : Normal rendu plus abordable (retour de test joueur) */
static const u8 ai_speed[3]  = { 74, 86, 110 };
static const u8 ai_react[3]  = { 22, 16, 6 };
static const u8 ai_spread[3] = { 34, 26, 16 };
static const u8 ai_margin[3] = { 28, 22, 11 };
static const u8 ai_power[3]  = { 0, 4, 14 };
static const u8 ai_guess[3]  = { 18, 12, 5 };    /* imprécision d'anticipation (px) */
static const u8 ai_fault[3]  = { 3, 7, 15 };   /* faute directe : 1 frappe sur 4 / 8 / 16 */
#define HUMAN_SPEED 140

/* styles de jeu (CDC 10.2), indexés par char_style[] :
   équilibré, fond de court, puissance, filet, défensif, offensif, tactique, placement */
/* tables des styles : banque fixe (gfx.c), pour libérer la banque du moteur */
extern const s8 sty_pow[9], sty_margin[9], sty_spread[9];
extern const u8 sty_net[9], sty_lob[9], sty_drop[9], sty_slice[9], sty_deep[9];


#define side_of(t) ((u8)(((t) & 1) ^ sc_swap))   /* 0 = bas, 1 = haut */

/* v * k / 256 en 16 bits (|v| < 2600) : évite la multiplication 32 bits */



/* joueur au service et receveur (simple : index = équipe) */
static u8 server_p(void)
{
    u8 t = sc_server;
    return (u8)(cfg.doubles ? t + 2 * srv_slot[t] : t);
}
static u8 receiver_p(void)
{
    u8 t = (u8)(sc_server ^ 1);
    /* double : le partenaire 0 reçoit côté égalité, le 2 côté avantage */
    return (u8)(cfg.doubles ? t + 2 * rules_ad_side() : t);
}

/* ====================================================================== */
/* Lancement de la balle : atteindre (tx, ty) en T trames                  */
/* ====================================================================== */

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
    ump_arm = (u8)(reason == 1);     /* faute adverse (dehors, filet) : bras tendu */
    pending_winner = winner;
    ++st_points[winner];
    if (rally_hits > st_longest) st_longest = rally_hits;
    if (cfg.mode != GM_MATCH) {
        if (winner == 1) { tr_ev = TEV_MISS; bank_call(BANK_EXTRAS, train_event); }
        end_reason = END_FAULT;
        ms_timer = 40;
        return;
    }
    if (reason == 2) {
        ++st_winners[winner];
        if (rally_hits == 1) { ++st_aces[winner]; set_msg("ACE !", "", 70); }
        else set_msg(win_label(last_shot, hit_pass, hit_jump), "", 70);
        sfx_winner();   /* carillon bref (canal des bruitages) */
        sfx_crowd(7);
        crowd_timer = 64;
        /* ralenti : aces, beaux échanges, et coups parfaits gagnants même en retour */
        if (rally_hits >= 3 || rally_hits == 1 || hit_perf) rp_pending = 1;
    } else if (reason == 1) {
        ++st_errors[winner ^ 1];
        sfx_crowd(2);
    }
    if (rally_hits >= 12) { set_msg("QUEL ECHANGE !", "", 70); sfx_crowd(7); crowd_timer = 64; rp_pending = 1; }
    if (!cfg.replays || cfg.demo) rp_pending = 0;
    if (rp_pending) ms_timer = 110;
}

static void serve_fault(const char *why)
{
    ms = MS_DEAD;
    ms_timer = 60;
    b_live = 0;
    ump_arm = 1;
    sfx_svfault();       /* service faux : bip du juge électronique et « Fault! » */
    if (cfg.mode == GM_TRAIN) {
        end_reason = END_FAULT;
        set_msg("FAUTE", why, 50);
        tr_ev = TEV_SERVE_FAULT; bank_call(BANK_EXTRAS, train_event);
        return;
    }
    if (faults == 0) {
        end_reason = END_FAULT;
        set_msg("FAUTE", why, 55);
    } else {
        ++st_dfaults[sc_server];
        set_msg("DOUBLE FAUTE", "", 70);
        end_reason = END_POINT;
        pending_winner = (u8)(sc_server ^ 1);
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
    s16 l = cfg.doubles ? DBL_L : SGL_L, r = cfg.doubles ? DBL_R : SGL_R;

    if (bspin == SPIN_TOP)   { rest = (u8)(rest + 24); if (fric < 240) fric += 12; }
    if (bspin == SPIN_SLICE) { rest = (u8)(rest - 46); }
    if (bspin == SPIN_DROP) { rest >>= 1; fric >>= 1; }   /* amortie : rebond bas, balle freinée */
    if (cfg.cond == COND_RAIN) { rest = (u8)(rest - 20); if (fric < 248) fric += 6; }  /* court glissant */
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
            if (b_netcord) {
                /* service qui touche le filet et tombe dans le carré : à remettre */
                ms = MS_DEAD; ms_timer = 50; b_live = 0; end_reason = END_LET;
                set_msg("LET", "A REMETTRE", 50);
                sfx_let();
                ++bounces;
                return;
            }
            if (faults == 0) ++st_first_in[hitter];
            if (cfg.mode == GM_TRAIN) {
                ev_x = x; ev_y = y;
                tr_ev = TEV_SERVE_IN; bank_call(BANK_EXTRAS, train_event);
                ms = MS_DEAD; ms_timer = 50; b_live = 0; end_reason = END_FAULT;
                ++bounces;
                return;
            }
        } else if (x < l || x > r || y < TOP_BASE || y > BOT_BASE) {
            set_msg("DEHORS", "", 50);
            if (cfg.mode == GM_TRAIN && hitter == 0) {
                tr_ev = TEV_LAND_OUT; bank_call(BANK_EXTRAS, train_event);
                ms = MS_DEAD; ms_timer = 40; b_live = 0; end_reason = END_FAULT;
                ++bounces;
                return;
            }
            point_over(recv, 1);
            ++bounces;
            return;
        } else if (cfg.mode == GM_TRAIN && hitter == 0) {
            /* entraînement : balle bonne ; l'exercice attribue les points */
            ev_x = x; ev_y = y;
            tr_ev = TEV_LAND_IN; bank_call(BANK_EXTRAS, train_event);
            if (cfg.ex != EX_STATIC) {
                ms = MS_DEAD; ms_timer = 40; b_live = 0; end_reason = END_FAULT;
                ++bounces;
                return;
            }
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
        && bz >= (NET_H - 2) * FP && b_live) {
        /* la balle effleure la bande du filet et passe, ralentie */
        bvy >>= 1;
        bvx >>= 1;
        bvz = 40;
        b_netcord = 1;
        sfx_net();
        net_shake = 14;                 /* le filet tremble */
    } else if (((oldy < NET_Y * FP) != (by < NET_Y * FP)) && bz < NET_H * FP
        && bx > (DBL_L - 6) * FP && bx < (DBL_R + 6) * FP) {
        by = oldy;
        bvy = -(bvy >> 2);
        bvx >>= 1;
        if (bvz > 0) bvz = 0;
        sfx_net();
        net_shake = 14;                 /* le filet tremble */
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
                if (b_serve) serve_fault("");
                else point_over((u8)(hitter ^ 1), 1);
            } else point_over(hitter, 2);
        }
    }
    /* la balle ratée poursuit sa course et disparaît une fois sortie de
       l'écran (au lieu de rester bloquée contre le bord) */
    if (bx < 4 * FP || bx > 252 * FP || by < 0 || by > 248 * FP) {
        b_vis = 0;
        bvx = bvy = bvz = 0;
    }
}

/* ====================================================================== */
/* Frappes                                                                 */
/* ====================================================================== */
/* can_hit : banque fixe (gamefix.c) */
u8 can_hit(u8 p);
void score_recap(void);
u8 recap_pend, brk_pend;          /* récapitulatif du score à afficher / fin de set */
u8 perfect_hit(u8 p, s16 dx);
void serve_fb(s16 vz);

/* pad : directions maintenues ; btn : PAD_A ou PAD_B ; power 0..32 */
static void do_hit(u8 p, u8 pad, u8 btn, u8 power, u8 spread)
{
    u8 t = (u8)(p & 1);
    u8 side = side_of(t);
    s8 dir = side ? 1 : -1;          /* vers l'adversaire en y */
    s16 depth, tx, ty, dist, dx;
    u8 speed, grav, T;
    u8 smash = (u8)(bounces == 0 && bz > 20 * FP);
    u8 volley = (u8)(bounces == 0);
    s16 l = cfg.doubles ? DBL_L : SGL_L, r = cfg.doubles ? DBL_R : SGL_R;

    dx = PX(bx) - PX(plx[p]);
    if (dx > 14 || dx < -14) spread += 8;          /* frappe en extension */

    bspin = SPIN_FLAT;
    hit_jump = (u8)(bz > 40 * FP);
    if (hit_jump) pl_jump[p] = 14;          /* le joueur saute, frappe en l'air et retombe */
    if (smash) {
        depth = 62; speed = 80; grav = 10; spread += 4; last_shot = SH_SMASH;
        if (hit_jump) { depth = 56; speed = 96; grav = 12; spread += 4; }
        sfx_crowd(5);
    } else if (btn == PAD_B && (pad & PAD_UP)) {        /* lob */
        depth = 74; speed = 24; grav = 6; bspin = SPIN_TOP; last_shot = SH_LOB;
    } else if (btn == PAD_B && (pad & PAD_DOWN)) {      /* amortie */
        /* amortie : trajectoire basse et vive juste au-dessus du filet, puis la
           balle « meurt » au rebond (effet SPIN_DROP) */
        /* plus elle est jouée loin du filet, plus elle est imprécise et risque
           de rester trop courte (filet) : jamais un point garanti */
        T = (u8)(iabs(PX(by) - NET_Y) >> 3);
        depth = (s16)(22 - T); speed = 40; grav = 5; bspin = SPIN_DROP; spread += T; last_shot = SH_DROP;
    } else if (btn == PAD_B) {                          /* coupé */
        depth = 66; speed = 36; grav = 4; bspin = SPIN_SLICE; last_shot = SH_SLICE;
    } else if (pad & PAD_UP) {                          /* à plat : tendue, rapide, risquée */
        depth = 78; speed = (u8)(54 + power); grav = 4; spread += 7; last_shot = SH_FLAT;
    } else if (pad & PAD_DOWN) {                        /* croisé court : en angle, carré de service */
        depth = 30; speed = (u8)(34 + (power >> 1)); grav = 9; bspin = SPIN_TOP; spread += 3; last_shot = SH_SHORT;
    } else {                                            /* lifté */
        depth = 70; speed = (u8)(40 + power); grav = 7; bspin = SPIN_TOP; last_shot = SH_DRIVE;
    }
    if (volley && !smash) {
        depth = (s16)(depth - 10);
        if (speed > 42) speed = 42;
        if (last_shot == SH_DRIVE || last_shot == SH_FLAT) last_shot = SH_VOLLEY;
    }
    /* frappe parfaite (lifté, à plat, coupé, croisé court ; banque fixe) :
       +37,5 % de vitesse, presque sans écart */
    hit_perf = perfect_hit(p, dx);
    if (hit_perf) { speed = (u8)(speed + (speed >> 2) + (speed >> 3)); spread = 1; power = 0; }
    /* fatigue (long échange) : faute de jambes, on frappe plus fort pour
       conclure, mais moins précis : jusqu'à +23 % de vitesse et + 5 d'écart
       (tired 0..10) ; la frappe parfaite reste précise */
    else if (tired) { speed = (u8)(speed + (u8)(((u16)speed * (u8)(tired * 3)) >> 7)); spread = (u8)(spread + (tired >> 1)); }
    last_aim = (u8)(pad & (PAD_LEFT | PAD_RIGHT));

    /* direction : croix gauche/droite, sinon croisé naturel */
    if (pad & PAD_LEFT) tx = l + hit_margin;
    else if (pad & PAD_RIGHT) tx = r - hit_margin;
    else tx = CENTER_X + ((CENTER_X - PX(plx[p])) >> 1);
    if (depth < 30 && !(pad & (PAD_LEFT | PAD_RIGHT))) tx = PX(plx[p]);
    /* croisé court : près de la ligne de côté (croisé, sauf direction) */
    if (last_shot == SH_SHORT)
        tx = ((pad & PAD_LEFT) || (!(pad & PAD_RIGHT) && PX(plx[p]) > CENTER_X)) ? l + 6 : r - 6;
    spread = (u8)(spread + (power >> 2));
    if (spread > 100) spread = 100;
    tx += rnd_spread(spread);
    ty = NET_Y + dir * (depth + (rnd_spread((u8)(spread >> 1))));

    dist = iabs(ty - PX(by)) + (iabs(tx - PX(bx)) >> 2);
    dist = (dist * 16) / speed;
    if (dist > 200) dist = 200;
    T = (u8)dist;
    if (T < 14) T = 14;

    /* passing : un adversaire est au filet au moment de la frappe */
    hit_pass = (u8)(iabs(PX(ply[t ^ 1]) - NET_Y) < 40 || (npl == 4 && iabs(PX(ply[(t ^ 1) + 2]) - NET_Y) < 40));
    hitter = t;
    hitter_p = p;
    hit_now = 1;
    b_serve = 0;
    ++rally_hits;
    pl_hit[p] = 1;
    pl_flip[p] = (u8)(dx < 0);
    launch(tx, ty, T, grav);
    if (hit_perf) sfx_perfect(); else sfx_hit((u8)(power >> 3));
    /* IA adaptative : on note les habitudes des joueurs humains */
    if (cfg.ctrl[p] != CTRL_CPU && ad_n[t] < 250) {
        ++ad_n[t];
        if (tx < CENTER_X - 18) ++ad_l[t];
        else if (tx > CENTER_X + 18) ++ad_r[t];
        if (iabs(PX(ply[p]) - NET_Y) < 50) ++ad_net[t];
    }
    /* les adversaires IA réagissent après un délai */
    ai_delay[t ^ 1] = ai_react[cfg.level[t ^ 1]];
    /* partenaire : 2 trames de plus, pour que les deux IA ne calculent pas
       leur trajectoire dans la même trame */
    if (npl == 4) ai_delay[(t ^ 1) + 2] = (u8)(ai_react[cfg.level[(t ^ 1) + 2]] + 2);
    pcache_y[0] = pcache_y[1] = pcache_y[2] = pcache_y[3] = 0x7FFF;
    ai_gy[0] = ai_gy[1] = ai_gy[2] = ai_gy[3] = 0x7FFF;
}

/* ====================================================================== */
/* Service                                                                 */
/* ====================================================================== */
static s16 base_y(u8 side, s8 off) { return (side == 0 ? BOT_BASE + off : TOP_BASE - off) * FP; }
static s16 net_y(u8 side) { return (side == 0 ? NET_Y + 34 : NET_Y - 30) * FP; }

static void place_for_serve(void)
{
    u8 srv = sc_server, rcv = (u8)(srv ^ 1);
    u8 sp = server_p(), rp = receiver_p();
    u8 ss = side_of(srv), rs = side_of(rcv);
    u8 ad = rules_ad_side();
    u8 left = (u8)((ss == 0) == (ad == 0));     /* carré visé à gauche ? */
    u8 i;
    plx[sp] = (left ? CENTER_X + 22 : CENTER_X - 22) * FP;
    /* serveur juste derrière la ligne de fond ; en haut, le sprite se dessine
       au-dessus des pieds : 2 px de recul suffisent (8 px en bas) */
    ply[sp] = ss ? (TOP_BASE - 2) * FP : base_y(0, 8);
    plx[rp] = (left ? CENTER_X - 46 : CENTER_X + 46) * FP;
    ply[rp] = base_y(rs, 2);
    if (npl == 4) {
        /* partenaire du serveur au filet, partenaire du receveur à la ligne de service */
        u8 sm = (u8)(sp ^ 2), rm = (u8)(rp ^ 2);
        plx[sm] = (left ? CENTER_X - 34 : CENTER_X + 34) * FP;
        ply[sm] = net_y(ss);
        plx[rm] = (left ? CENTER_X + 40 : CENTER_X - 40) * FP;
        ply[rm] = (rs == 0 ? BOT_SERV + 4 : TOP_SERV - 4) * FP;
    }
    for (i = 0; i < 4; ++i) {
        pl_state[i] = ST_IDLE;
        pl_charge[i] = 0;
        ai_net[i] = 0;
        pl_jump[i] = 0;
    }
    if (npl == 4) { ai_net[sp ^ 2] = 1; }
    /* style « filet » : montée systématique derrière le service */
    for (i = 0; i < npl; ++i)
        if (cfg.ctrl[i] == CTRL_CPU && i == sp && rand8() < sty_net[char_style[cfg.chr[i]]] && sty_net[char_style[cfg.chr[i]]] > 150)
            ai_net[i] = 1;
    if (cfg.mode == GM_TRAIN) {
        /* le lanceur de balles est invisible, en haut ; volée : joueur au filet */
        plx[1] = CENTER_X * FP; ply[1] = (TOP_BASE - 4) * FP;
        if (feed_px_set) { plx[0] = feed_px; ply[0] = feed_py; }
    }
    b_vis = 0;
    b_live = 0;
    b_netcord = 0;
    rally_hits = 0;
    ms = MS_SERVE;
    ms_timer = (u8)(40 + (rand8() & 31));
    if (cfg.mode == GM_TRAIN) ms_timer = 45;
    hud_dirty = 1;
}

static void toss(u8 p)
{
    bx = plx[p] + 5 * FP;
    by = ply[p];
    bz = 18 * FP;
    bvx = bvy = 0;
    bvz = 172;          /* lancer plus haut : sommet ~56 px */
    bgrav = 6;
    b_vis = 1;
    b_live = 0;
    pl_state[p] = ST_TOSS;
    ms = MS_TOSS;
    sfx_toss();
}

/* frappe de service ; quality : 0 parfait .. grand = mauvais */
static void serve_hit(u8 p, u8 pad, u8 btn, u8 quality, u8 spread)
{
    u8 t = (u8)(p & 1);
    u8 ss = side_of(t);
    u8 ad = rules_ad_side();
    u8 left = (u8)((ss == 0) == (ad == 0));
    s16 tx, ty, dist;
    u8 speed, grav, T;
    s8 dir = ss ? 1 : -1;

    srv_canon = 0;
    if (btn == PAD_B) { speed = 44; grav = 7; bspin = SPIN_SLICE; }
    else {
        speed = 64; grav = 9; bspin = SPIN_FLAT; spread += 4;
        if (cfg.ctrl[p] != CTRL_CPU) ++st_ctry[t];
        /* SERVICE CANON : frappe A pile au sommet du lancer (quality 0 : les
           2 trames où |vz| < 8). Le CPU Expert le réussit une fois sur deux
           quand il frappe au sommet ; Facile et Normal jamais */
        if (!quality && (cfg.ctrl[p] != CTRL_CPU || (cfg.level[p] == 2 && (rand8() & 1)))) {
            srv_canon = 1; speed = 96; grav = 12;     /* +50 % */
            spread = (u8)(faults ? 6 : 1);    /* 2e balle : un peu plus risqué */
            ++st_canon[t];
        }
    }
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
    hitter_p = p;
    b_serve = 1;
    rally_hits = 1;
    if (faults == 0) ++st_first_tot[t];
    launch(tx, ty, T, grav);
    ms = MS_RALLY;
    pl_state[p] = ST_SWING;
    pl_timer[p] = 12;
    if (srv_canon) { sfx_canon(); set_msg("SERVICE CANON", "", 50); }
    else sfx_hit(btn == PAD_B ? 1 : 3);
    ai_delay[t ^ 1] = ai_react[cfg.level[t ^ 1]];
    if (npl == 4) ai_delay[(t ^ 1) + 2] = ai_react[cfg.level[(t ^ 1) + 2]];
    pcache_y[0] = pcache_y[1] = pcache_y[2] = pcache_y[3] = 0x7FFF;
    ai_gy[0] = ai_gy[1] = ai_gy[2] = ai_gy[3] = 0x7FFF;
}

/* ====================================================================== */
/* Déplacements                                                            */
/* ====================================================================== */
static void move_toward(u8 p, s16 tx, s16 ty, u8 speed)
{
    s16 dx = tx - plx[p], dy = ty - ply[p];
    s16 sx = speed, sy = speed;
    pl_moving[p] = 0;
    if (iabs(dx) > 2 * FP && iabs(dy) > 2 * FP) { sx = (s16)(((u16)speed + ((u16)speed << 1)) >> 2);   /* x 3/4 sans multiplication */ sy = sx; }
    if (dx > sx) { plx[p] += sx; pl_moving[p] = 1; }
    else if (dx < -sx) { plx[p] -= sx; pl_moving[p] = 1; }
    else plx[p] = tx;
    if (dy > sy) { ply[p] += sy; pl_moving[p] = 1; }
    else if (dy < -sy) { ply[p] -= sy; pl_moving[p] = 1; }
    else ply[p] = ty;
}

static void clamp_player(u8 p)
{
    s16 ymin, ymax;
    if (side_of(p) == 0) { ymin = (NET_Y + 10) * FP; ymax = 228 * FP; }
    else { ymin = 30 * FP; ymax = (NET_Y - 6) * FP; }
    if (ms == MS_SERVE && p == server_p()) {
        /* le serveur reste derrière sa ligne de fond */
        if (side_of(p) == 0) ymin = (BOT_BASE + 6) * FP; else ymax = (TOP_BASE - 1) * FP;
    }
    /* x limité à 48..208 : les sprites n'empiètent pas sur les panneaux de score */
    if (plx[p] < 48 * FP) plx[p] = 48 * FP;
    if (plx[p] > 208 * FP) plx[p] = 208 * FP;
    if (ply[p] < ymin) ply[p] = ymin;
    if (ply[p] > ymax) ply[p] = ymax;
}

/* ====================================================================== */
/* Contrôle humain                                                         */
/* ====================================================================== */
static void human_update(u8 p, u8 pad, u8 pressed)
{
    s16 sp = HUMAN_SPEED - (s16)((tired << 1) + tired);   /* fatigue : jusqu'à -21 % */
    s16 nx = plx[p], ny = ply[p];
    u8 st = pl_state[p];

    if (ms == MS_SERVE && p == server_p()) {
        if (pressed & (PAD_A | PAD_B)) { toss(p); return; }
        if (pad & PAD_LEFT) plx[p] -= 64;
        if (pad & PAD_RIGHT) plx[p] += 64;
        {
            /* rester du bon côté du centre */
            u8 left = (u8)((side_of(p) == 0) == (rules_ad_side() == 0));
            if (left && plx[p] < (CENTER_X + 4) * FP) plx[p] = (CENTER_X + 4) * FP;
            if (left && plx[p] > (SGL_R + 10) * FP) plx[p] = (SGL_R + 10) * FP;
            if (!left && plx[p] > (CENTER_X - 4) * FP) plx[p] = (CENTER_X - 4) * FP;
            if (!left && plx[p] < (SGL_L - 10) * FP) plx[p] = (SGL_L - 10) * FP;
        }
        pl_moving[p] = (u8)((pad & (PAD_LEFT | PAD_RIGHT)) != 0);
        return;
    }
    if (ms == MS_TOSS && p == server_p()) {
        if (pressed & (PAD_A | PAD_B)) {
            s16 q = iabs(bvz) >> 3;          /* 0 au sommet du lancer */
            if (cfg.mode == GM_TRAIN && (pressed & PAD_A)) serve_fb(bvz);   /* entraînement : timing */
            serve_hit(p, pad, (u8)(pressed & PAD_A ? PAD_A : PAD_B), (u8)(q > 20 ? 20 : q), 3);
        }
        return;
    }
    if (ms == MS_SERVE || ms == MS_TOSS) {
        /* avant le service : petits ajustements latéraux */
        if (pad & PAD_LEFT) plx[p] -= 64;
        if (pad & PAD_RIGHT) plx[p] += 64;
        clamp_player(p);
        return;
    }

    /* frappe : appui = armer (la puissance monte), relâcher = frapper */
    if (st == ST_IDLE && (pressed & (PAD_A | PAD_B))) {
        pl_state[p] = st = ST_CHARGE;
        pl_btn[p] = (u8)(pressed & PAD_A ? PAD_A : PAD_B);
        pl_charge[p] = 0;
        pl_hit[p] = 0;
    }
    if (st == ST_CHARGE) {
        if (pad & pl_btn[p]) { if (pl_charge[p] < 32) ++pl_charge[p]; }
        else { pl_state[p] = st = ST_SWING; pl_timer[p] = 14; sfx_swing(); }
        sp = HUMAN_SPEED / 2;
    }
    if ((st == ST_CHARGE || (st == ST_SWING && pl_timer[p] > 4)) && !pl_hit[p] && can_hit(p)) {
        /* frappe partie dans les 3 premières trames du geste : bouton relâché
           juste avant l'arrivée de la balle (condition de la frappe parfaite) */
        swing_early = (u8)(st == ST_SWING ? pl_timer[p] : 0);   /* 0 : encore armé */
        do_hit(p, rel_pad(p, pad), pl_btn[p], pl_charge[p], 3);   /* HAUT/BAS relatifs au joueur */
        pl_state[p] = ST_SWING;
        pl_timer[p] = 10;
    }
    if (st == ST_SWING) {
        sp = HUMAN_SPEED / 3;
        if (--pl_timer[p] == 0) pl_state[p] = ST_IDLE;
    }
    if (ms == MS_DEAD || ms == MS_OVER) sp = HUMAN_SPEED / 2;

    if (pad & PAD_LEFT) nx -= sp;
    if (pad & PAD_RIGHT) nx += sp;
    if (pad & PAD_UP) ny -= sp;
    if (pad & PAD_DOWN) ny += sp;
    pl_moving[p] = (u8)(nx != plx[p] || ny != ply[p]);
    plx[p] = nx; ply[p] = ny;
    clamp_player(p);
}

/* ====================================================================== */
/* Intelligence artificielle                                               */
/* ====================================================================== */
/* Point de frappe prévu pour le joueur p (pixels). Recalculé une trame
 * sur quatre par joueur (calcul en 32 bits coûteux sur 6502). */
static s16 pred_x, pred_y;
static void predict(u8 p)
{
    if (((u8)(frame_cnt + p) & 3) != 0 && ai_delay[p] == 0 && pcache_y[p] != 0x7FFF) {
        pred_x = pcache_x[p]; pred_y = pcache_y[p];
        return;
    }
    /* erreur tirée une fois par frappe adverse */
    if (pcache_y[p] == 0x7FFF) ai_err[p] = rnd_spread(ai_guess[cfg.level[p]]);
    if (bounces == 0) {
        /* x 12 = x 8 + x 4 : décalages au lieu de multiplications */
        pred_x = land_x + PX((bvx << 3) + (bvx << 2));
        pred_y = land_y + PX((bvy << 3) + (bvy << 2));
        /* joueur au filet : interception de volée */
        if (iabs(PX(ply[p]) - NET_Y) < 46 && !b_serve) {
            pred_y = PX(ply[p]);
            if ((bvy >> 2) != 0) pred_x = PX(bx) + ((pred_y - PX(by)) * (bvx >> 2)) / (bvy >> 2);
        }
        /* CPU expert contre un lob pas trop profond : il recule et se place
           ~11 trames avant le point de chute, là où la balle passe vers 50 px
           de haut, pour la smasher en sautant */
        if (last_shot == SH_LOB && cfg.level[p] == 2 && (ai_err[p] & 3) && iabs(land_y - NET_Y) < 76) {
            pred_x = land_x - PX(bvx * 11);
            pred_y = land_y - PX(bvy * 11);
        }
    } else {
        pred_x = PX(bx + (bvx << 2) + (bvx << 1));
        pred_y = PX(by + (bvy << 2) + (bvy << 1));
    }
    /* l'IA n'anticipe pas parfaitement : une erreur tirée à chaque frappe
       adverse peut lui faire rater une balle bien croisée */
    pred_x += ai_err[p];
    pcache_x[p] = pred_x; pcache_y[p] = pred_y;
}

/* En double, un seul partenaire joue la balle : celui dont la moitié
 * (gauche/droite) contient le point de frappe, ou le plus proche si les
 * deux sont du même côté. Évite que les deux partenaires se gênent. */
static u8 responsible(u8 p)
{
    u8 m = (u8)(p ^ 2);
    s16 dp, dm;
    if (npl != 4) return 1;
    if (ms == MS_RALLY && b_serve) return (u8)(p == receiver_p());
    dp = iabs(PX(plx[p]) - pred_x) + iabs(PX(ply[p]) - pred_y);
    dm = iabs(PX(plx[m]) - pred_x) + iabs(PX(ply[m]) - pred_y);
    if (dp == dm) return (u8)(p < m);
    return (u8)(dp < dm);
}

static void ai_update(u8 p)
{
    u8 t = (u8)(p & 1);
    u8 lv = cfg.level[p];
    u8 side = side_of(t);
    s16 tx, ty;
    u8 pad = 0, btn = PAD_A;

    if (ms == MS_SERVE) {
        if (p == server_p()) {
            if (cfg.mode == GM_TRAIN) {
                if (tr_done || --ms_timer != 0) { pl_moving[p] = 0; return; }
                /* lanceur de balles : l'exercice choisit la balle, le moteur la lance */
                bank_call(BANK_EXTRAS, train_feed);
                bx = feed_x; by = feed_y; bz = feed_z;
                b_vis = 1; hitter = t; hitter_p = p; b_serve = 0; rally_hits = 0;
                hud_dirty = 1;
                bspin = feed_spin;
                launch(feed_tx, feed_ty, feed_T, feed_g);
                ms = MS_RALLY;
                sfx_hit(1);
                return;
            }
            if (--ms_timer == 0) toss(p);
        }
        pl_moving[p] = 0;
        return;
    }
    if (ms == MS_TOSS) {
        if (p == server_p() && bvz <= 0) {
            u8 q = (u8)(rand8() % (u8)(10 - lv * 3));
            if (faults == 0) serve_hit(p, (u8)(rand8() & 1 ? PAD_LEFT : PAD_RIGHT), PAD_A, q, (u8)(ai_spread[lv] >> 2));
            else serve_hit(p, 0, PAD_B, (u8)(q >> 1), (u8)(ai_spread[lv] >> 3));
        }
        return;
    }
    if (cfg.mode == GM_TRAIN) {
        /* adversaire immobile : renvoie au centre si la balle arrive sur lui */
        pl_moving[p] = 0;
        if (cfg.ex == EX_STATIC && ms == MS_RALLY && hitter != t && can_hit(p) && bounces == 1) {
            hit_margin = 30;
            do_hit(p, 0, PAD_A, 4, 10);
            pl_state[p] = ST_SWING;
            pl_timer[p] = 12;
        }
        if (pl_state[p] == ST_SWING && --pl_timer[p] == 0) pl_state[p] = ST_IDLE;
        return;
    }

    if (ms == MS_RALLY && b_live && hitter != t) {
        if (ai_delay[p]) {
            --ai_delay[p];
            if (ai_x2 && ai_delay[p]) --ai_delay[p];
            pl_moving[p] = 0;
            return;
        }
        if (can_hit(p) && (bounces == 1 || bz > 14 * FP || iabs(PX(ply[p]) - NET_Y) < 50)) {
            u8 sty = char_style[cfg.chr[p]];
            u8 spread = (u8)(ai_spread[lv] + sty_spread[sty]);
            u8 o = (u8)(t ^ 1);
            s16 ox = PX(plx[o]), oy = PX(ply[o]);
            u8 lobp = sty_lob[sty];
            /* IA adaptative : contre un adversaire qui monte souvent au filet, plus de lobs */
            if (lv >= 1 && ad_n[o] >= 6 && ad_net[o] * 3 > ad_n[o]) lobp = (u8)(lobp > 175 ? 255 : lobp + 80);
            if (npl == 4) {
                /* viser l'adversaire du fond ou le trou entre les deux */
                ox = (PX(plx[o]) + PX(plx[o + 2])) >> 1;
                if (iabs(PX(ply[o + 2]) - NET_Y) > iabs(oy - NET_Y)) oy = PX(ply[o + 2]);
            }
            if (ox > CENTER_X + 10) pad = PAD_LEFT;
            else if (ox < CENTER_X - 10) pad = PAD_RIGHT;
            else pad = (u8)(rand8() & 1 ? PAD_LEFT : PAD_RIGHT);
            /* choix tactiques selon le niveau et le style du personnage */
            if (lv >= 1 && iabs(oy - NET_Y) < 50 && rand8() < lobp) {
                btn = PAD_B; pad |= PAD_UP;                     /* lob sur le volleyeur */
            } else if (lv >= 1 && iabs(oy - NET_Y) > 90 && rand8() < (u8)(sty_drop[sty] >> (2 - lv))) {
                btn = PAD_B; pad |= PAD_DOWN;                   /* amortie */
            } else if (rand8() < sty_slice[sty]) {
                btn = PAD_B;                                    /* coupé de variation */
            } else if (sty_pow[sty] > 4 && (rand8() & 3) == 0) {
                pad |= PAD_UP;                                  /* frappe à plat */
            }
            if ((rand8() & ai_fault[lv]) == 0) spread += 20;   /* faute directe */
            hit_margin = (u8)(ai_margin[lv] + sty_margin[sty]);
            do_hit(p, pad, btn, (u8)(ai_power[lv] + sty_pow[sty]), spread);
            pl_state[p] = ST_SWING;
            pl_timer[p] = 12;
            /* montée au filet après une frappe à l'intérieur du court */
            if (npl == 2 && lv >= 1 && iabs(PX(ply[p]) - NET_Y) < 66 && rand8() < (u8)(sty_net[sty] >> (2 - lv))) ai_net[p] = 1;
            return;   /* trame de frappe : pas de calcul de déplacement */
        }
        /* en simple, la décision (coûteuse) est prise une trame sur deux ;
           le déplacement vers la dernière destination a lieu à chaque trame */
        /* double (ai_x2) : les deux IA d'une équipe sont mises à jour la même
           trame ; elles décident en alternance (une décision par trame) */
        if (((u8)((ai_x2 ? (frame_cnt >> 1) ^ (p >> 1) : frame_cnt ^ p)) & 1) == 0 || ai_gy[p] == 0x7FFF) {
            predict(p);
            if (responsible(p)) {
                tx = pred_x; ty = pred_y;
                /* se placer pour avoir la balle sur le côté */
                tx += (PX(plx[p]) < tx) ? -10 : 10;
            } else {
                /* partenaire : couvre sa moitié du court */
                tx = (PX(plx[p ^ 2]) < CENTER_X) ? CENTER_X + 34 : CENTER_X - 34;
                ty = PX(ply[p]);
            }
            if (side == 0 && ty < NET_Y + 12) ty = NET_Y + 12;
            if (side == 1 && ty > NET_Y - 8) ty = NET_Y - 8;
            ai_gx[p] = tx * FP; ai_gy[p] = ty * FP;
        }
        move_toward(p, ai_gx[p], ai_gy[p], (u8)((ai_speed[lv] - (tired << 1)) << ai_x2));
        clamp_player(p);

    } else {
        /* replacement : centre (ou sa moitié en double), au fond ou au filet */
        u8 o = (u8)(t ^ 1);
        if (npl == 4) tx = (PX(plx[p]) < PX(plx[p ^ 2])) ? CENTER_X - 34 : CENTER_X + 34;
        else tx = CENTER_X + ((PX(bx) - CENTER_X) >> 2);
        /* IA adaptative : se décaler vers le côté où l'humain joue le plus (±16 px) */
        if (lv >= 1 && ad_n[o] >= 6) {
            s16 bias = (s16)(((s16)ad_r[o] - (s16)ad_l[o]) * 24) / (s16)ad_n[o];
            if (bias > 16) bias = 16;
            if (bias < -16) bias = -16;
            tx += bias;
        }
        if (ai_net[p]) ty = PX(net_y(side));
        else ty = PX(base_y(side, (s8)sty_deep[char_style[cfg.chr[p]]]));
        if (ms == MS_DEAD) ty = PX(ply[p]);
        move_toward(p, tx * FP, ty * FP, (u8)((ai_speed[lv] - 20) << ai_x2));
        clamp_player(p);
    }
    if (pl_state[p] == ST_SWING) {
        if (ai_x2 && pl_timer[p] > 1) --pl_timer[p];
        if (--pl_timer[p] == 0) pl_state[p] = ST_IDLE;
    }
}

/* ====================================================================== */
/* Score : deux panneaux dans les coins supérieurs, au-dessus des tribunes */
/* (les joueurs n'y vont jamais) ; messages au centre du bandeau.          */
/* ====================================================================== */
/* nom d'équipe sur 4 lettres : banque fixe (gamefix.c) */
void team_name(u8 t, char *out);

/* ====================================================================== */
void match_start(void)
{
    u8 i;
    npl = (u8)(cfg.doubles ? 4 : 2);
    rules_init(cfg.sets_to_win, cfg.games_per_set, (u8)(cfg.mode == GM_MATCH ? (rand8() & 1) : 1));
    for (i = 0; i < 2; ++i) {
        st_points[i] = st_aces[i] = st_dfaults[i] = st_winners[i] = st_errors[i] = 0;
        st_canon[i] = st_ctry[i] = st_perf[i] = st_ptry[i] = 0;
        st_first_in[i] = st_first_tot[i] = 0;
        srv_slot[i] = 0;
        ad_l[i] = ad_r[i] = ad_net[i] = ad_n[i] = 0;
    }
    for (i = 0; i < 4; ++i) { pl_flip[i] = 0; plx[i] = 128 * FP; ply[i] = 0; pose_cache[i] = 0; }
    st_longest = 0;
    tr_score = tr_total = tr_best = tr_streak = 0;
    tr_done = tr_wait = 0;
    tr_human_serves = 0;
    feed_px_set = 0;
    tgt_x = 0;
    faults = 0;
    paused = 0;
    pause_cur = 0;
    match_quit = 0;
    msg_timer = 0;
    crowd_timer = 0;
    rp_pending = 0;
    recap_pend = brk_pend = 0;
    if (cfg.mode == GM_TRAIN) {
        bank_call(BANK_EXTRAS, train_start);
        sc_server = (u8)(tr_human_serves ? 0 : 1);
    }
    place_for_serve();
    if (cfg.mode == GM_MATCH) set_msg("JEU !", "", 60);
}

/* Écran du court et palettes */
static void court_load(void)
{
    ppu_off();
    vbuf_reset();
    vram_adr(0x2000);
    bank_call(BANK_EXTRAS, load_court);
    pal_set_court(cfg.surface, cfg.cond, cfg.env);
    pal_set_player(0, cfg.chr[0]);
    pal_set_player(1, cfg.chr[1]);
}

/* Reprise après l'écran de fin de set : le score des sets dans le bandeau */
void match_resume(void)
{
    music_play(MUS_NONE);              /* jamais de musique pendant le match */
    court_load();
    score_recap();
    match_draw();
    ppu_on();
}

/* Début du match (appelé par la boucle principale) */
void match_begin(void)
{
    court_load();
    music_play(MUS_NONE);
    match_start();
    match_draw();
    ppu_on();
    sfx_whistle();
}

static void after_point(void)
{
    u8 ev, w = pending_winner, prev;
    if (end_reason == END_LET) { place_for_serve(); return; }
    if (end_reason == END_FAULT) {
        if (cfg.mode == GM_MATCH) faults = 1;
        if (cfg.mode == GM_TRAIN && tr_human_serves) sc_points[0] ^= 1;   /* alterne les côtés */
        place_for_serve();
        return;
    }
    faults = 0;
    if (cfg.mode != GM_MATCH) { place_for_serve(); return; }
    prev = sc_server;
    ev = rules_point(w);
    /* double : quand une équipe perd le service, son autre joueur servira */
    if (sc_server != prev) srv_slot[prev] ^= 1;
    if (ev & EV_MATCH) {
        ms = MS_OVER;
        ms_timer = 160;
        set_msg("JEU, SET", "ET MATCH !", 160);
        sfx_whistle();
        music_play(cfg.ctrl[w] == CTRL_CPU && cfg.ctrl[w ^ 1] != CTRL_CPU ? MUS_DEFEAT : MUS_VICTORY);
        sfx_crowd(7);
        crowd_timer = 120;
        return;
    }
    {
        static char nm[5];
        team_name(w, nm); nm[4] = 0;
        /* récapitulatif : écran de fin de set (sauf démonstration), score des
           sets dans le bandeau après chaque jeu */
        if (ev & EV_SET) { set_msg("JEU ET SET", nm, 90); music_play(MUS_GAME); crowd_timer = 64; if (!cfg.demo) brk_pend = 1; }
        else if (ev & EV_TIEBREAK) { set_msg("6 PARTOUT", "JEU DECISIF", 90); music_play(MUS_GAME); recap_pend = 1; }
        else if (ev & EV_GAME) { set_msg("JEU", nm, 70); music_play(MUS_GAME); recap_pend = 1; }
        else {
            if (rules_is_match_point(0) || rules_is_match_point(1)) set_msg("BALLE DE MATCH", "", 70);
            else if (rules_is_set_point(0) || rules_is_set_point(1)) set_msg("BALLE DE SET", "", 70);
            else if (!sc_tiebreak && sc_points[0] >= 3 && sc_points[0] == sc_points[1]) set_msg("EGALITE", "", 60);
            else if (!sc_tiebreak && sc_points[0] >= 3 && sc_points[1] >= 3) {
                team_name((u8)(sc_points[0] > sc_points[1] ? 0 : 1), nm);
                set_msg("AVANTAGE", nm, 60);
            }
            music_play(MUS_POINT);
        }
    }
    if (ev & EV_CHANGE) set_msg("CHANGEMENT", "DE COTE", 90);
    place_for_serve();
}

u8 match_tick(void)
{
    u8 p, ctrl, pad, pressed;

    /* démonstration : un bouton rend la main */
    if (cfg.demo && (pad1_new & (PAD_A | PAD_START))) { match_quit = 2; return 1; }

    /* menu de pause (START) : CONTINUER / QUITTER ; SELECT = quitter directement */
    if ((pad1_new & PAD_START) && ms != MS_OVER && ms != MS_REPLAY && !paused) {
        paused = 1; pause_cur = 0; hud_dirty = 1; pad1_new = 0;
    }
    if (paused) {
        if (pad1_new & (PAD_UP | PAD_DOWN)) { pause_cur ^= 1; hud_dirty = 1; sfx_menu_move(); }
        if ((pad1_new & PAD_SELECT) || ((pad1_new & PAD_A) && pause_cur)) { match_quit = 1; return 1; }
        if (pad1_new & (PAD_START | PAD_B) || ((pad1_new & PAD_A) && !pause_cur)) { paused = 0; hud_dirty = 1; }
        if (hud_dirty) bank_call(BANK_EXTRAS, hud_update);
        return 0;
    }

    /* ralenti : la banque des extras rejoue les dernières trames enregistrées */
    if (ms == MS_REPLAY) {
        bank_call(BANK_EXTRAS, rp_play);
        if (rp_done) { ms = MS_DEAD; ms_timer = 1; msg_timer = 1; rp_pending = 0; }
        if (msg_timer && --msg_timer == 0) hud_dirty = 1;
        if (hud_dirty) bank_call(BANK_EXTRAS, hud_update);
        return 0;
    }

    hit_now = 0;
    /* fatigue (option) : au-delà de 10 frappes dans l'échange, tous les
       joueurs ralentissent peu à peu (niveau 0..10), puis récupèrent au point suivant */
    tired = 0;
    if (cfg.fatigue && rally_hits > 10) tired = (u8)(rally_hits > 20 ? 10 : rally_hits - 10);
    for (p = 0; p < npl; ++p) {
        ctrl = cfg.ctrl[p];
        if (ctrl == CTRL_CPU) {
            /* double : chaque IA est mise à jour une trame sur deux (vitesse
               doublée), pour tenir dans le budget CPU d'une trame (aussi
               pendant la fête de fin de match) */
            if (npl == 4 && (u8)(ms - MS_RALLY) < 3) {   /* échange, point joué, fin du match */
                /* trame de frappe (calcul de trajectoire) : les IA restantes
                   attendent la trame suivante */
                if (hit_now) continue;
                if (((u8)(frame_cnt ^ p) & 1) == 0) { ai_x2 = 1; ai_update(p); ai_x2 = 0; }
            } else ai_update(p);
        }
        else {
            pad = ctrl == CTRL_PAD1 ? pad1 : pad2;
            pressed = ctrl == CTRL_PAD1 ? pad1_new : pad2_new;
            hit_margin = 8;
            human_update(p, pad, pressed);
        }
        if (pl_moving[p]) ++pl_walk[p];
    }

    if (ms == MS_TOSS) {
        bvz -= bgrav;
        bz += bvz;
        if (bz < 14 * FP && bvz < 0) {
            /* lancer raté : on recommence sans pénalité */
            b_vis = 0;
            ms = MS_SERVE;
            ms_timer = 30;
            pl_state[server_p()] = ST_IDLE;
        }
    } else {
        ball_update();
    }

    if (ms == MS_DEAD) {
        if (rp_pending && ms_timer == 50) {
            bank_call(BANK_EXTRAS, rp_start);
            ms = MS_REPLAY;
            set_msg("RALENTI", "A : PASSER", 255);
        } else if (--ms_timer == 0) {
            after_point();
            if (brk_pend) { brk_pend = 0; return 2; }      /* fin de set : écran récapitulatif */
        }
    } else if (ms == MS_OVER) {
        if (--ms_timer == 0) return 1;
        if (win_party()) fw_sound(ms_timer);
    }

    /* enregistrement pour les ralentis (match uniquement) */
    if (cfg.replays && cfg.mode == GM_MATCH && !cfg.demo && (ms == MS_RALLY || ms == MS_DEAD))
        rp_record();

    /* fin de l'exercice d'entraînement */
    if (cfg.mode == GM_TRAIN && tr_done && (ms == MS_SERVE || ms == MS_RALLY) && ++tr_wait > 90) return 1;

    if (cfg.cond == COND_RAIN) bank_call(BANK_EXTRAS, rain_tick);

    if (msg_timer) { if (--msg_timer == 0) { hud_dirty = 1; if (recap_pend) score_recap(); } }
    if (hud_dirty) bank_call(BANK_EXTRAS, hud_update);
    /* public animé (après le score : le tampon VRAM est partagé) */
    bank_call(BANK_EXTRAS, crowd_tick);      /* public : suit la balle, ola, réactions */
    /* filet qui tremble (après le score : le tampon VRAM est partagé) */
    if (net_shake) bank_call(BANK_EXTRAS, net_tick);
    return 0;
}

/* fête de victoire : quand un humain gagne (ou en démonstration), comme
   pour la musique de victoire */
static const u8 hop_h[16] = { 0, 1, 3, 5, 6, 7, 7, 7, 8, 7, 7, 7, 6, 5, 3, 1 };
static u8 win_party(void)
{
    return (u8)(ms == MS_OVER && !(cfg.ctrl[sc_winner] == CTRL_CPU && cfg.ctrl[sc_winner ^ 1] != CTRL_CPU));
}

/* Commandes des coups : HAUT et BAS par rapport au joueur. « Vers
   l'adversaire » = coup long (à plat, lob), « vers soi » = coup court
   (croisé court, amortie) : en haut du court, HAUT et BAS sont inversés.
   GAUCHE et DROITE restent ceux de l'écran. */
static u8 rel_pad(u8 p, u8 pad)
{
    u8 v = (u8)(pad & (PAD_UP | PAD_DOWN));
    if (((p & 1) ^ sc_swap) && (v == PAD_UP || v == PAD_DOWN)) pad ^= PAD_UP | PAD_DOWN;
    return pad;
}



void match_draw(void)
{
    u8 ord[4], ys[4], i, j, k, p, party;   /* j sert aussi au saut */
    s16 sy;
    /* trame de frappe (calcul de trajectoire coûteux) : on garde les sprites
       de la trame précédente, ce qui évite tout ralentissement ; pendant un
       ralenti, c'est rp_play qui dessine */
    if (hit_now || ms == MS_REPLAY) return;
    party = win_party();
    /* tri par profondeur : le joueur le plus bas à l'écran passe devant */
    /* clés sur 8 bits (y en pixels, toujours positif) : comparaisons rapides */
    for (i = 0; i < npl; ++i) { ord[i] = i; ys[i] = (u8)(((u16)ply[i] << 2) >> 8); }
    for (i = 1; i < npl; ++i) {
        k = ord[i];
        for (j = i; j > 0 && ys[ord[j - 1]] < ys[k]; --j) ord[j] = ord[j - 1];
        ord[j] = k;
    }
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
    if (tgt_x) spr1(tgt_x, tgt_y, 3, 3);   /* cible d'entraînement */
    bank_call(BANK_EXTRAS, poses_all);     /* poses et miroirs, balle du dribble, glissades */
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
        j = (u8)((j * j) >> 4);
        /* victoire : le vainqueur sautille de joie (un bond toutes les 16 trames) */
        if (party && (u8)(p & 1) == sc_winner) j = hop_h[ms_timer & 15];
        spr_person((u8)(((u16)plx[p] << 2) >> 8), (s16)(ys[p] - j), cfg.chr[p], side_of(p), k, flip_cache[p], (u8)(p & 1));
    }
    bank_call(BANK_EXTRAS, match_fx);   /* ombres, poussière, arbitre, repère */
    if (party) { win_fx(); fw_draw(ms_timer, 0); }
    if (cfg.cond == COND_RAIN) bank_call(BANK_EXTRAS, rain_draw);
    spr_end();
}
