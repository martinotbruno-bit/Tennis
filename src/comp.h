/* Royal Martinot — compétitions (banque des écrans). */
#ifndef COMP_H
#define COMP_H
#include "runtime.h"

#define CT_KO      0   /* tournoi à élimination directe, 8 joueurs */
#define CT_GROUPS  1   /* 2 poules de 4, demi-finales, finale */
#define CT_LEAGUE  2   /* championnat toutes rondes, 6 joueurs */
#define CT_SEASON  3   /* saison : 4 tournois (4 surfaces), classement aux points */
#define CT_CUSTOM  4   /* tournoi personnalisé */
#define NCT        5
extern const char * const comp_name[NCT];

/* options du tournoi personnalisé (bits de c_opts) */
#define CO_SIZE8   1   /* 8 joueurs (sinon 4) */
#define CO_PROG    2   /* adversaires de plus en plus forts */
#define CO_VARIED  4   /* surface différente à chaque tour */

extern u8 c_active, c_type, c_stage, c_seed, c_results, c_trophies, c_opts, c_lvl, c_over, c_rank;
extern u8 c_surf, c_env, c_cond, c_fmt;
extern u8 c_pl[8], c_rt[8], c_n;
extern u8 c_pl2[8], c_dbl, c_hum2;

extern char comp_l1[24], comp_l2[24];   /* libellés de l'étape (comp_stage_name) */

/* Banque BANK_COMP : appel depuis une autre banque par far_call(). */
u8 __fastcall__ comp_new(u8 unused);        /* c_type, c_pl[0], c_seed déjà réglés */
u8 __fastcall__ comp_build(u8 unused);
u8 __fastcall__ comp_opponent(u8 unused);
u8 __fastcall__ comp_surface(u8 unused);
u8 __fastcall__ comp_level(u8 unused);
u8 __fastcall__ comp_record(u8 won);
u8 __fastcall__ comp_stage_name(u8 unused); /* remplit comp_l1 / comp_l2 */
u8 __fastcall__ comp_draw_table(u8 y);
#define COMP(fn, arg) ((u8)far_call(BANK_COMP, (void *)(fn), (arg)))
/* codes de reprise (banque 5) : renseigner pw_s (13 signes) et pw_p (PwData) */
extern char *pw_s;
extern void *pw_p;
u8 __fastcall__ pw_enc_far(u8 unused);
u8 __fastcall__ pw_dec_far(u8 unused);
#endif
