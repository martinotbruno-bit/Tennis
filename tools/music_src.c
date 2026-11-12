/* Royal Martinot — partitions des musiques (source de tools/gen_music.py).
 * Ce fichier n'est pas compilé : gen_music.py le lit, ajoute l'harmonie
 * (carré 1) et les percussions (bruit) de chaque morceau et écrit
 * src/music_data.c (banque 10). Toutes les mélodies sont originales. */
/* notes : index 0..47 (Do2..Si5) ; 0xFE = silence ; 0xFF = fin/boucle.
 * Chaque entrée : note, durée en trames. */
#define R 0xFE
#define END 0xFF
#define G2 7
#define C3 12
#define D3 14
#define E3 16
#define F3 17
#define G3 19
#define A3 21
#define B3 23
#define C4 24
#define D4 26
#define E4 28
#define F4 29
#define G4 31
#define A4 33
#define B4 35
#define C5 36
#define D5 38
#define DS5 39
#define E5 40
#define F5 41
#define FS5 42
#define G5 43
#define A5 45
#define B5 47
#define A2 9
#define B2 11
#define AS4 34


/* Écran titre et menu : petite polka de cirque, ridicule mais propre et bien
 * carrée. Mélodie sautillante liée, sans aucun silence (carré 12,5 % « kazoo » à
 * volume constant : aucune note ne s'éteint avant la suivante), broderies chromatiques comiques ; « oum-pa » du triangle qui saute
 * du grave à l'octave. 16 mesures à 2 temps de 48 trames (12,8 s) ; la
 * dernière mesure remonte vers le Do du début : la boucle est sans couture. */
static const u8 mus_title_mel[] = {
 C5, 12, G4, 12, E5, 12, G4, 12, F5, 12, E5, 12, D5, 24, B4, 12,
 G4, 12, D5, 12, G4, 12, F5, 12, D5, 12, E5, 24, C5, 12, G4, 12,
 E5, 12, G4, 12, A5, 12, G5, 12, F5, 12, E5, 12, D5, 12, F5, 12,
 B4, 12, D5, 12, C5, 24, C4, 24, E5, 12, DS5, 12, E5, 12, G5, 12,
 F5, 12, E5, 12, F5, 12, A5, 12, G5, 12, FS5, 12, G5, 12, E5, 12,
 D5, 24, G4, 24, E5, 12, DS5, 12, E5, 12, G5, 12, F5, 12, E5, 12,
 F5, 12, A5, 12, G5, 12, E5, 12, D5, 12, B4, 12, C5, 12, G3, 12,
 G4, 12, B4, 12,
 END,0 };
static const u8 mus_title_bass[] = {
 C3, 12, C4, 12, G3, 12, C4, 12, G2, 12, G3, 12, D3, 12, G3, 12,
 G2, 12, G3, 12, D3, 12, G3, 12, C3, 12, C4, 12, G3, 12, C4, 12,
 C3, 12, C4, 12, G3, 12, C4, 12, F3, 12, F4, 12, C3, 12, F4, 12,
 G2, 12, G3, 12, D3, 12, G3, 12, C3, 12, C4, 12, G3, 12, C4, 12,
 C3, 12, C4, 12, G3, 12, C4, 12, F3, 12, F4, 12, C3, 12, F4, 12,
 C3, 12, C4, 12, G3, 12, C4, 12, G2, 12, G3, 12, D3, 12, G3, 12,
 C3, 12, C4, 12, G3, 12, C4, 12, F3, 12, F4, 12, C3, 12, F4, 12,
 G2, 12, G3, 12, D3, 12, G3, 12, C3, 12, C4, 12, G3, 12, C4, 12,
 END,0 };

static const u8 mus_point_mel[] = { G4,5, C5,10, END,0 };
static const u8 mus_game_mel[]  = { C5,5, E5,5, G5,5, C5,4, G5,14, END,0 };
/* Victoire : fanfare joyeuse de 160 trames, exactement la durée de la fête
 * (« JEU, SET ET MATCH ! ») ; elle s'achève sur l'écran des résultats. */
static const u8 mus_vict_mel[]  = {
 G4,6, C5,6, E5,6, G5,12, E5,6, G5,18, A4,6, D5,6, F5,6, A5,12, F5,6, A5,18,
 G5,6, A5,6, B5,6, G5,6, E5,6, D5,6, C5,16, END,0 };
static const u8 mus_vict_bass[] = {
 C3,12, G3,12, E3,12, G3,12, C3,6, F3,12, C4,12, A3,12, C4,12, F3,6,
 G3,12, B3,12, G3,12, C3,16, END,0 };
static const u8 mus_def_mel[]   = { G4,10, F4,10, E4,10, D4,10, C4,30, END,0 };

