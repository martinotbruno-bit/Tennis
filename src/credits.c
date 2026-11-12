/* Royal Martinot — générique de fin (banque BANK_CREDITS), après une saison
 * gagnée. Il défile de bas en haut, comme au cinéma, et se prend toujours
 * aussi au sérieux : équipe de production, postes absurdes, mentions
 * légales, puis « THE END »… et l'annonce d'une suite très improbable.
 *
 * Défilement : la NES n'a que 30 rangées par écran. Pour écrire les
 * nouvelles lignes hors de la vue, le MMC1 passe en miroir horizontal : les
 * deux écrans ($2000 puis $2800) sont empilés, soit 60 rangées. Une ligne du
 * générique occupe 2 rangées (texte + interligne, 16 px) : 30 emplacements,
 * dont environ 15 visibles. L'emplacement 16 lignes plus bas que le haut de
 * l'écran est réécrit (texte, interligne, attributs) chaque fois que le
 * défilement avance de 16 px. */
#include "app.h"
#include "gfx.h"
#include "audio.h"
#include "game.h"
#include "comp.h"

#pragma static-locals(on)

/* une ligne : texte (« @ » : nom du champion) et couleur (0 blanc, 1 or) */
typedef struct { const char *s; u8 pal; } Line;
static const Line lines[] = {
    { "ROYAL MARTINOT", 1 },
    { "GRAND SLAM PRESTIGE", 0 },
    { "PLATINUM DIAMOND EDITION", 0 },
    { "", 0 }, { "", 0 },
    { "#", 0 },          /* SEASON CHAMPION, ou le titre du Grand Martinot */
    { "@", 1 },
    { "", 0 }, { "", 0 },
    { "EXECUTIVE PRODUCER", 0 },
    { "BRUNO MARTINOT", 1 },
    { "", 0 },
    { "CHIEF EXECUTIVE SON", 0 },
    { "AUGUSTIN MARTINOT", 1 },
    { "", 0 },
    { "SPECIAL GUEST STAR", 0 },
    { "MARGAUX", 1 },
    { "", 0 },
    { "HEAD OF BALL PHYSICS", 0 },
    { "ISAAC NEWTON (UNPAID)", 1 },
    { "", 0 },
    { "INTERN", 0 },
    { "THE INTERN", 1 },
    { "(STILL SORRY ABOUT THE N)", 0 },
    { "", 0 },
    { "PIXELS HAND-PLACED BY", 0 },
    { "VERY SMALL TWEEZERS", 1 },
    { "", 0 },
    { "COLOUR CONSULTANT", 0 },
    { "DELTA E 25", 1 },
    { "", 0 },
    { "MUSIC", 0 },
    { "A FANFARE THAT DERAILED", 1 },
    { "", 0 },
    { "CATERING", 0 },
    { "THE FAMILY FRIDGE", 1 },
    { "", 0 },
    { "LEGAL DEPARTMENT", 0 },
    { "NOBODY. GOOD LUCK.", 1 },
    { "", 0 },
    { "FILMED ON LOCATION IN", 0 },
    { "THE LIVING ROOM", 1 },
    { "", 0 }, { "", 0 },
    { "NO TENNIS BALLS WERE HARMED", 0 },
    { "(ONE WAS SLIGHTLY BOUNCED)", 0 },
    { "", 0 },
    { "ANY RESEMBLANCE TO REAL", 0 },
    { "CHAMPIONS IS ENTIRELY", 0 },
    { "DESERVED.", 0 },
    { "", 0 }, { "", 0 }, { "", 0 },
    { "THANK YOU FOR PLAYING", 0 },
    { "", 0 },
    { "THE END", 1 },
    { "", 0 }, { "", 0 }, { "", 0 }, { "", 0 }, { "", 0 }, { "", 0 },
    { "...", 0 },
    { "", 0 }, { "", 0 }, { "", 0 },
    { "WAIT.", 0 },
    { "", 0 },
    { "ROYAL MARTINOT 2", 1 },
    { "COMING SOON", 0 },
    { "(PROBABLY NOT)", 0 },
};
#define NLINES (sizeof(lines) / sizeof(lines[0]))
#define FIRST 16                         /* lignes vides au départ (écran vide) */
/* arrêt quand la dernière ligne est au milieu de l'écran */
#define T_STOP ((u16)(NLINES - 1 + FIRST - 7) * 16)

