/* Royal Martinot — banque 15 : récapitulatif du score (venu
 * de la banque 0) ; mise en valeur du point pendant le ralenti : gros plan
 * de la frappe décisive et arrêt sur image zoomé au rebond. */
#include "game_int.h"
#include "app.h"
#include "rules.h"
#include "court.h"
#include "gfx.h"
#include "audio.h"

void set_msg(const char *a, const char *b, u8 t);   /* banque fixe */
void team_name(u8 t, char *out);
extern u8 recap_pend;

/* ---------------------------------------------------------------------- */
/* Récapitulatif du score : une ligne par équipe, « ALEX 6 3 2 », sets    */
/* terminés puis jeux du set en cours (14 caractères pour 5 sets).        */
/* ---------------------------------------------------------------------- */
/* ====================================================================== */
/* Mise en valeur du point pendant le ralenti               */
/*                                                                        */
/* Le ralenti (rp_play, banque 3) s'arrête sur deux repères posés pendant */
/* le point (game.c) : la frappe décisive de l'équipe gagnante, puis le    */
/* rebond qui a conclu. hl_mode passe alors à 1 ou 2 et match_tick appelle */
/* hl_tick à chaque trame jusqu'à la fin de la séquence.                   */
/*                                                                        */
/* 1. Gros plan : image figée, flash ; plan moyen en grand (image haute    */
/*    définition de la bande-annonce, aux couleurs du joueur) avec arrivée */
/*    de caméra en travelling, balle qui arrive sur le tamis, impact et    */
/*    traînée, titre du coup ; puis gros plan du visage qui exulte ;       */
/*    retour au court.                                                     */
/* 2. Zoom : réticule qui se resserre sur le point de rebond, flash, puis  */
/*    image x 2 calculée pendant le flash : décor (tuiles de l'écran lues  */
/*    dans la mémoire du PPU et agrandies), joueurs et balle (sprites de   */
/*    la trame figée, agrandis), trace de la balle, BONNE ou DEHORS.       */
/* ====================================================================== */
#include "assets.h"
#include "assets_tz.h"

#define PPU_DATA (*(volatile u8 *)0x2007)
/* repères posés pendant le point (game.c) : pour chaque équipe, sa
   dernière frappe (indice de la trame dans le tampon du ralenti, joueur,
   coup) ; premier rebond de la balle en jeu (indice, position, dedans) ; en
   fin de point, la frappe décisive de l'équipe gagnante et le rebond qui a
   conclu sont recopiés dans hl_s* (lus pendant le ralenti) */
#pragma bss-name(push, "WBSS")
static u8 hl_hi[2], hl_hp[2], hl_hk[2], hl_hv;
static u16 hl_hf[2], hl_sf;
u16 hl_bf, hl_sbf;                 /* rp_seq au rebond ; recopié en fin de point */
u8 hl_bi, hl_bx, hl_by, hl_bin, hl_bok, hl_w;
const char *hl_ptxt;
u8 hl_si, hl_sp, hl_sk, hl_sok, hl_sbi, hl_sbx, hl_sby, hl_sbin, hl_sbok;
char hl_txt[16];
#pragma bss-name(pop)
extern u8 rp_head;
extern u16 rp_seq;
/* frappe : pk = joueur | coup << 8 (bit 15 : service, début du point) */
/* retour plongeant sur un service canon : dive_canon = joueur + 1,
   posé par dive_try (shots.c) ; hl_cdv[t] : la dernière frappe de l'équipe t
   est ce plongeon (gros plan au ralenti s'il gagne le point, hl_select) */
u8 dive_canon;
static u8 hl_cdv[2];
extern u8 rp_pending;
u16 __fastcall__ hl_hit(u16 pk)
{
    u8 p = (u8)pk, k = (u8)(pk >> 8), t = (u8)(p & 1);
    if (k & 0x80) { hl_hv = 0; k &= 0x7F; dive_canon = 0; hl_cdv[0] = hl_cdv[1] = 0; }
    hl_cdv[t] = (u8)(k == 4 && dive_canon == (u8)(p + 1));
    if (hl_cdv[t]) { set_msg("PLONGEON !", "SUR LE CANON", 50); sfx_crowd(5); }
    hl_hi[t] = rp_head; hl_hp[t] = p; hl_hk[t] = k; hl_hf[t] = rp_seq;
    hl_hv |= (u8)(1 << t);
    hl_bok = 0;                     /* nouvelle balle : pas encore de rebond */
    return 0;
}
/* ace : « ACE ! » et la vitesse du service, comme à la télévision :
   environ 140 km/h coupé, 170 à plat, 210 canon (1,375 x la vitesse de la
   balle + 80, à quelques km/h près) */
u8 hl_spd, hl_srv;     /* vitesse du service (puis km/h - 80), joueur au service */
u8 st_vmax[2];         /* service le plus rapide de chaque équipe (km/h - 80, 0 : aucun) */
u8 st_vmaxp[2];        /* ... et le joueur qui l'a servi (record du palmarès) */
extern u8 sp_new;      /* nouveau record de vitesse (stats.c) */
/* à chaque service (game.c) : vitesse convertie en km/h façon télé, et record du match */
void hl_serve(void)
{
    hl_spd = (u8)(hl_spd + (hl_spd >> 2) + (hl_spd >> 3) + (rand8() & 7));
    if (hl_spd > st_vmax[hl_srv & 1]) { st_vmax[hl_srv & 1] = hl_spd; st_vmaxp[hl_srv & 1] = hl_srv; }
}
/* (hl_vmax_row, hl_rec_row : train.c, banque 20) */
/* Crépuscule (CONDITIONS : CREPUSCULE) : la lumière change au fil des
   jeux, en 4 paliers sur environ un match moyen (sets gagnants x jeux par
   set x 1,2 jeux) : 0 plein jour ; 1 soleil bas, lumière chaude (accentuation
   du rouge de la NES : toute l'image, joueurs compris, comme un vrai
   éclairage) ; 2 soleil couchant (rouge et bleu : lumière rose-violette,
   plus sombre) ; 3 nuit, projecteurs allumés (palette du match de nuit,
   sans accentuation). Sur une NES PAL, les bits du rouge et du vert sont
   inversés. Appelé au chargement du court (court_load, après ppu_off qui
   efface l'accentuation) et à la fin de chaque jeu (game.c). */
extern u8 ppu_emph;
#pragma zpsym("ppu_emph")
u8 sun_st;
void sun_game(void);                    /* sunset.c, banque 22 */
void sun_apply(void);
/* ====================================================================== */
/* Bandeau du score façon télévision                                */
/*                                                                        */
/* Entre deux points (temps mort après le point, joueurs assis au         */
/* changement de côté ; pas pendant la mise en place du service, le       */
/* joueur du bas serait caché), les quatre dernières rangées de l'écran   */
/* montrent le score détaillé : serveur (balle), noms, jeux de chaque set */
/* (le set en cours compris), points du jeu. Elles sont dessinées une     */
/* fois pour toutes dans le bas de l'écran 1 (rangées 25 à 28, palette du */
/* bandeau du haut) ; l'interruption de ligne du MMC3 fait passer le PPU  */
/* sur l'écran 1 à partir de la ligne 200 (crt0.s, band_on), le temps que */
/* le bandeau est affiché. Les sprites qui descendraient dans le bandeau  */
/* sont cachés. Filets : tuile $5F (« _ », jamais écrite en match).        */
/* ====================================================================== */
#define BAND_NT   0x2400
/* filet du bandeau : tuile $FF, jamais posée sur le court (vérifié dans tous
   les lieux, à plat et en perspective). Autrefois $5F (« _ »),
   que la vue en perspective emploie comme tuile vide au bord du court : des
   traits horizontaux apparaissaient entre le public et le court. */
#define T_RULE    0xFF
#define T_PCT     0xFE         /* « % » du bandeau, aussi en perspective (où la tuile $25 dessine le court) ; $FE n'est jamais posée sur le court */
#define BAND_ROW  25
#define BAND_LINE 199          /* (valeur de recharge du compteur : réglée sur jsnes et FCEUX) */
#define BAND_WAIT 48           /* délai d'apparition après le point */
#define CER_SEAT  5
#define CER_CHOOSE 11            /* tirage au sort : choix du gagnant (cer.c) */
#define CER_RECAP 13             /* récapitulatif avant le service (cer.c) */
extern u8 toss_q, toss_c;
extern u8 hl_mode, cer;
extern u8 band_hold, crowd_big;
#pragma zpsym("band_hold")
#pragma zpsym("crowd_big")
u8 band_vis, band_rows, band_dis;   /* band_dis : bandeau renvoyé par un appui (jusqu'au point suivant) */      /* band_vis : 0 caché, 1 texte, 2 sprites cachés, 3 affiché */
static u8 band_last[7];
char bb[28];                    /* (2.98 : aussi écrit par band_name_f, extra2.c) */
/* changement de côté : le bandeau alterne le score (page 0) et des
   statistiques du match, une page toutes les 50 trames (joueurs assis
   250 trames en match, cer.c) */