/* Thème de la finale (écrans d'avant-match) : marche solennelle, 8 mesures de 80 trames */
static const u8 mus_final_mel[] = {
 G4,9, R,1, G4,4, R,1, G4,5, C5,20, E5,20, G5,20,
 A5,20, G5,10, F5,10, E5,10, D5,10, E5,20,
 G4,9, R,1, G4,4, R,1, G4,5, D5,20, F5,20, A5,20,
 G5,20, F5,10, D5,10, B4,10, D5,10, G4,20,
 E5,19, R,1, E5,10, F5,10, G5,20, E5,20,
 A5,20, F5,20, G5,20, E5,20,
 F5,10, E5,10, D5,10, C5,10, D5,20, B4,20,
 C5,60, R,20,
 END,0 };
static const u8 mus_final_bass[] = {
 C3,40, G3,40, F3,40, C3,40, G3,40, D3,40, G3,40, B3,40,
 C3,40, E3,40, F3,40, C3,40, G3,40, G3,40, C3,60, R,20,
 END,0 };
/* Thème du court Margaux : petite valse façon boîte à musique, 16 mesures de 36 trames */
static const u8 mus_margaux_mel[] = {
 C5,12, E5,12, G5,12,  A5,24, G5,12,  F5,12, A5,12, F5,12,  E5,24, C5,12,
 D5,12, F5,12, A5,12,  G5,24, E5,12,  F5,12, E5,12, D5,12,  C5,24, R,12,
 E5,11, R,1, E5,6, F5,6, G5,12,  A5,12, G5,12, E5,12,  F5,11, R,1, F5,6, G5,6, A5,12,  G5,35, R,1,
 G5,12, E5,12, C5,12,  F5,12, A5,12, F5,12,  E5,12, D5,12, G4,12,  C5,24, R,12,
 END,0 };
static const u8 mus_margaux_bass[] = {
 C3,24, G3,12, F3,24, C4,12, F3,24, A3,12, C3,24, G3,12,
 D3,24, A3,12, C3,24, G3,12, G3,24, B3,12, C3,24, G3,12,
 C3,24, G3,12, A3,24, E3,12, F3,24, C4,12, G3,24, D3,12,
 C3,24, G3,12, F3,24, A3,12, G3,24, B3,12, C3,24, R,12,
 END,0 };

/* Fanfare du générique, « qui déraille » : 8 mesures majestueuses, puis des
   fausses notes, le tempo qui s'emballe, une glissade chromatique jusqu'au
   grave, un silence gêné, et un petit « pouet » quand le logo apparaît.
   Notes : index chromatiques (C3 = 12) ; 15 = ré dièse 3, 18 = fa dièse 3,
   37 = do dièse 5, 39 = ré dièse 5, 44 = sol dièse 5. */
static const u8 mus_intro_mel[] = {
 /* majestueux (480 trames) */
 C4,30, G4,15, C5,15,  E5,45, D5,15,  C5,15, D5,15, E5,15, G5,15,  A5,60,
 G5,30, E5,15, C5,15,  F5,45, E5,15,  D5,15, E5,15, F5,15, A5,15,  G5,60,
 /* ça déraille (240) */
 G4,14, R,1, G4,7, R,1, G4,7, C5,30,  E5,15, 39,8, E5,7, G5,30,  A5,20, 44,20, F5,20,  E5,30, 37,30,
 /* ça s'emballe (160) */
 C5,8, D5,8, E5,8, F5,8, G5,8, A5,8, B5,8, A5,8, G5,8, F5,8,
 E5,6, G5,6, C5,6, E5,6, G5,6, C5,6, E5,6, G5,6, C5,6, E5,6, G5,6, C5,6, B4,8,
 /* frénésie (96) puis glissade du si 5 au do 3, 2 trames par demi-ton (72) */
 C5,4, E5,4, G5,4, B5,4, C5,4, E5,4, G5,4, B5,4, C5,4, E5,4, G5,4, B5,4,
 C5,4, E5,4, G5,4, B5,4, C5,4, E5,4, G5,4, B5,4, C5,4, E5,4, G5,4, B5,4,
 47,2, 46,2, 45,2, 44,2, 43,2, 42,2, 41,2, 40,2, 39,2, 38,2, 37,2, 36,2,
 35,2, 34,2, 33,2, 32,2, 31,2, 30,2, 29,2, 28,2, 27,2, 26,2, 25,2, 24,2,
 23,2, 22,2, 21,2, 20,2, 19,2, 18,2, 17,2, 16,2, 15,2, 14,2, 13,2, 12,2,
 /* silence gêné, puis « pou-et » */
 R,92,  D4,6, C4,18,
 END,0 };
