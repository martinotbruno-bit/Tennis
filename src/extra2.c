/* Royal Martinot — fonctions de match, sorties des
   banques 15 et 20 pleines (banque 30) : balle faute renvoyée, annonce de
   l'ace, balle limite (zoom rapide), récapitulatif du score. */
#include <string.h>
#include "game_int.h"
#include "app.h"
#include "rules.h"
#include "court.h"
#include "gfx.h"
#include "audio.h"
#include "assets.h"

#define side_of(t) ((u8)(((t) & 1) ^ sc_swap))
extern u8 sc_swap;
void set_msg(const char *a, const char *b, u8 t);   /* banque fixe */

/* ---------------------------------------------------------------------- */
/* Balle faute renvoyée : après un service faux ou une balle dehors */
/* (le point est déjà joué, MS_DEAD), le joueur humain qui a armé son coup */
/* et a la balle à portée la renvoie quand même, mollement, vers l'autre   */
/* camp, comme on le fait sur un vrai court. L'échange reste terminé : la  */
/* balle n'est plus en jeu (b_live = 0), ses rebonds ne comptent pas.      */
/* Appelée par human_update (game.c) ; 1 si la balle part.                 */
/* ---------------------------------------------------------------------- */
void launch(s16 tx, s16 ty, u8 T, u8 grav);   /* gamefix.c */
extern u8 end_reason;
u16 __fastcall__ dead_hit(u16 pp)
{
    u8 p = (u8)pp, top;
    s16 d;
    if (!b_vis || end_reason == END_LET || bz > 40 * FP) return 0;
    top = (u8)(PX(ply[p]) < NET_Y);
    if ((u8)(PX(by) < NET_Y) != top) return 0;            /* balle dans son camp */
    if (top ? bvy >= 0 : bvy <= 0) return 0;              /* et qui vient vers lui (pas celle qu'il vient de renvoyer) */
    d = PX(by) - PX(ply[p]); if (d < -15 || d > 10) return 0;
    d = PX(bx) - PX(plx[p]); if (d < -24 || d > 24) return 0;
    bspin = 0;
    launch(CENTER_X + (s16)(rand8() & 31) - 16, top ? NET_Y + 70 : NET_Y - 70, 44, 7);
    b_live = 0;                                           /* (hors jeu) */
    sfx_hit(0);
    if (ms_timer < 45) ms_timer = 45;                     /* (le temps de la voir partir) */
    return 1;
}

/* annonce « ACE ! » et vitesse du service (déplacée de highlight.c,
   banque 15 pleine) */
extern u8 hl_spd;
static char ace_b[10];
void hl_ace_msg(void)
{
    u16 k = (u16)hl_spd + 80;
    ace_b[0] = (char)('0' + k / 100); ace_b[1] = (char)('0' + (k / 10) % 10); ace_b[2] = (char)('0' + k % 10);
    ace_b[3] = ' '; ace_b[4] = 'K'; ace_b[5] = 'M'; ace_b[6] = '/'; ace_b[7] = 'H'; ace_b[8] = 0;
    set_msg("ACE !", ace_b, 90);
}

/* déplacés de highlight.c (banque 15 pleine) : balle limite, puis
   récapitulatif du score */
extern u8 hl_bx, hl_by, hl_bin, hl_bok, hl_sok, hl_sbok, hl_sbi, hl_sbx, hl_sby, hl_sbin, hl_bi, hl_quick, rp_pending;
extern u16 hl_bf, hl_sbf;
/* balle dehors de peu : le point s'arrête sur une balle d'échange
   tombée à 3 px de la ligne ou moins ; ralenti court (12 trames avant le
   rebond, rp_start), sans gros plan, droit au zoom x 2 sur l'impact
   (« DEHORS ! », 50 trames au lieu de 80), puis fin du ralenti. Appelé par
   point_over (faute adverse). */
extern u8 hit_jump;
extern u8 b_serve;
#pragma static-locals(push, off)        /* (pile : la RAM de travail est pleine) */
/* balle bonne sur la ligne ou à 2 px à l'intérieur du point où elle la
   touche encore : distance à la ligne la plus proche, 4 px au plus */
static u8 near_in(s16 x, s16 y)
{
    s16 d, l = cfg.doubles ? IN_DL : IN_SL, r = cfg.doubles ? IN_DR : IN_SR;
    d = x - l; if (r - x < d) d = r - x;
    if (y - IN_TOP < d) d = y - IN_TOP;
    if (IN_BOT - y < d) d = IN_BOT - y;
    return (u8)(d <= 4);
}
void hl_out(void)
{
    s16 x = hl_bx, y = hl_by, d = 0, l, r;
    if (!hl_bok || b_serve || !cfg.replays || cfg.demo) return;   /* (prioritaire sur le ralenti complet) */
    l = cfg.doubles ? IN_DL : IN_SL; r = cfg.doubles ? IN_DR : IN_SR;   /* (ligne comprise) */
    if (hl_bin) {
        if (!near_in(x, y)) return;
    } else {
        if (x < l) d = l - x; else if (x > r) d = x - r;
        if (y < IN_TOP) d += IN_TOP - y; else if (y > IN_BOT) d += y - IN_BOT;
        if (d == 0 || d > 3) return;
    }
    hl_sok = 0;                         /* pas de gros plan */
    hl_sbok = 1; hl_sbi = hl_bi; hl_sbx = hl_bx; hl_sby = hl_by; hl_sbin = hl_bin; hl_sbf = hl_bf;
    hl_quick = 1; rp_pending = 1;
}
#pragma static-locals(pop)

extern u8 recap_pend;
void team_name(u8 t, char *out);   /* gamefix.c */
void score_line(u8 t, char *out)
{
    u8 i, k = 4;
    team_name(t, out);
    for (i = 0; i < sc_set_no && i < MAX_SETS; ++i) { out[k++] = ' '; out[k++] = (char)('0' + sc_hist[i][t]); }
    if (!sc_over && k < 14) { out[k++] = ' '; out[k++] = (char)('0' + sc_games[t]); }
    out[k] = 0;                     /* les deux lignes ont la même longueur : centrées ensemble */
}

/* bandeau : le score set par set pendant 2 secondes */
static char recap_a[16], recap_b[16];
void score_recap(void)
{
    score_line(0, recap_a);
    score_line(1, recap_b);
    set_msg(recap_a, recap_b, 120);
    recap_pend = 0;
}


