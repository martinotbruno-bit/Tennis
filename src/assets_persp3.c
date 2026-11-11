/* Fichier généré par tools/gen_assets.py — ne pas modifier. */
/* Vue en perspective : places des tuiles et pointeurs vers la banque 11 (banque 3). */
#include "assets.h"

extern const unsigned char persp_rle0[], persp_chr0[], persp_slot0[];
extern const unsigned char persp_rle1[], persp_chr1[], persp_slot1[];
extern const unsigned char persp_rle2[], persp_chr2[], persp_slot2[];
extern const unsigned char persp_rle3[], persp_chr3[], persp_slot3[];
extern const unsigned char persp_rle4[], persp_chr4[], persp_slot4[];
extern const unsigned char persp_rle5[], persp_chr5[], persp_slot5[];
/* 6 : le parc reprend le court du club */
const unsigned char * const persp_rle[7] = { persp_rle0, persp_rle1, persp_rle2, persp_rle3, persp_rle4, persp_rle5, persp_rle1 };
const unsigned char * const persp_chr[7] = { persp_chr0, persp_chr1, persp_chr2, persp_chr3, persp_chr4, persp_chr5, persp_chr1 };
const unsigned char * const persp_slot[7] = { persp_slot0, persp_slot1, persp_slot2, persp_slot3, persp_slot4, persp_slot5, persp_slot1 };
const unsigned char persp_ntile[7] = { 64, 67, 64, 64, 64, 64, 67 };
