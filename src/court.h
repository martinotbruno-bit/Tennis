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
   centre de la balle ; elle touche encore une ligne tant que son centre est
   à 2 px ou moins du bord extérieur de la ligne (rayon de la balle au sol,
   la trace du rebond le montre). Épaisseur des lignes (gen_assets.py) : 2 px
   pour les lignes de fond (TOP_BASE - 1 et TOP_BASE, BOT_BASE et
   BOT_BASE + 1), 1 px pour les lignes de côté, de service et la ligne
   médiane. (2.76 : rayon et épaisseur comptés pour toutes les lignes ; une
   balle dont la trace touchait la ligne était parfois jugée dehors) */
#define BALL_R   2
#define IN_TOP   (TOP_BASE - 1 - BALL_R)
#define IN_BOT   (BOT_BASE + 1 + BALL_R)
#define IN_TSRV  (TOP_SERV - BALL_R)
#define IN_BSRV  (BOT_SERV + BALL_R)
#define IN_SL    (SGL_L - BALL_R)
#define IN_SR    (SGL_R + BALL_R)
#define IN_DL    (DBL_L - BALL_R)
#define IN_DR    (DBL_R + BALL_R)
#define IN_CL    (CENTER_X - BALL_R)     /* ligne médiane de service, vue du carré droit */
#define IN_CR    (CENTER_X + BALL_R)     /* vue du carré gauche */
#endif