u16 __fastcall__ near_in_f(u16 xy) { return near_in((u8)xy, (u8)(xy >> 8)); }

/* trace de la balle des zooms (highlight.c) : l'empreinte exacte de
   la balle jugée, disque de 2 px du court ; voir le commentaire de
   highlight.c */
#pragma static-locals(push, off)
/* tuiles précalculées (contour sombre de 1 px, intérieur plein de la
   couleur de la balle, la trace évidée semblait une balle vide ; disque de rayon 4 z px, 16 x 16 px, haut gauche,
   haut droit, bas gauche, bas droit) : x 2 puis x 4 */
static const u8 mk_t2[64] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x03, 0x04, 0x08, 0x08,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xE0, 0xE0, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x20, 0x10, 0x10,
    0x07, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x04, 0x03, 0x00, 0x00, 0x00, 0x00,
    0xE0, 0xE0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x20, 0xC0, 0x00, 0x00, 0x00, 0x00 };
static const u8 mk_t4[64] = {
    0x00, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0x7F, 0x7F, 0x07, 0x18, 0x30, 0x60, 0x40, 0x80, 0x80, 0x80,
    0x00, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFE, 0xFE, 0xE0, 0x18, 0x0C, 0x06, 0x02, 0x01, 0x01, 0x01,
    0x7F, 0x7F, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x00, 0x80, 0x80, 0x80, 0x40, 0x60, 0x30, 0x18, 0x07,
    0xFE, 0xFE, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0x00, 0x01, 0x01, 0x01, 0x02, 0x06, 0x0C, 0x18, 0xE0 };
u16 __fastcall__ mark_build_f(u16 adz)
{
    const u8 *p = (adz & 15) == 4 ? mk_t4 : mk_t2;
    u8 i;
    vram_adr((u16)(adz & 0xFFF0));
    for (i = 0; i < 64; ++i) vram_put(p[i]);
    return 0;
}
#pragma static-locals(pop)

/* gros plans HD (cut_build, highlight.c) : décor harmonieux. La trame
   du fond de l'image (tuile $F8) couvre tout l'écran : elle remplace la tuile
   du fond uni ($FB) et, là où la palette est celle du décor, la tuile vide
   ($00) : avant, elle ne se voyait que dans le rectangle de l'image. Plan moyen (z = 0) : la bande du sol (rangées 27 à
   29, tuile $FC) continue sous le joueur, là où la palette est celle du décor
   (palette 0). Écran coupé. */
#define PPU_DATA2 (*(volatile u8 *)0x2007)
#pragma static-locals(push, off)
u16 __fastcall__ hd_backdrop_f(u16 z)
{
    u8 b[32], at[64], i, r, d, t;
    vram_adr(0xF8 << 4); d = PPU_DATA2; for (i = 0; i < 16; ++i) b[i] = PPU_DATA2;
    vram_adr(0xFB << 4); for (i = 0; i < 16; ++i) vram_put(b[i]);
    vram_adr(0x23C0); d = PPU_DATA2; for (i = 0; i < 64; ++i) at[i] = PPU_DATA2;
    for (r = 0; r < 30; ++r) {
        vram_adr(0x2000 + r * 32); d = PPU_DATA2; for (i = 0; i < 32; ++i) b[i] = PPU_DATA2;
        for (i = 0; i < 32; ++i) {
            if ((at[(r >> 2) * 8 + (i >> 2)] >> (((r & 2) << 1) | (i & 2))) & 3) continue;   /* (palette du joueur : inchangé) */
            t = b[i];
            if (!z && r >= 27) { if (t == 0 || t == 0xF8 || t == 0xFB) b[i] = 0xFC; }   /* sol */
            else if (t == 0) b[i] = 0xF8;                                            /* trame du décor */
        }
        vram_adr(0x2000 + r * 32); for (i = 0; i < 32; ++i) vram_put(b[i]);
    }
    return 0;
}
#pragma static-locals(pop)

/* ---------------------------------------------------------------------- */
/* Lieux : ce qui les distingue en match, en plus du décor dessiné.        */
/* - palettes (venue_pal_f, après pal_set_court) : au court Margaux, le    */
/*   court reste rose mais d'une teinte propre à chaque surface ; en salle, */
/*   la « nuit » devient une soirée : court éclairé comme de jour, murs     */
/*   sombres, panneaux allumés ;                                           */
/* - décor animé (decor_tick, chaque trame du match) : au château, les     */
/*   flammes des torches et les bannières vacillent ; au club et au parc,  */
/*   (2.83 : plus de rafale sur le feuillage) ; en salle, les panneaux s'allument */
/*   quand le public réagit ;                                              */
/* - feuilles d'automne qui tombent sur les côtés du parc (decor_draw) ;   */
/* - ambiance sonore sur le 2e canal carré, libre pendant le match (pas de */
/*   musique) : oiseaux au club et au parc, fanfare de trompettes au       */
/*   château (changement de côté, fin du match) ; au stade, la clameur se  */
/*   prolonge ; au club, au parc et au                                     */
/*   château, des applaudissements polis suivent les beaux points.         */
/* ---------------------------------------------------------------------- */
extern u8 cer, hl_mode, sun_st;
extern u8 band_hold;
#pragma zpsym("band_hold")
extern u8 ola_on;
#define CER_CHAIR 4
#pragma bss-name(push, "ZEROPAGE")
static u8 dk_t, dk_last, dk_base, dk_fl, dk_pc, dk_snd, dk_sp, dk_pcer, dk_pms, dk_cr, dk_fan, dk_h, dk_fx, dk_on;
#pragma bss-name(pop)
#pragma zpsym("dk_h")
#pragma zpsym("dk_fx")
#pragma zpsym("dk_on")
#pragma zpsym("dk_t")
#pragma zpsym("dk_last")
#pragma zpsym("dk_base")
#pragma zpsym("dk_fl")
#pragma zpsym("dk_pc")
#pragma zpsym("dk_snd")
#pragma zpsym("dk_sp")
#pragma zpsym("dk_pcer")
#pragma zpsym("dk_pms")
#pragma zpsym("dk_cr")
#pragma zpsym("dk_fan")

