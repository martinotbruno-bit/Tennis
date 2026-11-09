/* Pixel Open — compétitions : tournoi, poules, championnat, saison et
 * tournoi personnalisé (CDC 4.2).
 *
 * Tout ce qui ne dépend pas du joueur est déterminé par une graine de 8 bits :
 * le tirage des adversaires, leur niveau et les résultats des matchs simulés
 * entre joueurs contrôlés par l'ordinateur. Un code de reprise n'a donc besoin
 * que de la graine, de l'étape et des résultats du joueur. */
#include "comp.h"
#include "gfx.h"
#include "app.h"

#pragma static-locals(on)

/* comp_name[] est dans la banque fixe (gfx.c) : lu depuis plusieurs banques */

u8 c_active, c_type, c_stage, c_seed, c_results, c_trophies, c_opts, c_lvl, c_over, c_rank;
u8 c_surf, c_env, c_cond, c_fmt;
u8 c_pl[8], c_rt[8], c_n;
u8 c_pl2[8], c_dbl, c_hum2;   /* double : partenaire de chaque inscrit */
char comp_l1[24], comp_l2[24];

static u16 rs;
static void rs_seed(u8 a, u8 b) { rs = (u16)(((u16)a << 8) | b) ^ 0xACE1; if (!rs) rs = 1; }
static u8 rn(void)
{
    rs ^= rs << 7;
    rs ^= rs >> 9;
    rs ^= rs << 8;
    return (u8)rs;
}

/* a (index d'inscrit, jamais le joueur) bat-il b ? Probabilité selon le niveau */
static u8 sim_win(u8 a, u8 b, u8 salt)
{
    s16 p = 128 + ((s16)c_rt[a] - (s16)c_rt[b]) * 50;
    rs_seed((u8)(c_seed ^ (a << 4) ^ b), (u8)(salt * 29 + 7));
    rn();
    return (u8)((s16)rn() < p);
}

static u8 nrounds(void) { return (u8)(c_n == 8 ? 3 : c_n == 4 ? 2 : 0); }

u8 __fastcall__ comp_build(u8 unused)
{
    static const u8 rt8[7] = { 2, 2, 1, 1, 1, 0, 0 };
    static const u8 rt6[5] = { 2, 1, 1, 0, 0 };
    static const u8 rt4[3] = { 2, 1, 0 };
    u8 pool[NCHARS], np = 0, i, j, t;
    c_n = 8;
    if (c_type == CT_LEAGUE) c_n = 6;
    if (c_type == CT_CUSTOM && !(c_opts & CO_SIZE8)) c_n = 4;
    /* adversaires tirés parmi les 16 personnages d'origine : Margaux (16) est
       la joueuse que l'on choisit, jamais une adversaire tirée au sort ; les
       anciens codes redonnent ainsi exactement les mêmes tableaux */
    for (i = 0; i < 16; ++i) if (i != c_pl[0] && (!c_dbl || i != c_pl2[0])) pool[np++] = i;
    rs_seed(c_seed, 0x33);
    for (i = np - 1; i > 0; --i) { j = (u8)(rn() % (u8)(i + 1)); t = pool[i]; pool[i] = pool[j]; pool[j] = t; }
    c_rt[0] = 1;
    if (!c_dbl) c_pl2[0] = 255;
    for (i = 1; i < c_n; ++i) {
        if (c_dbl) { c_pl[i] = pool[(i - 1) * 2]; c_pl2[i] = pool[(i - 1) * 2 + 1]; }
        else { c_pl[i] = pool[i - 1]; c_pl2[i] = 255; }
        c_rt[i] = c_n == 8 ? rt8[i - 1] : c_n == 6 ? rt6[i - 1] : rt4[i - 1];
    }
    /* mélange des niveaux pour que les têtes de série ne soient pas toujours au même endroit */
    for (i = (u8)(c_n - 1); i > 1; --i) { j = (u8)(1 + rn() % i); t = c_rt[i]; c_rt[i] = c_rt[j]; c_rt[j] = t; }
    (void)unused;
    c_active = 1;
    c_over = 0;
    c_rank = 0;
    return 0;
}

