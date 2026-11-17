/* Royal Martinot 2.76 : le soleil couchant du crépuscule (banque 22).
   Image plein écran façon jeux de course des années 80, une par phase
   (tools/gen_sun.py) ; le plan lui-même (titre, cycle de couleurs de la
   grille, retour au court) est dans highlight.c (hl_mode 5). */
#include "game_int.h"
#include "app.h"
#include "gfx.h"
#include "runtime.h"

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