#pragma static-locals(push, off)
static const u8 mg_court[6] = { 0x15, 0x24, 0x25, 0x05, 0x35, 0x34 };   /* court Margaux : terre (rose foncé), gazon (fuchsia), dur (rose), moquette (bordeaux), plage (rose pâle), glace (lilas) */
#include "assets_decor.h"
u16 __fastcall__ venue_pal_f(u16 cw)
{
    u8 c, i, cond = (u8)cw;
    const u8 *src;
    /* (2.80) petites animations : 2 tuiles propres au lieu, écrites au
       chargement du court (bit 8 : rendu coupé, game.c court_load) à la
       place de la goutte (4) et de l'éclaboussure (8), inutilisées sans
       pluie ; rétablies à la sortie du match (gfx.c chr_borrow_restore) */
    dk_on = (u8)(cond != COND_RAIN);
    if ((cw & 0x100) && dk_on) {
        src = dk_tiles + (u16)cfg.env * 32;
        vram_adr(0x1000 + (4 << 4)); for (i = 0; i < 16; ++i) vram_put(src[i]);
        vram_adr(0x1000 + (8 << 4)); for (i = 16; i < 32; ++i) vram_put(src[i]);
    }
    if (cfg.env == ENV_MARGAUX) {
        c = mg_court[cfg.surface < 6 ? cfg.surface : 2];
        if (cond == COND_RAIN && c >= 0x10) c -= 0x10;
        pal_buf[1] = c;
    }
    if (cfg.env == ENV_PARK && cond != COND_RAIN) pal_buf[16 + 14] = 0x27;   /* feuilles d'automne orangées (couleur de la pluie et de la sueur) */
    if (cfg.env == ENV_HALL && cond == COND_NIGHT) {
        pal_set_court(cfg.surface, COND_DAY, ENV_HALL);
        pal_buf[9] = 0x2D; pal_buf[10] = 0x3A; pal_buf[11] = 0x0F;   /* murs sombres, panneaux allumés */
    }
    dk_t = dk_fl = dk_snd = dk_cr = dk_fan = dk_h = dk_fx = 0; dk_last = 0xFF;
    dk_pc = crowd_timer; dk_pcer = cer; dk_pms = ms;
    pal_dirty = 1;
    return 0;
}

/* 2e canal carré : vd = rapport cyclique et volume, sw = balayage */
static void p2(u8 vd, u8 sw, u16 per)
{
    APU(0x4004) = vd;
    APU(0x4005) = sw;
    APU(0x4006) = (u8)per;
    APU(0x4007) = (u8)(0x08 | (per >> 8));
}
static const u16 fan_per[4] = { 284, 213, 169, 142 };   /* sol, do, mi, sol : fanfare */
static void snd_step(void)
{
    u8 s = dk_sp++;
    switch (dk_snd) {
    case 1:     /* oiseaux : trois pépiements montants */
        if (s == 0 || s == 6 || s == 12) p2(0x94, 0x9A, (u16)(56 - s));
        else if (s == 4 || s == 10 || s == 16) APU(0x4004) = 0x30;
        if (s == 16) dk_snd = 0;
        break;
    case 2:     /* fanfare de trompettes : sol do mi sol */
        if ((s & 7) == 0 && s < 32) p2((u8)(s < 24 ? 0x96 : 0x98), 0x08, fan_per[s >> 3]);
        else if (s == 30 || s == 46) { APU(0x4004) = 0x30; if (s == 46) dk_snd = 0; }
        else if (s > 32 && (s & 3) == 0) APU(0x4004) = (u8)(0x98 - ((s - 32) >> 2));   /* la dernière note s'éteint */
        break;
    }
}
static void snd_start(u8 k) { if (!dk_snd) { dk_snd = k; dk_sp = 0; } }

extern u8 noise_hold, noise_pri;
void decor_tick(void)
{
    static const u8 flick[16] = { 0, 0, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0 };
    u8 e = cfg.env, idx = 0, v, up;
    if (hl_mode || paused) { if (dk_snd) { APU(0x4004) = 0x30; dk_snd = 0; } return; }
    /* (pendant l'échange, decor_tick n'est pas appelée ; dès la mise en
       place du service, la couleur du décor reprend sa valeur et le son
       s'arrête : les animations n'ont lieu qu'à la fin du point, tant que
       le bandeau du score est affiché, et pendant les pauses - changement
       de côté, cérémonies) */
    if ((ms == MS_SERVE && !cer && !band_hold) || ms == MS_TOSS) {
        if (dk_last != 0xFF && pal_buf[e == ENV_CASTLE ? 11 : 10] == dk_last && dk_last != dk_base) { pal_buf[e == ENV_CASTLE ? 11 : 10] = dk_base; pal_dirty = 1; }
        dk_last = 0xFF;
        if (dk_snd) { APU(0x4004) = 0x30; dk_snd = 0; }
        return;
    }
    if (++dk_t == 0) ++dk_h;
    up = (u8)(crowd_timer > dk_pc);           /* le public vient de réagir */
    dk_pc = crowd_timer;
    if (dk_fx) --dk_fx;
    if (up && (e == ENV_STADIUM || e == ENV_HALL)) dk_fx = 120;   /* (2.80) flashs des photographes */
    /* décor animé : une couleur du décor (palette 2) qui varie */
    if (e == ENV_CASTLE || e == ENV_MARGAUX) idx = 11;
    else if (e == ENV_HALL) idx = 10;   /* (2.83 : plus de rafale sur le feuillage du club et du parc, elle ressemblait à un défaut d'affichage) */
    if (idx) {
        if (pal_buf[idx] != dk_last) dk_base = pal_buf[idx];   /* (palette refaite : nuit, crépuscule, pluie) */
        v = dk_base;
        if (e == ENV_CASTLE) { if (flick[(dk_t >> 2) & 15] && v < 0x30) v += 0x10; }
        else if (e == ENV_MARGAUX) { static const u8 twk[8] = { 0, 0, 0x38, 0, 0, 0x3C, 0, 0x37 }; if (twk[(dk_t >> 3) & 7]) v = twk[(dk_t >> 3) & 7]; }   /* (2.80) étoiles qui scintillent */
        else if (e == ENV_HALL) {
            if (up) dk_fl = 40;
            if (dk_fl) { --dk_fl; if ((dk_fl & 8) && v < 0x30) v += 0x10; }
        }
        if (v != pal_buf[idx]) { pal_buf[idx] = v; pal_dirty = 1; }
        dk_last = v;
    }
    /* ambiance sonore ; château : fanfare au changement de côté et à la fin
       du match, jouée dès que la petite musique de fin de jeu est finie */
    if (e == ENV_CASTLE && ((cer == CER_CHAIR && dk_pcer != CER_CHAIR) || (ms == MS_OVER && dk_pms != MS_OVER))) dk_fan = 1;
    dk_pcer = cer; dk_pms = ms;
    if (snd_mode == 2 || !music_none() || cfg.mode != GM_MATCH) return;
    if (dk_fan) { dk_fan = 0; dk_snd = 0; snd_start(2); }
    if (dk_snd) snd_step();
    if (e == ENV_STADIUM) { if (up) dk_cr = 28; if (dk_cr && --dk_cr == 0 && !noise_hold) sfx_crowd(6); }   /* clameur prolongée (2.90 : pas de relance par-dessus une clameur en cours) */
    if (e == ENV_CLUB || e == ENV_PARK || e == ENV_CASTLE || e == ENV_NUDE) {
        /* (2.88) applaudissements polis, seulement après un beau point (ace,
           échange d'au moins 6 frappes) : un crépitement d'environ une
           seconde, de salves de bruit très brèves à intervalles et hauteurs
           irréguliers, qui s'éteint doucement (avant : trois claps réguliers,
           « tac tac tac », à presque chaque point) */
        if (up && (rally_hits == 1 || rally_hits >= 6)) dk_cr = 60;
        if (dk_cr) {
            static const u8 cr_pat[16] = { 3, 0, 1, 4, 0, 2, 0, 3, 1, 0, 4, 2, 0, 1, 3, 0 };
            v = cr_pat[(u8)(dk_cr + (dk_t >> 2)) & 15];
            --dk_cr;
            if (v && !(noise_hold && noise_pri > 2)) {
                noise_pri = 2; noise_hold = 2;
                APU(0x400C) = (u8)(0x10 | (2 + (dk_cr >> 3)));   /* volume fixe 2 à 9, décroissant */
                APU(0x400E) = v;
                APU(0x400F) = 0x18;                              /* durée : une trame */
            }
        }
    }
    if ((e == ENV_PARK || e == ENV_NUDE || e == ENV_CLUB) && !(dk_h & 1) && ms != MS_RALLY && dk_t == 0x70) snd_start(1);   /* oiseaux entre les points, une fois sur deux, environ toutes les 8 secondes (2.90) ; club : pendant leur vol, decor_draw */
    /* (2.86 : plus de double bip des panneaux en salle, entêtant à chaque point) */
}

