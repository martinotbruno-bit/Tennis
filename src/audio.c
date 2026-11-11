/* Royal Martinot — son. Canal carré 1 + bruit : bruitages ; carré 2 + triangle :
 * musiques et jingles (jamais pendant les échanges, cf. CDC 11.2).
 * Toutes les mélodies sont des compositions originales. */
#include "audio.h"
#include "game.h"

u8 noise_hold;          /* le canal de bruit sert à un bruitage : pas de pas */

const u16 note_period[48] = {
 1709,1613,1523,1437,1356,1280,1208,1140,1076,1016,959,905,
 854,806,761,718,678,640,604,570,538,507,479,452,
 427,403,380,359,338,319,301,284,268,253,239,225,
 213,201,189,179,169,159,150,142,134,126,119,112 };

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

/* état du lecteur, lu par audio_tick (src/sprp.s), appelé par la NMI à
   chaque trame : la musique avance même pendant la construction d'un écran
   (rendu coupé), sans à-coup */
const u8 *mel_ptr, *bass_ptr, *mel_start, *bass_start;
u8 mel_timer, bass_timer, music_loop, mel_duty;
u8 audio_lock;     /* 1 : état en cours de modification, la NMI attend */
u8 music_cur;      /* morceau en cours (MUS_*) */
u8 snd_mode;       /* 0 musique + bruitages, 1 bruitages seuls, 2 aucun son */

void music_play(u8 id)
{
    audio_lock = 1;
    mel_ptr = bass_ptr = 0;
    music_loop = 0;
    mel_duty = 0x86;                    /* petits jingles : note qui s'éteint */
    music_cur = id;
    APU(0x4004) = 0x30;
    APU(0x4008) = 0x80;
    if (snd_mode) { audio_lock = 0; return; }   /* musique coupée (réglage SON) */
    switch (id) {
    case MUS_TITLE:   mel_ptr = mus_title_mel; bass_ptr = mus_title_bass; music_loop = 1; mel_duty = 0x36; break;
    case MUS_POINT:   mel_ptr = mus_point_mel; break;
    case MUS_GAME:    mel_ptr = mus_game_mel; break;
    case MUS_VICTORY: mel_ptr = mus_vict_mel; bass_ptr = mus_vict_bass; mel_duty = 0xB8; break;
    case MUS_DEFEAT:  mel_ptr = mus_def_mel; break;
    case MUS_FINAL:   mel_ptr = mus_final_mel; bass_ptr = mus_final_bass; music_loop = 1; mel_duty = 0xB7; break;
    case MUS_INTRO:   mel_ptr = mus_intro_mel; bass_ptr = mus_intro_bass; mel_duty = 0xB8; break;
    case MUS_CREDITS: mel_ptr = mus_cred_mel; bass_ptr = mus_cred_bass; music_loop = 1; mel_duty = 0x76; break;
    case MUS_BREAK:   mel_ptr = mus_break_mel; bass_ptr = mus_break_bass; music_loop = 1; mel_duty = 0x76; break;
    case MUS_MARGAUX: mel_ptr = mus_margaux_mel; bass_ptr = mus_margaux_bass; music_loop = 1; mel_duty = 0x77; break;
    default: break;
    }
    mel_start = mel_ptr;
    bass_start = bass_ptr;
    mel_timer = bass_timer = 1;
    audio_lock = 0;
}

u8 music_none(void) { return (u8)(mel_ptr == 0); }


static void pulse1(u8 env, u8 sweep, u16 period)
{
    APU(0x4000) = env;
    APU(0x4001) = sweep;
    APU(0x4002) = (u8)period;
    APU(0x4003) = (u8)(0x08 | (period >> 8));
}

/* canal de bruit partagé : chaque son a une priorité ; tant qu'un son plus
   important n'est pas fini (noise_hold), un son moins important est ignoré
   (un rebond ne coupe plus l'ovation, une frappe ne coupe plus le « OOOH ») */
static u8 noise_pri;
static void noise(u8 env, u8 period, u8 pri, u8 hold)
{
    if (noise_hold && pri < noise_pri) return;
    noise_pri = pri;
    noise_hold = hold;
    APU(0x400C) = env;
    APU(0x400E) = period;
    APU(0x400F) = 0x08;
}

void sfx_hit(u8 power)
{
    /* « pok » : note brève qui descend, plus grave si la frappe est forte */
    pulse1(0x81, 0x8A, (u16)(120 + power * 30));
    noise(0x01, (u8)(0x04 + power), 2, 8);
    /* corps du « pok » : note grave et brève au triangle (libre pendant le
       match, sans musique), plus grave quand la frappe est forte */
    if (!bass_ptr) {
        u16 p = (u16)(260 + power * 6);
        APU(0x4008) = 0x0A;             /* compteur linéaire : ~40 ms */
        APU(0x400A) = (u8)p;
        APU(0x400B) = (u8)(0x08 | (p >> 8));
    }
}
/* rebond : son propre à chaque surface (période du bruit, durée de
   l'enveloppe) : terre sourde, gazon doux, dur sec, moquette mate, sable
   feutré, glace claquante */