#pragma bss-name(push, "WBSS")
static u16 tpos;                         /* défilement total, en pixels */
static u8 at[2][64];                     /* attributs des deux écrans */
static u8 halt, wait_t;
static char row[33];
#pragma bss-name(pop)

/* emplacement s (0..29) : rangées 2s et 2s+1 de l'ensemble des 60 rangées */
static void write_slot(u8 s, u8 i)
{
    u8 n, k, x, nt = (u8)(s >= 15), ls = (u8)(s % 15), pal = 0, b, v, sh;
    const char *t = "";
    u16 base = nt ? 0x2800 : 0x2000;
    static char name[8];
    if (i < NLINES) {
        t = lines[i].s; pal = lines[i].pal;
        if (t[0] == '#')
            t = c_type < CT_GRAND ? "SEASON CHAMPION" : c_slam ? "GRAND SLAM CHAMPION" :
                c_type == CT_GRAND ? "LE GRAND MARTINOT CHAMPION" : "MARTINWORLD TOUR CHAMPION";
        if (t[0] == '@') {
            /* le champion de la saison (ou l'équipe, en double) */
            if (c_dbl) team_label(c_pl[0], c_pl2[0], name);
            else { const char *p = char_name[c_pl[0]]; for (k = 0; k < 7 && p[k] > ' '; ++k) name[k] = p[k]; name[k] = 0; }
            t = name;
        }
    }
    for (n = 0; t[n]; ++n) ;
    x = (u8)((32 - n) >> 1);
    for (k = 0; k < 32; ++k) row[k] = (char)(k >= x && k < x + n ? t[k - x] : ' ');
    vbuf_raw((u16)(base + (u16)(ls * 2) * 32), (const u8 *)row, 32);
    for (k = 0; k < 32; ++k) row[k] = ' ';
    vbuf_raw((u16)(base + (u16)(ls * 2 + 1) * 32), (const u8 *)row, 32);
    /* attributs : moitié haute ou basse de la rangée de blocs ls/2 */
    b = (u8)(ls >> 1);
    sh = (u8)((ls & 1) << 2);
    v = (u8)((pal | (pal << 2)) << sh);
    for (k = 0; k < 8; ++k) at[nt][b * 8 + k] = (u8)((at[nt][b * 8 + k] & (u8)~(0x0F << sh)) | v);
    vbuf_raw((u16)(base + 0x3C0 + b * 8), &at[nt][b * 8], 8);
}

void credits_begin(void)
{
    u8 k;
    ppu_off();
    mirror_h(1);
    vram_adr(0x2000); vram_fill(' ', 960); vram_fill(0, 64);
    vram_adr(0x2800); vram_fill(' ', 960); vram_fill(0, 64);
    for (k = 0; k < 64; ++k) at[0][k] = at[1][k] = 0;
    pal_set_menu();
    pal_buf[7] = 0x28;                 /* palette 1 du décor : texte or */
    spr_begin(); spr_end();
    scroll_y = scroll_nt = 0;
    tpos = 0; halt = 0; wait_t = 0;
    vbuf_reset();
    ppu_on();
    music_play(MUS_CREDITS);
}

static void finish(void)
{
    music_play(MUS_NONE);
    ppu_off();
    scroll_y = scroll_nt = 0;
    mirror_h(0);
    go_req = GO_SCREEN;
    go_arg = SCR_TITLE;
}

void credits_tick(void)
{
    u8 step, p;
    u16 pp;
    vbuf_reset();
    if (halt) {
        /* fin : un seul applaudissement timide, puis A pour revenir */
        ++wait_t;
        if (wait_t == 60) sfx_clap();
        if (wait_t > 60 && (pad1_new & (PAD_A | PAD_START))) finish();
        if (wait_t == 255) wait_t = 254;
        return;
    }
    /* 1 px toutes les 2 trames ; A ou START maintenu : 2 px par trame */
    step = (u8)((pad1 & (PAD_A | PAD_START)) ? 2 : ((frame_cnt & 1) ? 1 : 0));
    for (; step; --step) {
        if ((tpos & 15) == 0) {
            p = (u8)(tpos >> 4);                    /* emplacement en haut de l'écran */
            write_slot((u8)((p + FIRST) % 30), p);   /* ligne p du générique */
        }
        if (tpos >= T_STOP) { halt = 1; break; }
        ++tpos;
    }
    pp = tpos % 480;
    scroll_nt = (u8)(pp >= 240 ? 2 : 0);
    scroll_y = (u8)(pp >= 240 ? pp - 240 : pp);
}
