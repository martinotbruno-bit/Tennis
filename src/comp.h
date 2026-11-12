/* Royal Martinot — compétitions (banque des écrans). */
#ifndef COMP_H
#define COMP_H
#include "runtime.h"

#define CT_KO      0   /* tournoi à élimination directe, 8 joueurs */
#define CT_GROUPS  1   /* 2 poules de 4, demi-finales, finale */
#define CT_LEAGUE  2   /* championnat toutes rondes, 6 joueurs */
#define CT_SEASON  3   /* saison : 4 tournois (4 surfaces), classement aux points */
#define CT_CUSTOM  4   /* tournoi personnalisé */
#define CT_GRAND   5   /* le Grand Martinot : 4 tournois de 16 (8 équipes en double) */
#define CT_WORLD   6   /* le Tour du monde : 7 tournois de 8, un par surface (version 2.47) */
#define NCT        7
u8 __fastcall__ comp_cname(u8 t);           /* banque 5 : nom de la compétition t dans comp_l2 */

/* options du tournoi personnalisé (bits de c_opts) */
#define CO_SIZE8   1   /* 8 joueurs (sinon 4) */
#define CO_PROG    2   /* adversaires de plus en plus forts */
#define CO_VARIED  4   /* surface différente à chaque tour */

extern u8 c_active, c_type, c_stage, c_seed, c_results, c_trophies, c_opts, c_lvl, c_over, c_rank;
extern u8 c_surf, c_env, c_cond, c_fmt;
extern u8 c_pl[16], c_rt[16], c_n;
extern u8 c_pl2[16], c_dbl, c_hum2;
extern u16 c_gres;   /* tours gagnés à chaque tournoi : Grand Martinot 3 bits, Tour du monde 2 bits */
extern u8 c_slam;    /* Grand Chelem : tous les tournois gagnés (Grand Martinot, Tour du monde) */

extern char comp_l1[32], comp_l2[32];   /* libellés de l'étape (comp_stage_name) */

/* Banque BANK_COMP : appel depuis une autre banque par far_call(). */
u8 __fastcall__ comp_new(u8 unused);        /* c_type, c_pl[0], c_seed déjà réglés */
u8 __fastcall__ comp_build(u8 unused);
u8 __fastcall__ comp_opponent(u8 unused);
u8 __fastcall__ comp_surface(u8 unused);
u8 __fastcall__ comp_level(u8 unused);
u8 __fastcall__ comp_record(u8 won);
u8 __fastcall__ comp_stage_name(u8 unused); /* remplit comp_l1 / comp_l2 */
u8 __fastcall__ comp_draw_table(u8 y);
u8 __fastcall__ comp_podium(u8 unused);    /* remplit c_pod (compétition gagnée) */
u8 __fastcall__ comp_desc(u8 k);
u8 __fastcall__ comp_gm_intro(u8 unused);   /* Grand Martinot, Tour du monde : tournoi à annoncer, ou $FF */
extern u8 c_pod[3];
#define COMP(fn, arg) ((u8)far_call(BANK_COMP, (void *)(fn), (arg)))
/* codes de reprise (banque 5) : renseigner pw_s (13 signes) et pw_p (PwData) */
extern char *pw_s;
extern void *pw_p;
u8 __fastcall__ pw_enc_far(u8 unused);
u8 __fastcall__ pw_dec_far(u8 unused);
#endif
