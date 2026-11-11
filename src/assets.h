#ifndef ASSETS_H
#define ASSETS_H
/* banque fixe */
extern const unsigned char logo_tiles[56];   /* « ROYAL MARTINOT » */
extern const unsigned char bigball_tiles[16];
/* personnages : réserve commune de tuiles à partir de $10 ; pour chaque
   vue (person_row[face] = face * NPOSE * 3) et chaque pose, 3 octets = tuile
   gauche de chaque rangée (la tuile droite suit) ; une table par silhouette */
#define NPOSE 27
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
/* vue en perspective : places des tuiles (banque 3), courts et tuiles
   (banque 11), tables de projection y' et échelle x 128 par y à plat (banque 11) */
extern const unsigned char * const persp_rle[7], * const persp_chr[7], * const persp_slot[7];
extern const unsigned char persp_ntile[7];
extern const unsigned char * const persp_ps[7];   /* spectateurs de la perspective : 1re des 24 places (banque 11) */
extern const unsigned char small_normA[192], small_upA[192], small_normB[192], small_upB[192];   /* 3 tailles x 4 dessins */
extern const unsigned char proj_tab[512 + 640];   /* y', m (signé), puis produits v x |m| / 128 */

/* jeu de tuiles par phase (version 2.42) : banque 3 pour les tables,
   banque 13 pour les tuiles */
extern const unsigned char pose_set[], pose_fb[];   /* 0 permanente, 1 célébrations, 2 revers ; pose de secours */
extern const unsigned char swap_base, swap_ntile[2], ice_tile;
extern const unsigned char swap_chrC[], swap_nudeC[], swap_chrR[], swap_nudeR[];
#endif
