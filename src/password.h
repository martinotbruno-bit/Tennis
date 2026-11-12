#ifndef PASSWORD_H
#define PASSWORD_H
/* Codes de reprise des compétitions : 13 caractères (65 bits).
 * 52 bits de données, 2 bits de version, 11 bits de contrôle. */
typedef unsigned char pu8;
#define PW_LEN 13
#define PW_ALPHA "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"
#define PW_NCHARS 21   /* personnages codables (0..20, 5 bits) */
typedef struct {
    pu8 type;      /* 0..7  type de compétition */
    pu8 stage;     /* 0..15 étape */
    pu8 seed;      /* 0..255 tirage (adversaires, résultats simulés) */
    pu8 chr;       /* 0..16 personnage */
    pu8 surf;      /* 0..7 */
    pu8 fmt;       /* 0..3 */
    pu8 cond;      /* 0..3 */
    pu8 env;       /* 0..6 */
    pu8 lvl;       /* 0..3 */
    pu8 results;   /* 8 bits de résultats du joueur */
    pu8 opts;      /* 0..7 options (personnalisé) */
    pu8 trophies;  /* 0..15 */
    pu8 doubles;   /* 0..1 compétition en double */
    pu8 partner;   /* 0..16 partenaire du joueur (double) */
    pu8 hum2;      /* 0..1 partenaire joué par la manette 2 */
    unsigned short gres;   /* Grand Martinot (type 5) : tours gagnés, 3 bits par tournoi */
} PwData;
void pw_encode(char *out, const PwData *d);
pu8 pw_decode(const char *in, PwData *d);
#endif