u8 __fastcall__ comp_new(u8 unused)
{
    c_stage = 0;
    c_results = 0;
    /* c_trophies : cumulé sur la session (et transmis par les codes) */
    return comp_build(unused);
}

/* ---------------------------------------------------------------------- */
/* Tableau à élimination directe : joueurs encore en lice au tour « upto »
 * de l'épreuve ev ; hr = nombre de tours gagnés par le joueur (index 0). */
static u8 ko_cur[8];
static void ko_run(u8 ev, u8 hr, u8 upto)
{
    u8 m = c_n, r, i, a, b, w, t, j;
    for (i = 0; i < c_n; ++i) ko_cur[i] = i;
    if (ev) {
        /* saison : un tableau différent pour chaque tournoi */
        rs_seed(c_seed, (u8)(0x40 + ev));
        for (i = (u8)(c_n - 1); i > 1; --i) { j = (u8)(1 + rn() % i); t = ko_cur[i]; ko_cur[i] = ko_cur[j]; ko_cur[j] = t; }
    }
    for (r = 0; r < upto && m > 1; ++r) {
        for (i = 0; i < (u8)(m >> 1); ++i) {
            a = ko_cur[i * 2]; b = ko_cur[i * 2 + 1];
            if (a == 0 || b == 0) w = (u8)((r < hr) == (a == 0) ? a : b);
            else w = sim_win(a, b, (u8)(ev * 16 + r * 4 + i)) ? a : b;
            ko_cur[i] = w;
        }
        m >>= 1;
    }
}
/* index du tour atteint par chaque inscrit dans l'épreuve ev (saison) */
static u8 reach[8];
static void ko_reach(u8 ev, u8 hr)
{
    u8 r, i, k;
    for (i = 0; i < c_n; ++i) reach[i] = 0;
    for (r = 1; r <= 3; ++r) {
        ko_run(ev, hr, r);
        for (k = 0; k < (u8)(8 >> r); ++k) reach[ko_cur[k]] = r;
    }
}

