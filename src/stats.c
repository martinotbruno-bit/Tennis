/* Royal Martinot — palmarès (CDC 12.1-12.2) dans la RAM de cartouche ($7800). */
#include "stats.h"
#include "rules.h"
#include "comp.h"

#pragma static-locals(on)
#pragma bss-name(push, "SAVEDATA")
SaveData sv;
SaveOpts so;
SaveCanon sx;
/* vue (option VUE), bloc à part : les réglages déjà mémorisés restent valables */
u8 sw_view[3];          /* 'V', valeur, contrôle */
/* meilleur rang du Grand Martinot et du Martinworld Tour (types 5 et 6), bloc à
   part : SaveData n'a de place que pour les types 0 à 4 */
u8 sw_best[4];          /* 'B', type 5, type 6, contrôle */
/* record du service le plus rapide (2.71), bloc à part : 'S', km/h - 80, personnage, contrôle */
u8 sw_spd[4];
/* face-à-face (2.72) : 'H', puis pour chaque personnage adverse vos victoires
   (4 bits hauts) et défaites (4 bits bas) contre lui, en simple, contrôle */
u8 sw_h2h[NCHARS + 2];
/* carrière (2.72) : 'C', points (2 octets), compétitions, titres, meilleur
   classement, derniers points gagnés (2 octets), option CARRIERE, -, -, contrôle */
u8 sw_car[12];
#pragma bss-name(pop)
u8 sp_new;              /* 1 : record battu au dernier match (écran des résultats) */
extern u8 st_vmax[2], st_vmaxp[2];

static u8 sx_sum(void)
{
    return (u8)(0x3C ^ (u8)sx.ok ^ (u8)(sx.ok >> 8) ^ (u8)(sx.tried << 1) ^ (u8)(sx.tried >> 8)
                ^ (u8)(sx.pok << 2) ^ (u8)(sx.pok >> 8) ^ (u8)(sx.ptry << 3) ^ (u8)(sx.ptry >> 8));
}

void sx_check(void)
{
    if (sx.sig[0] != 'P' || sx.sig[1] != 'D' || sx.sum != sx_sum()) {
        sx.sig[0] = 'P'; sx.sig[1] = 'D'; sx.ok = sx.tried = sx.pok = sx.ptry = 0; sx.sum = sx_sum();
    }
}

static u8 sb_sum(void) { return (u8)(0xA5 ^ sw_best[1] ^ (u8)(sw_best[2] << 1)); }
static u8 sp_sum(void) { return (u8)(0x5C ^ sw_spd[1] ^ (u8)(sw_spd[2] << 3)); }
static void sp_check(void)
{
    if (sw_spd[0] != 'S' || sw_spd[3] != sp_sum()) { sw_spd[0] = 'S'; sw_spd[1] = sw_spd[2] = 0; sw_spd[3] = sp_sum(); }
}
static u8 blk_sum(const u8 *b, u8 n) { u8 s = 0x3B; while (n--) s = (u8)(((s << 1) | (s >> 7)) ^ *b++); return s; }
static void hc_check(void)
{
    u8 i;
    if (sw_h2h[0] != 'H' || sw_h2h[NCHARS + 1] != blk_sum(sw_h2h, NCHARS + 1)) {
        sw_h2h[0] = 'H'; for (i = 1; i <= NCHARS; ++i) sw_h2h[i] = 0; sw_h2h[NCHARS + 1] = blk_sum(sw_h2h, NCHARS + 1);
    }
    if (sw_car[0] != 'C' || sw_car[11] != blk_sum(sw_car, 11)) {
        for (i = 1; i < 11; ++i) sw_car[i] = 0;
        sw_car[0] = 'C'; sw_car[11] = blk_sum(sw_car, 11);
    }
}
static void car_commit(void) { sw_car[11] = blk_sum(sw_car, 11); }
static void sb_check(void)
{
    if (sw_best[0] != 'B' || sw_best[3] != sb_sum()) { sw_best[0] = 'B'; sw_best[1] = sw_best[2] = 0; sw_best[3] = sb_sum(); }
}

static u8 so_sum(void)
{
    u8 i, s = 0x5A;
    for (i = 0; i < NOPT_SAVE; ++i) s = (u8)(((s << 1) | (s >> 7)) ^ so.opt[i]);
    return s;
}

void car_comp_end(void);
u8 __fastcall__ so_save(u8 unused)
{
    u8 i;
    hc_check(); sw_car[8] = opt_val[NOPT_SAVE + 1]; car_commit();   /* option CARRIERE (2.72) */
    for (i = 0; i < NOPT_SAVE; ++i) so.opt[i] = opt_val[i];
    so.sig[0] = 'P'; so.sig[1] = 'O';
    sw_view[0] = 'V'; sw_view[1] = opt_val[NOPT_SAVE]; sw_view[2] = (u8)(0x5A ^ opt_val[NOPT_SAVE]);
    so.sum = so_sum();
    return unused;
}

u8 __fastcall__ so_load(u8 unused)
{
    u8 i;
    (void)unused;
    if (so.sig[0] != 'P' || so.sig[1] != 'O' || so.sum != so_sum()) return 0;
    for (i = 0; i < NOPT_SAVE; ++i) opt_val[i] = so.opt[i];
    if (sw_view[0] == 'V' && sw_view[1] < 2 && sw_view[2] == (u8)(0x5A ^ sw_view[1])) opt_val[NOPT_SAVE] = sw_view[1];
    hc_check(); opt_val[NOPT_SAVE + 1] = (u8)(sw_car[8] == 1);
    return 1;
}