/* Rebond de la balle en match (2.86), en trois parties :
   - souffle de bruit (période selon la surface) : terre sourde, gazon doux,
     dur sec, moquette mate, sable grave et court, glace claquante ;
   - volume selon la force du rebond (f, 0 à 3, game.c) : une amortie ou une
     balle qui retombe doucement rebondit faiblement (volume fixe 4, 7 ou
     11, durée selon la surface), un coup fort claque (enveloppe complète) ;
   - « poc » : note grave très brève au triangle (libre pendant le match,
     sans musique), plus grave et sourde sur terre et gazon, plus claire sur
     dur, très grave et étouffée sur le sable (la balle s'enfonce), aiguë et
     brève sur glace ; plus courte si le rebond est faible.
   (2.87 : glace et sable revus ; le bruit périodique de la glace, qui
   tintait, et le long souffle du sable sans poc ne convainquaient pas) */
extern u8 noise_hold, noise_pri, snd_mode;
extern const u8 *trk_ptr[4];
static const u8 bnc_per[6] = { 0x0E, 0x0D, 0x0A, 0x0C, 0x0F, 0x07 };
static const u8 bnc_len[6] = { 0x00, 0x48, 0x28, 0x48, 0x48, 0x28 };   /* rebond faible : durée (compteur de longueur) */
static const u16 bnc_tri[6] = { 470, 540, 330, 420, 760, 210 };        /* poc : période du triangle */
static const u8 bnc_lin[6] = { 4, 3, 3, 4, 3, 2 };                     /* poc : durée (quarts de trame ; 2.94 : plus court, le triangle n'a pas de volume) */
static const u8 bnc_vol[4] = { 0x13, 0x14, 0x16, 0x18 };                /* volume fixe 3, 4, 6, 8 (2.94-2.95 : le rebond couvrait l'impact ; mesures sur le son audible, au-dessus de 150 Hz) */
u16 __fastcall__ bounce_f(u16 fw)
{
    u8 s = cfg.surface < 6 ? cfg.surface : 2, n, f = (u8)fw;
    u16 p;
    if (f > 3) f = 3;
    if (!(noise_hold && noise_pri > 1)) {
        noise_pri = 1; noise_hold = 12;   /* (toute la durée du souffle : les pas ne le coupent plus) */
        APU(0x400C) = bnc_vol[f];
        APU(0x400E) = bnc_per[s];
        APU(0x400F) = bnc_len[s];
    }
    p = bnc_tri[s];
    if (p && !trk_ptr[1] && snd_mode != 2) {
        n = bnc_lin[s];
        if (f < 2) n = (u8)(n - 1 - (f == 0 && n > 2));
        APU(0x4008) = n;
        APU(0x400A) = (u8)p;
        APU(0x400B) = (u8)(0x08 | (p >> 8));
    }
    return 0;
}

/* Impact balle-raquette en match (2.93) : un échantillon réaliste selon le
   coup (k : SH_*, 8 = service), joué seul (tools/gen_voice.py) :
   « tac » sec et claquant (à plat, smash, service), « ploc » plus rond
   (lifté, coupé, croisé court, volée), « poc » mat (lob, amortie : le « ploc » lu plus lentement) ; un coup
   puissant (pw ≥ 3) est lu un cran plus vite, plus clair. Jamais par-dessus
   une annonce de l'arbitre. (2.91-2.92 : clic au carré 1, « pok » et corps
   au triangle : jugé insatisfaisant) */
