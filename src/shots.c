/* Royal Martinot — choix du coup (banque 13), appelé par do_hit (banque 0)
 * via bank_call. Voir le commentaire de shot_select. */
#include "game_int.h"
#include "app.h"
#include "rules.h"
#include "court.h"
#include "gfx.h"
#include "audio.h"

extern u8 tired, pl_jump[4];
/* ---------------------------------------------------------------------- */
/* Choix du coup (déplacé de do_hit, banque 0, version 2.42).
 * Entrées : sh_p (joueur), sh_pad (directions, HAUT/BAS relatifs au joueur),
 * sh_btn (PAD_A / PAD_B), sh_power (charge 0..32), sh_spread, sh_dx (balle -
 * joueur, px). Sorties : sh_depth, sh_speed, sh_grav, sh_spread, sh_short
 * (viser l'angle court comme le croisé court), bspin, last_shot, hit_jump,
 * hit_perf.
 *  - smash : balle prise avant le rebond à plus de 24 px de haut (au-dessus
 *    de la tête) ; sauté au-delà de 40 px. A : smash à plat, HAUT vers la
 *    ligne de fond, BAS dans l'angle court ; B : smash coupé, B + BAS smash
 *    amorti. La charge ajoute de la vitesse (jusqu'à + 16) et de l'écart ;
 *  - volée : les choix du joueur sont gardés, raccourcis (profondeur - 10)
 *    et bridés (vitesse 42 au plus) ; la volée à plat bien chargée passe
 *    jusqu'à 52 ; l'amortie volée meurt encore plus près du filet. */
/* ---------------------------------------------------------------------- */
#pragma bss-name(push, "WBSS")
u8 sh_p, sh_pad, sh_btn, sh_power, sh_spread, sh_speed, sh_grav, sh_short;
s16 sh_depth, sh_dx;
#pragma bss-name(pop)
extern u8 hit_jump;
s16 iabs(s16 v);

/* ---------------------------------------------------------------------- */
/* Frappe parfaite et messages de timing de l'entraînement (déplacés de la */
/* banque fixe, gamefix.c, version 2.52 : place pour les nouvelles poses)  */
/* ---------------------------------------------------------------------- */
#define PXU(v) ((u8)(((u16)(v) << 2) >> 8))      /* pixels, valeur positive */
extern u8 st_perf[2], st_ptry[2], swing_early, perf_t, perf_x, perf_y;
void set_msg(const char *a, const char *b, u8 t);   /* banque fixe */
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
u8 __fastcall__ serve_fb(s16 vz)
{
    if (vz > 4) timing_msg(1, (u8)((u8)(vz - 4) / 6));
    else if (vz < -2) timing_msg(0, (u8)((u8)(-2 - vz) / 6));
    return 0;
}

