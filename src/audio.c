/* Royal Martinot — son. Bruitages : carré 1 et bruit ; musiques et jingles
 * (jamais pendant les échanges, cf. CDC 11.2) : carré 2 (mélodie), triangle
 * (basse), carré 1 (harmonie) et bruit (percussions), ces deux derniers
 * cédant la place aux bruitages.
 * Toutes les mélodies sont des compositions originales. */
#include "audio.h"
#include "game.h"
#include "app.h"
#include <string.h>

u8 noise_hold;          /* le canal de bruit sert à un bruitage : pas de pas */

const u16 note_period[48] = {
 1709,1613,1523,1437,1356,1280,1208,1140,1076,1016,959,905,
 854,806,761,718,678,640,604,570,538,507,479,452,
 427,403,380,359,338,319,301,284,268,253,239,225,
 213,201,189,179,169,159,150,142,134,126,119,112 };

/* état du lecteur, lu par audio_tick (src/sprp.s), appelé par la NMI à
   chaque trame : la musique avance même pendant la construction d'un écran
   (rendu coupé), sans à-coup. 4 voix : mélodie, basse, harmonie, percussions
   (trk_ptr, trk_start, trk_timer : définis dans sprp.s). Les morceaux sont
   dans la banque 10 (src/music_data.c, généré par tools/gen_music.py) et
   copiés ici par music_play : la NMI peut ainsi les lire quelle que soit la
   banque en place, et la banque fixe garde la place des échantillons. */
extern const u8 *trk_ptr[4], *trk_start[4];
extern u8 trk_timer[8], music_loop, mel_duty, harm_duty, p1_hold;
extern const u8 * const song_trk[20][4];
extern const u8 song_loop[20], song_duty[20][2];
extern const u16 song_len[20][4];
#define MUS_BUF 520
#pragma bss-name(push, "WBSS")
static u8 mus_buf[MUS_BUF];
#pragma bss-name(pop)
u8 audio_lock;     /* 1 : état en cours de modification, la NMI attend */
u8 music_cur;      /* morceau en cours (MUS_*) */
u8 snd_mode;       /* 0 musique + bruitages, 1 bruitages seuls, 2 aucun son */

static void music_stop(void)
{
    u8 k;
    for (k = 0; k < 4; ++k) trk_ptr[k] = 0;
    APU(0x4004) = 0x30;
    APU(0x4008) = 0x80;
    if (!p1_hold) APU(0x4000) = 0x30;
}

void music_play(u8 id)
{
    u8 k, prev;
    u16 o = 0;
    const u8 *src;
    audio_lock = 1;
    music_stop();
    music_cur = id;
    if (snd_mode || !id || id > MUS_ROCK) { audio_lock = 0; return; }   /* musique coupée (réglage SON) */
    prev = cur_bank;
    set_bank(BANK_MUSIC);
    music_loop = song_loop[id];
    mel_duty = song_duty[id][0];
    harm_duty = song_duty[id][1];
    for (k = 0; k < 4; ++k) {
        src = song_trk[id][k];
        if (!src) continue;
        trk_ptr[k] = trk_start[k] = mus_buf + o;
        trk_timer[k << 1] = 1;
        memcpy(mus_buf + o, src, song_len[id][k]);   /* copie rapide (bibliothèque, assembleur) */
        o += song_len[id][k];
    }
    set_bank(prev);
    audio_lock = 0;
}

u8 music_none(void) { return (u8)(trk_ptr[0] == 0); }


/* Balayage « inactif » : $08 (sens descendant, décalage 0) et non $00. Sur
   la vraie NES, avec le sens montant et un décalage 0, l'unité de balayage
   calcule en permanence une période cible double ; au-delà de $7FF elle
   coupe le canal, même désactivée : toute note de période > $3FF serait
   muette. (Les émulateurs simples, dont jsnes, ne le reproduisent pas.) */
