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
/* (2.76) chronomètre du match : temps réel (trames de l'écran), hors pause,
   par set ; remis à zéro à l'entrée des joueurs (cer.c) */
extern u16 tm_set[5];
extern u8 tm_last, tm_sub;
static void tm_tick(void)
{
    u8 d = (u8)(frame_cnt - tm_last), fps = (u8)(is_pal ? 50 : 60);
    tm_last = frame_cnt;
    if (paused || cfg.mode != GM_MATCH || sc_over) return;
    tm_sub = (u8)(tm_sub + d);
    while (tm_sub >= fps) { tm_sub = (u8)(tm_sub - fps); ++tm_set[sc_set_no < 5 ? sc_set_no : 4]; }
}
void flags_draw(void)
{
    u8 i, a;
    tm_tick();
    if (cfg.mode != GM_MATCH || cfg.env == ENV_NUDE || cfg.env == ENV_MARGAUX || hl_mode) return;
    if (!((u8)(cer - 4) < 7 || ola_on >= 2)) return;     /* (cer 4 à 10 : chaises, assis, retour ; fin du match) */
    for (i = 0; i < 6 && oam_i < 252; ++i) {
        a = (u8)(i < 3 ? 0 : 1);                         /* palette : équipe de gauche, de droite */
        if (((frame_cnt >> 3) + i) & 1) a |= 0x40;       /* agité */
        spr1(flag_x[i], flag_y[i], flag_tile, a);
    }
}

/* ---------------------------------------------------------------------- */
/* Médailles du palmarès (2.72, déplacé de highlight.c en 2.75)            */
/* ---------------------------------------------------------------------- */
/* (2.72 : déplacée de la banque 8, pleine ; appelée par screens.c) */
extern u8 pal_page;
#include "stats.h"
#include "comp.h"
/* page des résultats : une médaille devant chaque meilleur résultat et
   chaque compétition de l'historique terminée sur le podium (1er, 2e, 3e) */
void palmares_sprites(void)
{
    u8 i, t;
    spr_begin();
    if (pal_page == 1) {
        for (i = 0; i < NCT; ++i) {
            t = (u8)(i < 5 ? sv.comp_best[i] : sw_best[i - 4]);
            if (t >= 1 && t <= 3) spr1(9, (u8)((6 + i) * 8 - 1), (u8)(ball_base + 8), (u8)(t - 1));
        }
        for (i = 0; i < sv.hist_n && i < 8; ++i) {
            t = sv.hist[i][1];
            if (t >= 1 && t <= 3) spr1(9, (u8)((15 + i) * 8 - 1), (u8)(ball_base + 8), (u8)(t - 1));
        }
    }
    spr_end();
}

/* ---------------------------------------------------------------------- */
/* (2.73 : déplacé de la banque fixe) message du coup gagnant (game.c) */
extern u8 hit_perf, hit_pass, hit_jump;   /* (last_shot : game_int.h) */
void set_msg(const char *a, const char *b, u8 t);
static /* Nom du coup gagnant affiché au centre du bandeau (15 caractères au plus) */
const char *win_label(u8 shot, u8 pass, u8 jump)
{
    if (hit_perf) return "COUP PARFAIT !";
    if (shot == SH_SMASH) return bspin == SPIN_DROP ? "AMORTIE HAUTE" : jump ? "SMASH SAUTE !" : "SMASH !";   /* (2.76 : smash amorti) */
    if (shot == SH_DROP) return "AMORTIE !";
    if (shot == SH_LOB) return "LOB GAGNANT !";
    if (shot == SH_VOLLEY) return "VOLEE GAGNANTE";
    if (rally_hits == 2) return "RETOUR GAGNANT";   /* (2.77) */
    if (pass) return "PASSING !";
    return "COUP GAGNANT !";
}
void win_msg(void) { set_msg(win_label(last_shot, hit_pass, hit_jump), "", 70); }