extern const u8 dpcm_addr[], dpcm_len[], dpcm_rate[];
static const u8 hs_smp[9] = { 7, 7, 7, 7, 3, 7, 7, 3, 3 };   /* 3 tac, 7 ploc */
extern u8 p1_hold;
static const u8 hs_knk[9] = { 0x80, 0x80, 0xC0, 0xC0, 0x50, 0x80, 0x70, 0x48, 0x50 };   /* toc : période de départ */
static const u8 hs_slow[9] = { 0, 0, 2, 2, 0, 0, 0, 0, 0 }; /* lob, amortie : ploc lu 2 crans plus lentement, « poc » */
u16 __fastcall__ hit_snd_f(u16 w)
{
    u8 pw = (u8)w, k = (u8)(w >> 8), n, r;
    if (snd_mode == 2 || (APU(0x4015) & 0x10)) return 0;
    if (k > 8) k = 0;
    n = hs_smp[k];
    r = (u8)(dpcm_rate[n] - hs_slow[k] + (pw >= 3));
    if (r > 15) r = 15;
    APU(0x4015) = 0x0F;
    APU(0x4010) = r;
    APU(0x4012) = dpcm_addr[n];
    APU(0x4013) = dpcm_len[n];
    APU(0x4015) = 0x1F;
    /* (2.95) présence : le DMC est déjà à son maximum ; s'y ajoutent un
       claquement de bruit très bref (2 trames, volume 11 à 15 selon la
       puissance, seulement si le canal de bruit est libre : il ne coupe ni
       la clameur ni un rebond) et un coup sourd très court au triangle
       (trop bref pour qu'on entende une note) */
    /* « toc » au carré 1 : enveloppe de 15 qui s'éteint en 4 trames,
       hauteur qui chute très vite (balayage) : un choc, pas une note */
    p1_hold = 6;
    APU(0x4000) = 0x81;                 /* 50 %, enveloppe de 8 trames environ */
    APU(0x4001) = 0x83;                 /* balayage vers le grave */
    APU(0x4002) = hs_knk[k];
    APU(0x4003) = 0x08;
    /* le même au carré 2 (libre pendant l'échange : les ambiances ne jouent
       qu'entre les points), un peu désaccordé : son plus épais */
    if (!dk_snd) {
        APU(0x4004) = 0x81;
        APU(0x4005) = 0x83;
        APU(0x4006) = (u8)(hs_knk[k] + 3);
        APU(0x4007) = 0x08;
    }
    if (!noise_hold) {
        APU(0x400C) = (u8)(0x1B + (pw > 4 ? 4 : pw));
        APU(0x400E) = (u8)(n == 3 ? 0x03 : 0x05);
        APU(0x400F) = 0x48;             /* (environ 4 trames) */
    }
    if (!trk_ptr[1]) {
        APU(0x4008) = 6;
        APU(0x400A) = (u8)(n == 3 ? 0x40 : 0x80);
        APU(0x400B) = 0x09;
    }
    return 0;
}

/* Sprites du décor (fin du point, bandeau du score, pauses ; mdraw.c) :
   - parc : feuilles d'automne sur les côtés (l'ellipse des ombres, orange,
     retournée au fil de la chute, et quelques éclats jaunes ; 6 en simple,
     3 en double) et, de temps en temps, un écureuil qui descend en
     zigzag un côté du parc, avec une halte à mi-chemin ;
   - court Margaux : petits cœurs qui montent sur les côtés ;
   - stade et salle : flashs des photographes dans le public, nombreux
     quand le public réagit, un de temps en temps sinon ;
   - club nudiste : papillons qui volettent sur les côtés ;
   - château : un vol de 3 oiseaux en V traverse l'écran de temps en temps.
   (tuiles 4 et 8 : dessins propres au lieu, voir venue_pal_f) */
static const u8 lf_sway[8] = { 0, 1, 2, 3, 3, 2, 1, 0 };
static const u8 sq_hop[4] = { 0, 2, 3, 2 };
static const u8 fl_x[16] = { 6, 236, 14, 228, 20, 242, 8, 232, 16, 240, 4, 230, 18, 236, 10, 244 };
static const u8 fl_y[16] = { 52, 120, 176, 70, 140, 40, 200, 96, 64, 184, 112, 150, 36, 80, 164, 208 };
static const s8 bd_dx[3] = { 0, -12, -12 }, bd_dy[3] = { 0, -6, 6 };
/* (variables en page zéro : les locales sur la pile du C coûtent cher, et
   la trame d'après le point est chargée) */
#pragma bss-name(push, "ZEROPAGE")
static u8 dd_i, dd_n, dd_x, dd_y, dd_t, dd_k, dd_e;
static u16 dd_ph;
static s16 dd_sx;
#pragma bss-name(pop)
#pragma zpsym("dd_i")
#pragma zpsym("dd_n")
#pragma zpsym("dd_x")
#pragma zpsym("dd_y")
#pragma zpsym("dd_t")
#pragma zpsym("dd_k")
#pragma zpsym("dd_e")
#pragma zpsym("dd_ph")
#pragma zpsym("dd_sx")
#define i dd_i
#define n dd_n
#define x dd_x
#define y dd_y
#define t dd_t
#define k dd_k
#define e dd_e
#define ph dd_ph
#define sx dd_sx
void decor_draw(void)
{
    e = cfg.env;
    if (!dk_on || hl_mode) return;
    n = (u8)(npl == 4 ? 3 : 6);
    ph = (u16)(((u16)(dk_h & 1) << 8) | dk_t);      /* (cycle de 512 trames) */
    if (e == ENV_PARK) {
        /* (sans multiplication ni division : la trame d'après le point est chargée) */
        for (i = 0, t = (u8)(frame_cnt >> 1); i < n; ++i, t += 43) {
            y = (u8)(28 + (t < 196 ? t : (u8)(t - 196)));
            x = (u8)((i & 1) ? 226 + (i << 1) : 10 + (i << 1));
            x = (u8)(x + lf_sway[((t >> 3) + i) & 7]);
            if (i == 5) spr1(x, y, 6, 3);                                  /* (éclat jaune) */
            else spr1(x, y, 5, (u8)(3 | ((t & 16) ? 0x40 : 0) | ((t & 32) ? 0x80 : 0)));   /* feuille qui tournoie */
        }
        if (ph < 192) {                                                    /* écureuil */
            t = (u8)ph;
            k = (u8)((t >> 4) & 1);                                        /* sens du zigzag */
            x = (u8)(((dk_h & 2) ? 232 : 8) + ((k ? (t & 15) : 15 - (t & 15)) >> 1));
            if (t >= 96 && t < 128) spr1(x, 132, 4, (u8)(3 | (k ? 0 : 0x40)));   /* (halte, assis) */
            else {
                y = (u8)(36 + (t < 96 ? t : t - 32) - sq_hop[(t >> 1) & 3]);
                spr1(x, y, (u8)((t & 4) ? 4 : 8), (u8)(3 | (k ? 0 : 0x40)));
            }
        }
        return;
    }
    if (e == ENV_MARGAUX) {                                               /* cœurs qui montent */
        for (i = 0, t = (u8)((dk_t >> 1) | (dk_h << 7)); i < n; ++i, t += 43) {
            y = (u8)(220 - (t < 188 ? t : (u8)(t - 188)));
            x = (u8)((i & 1) ? 228 + (i << 1) : 8 + (i << 1));
            spr1((u8)(x + lf_sway[((t >> 3) + i) & 7]), y, (u8)((i & 2) ? 8 : 4), 2);
        }
        return;
    }
    if (e == ENV_STADIUM || e == ENV_HALL) {                              /* flashs des photographes */
        /* (positions en table : pas de multiplication ni de division, la
           trame de l'ola est déjà chargée) */
        if (ola_on) return;                                               /* (après l'ola : sa trame est pleine) */
        n = (u8)(dk_fx ? (npl == 4 ? 2 : 4) : 1);
        for (i = 0; i < n; ++i) {
            t = (u8)(dk_t + (i << 4) + (i << 2));
            k = (u8)(t & 15);
            if (k >= 7 || (!dk_fx && (t & 0x30))) continue;
            t = (u8)(((t >> 4) + (i << 2) + i) & 15);
            spr1(fl_x[t], fl_y[t], (u8)(k < 3 ? 4 : 8), 3);
        }
        return;
    }
    if (e == ENV_NUDE) {                                                  /* papillons */
        n = (u8)(npl == 4 ? 1 : 2);
        for (i = 0; i < n; ++i) {
            t = (u8)(dk_t + (i << 7));
            y = (u8)(48 + (t < 128 ? t : 255 - t) + lf_sway[(t >> 1) & 7]);
            x = (u8)((i ? 230 : 10) + (lf_sway[(t >> 2) & 7] << 1));
            spr1(x, y, (u8)((dk_t & 4) ? 4 : 8), 3);
        }
        return;
    }
    if ((e == ENV_CASTLE || e == ENV_CLUB) && ph < 150) {                 /* vol d'oiseaux (club : petits oiseaux, leur chant au milieu du vol) */
        n = (u8)(npl == 4 ? 2 : 3);
        for (k = 0; k < n; ++k) {
            sx = (s16)((ph << 1) - 16 + bd_dx[k]);
            if (dk_h & 2) sx = (s16)(240 - sx);
            if (sx < 0 || sx > 247) continue;
            spr1((u8)sx, (u8)(60 - (ph >> 3) + bd_dy[k]), (u8)((((dk_t >> 3) + k) & 1) ? 4 : 8), 2);
        }
    }
}
#undef i
#undef n
#undef x
#undef y
#undef t
#undef k
#undef e
#undef ph
#undef sx
#pragma static-locals(pop)