static const u8 mus_intro_bass[] = {
 C3,60,  C3,30, G3,30,  C3,60,  F3,60,
 C3,60,  F3,30, C3,30,  D3,30, F3,30,  G3,60,
 C3,60,  C3,30, 15,30,  F3,60,  18,60,
 C3,20, G3,20, C3,20, G3,20,  F3,16, C3,16, G3,16, D3,16, G3,16,
 C3,24, E3,24, G3,24, C4,24,
 24,6, 23,6, 22,6, 21,6, 20,6, 19,6, 18,6, 17,6, 16,6, 15,6, 14,6, 13,6,
 R,2,  R,92,  R,24,
 END,0 };

/* Générique de fin : hymne solennel… joué au kazoo (carré 25 % à volume
 * constant, notes liées), sur une basse en blanches. 8 mesures de 96 trames
 * (4 temps de 24), en boucle ; la dernière mesure remonte vers le début. */
static const u8 mus_cred_mel[] = {
 E5,24, D5,24, C5,24, G4,24,   A4,24, C5,24, F5,48,
 E5,24, D5,24, C5,24, A4,24,   G4,48, A4,24, B4,24,
 C5,24, E5,24, G5,48,          A5,36, G5,12, F5,24, E5,24,
 D5,24, F5,24, E5,24, D5,24,   C5,72, G4,24,
 END,0 };
static const u8 mus_cred_bass[] = {
 C3,48, G3,48,  F3,48, C4,48,  A3,48, E3,48,  G3,48, D3,48,
 C3,48, E3,48,  F3,48, C3,48,  G3,48, D3,48,  C3,48, G3,48,
 END,0 };

/* Interlude de fin de set : air calme et léger, 8 mesures de 48 trames */
static const u8 mus_break_mel[] = {
 E5,12, G5,12, A5,12, G5,12,  E5,24, C5,24,  D5,12, E5,12, F5,12, A5,12,  G5,36, R,12,
 A5,12, G5,12, E5,12, D5,12,  C5,24, E5,24,  D5,12, F5,12, E5,12, D5,12,  C5,36, R,12,
 END,0 };
static const u8 mus_break_bass[] = {
 C3,24, G3,24,  A3,24, E3,24,  D3,24, F3,24,  G3,24, D3,24,
 F3,24, C3,24,  A3,24, E3,24,  D3,24, G3,24,  C3,36, R,12,
 END,0 };

/* Le Grand Martinot : fanfares originales des écrans d'annonce des tournois
   (version 2.44), une par tournoi, environ 3 à 4 secondes, sans boucle. */
/* Open d'Augustralie : air sautillant, comme un kangourou (2 temps de 24) */
static const u8 mus_gm1_mel[] = {
 G4,6, R,6, B4,6, R,6, D5,12, B4,12,   C5,6, R,6, E5,6, R,6, G5,24,
 F5,6, E5,6, D5,6, C5,6, B4,12, D5,12, G5,36, R,12,
 END,0 };
static const u8 mus_gm1_bass[] = {
 G3,24, D3,24,  C3,24, G3,24,  D3,24, D3,24,  G3,48,
 END,0 };
/* Debogarros : valse musette (3 temps de 12) */
static const u8 mus_gm2_mel[] = {
 E5,12, D5,6, E5,6, G5,12,   F5,12, E5,6, F5,6, A5,12,   G5,12, F5,6, E5,6, D5,12,
 C5,24, R,12,                E5,6, F5,6, G5,12, E5,12,   D5,12, B4,12, G4,12,
 C5,36,
 END,0 };
static const u8 mus_gm2_bass[] = {
 C3,12, G3,12, G3,12,  F3,12, C4,12, C4,12,  G2,12, D3,12, D3,12,  C3,12, G3,12, G3,12,
 C3,12, G3,12, G3,12,  G2,12, D3,12, D3,12,  C3,36,
 END,0 };
/* Martinbledon : fanfare de cuivres, solennelle (4 temps de 12) */
static const u8 mus_gm3_mel[] = {
 C5,12, C5,6, C5,6, G4,24,   E5,12, E5,6, E5,6, C5,24,
 G5,12, F5,12, E5,12, D5,12, C5,36, R,12,
 END,0 };
static const u8 mus_gm3_bass[] = {
 C3,24, G2,24,  C3,24, E3,24,  G2,24, G3,24,  C3,48,
 END,0 };
/* US Opaul : petit swing, notes « bleues » (4 temps de 12) */
static const u8 mus_gm4_mel[] = {
 C5,8, DS5,4, E5,12, G5,12, A5,12,   G5,8, E5,4, C5,12, D5,12, DS5,12,
 E5,8, G5,4, A5,12, G5,8, E5,4, C5,12,   D5,12, B4,12, C5,24,
 END,0 };
static const u8 mus_gm4_bass[] = {
 C3,12, E3,12, G3,12, A3,12,  F3,12, A3,12, C4,12, A3,12,
 C3,12, E3,12, G3,12, E3,12,  G2,12, B2,12, C3,24,
 END,0 };