/* ---------------------------------------------------------------------- */
/* Lignes des statistiques et du palmarès (2.70-2.71, déplacées de highlight.c) */
/* ---------------------------------------------------------------------- */
extern u8 sp_new;      /* nouveau record de vitesse (stats.c) */
extern u8 st_vmax[2];  /* (highlight.c) */
/* ligne « SERVICE MAX KM/H » des écrans de statistiques (screens.c), « --- » sans service */
u16 __fastcall__ hl_vmax_row(u16 y)
{
    u8 t;
    u16 k;
    char n[4];
    if (sp_new) { txt_off = 0x40; txt_at(2, (u8)y, "KM/H : RECORD !"); txt_off = 0; }   /* (palmarès, en or) */
    else txt_at(2, (u8)y, "SERVICE MAX KM/H");
    n[3] = 0;
    for (t = 0; t < 2; ++t) {
        k = (u16)st_vmax[t] + 80;
        if (st_vmax[t]) { n[0] = (char)('0' + k / 100); n[1] = (char)('0' + (k / 10) % 10); n[2] = (char)('0' + k % 10); }
        else n[0] = n[1] = n[2] = '-';
        txt_at((u8)(19 + t * 6), (u8)y, n);
    }
    return 0;
}
/* (2.75) écran des résultats : l'échange le plus long est commun aux deux
   joueurs ; hors du tableau, en or, centré entre les deux joueurs :
   « PLUS LONG / ECHANGE / 20 FRAPPES » */
extern u8 st_longest;
u16 __fastcall__ hl_rally_row(u16 y)
{
    char n[3];
    u8 x = 0, v = st_longest;
    txt_off = 0x40;
    txt_at(11, (u8)y, "PLUS LONG");
    txt_at(12, (u8)(y + 1), "ECHANGE");
    if (v >= 10) n[x++] = (char)('0' + v / 10);
    n[x++] = (char)('0' + v % 10); n[x] = 0;
    x = (u8)(v > 1 ? 11 - (x >> 1) : 12 - (x >> 1));      /* (centré sur la colonne 15,5) */
    txt_at(x, (u8)(y + 2), n);
    txt_at((u8)(x + (v >= 10 ? 3 : 2)), (u8)(y + 2), v > 1 ? "FRAPPES" : "FRAPPE");
    txt_off = 0;
    return 0;
}
/* palmarès, rangée 22 (menuart.c) : « RECORD SERVICE  212 KM/H ALEX » */
extern u8 sw_spd[4];
void hl_rec_row(void)
{
    u16 k = (u16)sw_spd[1] + 80;
    char n[5];
    txt_at(3, 22, "RECORD SERVICE");
    if (!sw_spd[1]) { txt_at(22, 22, "---"); return; }
    n[0] = (char)('0' + k / 100); n[1] = (char)('0' + (k / 10) % 10); n[2] = (char)('0' + k % 10); n[3] = 0;
    txt_at(18, 22, n); txt_at(22, 22, "KM/H");
    team_label(sw_spd[2], 255, n); txt_at(27, 22, n);
}

/* ====================================================================== */
/* Remise du trophée (2.72, podium.c, banque 8 pleine) : tribunes de chaque  */
/* côté du court (colonnes 0-1 et 30-31, rangées 10 à 27) ; le public est    */
/* assis pendant les médailles, se lève quand la coupe est brandie et agite */
/* les bras ; le finaliste lève une plaque d'argent (sprites 62-63, palette */
/* des médailles) ; confettis aux couleurs du vainqueur et or (podium.c).   */
/* ====================================================================== */
#define T_FAN 0xB1                 /* 2 tuiles du décor ($B1, $B2), libres au podium */
#define S_PLATE 0x3C               /* 2 tuiles de sprites (personnages de dos, inutiles ici) */
static const u8 fan_sit[32] = {
    0x00,0x18,0x18,0x3C,0x7E,0x7E,0x00,0x00, 0x00,0x18,0x18,0x00,0x00,0x00,0x7E,0x00,
    0x00,0x0C,0x0C,0x00,0x00,0x00,0x3F,0x00, 0x00,0x0C,0x0C,0x1E,0x3F,0x3F,0x00,0x00 };
static const u8 fan_up[32] = {
    0x42,0x5A,0x5A,0x7E,0x3C,0x3C,0x24,0x24, 0x42,0x18,0x18,0x00,0x00,0x00,0x00,0x00,
    0x81,0x5A,0x3A,0x3C,0x3C,0x3C,0x24,0x24, 0x81,0x18,0x18,0x00,0x00,0x00,0x00,0x00 };
