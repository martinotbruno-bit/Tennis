/* Royal Martinot — accès aux règles du score depuis toutes les banques
 * (2.71). Le code des règles (rules.c) est passé de la banque fixe à la
 * banque 18 pour faire de la place aux voix de l'arbitre (le DMC ne lit que
 * $C000-$FFFF) ; ces petites fonctions de la banque fixe y passent, appellent
 * la règle et remettent la banque de l'appelant. */
#include "runtime.h"
#include "app.h"
#include "rules.h"

void rb_init(ru8 sets_to_win, ru8 games_per_set, ru8 first_server);
ru8 rb_point(ru8 team);
ru8 rb_ad_side(void);
void rb_point_label(ru8 team, char *out);
ru8 rb_is_set_point(ru8 team);
ru8 rb_is_match_point(ru8 team);

static u8 rf_b, rf_r;

void rules_init(ru8 sets_to_win, ru8 games_per_set, ru8 first_server)
{
    rf_b = cur_bank; set_bank(BANK_RULES);
    rb_init(sets_to_win, games_per_set, first_server);
    set_bank(rf_b);
}
ru8 rules_point(ru8 team)
{
    rf_b = cur_bank; set_bank(BANK_RULES); rf_r = rb_point(team); set_bank(rf_b); return rf_r;
}
ru8 rules_ad_side(void)
{
    rf_b = cur_bank; set_bank(BANK_RULES); rf_r = rb_ad_side(); set_bank(rf_b); return rf_r;
}
void rules_point_label(ru8 team, char *out)
{
    rf_b = cur_bank; set_bank(BANK_RULES); rb_point_label(team, out); set_bank(rf_b);
}
ru8 rules_is_set_point(ru8 team)
{
    rf_b = cur_bank; set_bank(BANK_RULES); rf_r = rb_is_set_point(team); set_bank(rf_b); return rf_r;
}
ru8 rules_is_match_point(ru8 team)
{
    rf_b = cur_bank; set_bank(BANK_RULES); rf_r = rb_is_match_point(team); set_bank(rf_b); return rf_r;
}