u8 perfect_hit(u8 p, s16 dx)
{
    u8 t = (u8)(p & 1), r = 0;
    if (bounces == 0 || (last_shot != SH_DRIVE && last_shot != SH_FLAT && last_shot != SH_SLICE && last_shot != SH_SHORT && last_shot != SH_LOB)) return 0;
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


extern u8 hit_margin;              /* game.c : angle des coups le long des lignes */
void shot_select(void)
{
    u8 p = sh_p, pad = sh_pad, power = sh_power, spread = sh_spread, speed, grav, T;
    u8 smash = (u8)(bounces == 0 && bz > 24 * FP);
    u8 volley = (u8)(bounces == 0), flat = 0;
    s16 depth;
    if (sh_dx > 14 || sh_dx < -14) spread += 8;          /* frappe en extension */
    sh_short = 0;
    bspin = SPIN_FLAT;
    hit_jump = (u8)(bz > 40 * FP);
    if (hit_jump) pl_jump[p] = 14;          /* le joueur saute, frappe en l'air et retombe */
    if (smash) {
        last_shot = SH_SMASH;
        if (sh_btn == PAD_B) {
            if (pad & PAD_DOWN) { depth = 24; speed = 44; grav = 8; bspin = SPIN_DROP; spread += 6; }   /* smash amorti */
            else { depth = 52; speed = 58; grav = 8; bspin = SPIN_SLICE; spread += 4; }                 /* smash coupé */
        } else {
            depth = 62; speed = 80; grav = 10; spread += 4;
            if (hit_jump) { depth = 56; speed = 96; grav = 12; spread += 4; }
            if (pad & PAD_UP) { depth += 12; spread += 2; }          /* vers la ligne de fond */
            else if (pad & PAD_DOWN) { depth = 34; sh_short = 1; spread += 2; hit_margin = 8; }   /* angle court */
            speed = (u8)(speed + (power >> 1));                      /* armé : plus fort... */
            spread = (u8)(spread + (power >> 2));                    /* ...et plus risqué */
        }
        sfx_crowd(5);
    } else if (sh_btn == PAD_B && (pad & PAD_UP)) {        /* lob */
        /* version 2.59 : longueur selon l'armé (relâché vite : lob court,
           smashable ; bouton tenu : près de la ligne de fond, risque de
           faute) ; trajectoire selon la position : du fond, lob défensif
           très haut et lent ; de l'intérieur du court, lob d'attaque plus
           bas et plus vif */
        T = (u8)(power > 22 ? 22 : power);
        depth = (s16)(60 + T);
        T = (u8)iabs(PX(ply[p]) - NET_Y);
        speed = (u8)(T >= 80 ? 19 : T < 60 ? 31 : 24);
        grav = 6; bspin = SPIN_TOP; last_shot = SH_LOB;
    } else if (sh_btn == PAD_B && (pad & PAD_DOWN)) {      /* amortie */
        /* amortie : trajectoire basse et vive juste au-dessus du filet, puis la
           balle « meurt » au rebond (effet SPIN_DROP) ; plus elle est jouée
           loin du filet, plus elle est imprécise et risque de rester trop
           courte (filet) : jamais un point garanti */
        T = (u8)(iabs(PX(by) - NET_Y) >> 3);
        depth = (s16)(22 - T); speed = 40; grav = 5; bspin = SPIN_DROP; spread += T; last_shot = SH_DROP;
    } else if (sh_btn == PAD_B) {                          /* coupé */
        depth = 66; speed = 36; grav = 4; bspin = SPIN_SLICE; last_shot = SH_SLICE;
    } else if (pad & PAD_UP) {                             /* à plat : tendue, rapide, risquée */
        depth = 78; speed = (u8)(54 + power); grav = 4; spread += 7; last_shot = SH_FLAT; flat = 1;
    } else if (pad & PAD_DOWN) {                           /* croisé court : en angle, carré de service */
        /* version 2.60 : l'armé ferme l'angle (relâché vite : 14 px de la
           ligne de côté, prudent ; bouton tenu : 6 px, dur à rattraper mais
           risqué) plutôt que d'accélérer ; il ajoute peu d'imprécision */
        T = (u8)(power > 24 ? 24 : power);
        depth = 30; speed = (u8)(34 + (T >> 2)); grav = 9; bspin = SPIN_TOP; spread += 3; last_shot = SH_SHORT;
        hit_margin = (u8)(14 - T / 3);
        power = (u8)(T >> 1);
    } else {                                               /* lifté */
        /* version 2.60 : relâché aussitôt, lifté plus court et sûr ; armé,
           plus de lift (trajectoire plus bombée, rebond plus haut) */
        depth = power < 4 ? 62 : 70; speed = (u8)(40 + power); grav = (u8)(7 + (power >> 3)); bspin = SPIN_TOP; last_shot = SH_DRIVE;
    }
    if (volley && !smash) {
        if (last_shot == SH_DROP) depth = (s16)(depth - 6);      /* volée de toucher : meurt près du filet */
        else if (last_shot == SH_SHORT) depth = (s16)(depth - 2); /* volée croisée courte : franche, en angle (version 2.61) */
        else depth = (s16)(depth - 10);
        T = (u8)(flat && power >= 16 ? 52 : last_shot == SH_SHORT ? 48 : 42);   /* volée à plat appuyée, volée croisée : plus vives */
        if (speed > T) speed = T;
        if (last_shot == SH_DRIVE || last_shot == SH_FLAT) last_shot = SH_VOLLEY;
    }
    /* frappe parfaite (lifté, à plat, coupé, croisé court, lob ; banque fixe) :
       +37,5 % de vitesse, presque sans écart */
    hit_perf = perfect_hit(p, sh_dx);
    if (hit_perf && last_shot == SH_LOB) { depth += 4; spread = 1; power = 0; }   /* lob parfait : précis, profond */
    else if (hit_perf) { speed = (u8)(speed + (speed >> 2) + (speed >> 3)); spread = 1; power = 0; }
    /* fatigue (long échange) : faute de jambes, on frappe plus fort pour
       conclure, mais moins précis : jusqu'à +23 % de vitesse et + 5 d'écart
       (tired 0..10) ; la frappe parfaite reste précise */
    else if (tired) { speed = (u8)(speed + (u8)(((u16)speed * (u8)(tired * 3)) >> 7)); spread = (u8)(spread + (tired >> 1)); }
    sh_power = power;
    sh_depth = depth; sh_speed = speed; sh_grav = grav; sh_spread = spread;
}

/* ---------------------------------------------------------------------- */
/* Plongeon (version 2.50), appelé par le moteur (banque 0) via bank_call.
 * Entrées : sh_p (joueur), sh_pad (0 : joueur humain, au lâcher du bouton ;
 * $FF : joueur de l'ordinateur). Sortie : sh_short = 1 si le plongeon part.
 * La balle est suivie trame par trame jusqu'à ce qu'elle soit frappable en
 * profondeur (comme can_hit) : écart d (px) et nombre de trames n. La
 * frappe est possible pendant l'élan, le vol et 3 trames au sol (12 trames). Le plongeon (poses.s) avance de 3 px par
 * trame après 2 trames d'élan, jusqu'à 18 px : la raquette atteint alors
 * 24 + 3 (n - 2) px, 42 au plus.
 *  - humain : au lâcher du bouton (la frappe), sans direction à tenir :
 *    s'il est bien placé, frappe normale ; si la balle passera trop loin
 *    sur le côté (au-delà de sa portée, 45 px au plus) et arrive dans 12
 *    trames au plus, il plonge ;
 *  - ordinateur : jamais sur une balle qui va sortir ; il plonge quand il
 *    n'arriverait pas à temps en courant
 *    (course en diagonale, partie en retard : 3/4 de sa vitesse), mais
 *    assez tôt pour toucher la balle ; souvent en Expert, parfois en
 *    Normal, rarement en Facile. */
/* ---------------------------------------------------------------------- */
extern u8 dive_t[4], dive_f[4], pl_state[4], pl_moving[4];
#pragma static-locals(off)          /* RAM interne pleine : variables sur la pile */
void dive_try(void)
{
    static const u8 ai_spd[3] = { 74, 86, 110 };     /* comme ai_speed (game.c) */
    static const u8 dive_p[3] = { 40, 120, 220 };
    /* boucle de prédiction en page zéro (variables register : rapides et
       sans RAM fixe) ; appelée à chaque trame pour l'ordinateur quand la
       balle arrive loin de lui, elle coûtait près de 4 000 cycles sur la
       pile C (version 2.59) */
    register s16 x, y, py;
    u8 p = sh_p, n, lv, ok = 0;
    s16 d, a, run, reach;
    sh_short = 0;
    if (ms != MS_RALLY || !b_live || hitter == (u8)(p & 1) || bounces > 1 || dive_t[p] || pl_jump[p]) return;
    if ((b_serve && bounces == 0) || bz > 36 * FP) return;
    if (sh_pad == 0xFF && ((frame_cnt ^ p) & (npl == 4 ? 3 : 1))) return;   /* ordinateur : une trame sur deux (sur quatre en double) */
    x = bx; y = by; py = ply[p];
    /* balle encore loin en profondeur (plus de 13 trames à 4 px) : rien à
       décider, sans parcourir la boucle */
    if (y > py + 56 * FP || y < py - 60 * FP) return;
    for (n = 0; ; ++n) {
        if (n > 12) return;
        if (y > py - 14 * FP && y < py + 9 * FP) break; /* frappable en profondeur (can_hit) */
        x += bvx; y += bvy;
    }
    d = PX(x) - PX(plx[p]);
    a = d < 0 ? -d : d;
    if (sh_pad != 0xFF) {
        /* humain, au lâcher du bouton : bien placé (la balle passera à portée,
           en comptant le pas glissé pendant le geste), frappe normale ; trop
           loin (jusqu'à 45 px), plongeon */
        ok = (u8)(a > (s16)(24 + (n >> 1)) && a < 46);
    } else {
        lv = cfg.level[p];
        /* balle qui va sortir (pas encore rebondi) : on la laisse */
        if (bounces == 0) {
            s16 l = cfg.doubles ? DBL_L : SGL_L, r = cfg.doubles ? DBL_R : SGL_R;
            if (land_x < l || land_x > r || land_y < TOP_BASE || land_y > BOT_BASE) return;
        }
        run = (s16)(22 + (((u16)ai_spd[lv] * n * 3) >> 8));
        reach = (s16)(n > 2 ? 24 + (n - 2) * 3 : 24);
        if (reach > 42) reach = 42;
        ok = (u8)(a > run && a <= reach && rand8() < dive_p[lv]);
    }
    if (!ok) return;
    dive_t[p] = 42;
    dive_f[p] = (u8)(d < 0);
    pl_state[p] = 0;                /* ST_IDLE */
    pl_moving[p] = 0;
    sfx_swing();
    sh_short = 1;
}
#pragma static-locals(on)
