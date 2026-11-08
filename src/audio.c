/* Pixel Open — son. Canal carré 1 + bruit : bruitages ; carré 2 + triangle :
 * musiques et jingles (jamais pendant les échanges, cf. CDC 11.2).
 * Toutes les mélodies sont des compositions originales. */
#include "audio.h"

static const u16 note_period[48] = {
 1709,1613,1523,1437,1356,1280,1208,1140,1076,1016,959,905,
 854,806,761,718,678,640,604,570,538,507,479,452,
 427,403,380,359,338,319,301,284,268,253,239,225,
 213,201,189,179,169,159,150,142,134,126,119,112 };

/* notes : index 0..47 (Do2..Si5) ; 0xFE = silence ; 0xFF = fin/boucle.
 * Chaque entrée : note, durée en trames. */
#define R 0xFE
#define END 0xFF
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
#define E5 40
#define F5 41
#define G5 43
#define A5 45

static const u8 mus_title_mel[] = {
 C5,8, E5,8, G5,8, E5,8, F5,8, A5,8, G5,16,
 E5,8, D5,8, C5,8, D5,8, E5,16, G4,16,
 A4,8, C5,8, F5,8, E5,8, D5,8, C5,8, D5,8, E5,8,
 F5,8, E5,8, D5,8, B4,8, C5,24, R,8,
 END,0 };
static const u8 mus_title_bass[] = {
 C3,16, G3,16, F3,16, C3,16, A3,16, E3,16, D3,16, G3,16,
 F3,16, C3,16, D3,16, A3,16, G3,16, G3,16, C3,24, R,8,
 END,0 };
static const u8 mus_point_mel[] = { G4,5, C5,10, END,0 };
static const u8 mus_game_mel[]  = { C5,5, E5,5, G5,5, C5,4, G5,14, END,0 };
static const u8 mus_vict_mel[]  = { C5,6, E5,6, G5,6, C5,6, E5,6, G5,12, R,4, A5,6, G5,6, A5,6, G5,24, END,0 };
static const u8 mus_vict_bass[] = { C3,24, E3,24, F3,16, G3,16, C3,24, END,0 };
static const u8 mus_def_mel[]   = { G4,10, F4,10, E4,10, D4,10, C4,30, END,0 };

static const u8 *mel_ptr, *bass_ptr, *mel_start, *bass_start;
static u8 mel_timer, bass_timer, music_loop;

static void play_pulse2(u8 n)
{
    u16 p;
    if (n == R) { APU(0x4004) = 0x30; return; }
    p = note_period[n];
    APU(0x4004) = 0x86;            /* rapport cyclique 50 %, enveloppe */
    APU(0x4005) = 0x00;
    APU(0x4006) = (u8)p;
    APU(0x4007) = (u8)(0x08 | (p >> 8));
}

static void play_tri(u8 n)
{
    u16 p;
    if (n == R) { APU(0x4008) = 0x80; return; }
    p = note_period[n];
    APU(0x4008) = 0xFF;
    APU(0x400A) = (u8)p;
    APU(0x400B) = (u8)(0x08 | (p >> 8));
}

void music_play(u8 id)
{
    mel_ptr = bass_ptr = 0;
    music_loop = 0;
    APU(0x4004) = 0x30;
    APU(0x4008) = 0x80;
    switch (id) {
    case MUS_TITLE:   mel_ptr = mus_title_mel; bass_ptr = mus_title_bass; music_loop = 1; break;
    case MUS_POINT:   mel_ptr = mus_point_mel; break;
    case MUS_GAME:    mel_ptr = mus_game_mel; break;
    case MUS_VICTORY: mel_ptr = mus_vict_mel; bass_ptr = mus_vict_bass; break;
    case MUS_DEFEAT:  mel_ptr = mus_def_mel; break;
    default: break;
    }
    mel_start = mel_ptr;
    bass_start = bass_ptr;
    mel_timer = bass_timer = 1;
}

void audio_update(void)
{
    if (mel_ptr) {
        if (--mel_timer == 0) {
            if (mel_ptr[0] == END) {
                if (music_loop) { mel_ptr = mel_start; }
                else { mel_ptr = 0; APU(0x4004) = 0x30; }
            }
            if (mel_ptr) {
                play_pulse2(mel_ptr[0]);
                mel_timer = mel_ptr[1];
                mel_ptr += 2;
            }
        }
    }
    if (bass_ptr) {
        if (--bass_timer == 0) {
            if (bass_ptr[0] == END) {
                if (music_loop) { bass_ptr = bass_start; }
                else { bass_ptr = 0; APU(0x4008) = 0x80; }
            }
            if (bass_ptr) {
                play_tri(bass_ptr[0]);
                bass_timer = bass_ptr[1];
                bass_ptr += 2;
            }
        }
    }
}

static void pulse1(u8 env, u8 sweep, u16 period)
{
    APU(0x4000) = env;
    APU(0x4001) = sweep;
    APU(0x4002) = (u8)period;
    APU(0x4003) = (u8)(0x08 | (period >> 8));
}

static void noise(u8 env, u8 period)
{
    APU(0x400C) = env;
    APU(0x400E) = period;
    APU(0x400F) = 0x08;
}

void sfx_hit(u8 power)
{
    /* « pok » : note brève qui descend, plus grave si la frappe est forte */
    pulse1(0x81, 0x8A, (u16)(120 + power * 30));
    noise(0x01, (u8)(0x04 + power));
}
void sfx_bounce(void)     { noise(0x01, 0x0C); }
void sfx_net(void)        { noise(0x03, 0x0E); pulse1(0x83, 0x00, 900); }
void sfx_fault(void)      { pulse1(0x84, 0x00, 1140); }
void sfx_whistle(void)    { pulse1(0x45, 0xB1, 100); }
void sfx_crowd(u8 s)      { noise((u8)(0x08 + (s & 7)), 0x02); }
void sfx_menu_move(void)  { pulse1(0x81, 0x00, 240); }
void sfx_menu_ok(void)    { pulse1(0x82, 0x89, 180); }
void sfx_toss(void)       { pulse1(0x81, 0x8B, 400); }
