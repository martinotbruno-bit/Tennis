/* Pixel Open — palmarès (CDC 12.1-12.2) dans la RAM de cartouche ($7800). */
#include "stats.h"
#include "rules.h"
#include "comp.h"

#pragma static-locals(on)
#pragma bss-name(push, "SAVEDATA")
SaveData sv;
#pragma bss-name(pop)

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
    return unused;
}

u8 __fastcall__ sv_check(u8 unused)
{
    if (sv.sig[0] != 'P' || sv.sig[1] != 'X' || sv.sig[2] != 'O' || sv.sig[3] != '1' || sv.sum != compute_sum())
        sv_clear(unused);
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
