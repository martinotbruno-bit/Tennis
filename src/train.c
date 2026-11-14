/* Royal Martinot — entraînement (CDC 9) : déplacé de extras.c (banque 3,
 * pleine) en banque 20, version 2.74. */
#include <string.h>
#include "game_int.h"
#include "app.h"
#include "rules.h"
#include "court.h"
#include "gfx.h"
#include "audio.h"
#include "assets.h"
#define side_of(t) ((u8)(((t) & 1) ^ sc_swap))
#pragma static-locals(off)          /* (variables locales sur la pile, idem) */

/* Entraînement (CDC 9) */
#pragma bss-name(push, "SAVEDATA")   /* (zone libre après les blocs sauvegardés, comme fly_t : WBSS est pleine ; train_start les initialise) */
static u8 tr_count, tr_alt, tr_got;
#pragma bss-name(pop)

static void tmsg(const char *a, const char *b, u8 t)
{
    center14(msg1, a);
    center14(msg2, b);
    msg_timer = t;
    hud_dirty = 1;
}

static s8 rnd(u8 spread) { return (s8)((s16)(rand8() % (u8)(spread * 2 + 1)) - spread); }

/* tutoriel : consignes et nombre de réussites demandées par étape */
static const char * const tut_l1[6] = {
    "FRAPPEZ AVEC A", "AMORTIE :", "LOB :", "VISEZ :", "SERVICE : A", "BRAVO !" };
static const char * const tut_l2[6] = {
    "DANS LE COURT", "B + BAS", "B + HAUT", "CROIX + A", "LANCER+FRAPPER", "TUTORIEL FINI" };
static const u8 tut_need[5] = { 3, 2, 2, 2, 2 };

static void tut_show(void)
{
    tmsg(tut_l1[tr_step], tut_l2[tr_step], 250);
}

void train_start(void)
{
    tr_count = cfg.ex == EX_CUSTOM ? cfg.ex_count : 20;
    tr_alt = 0;
    tr_step = 0;
    tr_got = 0;
    tr_human_serves = (u8)(cfg.ex == EX_SERVE);
    if (cfg.ex == EX_VOLLEY) {
        feed_px_set = 1;
        feed_px = CENTER_X * FP;
        feed_py = (NET_Y + 30) * FP;
    }
    switch (cfg.ex) {
    case EX_TARGETS:   tmsg("VISEZ L'ETOILE", "ZONE : +2 PTS", 120); break;
    case EX_MOVE:      tmsg("DEPLACEMENTS", "COIN A COIN", 120); break;
    case EX_STREAK:    tmsg("REGULARITE :", "SANS FAUTE !", 120); break;
    case EX_PRECISION: tmsg("PRECISION :", "SEULE LA ZONE", 120); break;
    case EX_VOLLEY:    tmsg("VOLEE : JOUEZ", "AVANT REBOND", 120); break;
    case EX_SERVE:     tmsg("SERVICE :", "VISEZ L'ETOILE", 120); break;
    case EX_STATIC:    tmsg("ECHANGEZ AVEC", "LE JOUEUR FIXE", 120); break;
    case EX_CUSTOM:    tmsg("EXERCICE", "PERSONNALISE", 120); break;
    case EX_TUTORIAL:  tut_show(); break;
    }
    tgt_x = 0;
    if (cfg.ex == EX_SERVE) { tgt_x = 92; tgt_y = 100; }
}

/* cibles : 0 fond gauche, 1 fond droit, 2 court gauche, 3 court droit */
static const u8 zone_x[4] = { 94, 154, 94, 154 };
static const u8 zone_y[4] = { 58, 58, 100, 100 };

