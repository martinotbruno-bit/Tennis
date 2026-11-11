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
u8 perfect_hit(u8 p, s16 dx);
s16 iabs(s16 v);

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
            else if (pad & PAD_DOWN) { depth = 34; sh_short = 1; spread += 2; }   /* angle court */
            speed = (u8)(speed + (power >> 1));                      /* armé : plus fort... */
            spread = (u8)(spread + (power >> 2));                    /* ...et plus risqué */
        }
        sfx_crowd(5);
    } else if (sh_btn == PAD_B && (pad & PAD_UP)) {        /* lob */
        depth = 74; speed = 24; grav = 6; bspin = SPIN_TOP; last_shot = SH_LOB;
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
        depth = 30; speed = (u8)(34 + (power >> 1)); grav = 9; bspin = SPIN_TOP; spread += 3; last_shot = SH_SHORT;
    } else {                                               /* lifté */
        depth = 70; speed = (u8)(40 + power); grav = 7; bspin = SPIN_TOP; last_shot = SH_DRIVE;
    }
    if (volley && !smash) {
        if (last_shot == SH_DROP) depth = (s16)(depth - 6);      /* volée de toucher : meurt près du filet */
        else depth = (s16)(depth - 10);
        T = (u8)(flat && power >= 16 ? 52 : 42);                 /* volée à plat appuyée : plus vive */
        if (speed > T) speed = T;
        if (last_shot == SH_DRIVE || last_shot == SH_FLAT) last_shot = SH_VOLLEY;
    }
    /* frappe parfaite (lifté, à plat, coupé, croisé court ; banque fixe) :
       +37,5 % de vitesse, presque sans écart */
    hit_perf = perfect_hit(p, sh_dx);
    if (hit_perf) { speed = (u8)(speed + (speed >> 2) + (speed >> 3)); spread = 1; power = 0; }
    /* fatigue (long échange) : faute de jambes, on frappe plus fort pour
       conclure, mais moins précis : jusqu'à +23 % de vitesse et + 5 d'écart
       (tired 0..10) ; la frappe parfaite reste précise */
    else if (tired) { speed = (u8)(speed + (u8)(((u16)speed * (u8)(tired * 3)) >> 7)); spread = (u8)(spread + (tired >> 1)); }
    sh_power = power;
    sh_depth = depth; sh_speed = speed; sh_grav = grav; sh_spread = spread;
}
