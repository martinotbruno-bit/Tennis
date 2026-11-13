/* Fichier généré par tools/gen_teaser.py — ne pas modifier. */
/* Bande-annonce : descripteurs des images en haute définition (teaser.c). */

extern const unsigned char tz_chr_impact_m[];
extern const unsigned char tz_map_impact_m[];
extern const unsigned char tz_at_impact_m[];
extern const unsigned char tz_chr_impact_f[];
extern const unsigned char tz_map_impact_f[];
extern const unsigned char tz_at_impact_f[];
extern const unsigned char tz_chr_impact_g[];
extern const unsigned char tz_map_impact_g[];
extern const unsigned char tz_at_impact_g[];
extern const unsigned char tz_chr_slide_m[];
extern const unsigned char tz_map_slide_m[];
extern const unsigned char tz_at_slide_m[];
extern const unsigned char tz_chr_slide_f[];
extern const unsigned char tz_map_slide_f[];
extern const unsigned char tz_at_slide_f[];
extern const unsigned char tz_chr_slide_g[];
extern const unsigned char tz_map_slide_g[];
extern const unsigned char tz_at_slide_g[];
extern const unsigned char tz_chr_slide_z_m[];
extern const unsigned char tz_map_slide_z_m[];
extern const unsigned char tz_at_slide_z_m[];
extern const unsigned char tz_chr_slide_z_f[];
extern const unsigned char tz_map_slide_z_f[];
extern const unsigned char tz_at_slide_z_f[];
extern const unsigned char tz_chr_slide_z_g[];
extern const unsigned char tz_map_slide_z_g[];
extern const unsigned char tz_at_slide_z_g[];
extern const unsigned char tz_chr_victory_m[];
extern const unsigned char tz_map_victory_m[];
extern const unsigned char tz_at_victory_m[];
extern const unsigned char tz_chr_victory_f[];
extern const unsigned char tz_map_victory_f[];
extern const unsigned char tz_at_victory_f[];
extern const unsigned char tz_chr_victory_g[];
extern const unsigned char tz_map_victory_g[];
extern const unsigned char tz_at_victory_g[];
extern const unsigned char tz_chr_victory_z_m[];
extern const unsigned char tz_map_victory_z_m[];
extern const unsigned char tz_at_victory_z_m[];
extern const unsigned char tz_chr_victory_z_f[];
extern const unsigned char tz_map_victory_z_f[];
extern const unsigned char tz_at_victory_z_f[];
extern const unsigned char tz_chr_victory_z_g[];
extern const unsigned char tz_map_victory_z_g[];
extern const unsigned char tz_at_victory_z_g[];
extern const unsigned char tz_chr_angry_m[];
extern const unsigned char tz_map_angry_m[];
extern const unsigned char tz_at_angry_m[];
extern const unsigned char tz_chr_angry_f[];
extern const unsigned char tz_map_angry_f[];
extern const unsigned char tz_at_angry_f[];
extern const unsigned char tz_chr_angry_g[];
extern const unsigned char tz_map_angry_g[];
extern const unsigned char tz_at_angry_g[];
extern const unsigned char tz_chr_angry_z_m[];
extern const unsigned char tz_map_angry_z_m[];
extern const unsigned char tz_at_angry_z_m[];
extern const unsigned char tz_chr_angry_z_f[];
extern const unsigned char tz_map_angry_z_f[];
extern const unsigned char tz_at_angry_z_f[];
extern const unsigned char tz_chr_angry_z_g[];
extern const unsigned char tz_map_angry_z_g[];
extern const unsigned char tz_at_angry_z_g[];
extern const unsigned char tz_chr_smash_m[];
extern const unsigned char tz_map_smash_m[];
extern const unsigned char tz_at_smash_m[];
extern const unsigned char tz_chr_smash_f[];
extern const unsigned char tz_map_smash_f[];
extern const unsigned char tz_at_smash_f[];
extern const unsigned char tz_chr_smash_g[];
extern const unsigned char tz_map_smash_g[];
extern const unsigned char tz_at_smash_g[];