/* ---------------------------------------------------------------------- */
/* Poules et championnat : calendriers toutes rondes */
static const u8 g4[3][4] = { { 0, 1, 2, 3 }, { 0, 2, 1, 3 }, { 0, 3, 1, 2 } };
static u8 lg_pair(u8 d, u8 k, u8 *a, u8 *b)
{
    /* méthode du cercle à 6 : 0 fixe, 1..5 tournent */
    u8 arr[5], i;
    for (i = 0; i < 5; ++i) arr[i] = (u8)(1 + (i + d) % 5);
    if (k == 0) { *a = 0; *b = arr[0]; }
    else if (k == 1) { *a = arr[1]; *b = arr[4]; }
    else { *a = arr[2]; *b = arr[3]; }
    return 0;
}
static u8 wins[8];
/* résultat de a contre b ; joueur : bit du jour dans c_results */
static u8 rr_win(u8 a, u8 b, u8 day, u8 salt)
{
    if (a == 0) return (u8)((c_results >> day) & 1);
    if (b == 0) return (u8)(!((c_results >> day) & 1));
    return sim_win(a, b, salt);
}
static void groups_wins(u8 days)
{
    u8 d, g, k, a, b, base;
    for (k = 0; k < 8; ++k) wins[k] = 0;
    for (d = 0; d < days; ++d)
        for (g = 0; g < 2; ++g) {
            base = (u8)(g * 4);
            for (k = 0; k < 2; ++k) {
                a = (u8)(base + g4[d][k * 2]); b = (u8)(base + g4[d][k * 2 + 1]);
                if (rr_win(a, b, d, (u8)(d * 8 + g * 2 + k))) ++wins[a]; else ++wins[b];
            }
        }
}
static void league_wins(u8 days)
{
    u8 d, k, a, b;
    for (k = 0; k < 8; ++k) wins[k] = 0;
    for (d = 0; d < days; ++d)
        for (k = 0; k < 3; ++k) {
            lg_pair(d, k, &a, &b);
            if (rr_win(a, b, d, (u8)(d * 8 + k))) ++wins[a]; else ++wins[b];
        }
}
/* confrontation directe dans le championnat : a a-t-il battu b ? */
static u8 lg_h2h(u8 a, u8 b)
{
    u8 d, k, x, y;
    for (d = 0; d < 5; ++d)
        for (k = 0; k < 3; ++k) {
            lg_pair(d, k, &x, &y);
            if (x == a && y == b) return rr_win(a, b, d, (u8)(d * 8 + k));
            if (x == b && y == a) return (u8)!rr_win(b, a, d, (u8)(d * 8 + k));
        }
    return 0;
}
/* classement : victoires (ou points), puis confrontation directe, puis niveau */
static u8 order[8];
static u16 pts[8];
static void rank_by(u8 from, u8 n, u8 use_pts)
{
    u8 i, j, k, a, better;
    for (i = 0; i < n; ++i) order[i] = (u8)(from + i);
    for (i = 1; i < n; ++i) {
        k = order[i];
        for (j = i; j > 0; --j) {
            a = order[j - 1];
            if (use_pts) better = (u8)(pts[k] > pts[a] || (pts[k] == pts[a] && c_rt[k] > c_rt[a]));
            else better = (u8)(wins[k] > wins[a] ||
                (wins[k] == wins[a] && (c_type == CT_LEAGUE ? lg_h2h(k, a) : c_rt[k] > c_rt[a])));
            if (!better) break;
            order[j] = a;
        }
        order[j] = k;
    }
}
static u8 group_rank_of_human(void)
{
    u8 i;
    groups_wins(3);
    rank_by(0, 4, 0);
    for (i = 0; i < 4; ++i) if (order[i] == 0) return i;
    return 3;
}
/* demi-finales des poules : A1-B2 et B1-A2 */
static u8 sf_a[2], sf_b[2];
static void groups_semis(void)
{
    u8 a1, a2, b1, b2;
    groups_wins(3);
    rank_by(0, 4, 0); a1 = order[0]; a2 = order[1];
    rank_by(4, 4, 0); b1 = order[0]; b2 = order[1];
    sf_a[0] = a1; sf_b[0] = b2;
    sf_a[1] = b1; sf_b[1] = a2;
}

/* points de saison selon le tour atteint (0 : perdu en quart .. 3 : vainqueur) */
static const u8 season_pts[4] = { 45, 90, 150, 250 };
static void season_points(u8 events)
{
    u8 e, i, hr;
    for (i = 0; i < 8; ++i) pts[i] = 0;
    for (e = 0; e < events; ++e) {
        hr = (u8)((c_results >> (e * 2)) & 3);
        ko_reach(e, hr);
        reach[0] = hr;
        for (i = 0; i < 8; ++i) pts[i] += season_pts[reach[i] > 3 ? 3 : reach[i]];
    }
}

/* ---------------------------------------------------------------------- */
u8 __fastcall__ comp_opponent(u8 unused)
{
    u8 d, k, a, b;
    (void)unused;
    switch (c_type) {
    case CT_GROUPS:
        if (c_stage < 3) {
            d = c_stage;
            return (u8)(g4[d][0] == 0 ? g4[d][1] : g4[d][0]);
        }
        groups_semis();
        if (c_stage == 3) return sf_a[0] == 0 ? sf_b[0] : sf_a[1] == 0 ? sf_b[1] : sf_b[1] == 0 ? sf_a[1] : sf_a[0];
        /* finale : vainqueur de l'autre demi-finale */
        k = (u8)(sf_a[0] == 0 || sf_b[0] == 0 ? 1 : 0);
        return sim_win(sf_a[k], sf_b[k], 0x70) ? sf_a[k] : sf_b[k];
    case CT_LEAGUE:
        for (k = 0; k < 3; ++k) {
            lg_pair(c_stage, k, &a, &b);
            if (a == 0) return b;
            if (b == 0) return a;
        }
        return 1;
    case CT_SEASON:
        ko_run((u8)(c_stage / 3), 3, (u8)(c_stage % 3));
        return ko_cur[1];
    default:
        ko_run(0, 3, c_stage);
        return ko_cur[1];
    }
}