u8 band_pg;
u8 band_t;                             /* (remis à 0 à l'arrivée sur la chaise, cer.c) */
static const char * const band_lbl[5] = { "1ERES BALLES", "ACES", "COUPS GAGNANTS", "FAUTES", "SERVICE MAX KM/H" };   
static void band_num(u8 at, u16 v)
{
    bb[at + 2] = (char)('0' + v % 10);
    if (v >= 10) bb[at + 1] = (char)('0' + (v / 10) % 10);
    if (v >= 100) bb[at] = (char)('0' + v / 100);
}
static void band_init(void)
{
    static const u8 rule[16] = { 0, 0, 0xFF, 0, 0, 0, 0, 0,   0, 0, 0xFF, 0xFF, 0, 0, 0, 0 };   /* trait blanc, ombre noire */
    u8 i;
    vram_adr((u16)T_RULE << 4);
    for (i = 0; i < 16; ++i) vram_put(rule[i]);
    chr_copy(BANK_CHR, chr_data + (0x25 << 4), (u16)T_PCT << 4, 16);   /* « % » de la police */
    vram_adr(BAND_NT + 24 * 32); vram_fill(0, 32);
    vram_put(0); vram_fill(T_RULE, 30); vram_put(0);          /* rangée 25 : filet */
    vram_fill(0, 64);                                       /* rangées 26-27 : texte */
    vram_put(0); vram_fill(T_RULE, 30); vram_put(0);          /* rangée 28 : filet */
    vram_fill(0, 32);
    vram_adr(BAND_NT + 0x3C0 + 6 * 8); vram_fill(0x55, 16);  /* palette 1 (bandeau du haut) */
    band_vis = 0;
}
u16 __fastcall__ band_name_f(u16 a);   /* extra2.c */
static void band_text(u8 t)
{
    u8 i, k;
    const char *q;
    for (i = 0; i < 28; ++i) bb[i] = ' ';
    if (band_pg == 6) {             /* tirage au sort : le choix du gagnant */
        if (toss_q == 3) {          /* récapitulatif : « ALEX SERT EN BAS » / « MAYA RECOIT    A : JOUER » */
            k = (u8)(t ? sc_server ^ 1 : sc_server);
            team_label(cfg.chr[k], cfg.doubles ? cfg.chr[k + 2] : 255, bb);
            q = t ? "RECOIT     A : JOUER" : (((sc_server & 1) ^ sc_swap) ? "SERT EN HAUT" : "SERT EN BAS");
            for (i = 6; *q; ) bb[i++] = *q++;
            return;
        }
        q = t ? (toss_q == 1 ? "A : SERVIR     B : LE COTE" : toss_q == 4 ? "A : SERVIR   B : RECEVOIR" : "A : EN HAUT    B : EN BAS")
              : (toss_q == 1 ? "GAGNE LE TIRAGE" : toss_q == 4 ? "SERT OU RECOIT ?" : "CHOISIT SON COTE");
        if (!t) team_label(cfg.chr[toss_c], cfg.doubles ? cfg.chr[toss_c + 2] : 255, bb);   /* (2.98 : la 2e ligne écrivait le nom après bb, sur band_pg) */
        for (i = (u8)(t ? 1 : 6); *q; ) bb[i++] = *q++;
        return;
    }
    if (band_pg) {
        if (t == 0) {               /* noms au-dessus des colonnes */
            /* (2.96 : en double, « AL/MA » et son 0 final débordaient de bb
               à la position 23 et remettaient band_pg à 0 : les statistiques
               ne défilaient pas ; second nom une case plus à gauche) */
            far_call(BANK_X2, (void *)band_name_f, 19 << 8);          /* (2.98 : noms calés à droite sur leur colonne de chiffres) */
            far_call(BANK_X2, (void *)band_name_f, 1 | (26 << 8));
            return;
        }
        for (q = band_lbl[band_pg - 1], i = 0; *q; ) bb[i++] = *q++;
        for (k = 0; k < 2; ++k) {
            i = (u8)(17 + k * 7);
            switch (band_pg) {
            case 1:
                band_num(i, st_first_tot[k] ? (u16)(((u16)st_first_in[k] * 100) / st_first_tot[k]) : 0);
                bb[i + 3] = (char)T_PCT;        /* « % » (aussi en perspective, tuile $FE) */
                break;
            case 2: band_num(i, st_aces[k]); break;
            case 3: band_num(i, st_winners[k]); break;
            case 4: band_num(i, st_errors[k]); break;
            default:
                if (st_vmax[k]) band_num(i, (u16)st_vmax[k] + 80);
                else bb[i] = bb[i + 1] = bb[i + 2] = '-';
            }
        }
        return;
    }
    if (sc_server == t) bb[0] = '@';                          /* balle : au service */
    team_label(cfg.chr[t], cfg.doubles ? cfg.chr[t + 2] : 255, bb + 2);
    bb[7] = ' ';   /* (2.96 : en double, 5 lettres « AL/MA » : l'espace écrasait la dernière) */
    for (i = 0; i <= sc_set_no && i < MAX_SETS; ++i)
        bb[8 + i * 3] = (char)('0' + (i < sc_set_no ? sc_hist[i][t] : sc_games[t]));
    rules_point_label(t, bb + 24);
}
/* Annonces du score par l'arbitre : « Game! » à la fin d'un jeu,
   « Deuce! » et « Advantage! » (hors jeu décisif). Posées à la fin du point
   (hl_point_ev, game.c), dites un peu après, quand le DMC a fini l'annonce
   en cours (« Out! », « Fault! ») : rien n'est coupé. */
void msave_write(void);
#define V_DEUCE 4
#define V_GAME  5
#define V_ADV   6
u8 voice_pend, voice_t;       /* (globales : lues par les tests) */
extern u8 sc_crit, pending_winner;
u16 __fastcall__ hl_point_ev(u16 ev)
{
    u8 t;
    /* balles de set et de match, lues par l'IA à chaque trame (extras.c) :
       une seule fois par point (chaque appel des règles change de banque) */
    sc_crit = 0;
    for (t = 0; t < 2; ++t) {
        if (rules_is_match_point(t)) sc_crit |= (u8)(4 | (1 << t));
        else if (rules_is_set_point(t)) sc_crit |= (u8)(1 << t);
    }
    voice_pend = 0;
    if (ev & (EV_GAME | EV_SET)) { bank_call(BANK_SUN, sun_game); voice_pend = V_GAME; }
    else if (!sc_tiebreak && sc_points[0] >= 3 && sc_points[1] >= 3)
        voice_pend = sc_points[0] == sc_points[1] ? V_DEUCE : V_ADV;
    voice_t = 24;
    bank_call(BANK_CAR, msave_write);       /* sauvegarde du match en cours (career.c) */
    return 0;
}
static void voice_tick(void)
{
    if (!voice_pend) return;
    if (voice_t) { --voice_t; return; }
    if ((ms == MS_DEAD || ms == MS_SERVE) && !((*(volatile u8 *)0x4015) & 0x10)) { sfx_umpire(voice_pend); voice_pend = 0; }
}
/* ====================================================================== */
/* Bandes « cinéma » du ralenti : rangées 0-1 et 28-29 de l'écran 1   */
/* en noir (tuile $01, palette 1 : couleur 1 = noir), avec un cartouche    */
/* « RALENTI » et « A : PASSER » dans celle du haut ; le haut de l'image    */
/* vient de l'écran 1 (scroll_nt = 1), l'interruption de la ligne 16 passe */
/* au court, celle de la ligne 224 revient à l'écran 1 (crt0.s, lbox_on).  */
/* La rangée 28 est aussi le filet du bandeau du score : il la réécrit à   */
/* sa prochaine apparition (band_rows, bit 2). Les joueurs passent devant */
/* les bandes. lb_st : 0 rien, 0 à 3 rangées en écriture, 7 affiché.        */
/* ====================================================================== */
u8 lb_st;
u8 hl_quick;                             /* ralenti rapide d'une balle dehors de peu (hl_out) */
/* (bandes du ralenti retirées ; lbox_end reste, lb_st vaut toujours 0) */
static void lbox_end(void)
{
    if (!lb_st) return;
    if (lb_st == 7) { split_stop(); scroll_nt = 0; }
    lb_st = 0;
    hl_quick = 0;
    band_vis = 0;                        /* (le bandeau du score réécrit ses rangées) */
}
/* un joueur au moins à la manette (pas de démonstration) */
#pragma static-locals(push, off)        /* (pile : la RAM de travail est pleine) */
static u8 band_human(void)
{
    u8 i;
    if (cfg.demo) return 0;
    for (i = 0; i < npl; ++i) if (cfg.ctrl[i] != CTRL_CPU) return 1;
    return 0;
}
#pragma static-locals(pop)
/* drapeaux des supporters : trois par tribune, aux couleurs des
   joueurs (palette de sprites de l'équipe : étoffe = tenue, hampe = couleur
   sombre), agités (retournés toutes les 8 trames, en décalé) pendant les
   changements de côté, la standing ovation et la fin du match ; ni au club
   nudiste ni au court Margaux (public à part). Sprites ajoutés après
   spr_end (emplacements restés libres). */
