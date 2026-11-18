/* Royal Martinot 2.76 : le soleil couchant du crépuscule (banque 22).
   Image plein écran façon jeux de course des années 80, une par phase
   (tools/gen_sun.py) ; le plan lui-même (titre, cycle de couleurs de la
   grille, retour au court) est dans highlight.c (hl_mode 5). */
#include "game_int.h"
#include "app.h"
#include "gfx.h"
#include "runtime.h"
#include "rules.h"
#include "game.h"

/* phases du crépuscule (2.76, déplacées de highlight.c, banque 15 pleine) */
extern u8 sun_st, sun_pend;
extern u8 ppu_emph;
#pragma zpsym("ppu_emph")

extern const u8 sun_at[64];
extern const u8 sun_pal[18][16];
extern const u8 sun_ntl[18], sun_bk[18];
extern const u8 * const sun_chr[18];
extern const u8 * const sun_nt[18];

/* rendu coupé : tuiles (table de fond), écran 0, attributs, palettes.
   Image ph = surface * 3 + phase : le premier plan suit la surface (et le
   lieu du tournoi Martinot qui s'y joue) ; images en banques 22 à 27 */
u16 __fastcall__ sun_build(u16 ph)
{
    u8 k = (u8)ph, i, n = sun_ntl[k];
    const u8 *p = sun_chr[k];
    tz_bank = sun_bk[k];
    vram_adr(0x0000);
    /* par paquets de 16 tuiles (256 octets ; tz_n = 0) : n <= 250, le
       compteur ne doit pas repasser par 0 (240 + 16 = 256) */
    for (i = 0; i < 16 && (u8)(i << 4) < n; ++i) { tz_n = (u8)(n - (u8)(i << 4) >= 16 ? 0 : (u8)((n - (u8)(i << 4)) << 4)); tz_copy(p); p += 256; }
    vram_adr(0x2000);
    p = sun_nt[k];
    for (i = 0; i < 3; ++i) { tz_n = 0; tz_copy(p); p += 256; }
    tz_n = 192; tz_copy(p);
    tz_bank = BANK_SUN;
    tz_n = 64; tz_copy(sun_at);
    for (i = 0; i < 16; ++i) pal_buf[i] = sun_pal[k][i];
    return 0;
}

/* phase de la lumière d'après les jeux joués (0 jour .. 3 nuit) */
static u8 sun_calc(void)
{
    u8 i, g, L, st;
    g = (u8)(sc_games[0] + sc_games[1]);
    for (i = 0; i < sc_set_no && i < MAX_SETS; ++i) g = (u8)(g + sc_hist[i][0] + sc_hist[i][1]);
    L = (u8)((cfg.sets_to_win * cfg.games_per_set * 6) / 5);
    st = (u8)(((u16)g * 4) / L);
    return st > 3 ? 3 : st;
}
static void sun_light(void)
{
    u8 red = is_pal ? 0x40 : 0x20;
    pal_set_court(cfg.surface, sun_st == 3 ? COND_NIGHT : COND_DAY, cfg.env);
    /* (2.77) château : sa pierre grise virait au rose puis au lilas sous
       l'accentuation ; pierre dorée au couchant, brun orangé au crépuscule */
    if (cfg.env == ENV_CASTLE && sun_st && sun_st < 3) {
        pal_buf[9] = sun_st == 1 ? 0x37 : 0x27;
        pal_buf[10] = sun_st == 1 ? 0x27 : 0x17;
    }
    ppu_emph = sun_st == 1 ? red : sun_st == 2 ? (u8)(red | 0x80) : 0;
}
/* (2.76) la lumière ne change plus en plein jeu : la nouvelle phase attend
   le changement de côté suivant, où un plan du soleil couchant la montre
   (hl_mode 5, sun_pend) ; au chargement du court (début du match, reprise),
   la phase réelle s'applique directement, sauf si un plan est en attente */
void sun_apply(void)
{
    if (cfg.cond != COND_SUNSET || cfg.mode != GM_MATCH) return;
    if (!sun_pend) sun_st = sun_calc();
    sun_light();
}
void sun_game(void)
{
    u8 st;
    if (cfg.cond != COND_SUNSET || cfg.mode != GM_MATCH) return;
    st = sun_calc();
    if (st == sun_st) return;
    if (cfg.demo) { sun_st = st; sun_light(); }
    else sun_pend = 1;
}
u16 __fastcall__ sun_calc_f(u16 unused) { (void)unused; return sun_calc(); }