u8 __fastcall__ comp_surface(u8 unused)
{
    (void)unused;
    if (c_type == CT_SEASON) return (u8)(c_stage / 3);
    if (c_type == CT_CUSTOM && (c_opts & CO_VARIED)) return (u8)((c_surf + c_stage) & 3);
    return c_surf;
}

u8 __fastcall__ comp_level(u8 unused)
{
    s8 lv;
    if (c_type == CT_CUSTOM && (c_opts & CO_PROG)) lv = (s8)(c_stage + (c_n == 4 ? 1 : 0));
    else lv = (s8)(c_rt[comp_opponent(unused)] + c_lvl - 1);
    if (c_type == CT_GROUPS && c_stage >= 3) ++lv;
    if (c_type == CT_SEASON && c_stage % 3 == 2) ++lv;
    if (lv < 0) lv = 0;
    if (lv > 2) lv = 2;
    return (u8)lv;
}

u8 __fastcall__ comp_record(u8 won)
{
    u8 r, ev;
    switch (c_type) {
    case CT_GROUPS:
        if (c_stage < 3) {
            if (won) c_results |= (u8)(1 << c_stage);
            if (++c_stage == 3 && group_rank_of_human() > 1) { c_over = 1; c_rank = 5; }
        } else if (!won) { c_over = 1; c_rank = (u8)(c_stage == 3 ? 3 : 2); }
        else if (c_stage == 4) { c_over = 2; c_rank = 1; }
        else ++c_stage;
        break;
    case CT_LEAGUE:
        if (won) c_results |= (u8)(1 << c_stage);
        if (++c_stage == 5) {
            u8 i;
            league_wins(5);
            rank_by(0, 6, 0);
            for (i = 0; i < 6; ++i) if (order[i] == 0) c_rank = (u8)(i + 1);
            c_over = 2;
        }
        break;
    case CT_SEASON:
        ev = (u8)(c_stage / 3);
        r = (u8)(c_stage % 3);
        if (won) ++r;
        if (!won || r == 3) {
            c_results |= (u8)(r << (ev * 2));
            c_stage = (u8)((ev + 1) * 3);
            if (ev == 3) {
                u8 i;
                season_points(4);
                rank_by(0, 8, 1);
                for (i = 0; i < 8; ++i) if (order[i] == 0) c_rank = (u8)(i + 1);
                c_over = 2;
            }
        } else ++c_stage;
        break;
    default:
        if (!won) {
            /* rang : 2 finaliste, 3 demi-finaliste, 5 éliminé avant */
            r = (u8)(nrounds() - c_stage);
            c_over = 1;
            c_rank = (u8)(r == 1 ? 2 : r == 2 ? 3 : 5);
        }
        else if (++c_stage == nrounds()) { c_over = 2; c_rank = 1; }
        break;
    }
    if (c_over && c_rank == 1 && c_trophies < 15) ++c_trophies;
    return c_over;
}

static char nb[6];
static void ent_at(u8 i, u8 x, u8 y)
{
    team_label(c_pl[i], c_pl2[i], nb);
    txt_at(x, y, nb);
}

static const char * const ko_round_name[3] = { "FINALE", "DEMI-FINALE", "QUART DE FINALE" };
static void copy_txt(char *d, const char *s) { while (*s) *d++ = *s++; *d = 0; }