/* ---------------------------------------------------------------------- */
/* Club nudiste (2.79) : les images HD (gros plans, portrait du vainqueur,  */
/* ramasseuse, coupe...) sont montrées, la tenue prend la couleur de la    */
/* peau, et une mosaïque couleur peau (deux tuiles, $F9 et $FA, libres     */
/* pendant les gros plans) couvre le bassin, et la poitrine des joueuses : */
/* une censure comme à la télévision. Rectangles : tools/gen_teaser.py.    */
/* ---------------------------------------------------------------------- */
#include "assets_cens.h"
#pragma static-locals(push, off)
/* mosaïque : blocs de 4 x 4 px, peau éclairée (1) et ombrée (2), deux variantes */
static const u8 mosaic[32] = {
    0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F,   0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0,
    0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF,   0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00 };
void __fastcall__ cens_tiles(u8 t0)
{
    u8 i;
    vram_adr((u16)t0 << 4);
    for (i = 0; i < 32; ++i) vram_put(mosaic[i]);
}
/* couvre un rectangle de la table de noms (adresse du coin), seulement là
   où l'image a une tuile (pas le fond) */
void __fastcall__ cens_rect(u16 ad, u8 w, u8 h, u8 t0, u8 lo, u8 hi)
{
    u8 x, y, t;
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            vram_adr((u16)(ad + x)); t = PPU_DATA2; t = PPU_DATA2;
            if (t >= lo && t <= hi) { vram_adr((u16)(ad + x)); vram_put((u8)(t0 + ((x + y) & 1))); }
        }
        ad += 32;
    }
}
/* image sans vêtements (copie de son descripteur en page zéro) : art | genre << 8 */
#pragma bss-name(push, "ZEROPAGE")
static tzn_t tz_nd;
static u8 cn_art, cn_g;
#pragma bss-name(pop)
#pragma zpsym("tz_nd")
#pragma zpsym("cn_art")
#pragma zpsym("cn_g")
/* (2.80) poignée de main du perdant (joueur humain battu) : visage triste.
   Les cases qui diffèrent de l'image souriante (tools/gen_teaser.py,
   sad_patch) : tuile réécrite, case de l'écran pointée dessus ; rendu
   coupé (highlight.c cut_build). v : 0 homme, 1 femme, 2 Margaux, 3 homme
   nu, 4 femme nue */
#include "assets_sad.h"
static void shake_sad(u8 v)
{
    const u8 *p = sad_pt[v];
    u8 n = *p++, i;
    for (; n; --n, p += 19) {
        vram_adr((u16)p[2] << 4); for (i = 3; i < 19; ++i) vram_put(p[i]);
        vram_adr((u16)(p[0] | (p[1] << 8))); vram_put(p[2]);
    }
}

u16 __fastcall__ nude_art_f(u16 a)
{
    cn_art = (u8)a; cn_g = (u8)((a >> 8) & 1);
    memcpy(&tz_nd, &tz_nude[cn_art][cn_g], sizeof(tzn_t));
    return (u16)&tz_nd;
}
/* gros plan (rendu coupé) : z (gros plan zoomé) ; image : nude_art_f */
u16 __fastcall__ cens_hd_f(u16 a)
{
    /* a : z (bit 0), image (bits 1 à 5), g (bits 6 et 7) ; appelée après
       chaque chargement d'image HD (highlight.c cut_build) */
    const u8 *r = tz_cens[cn_art][cn_g];
    u8 k, z = (u8)(a & 1), g = (u8)((u8)a >> 6);
    if ((u8)((u8)a >> 1 & 31) == SAD_ART && cfg.ctrl[sc_winner] == CTRL_CPU && cfg.ctrl[sc_winner ^ 1] != CTRL_CPU) shake_sad((u8)(cfg.env == ENV_NUDE ? g + 3 : g));   /* (2.80 : joueur humain battu par l'ordinateur, visage triste ; comme sad, highlight.c final_tick) */
    if (cfg.env != ENV_NUDE) return 0;
    cens_tiles(0xF9);
    for (k = 0; k < 4 && r[2]; ++k, r += 4) cens_rect(NTADR(r[0], r[1]), r[2], r[3], 0xF9, 1, 0xF6);
    /* tenue couleur de peau */
    pal_buf[9] = pal_buf[5]; pal_buf[10] = pal_buf[6];
    if (z) pal_buf[15] = pal_buf[9]; else pal_buf[14] = pal_buf[9];
    pal_dirty = 1;
    return 0;
}
#pragma static-locals(pop)