void train_feed(void)
{
    s16 px = PX(plx[0]);
    s16 tx, ty = BOT_BASE - 28;
    u8 T = 50, side;
    feed_x = (CENTER_X + rnd(20)) * FP;
    feed_y = (TOP_BASE - 4) * FP;
    feed_z = 22 * FP;
    feed_g = 9;
    feed_spin = SPIN_TOP;
    if (cfg.ex != EX_STATIC) ++tr_total;
    tx = CENTER_X + rnd(40);
    switch (cfg.ex) {
    case EX_TARGETS:
    case EX_PRECISION:
        tr_target = (u8)(rand8() & (cfg.ex == EX_TARGETS ? 1 : 3));
        tgt_x = zone_x[tr_target]; tgt_y = zone_y[tr_target];
        break;
    case EX_MOVE:
        tr_alt ^= 1;
        tx = tr_alt ? SGL_L + 12 : SGL_R - 12;
        ty = BOT_BASE - 20;
        T = 46;
        break;
    case EX_STREAK:
        tx = CENTER_X + rnd(30);
        T = 54;
        break;
    case EX_VOLLEY:
        tx = px + rnd(26);
        ty = NET_Y + 30;
        T = 32; feed_g = 4; feed_spin = SPIN_FLAT; feed_z = 16 * FP;
        break;
    case EX_CUSTOM:
        side = cfg.ex_side;
        if (side == 2) side = tr_alt ^= 1;
        else if (side == 3) side = (u8)(rand8() & 1);
        tx = side ? px - 28 : px + 28;        /* 0 coup droit (à droite), 1 revers */
        T = (u8)(cfg.ex_speed == 0 ? 62 : cfg.ex_speed == 1 ? 50 : 38);
        if (cfg.ex_spin == 1) { feed_spin = SPIN_SLICE; feed_g = 5; }
        else if (cfg.ex_spin == 2) { feed_g = 6; T += 22; ty = BOT_BASE - 14; }
        break;
    case EX_TUTORIAL:
        T = 60;
        break;
    }
    if (tx < SGL_L + 6) tx = SGL_L + 6;
    if (tx > SGL_R - 6) tx = SGL_R - 6;
    feed_tx = tx; feed_ty = ty; feed_T = T;
}

static void check_end(void)
{
    if (cfg.ex != EX_STREAK && cfg.ex != EX_TUTORIAL && tr_total >= tr_count) {
        tr_done = 1;
        tgt_x = 0;
        tmsg("EXERCICE", "TERMINE !", 200);
    }
}

void train_event(void)
{
    u8 zone;
    hud_dirty = 1;
    if (cfg.ex == EX_TUTORIAL) {
        if ((tr_ev == TEV_LAND_IN && (tr_step == 0 || (tr_step == 1 && last_shot == SH_DROP)
             || (tr_step == 2 && last_shot == SH_LOB) || (tr_step == 3 && last_aim)))
            || (tr_ev == TEV_SERVE_IN && tr_step == 4)) {
            ++tr_score;
            if (++tr_got >= tut_need[tr_step]) {
                tr_got = 0;
                ++tr_step;
                if (tr_step == 4) { tr_human_serves = 1; sc_server = 0; }
                if (tr_step == 5) { tr_done = 1; music_play(MUS_VICTORY); }
                sfx_menu_ok();
            }
            tut_show();
        } else if (tr_ev != TEV_LAND_IN && tr_ev != TEV_SERVE_IN) {
            tut_show();
        }
        if (tr_ev == TEV_SERVE_IN || tr_ev == TEV_SERVE_FAULT) ++tr_total;
        return;
    }
    /* joueur fixe : l'exercice compte les frappes du joueur, pas les balles lancées */
    if (cfg.ex == EX_STATIC && (tr_ev == TEV_LAND_IN || tr_ev == TEV_LAND_OUT || tr_ev == TEV_MISS)) ++tr_total;
    switch (tr_ev) {
    case TEV_LAND_IN:
        if (cfg.ex == EX_STREAK) {
            ++tr_streak;
            if (tr_streak > tr_score) tr_score = tr_streak;
            tmsg("SERIE", "", 30);
            msg1[6] = (char)(tr_streak >= 10 ? '0' + tr_streak / 10 : ' ');
            msg1[7] = (char)('0' + tr_streak % 10);
            if (tr_streak >= 50) { tr_done = 1; }
            break;
        }
        if (cfg.ex == EX_TARGETS || cfg.ex == EX_PRECISION) {
            zone = (u8)((ev_x >= CENTER_X ? 1 : 0) | (ev_y >= TOP_SERV + 6 ? 2 : 0));
            if (zone == tr_target) { tr_score += 2; tmsg("CIBLE ! +2", "", 40); sfx_crowd(4); }
            else if (cfg.ex == EX_TARGETS) { ++tr_score; tmsg("BONNE +1", "", 40); }
            else tmsg("HORS ZONE", "", 40);
            break;
        }
        ++tr_score;
        tmsg("BONNE +1", "", 40);
        break;
    case TEV_LAND_OUT:
    case TEV_MISS:
        if (cfg.ex == EX_STREAK) {
            tr_done = 1;
            tmsg("FIN DE SERIE", "", 200);
            return;
        }
        tmsg(tr_ev == TEV_MISS ? "MANQUE" : "DEHORS", "", 40);
        break;
    case TEV_SERVE_IN:
        ++tr_total;
        ++tr_score;
        /* bonus si le service tombe dans le tiers visé (extérieur ou T) */
        if (ev_x < SGL_L + 18 || ev_x > SGL_R - 18 || (ev_x > CENTER_X - 18 && ev_x < CENTER_X + 18)) {
            ++tr_score; tmsg("ACE ? +2", "", 40);
        } else tmsg("BON +1", "", 40);
        break;
    case TEV_SERVE_FAULT:
        ++tr_total;
        break;
    }
    if (cfg.ex == EX_SERVE) {
        /* étoile : alterne la cible à chaque service selon le côté */
        tgt_x = (u8)(((sc_points[0] ^ 1) & 1) ? 160 : 92);   /* côté du prochain service */
    }
    check_end();
}

