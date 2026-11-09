/* Royal Martinot — règles du tennis.
 * Points 0/15/30/40/AV, égalité, jeu décisif à 7 points (2 d'écart) à
 * N jeux partout, sets de N jeux avec 2 jeux d'écart, alternance des services,
 * changements de côté après chaque jeu impair et tous les 6 points du jeu
 * décisif. Code C portable : compilé par cc65 pour la NES et par gcc pour
 * les tests unitaires (tests/rules_test.c). */
#include "rules.h"

ru8 sc_points[2], sc_games[2], sc_sets[2];
ru8 sc_hist[MAX_SETS][2];
ru8 sc_set_no, sc_tiebreak, sc_server, sc_swap, sc_over, sc_winner;
ru8 sc_sets_to_win, sc_games_per_set, sc_tb_first, sc_set_first_server;

void rules_init(ru8 sets_to_win, ru8 games_per_set, ru8 first_server)
{
    ru8 i;
    sc_points[0] = sc_points[1] = 0;
    sc_games[0] = sc_games[1] = 0;
    sc_sets[0] = sc_sets[1] = 0;
    for (i = 0; i < MAX_SETS; ++i) { sc_hist[i][0] = sc_hist[i][1] = 0; }
    sc_set_no = 0;
    sc_tiebreak = 0;
    sc_server = first_server;
    sc_set_first_server = first_server;
    sc_swap = 0;
    sc_over = 0;
    sc_winner = 0;
    sc_sets_to_win = sets_to_win;
    sc_games_per_set = games_per_set;
}

ru8 rules_ad_side(void)
{
    return (ru8)((sc_points[0] + sc_points[1]) & 1);
}

static ru8 tb_server_for(ru8 n)
{
    /* point 0 : premier serveur, puis alternance tous les deux points */
    return (((n + 1) >> 1) & 1) ? (ru8)(sc_tb_first ^ 1) : sc_tb_first;
}

static ru8 end_set(ru8 team)
{
    ru8 ev = EV_SET;
    ru8 total;
    sc_hist[sc_set_no][0] = sc_games[0];
    sc_hist[sc_set_no][1] = sc_games[1];
    total = (ru8)(sc_games[0] + sc_games[1]);
    ++sc_sets[team];
    if (sc_sets[team] >= sc_sets_to_win) {
        sc_over = 1;
        sc_winner = team;
        return EV_SET | EV_MATCH;
    }
    if (sc_set_no < MAX_SETS - 1) ++sc_set_no;
    sc_games[0] = sc_games[1] = 0;
    if (total & 1) { sc_swap ^= 1; ev |= EV_CHANGE; }
    return ev;
}

ru8 rules_point(ru8 team)
{
    ru8 other = (ru8)(team ^ 1);
    ru8 ev = EV_POINT;
    ru8 g;

    if (sc_over) return EV_MATCH;
    ++sc_points[team];

    if (sc_tiebreak) {
        ru8 n = (ru8)(sc_points[0] + sc_points[1]);
        if (sc_points[team] >= 7 && sc_points[team] >= sc_points[other] + 2) {
            /* le jeu décisif compte comme un jeu gagné */
            ++sc_games[team];
            sc_points[0] = sc_points[1] = 0;
            sc_tiebreak = 0;
            /* celui qui a servi en premier dans le jeu décisif relance */
            sc_server = (ru8)(sc_tb_first ^ 1);
            sc_set_first_server = sc_server;
            return (ru8)(EV_GAME | end_set(team));
        }
        sc_server = tb_server_for(n);
        if (n % 6 == 0) { sc_swap ^= 1; ev |= EV_CHANGE; }
        return ev;
    }

    if (sc_points[team] >= 4 && sc_points[team] >= sc_points[other] + 2) {
        sc_points[0] = sc_points[1] = 0;
        g = ++sc_games[team];
        ev = EV_GAME;
        sc_server ^= 1;
        if (g >= sc_games_per_set && g >= sc_games[other] + 2) {
            return (ru8)(ev | end_set(team));
        }
        if (sc_games[0] == sc_games_per_set && sc_games[1] == sc_games_per_set) {
            sc_tiebreak = 1;
            sc_tb_first = sc_server;
            ev |= EV_TIEBREAK;
        }
        if ((sc_games[0] + sc_games[1]) & 1) { sc_swap ^= 1; ev |= EV_CHANGE; }
        return ev;
    }
    /* égalité prolongée : on ramène 4-4 à 3-3 pour borner les compteurs */
    if (sc_points[0] >= 4 && sc_points[1] >= 4) {
        --sc_points[0];
        --sc_points[1];
    }
    return ev;
}

void rules_point_label(ru8 team, char *out)
{
    ru8 p = sc_points[team], o = sc_points[team ^ 1];
    if (sc_tiebreak) {
        out[0] = (char)(p >= 10 ? '0' + p / 10 : ' ');
        out[1] = (char)('0' + p % 10);
        return;
    }
    if (p >= 3 && o >= 3) {
        if (p == o)      { out[0] = '4'; out[1] = '0'; }
        else if (p > o)  { out[0] = 'A'; out[1] = 'V'; }
        else             { out[0] = ' '; out[1] = '-'; }
        return;
    }
    switch (p) {
    case 0:  out[0] = ' '; out[1] = '0'; break;
    case 1:  out[0] = '1'; out[1] = '5'; break;
    case 2:  out[0] = '3'; out[1] = '0'; break;
    default: out[0] = '4'; out[1] = '0'; break;
    }
}

/* L'équipe gagne-t-elle le jeu (ou jeu décisif) si elle marque le point ? */
static ru8 wins_game_next(ru8 team)
{
    ru8 p = (ru8)(sc_points[team] + 1), o = sc_points[team ^ 1];
    if (sc_tiebreak) return (ru8)(p >= 7 && p >= o + 2);
    return (ru8)(p >= 4 && p >= o + 2);
}

ru8 rules_is_set_point(ru8 team)
{
    ru8 g, o;
    if (!wins_game_next(team)) return 0;
    if (sc_tiebreak) return 1;
    g = (ru8)(sc_games[team] + 1);
    o = sc_games[team ^ 1];
    return (ru8)(g >= sc_games_per_set && g >= o + 2);
}

ru8 rules_is_match_point(ru8 team)
{
    return (ru8)(rules_is_set_point(team) && sc_sets[team] + 1 >= sc_sets_to_win);
}
