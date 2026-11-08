#ifndef PASSWORD_H
#define PASSWORD_H
/* Codes de reprise des compétitions : 12 caractères (60 bits).
 * 45 bits de données, 3 bits de version, 12 bits de contrôle. */
typedef unsigned char pu8;
#define PW_LEN 12
#define PW_ALPHA "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"
typedef struct {
    pu8 type;      /* 0..7  type de compétition */
    pu8 stage;     /* 0..15 étape */
    pu8 seed;      /* 0..255 tirage (adversaires, résultats simulés) */
    pu8 chr;       /* 0..15 personnage */
    pu8 surf;      /* 0..7 */
    pu8 fmt;       /* 0..3 */
    pu8 cond;      /* 0..3 */
    pu8 env;       /* 0..3 */
    pu8 lvl;       /* 0..3 */
    pu8 results;   /* 8 bits de résultats du joueur */
    pu8 opts;      /* 0..7 options (personnalisé) */
    pu8 trophies;  /* 0..15 */
} PwData;
void pw_encode(char *out, const PwData *d);
pu8 pw_decode(const char *in, PwData *d);
#endif