void flags_draw(void);                  /* drapeaux des supporters : train.c, banque 20 */
extern u8 hl_flags;
#pragma zpsym("hl_flags")
static void pause_tick(void);
void mp_tick(void);
void st_tick(void);                     /* bandeau du set (train.c) */
extern u8 brk_pend;
extern u8 mp_t;
#pragma zpsym("mp_t")
void band_draw(void)
{
    u8 i, v;
    bank_call(BANK_TRAIN, flags_draw);
    pause_tick();
    if (hl_flags & 1) return;                /* (pause : son bandeau seulement) */
    /* balle de match : annoncée quand le serveur se met en place, à
       chaque balle de match (remise à zéro à la fin de chaque point) */
    /* fin de set : bandeau central du set pendant le passage aux
       chaises (train.c), puis l'écran du set */
    if (brk_pend && (cer == 4 || cer == CER_SEAT)) { bank_call(BANK_TRAIN, st_tick); band_hold = 1; voice_tick(); return; }
    if (cfg.mode == GM_MATCH && !cfg.demo) {
        if (ms == MS_DEAD && end_reason == END_POINT) hl_flags &= (u8)~2;
        else bank_call(BANK_TRAIN, mp_tick);        /* (bandeau central, train.c) */
        if (mp_t) { band_hold = 1; voice_tick(); return; }
    }
    voice_tick();
    lbox_end();
    /* le bandeau du score apparaît après le point (après le ralenti
       s'il y en a un) et reste jusqu'à un appui d'un joueur humain : c'est
       la seule façon de le fermer, et cet appui ne lance pas la balle ;
       l'ordinateur attend pour servir (band_hold, game.c). Sans joueur
       humain (démonstration, ordinateur contre ordinateur), il disparaît
       au lancer de balle, comme avant. */
    if (ms != MS_DEAD && ms != MS_SERVE && ms != MS_TOSS) band_dis = 0;   /* (3.07 : lancer non frappé, retour à la mise en place : le bandeau fermé ne revient pas) */
    else if (!rp_pending && (hl_flags & 4) && (pad1_new | pad2_new)) band_dis = 1;   /* (2.82 : seulement bandeau entièrement monté ; un appui plus tôt, par exemple juste après avoir passé le ralenti, l'empêchait d'apparaître) */
    if (cer == CER_CHOOSE || cer == CER_RECAP) band_pg = 6;
    /* balle de match gagnée : pas de bandeau avant la fête (le
       score n'est compté qu'à la fin du temps mort) */
    v = (u8)(ms == MS_DEAD && end_reason == END_POINT && (sc_crit & 4) && ((sc_crit >> pending_winner) & 1));
    v = (u8)(cfg.mode == GM_MATCH && !paused && !hl_mode && !sc_over && !v &&
             ((cer == CER_SEAT) || ((cer == CER_CHOOSE || cer == CER_RECAP) && toss_q) || (cer == 4 && !band_dis) || ((!cer || cer >= 0xF0) && !band_dis && !rp_pending && (st_first_tot[0] | st_first_tot[1]) && end_reason != END_FAULT && !(faults && end_reason == END_LET) && (ms == MS_DEAD || ms == MS_SERVE))));   /* (2.96 : pas de bandeau quand un second service est en attente : après une première faute, ou un let sur second service ; une double faute termine le point : bandeau comme d'habitude ; 2.98 : rien au premier service du match, aucun service frappé) */
    /* cer 4 (CER_CHAIR, vers les chaises) : le bandeau reste affiché
       depuis la fin du point jusqu'aux statistiques, sans disparaître */
    /* apparition : 48 trames après la fin du point (ou du ralenti),
       le temps de lire le message du point, puis le bandeau monte du bas en
       10 trames (la ligne de l'interruption remonte de 4 lignes par trame).
       Pendant ce temps l'ordinateur attend aussi, même sans joueur humain,
       pour que le bandeau se voie. (Pas de fondu : sa palette est aussi celle
       d'une partie du court.) */
    band_hold = (u8)(v && cer != CER_SEAT && (band_human() || band_t < BAND_WAIT + 10));
    if (!v) {
        hl_flags &= (u8)~4;
        if (band_vis) { split_stop(); band_vis = 0; }
        /* pendant un ralenti (ou avant qu'il commence) : à sa fin,
           joué ou passé, le bandeau monte aussitôt, sans le délai de lecture */
        band_t = (u8)(rp_pending || hl_mode || ms == MS_REPLAY ? BAND_WAIT : 0);
        return;
    }
    if (cer != CER_SEAT && cer != CER_CHOOSE && cer != CER_RECAP && band_t < BAND_WAIT) { ++band_t; return; }   /* (délai : pas encore de bandeau) */
    if (!band_vis) { band_vis = 1; band_rows = 15; }  /* (bits 2-3 : filet du bas, bords et rangée 29, que les bandes du ralenti recouvrent) */
    if (cer == CER_SEAT) {
        if (band_human()) {
            /* une page par appui ; après la dernière, les joueurs repartent */
            if ((pad1_new | pad2_new) & PAD_START) band_hold = 2;                                       /* START : toutes passées, on reprend */
            else if ((pad1_new | pad2_new) & (PAD_A | PAD_B)) { if (band_pg >= 5) band_hold = 2; else ++band_pg; }
        } else if (++band_t >= 40) { band_t = 0; if (++band_pg > 5) band_pg = 0; }   /* (40 trames par page : les 5 statistiques passent pendant la pause assise) */
    } else if (cer != CER_CHOOSE && cer != CER_RECAP) band_pg = 0;
    /* texte : rafraîchi quand le score change, une rangée par trame */
    if (band_last[0] != sc_points[0] || band_last[1] != sc_points[1] || band_last[2] != sc_games[0] ||
        band_last[3] != sc_games[1] || band_last[4] != sc_set_no || band_last[5] != sc_server || band_last[6] != band_pg) {
        band_last[6] = band_pg;
        band_last[0] = sc_points[0]; band_last[1] = sc_points[1]; band_last[2] = sc_games[0];
        band_last[3] = sc_games[1]; band_last[4] = sc_set_no; band_last[5] = sc_server;
        band_rows |= 3;                  /* (sans effacer les bits 2-3 : filet et bords à réécrire) */
    }
    if (band_rows & 8) {                 /* bords de la rangée 27 et rangée 29 : vides */
        for (i = 0; i < 16; ++i) bb[i] = 0;
        if (vram_len + 50 <= VBUF_MAX) {
            vbuf_raw((u16)(BAND_NT + 27 * 32), (const u8 *)bb, 2);
            vbuf_raw((u16)(BAND_NT + 27 * 32 + 30), (const u8 *)bb, 2);
            vbuf_raw((u16)(BAND_NT + 29 * 32), (const u8 *)bb, 16);
            vbuf_raw((u16)(BAND_NT + 29 * 32 + 16), (const u8 *)bb, 16);
            band_rows &= (u8)~8;
        }
    } else if (band_rows & 4) {                 /* filet du bas (rangée 28) et coins */
        for (i = 0; i < 32; ++i) bb[i & 15] = T_RULE;
        bb[0] = 0;
        if (vbuf_raw((u16)(BAND_NT + 28 * 32), (const u8 *)bb, 16)) {
            bb[0] = T_RULE; bb[15] = 0;
            if (vbuf_raw((u16)(BAND_NT + 28 * 32 + 16), (const u8 *)bb, 16)) band_rows &= (u8)~4;
        }
    } else if (band_rows) {
        i = (u8)((band_rows & 1) ? 0 : 1);
        band_text(i);
        if (vbuf_raw((u16)(BAND_NT + (BAND_ROW + 1 + i) * 32 + 2), (const u8 *)bb, 28)) band_rows &= (u8)~(1 << i);
    }
    /* 1 : texte en cours d'écriture ; 2 : texte prêt, sprites cachés dès
       cette trame ; 3 : bandeau affiché, une trame plus tard (sur une trame
       en retard, l'OAM envoyée au PPU est celle de la trame d'avant : elle
       doit déjà être sans les sprites du bas) */
    if (band_vis == 1) { if (band_rows) return; band_vis = 2; }
    else if (band_vis == 2) { band_vis = 3; band_on(cer == CER_SEAT || cer == CER_CHOOSE || cer == CER_RECAP ? BAND_LINE : BAND_LINE + 40); }
    else if (cer != CER_SEAT && band_t < BAND_WAIT + 10) { ++band_t; band_on((u8)(BAND_LINE + (BAND_WAIT + 10 - band_t) * 4)); }   /* (montée) */
    /* (3.08) hauteur de coupe des sprites : le haut du bandeau à cette
       trame, moins 4 lignes (une trame en retard montre l'OAM d'avant),
       et non plus 192 dès l'écriture du texte : le bas du corps du joueur
       du bas disparaissait avant que le bandeau monte le recouvrir */
    v = 192;
    if (band_vis == 2) v = 232;
    else if (cer != CER_SEAT && band_t < BAND_WAIT + 10) v = (u8)(188 + (BAND_WAIT + 10 - band_t) * 4);
    /* hl_flags bit 2 : bandeau entièrement monté (les chaises du
       changement de côté n'apparaissent qu'alors, cer.c) */
    if (band_vis == 3 && (cer == CER_SEAT || band_t >= BAND_WAIT + 10)) hl_flags |= 4; else hl_flags &= (u8)~4;
    /* sprites : rien ne descend dans le bandeau */
    /* (en assembleur : la boucle en C coûtait une trame en retard sur
       quatre en double en perspective) */
    __asm__("ldx #0");
    __asm__("band_hide: lda %v,x", oam_buf);
    __asm__("cmp %v", v);
    __asm__("bcc band_keep");
    __asm__("cmp #240");
    __asm__("bcs band_keep");
    __asm__("lda #$FF");
    __asm__("sta %v,x", oam_buf);
    __asm__("band_keep: inx");
    __asm__("inx");
    __asm__("inx");
    __asm__("inx");
    __asm__("bne band_hide");
}

/* phases du crépuscule et lumière : sunset.c (banque 22) */
u8 sun_pend;
void sun_apply(void);
void sun_game(void);
u16 __fastcall__ sun_calc_f(u16 unused);
/* chargement du court (court_load, game.c ; rendu coupé) : bandeau du score
   (écran 1) puis lumière du crépuscule. band_init écrit dans la VRAM : jamais
   pendant l'affichage (sun_apply, elle, est aussi appelée à la fin de chaque
   jeu, écran allumé : si elle appelait band_init, ces écritures en
   plein affichage abîmeraient des tuiles du court et de la police) */
void car_match_setup(void);           /* career.c : bonus de la carrière */
void court_post(void)
{
    lb_st = 0;                          /* (bandes du ralenti : écran 1 réécrit par band_init) */
    band_init();
    bank_call(BANK_SUN, sun_apply);
    bank_call(BANK_CAR, car_match_setup);
}
/* début du ralenti : les repères ne valent que s'ils sont dans les trames
   rejouées (tampon circulaire de 160 trames : un indice plus ancien a été
   réutilisé par une trame récente) */
extern u8 rp_left;
extern u16 rp_seq;                 /* trames enregistrées (crt0.s) */
static u8 hl_fresh(u16 f)
{
    /* âge exact en trames enregistrées : dans les trames rejouées */
    return (u8)(rp_seq - f <= rp_left);
}
void hl_check(void)
{
    if (cfg.replays == 2) hl_sok = 0;   /* « OUI, SIMPLE » : pas de gros plan, mais le zoom */
    /* frappe décisive plus ancienne que le ralenti (lob, long échange
       conclu par une faute adverse) : le gros plan ouvre le ralenti (son
       image ne dépend pas de la trame) */
    if (!hl_fresh(hl_sf)) hl_si = (u8)(rp_head >= rp_left ? rp_head - rp_left : rp_head + 160 - rp_left);
    if (!hl_fresh(hl_sbf)) hl_sbok = 0;
}
extern u8 hit_jump;
u16 __fastcall__ near_in_f(u16 xy);     /* extra2.c */
/* fin de point : hl_w gagnant, hl_ptxt titre */
void hl_select(void)
{
    u8 i = 0;
    const char *txt = hl_ptxt;
    hl_sok = (u8)((hl_hv >> hl_w) & 1);
    hl_si = hl_hi[hl_w]; hl_sp = hl_hp[hl_w]; hl_sk = hl_hk[hl_w]; hl_sf = hl_hf[hl_w]; hl_sbf = hl_bf;
    hl_sbok = hl_bok; hl_sbi = hl_bi; hl_sbx = hl_bx; hl_sby = hl_by; hl_sbin = hl_bin;
    if (hl_sok && hl_sk == 1 && hit_jump) { rp_pending = 1; crowd_big |= 1; }   /* smash sauté gagnant : toujours au ralenti, standing ovation */
    if (rally_hits >= 12) crowd_big |= 1;                  /* (très long échange : standing ovation) */
    if (hl_sok && hl_cdv[hl_w] && hl_sk == 4) {     /* plongeon gagnant sur un canon : ralenti et gros plan */
        set_msg("PLONGEON", "GAGNANT !", 70);    /* (titre du gros plan : 8 lettres, une ligne) */
        rp_pending = 1;
    }
    while (*txt == ' ') ++txt;      /* (messages centrés : sans les blancs) */
    while (i < 15 && txt[i]) { hl_txt[i] = txt[i]; ++i; }
    while (i && hl_txt[i - 1] == ' ') --i;
    hl_txt[i] = 0;
}
extern u8 ctrl_swap, bg_dirty, swap_on;
extern u8 proj_x, proj_y;
#pragma zpsym("proj_x")
#pragma zpsym("proj_y")
void __fastcall__ proj2(u8 x, u8 y);           /* sprp.s */
void court_reload(void);                       /* banque 0 */
void rp_play(void);                            /* banque 3 */
#include "assets_swap.h"   /* SWAP_BASE : swap_base est en banque 3, illisible d'ici */

