#ifndef ASSETS_H
#define ASSETS_H
/* banque fixe */
extern const unsigned char logo_tiles[56];   /* « ROYAL MARTINOT » */
extern const unsigned char bigball_tiles[16];
/* personnages : réserve commune de tuiles à partir de $10 ; pour chaque
   vue (person_row[face] = face * NPOSE * 3) et chaque pose, 3 octets = tuile
   gauche de chaque rangée (la tuile droite suit) ; une table par silhouette */
#define NPOSE 22
extern const unsigned char person_map_m[2 * NPOSE * 3], person_map_f[2 * NPOSE * 3], person_row[2];
extern const unsigned char prof_map[24];        /* course de profil : (femme * 4 + image) * 3 */
extern const unsigned char marg_map[2 * NPOSE]; /* tête de Margaux : face * NPOSE + pose */
extern const unsigned int people_bytes;         /* taille de la réserve (octets de CHR) */
extern const unsigned char marg_prof_l, marg_prof_r;
extern const unsigned char trophy_tile, confetti_tile, ball_base;
/* banque du moteur de jeu (BANK0) */
extern const unsigned char * const court_nt_rle[7];
extern const unsigned char crowd_idx4[3];
extern const unsigned char crowd_norm4[48];
extern const unsigned char crowd_cheer4[48];
extern const unsigned char crowd_idx3[18];
extern const unsigned char crowd_norm3[288];
extern const unsigned char crowd_cheer3[288];
extern const unsigned char sign_chr[128];      /* panneau du court Margaux (8 tuiles) */
extern const unsigned char sign2_chr[128];     /* panneau du club nudiste (8 tuiles) */
extern const unsigned char sign_idx[8], sign2_idx[8];        /* tuiles qu'ils remplacent */
extern const unsigned char sign_orig[128], sign2_orig[128];  /* dessin d'origine de ces tuiles */
/* arbitre de chaise : par jeu, tuiles 11 à 13 (têtes, buste) puis 4 tuiles
   chargées à la place de la coupe pendant un match (buste bras tendu, main,
   bras levé, baguette de la princesse) */
extern const unsigned char ump_chr[224];   /* 2 jeux de 7 tuiles : arbitre, princesse (court Margaux) */
extern const unsigned char net_idx[2];          /* tuiles des mailles du filet */
extern const unsigned char net_norm[32], net_shk[32];   /* au repos / secouées */
/* banque 6 */
extern const unsigned char nude_people[];      /* people_bytes octets */
extern const unsigned char crowd_idx[6];
extern const unsigned char crowd_norm[96];
extern const unsigned char crowd_cheer[96];
extern const unsigned char cheer_idx[6];       /* ola : spectateurs bras levés (tuiles propres) */
extern const unsigned char crowd_up[96];       /* spectateurs qui regardent vers le haut du court */
/* jeu B (club, salle ; palette 3) */
extern const unsigned char crowdb_idx[6], cheerb_idx[6], crowdb_norm[96], crowdb_up[96];
#endif
