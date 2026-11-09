/* Royal Martinot — palmarès (CDC 12.1-12.2) dans la RAM de cartouche ($7800). */
#include "stats.h"
#include "rules.h"
#include "comp.h"

#pragma static-locals(on)
#pragma bss-name(push, "SAVEDATA")
SaveData sv;
SaveOpts so;
SaveCanon sx;
#pragma bss-name(pop)

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

static u8 so_sum(void)
{
    u8 i, s = 0x5A;
    for (i = 0; i < NOPT_SAVE; ++i) s = (u8)(((s << 1) | (s >> 7)) ^ so.opt[i]);
    return s;
}

u8 __fastcall__ so_save(u8 unused)
{
    u8 i;
    for (i = 0; i < NOPT_SAVE; ++i) so.opt[i] = opt_val[i];
    so.sig[0] = 'P'; so.sig[1] = 'O';
    so.sum = so_sum();
    return unused;
}

u8 __fastcall__ so_load(u8 unused)
{
    u8 i;
    (void)unused;
    if (so.sig[0] != 'P' || so.sig[1] != 'O' || so.sum != so_sum()) return 0;
    for (i = 0; i < NOPT_SAVE; ++i) opt_val[i] = so.opt[i];
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
    return unused;
}

u8 __fastcall__ sv_check(u8 unused)
{
    if (sv.sig[0] != 'P' || sv.sig[1] != 'X' || sv.sig[2] != 'O' || sv.sig[3] != '1' || sv.sum != compute_sum())
        sv_clear(unused);
    sx_check();
    return 0;
}

static void add16(u16 *v, u8 n) { *v = (u16)(*v + n < *v ? 0xFFFF : *v + n); }

u8 __fastcall__ sv_after_match(u8 unused)
{
    u8 d = cfg.doubles;
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
    if (!sv.comp_best[type] || rank < sv.comp_best[type]) sv.comp_best[type] = rank;
    /* historique : les 8 dernières compétitions, la plus récente en tête */
    for (i = 7; i > 0; --i) {
        sv.hist[i][0] = sv.hist[i - 1][0]; sv.hist[i][1] = sv.hist[i - 1][1];
        sv.hist[i][2] = sv.hist[i - 1][2]; sv.hist[i][3] = sv.hist[i - 1][3];
    }
    sv.hist[0][0] = type; sv.hist[0][1] = rank; sv.hist[0][2] = chr; sv.hist[0][3] = surf;
    if (sv.hist_n < 8) ++sv.hist_n;
    commit();
    return 0;
}