#pragma bss-name(push, "WBSS")
u8 hl_mode;
static u8 hl_t, hl_c, hl_n, hl_shake;
static s16 hl_cam;
static u8 hl_rx, hl_ry;                        /* point visé (zoom), à l'écran */
static u8 hl_bk;                               /* couleur de fond du court */
static const char *zl;                         /* titre du zoom : BONNE ou DEHORS */
#pragma bss-name(pop)

/* tampons du zoom : zone libre de la RAM sauvegardée, après les blocs
   sauvegardés (jamais lue au démarrage ; voir stats.c). Superposés
   aux tampons des menus (menuart.c, segment MENUBUF) : le zoom ne sert
   qu'en match, les menus jamais pendant un match */
#pragma bss-name(push, "ZOOMBUF")
static u8 zb_nt[240];          /* fenêtre de 16 x 15 tuiles du court */
static u8 zb_at[64];           /* attributs de l'écran du court */
static u8 zb_map[256];         /* tuile source -> entrée de zb_q (0xFF : pas encore) */
#define ZQ 80
static u8 zb_q[ZQ][4];         /* les 4 quarts agrandis d'une tuile source */
static u8 zb_src[16], zb_out[16];
#define ZS 12
static u8 zb_sx[ZS], zb_sy[ZS], zb_st[ZS], zb_sa[ZS];   /* sprites de la fenêtre */
static u8 zb_spr[ZS][16];      /* leurs tuiles */
#pragma bss-name(pop)

static const u8 dbl[16] = { 0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F,
                            0xC0, 0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF };

static u8 vdummy;
static void vread(u16 a, u8 *d, u8 n)
{
    vram_adr(a);
    vdummy = PPU_DATA;             /* lecture différée du PPU : la première rend l'ancien tampon */
    while (n--) *d++ = PPU_DATA;
}
static void vwrite(u16 a, const u8 *d, u8 n)
{
    vram_adr(a);
    while (n--) vram_put(*d++);
}
/* quart q (0 haut gauche, 1 haut droit, 2 bas gauche, 3 bas droit) de
   zb_src agrandi deux fois dans zb_out ; rend 0..3 si le quart est d'une
   seule couleur (tuile unie), 0xFF sinon */
static u8 quarter(u8 q)
{
    register u8 r;
    u8 sr, b0, b1, o0, o1, solid = 1;
    for (r = 0; r < 8; ++r) {
        sr = (u8)(((q >> 1) << 2) + (r >> 1));
        b0 = zb_src[sr]; b1 = zb_src[8 + sr];
        if (q & 1) { b0 &= 15; b1 &= 15; } else { b0 >>= 4; b1 >>= 4; }
        zb_out[r] = o0 = dbl[b0]; zb_out[8 + r] = o1 = dbl[b1];
        if ((o0 != 0 && o0 != 0xFF) || (o1 != 0 && o1 != 0xFF) || o0 != zb_out[0] || o1 != zb_out[8]) solid = 0;
    }
    return solid ? (u8)((zb_out[0] ? 1 : 0) | (zb_out[8] ? 2 : 0)) : 0xFF;
}