static u8 compute_sum(void)
{
    const u8 *p = (const u8 *)&sv;
    u8 s = 0x6B;
    u16 i;
    for (i = 0; i < sizeof(SaveData) - 1; ++i) s = (u8)(((s << 1) | (s >> 7)) ^ p[i]);
    return s;
}

static void commit(void) { sv.sum = compute_sum(); }

u8 __fastcall__ sv_clear(u8 unused)
{
    u8 *p = (u8 *)&sv;
    u16 i;
    for (i = 0; i < sizeof(SaveData); ++i) p[i] = 0;
    sv.sig[0] = 'P'; sv.sig[1] = 'X'; sv.sig[2] = 'O'; sv.sig[3] = '1';
    commit();
    sx.sig[0] = 0; sx_check();             /* services canon remis à zéro */
    sw_best[0] = 0; sb_check();
    sw_spd[0] = 0; sp_check();
    sw_h2h[0] = sw_car[0] = 0; hc_check();
    return unused;
}

u8 __fastcall__ sv_check(u8 unused)
{
    if (sv.sig[0] != 'P' || sv.sig[1] != 'X' || sv.sig[2] != 'O' || sv.sig[3] != '1' || sv.sum != compute_sum())
        sv_clear(unused);
    sx_check();
    sb_check();
    sp_check();
    hc_check();
    return 0;
}

static void add16(u16 *v, u8 n) { *v = (u16)(*v + n < *v ? 0xFFFF : *v + n); }

u8 __fastcall__ sv_after_match(u8 unused)
{
    u8 d = cfg.doubles;
    sp_new = 0;
    /* seules les parties où l'équipe 1 est jouée par un humain comptent */
    if (cfg.demo || cfg.ctrl[0] == CTRL_CPU || cfg.mode != GM_MATCH) return 0;
    sv_check(unused);
    add16(&sv.played[d], 1);
    if (sc_winner == 0) add16(&sv.won[d], 1);
    add16(&sv.pts_w, st_points[0]);
    add16(&sv.pts_l, st_points[1]);
    add16(&sv.aces, st_aces[0]);
    add16(&sv.dfaults, st_dfaults[0]);
    add16(&sv.winners, st_winners[0]);
    add16(&sv.errors, st_errors[0]);
    add16(&sv.fs_in, st_first_in[0]);
    add16(&sv.fs_tot, st_first_tot[0]);
    if (st_longest > sv.best_rally) sv.best_rally = st_longest;
    if (st_aces[0] > sv.best_aces) sv.best_aces = st_aces[0];
    commit();
    if (!d && cfg.ctrl[1] == CTRL_CPU) {   /* face-à-face (simple, contre l'ordinateur) */
        u8 *h = &sw_h2h[1 + cfg.chr[1]];
        if (sc_winner == 0) { if (*h < 0xF0) *h += 0x10; }
        else if ((*h & 15) < 15) ++*h;
        sw_h2h[NCHARS + 1] = blk_sum(sw_h2h, NCHARS + 1);
    }
    if (st_vmax[0] > sw_spd[1]) {          /* service le plus rapide (joueur humain de l'équipe 1) */
        sw_spd[1] = st_vmax[0]; sw_spd[2] = cfg.chr[st_vmaxp[0]]; sw_spd[3] = sp_sum(); sp_new = 1;
    }
    sx_check();
    add16(&sx.ok, st_canon[0]);
    add16(&sx.tried, st_ctry[0]);
    add16(&sx.pok, st_perf[0]);
    add16(&sx.ptry, st_ptry[0]);
    sx.sum = sx_sum();
    return 0;
}

u8 __fastcall__ sv_after_train(u8 unused)
{
    sv_check(unused);
    if (tr_score > sv.best_train[cfg.ex]) sv.best_train[cfg.ex] = tr_score;
    commit();
    return 0;
}

u8 __fastcall__ sv_comp_end(u8 unused)
{
    u8 i, type = c_type, rank = c_rank, chr = c_pl[0], surf = c_surf;
    sv_check(unused);
    if (sv.comps < 255) ++sv.comps;
    if (rank == 1 && sv.titles < 255) ++sv.titles;
    if (rank == 1) sv.nude = 1;
    {
        u8 *b = type < 5 ? &sv.comp_best[type] : &sw_best[type - 4];
        if (!*b || rank < *b) *b = rank;
        sw_best[3] = sb_sum();
    }
    /* historique : les 8 dernières compétitions, la plus récente en tête */
    for (i = 7; i > 0; --i) {
        sv.hist[i][0] = sv.hist[i - 1][0]; sv.hist[i][1] = sv.hist[i - 1][1];
        sv.hist[i][2] = sv.hist[i - 1][2]; sv.hist[i][3] = sv.hist[i - 1][3];
    }
    sv.hist[0][0] = type; sv.hist[0][1] = rank; sv.hist[0][2] = chr; sv.hist[0][3] = surf;
    if (sv.hist_n < 8) ++sv.hist_n;
    commit();
    bank_call(BANK_CAR, car_comp_end);   /* carrière : points et classement (career.c) */
    return 0;
}