static const u8 plate_chr[32] = {
    0x00,0x7F,0x80,0x80,0x80,0x7F,0x00,0x00, 0x00,0x00,0x7F,0x7F,0x7F,0x00,0x00,0x00,
    0x00,0xFE,0x01,0x01,0x01,0xFE,0x00,0x00, 0x00,0x00,0xFE,0xFE,0xFE,0x00,0x00,0x00 };
static u8 pod_px, pod_py;
/* écran du podium (rendu coupé) */
void pod_extra_build(void)
{
    u8 r, i;
    vram_adr((u16)T_FAN << 4);
    for (i = 0; i < 32; ++i) vram_put(fan_sit[i]);
    vram_adr(0x1000 + ((u16)S_PLATE << 4));
    for (i = 0; i < 32; ++i) vram_put(plate_chr[i]);
    for (r = 10; r < 28; ++r) {
        i = (u8)(T_FAN + (r & 1));
        vram_adr(NTADR(0, r)); vram_put(i); vram_put((u8)(i ^ 3));     /* ($B1 <-> $B2) */
        vram_adr(NTADR(30, r)); vram_put((u8)(i ^ 3)); vram_put(i);
    }
    /* le finaliste (2e marche, à gauche) : x2 + 1, rangée 14 */
    pod_px = (u8)((((32 - (c_dbl ? 22 : 18)) >> 1) + 1) * 8 + 8);
    pod_py = 14 * 8;
}
/* à chaque trame : t = 0 avant la coupe, puis l'âge de la coupe (cup_t) */
u16 __fastcall__ pod_extra_tick(u16 t)
{
    u8 y;
    oam_buf[248] = oam_buf[252] = 0xFF;
    if (t < 24) return 0;
    if (t == 24) sfx_crowd(7);                         /* le public se lève */
    if (t == 24 || (t & 15) == 8)                      /* et agite les bras */
        if (vbuf_raw((u16)T_FAN << 4, fan_up + (((t >> 4) & 1) << 4), 16))
            vbuf_raw((u16)(T_FAN + 1) << 4, fan_up + ((((t >> 4) & 1) ^ 1) << 4), 16);
    /* la plaque, tenue à deux mains devant le finaliste : elle apparaît
       avec la coupe, puis il la soulève un peu, en rythme */
    y = (u8)(pod_py + 28 - (t < 40 ? (t - 24) >> 2 : 4 + (((t >> 4) & 1) ? 1 : 0)));
    oam_buf[248] = (u8)(y - 1); oam_buf[249] = S_PLATE;     oam_buf[250] = 3; oam_buf[251] = pod_px;
    oam_buf[252] = (u8)(y - 1); oam_buf[253] = S_PLATE + 1; oam_buf[254] = 3; oam_buf[255] = (u8)(pod_px + 8);
    return 0;
}

/* Balle de match (2.75) : bandeau au centre de l'écran (rangées 12 à 17, les
   mêmes que le bandeau de la pause), dessiné d'avance sur l'écran 1 à raison
   d'une rangée par trame, puis montré par deux interruptions de ligne
   (cband_on, crt0.s) : pas d'écran coupé. Il disparaît seul après 2 s, au
   lancer de la balle ou à un appui. Appelé par band_draw (highlight.c) ;
   mp_t : 0 rien, 1 à 7 écriture, puis 8 à 8 + MP_SHOW affiché. */