/* Affiche VS au club nudiste : la poitrine des joueuses, en mosaïque
   (tuiles VS_MOS, couleurs 2 et 3 de la palette A : peau, peau ombrée) ;
   a = op << 12 | y << 8 | x << 1 | côté : op 0 rangée y de vs_row (menuart.c),
   op 1 le bloc (x, y) touche-t-il la mosaïque, op 2 tuiles de la mosaïque */
#include "assets_vs.h"
#define VS_MOS 0xFC
extern u8 vs_row[];
static const u8 vs_cr[2][4] = { { VS_FL_CENS }, { VS_FR_CENS } };
#pragma static-locals(push, off)
u16 __fastcall__ vs_cens_f(u16 a)
{
    u8 side = (u8)(a & 1), x = (u8)((a >> 1) & 0x7F), y = (u8)((a >> 8) & 15), op = (u8)(a >> 12), i;
    const u8 *q = vs_cr[side];
    if (op == 2) {
        vram_adr((u16)VS_MOS << 4);
        for (i = 0; i < 32; ++i) vram_put((u8)((i & 8) ? 0xFF : (((i >> 2) ^ (i >> 4)) & 1) ? 0x0F : 0xF0));
        return 0;
    }
    if (cfg.env != ENV_NUDE || !char_female[cfg.chr[side]] || cfg.chr[side] == 16) return 0;
    if (op == 1) return (u8)(x + 1 >= q[0] && x < (u8)(q[0] + q[2]) && y + 1 >= q[1] && y < (u8)(q[1] + q[3]));
    if (y < q[1] || y >= (u8)(q[1] + q[3])) return 0;
    for (x = q[0]; x < (u8)(q[0] + q[2]); ++x) if (vs_row[x]) vs_row[x] = (u8)(VS_MOS + ((x + y) & 1));
    return 0;
}
#pragma static-locals(pop)

/* (déplacé de menuart.c, banque 8 pleine) */
/* Réglages : pastille de la couleur de la surface et vignette du lieu
   (16 x 8, palette de sprite 3 : blanc, couleur du lieu, or), devant leur
   valeur */
extern u8 opt_val[], vis[], nvis;
extern const unsigned char ball_base;
void menu_marks(void)
{
    static const u8 sc[6][3] = { { 0x27, 0x17, 0x37 }, { 0x3A, 0x2A, 0x30 }, { 0x21, 0x11, 0x31 },
                                 { 0x24, 0x14, 0x34 }, { 0x38, 0x28, 0x30 }, { 0x31, 0x21, 0x30 } };
    static const u8 ec[7] = { 0x2A, 0x2A, 0x10, 0x16, 0x35, 0x10, 0x27 };
    u8 k, y, e;
    const u8 *c;
    for (k = 0; k < nvis; ++k) {
        y = (u8)((4 + ((u8)(18 - nvis) >> 1)) * 8 + k * 8 - 1);   /* (liste centrée, comme screens.c) */
        if (vis[k] == 6) {              /* SURFACE */
            c = sc[opt_val[6] < 6 ? opt_val[6] : 0];
            if (pal_buf[25] != c[0]) { pal_buf[25] = c[0]; pal_buf[26] = c[1]; pal_buf[27] = c[2]; pal_dirty = 1; }
            spr1(110, y, (u8)(ball_base + 8), 2);
        }
        if (vis[k] == 7) {              /* LIEU */
            e = (u8)(opt_val[7] < 7 ? opt_val[7] : 0);
            if (pal_buf[30] != ec[e]) { pal_buf[30] = ec[e]; pal_dirty = 1; }
            spr1(104, y, (u8)(0x40 + e * 2), 3);
            spr1(112, y, (u8)(0x41 + e * 2), 3);
        }
    }
}

/* (déplacé de menuart.c, banque 8 pleine) */
#pragma static-locals(push, off)
/* affiche VS : la couleur « sombre » sert aussi aux contours ;
   cheveux clairs (joueur personnalisé) : teinte sombre de la même famille */
static u8 hd_dark(u8 d) { return (u8)(d == 0x37 ? 0x18 : (d == 0x30 || d == 0x10) ? 0x00 : d); }
/* palettes du joueur (couleurs de char_pal) : A et B */
u16 __fastcall__ vs_pal_f(u16 sd)
{
    u8 side = (u8)sd;
    u8 b = (u8)(side * 8), S, O, D;
    pal_set_player(61, cfg.chr[side]);       /* (palette 1 du décor : peau, tenue, sombre) */
    S = pal_buf[5]; O = pal_buf[6]; D = pal_buf[7];
    pal_buf[b + 1] = hd_dark(D);                                      /* (cheveux clairs : contours sombres) */
    pal_buf[b + 2] = (u8)(S >= 0x10 ? S : S + 0x10);                 /* peau (éclairée) */
    pal_buf[b + 6] = (u8)(O >= 0x10 ? O : O + 0x10);                  /* tenue */
    pal_buf[b + 5] = (u8)((D & 0x30) >= 0x20 ? 0x07 : D);             /* B : sombre (ombre du texte : noir si cheveux clairs), tenue, blanc */
    pal_buf[b + 3] = pal_buf[b + 6];                                  /* tenue (aussi dans A : bras croisés) */
    pal_buf[b + 7] = 0x30;
    if (cfg.env == ENV_NUDE) pal_buf[b + 3] = pal_buf[b + 6] = (u8)(pal_buf[b + 2] >= 0x20 ? pal_buf[b + 2] - 0x10 : pal_buf[b + 2]);   /* (club nudiste : la peau, ombrée) */
    if (cfg.chr[side] == 16 || cfg.chr[side] == 19) {   /* Margaux, Papi : A (noir, peau, cheveux clairs), B (noir, peau, tenue) */
        pal_buf[b + 1] = pal_buf[b + 5] = 0x0F;
        pal_buf[b + 3] = D;
        pal_buf[b + 6] = pal_buf[b + 2];
        pal_buf[b + 7] = (u8)(O >= 0x10 ? O : O + 0x10);
    }
    return 0;
}
#pragma static-locals(pop)