u8 __fastcall__ comp_stage_name(u8 unused)
{
    char *l1 = comp_l1, *l2 = comp_l2;
    u8 left;
    (void)unused;
    l2[0] = 0;
    switch (c_type) {
    case CT_GROUPS:
        if (c_stage < 3) { copy_txt(l1, "POULE A - JOURNEE"); l1[17] = ' '; l1[18] = (char)('1' + c_stage); l1[19] = '/'; l1[20] = '3'; l1[21] = 0; }
        else copy_txt(l1, c_stage == 3 ? "DEMI-FINALE" : "FINALE");
        break;
    case CT_LEAGUE:
        copy_txt(l1, "JOURNEE  /5");
        l1[8] = (char)('1' + c_stage);
        break;
    case CT_SEASON:
        copy_txt(l1, "TOURNOI  /4 :");
        l1[8] = (char)('1' + c_stage / 3);
        copy_txt(l1 + 14, surf_name[c_stage / 3]);
        copy_txt(l2, ko_round_name[2 - c_stage % 3]);
        break;
    default:
        left = (u8)(nrounds() - c_stage);
        copy_txt(l1, ko_round_name[left - 1]);
        break;
    }
    return 0;
}

/* tableau ou classement à partir de la ligne y (rendu coupé) */
u8 __fastcall__ comp_draw_table(u8 y)
{
    static char line[20];
    u8 i, a, b, n;
    switch (c_type) {
    case CT_GROUPS:
        if (c_stage < 3) {
            groups_wins(c_stage);
            txt_at(3, y, "POULE A      POULE B");
            for (i = 0; i < 4; ++i) {
                ent_at(i, 3, (u8)(y + 1 + i));
                num2(line, wins[i]); line[2] = 'V'; line[3] = 0; txt_at(9, (u8)(y + 1 + i), line);
                ent_at((u8)(i + 4), 16, (u8)(y + 1 + i));
                num2(line, wins[i + 4]); txt_at(22, (u8)(y + 1 + i), line);
            }
        } else {
            groups_semis();
            txt_at(3, y, "DEMI-FINALES");
            for (i = 0; i < 2; ++i) {
                ent_at(sf_a[i], 3, (u8)(y + 1 + i));
                txt_at(9, (u8)(y + 1 + i), "-");
                ent_at(sf_b[i], 11, (u8)(y + 1 + i));
            }
        }
        break;
    case CT_LEAGUE:
        league_wins(c_stage);
        rank_by(0, 6, 0);
        txt_at(3, y, "CLASSEMENT   VICTOIRES");
        for (i = 0; i < 6; ++i) {
            line[0] = (char)('1' + i); line[1] = '.'; line[2] = 0;
            txt_at(3, (u8)(y + 1 + i), line);
            ent_at(order[i], 6, (u8)(y + 1 + i));
            num2(line, wins[order[i]]); line[2] = 0;
            txt_at(17, (u8)(y + 1 + i), line);
        }
        break;
    case CT_SEASON:
        season_points((u8)(c_stage / 3 > 4 ? 4 : c_stage / 3));
        rank_by(0, 8, 1);
        txt_at(3, y, "CLASSEMENT   POINTS");
        for (i = 0; i < 8; ++i) {
            line[0] = (char)('1' + i); line[1] = '.'; line[2] = 0;
            txt_at(3, (u8)(y + 1 + i), line);
            ent_at(order[i], 6, (u8)(y + 1 + i));
            num3(line, pts[order[i]]); line[3] = 0;
            txt_at(16, (u8)(y + 1 + i), line);
        }
        break;
    default:
        ko_run(0, 3, c_stage);
        n = (u8)(c_n >> c_stage);
        txt_at(3, y, "TABLEAU");
        for (i = 0; i < (u8)(n >> 1); ++i) {
            a = ko_cur[i * 2]; b = ko_cur[i * 2 + 1];
            ent_at(a, 3, (u8)(y + 1 + i));
            txt_at(9, (u8)(y + 1 + i), "-");
            ent_at(b, 11, (u8)(y + 1 + i));
        }
        break;
    }
    return 0;
}