extern u8 band_vis, band_dis, band_hold, hl_flags, mp_t, sc_crit;
#pragma zpsym("band_hold")
#pragma zpsym("hl_flags")
#pragma zpsym("mp_t")
#define MP_ROW  12
#define T_RULE  0xFF            /* (filet du bandeau, highlight.c) */
#define MP_SHOW 120
void mp_tick(void)
{
    static const char t_mp[] = "BALLE DE MATCH";
    u8 i, r;
    char b[32];
    if (!mp_t) {
        if (ms != MS_SERVE || cer || paused || rp_pending || !(sc_crit & 4) || (hl_flags & 2)) return;
        if (band_hold && !band_dis) return;          /* (bandeau du score attendu ou en attente d'un appui) */
        hl_flags |= 2;
        if (band_vis) { split_stop(); band_vis = 0; }
        band_dis = 1;
        mp_t = 1;
        if (msg_timer && !memcmp(msg1, t_mp, 14)) msg_timer = 1;   /* (l'annonce du haut s'efface : le bandeau la remplace) */
        sfx_crowd(4);
    }
    if (mp_t < 8) {
        r = (u8)(mp_t - 1);
        memset(b, 0, 32);
        if (r == 1 || r == 4) { memset(b + 1, T_RULE, 30); }
        else if (r == 2) memcpy(b + 9, t_mp, 14);
        else if (r == 3) {
            i = (u8)((sc_crit & 1) ? 0 : 1);
            memcpy(b + 11, "POUR ", 5);
            team_label(cfg.chr[i], cfg.doubles ? cfg.chr[i + 2] : 255, b + 16);
        }
        if (r == 6) {                                /* attributs : palette 1 */
            memset(b, 0x55, 16);
            if (!vbuf_raw(0x27C0 + 24, (const u8 *)b, 16)) return;
        } else {
            if (vram_len + 40 > VBUF_MAX) return;
            vbuf_raw((u16)(0x2400 + (MP_ROW + r) * 32), (const u8 *)b, 16);
            vbuf_raw((u16)(0x2400 + (MP_ROW + r) * 32 + 16), (const u8 *)b + 16, 16);
        }
        if (++mp_t == 8) cband_on(47);
        return;
    }
    if (paused || ms != MS_SERVE || ++mp_t >= 8 + MP_SHOW || (pad1_new | pad2_new)) {
        split_stop();
        mp_t = 0;
        return;
    }
    /* (2.76) rien par-dessus le bandeau (l'arbitre de chaise s'y incrustait) */
    for (i = 0; i < 64; ++i) { r = oam_buf[i * 4]; if (r >= MP_ROW * 8 - 1 && r < (MP_ROW + 6) * 8) oam_buf[i * 4] = 0xFF; }
}

/* ---------------------------------------------------------------------- */
/* Fin de set (2.76) : pendant que les joueurs vont s'asseoir, le bandeau   */
/* central (comme celui de la balle de match) annonce « 1ER SET » et       */
/* « POUR ALEX 6-4 » ; puis l'écran du set en fondu (cer.c, game.c).       */
/* st_t : 0 rien, 1 à 7 écriture, 8 affiché (remis à 0 par match_resume).   */
/* ---------------------------------------------------------------------- */
u8 st_t;
void st_tick(void)
{
    u8 i, r, n, w, k;
    char b[32];
    if (!st_t) {
        if (band_vis) { split_stop(); band_vis = 0; }
        band_dis = 1;
        st_t = 1;
    }
    if (st_t < 8) {
        r = (u8)(st_t - 1);
        n = 0; while (n < 5 && (sc_hist[n][0] | sc_hist[n][1])) ++n;
        if (!n) n = 1;
        w = (u8)(sc_hist[n - 1][1] > sc_hist[n - 1][0]);
        memset(b, 0, 32);
        if (r == 1 || r == 4) { memset(b + 1, T_RULE, 30); }
        else if (r == 2) {
            b[12] = (char)('0' + n);
            if (n == 1) memcpy(b + 13, "ER SET", 6); else memcpy(b + 13, "E SET", 5);
        } else if (r == 3) {
            memcpy(b + 6, "POUR ", 5);
            team_label(cfg.chr[w], cfg.doubles ? cfg.chr[w + 2] : 255, b + 11);
            for (k = 11; b[k]; ++k) ;
            k = (u8)(k + 2);
            b[k] = (char)('0' + sc_hist[n - 1][w]); b[k + 1] = '-'; b[k + 2] = (char)('0' + sc_hist[n - 1][w ^ 1]);
        }
        if (r == 6) {
            memset(b, 0x55, 16);
            if (!vbuf_raw(0x27C0 + 24, (const u8 *)b, 16)) return;
        } else {
            if (vram_len + 40 > VBUF_MAX) return;
            vbuf_raw((u16)(0x2400 + (MP_ROW + r) * 32), (const u8 *)b, 16);
            vbuf_raw((u16)(0x2400 + (MP_ROW + r) * 32 + 16), (const u8 *)b + 16, 16);
        }
        if (++st_t == 8) { cband_on(47); sfx_crowd(4); }
        return;
    }
    /* rien par-dessus le bandeau (comme pour la balle de match) */
    for (i = 0; i < 64; ++i) { r = oam_buf[i * 4]; if (r >= MP_ROW * 8 - 1 && r < (MP_ROW + 6) * 8) oam_buf[i * 4] = 0xFF; }
}