/* ---------------------------------------------------------------------- */
/* Gros plan                                                               */
/* ---------------------------------------------------------------------- */
static const u8 whip_in[6] = { 150, 90, 48, 22, 8, 2 };
#define T_BACK  0xFB          /* fond uni (couleur 0) */
#define T_GND   0xFC          /* sol (couleur 3) */
static const u8 z8[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const u8 ff[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static void flash(u8 c)
{
    pal_buf[0] = pal_buf[4] = pal_buf[8] = pal_buf[12] = pal_buf[16] = c;
    pal_dirty = 1;
}

/* titre : lettres de la police chargées dans la zone des jeux de tuiles
   (rétablie par load_court au retour), dessinées en sprites sur une ou
   deux lignes (8 sprites au plus par ligne) */
static u8 lbl_n, lbl_brk;
static void label_load(void)
{
    u8 i, c;
    lbl_n = 0; lbl_brk = 0xFF;
    for (i = 0; hl_txt[i]; ++i) {
        c = (u8)hl_txt[i];
        if (c == ' ' && lbl_brk == 0xFF && i >= 3) lbl_brk = i;
        if (c > ' ' && c < 0x60) chr_copy(BANK_CHR, chr_data + ((u16)c << 4), 0x1000 + ((u16)(SWAP_BASE + i) << 4), 16);
    }
    lbl_n = i;
    /* une seule ligne si elle tient en 8 lettres */
    for (c = 0, i = 0; i < lbl_n; ++i) if (hl_txt[i] != ' ') ++c;
    if (c <= 8) lbl_brk = 0xFF;
}
static void confetti(void);
static void label_draw(u8 y, u8 shown)
{
    u8 i, x, l0 = lbl_brk == 0xFF ? lbl_n : lbl_brk, l1, yy = y;
    x = (u8)(128 - l0 * 4);
    for (i = 0; i < lbl_n && i < shown; ++i) {
        if (i == lbl_brk) { l1 = (u8)(lbl_n - i - 1); x = (u8)(128 - l1 * 4); yy = (u8)(y + 10); continue; }
        if (hl_txt[i] != ' ') spr1(x, yy, (u8)(SWAP_BASE + i), 2);
        x += 8;
    }
}

/* un plan : image haute définition, fond uni, sol ; palettes du joueur */
u16 __fastcall__ hd_backdrop_f(u16 z);   /* extra2.c (banque 30) */
u16 __fastcall__ cens_hd_f(u16 a); u16 __fastcall__ nude_art_f(u16 a);   /* extra2.c : club nudiste */
static void cut_build(u8 art)
{
    u8 i, S, O, D, z, c = (art == TZ_BALLKID || art == TZ_CROWD) ? 11 : cfg.chr[hl_sp];   /* (ramasseuse, public : tenues vertes, celles de JADE) */
    u8 g = (u8)(char_female[c] ? (c == 16 ? 2 : 1) : 0);
    const tz_art_t *a = &tz_art[art][g];
    const u8 *m;
    u16 ad;
    if (cfg.env == ENV_NUDE) a = (const tz_art_t *)far_call(BANK_X2, (void *)nude_art_f, (u16)(art | (g << 8)));   /* (club nudiste : sans vêtements, extra2.c) */
    z = a->zoom;
    flash(0x30);                        /* (écran blanc pendant le chargement) */
    fade_skip = 1; fade_lvl = 0;
    ppu_off();
    vram_adr(0x0000); vram_fill(0, 16);
    tz_bank = a->bank; tz_n = a->ntile;
    tz_unpack(a->chr);
    vram_adr((u16)T_BACK << 4); for (i = 0; i < 8; ++i) vram_put(z8[i]); for (i = 0; i < 8; ++i) vram_put(z8[i]);
    vram_adr((u16)T_GND << 4); for (i = 0; i < 8; ++i) vram_put(ff[i]); for (i = 0; i < 8; ++i) vram_put(ff[i]);
    vram_adr(0x2000);
    for (i = 0; i < 30; ++i) vram_fill(z || i < 27 ? T_BACK : T_GND, 32);
    vram_adr(0x2400); vram_fill(T_BACK, 960);
    m = a->map; ad = NTADR(a->x0, a->y0);
    for (i = 0; i < a->h; ++i) { vram_adr(ad); ad += 32; tz_n = a->w; tz_copy(m); m += a->w; }
    vram_adr(0x23C0); tz_n = 64; tz_copy(a->at);
    vram_adr(0x27C0); vram_fill(0, 64);
    far_call(BANK_X2, (void *)hd_backdrop_f, z);   /* décor sur tout l'écran, sol continu (extra2.c) */
    /* palettes (comme la bande-annonce) */
    pal_set_player(61, c);
    S = pal_buf[5]; O = pal_buf[6]; D = pal_buf[7];
    /* fond de couleur  : gros plan rouge
       sombre, plan moyen bleu nuit. Les contours, les yeux et la bouche des
       images HD sont dessinés dans la couleur du fond (tools/gen_teaser.py) :
       ces deux couleurs, les plus sombres de la console après le noir, les
       teintent le moins possible ; un fond noir laissait des carrés noirs
       autour du joueur sur le décor (tuiles du bord de l'image) */
    flash(z ? 0x06 : 0x01);
    if (S >= 0x10) { pal_buf[5] = S; pal_buf[6] = (u8)(S - 0x10); } else { pal_buf[5] = (u8)(S + 0x10); pal_buf[6] = S; }
    if (O >= 0x10) { pal_buf[9] = O; pal_buf[10] = (u8)(O - 0x10); } else { pal_buf[9] = (u8)(O + 0x10); pal_buf[10] = O; }
    if (O == 0x30) pal_buf[10] = 0x10;
    pal_buf[7] = pal_buf[11] = D;
    pal_buf[13] = pal_buf[5]; pal_buf[14] = pal_buf[9]; pal_buf[15] = 0x30;
    if (z) { pal_buf[1] = pal_buf[5]; pal_buf[2] = D; pal_buf[3] = 0x30; pal_buf[14] = pal_buf[6]; pal_buf[15] = pal_buf[9]; }
    else { pal_buf[1] = 0x0F; pal_buf[2] = 0x02; pal_buf[3] = 0x11; }   /* (tirets du décor : noirs sur le bleu nuit) */
    pal_buf[16 + 9] = 0x0F; pal_buf[16 + 10] = 0x0F; pal_buf[16 + 11] = 0x30;   /* titre : blanc, ombre noire */
    pal_buf[16 + 13] = 0x38; pal_buf[16 + 14] = 0x27; pal_buf[16 + 15] = 0x30;  /* balle */
    pal_dirty = 1;
    hl_cam = z ? 0 : -(s16)whip_in[0];
    scroll_x = (u8)hl_cam; scroll_nt = (u8)((hl_cam >> 8) & 1); scroll_y = 0;
    spr_begin(); spr_end();
    label_load();                       /* (lettres du titre, rendu coupé) */
    far_call(BANK_X2, (void *)cens_hd_f, (u8)(z | (art << 1) | (g << 6)));   /* (club nudiste : mosaïque ; perdant : visage triste, extra2.c) */
    ppu_on();
}

/* retour au court : tuiles d'origine (les deux tables), écran du court,
   puis la trame du ralenti */
static void back_to_court(void)
{
    flash(0x30);
    fade_skip = 1; fade_lvl = 0;
    ppu_off();
    ctrl_swap = 0;
    scroll_x = 0; scroll_y = 0; scroll_nt = 0;
    chr_reload();
    bg_dirty = 0;
    swap_on = 0;
    bank_call(BANK_GAME, court_reload);  /* (rend aussi les palettes du court) */
    hl_mode = 0;
    bank_call(BANK_EXTRAS, rp_play);
    if (rp_done) { spr_begin(); spr_end(); }   /* (ralenti passé : pas de sprites du gros plan) */
    ppu_on();
}

#define SPR(px, py, t, at) { sx = (s16)(px) - hl_cam; if (sx >= 0 && sx < 248) spr1((u8)sx, (u8)(py), (u8)(t), (u8)(at)); }
static void closeup_tick(void)
{
    u8 i, k;
    s16 sx;
    ++hl_t;
    if (hl_t < 12) {                     /* image figée */
        if (hl_t == 9) { flash(0x30); sfx_perfect(); }
        if (hl_t == 11) cut_build(hl_sk == 4 ? TZ_SLIDE : hl_sk == 3 ? TZ_BACKHAND : hl_sk == 1 ? TZ_SMASH_AIR : hl_sk ? TZ_SMASH : TZ_IMPACT);   /* (smash en plein vol) */   /* plongeon : la glissade */   /* revers ; service, smash : raquette au-dessus de la tête */
        return;
    }
    hl_c = (u8)(hl_t - 11);             /* temps du plan */
    spr_begin();
    if (hl_t < 70) {
        /* plan moyen : travelling éclair, puis lente dérive */
        hl_cam = hl_c < 6 ? -(s16)whip_in[hl_c] : (s16)(hl_c >> 2);
        scroll_x = (u8)hl_cam; scroll_nt = (u8)((hl_cam >> 8) & 1);
        if ((hl_c & 3) == 0 && hl_c > 2) flash(0x01);
        /* la balle arrive sur le tamis, impact (flash), traînée : coup droit
           (elle arrive de la droite, repart vers la gauche en montant) ;
           service et smash (elle tombe d'en haut, repart vers le bas) */
        if (hl_c >= 8 && hl_c < 22) {
            k = (u8)(22 - hl_c);
            if (hl_sk == 4) { SPR(TZ_SLIDE_RACKET_X - k * 9, TZ_SLIDE_RACKET_Y - 4 + k * 3, ball_base + 8, 3); }   /* plongeon : de la gauche, au ras du sol */
            else if (hl_sk == 3) { SPR(TZ_BACKHAND_RACKET_X - k * 9, TZ_BACKHAND_RACKET_Y - 4 + k * 5, ball_base + 8, 3); }   /* revers : de la gauche */
            else if (hl_sk == 1) { if ((s16)(TZ_SMASH_AIR_RACKET_Y - 4) - (s16)(k * 7) >= 0) SPR(TZ_SMASH_AIR_RACKET_X + k * 4, TZ_SMASH_AIR_RACKET_Y - 4 - k * 7, ball_base + 8, 3); }
            else if (hl_sk) { if ((s16)(TZ_SMASH_RACKET_Y - 4) - (s16)(k * 7) >= 0) SPR(TZ_SMASH_RACKET_X + k * 4, TZ_SMASH_RACKET_Y - 4 - k * 7, ball_base + 8, 3); }   /* (au-dessus du cadre : cachée) */
            else { SPR(TZ_IMPACT_RACKET_X + k * 9, TZ_IMPACT_RACKET_Y - 4 + k * 5, ball_base + 8, 3); }
        } else if (hl_c == 22) {
            flash(0x30); sfx_hit(3);
        } else if (hl_c > 22 && hl_c < 34) {
            for (i = 0; i < 3; ++i) {
                k = (u8)((hl_c - 22) * 10 - i * 6);
                if (k >= 40) continue;
                if (hl_sk == 4) { SPR(TZ_SLIDE_RACKET_X - 4 - k, TZ_SLIDE_RACKET_Y - 8 - k, ball_base + 8, 3); }   /* (relevée vers le haut) */
                else if (hl_sk == 3) { SPR(TZ_BACKHAND_RACKET_X - 4 + k, TZ_BACKHAND_RACKET_Y - 8 - (k >> 1), ball_base + 8, 3); }
                else if (hl_sk == 1) { SPR(TZ_SMASH_AIR_RACKET_X - k, TZ_SMASH_AIR_RACKET_Y + k + (k >> 1), ball_base + 8, 3); }
                else if (hl_sk) { SPR(TZ_SMASH_RACKET_X - k, TZ_SMASH_RACKET_Y + k + (k >> 1), ball_base + 8, 3); }
                else { SPR(TZ_IMPACT_RACKET_X + 4 - k, TZ_IMPACT_RACKET_Y - 8 - (k >> 1), ball_base + 8, 3); }
            }
        }
        /* titre, lettre par lettre */
        if (hl_c >= 26) label_draw(12, (u8)((hl_c - 26) >> 1));   /* (en haut : libre dans les deux images, sous la marge des téléviseurs) */
        if (hl_c == 26) sfx_crowd(7);
        spr_end();
        if (hl_t == 69) flash(0x30);   /* (2.90 : plus de carillon) */
        return;
    }
    if (hl_t == 70) {
        /* après un très long échange (20 frappes ou plus) : le public debout */
        if (rally_hits >= 20 && cfg.env != ENV_NUDE) { cut_build(TZ_CROWD); hl_cam = 0; scroll_x = 0; scroll_nt = 0; sfx_crowd(7); }
        else cut_build(TZ_VICTORY_Z);
        hl_shake = 6;
    }
    /* gros plan : la joie, l'écran tremble puis s'apaise ; confettis */
    if (hl_shake) { scroll_y = (u8)((hl_t & 1) ? hl_shake : 0); if ((hl_t & 3) == 0) --hl_shake; } else scroll_y = 0;
    confetti();                         /* (la même pluie que le portrait final : place de la banque) */
    label_draw(206, 16);
    spr_end();
    if (hl_t >= 118) back_to_court();
}

/* ---------------------------------------------------------------------- */
/* Arrêt sur image zoomé                                                   */
/* ---------------------------------------------------------------------- */
static const u8 t_corner[16] = { 0xF0, 0x80, 0x80, 0x80, 0, 0, 0, 0,  0xF0, 0x80, 0x80, 0x80, 0, 0, 0, 0 };
#define T_CORNER (SWAP_BASE + 20)
static u8 zx, zy, zwx, zwy;            /* point visé dans l'image agrandie ; coin de la fenêtre (px) */
/* trace de la balle : l'empreinte exacte de la balle jugée. Une balle
   est bonne si son bord (rayon BALL_R = 2 px) touche la
   ligne : la trace est donc un disque de 2 px de rayon (du court), centré
   sur le centre du pixel de rebond, celui que juge l'arbitre. Balle bonne :
   la trace mord sur la ligne ; dehors : un écart se voit. Contour sombre
   (couleur 2), intérieur plein de la couleur de la balle (couleur 1). 2 x 2 sprites (16 x 16 px),
   tuiles hl_n à hl_n + 3, le titre ensuite. (L'image agrandie est
   toujours celle du court à plat : voir zoom_build.) */
u16 __fastcall__ mark_build_f(u16 adz);   /* extra2.c (banque 30) : adresse des 4 tuiles | z */
#define mark_build(ad, z) far_call(BANK_X2, (void *)mark_build_f, (u16)((ad) | (z)))

/* Bandeau du score pendant la pause : rangées 12 à 17 (filet, deux
   lignes du score comme le bandeau du bas, filet), écrites sur l'écran 1 une
   rangée par trame puis montrées par deux interruptions de ligne (cband_on,
   comme le bandeau de la balle de match) : ni écran coupé, ni appui perdu.
   mp_t (page zéro) : 1 à 7 écriture, 8 affiché. Les sprites qui passeraient
   dessous sont cachés. */
#define PB_ROW 12
static void pause_tick(void)
{
    u8 i, r;
    if (cfg.mode != GM_MATCH || hl_mode) return;
    if (paused) {
        if (!(hl_flags & 1)) {
            hl_flags |= 1;
            if (band_vis) band_vis = 0;
            split_stop();
            mp_t = 1;
        }
        if (mp_t < 8) {
            if (vram_len + 44 > VBUF_MAX) return;
            r = (u8)(mp_t - 1);
            for (i = 0; i < 16; ++i) bb[i] = (char)(r == 1 || r == 4 ? T_RULE : 0);
            if (r == 2 || r == 3) {
                i = band_pg; band_pg = 0;
                band_text((u8)(r - 2));
                band_pg = i;
                vbuf_raw(BAND_NT + (PB_ROW + r) * 32 + 2, (const u8 *)bb, 28);
                bb[0] = bb[1] = 0;               /* bords : le décor n'y transparaît plus */
                vbuf_raw(BAND_NT + (PB_ROW + r) * 32, (const u8 *)bb, 2);
                vbuf_raw(BAND_NT + (PB_ROW + r) * 32 + 30, (const u8 *)bb, 2);
            } else if (r == 6) {
                for (i = 0; i < 16; ++i) bb[i] = 0x55;
                vbuf_raw(BAND_NT + 0x3C0 + 24, (const u8 *)bb, 16);
            } else {
                bb[16] = bb[0]; bb[0] = 0;               /* (filet : coins vides) */
                vbuf_raw(BAND_NT + (PB_ROW + r) * 32, (const u8 *)bb, 16);
                bb[0] = bb[16]; bb[15] = 0;
                vbuf_raw(BAND_NT + (PB_ROW + r) * 32 + 16, (const u8 *)bb, 16);
            }
            if (++mp_t == 8) cband_on(47);
        }
        for (i = 0; i < 64; ++i) { r = oam_buf[i * 4]; if (r >= PB_ROW * 8 - 8 && r < (PB_ROW + 6) * 8) oam_buf[i * 4] = 0xFF; }   /* (aussi ceux qui débordent par le haut) */
    } else if (hl_flags & 1) {
        hl_flags &= (u8)~1;
        split_stop();
        mp_t = 0;
    }
}

void load_court(void);                  /* banque 3 (extras.c) */
static void zoom_build(void)
{
    register u8 i;
    u8 r, c, k, n, q, nq = 0, next = 4, ns = 0, src, a;
    u16 ad;
    u8 tx0, ty0;
    flash(0x30);
    fade_skip = 1; fade_lvl = 0;
    ppu_off();
    /* en perspective, l'image agrandie est celle du court à plat :
       les lignes du court en perspective et la projection du point de
       rebond s'arrondissent différemment (jusqu'à un pixel), de quoi fausser
       une balle jugée au demi-pixel. À plat, lignes et trace sont exactes ;
       les joueurs (placés en perspective) n'y sont pas. Le retour au court
       recharge la perspective (back_to_court). */
    src = persp;                        /* (src : réutilisé plus bas pour les tuiles) */
    if (src) {
        k = cfg.view; cfg.view = 0;
        vram_adr(0x2000); bank_call(BANK_EXTRAS, load_court);
        cfg.view = k;
        hl_rx = hl_sbx; hl_ry = hl_sby;
    }
    /* fenêtre de 128 x 120 px autour du point visé, alignée sur les tuiles,
       dans le court (rangées 4 à 29) */
    a = (u8)(hl_rx < 64 ? 0 : (u8)((hl_rx - 64) >> 3)); tx0 = a > 16 ? 16 : a;
    a = (u8)(hl_ry < 92 ? 4 : (u8)((hl_ry - 60) >> 3)); ty0 = a > 15 ? 15 : a;
    zwx = (u8)(tx0 << 3); zwy = (u8)(ty0 << 3);
    zx = (u8)((hl_rx - zwx) << 1); zy = (u8)((hl_ry - zwy) << 1);
    /* sprites de la trame figée dans la fenêtre (balle d'abord : elle est en
       tête), sans les coins du réticule (emplacements 60 à 63) */
    for (i = 0; i < 60 && ns < ZS && !src; ++i) {
        u8 y = oam_buf[i * 4], x = oam_buf[i * 4 + 3];
        if (y >= 0xEF || (oam_buf[i * 4 + 2] & 2)) continue;   /* (seulement les joueurs, palettes 0 et 1 : ni ombres ni poussière, ni la balle : la trace la remplace) */
        ++y;
        if ((u8)(x - zwx) >= 124 || x < zwx || (u8)(y - zwy) >= 116 || y < zwy) continue;
        zb_sx[ns] = (u8)(x - zwx); zb_sy[ns] = (u8)(y - zwy);
        zb_st[ns] = oam_buf[i * 4 + 1]; zb_sa[ns] = oam_buf[i * 4 + 2];
        vread(0x1000 + ((u16)zb_st[ns] << 4), zb_spr[ns], 16);
        ++ns;
    }
    /* décor : fenêtre et attributs */
    vread(0x23C0, zb_at, 64);
    for (r = 0; r < 15; ++r) vread(NTADR(tx0, ty0 + r), zb_nt + r * 16, 16);
    /* tuiles agrandies : écrites dans la table $1000 (le fond y est lu
       pendant le zoom, ctrl_swap), les sources restent lisibles en $0000 */
    for (c = 0; c < 4; ++c) {           /* 0 à 3 : tuiles unies */
        for (i = 0; i < 16; ++i) zb_out[i] = (u8)(((i < 8 ? c & 1 : c & 2)) ? 0xFF : 0);
        vwrite(0x1000 + ((u16)c << 4), zb_out, 16);
    }
    for (i = 0; ; ++i) { zb_map[i] = 0xFF; if (i == 255) break; }
    for (k = 0; k < 240; ++k) {
        src = zb_nt[k];
        if (zb_map[src] != 0xFF) continue;
        if (nq >= ZQ) { zb_map[src] = 0xFE; continue; }
        vread((u16)src << 4, zb_src, 16);
        for (q = 0; q < 4; ++q) {
            n = quarter(q);
            if (n == 0xFF) {
                if (next == 0) n = 0;    /* (plus de place : uni) */
                else { n = next; vwrite(0x1000 + ((u16)next << 4), zb_out, 16); if (++next == 0) next = 0; }
            }
            zb_q[nq][q] = n;
        }
        zb_map[src] = nq++;
    }
    /* table de noms agrandie (écran 0) */
    for (r = 0; r < 15; ++r) {
        ad = NTADR(0, r * 2);
        for (q = 0; q < 2; ++q) {
            vram_adr(ad + q * 32);
            for (c = 0; c < 16; ++c) {
                k = zb_map[zb_nt[r * 16 + c]];
                if (k >= ZQ) { vram_put(0); vram_put(0); }
                else { vram_put(zb_q[k][q * 2]); vram_put(zb_q[k][q * 2 + 1]); }
            }
        }
    }
    /* attributs : chaque tuile source devient un bloc de 16 x 16 */
    vram_adr(0x23C0);
    for (r = 0; r < 8; ++r)
        for (c = 0; c < 8; ++c) {
            u8 v = 0, qq;
            for (qq = 0; qq < 4; ++qq) {
                u8 sx = (u8)(tx0 + c * 2 + (qq & 1)), sy = (u8)(ty0 + r * 2 + (qq >> 1)), p;
                if (sy > 29) sy = 29;
                p = (u8)((zb_at[(sy >> 2) * 8 + (sx >> 2)] >> (((sy & 2) << 1) | (sx & 2))) & 3);
                v |= (u8)(p << (qq * 2));
            }
            vram_put(v);
        }
    /* sprites agrandis : tuiles en $0000 (table des sprites pendant le zoom) */
    spr_begin();
    next = 0;
    for (k = 0; k < ns; ++k) {
        u8 id[4], hf = (u8)(zb_sa[k] & 0x40), vf = (u8)(zb_sa[k] & 0x80);
        for (i = 0; i < 16; ++i) zb_src[i] = zb_spr[k][i];
        for (q = 0; q < 4; ++q) {
            quarter(q);
            for (i = 0, n = 0; i < 16; ++i) n |= zb_out[i];
            if (!n) { id[q] = 0xFF; continue; }
            id[q] = next;
            vwrite((u16)next << 4, zb_out, 16);
            ++next;
        }
        for (q = 0; q < 4; ++q) {
            /* quart affiché q : quart source selon les miroirs */
            u8 s = (u8)(((hf ? 1 : 0) ^ (q & 1)) | (((vf ? 2 : 0) ^ (q & 2))));
            u16 x = (u16)zb_sx[k] * 2 + (q & 1) * 8, y = (u16)zb_sy[k] * 2 + (q >> 1) * 8;
            if (id[s] == 0xFF || x > 248 || y > 230 || oam_i >= 208) continue;   /* (52 à 63 : trace et titre) */
            spr1((u8)x, (u8)y, id[s], zb_sa[k]);
        }
    }
    /* trace de la balle et titre (police) */
    mark_build((u16)next << 4, 2);
    hl_n = next;
    for (i = 0; zl[i]; ++i)
        if ((u8)zl[i] > ' ' && (u8)zl[i] < 0x60)
            chr_copy(BANK_CHR, chr_data + ((u16)(u8)zl[i] << 4), (u16)(next + 4 + i) << 4, 16);
    lbl_n = i; lbl_brk = 0xFF;
    spr_end();
    ctrl_swap = 0x18;
    scroll_x = 0; scroll_y = 0; scroll_nt = 0;
    pal_buf[16 + 9] = 0x0F; pal_buf[16 + 10] = 0x0F; pal_buf[16 + 11] = 0x30;
    pal_buf[16 + 14] = 0x0F;            /* trace : contour sombre (couleur 2 de la palette de la balle) */
    flash(hl_bk);
    ppu_on();
}

/* Second temps du zoom des balles limites : x 4 sur l'impact, sans
   les joueurs (8 sprites par ligne : un joueur agrandi 4 fois ne tiendrait
   pas). Une fenêtre de 64 x 60 px du court autour de l'impact est agrandie
   à partir de l'image x 2 (voir zoom4_build). Chaque tuile de l'image agrandie couvre 2 x 2 px du court
   (4 couleurs de 2 bits : au plus 256 tuiles, quelques dizaines en
   pratique), créée à la première rencontre (zb_map). Trace de la balle :
   l'empreinte de la balle jugée (mark_build), titre BONNE / DEHORS. */
#define Z4 26                           /* trames du premier temps (x 2)  */

#pragma static-locals(push, off)        /* (pile : la RAM de travail est pleine) */
static void zoom4_build(void)
{
    /* construit à partir de l'image x 2 affichée (complète : en
       perspective, le court rechargé n'avait pas encore ses lignes) : un
       pixel du court y occupe 2 x 2 pixels ; image x 2 : écran 0, tuiles en
       $1000 (fond pendant le zoom) ; image x 4 : écran 1, tuiles en $0000
       (les sprites agrandis du x 2 y étaient ; il n'y a plus de joueurs),
       trace et titre en $1000, tables des tuiles remises à l'endroit */
    u8 r, c, j, sh, v, t, wx, wy, y2, px2, next = 0;
    u8 *buf = zb_q[0];                  /* (256 octets : les 16 tuiles d'une rangée de l'image x 2) */
    u8 *row = zb_nt;                    /* (32 octets : une rangée de l'image x 4) */
    flash(0x30);
    fade_skip = 1; fade_lvl = 0;
    ppu_off();
    /* fenêtre x 4 : 64 x 60 px du court autour de l'impact, dans la fenêtre
       x 2 (128 x 120 px depuis zwx, zwy) ; x multiple de 8, y de 4 */
    wx = (u8)(hl_rx < 32 ? 0 : ((hl_rx - 32) & 0xF8));
    if (wx < zwx) wx = zwx; if (wx > (u8)(zwx + 64)) wx = (u8)(zwx + 64);
    wy = (u8)(hl_ry < 30 ? 0 : ((hl_ry - 30) & 0xFC));
    if (wy < zwy) wy = zwy; if (wy > (u8)(zwy + 60)) wy = (u8)(zwy + 60);
    zx = (u8)((hl_rx - wx) << 2); zy = (u8)((hl_ry - wy) << 2);
    for (j = 0; ; ++j) { zb_map[j] = 0xFF; if (j == 255) break; }
    for (r = 0; r < 30; ++r) {
        y2 = (u8)(((wy - zwy) << 1) + (r << 2));      /* (ligne de l'image x 2 : multiple de 4) */
        px2 = (u8)(y2 & 7);                           /* (0 ou 4 : les deux lignes lues dans la même tuile) */
        if (r == 0 || px2 == 0) {
            u8 tx0 = (u8)((wx - zwx) >> 2);           /* (première tuile x 2 de la fenêtre) */
            vread(NTADR(tx0, y2 >> 3), row, 16);
            for (j = 0; j < 16; ++j) vread(0x1000 + ((u16)row[j] << 4), buf + j * 16, 16);
        }
        for (c = 0; c < 32; ++c) {
            u8 *q = buf + (c >> 1) * 16 + px2;
            sh = (u8)(7 - ((c & 1) << 2));            /* (pixel x 2 : 0 ou 4 dans la tuile ; son voisin du court à 2) */
            v = (u8)(((q[0] >> sh) & 1) | (((q[8] >> sh) & 1) << 1)
                   | (((q[0] >> (sh - 2)) & 1) << 2) | (((q[8] >> (sh - 2)) & 1) << 3)
                   | (((q[2] >> sh) & 1) << 4) | (((q[10] >> sh) & 1) << 5)
                   | (((q[2] >> (sh - 2)) & 1) << 6) | (((q[10] >> (sh - 2)) & 1) << 7));
            t = zb_map[v];
            if (t == 0xFF) {
                for (j = 0; j < 8; ++j) {
                    u8 k = (u8)(j < 4 ? v : v >> 4);
                    zb_out[j] = (u8)(((k & 1) ? 0xF0 : 0) | ((k & 4) ? 0x0F : 0));
                    zb_out[8 + j] = (u8)(((k & 2) ? 0xF0 : 0) | ((k & 8) ? 0x0F : 0));
                }
                t = next;
                vwrite((u16)t << 4, zb_out, 16);
                zb_map[v] = t;
                if (next < 255) ++next;
            }
            row[16 + (c & 15)] = t;
            if ((c & 15) == 15) vwrite((u16)(0x2400 + r * 32 + (c & 16)), row + 16, 16);
        }
    }
    /* attributs (écran 1) : un bloc de 16 x 16 couvre 4 x 4 px du court */
    vram_adr(0x27C0);
    for (r = 0; r < 8; ++r)
        for (c = 0; c < 8; ++c) {
            u8 a = 0, qq, p, sx, sy;
            for (qq = 0; qq < 4; ++qq) {
                sx = (u8)((wx + (c * 2 + (qq & 1)) * 4) >> 3);
                sy = (u8)((wy + (r * 2 + (qq >> 1)) * 4) >> 3);
                if (sy > 29) sy = 29;
                p = (u8)((zb_at[(sy >> 2) * 8 + (sx >> 2)] >> (((sy & 2) << 1) | (sx & 2))) & 3);
                a |= (u8)(p << (qq * 2));
            }
            vram_put(a);
        }
    /* sprites (table $1000) : trace (tuiles 0 et 15), titre (1...) */
    spr_begin(); spr_end();
    mark_build(0x1000, 4);
    hl_n = 0;
    for (j = 0; zl[j]; ++j)
        if ((u8)zl[j] > ' ' && (u8)zl[j] < 0x60)
            chr_copy(BANK_CHR, chr_data + ((u16)(u8)zl[j] << 4), 0x1000 + ((u16)(4 + j) << 4), 16);
    ctrl_swap = 0;                      /* (fond en $0000, sprites en $1000 : comme d'habitude) */
    scroll_x = 0; scroll_y = 0; scroll_nt = 1;
    flash(hl_bk);
    ppu_on();
}
#pragma static-locals(pop)

static void zoom_tick(void)
{
    u8 i, k, d, ob;
    ++hl_t;
    if (hl_t < 16) {
        /* réticule : 4 coins qui se resserrent sur le point visé, par-dessus
           l'image figée (emplacements 60 à 63 des sprites) ; balle
           limite : plus vite (10 trames au lieu de 15) */
        if (hl_t == 1) { hl_bk = pal_buf[0]; vbuf_raw((u16)(0x1000 + ((u16)T_CORNER << 4)), t_corner, 16); if (hl_quick) hl_t = 5; return; }
        d = (u8)(40 - hl_t * 2);
        ob = 240;
        for (k = 0; k < 4; ++k, ob += 4) {
            oam_buf[ob] = (u8)(hl_ry - 1 + ((k & 2) ? d - 8 : -(s8)d));
            oam_buf[ob + 1] = T_CORNER;
            oam_buf[ob + 2] = (u8)(2 | ((k & 1) ? 0x40 : 0) | ((k & 2) ? 0x80 : 0));
            oam_buf[ob + 3] = (u8)(hl_rx + ((k & 1) ? d - 8 : -(s8)d));
        }
        if (hl_t == 13) flash(0x30);   /* (2.90 : zoom sans bip) */
        if (hl_t == 15) zoom_build();
        return;
    }
    /* image agrandie : trace de la balle qui clignote, titre */
    hl_c = (u8)(hl_t - 15);
    if (hl_c == 1) { if (hl_sbin) sfx_crowd(7); else sfx_out(); }
    if (hl_quick && hl_c == Z4) zoom4_build();   /* balle limite : second temps, x 4 (2.90 : sans bip) */
    /* (les sprites agrandis restent ; seuls la trace et le titre changent :
       on les écrit aux emplacements 52 à 63) */
    ob = 208;
    for (i = 0; i < 12; ++i) oam_buf[ob + i * 4] = 0xFF;
    if (hl_c >= 4) {
        /* trace : 2 x 2 sprites centrés sur le centre du pixel de
           rebond (zx, zy : son coin ; + 1 en x 2, + 2 en x 4) */
        d = (u8)(hl_quick && hl_c >= Z4 ? 2 : 1);
        for (k = 0; k < 4; ++k) {
            oam_buf[ob + k * 4] = (u8)(zy + d - 9 + ((k & 2) << 2));   /* (y de l'OAM : une ligne plus haut) */
            oam_buf[ob + k * 4 + 1] = (u8)(hl_n + k); oam_buf[ob + k * 4 + 2] = 3;
            oam_buf[ob + k * 4 + 3] = (u8)(zx + d - 8 + ((k & 1) << 3));
        }
    }
    if (hl_c > 10) {
        /* titre sur une partie unie du court, loin du point visé */
        u8 x = (u8)(128 - lbl_n * 4), ly = (u8)(zy > 120 ? 79 : 159);
        for (i = 0, k = 4; zl[i] && k < 12; ++i, x += 8) {
            if (zl[i] == ' ') continue;
            oam_buf[ob + k * 4] = ly; oam_buf[ob + k * 4 + 1] = (u8)(hl_n + 4 + i);
            oam_buf[ob + k * 4 + 2] = 2; oam_buf[ob + k * 4 + 3] = x;
            ++k;
        }
    }
    if (hl_c >= (hl_quick ? Z4 + 48 : 80)) {
        back_to_court();
        if (hl_quick) { hl_quick = 0; rp_done = 1; }   /* dehors de peu : le ralenti s'arrête là */
    }
}

/* ---------------------------------------------------------------------- */
/* Fin du match (hl_mode 3) : portrait du vainqueur avant     */
/* l'écran des résultats : en pied, bras levés, sous les confettis, son    */
/* nom (« ALEX GAGNE ! » ; en double « VICTOIRE ! »), puis gros plan du    */
/* visage (en double : son partenaire). L'écran des résultats se construit */
/* ensuite : pas de retour au court.                                       */
/* ---------------------------------------------------------------------- */
/* confettis qui tombent (fin du match, champion) */
static void confetti(void)
{
    u8 i;
    for (i = 0; i < 12; ++i)
        spr1((u8)(i * 21 + 6), (u8)(((hl_t * (1 + (i & 1)) + i * 23) & 0xBF) + 8), confetti_tile, (u8)(i & 3));
}
/* fin : tuiles d'origine (l'image a remplacé la police et les décors),
   écran coupé ; l'écran des résultats se construit ensuite */
static void final_end(void)
{
    fade_skip = 0;
    ppu_off();                          /* (fondu au noir) */
    scroll_x = 0; scroll_y = 0; scroll_nt = 0;
    chr_reload();
    bg_dirty = 0; swap_on = 0;
    hl_mode = 0; hl_t = 0;
}
static void final_tick(void)
{
    u8 i, w = sc_winner;
    /* match perdu par le joueur humain contre l'ordinateur : le
       perdant abattu, à genoux (au lieu de la joie du vainqueur) */
    u8 sad = (u8)(cfg.ctrl[w] == CTRL_CPU && cfg.ctrl[w ^ 1] != CTRL_CPU);
    if (hl_t == 0) {
        const char *nm = chr_nm(cfg.chr[w]);
        if (cfg.doubles) nm = "VICTOIRE";
        if (sad) nm = "DEFAITE";
        for (i = 0; nm[i] && i < 8; ++i) hl_txt[i] = nm[i];
        if (!cfg.doubles && !sad) { const char *g = " GAGNE !"; u8 k = 0; while ((hl_txt[i] = g[k]) != 0) { ++i; ++k; } }
        else if (sad) hl_txt[i] = 0;
        else { hl_txt[i++] = ' '; hl_txt[i++] = '!'; hl_txt[i] = 0; }
        hl_sp = (u8)(sad ? w ^ 1 : w);
        flash(sad ? 0x10 : 0x30);
    }
    ++hl_t;
    /* en pied (3-81), poignée de main au filet (82-141), gros plan du visage */
    if (hl_t == 3) cut_build(sad ? TZ_DEFEAT : TZ_VICTORY);
    if (hl_t == 80 || hl_t == 140) flash(sad ? 0x10 : 0x30);   /* (2.90 : plus de carillon) */
    if (hl_t == 82) cut_build(TZ_HANDSHAKE);
    if (hl_t == 142) { if (cfg.doubles) hl_sp = (u8)(sad ? (w ^ 1) + 2 : w + 2); cut_build(sad ? TZ_DEFEAT : TZ_VICTORY_Z); if (!sad) hl_shake = 6; }
    if (hl_t < 4) return;
    if (!sad && (hl_t == 4 || hl_t == 144)) sfx_crowd(7);
    if (hl_t == 96) sfx_clap();
    if (hl_shake) { scroll_y = (u8)((hl_t & 1) ? hl_shake : 0); if ((hl_t & 3) == 0) --hl_shake; } else scroll_y = 0;
    /* plans en pied : arrivée en travelling éclair ; gros plan : fixe */
    hl_c = (u8)(hl_t - (hl_t >= 82 ? 81 : 3));
    hl_cam = (hl_t < 142 && hl_c < 6) ? -(s16)whip_in[hl_c] : 0;
    scroll_x = (u8)hl_cam; scroll_nt = (u8)((hl_cam >> 8) & 1);
    spr_begin();
    if (!sad) confetti();               /* (pas de confettis pour une défaite) */
    if (hl_t >= 14) label_draw(12, (u8)((hl_t - 14) >> 1));
    spr_end();
    if (hl_t >= 220) final_end();
}

/* ---------------------------------------------------------------------- */
/* Ramasseuse de balles (hl_mode 4) : au changement de côté, */
/* un sur deux, un plan télé avant que les joueurs ne retournent sur le    */
/* court ; une balle roule vers elle. A, B ou START passe. Retour au court */
/* sans ralenti.                                                           */
/* ---------------------------------------------------------------------- */
static void kid_end(void)
{
    flash(0x30);
    fade_skip = 1; fade_lvl = 0;
    ppu_off();
    ctrl_swap = 0;
    scroll_x = 0; scroll_y = 0; scroll_nt = 0;
    chr_reload();
    bg_dirty = 0;
    swap_on = 0;
    bank_call(BANK_GAME, court_reload);
    hl_mode = 0; hl_t = 0;
    spr_begin(); spr_end();
    ppu_on();
}
#pragma static-locals(push, off)
static void kid_tick(void)
{
    u8 k;
    s16 sx;
    if (hl_t == 0) {
        static const char nm[] = "LES BALLES";   /* (deux lignes : 8 sprites par ligne au plus) */
        for (k = 0; (hl_txt[k] = nm[k]) != 0; ++k) ;
        if (band_vis) { split_stop(); band_vis = 0; }
        flash(0x30);
    }
    ++hl_t;
    if (hl_t == 3) { cut_build(TZ_BALLKID); sfx_crowd(3); }
    if (hl_t < 4) return;
    if ((hl_t > 20 && (pad1_new & (PAD_A | PAD_B | PAD_START))) || hl_t >= 110) { kid_end(); return; }
    hl_c = (u8)(hl_t - 3);
    hl_cam = hl_c < 6 ? -(s16)whip_in[hl_c] : 0;
    scroll_x = (u8)hl_cam; scroll_nt = (u8)((hl_cam >> 8) & 1);
    spr_begin();
    /* une balle arrive de la droite en rebondissant, puis roule jusqu'à elle */
    if (hl_c >= 10 && hl_c < 70) {
        k = (u8)(hl_c - 10);
        sx = (s16)(TZ_BALLKID_FOOTR_X + 140 - k * 2);
        spr1((u8)sx, (u8)(TZ_BALLKID_FOOTR_Y - 6 - (k < 40 ? (u8)(((k & 15) * (u8)(16 - (k & 15))) >> (k < 16 ? 1 : 2)) : 0)), ball_base + 8, 3);
        if (k < 40 && (k & 15) == 0) sfx_bounce();
    }
    if (hl_c >= 14) label_draw(204, (u8)((hl_c - 14) >> 1));   /* (en bas : la balle levée est en haut de l'image) */
    spr_end();
}
#pragma static-locals(pop)

/* ---------------------------------------------------------------------- */
/* Le champion : après la remise du trophée d'une compétition        */
/* gagnée, image haute définition du joueur qui brandit la coupe d'argent   */
/* au-dessus de sa tête, arrivée en travelling éclair, sous les confettis   */
/* et les flashs des photographes, « CHAMPION ! ». Appelée par l'écran de   */
/* fin de compétition (screens.c) : a = $100 pour commencer, puis la        */
/* manette 1 à chaque trame ; 1 à la fin (A ou START après 1 s, ou 4 s).    */
/* ---------------------------------------------------------------------- */
u16 __fastcall__ champ_tick(u16 a)
{
    u8 i;
    if (a & 0x100) {
        static const char nm[] = "CHAMPION";       /* (8 lettres : une ligne, sur le sol) */
        for (i = 0; (hl_txt[i] = nm[i]) != 0; ++i) ;
        hl_t = 0; hl_sp = 0;
        flash(0x30);
        return 0;
    }
    ++hl_t;
    if (hl_t == 3) { cut_build(TZ_TROPHY); sfx_crowd(7); }   /* (2.90 : plus de carillon) */
    if (hl_t < 4) return 0;
    if ((hl_t > 60 && ((u8)a & (PAD_A | PAD_START))) || hl_t >= 250) return 1;
    hl_c = (u8)(hl_t - 3);
    hl_cam = hl_c < 6 ? -(s16)whip_in[hl_c] : 0;
    scroll_x = (u8)hl_cam; scroll_nt = (u8)((hl_cam >> 8) & 1);
    /* flashs des photographes : le fond s'illumine une trame, par salves */
    i = (u8)(hl_t & 63);
    if (hl_t > 20 && (i == 7 || i == 11 || i == 30 || i == 47 || i == 50)) flash(0x30);
    else if (pal_buf[0] == 0x30 && hl_t > 6) flash(0x01);
    spr_begin();
    confetti();
    if (hl_t >= 16) label_draw(221, (u8)((hl_t - 16) >> 1));   /* (sur le sol, sous les pieds : la coupe est en haut) */
    spr_end();
    if (hl_t == 40 || hl_t == 140) sfx_crowd(7);
    return 0;
}

/* ---------------------------------------------------------------------- */
/* Soleil couchant (hl_mode 5) : au crépuscule, quand la     */
/* lumière change, un plan plein écran du soleil façon jeux de course des  */
/* années 80 (sunset.c, banques 22-27), au changement de côté ; premier   */
/* plan selon la surface (lieux Martinot). A, B ou START passe. Au retour, */
/* le court prend la nouvelle lumière.                                     */
/* ---------------------------------------------------------------------- */
u16 __fastcall__ sun_build(u16 ph);    /* sunset.c, banque 22 */
static void sun_end(void)
{
    sun_pend = 0;                       /* (court_reload -> sun_apply : la phase réelle) */
    kid_end();
}
static void sun_tick(void)
{
    u8 k;
    if (hl_t == 0) {
        static const char * const nm[3] = { "SOLEIL COUCHANT", "SOLEIL ROUGE", "LA NUIT TOMBE" };
        const char *t;
        k = (u8)far_call(BANK_SUN, (void *)sun_calc_f, 0);
        t = nm[k ? k - 1 : 0];
        for (k = 0; (hl_txt[k] = t[k]) != 0; ++k) ;
        if (band_vis) { split_stop(); band_vis = 0; }
        flash(0x0F);
    }
    ++hl_t;
    if (hl_t == 3) {
        k = (u8)far_call(BANK_SUN, (void *)sun_calc_f, 0);
        fade_skip = 1; fade_lvl = 0;
        ppu_off();
        ppu_emph = 0;
        far_call(BANK_SUN, (void *)sun_build, (u16)(cfg.surface * 3 + (k ? k - 1 : 0)));   /* (premier plan : la surface, le lieu) */
        pal_buf[16] = 0x0F;
        pal_buf[16 + 9] = 0x0F; pal_buf[16 + 10] = 0x0F; pal_buf[16 + 11] = 0x30;   /* titre : blanc */
        pal_dirty = 1;
        hl_cam = 0; scroll_x = 0; scroll_y = 0; scroll_nt = 0;
        spr_begin(); spr_end();
        label_load();
        ppu_on();
    }
    if (hl_t < 4) return;
    if ((hl_t > 30 && (pad1_new & (PAD_A | PAD_B | PAD_START))) || hl_t >= 170) { sun_end(); return; }
    /* plage : le reflet du soleil scintille sur la mer */
    if (cfg.surface == 4 && (hl_t & 7) == 0) {
        pal_buf[14] = (u8)(pal_buf[14] ^ 0x10);
        pal_dirty = 1;
    }
    spr_begin();
    if (hl_t >= 16) label_draw(18, (u8)((hl_t - 16) >> 1));
    spr_end();
}

void hl_tick(void)
{
    if (hl_mode == 5) { sun_tick(); return; }
    if (hl_mode == 4) { kid_tick(); return; }
    if (hl_mode == 3) {
        if (hl_t > 20 && (pad1_new & (PAD_A | PAD_START))) { final_end(); return; }
        final_tick();
        return;
    }
    if (pad1_new & (PAD_A | PAD_B | PAD_START)) {   /* passer : fin du ralenti */
        back_to_court();
        rp_done = 1; hl_sok = hl_sbok = 0; hl_t = 0;
        return;
    }
    if (hl_t == 0 && hl_mode == 2) {
        /* point visé : le rebond, à l'écran (projeté en perspective) ; texte */
        hl_rx = hl_sbx; hl_ry = hl_sby;
        if (persp) { proj2(hl_sbx, hl_sby); hl_rx = proj_x; hl_ry = proj_y; }   /* (réticule, sur l'image en perspective) */
        zl = !hl_sbin ? "DEHORS !" : far_call(BANK_X2, (void *)near_in_f, (u16)(hl_sbx | (hl_sby << 8))) ? "BONNE !" : "";   /* (« BONNE ! » seulement près de la ligne) */
    }
    if (hl_mode == 1) closeup_tick(); else zoom_tick();
    if (!hl_mode) hl_t = 0;
}

/* ---------------------------------------------------------------------- */
/* Filet qui tremble en perspective                         */
/* Le générateur liste, pour chaque lieu, les tuiles dont le dessin change */
/* quand le filet est secoué (banque 16 : nombre, puis numéro, dessin au   */
/* repos, dessin secoué). Le filet s'affaisse tant que net_shake > 6, puis  */
/* revient ; chaque changement est écrit par le tampon VRAM, sur une ou     */
/* plusieurs trames (budget du VBlank), tuile après tuile.                  */
/* ---------------------------------------------------------------------- */
/* (pnet_tick : train.c, banque 20, banque 15 pleine) */

/* (remise du trophée : pod_extra_build, pod_extra_tick : train.c, banque 20) */
#include "comp.h"

/* (palmares_sprites : train.c, banque 20, banque 15 pleine) */

/* (win_msg : train.c, banque 20) */