static const u8 bnc_per[6] = { 0x0E, 0x0D, 0x0A, 0x0C, 0x0F, 0x07 };
static const u8 bnc_env[6] = { 0x02, 0x01, 0x00, 0x01, 0x03, 0x00 };
void sfx_bounce(void)     { u8 s = cfg.surface < 6 ? cfg.surface : 2; noise(bnc_env[s], bnc_per[s], 1, 8); }
void sfx_net(void)        { noise(0x03, 0x0E, 3, 12); pulse1(0x83, 0x00, 900); }
void sfx_fault(void)      { pulse1(0x84, 0x00, 1140); }
void sfx_whistle(void)    { pulse1(0x45, 0xB1, 100); }
void sfx_crowd(u8 s)      { noise((u8)(0x08 + (s & 7)), 0x02, 2, 40); }
void sfx_menu_move(void)  { pulse1(0x81, 0x00, 240); }
void sfx_menu_ok(void)    { pulse1(0x82, 0x89, 180); }
void sfx_toss(void)       { pulse1(0x81, 0x8B, 400); }
/* pas : bruit sourd très bref à faible volume constant, jamais par-dessus un
   rebond, une frappe ou le public */
void sfx_step(void)
{
    if (noise_hold) return;
    APU(0x400C) = 0x11;     /* volume constant 1, compteur de durée actif */
    APU(0x400E) = 0x0D;
    APU(0x400F) = 0x18;     /* durée très courte */
}
/* coup gagnant : « ting » bref qui monte, sur le canal des bruitages (pas de musique) */
void sfx_winner(void)     { pulse1(0x83, 0x9A, 360); }

/* réglage SON : 2 = APU muet, 1 = musique arrêtée, 0 = tout */
void snd_set(u8 m)
{
    snd_mode = m;
    APU(0x4015) = (u8)(m == 2 ? 0x00 : 0x0F);
    if (m) { audio_lock = 1; mel_ptr = bass_ptr = 0; APU(0x4004) = 0x30; APU(0x4008) = 0x80; audio_lock = 0; }
}
/* service canon : « boum » sourd et bref, puis un claquement qui descend */
void sfx_canon(void)      { noise(0x05, 0x0F, 4, 15); pulse1(0x84, 0x93, 140); }
/* frappe parfaite : impact plus sec et plus aigu que la frappe normale */
void sfx_perfect(void)    { pulse1(0x83, 0x8B, 70); noise(0x02, 0x03, 3, 10); }
/* un seul applaudissement timide (générique) */
void sfx_clap(void)       { noise(0x01, 0x05, 2, 8); }
/* public : « OOOH » (balle dehors de peu), soupir (double faute), silence */
void sfx_ooh(void)        { noise(0x0B, 0x05, 4, 40); pulse1(0x4C, 0xBA, 520); }
void sfx_groan(void)      { noise(0x09, 0x09, 4, 30); pulse1(0x4B, 0xB3, 420); }
void sfx_hush(void)       { APU(0x400C) = 0x30; noise_hold = 0; }
/* balle dehors : « bip » du juge électronique (aigu, 1/12 s) */
void sfx_line(void)
{
    APU(0x4000) = 0x9A;                 /* 50 %, volume fixe 10 */
    APU(0x4001) = 0x00;
    APU(0x4002) = 0x5C;
    APU(0x4003) = 0x00;                 /* durée : 10 demi-trames */
}
/* geste de raquette : souffle bref (la frappe passe devant si la balle est touchée) */
void sfx_swing(void)      { noise(0x01, 0x02, 0, 4); }
/* demi-tour brusque sur dur ou moquette : crissement des chaussures (carré 2,
   libre pendant le match) */
static u8 sq_last;
void sfx_squeak(void)
{
    /* au plus un crissement toutes les 2,5 s : un détail, pas un bruit de fond */
    if (mel_ptr || (cfg.surface != 2 && cfg.surface != 3 && cfg.surface != 5) || (u8)(frame_cnt - sq_last) < 150) return;
    sq_last = frame_cnt;
    APU(0x4004) = 0x56;                 /* 25 %, volume fixe 6 : discret */
    APU(0x4005) = 0x8B;                 /* balayage vers l'aigu : « iiik » */
    APU(0x4006) = 0x90;
    APU(0x4007) = 0x48;                 /* durée : 8 demi-trames (67 ms) */
}
/* feu d'artifice : sifflement de la fusée qui monte, puis « pop » */
void sfx_fw_launch(void)  { pulse1(0x4A, 0xBB, 700); }
void sfx_fw_pop(void)     { noise(0x04, 0x0C, 3, 12); pulse1(0x82, 0x00, 1600); }