/* ---------------------------------------------------------------------- */
/* Rebond (2.76, appelés par ball_bounce, game.c)                          */
extern u8 sc_swap;
#define side_of(t) ((u8)(((t) & 1) ^ sc_swap))
/* Point de contact : le rebond est vu à la fin de la trame, la balle déjà
   passée sous le sol ; elle a touché le sol une fraction t = bz / bvz de
   trame plus tôt (jusqu'à 3-4 px plus loin à pleine vitesse : une balle sur
   la ligne était jugée dehors). Elle y est ramenée avant d'être jugée. */
void bounce_fix(void)
{
    u16 t;
    if (bz >= 0 || bvz >= 0) return;
    t = (u16)(((u16)(-bz) << 6) / (u16)(-bvz));     /* (0 à 64) */
    if (t > 64) t = 64;
    bx -= (s16)((bvx * (s16)t) >> 6);
    by -= (s16)((bvy * (s16)t) >> 6);
}
/* service dans le bon carré (ligne touchée : bonne) */
u16 __fastcall__ srv_box(u16 unused)
{
    u8 srv_side = side_of(hitter);
    u8 ad = rules_ad_side();
    /* bas+égalité -> carré gauche ; bas+avantage -> droit ; inversé en haut */
    u8 left = (u8)((srv_side == 0) == (ad == 0));
    s16 x = PX(bx), y = PX(by);
    (void)unused;
    if (left) { if (x < IN_SL || x > IN_CR) return 0; }    /* (ligne médiane comprise, rayon de la balle) */
    else      { if (x < IN_CL || x > IN_SR) return 0; }
    if (srv_side == 0) return (u16)(y >= IN_TSRV && y <= NET_Y);
    return (u16)(y >= NET_Y && y <= IN_BSRV);
}

/* (2.76) écrans de fin de set et de résultat : « DUREE DU MATCH 1H05 » au-
   dessus du tableau des sets (rangée y - 1), minutes de chaque set dessous
   (rangée y + 3), dans la colonne du set. a : y | (sets joués << 8) */
static void tm_put(u8 x, u8 y, u16 v)          /* nombre à droite de x */
{
    char n[4];
    u8 i = 3;
    n[3] = 0;
    do { n[--i] = (char)('0' + v % 10); v /= 10; } while (v && i);
    txt_at((u8)(x - (3 - i) + 1), y, n + i);
}
u16 __fastcall__ hl_time_rows(u16 a)
{
    u8 y = (u8)a, n = (u8)(a >> 8), i;
    u16 tot = 0, m;
    for (i = 0; i < 5; ++i) tot += tm_set[i];
    m = (tot + 30) / 60;
    txt_at(8, (u8)(y - 1), "DUREE DU MATCH");
    if (m >= 60) { tm_put(24, (u8)(y - 1), m / 60); txt_at(25, (u8)(y - 1), "H"); m %= 60; txt_at(26, (u8)(y - 1), m < 10 ? "0" : ""); tm_put(27, (u8)(y - 1), m); }
    else { tm_put(25, (u8)(y - 1), m); txt_at(27, (u8)(y - 1), "MIN"); }
    if (n) {
        txt_at(8, (u8)(y + 3), "MIN");
        for (i = 0; i < n && i < 5; ++i) tm_put((u8)(15 + i * 3), (u8)(y + 3), (tm_set[i] + 30) / 60);
    }
    return 0;
}