/* ---------------------------------------------------------------------- */
/* Écran des réglages (2.96) : grande balle de tennis tramée en fond, en    */
/* bas à droite (tools/gen_menuball.py). mb_draw_f : tuiles et carte, rendu */
/* coupé ; plus de filets dorés (rangées 3 et 22) ; rangées 22-23 sur les   */
/* palettes des lignes (couleur 1 noire). Le texte du menu efface ses cases */
/* avec des espaces : mb_fix_buf_f (tampon VRAM) et mb_fix_txt_f (écriture  */
/* directe) y remettent la tuile de la balle. Tout est rechargé au prochain */
/* écran (bg_dirty = 1, screen_begin).                                      */
/* ---------------------------------------------------------------------- */
#include "assets_mball.h"
#pragma bss-name(push, "ZEROPAGE")
u8 mb_on, mb_n;
const char *mb_s;
static const u8 *mb_p, *mb_q;
static u8 mb_k, mb_i, mb_x0, mb_w, mb_r;
#pragma bss-name(pop)
#pragma static-locals(push, off)
#define MB_PPU (*(volatile u8 *)0x2007)
void mb_draw_f(void)
{
    mb_p = mb_chr;
    for (mb_k = 0; mb_k < MB_NT; mb_k += mb_w) {        /* (suites de cases consécutives : copie rapide) */
        for (mb_w = 1; mb_k + mb_w < MB_NT && mb_id[mb_k + mb_w] == (u8)(mb_id[mb_k] + mb_w); ++mb_w) ;
        chr_copy(BANK_X2, mb_p, (u16)mb_id[mb_k] << 4, (u16)mb_w << 4);
        mb_p += (u16)mb_w << 4;
    }
    for (mb_r = 0, mb_p = mb_map; mb_r < MB_ROWS; ++mb_r) {          /* (carte lue d'un seul passage) */
        mb_x0 = mb_p[0]; mb_w = mb_p[1];
        vram_adr(NTADR(0, MB_ROW0 + mb_r));
        for (mb_i = mb_x0; mb_i; --mb_i) MB_PPU = 0;
        if (mb_w) { tz_bank = BANK_X2; tz_n = mb_w; tz_copy(mb_p + 2); }
        for (mb_i = (u8)(32 - mb_x0 - mb_w); mb_i; --mb_i) MB_PPU = 0;
        mb_p += 2 + mb_w;
    }
    vram_adr(0x23C0 + 5 * 8);                           /* rangées 20 à 23 : palettes 1 (gauche) et 2 (droite) */
    for (mb_i = 0; mb_i < 8; ++mb_i) MB_PPU = (u8)(mb_i < 4 ? 0x55 : 0xAA);
    mb_on = 1;
    bg_dirty = 1;                                       /* (logo et police rechargés en quittant l'écran) */
}
/* (seule la partie de la ligne qui croise la balle est examinée) */
static u8 mb_row(u8 y)                            /* mb_q : tuiles de la rangée y ; 0 hors balle */
{
    if (y < MB_ROW0 || y >= MB_ROW0 + MB_ROWS) return 0;
    mb_p = mb_map + mb_off[y - MB_ROW0];
    mb_x0 = mb_p[0]; mb_w = mb_p[1];
    mb_q = mb_p + 2;
    return 1;
}
u16 __fastcall__ mb_fix_buf_f(u16 xy)
{
    if (!mb_on || !mb_row((u8)(xy >> 8))) return 0;
    mb_k = (u8)((u8)xy - mb_x0);                        /* colonne dans la rangée de la balle */
    mb_r = (u8)(vram_len - mb_n);
    for (mb_i = mb_n; mb_i; --mb_i, ++mb_k, ++mb_r)
        if (mb_k < mb_w && vram_buf[mb_r] == ' ' && mb_q[mb_k]) vram_buf[mb_r] = mb_q[mb_k];
    return 0;
}
u16 __fastcall__ mb_fix_txt_f(u16 xy)
{
    if (!mb_on || !mb_row((u8)(xy >> 8))) return 0;
    mb_k = (u8)((u8)xy - mb_x0);
    for (mb_p = (const u8 *)mb_s; *mb_p; ++mb_p, ++mb_k)
        if (mb_k < mb_w && *mb_p == ' ' && mb_q[mb_k]) { vram_adr(NTADR((u8)(mb_k + mb_x0), (u8)(xy >> 8))); MB_PPU = mb_q[mb_k]; }
    return 0;
}
#pragma static-locals(pop)

/* ---------------------------------------------------------------------- */
/* (2.97) Serveur ordinateur : une fois sur deux, avant de servir, il se   */
/* replace sur la ligne de fond (près du T ou plus écarté que d'habitude, */
/* CENTER_X ± 6 à ± 40) : la balle part d'un autre point vers les mêmes   */
/* cibles (extérieur, T, corps), l'angle du service change.               */
/* ---------------------------------------------------------------------- */
extern u8 pl_moving[4];
#pragma bss-name(push, "BSS")
static s16 srv_tx;
#pragma bss-name(pop)
#pragma static-locals(push, off)
u16 __fastcall__ srv_pick_f(u16 a)
{
    static const s8 srv_dx[4] = { -16, -9, 9, 18 };
    u8 sp = (u8)a, left = (u8)(a >> 8);
    s8 d;
    srv_tx = plx[sp];
    if (cfg.ctrl[sp] == CTRL_CPU && cfg.mode != GM_TRAIN && (rand8() & 1)) {
        d = srv_dx[rand8() & 3];
        srv_tx = (s16)((left ? CENTER_X + 22 + d : CENTER_X - 22 - d) * FP);
    }
    return 0;
}
u16 __fastcall__ srv_walk_f(u16 a)                      /* 1 : encore en chemin (pas chassés, 1 px par trame) */
{
    u8 p = (u8)a;
    if (plx[p] == srv_tx) return 0;
    if (plx[p] < srv_tx) { plx[p] += 64; if (plx[p] > srv_tx) plx[p] = srv_tx; }
    else { plx[p] -= 64; if (plx[p] < srv_tx) plx[p] = srv_tx; }
    pl_moving[p] = 1;
    if (ms_timer < 12) ms_timer = 12;                   /* (le lancer attend un instant, une fois en place) */
    return 1;
}
#pragma static-locals(pop)
