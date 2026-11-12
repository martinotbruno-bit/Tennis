/* Royal Martinot — palmarès et statistiques cumulées (RAM de cartouche).
 * Conservés tant que la console est allumée ; sur une cartouche avec pile,
 * conservés aussi à l'extinction (signature + somme de contrôle). */
#ifndef STATS_H
#define STATS_H
#include "runtime.h"
#include "game.h"

typedef struct {
    u8 sig[4];
    u16 played[2], won[2];         /* simple, double */
    u16 pts_w, pts_l, aces, dfaults, winners, errors, fs_in, fs_tot;
    u8 titles, comps, best_rally, best_aces;
    u8 best_train[NEX];
    u8 hist_n;
    u8 hist[8][4];                 /* type, rang, personnage, surface */
    u8 comp_best[5];               /* meilleur rang par type (0 = jamais joué) */
    u8 nude;                       /* club nudiste débloqué */
    u8 sum;
} SaveData;

extern SaveData sv;
extern u8 sw_best[4];   /* meilleurs rangs des types 5 et 6 : sw_best[1], sw_best[2] */

/* derniers réglages du menu, dans un bloc à part (le palmarès garde sa
   disposition et n'est pas effacé) */
#define NOPT_SAVE 24
typedef struct {
    u8 sig[2];
    u8 opt[NOPT_SAVE];
    u8 sum;
} SaveOpts;
extern SaveOpts so;

/* services canon (palmarès), bloc à part : la disposition du palmarès ne
   change pas et les sauvegardes existantes restent valables */
typedef struct {
    u8 sig[2];
    u16 ok, tried;      /* services canon réussis / services A */
    u16 pok, ptry;      /* frappes parfaites / coups lifté, à plat, coupé */
    u8 sum;
} SaveCanon;
extern SaveCanon sx;
void sx_check(void);
extern u8 opt_val[NOPT_SAVE + 1];    /* + VUE, mémorisée à part (sw_view) */
u8 __fastcall__ so_load(u8 unused);    /* 1 si des réglages valides ont été copiés dans opt_val */
u8 __fastcall__ so_save(u8 unused);
/* Banque BANK_COMP : appel par far_call() */
u8 __fastcall__ sv_check(u8 unused);
u8 __fastcall__ sv_after_match(u8 unused);
u8 __fastcall__ sv_after_train(u8 unused);
u8 __fastcall__ sv_comp_end(u8 unused);   /* d'après c_type, c_rank, c_pl[0], c_surf */
u8 __fastcall__ sv_clear(u8 unused);
#define SV(fn) ((void)far_call(BANK_COMP, (void *)(fn), 0))
#endif