/* (2.76, déplacé de screens.c, banque 1 pleine) statistiques de l'écran des
   résultats : noms (rangée 10), lignes 11 à 18, vitesse (19) */
static void st_row(u8 y, const char *label, u8 a, u8 b)
{
    char n[4];
    u8 t, v;
    n[3] = 0;
    txt_at(2, y, label);
    for (t = 0; t < 2; ++t) {
        v = t ? b : a;
        n[0] = (char)(v >= 100 ? '0' + v / 100 : ' ');
        n[1] = (char)(v >= 10 ? '0' + (v / 10) % 10 : ' ');
        n[2] = (char)('0' + v % 10);
        txt_at((u8)(19 + t * 6), y, n);
    }
}
static const char st_pc[2] = { 0x25, 0 };   /* « % » */
static u8 st_pct(u8 a, u8 b) { return b ? (u8)(((u16)a * 100) / b) : 0; }
u16 __fastcall__ res_stats(u16 unused)
{
    char b[6];
    (void)unused;
    team_label(cfg.chr[0], 255, b); txt_at(19, 10, b);
    team_label(cfg.chr[1], 255, b); txt_at(25, 10, b);
    st_row(11, "POINTS GAGNES", st_points[0], st_points[1]);
    st_row(12, "ACES", st_aces[0], st_aces[1]);
    st_row(13, "DOUBLES FAUTES", st_dfaults[0], st_dfaults[1]);
    st_row(14, "COUPS GAGNANTS", st_winners[0], st_winners[1]);
    st_row(15, "FAUTES", st_errors[0], st_errors[1]);
    st_row(16, "1ERES BALLES", st_pct(st_first_in[0], st_first_tot[0]), st_pct(st_first_in[1], st_first_tot[1]));
    txt_at(22, 16, st_pc); txt_at(28, 16, st_pc);  /* (symbole après chaque pourcentage ; écran des résultats, hors match : la tuile n'y dessine pas le court) */
    st_row(17, "SERVICES CANON", st_canon[0], st_canon[1]);
    st_row(18, "COUPS PARFAITS", st_perf[0], st_perf[1]);
    hl_vmax_row(19);
    return 0;
}

/* (2.76, déplacé de la banque fixe, pleine) feux d'artifice : game.c, podium.c */
static const u8 fw_pop_t2[3] = { 115, 75, 35 };
static const u8 fw_px2[3] = { 72, 176, 124 };
static const u8 fw_py2[3] = { 64, 80, 52 };
u8 fw_o2;                                         /* podium : où écrire les étincelles jaunes (0 : à la suite) */
u16 __fastcall__ fw_draw(u16 t16)
{
    u8 t = (u8)t16, k, d, x, y;
    if (t > 115) return 0;                         /* rien avant le 1er éclatement */
    k = (u8)(t > 75 ? 0 : (t > 35 ? 1 : 2));     /* dernier feu éclaté */
    d = (u8)(fw_pop_t2[k] - t);                   /* trames depuis l'éclatement */
    if (d >= 14) return 0;
    x = fw_px2[k]; y = (u8)(fw_py2[k]);
    /* (2.76) étincelles blanches (palette 2) en croix, jaunes (palette 3,
       couleur 3 passée au jaune pendant la fête) en diagonale */
    if (pal_buf[31] != 0x28) { pal_buf[31] = 0x28; pal_dirty = 1; }
    spr1((u8)(x - d), y, 3, 2);
    spr1((u8)(x + d), y, 3, 2);
    spr1(x, (u8)(y - d), 3, 2);
    spr1(x, (u8)(y + d), 3, 2);
    if (fw_o2) oam_i = fw_o2;                     /* (podium : entrées libres en fin d'OAM) */
    d = (u8)((d * 3) >> 2);
    spr1((u8)(x - d), (u8)(y - d), 3, 3);
    spr1((u8)(x + d), (u8)(y - d), 3, 3);
    if (fw_o2) return 0;                          /* (podium, 2.77 : 2 seulement, 62-63 sont la plaque du finaliste) */
    spr1((u8)(x - d), (u8)(y + d), 3, 3);
    spr1((u8)(x + d), (u8)(y + d), 3, 3);
    return 0;
}

