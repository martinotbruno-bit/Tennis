/* Géométrie du court en pixels (identique à tools/gen_assets.py) */
#ifndef COURT_H
#define COURT_H
#define TOP_BASE 48
#define BOT_BASE 216
#define NET_Y    132
#define TOP_SERV 87
#define BOT_SERV 177
#define SGL_L    74
#define SGL_R    182
#define DBL_L    54
#define DBL_R    202
#define CENTER_X 128
#define NET_H    7      /* hauteur du filet (unités de hauteur) */
/* Règle : une balle qui touche la ligne est bonne. Le point jugé est le
   centre de la balle ; il est bon jusqu'à 1 px au-delà de la ligne (la
   trace de la balle, 3 x 2 px, la touche encore). Les lignes de fond ont
   2 px d'épaisseur (TOP_BASE - 1 et BOT_BASE + 1 en font partie). (2.73) */
#define IN_TOP   (TOP_BASE - 2)
#define IN_BOT   (BOT_BASE + 2)
#define IN_TSRV  (TOP_SERV - 1)
#define IN_BSRV  (BOT_SERV + 1)
#define IN_SL    (SGL_L - 1)
#define IN_SR    (SGL_R + 1)
#define IN_DL    (DBL_L - 1)
#define IN_DR    (DBL_R + 1)
#endif