typedef struct { unsigned char bank, ntile, x0, w, y0, h, zoom; const unsigned char *chr, *map, *at; } tz_art_t;

#define TZ_IMPACT 0
#define TZ_SLIDE 1
#define TZ_SLIDE_Z 2
#define TZ_VICTORY 3
#define TZ_VICTORY_Z 4
#define TZ_ANGRY 5
#define TZ_ANGRY_Z 6
#define TZ_SMASH 7
#define TZ_N 8
static const tz_art_t tz_art[TZ_N][3] = {   /* homme, femme, fillette (Margaux) */
    { { 17, 209, 0, 30, 2, 26, 0, tz_chr_impact_m, tz_map_impact_m, tz_at_impact_m },
      { 12, 217, 0, 30, 2, 26, 0, tz_chr_impact_f, tz_map_impact_f, tz_at_impact_f },
      { 10, 192, 0, 30, 2, 26, 0, tz_chr_impact_g, tz_map_impact_g, tz_at_impact_g } },
    { { 4, 194, 2, 30, 2, 26, 0, tz_chr_slide_m, tz_map_slide_m, tz_at_slide_m },
      { 2, 182, 2, 30, 4, 24, 0, tz_chr_slide_f, tz_map_slide_f, tz_at_slide_f },
      { 7, 168, 2, 30, 4, 24, 0, tz_chr_slide_g, tz_map_slide_g, tz_at_slide_g } },
    { { 9, 232, 0, 32, 0, 28, 0, tz_chr_slide_z_m, tz_map_slide_z_m, tz_at_slide_z_m },
      { 17, 227, 0, 32, 0, 28, 0, tz_chr_slide_z_f, tz_map_slide_z_f, tz_at_slide_z_f },
      { 12, 206, 0, 32, 0, 28, 0, tz_chr_slide_z_g, tz_map_slide_z_g, tz_at_slide_z_g } },
    { { 2, 158, 4, 20, 0, 28, 0, tz_chr_victory_m, tz_map_victory_m, tz_at_victory_m },
      { 6, 152, 4, 18, 0, 28, 0, tz_chr_victory_f, tz_map_victory_f, tz_at_victory_f },
      { 17, 136, 4, 18, 0, 28, 0, tz_chr_victory_g, tz_map_victory_g, tz_at_victory_g } },
    { { 4, 216, 0, 30, 0, 28, 1, tz_chr_victory_z_m, tz_map_victory_z_m, tz_at_victory_z_m },
      { 17, 213, 0, 28, 0, 28, 1, tz_chr_victory_z_f, tz_map_victory_z_f, tz_at_victory_z_f },
      { 9, 192, 0, 28, 0, 28, 1, tz_chr_victory_z_g, tz_map_victory_z_g, tz_at_victory_z_g } },
    { { 12, 168, 8, 16, 4, 24, 0, tz_chr_angry_m, tz_map_angry_m, tz_at_angry_m },
      { 9, 153, 8, 16, 4, 24, 0, tz_chr_angry_f, tz_map_angry_f, tz_at_angry_f },
      { 7, 139, 8, 16, 4, 24, 0, tz_chr_angry_g, tz_map_angry_g, tz_at_angry_g } },
    { { 9, 244, 0, 32, 0, 28, 1, tz_chr_angry_z_m, tz_map_angry_z_m, tz_at_angry_z_m },
      { 17, 192, 4, 26, 4, 24, 1, tz_chr_angry_z_f, tz_map_angry_z_f, tz_at_angry_z_f },
      { 4, 184, 4, 26, 4, 24, 1, tz_chr_angry_z_g, tz_map_angry_z_g, tz_at_angry_z_g } },
    { { 10, 126, 6, 16, 2, 26, 0, tz_chr_smash_m, tz_map_smash_m, tz_at_smash_m },
      { 4, 113, 8, 14, 2, 26, 0, tz_chr_smash_f, tz_map_smash_f, tz_at_smash_f },
      { 12, 105, 8, 14, 2, 26, 0, tz_chr_smash_g, tz_map_smash_g, tz_at_smash_g } },
};
#define TZ_IMPACT_RACKET_X 13
#define TZ_IMPACT_RACKET_Y 52
#define TZ_IMPACT_FOOTR_X 248
#define TZ_IMPACT_FOOTR_Y 239
#define TZ_IMPACT_FACE_X 175
#define TZ_IMPACT_FACE_Y 39
#define TZ_IMPACT_RACKET_TIP_X 0
#define TZ_IMPACT_RACKET_TIP_Y 45
#define TZ_SLIDE_RACKET_X 235
#define TZ_SLIDE_RACKET_Y 116
#define TZ_SLIDE_FOOTR_X 192
#define TZ_SLIDE_FOOTR_Y 223
#define TZ_SLIDE_FACE_X 111
#define TZ_SLIDE_FACE_Y 74
#define TZ_SLIDE_RACKET_TIP_X 252
#define TZ_SLIDE_RACKET_TIP_Y 113
#define TZ_SLIDE_Z_RACKET_X 240
#define TZ_SLIDE_Z_RACKET_Y 99
#define TZ_SLIDE_Z_FOOTR_X 189
#define TZ_SLIDE_Z_FOOTR_Y 223
#define TZ_SLIDE_Z_FACE_X 95
#define TZ_SLIDE_Z_FACE_Y 50
#define TZ_SLIDE_Z_RACKET_TIP_X 255
#define TZ_SLIDE_Z_RACKET_TIP_Y 96
#define TZ_VICTORY_RACKET_X 50
#define TZ_VICTORY_RACKET_Y 5
#define TZ_VICTORY_FOOTR_X 155
#define TZ_VICTORY_FOOTR_Y 225
#define TZ_VICTORY_FACE_X 126
#define TZ_VICTORY_FACE_Y 62
#define TZ_VICTORY_RACKET_TIP_X 38
#define TZ_VICTORY_RACKET_TIP_Y 0
#define TZ_VICTORY_Z_RACKET_X 0
#define TZ_VICTORY_Z_RACKET_Y 0
#define TZ_VICTORY_Z_FOOTR_X 187
#define TZ_VICTORY_Z_FOOTR_Y 239
#define TZ_VICTORY_Z_FACE_X 128
#define TZ_VICTORY_Z_FACE_Y 79
#define TZ_VICTORY_Z_RACKET_TIP_X 0
#define TZ_VICTORY_Z_RACKET_TIP_Y 0
#define TZ_ANGRY_RACKET_X 86
#define TZ_ANGRY_RACKET_Y 186
#define TZ_ANGRY_FOOTR_X 176
#define TZ_ANGRY_FOOTR_Y 224
#define TZ_ANGRY_FACE_X 119
#define TZ_ANGRY_FACE_Y 53
#define TZ_ANGRY_RACKET_TIP_X 83
#define TZ_ANGRY_RACKET_TIP_Y 202
#define TZ_ANGRY_Z_RACKET_X 24
#define TZ_ANGRY_Z_RACKET_Y 239
#define TZ_ANGRY_Z_FOOTR_X 255
#define TZ_ANGRY_Z_FOOTR_Y 239
#define TZ_ANGRY_Z_FACE_X 126
#define TZ_ANGRY_Z_FACE_Y 59
#define TZ_ANGRY_Z_RACKET_TIP_X 15
#define TZ_ANGRY_Z_RACKET_TIP_Y 239
#define TZ_SMASH_RACKET_X 70
#define TZ_SMASH_RACKET_Y 32
#define TZ_SMASH_FOOTR_X 169
#define TZ_SMASH_FOOTR_Y 222
#define TZ_SMASH_FACE_X 113
#define TZ_SMASH_FACE_Y 88
#define TZ_SMASH_RACKET_TIP_X 63
#define TZ_SMASH_RACKET_TIP_Y 21
#define TZ_GROUND_ROW 28
#define TZ_FIRST_TILE 1
