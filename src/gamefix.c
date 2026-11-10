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

#define CHAIR_SY 112     /* haut de la chaise d'arbitre (tools/gen_assets.py)  */
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

/* Nom du coup gagnant affiché au centre du bandeau (15 caractères au plus) */
const char *win_label(u8 shot, u8 pass, u8 jump)
{
    if (hit_perf) return "COUP PARFAIT !";
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
extern u8 pl_jump[4], tired;
u8 st_perf[2], st_ptry[2], hit_perf, swing_early;
static u8 fx_t, fx_x, fx_y, fx_kind, fx_bounces, fx_hits, mk_t;
static u8 perf_t, perf_x, perf_y;

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
    /* ombres de tous les joueurs ; en double, quand trop de sprites
       partagent une ligne, la rotation de l'ordre des sprites (match_draw)
       évite qu'un sprite disparaisse */
    for (i = 0; i < npl; ++i) {
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
    /* fatigue : goutte de sueur au-dessus des joueurs (clignote) ; en double,
       seulement pour les joueurs humains (limite de sprites par ligne) */
    if (tired >= 4 && (frame_cnt & 16)) {
        for (i = 0; i < npl; ++i) {
            if (npl == 4 && cfg.ctrl[i] == CTRL_CPU) continue;
            spr1((u8)(PXU(plx[i]) + 6), (u8)(PXU(ply[i]) - 30), 14, 3);
        }
    }
    /* frappe parfaite : petit éclat près de la raquette */
    if (perf_t) { --perf_t; spr1(perf_x, perf_y, 3, 2); }   /* blanc : visible sur tous les courts */
    /* service canon : traînée de deux points derrière la balle */
    if (srv_canon && b_serve && b_live && bounces == 0) {
        for (j = 2; j < 6; j += 3) {
            s16 z = bz - bvz * j;
            if (z < 0) z = 0;
            spr1((u8)(PXU(bx - bvx * j) - 2), (u8)(PXU(by - bvy * j) - PXU(z) - 2), 1, 3);
        }
    }
    /* arbitre de chaise (tous les lieux), assis de profil sur sa chaise
       (décor, x 16-31), tourné vers le court : bras tendu sur une faute ou
       une balle dehors ; bras levé qui s'agite sur un let ; bras levé en
       grand à la fin du match. Tuiles : 11 tête, 12 buste, 13 bras tendu ;
       coupe + 0 main, + 1 bras levé, + 2 buste bras levé */
    ump_draw();      /* assembleur (src/sprp.s) : 3 sprites au plus, sans appel coûteux */
}

/* Nom d'équipe sur 4 lettres (panneaux de score, annonces) : « ALEX » en
   simple, « MXAL » en double. Abréviations de team_label : MARGAUX donne
   « MX », pour ne pas se confondre avec MAYA (« MA »). */
void team_name(u8 t, char *out)
{
    char b[6];
    if (cfg.doubles) {
        team_label(cfg.chr[t], cfg.chr[t + 2], b);
        out[0] = b[0]; out[1] = b[1]; out[2] = b[3]; out[3] = b[4];
    } else {
        team_label(cfg.chr[t], 255, b);
        out[0] = b[0]; out[1] = b[1]; out[2] = b[2]; out[3] = b[3];
    }
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
    dx = PX(bx) - PX(plx[p]);
    dy = PX(by) - PX(ply[p]);
    /* zone de frappe généreuse : les balles un peu loin partent quand même */
    if (dx < -24 || dx > 24 || dy < -15 || dy > 10) return 0;
    if (bz > 40 * FP) {
        /* balle très haute : smash sauté (joueurs humains et CPU expert),
           avant le rebond et hors du fond du court */
        if (bz > 62 * FP || bounces || iabs(PX(ply[p]) - NET_Y) > 70) return 0;
        if (cfg.ctrl[p] == CTRL_CPU && cfg.level[p] != 2) return 0;
    }
    return 1;
}

/* ---------------------------------------------------------------------- */
/* Frappe parfaite : coup lifté, à plat ou coupé après le rebond, balle à  */
/* côté du corps (ni collée, ni en extension) et, pour un humain, bouton   */
/* relâché juste avant l'arrivée de la balle (3 premières trames du geste) */
/* ---------------------------------------------------------------------- */

/* Entraînement : écart au bon moment, en images (trames) */
#pragma bss-name(push, "WBSS")
static char timing_b[12];
#pragma bss-name(pop)
void timing_msg(u8 early, u8 n)
{
    char *b = timing_b;
    if (n > 9) n = 9;
    b[0] = (char)('0' + n);
    b[1] = ' '; b[2] = 'I'; b[3] = 'M'; b[4] = 'A'; b[5] = 'G'; b[6] = 'E';
    b[7] = (char)(n > 1 ? 'S' : 0); b[8] = 0;
    set_msg(early ? "TROP TOT DE" : "TROP TARD DE", b, 45);
}

/* Entraînement, service frappé avec A : écart au sommet du lancer (vz = 4
   ou -2 au sommet ; la vitesse verticale perd 6 par trame) */
void serve_fb(s16 vz)
{
    if (vz > 4) timing_msg(1, (u8)((u8)(vz - 4) / 6));
    else if (vz < -2) timing_msg(0, (u8)((u8)(-2 - vz) / 6));
}

u8 perfect_hit(u8 p, s16 dx)
{
    u8 t = (u8)(p & 1), r = 0;
    if (bounces == 0 || (last_shot != SH_DRIVE && last_shot != SH_FLAT && last_shot != SH_SLICE)) return 0;
    if (cfg.ctrl[p] != CTRL_CPU) ++st_ptry[t];
    /* balle à côté du corps : ni collée (< 5 px), ni en extension (> 14 px) ;
       la frappe part dès que la balle entre dans la zone, sa position en
       profondeur n'est donc pas un critère */
    if (dx < 0) dx = -dx;
    if (dx < 5 || dx > 14) {
        if (cfg.mode == GM_TRAIN && cfg.ctrl[p] != CTRL_CPU)
            set_msg("BALLE TROP", dx < 5 ? "PRES DU CORPS" : "LOIN DU CORPS", 45);
        return 0;
    }
    if (cfg.ctrl[p] == CTRL_CPU) {
        /* CPU : Expert de temps en temps, Normal rarement, Facile jamais */
        if (cfg.level[p] == 2) r = (u8)(rand8() < 40);
        else if (cfg.level[p] == 1) r = (u8)(rand8() < 10);
    } else {
        /* bouton relâché 0 à 2 trames avant l'arrivée : 3 premières trames du
           geste (pl_timer 14, 13, 12) */
        r = (u8)(swing_early >= 12);
        if (cfg.mode == GM_TRAIN) {
            if (r) set_msg("COUP PARFAIT !", "", 45);
            else if (!swing_early) set_msg("RELACHEZ LE", "BOUTON AVANT", 45);
            else timing_msg(1, (u8)(12 - swing_early));
        }
    }
    if (r) {
        ++st_perf[t];
        /* éclat au bout de la raquette, hors du sprite du joueur (sinon il
           passe derrière lui) : à droite ou à gauche selon le côté de la balle */
        perf_t = 12;
        perf_x = (u8)(PXU(plx[p]) + (PX(bx) < PX(plx[p]) ? -16 : 8));
        perf_y = (u8)(PXU(ply[p]) - 22);
    }
    return r;
}

/* ---------------------------------------------------------------------- */
/* Récapitulatif du score : une ligne par équipe, « ALEX 6 3 2 », sets    */
/* terminés puis jeux du set en cours (14 caractères pour 5 sets).        */
/* ---------------------------------------------------------------------- */
extern u8 recap_pend;
void score_line(u8 t, char *out)
{
    u8 i, k = 4;
    team_name(t, out);
    for (i = 0; i < sc_set_no && i < MAX_SETS; ++i) { out[k++] = ' '; out[k++] = (char)('0' + sc_hist[i][t]); }
    if (!sc_over && k < 14) { out[k++] = ' '; out[k++] = (char)('0' + sc_games[t]); }
    out[k] = 0;                     /* les deux lignes ont la même longueur : centrées ensemble */
}

/* bandeau : le score set par set pendant 2 secondes */
#pragma bss-name(push, "WBSS")
static char recap_a[16], recap_b[16];
#pragma bss-name(pop)
void score_recap(void)
{
    char *a = recap_a, *b = recap_b;
    score_line(0, a);
    score_line(1, b);
    set_msg(a, b, 120);
    recap_pend = 0;
}

/* Feux d'artifice (fête de victoire sur le court, levée de coupe). t est un
   compte à rebours (ms_timer sur le court) : chaque fusée siffle en montant
   à t = éclatement + 15, puis éclate (« pop ») à t = 115, 75 ou 35 en 4
   étoiles qui s'écartent pendant 14 trames ; dy décale les gerbes vers le
   bas. Sons déclenchés par la logique (fw_sound), gerbes dessinées avec le
   reste (fw_draw). */
static const u8 fw_pop_t[3] = { 115, 75, 35 };
static const u8 fw_px[3] = { 72, 176, 124 };
static const u8 fw_py[3] = { 64, 80, 52 };
void fw_sound(u8 t)
{
    u8 k;
    for (k = 0; k < 3; ++k) {
        if (t == (u8)(fw_pop_t[k] + 15)) sfx_fw_launch();
        else if (t == fw_pop_t[k]) sfx_fw_pop();
    }
}
void fw_draw(u8 t, u8 dy)
{
    u8 k, d, x, y;
    if (t > 115) return;                         /* rien avant le 1er éclatement */
    k = (u8)(t > 75 ? 0 : (t > 35 ? 1 : 2));     /* dernier feu éclaté */
    d = (u8)(fw_pop_t[k] - t);                   /* trames depuis l'éclatement */
    if (d >= 14) return;
    x = fw_px[k]; y = (u8)(fw_py[k] + dy);
    spr1((u8)(x - d), y, 3, 3);
    spr1((u8)(x + d), y, 3, 3);
    spr1(x, (u8)(y - d), 3, 2);
    spr1(x, (u8)(y + d), 3, 2);
}
