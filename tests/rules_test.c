/* Tests unitaires des règles (compilés avec gcc) : make test */
#include <stdio.h>
#include <string.h>
#include "../src/rules.h"

static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; printf("ECHEC %s:%d : %s\n", __FILE__, __LINE__, #c); } } while (0)

static ru8 win_game(ru8 t) { ru8 ev = 0; int i; for (i = 0; i < 4; ++i) ev |= rules_point(t); return ev; }

static void label(ru8 t, char *b) { rules_point_label(t, b); b[2] = 0; }

int main(void)
{
    char b[3];
    ru8 ev;
    int i;

    /* comptage 0-15-30-40 */
    rules_init(2, 6, 0);
    label(0, b); CHECK(strcmp(b, " 0") == 0);
    rules_point(0); label(0, b); CHECK(strcmp(b, "15") == 0);
    rules_point(0); label(0, b); CHECK(strcmp(b, "30") == 0);
    rules_point(0); label(0, b); CHECK(strcmp(b, "40") == 0);
    CHECK(rules_ad_side() == 1);
    /* égalité et avantage */
    rules_point(1); rules_point(1); rules_point(1);
    label(0, b); CHECK(strcmp(b, "40") == 0);
    label(1, b); CHECK(strcmp(b, "40") == 0);
    rules_point(1); label(1, b); CHECK(strcmp(b, "AV") == 0);
    rules_point(0); label(0, b); CHECK(strcmp(b, "40") == 0);
    for (i = 0; i < 20; ++i) { rules_point(0); rules_point(1); }  /* longue égalité */
    CHECK(sc_games[0] == 0 && sc_games[1] == 0);
    rules_point(0); ev = rules_point(0);
    CHECK(ev & EV_GAME);
    CHECK(sc_games[0] == 1);
    CHECK(sc_server == 1);           /* alternance du service */
    CHECK(ev & EV_CHANGE);           /* 1 jeu joué : changement de côté */
    CHECK(sc_swap == 1);

    /* set 6-4 */
    rules_init(2, 6, 0);
    for (i = 0; i < 4; ++i) { win_game(0); win_game(1); }
    win_game(0);
    ev = win_game(0);
    CHECK(ev & EV_SET);
    CHECK(sc_sets[0] == 1 && sc_hist[0][0] == 6 && sc_hist[0][1] == 4);
    CHECK(sc_games[0] == 0 && sc_games[1] == 0);
    CHECK(!(ev & EV_MATCH));

    /* 6-5 ne suffit pas, 7-5 oui */
    rules_init(1, 6, 0);
    for (i = 0; i < 5; ++i) { win_game(0); win_game(1); }
    ev = win_game(0);
    CHECK(!(ev & EV_SET));
    ev = win_game(0);
    CHECK(ev & EV_MATCH);
    CHECK(sc_over && sc_winner == 0);
    CHECK(sc_hist[0][0] == 7 && sc_hist[0][1] == 5);

    /* jeu décisif à 6-6 */
    rules_init(2, 6, 0);
    for (i = 0; i < 6; ++i) { win_game(0); win_game(1); }
    CHECK(sc_tiebreak == 1);
    {
        ru8 first = sc_server;
        ru8 sw = sc_swap;
        rules_point(0);                       /* 1-0 */
        CHECK(sc_server == (first ^ 1));
        rules_point(1);                       /* 1-1 */
        CHECK(sc_server == (first ^ 1));
        rules_point(0);                       /* 2-1 */
        CHECK(sc_server == first);
        label(0, b); CHECK(strcmp(b, " 2") == 0);
        rules_point(0); rules_point(0);
        ev = rules_point(1);                  /* 4-2 : 6 points -> changement */
        CHECK(ev & EV_CHANGE);
        CHECK(sc_swap == (sw ^ 1));
        rules_point(0); rules_point(1); rules_point(1); rules_point(1); /* 5-5 */
        rules_point(1); /* 5-6 */
        CHECK(rules_is_set_point(1));
        rules_point(0); /* 6-6 */
        rules_point(0); /* 7-6 : pas fini */
        CHECK(sc_tiebreak == 1);
        ev = rules_point(0); /* 8-6 */
        CHECK(ev & EV_SET);
        CHECK(sc_hist[0][0] == 7 && sc_hist[0][1] == 6);
        CHECK(sc_server == (first ^ 1));
    }

    /* match en 3 sets gagnants, balle de match */
    rules_init(3, 6, 1);
    for (i = 0; i < 12; ++i) win_game(1);
    CHECK(sc_sets[1] == 2);
    for (i = 0; i < 5; ++i) win_game(1);
    rules_point(1); rules_point(1); rules_point(1);
    CHECK(rules_is_match_point(1));
    CHECK(!rules_is_match_point(0));
    ev = rules_point(1);
    CHECK(ev & EV_MATCH);
    CHECK(sc_winner == 1);
    /* plus aucun point compté après la fin */
    ev = rules_point(0);
    CHECK(sc_points[0] == 0);

    /* format court : sets de 4 jeux, jeu décisif à 4-4 */
    rules_init(1, 4, 0);
    for (i = 0; i < 4; ++i) { win_game(0); win_game(1); }
    CHECK(sc_tiebreak == 1);

    printf("%d vérifications, %d échec(s)\n", checks, fails);
    return fails ? 1 : 0;
}
