#ifndef PASSWORD_H
#define PASSWORD_H
typedef unsigned char pu8;
#define PW_ALPHA "BCDFGHJKLMNPRSTV"
/* Code de 6 lettres (24 bits) : progression du tournoi + total de trophées,
 * protégé par une somme de contrôle de 8 bits et un brouillage XOR. */
void pw_encode(char *out, pu8 round, pu8 chr, pu8 surf, pu8 fmt, pu8 night, pu8 wins);
pu8 pw_decode(const char *in, pu8 *round, pu8 *chr, pu8 *surf, pu8 *fmt, pu8 *night, pu8 *wins);
#endif