static void pulse1(u8 env, u8 sweep, u16 period)
{
    p1_hold = 12;                       /* l'harmonie de la musique attend */
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

/* Échantillons DPCM (src/dpcm.s, banque fixe) : voix de l'arbitre et « pok ».
   Un « pok » ne coupe jamais une annonce. Au court Margaux, la princesse
   annonce d'une voix plus aiguë (lecture 2 crans plus rapide). */
extern const u8 dpcm_addr[], dpcm_len[], dpcm_rate[];
#define V_OUT 0
#define V_FAULT 1
#define V_LET 2
#define V_POK 3
static void dpcm(u8 k)
{
    u8 r = dpcm_rate[k];
    if (snd_mode == 2) return;
    if (k == V_POK && (APU(0x4015) & 0x10)) return;      /* une annonce est en cours */
    if (k != V_POK && cfg.env == ENV_MARGAUX) r += 2;
    APU(0x4015) = 0x0F;                 /* arrête l'échantillon en cours */
    APU(0x4010) = r;
    APU(0x4012) = dpcm_addr[k];
    APU(0x4013) = dpcm_len[k];
    APU(0x4015) = 0x1F;
}
void sfx_hit(u8 power)
{
    /* « pok » : note brève qui descend, plus grave si la frappe est forte */
    pulse1(0x81, 0x8A, (u16)(120 + power * 30));
    noise(0x01, (u8)(0x04 + power), 2, 8);
    /* corps du « pok » : note grave et brève au triangle (libre pendant le
       match, sans musique), plus grave quand la frappe est forte */
    dpcm(V_POK);                        /* « pok » échantillonné */
    if (!trk_ptr[1]) {
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
void sfx_net(void)        { noise(0x03, 0x0E, 3, 12); pulse1(0x83, 0x08, 900); }
void sfx_fault(void)      { pulse1(0x84, 0x08, 1140); }
void sfx_whistle(void)    { pulse1(0x45, 0xB1, 100); }
void sfx_crowd(u8 s)      { noise((u8)(0x08 + (s & 7)), 0x02, 2, 40); }
void sfx_menu_move(void)  { pulse1(0x81, 0x08, 240); }
void sfx_menu_ok(void)    { pulse1(0x82, 0x89, 180); }
/* changer une valeur : petit glissando vers l'aigu (droite) ou le grave
   (gauche) ; retour : note qui descend, plus longue */
void sfx_menu_val(u8 right) { pulse1(0x41, (u8)(right ? 0x9A : 0x92), (u16)(right ? 320 : 220)); }
void sfx_menu_back(void)  { pulse1(0x83, 0xA3, 200); }
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
    if (m) { audio_lock = 1; music_stop(); audio_lock = 0; }
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
/* annonce : la voix doit dominer. Bip discret (volume 3) au lieu du bip
   franc, et le public se tait le temps de l'annonce (le canal de bruit est
   réservé 30 trames : le murmure lancé à la fin du point est coupé) */
static void announce(u8 k)
{
    APU(0x4000) = 0x93;                 /* bip discret : 50 %, volume fixe 3 */
    APU(0x4001) = 0x08;
    APU(0x4002) = 0x5C;
    APU(0x4003) = 0x00;
    p1_hold = 12;
    APU(0x400C) = 0x30;                 /* silence du public */
    noise_pri = 5; noise_hold = 30;
    dpcm(k);
}
void sfx_out(void)        { announce(V_OUT); }     /* « Out! » */
void sfx_svfault(void)    { announce(V_FAULT); }   /* service faux : « Fault! » */
void sfx_let(void)        { announce(V_LET); }     /* let : « Let! » */

/* balle dehors : « bip » du juge électronique (aigu, 1/12 s) */
void sfx_line(void)
{
    p1_hold = 12;
    APU(0x4000) = 0x9A;                 /* 50 %, volume fixe 10 */
    APU(0x4001) = 0x08;
    APU(0x4002) = 0x5C;
    APU(0x4003) = 0x00;                 /* durée : 10 demi-trames */
}
/* geste de raquette : souffle bref (la frappe passe devant si la balle est touchée) */
void sfx_swing(void)      { noise(0x01, 0x02, 0, 4); }
/* feu d'artifice : sifflement de la fusée qui monte, puis « pop » */
void sfx_fw_launch(void)  { pulse1(0x4A, 0xBB, 700); }
void sfx_fw_pop(void)     { noise(0x04, 0x0C, 3, 12); pulse1(0x82, 0x08, 1600); }