/* ---------------------------------------------------------------------- */
/* Filet qui tremble en perspective (version 2.68, déplacé de highlight.c) */
/* ---------------------------------------------------------------------- */
#define BANK_PNET 16
extern const unsigned char * const persp_net[7];
extern u8 net_shake, net_state;
#pragma bss-name(push, "WBSS")
static u8 pn_pos, pn_buf[17];
#pragma bss-name(pop)
void pnet_tick(void)
{
    const u8 *p;
    u8 n, want = (u8)(net_shake > 6);
    if (want != net_state) {
        bank_read(BANK_PNET, &p, &persp_net[cfg.env], 2);
        bank_read(BANK_PNET, &n, p, 1);
        while (pn_pos < n) {
            const u8 *q = p + 1 + (u16)pn_pos * 33;
            bank_read(BANK_PNET, pn_buf, q, 1);
            bank_read(BANK_PNET, pn_buf + 1, q + 1 + (want ? 16 : 0), 16);
            if (!vbuf_raw((u16)pn_buf[0] << 4, pn_buf + 1, 16)) break;   /* budget de la trame : la suite à la suivante */
            ++pn_pos;
        }
        if (pn_pos >= n) { net_state = want; pn_pos = 0; }
    }
    /* (le filet revient au repos avant la fin du tremblement) */
    if (net_shake > 1 || net_state == 0) --net_shake;
}

/* ---------------------------------------------------------------------- */
/* Drapeaux des supporters (version 2.74)                                  */
/* ---------------------------------------------------------------------- */
extern u8 ola_on, oam_i, hl_mode, cer;
#pragma zpsym("oam_i")
extern const unsigned char flag_tile;
static const u8 flag_x[6] = { 6, 14, 6, 242, 234, 242 };
static const u8 flag_y[6] = { 52, 96, 168, 64, 116, 176 };
void flags_draw(void)
{
    u8 i, a;
    if (cfg.mode != GM_MATCH || cfg.env == ENV_NUDE || cfg.env == ENV_MARGAUX || hl_mode) return;
    if (!((u8)(cer - 4) < 7 || ola_on >= 2)) return;     /* (cer 4 à 10 : chaises, assis, retour ; fin du match) */
    for (i = 0; i < 6 && oam_i < 252; ++i) {
        a = (u8)(i < 3 ? 0 : 1);                         /* palette : équipe de gauche, de droite */
        if (((frame_cnt >> 3) + i) & 1) a |= 0x40;       /* agité */
        spr1(flag_x[i], flag_y[i], flag_tile, a);
    }
}
