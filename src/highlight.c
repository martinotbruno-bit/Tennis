/* Royal Martinot — banque 15 (version 2.67) : récapitulatif du score (venu
 * de la banque 0) ; mise en valeur du point pendant le ralenti : gros plan
 * de la frappe décisive et arrêt sur image zoomé au rebond. */
#include "game_int.h"
#include "app.h"
#include "rules.h"
#include "court.h"
#include "gfx.h"
#include "audio.h"

void set_msg(const char *a, const char *b, u8 t);   /* banque fixe */
void team_name(u8 t, char *out);
extern u8 recap_pend;

/* ---------------------------------------------------------------------- */
/* Récapitulatif du score : une ligne par équipe, « ALEX 6 3 2 », sets    */
/* terminés puis jeux du set en cours (14 caractères pour 5 sets).        */
/* ---------------------------------------------------------------------- */
void score_line(u8 t, char *out)
{
    u8 i, k = 4;
    team_name(t, out);
    for (i = 0; i < sc_set_no && i < MAX_SETS; ++i) { out[k++] = ' '; out[k++] = (char)('0' + sc_hist[i][t]); }
    if (!sc_over && k < 14) { out[k++] = ' '; out[k++] = (char)('0' + sc_games[t]); }
    out[k] = 0;                     /* les deux lignes ont la même longueur : centrées ensemble */
}

/* bandeau : le score set par set pendant 2 secondes */
static char recap_a[16], recap_b[16];
void score_recap(void)
{
    score_line(0, recap_a);
    score_line(1, recap_b);
    set_msg(recap_a, recap_b, 120);
    recap_pend = 0;
}

/* ====================================================================== */
/* Mise en valeur du point pendant le ralenti (version 2.67)               */
/*                                                                        */
/* Le ralenti (rp_play, banque 3) s'arrête sur deux repères posés pendant */
/* le point (game.c) : la frappe décisive de l'équipe gagnante, puis le    */
/* rebond qui a conclu. hl_mode passe alors à 1 ou 2 et match_tick appelle */
/* hl_tick à chaque trame jusqu'à la fin de la séquence.                   */
/*                                                                        */
/* 1. Gros plan : image figée, flash ; plan moyen en grand (image haute    */
/*    définition de la bande-annonce, aux couleurs du joueur) avec arrivée */
/*    de caméra en travelling, balle qui arrive sur le tamis, impact et    */
/*    traînée, titre du coup ; puis gros plan du visage qui exulte ;       */
/*    retour au court.                                                     */
/* 2. Zoom : réticule qui se resserre sur le point de rebond, flash, puis  */
/*    image x 2 calculée pendant le flash : décor (tuiles de l'écran lues  */
/*    dans la mémoire du PPU et agrandies), joueurs et balle (sprites de   */
/*    la trame figée, agrandis), trace de la balle, BONNE ou DEHORS.       */
/* ====================================================================== */
#include "assets.h"
#include "assets_tz.h"

#define PPU_DATA (*(volatile u8 *)0x2007)
/* repères posés pendant le point (game.c) : pour chaque équipe, sa
   dernière frappe (indice de la trame dans le tampon du ralenti, joueur,
   coup) ; premier rebond de la balle en jeu (indice, position, dedans) ; en
   fin de point, la frappe décisive de l'équipe gagnante et le rebond qui a
   conclu sont recopiés dans hl_s* (lus pendant le ralenti) */
#pragma bss-name(push, "WBSS")
static u8 hl_hi[2], hl_hp[2], hl_hk[2], hl_hv;
static u16 hl_hf[2], hl_sf;
u16 hl_bf, hl_sbf;                 /* rp_seq au rebond ; recopié en fin de point */
u8 hl_bi, hl_bx, hl_by, hl_bin, hl_bok, hl_w;
const char *hl_ptxt;
u8 hl_si, hl_sp, hl_sk, hl_sok, hl_sbi, hl_sbx, hl_sby, hl_sbin, hl_sbok;
char hl_txt[16];
#pragma bss-name(pop)
extern u8 rp_head;
extern u16 rp_seq;
/* frappe : pk = joueur | coup << 8 (bit 15 : service, début du point) */
u16 __fastcall__ hl_hit(u16 pk)
{
    u8 p = (u8)pk, k = (u8)(pk >> 8), t = (u8)(p & 1);
    if (k & 0x80) { hl_hv = 0; k &= 0x7F; }
    hl_hi[t] = rp_head; hl_hp[t] = p; hl_hk[t] = k; hl_hf[t] = rp_seq;
    hl_hv |= (u8)(1 << t);
    hl_bok = 0;                     /* nouvelle balle : pas encore de rebond */
    return 0;
}
/* début du ralenti : les repères ne valent que s'ils sont dans les trames
   rejouées (tampon circulaire de 160 trames : un indice plus ancien a été
   réutilisé par une trame récente) */
extern u8 rp_left;
extern u16 rp_seq;                 /* trames enregistrées (crt0.s) */
static u8 hl_fresh(u16 f)
{
    /* âge exact en trames enregistrées : dans les trames rejouées */
    return (u8)(rp_seq - f <= rp_left);
}
void hl_check(void)
{
    if (cfg.env == ENV_NUDE) { hl_sok = hl_sbok = 0; return; }   /* (club nudiste : ni gros plan habillé, ni sprites agrandis) */
    /* frappe décisive plus ancienne que le ralenti (lob, long échange
       conclu par une faute adverse) : le gros plan ouvre le ralenti (son
       image ne dépend pas de la trame) */
    if (!hl_fresh(hl_sf)) hl_si = (u8)(rp_head >= rp_left ? rp_head - rp_left : rp_head + 160 - rp_left);
    if (!hl_fresh(hl_sbf)) hl_sbok = 0;
}
/* fin de point : hl_w gagnant, hl_ptxt titre */
void hl_select(void)
{
    u8 i = 0;
    const char *txt = hl_ptxt;
    hl_sok = (u8)((hl_hv >> hl_w) & 1);
    hl_si = hl_hi[hl_w]; hl_sp = hl_hp[hl_w]; hl_sk = hl_hk[hl_w]; hl_sf = hl_hf[hl_w]; hl_sbf = hl_bf;
    hl_sbok = hl_bok; hl_sbi = hl_bi; hl_sbx = hl_bx; hl_sby = hl_by; hl_sbin = hl_bin;
    while (*txt == ' ') ++txt;      /* (messages centrés : sans les blancs) */
    while (i < 15 && txt[i]) { hl_txt[i] = txt[i]; ++i; }
    while (i && hl_txt[i - 1] == ' ') --i;
    hl_txt[i] = 0;
}
extern u8 ctrl_swap, bg_dirty, swap_on;
extern u8 proj_x, proj_y;
#pragma zpsym("proj_x")
#pragma zpsym("proj_y")
void __fastcall__ proj2(u8 x, u8 y);           /* sprp.s */
void court_reload(void);                       /* banque 0 */
void rp_play(void);                            /* banque 3 */
extern const unsigned char swap_base;

#pragma bss-name(push, "WBSS")
u8 hl_mode;
static u8 hl_t, hl_c, hl_n, hl_shake;
static s16 hl_cam;
static u8 hl_rx, hl_ry;                        /* point visé (zoom), à l'écran */
static u8 hl_bk;                               /* couleur de fond du court */
static const char *zl;                         /* titre du zoom : BONNE ou DEHORS */
#pragma bss-name(pop)

/* tampons du zoom : zone libre de la RAM sauvegardée, après les blocs
   sauvegardés (jamais lue au démarrage ; voir stats.c) */
#pragma bss-name(push, "SAVEDATA")
static u8 zb_nt[240];          /* fenêtre de 16 x 15 tuiles du court */
static u8 zb_at[64];           /* attributs de l'écran du court */
static u8 zb_map[256];         /* tuile source -> entrée de zb_q (0xFF : pas encore) */
#define ZQ 80
static u8 zb_q[ZQ][4];         /* les 4 quarts agrandis d'une tuile source */
static u8 zb_src[16], zb_out[16];
#define ZS 12
static u8 zb_sx[ZS], zb_sy[ZS], zb_st[ZS], zb_sa[ZS];   /* sprites de la fenêtre */
static u8 zb_spr[ZS][16];      /* leurs tuiles */
#pragma bss-name(pop)

static const u8 dbl[16] = { 0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F,
                            0xC0, 0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF };

static u8 vdummy;
static void vread(u16 a, u8 *d, u8 n)
{
    vram_adr(a);
    vdummy = PPU_DATA;             /* lecture différée du PPU : la première rend l'ancien tampon */
    while (n--) *d++ = PPU_DATA;
}
static void vwrite(u16 a, const u8 *d, u8 n)
{
    vram_adr(a);
    while (n--) vram_put(*d++);
}
/* quart q (0 haut gauche, 1 haut droit, 2 bas gauche, 3 bas droit) de
   zb_src agrandi deux fois dans zb_out ; rend 0..3 si le quart est d'une
   seule couleur (tuile unie), 0xFF sinon */
static u8 quarter(u8 q)
{
    register u8 r;
    u8 sr, b0, b1, o0, o1, solid = 1;
    for (r = 0; r < 8; ++r) {
        sr = (u8)(((q >> 1) << 2) + (r >> 1));
        b0 = zb_src[sr]; b1 = zb_src[8 + sr];
        if (q & 1) { b0 &= 15; b1 &= 15; } else { b0 >>= 4; b1 >>= 4; }
        zb_out[r] = o0 = dbl[b0]; zb_out[8 + r] = o1 = dbl[b1];
        if ((o0 != 0 && o0 != 0xFF) || (o1 != 0 && o1 != 0xFF) || o0 != zb_out[0] || o1 != zb_out[8]) solid = 0;
    }
    return solid ? (u8)((zb_out[0] ? 1 : 0) | (zb_out[8] ? 2 : 0)) : 0xFF;
}

/* ---------------------------------------------------------------------- */
/* Gros plan                                                               */
/* ---------------------------------------------------------------------- */
static const u8 whip_in[6] = { 150, 90, 48, 22, 8, 2 };
#define T_BACK  0xFB          /* fond uni (couleur 0) */
#define T_GND   0xFC          /* sol (couleur 3) */
static const u8 z8[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const u8 ff[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static void flash(u8 c)
{
    pal_buf[0] = pal_buf[4] = pal_buf[8] = pal_buf[12] = pal_buf[16] = c;
    pal_dirty = 1;
}

/* titre : lettres de la police chargées dans la zone des jeux de tuiles
   (rétablie par load_court au retour), dessinées en sprites sur une ou
   deux lignes (8 sprites au plus par ligne) */
static u8 lbl_n, lbl_brk;
static void label_load(void)
{
    u8 i, c;
    lbl_n = 0; lbl_brk = 0xFF;
    for (i = 0; hl_txt[i]; ++i) {
        c = (u8)hl_txt[i];
        if (c == ' ' && lbl_brk == 0xFF && i >= 3) lbl_brk = i;
        if (c > ' ' && c < 0x60) chr_copy(BANK_CHR, chr_data + ((u16)c << 4), 0x1000 + ((u16)(swap_base + i) << 4), 16);
    }
    lbl_n = i;
    /* une seule ligne si elle tient en 8 lettres */
    for (c = 0, i = 0; i < lbl_n; ++i) if (hl_txt[i] != ' ') ++c;
    if (c <= 8) lbl_brk = 0xFF;
}
static void label_draw(u8 y, u8 shown)
{
    u8 i, x, l0 = lbl_brk == 0xFF ? lbl_n : lbl_brk, l1, yy = y;
    x = (u8)(128 - l0 * 4);
    for (i = 0; i < lbl_n && i < shown; ++i) {
        if (i == lbl_brk) { l1 = (u8)(lbl_n - i - 1); x = (u8)(128 - l1 * 4); yy = (u8)(y + 10); continue; }
        if (hl_txt[i] != ' ') spr1(x, yy, (u8)(swap_base + i), 2);
        x += 8;
    }
}

/* un plan : image haute définition, fond uni, sol ; palettes du joueur */
static void cut_build(u8 art)
{
    u8 i, S, O, D, z, c = cfg.chr[hl_sp];
    const tz_art_t *a = &tz_art[art][char_female[c] ? (c == 16 ? 2 : 1) : 0];
    const u8 *m;
    u16 ad;
    z = a->zoom;
    flash(0x30);                        /* (écran blanc pendant le chargement) */
    fade_skip = 1; fade_lvl = 0;
    ppu_off();
    vram_adr(0x0000); vram_fill(0, 16);
    tz_bank = a->bank; tz_n = a->ntile;
    tz_unpack(a->chr);
    vram_adr((u16)T_BACK << 4); for (i = 0; i < 8; ++i) vram_put(z8[i]); for (i = 0; i < 8; ++i) vram_put(z8[i]);
    vram_adr((u16)T_GND << 4); for (i = 0; i < 8; ++i) vram_put(ff[i]); for (i = 0; i < 8; ++i) vram_put(ff[i]);
    vram_adr(0x2000);
    for (i = 0; i < 30; ++i) vram_fill(z || i < 27 ? T_BACK : T_GND, 32);
    vram_adr(0x2400); vram_fill(T_BACK, 960);
    m = a->map; ad = NTADR(a->x0, a->y0);
    for (i = 0; i < a->h; ++i) { vram_adr(ad); ad += 32; tz_n = a->w; tz_copy(m); m += a->w; }
    vram_adr(0x23C0); tz_n = 64; tz_copy(a->at);
    vram_adr(0x27C0); vram_fill(0, 64);
    /* palettes (comme la bande-annonce) */
    pal_set_player(61, c);
    S = pal_buf[5]; O = pal_buf[6]; D = pal_buf[7];
    flash(z ? 0x06 : 0x01);             /* gros plan : rouge sombre ; plan moyen : nuit bleue */
    if (S >= 0x10) { pal_buf[5] = S; pal_buf[6] = (u8)(S - 0x10); } else { pal_buf[5] = (u8)(S + 0x10); pal_buf[6] = S; }
    if (O >= 0x10) { pal_buf[9] = O; pal_buf[10] = (u8)(O - 0x10); } else { pal_buf[9] = (u8)(O + 0x10); pal_buf[10] = O; }
    if (O == 0x30) pal_buf[10] = 0x10;
    if (D == 0x10) D = 0x00;
    pal_buf[7] = pal_buf[11] = D;
    pal_buf[13] = pal_buf[5]; pal_buf[14] = pal_buf[9]; pal_buf[15] = 0x30;
    if (z) { pal_buf[1] = pal_buf[5]; pal_buf[2] = D; pal_buf[3] = 0x30; pal_buf[14] = pal_buf[6]; pal_buf[15] = pal_buf[9]; }
    else { pal_buf[1] = 0x0F; pal_buf[2] = 0x02; pal_buf[3] = 0x11; }
    pal_buf[16 + 9] = 0x0F; pal_buf[16 + 10] = 0x0F; pal_buf[16 + 11] = 0x30;   /* titre : blanc, ombre noire */
    pal_buf[16 + 13] = 0x38; pal_buf[16 + 14] = 0x27; pal_buf[16 + 15] = 0x30;  /* balle */
    pal_dirty = 1;
    hl_cam = z ? 0 : -(s16)whip_in[0];
    scroll_x = (u8)hl_cam; scroll_nt = (u8)((hl_cam >> 8) & 1); scroll_y = 0;
    spr_begin(); spr_end();
    label_load();                       /* (lettres du titre, rendu coupé) */
    ppu_on();
}

/* retour au court : tuiles d'origine (les deux tables), écran du court,
   puis la trame du ralenti */
static void back_to_court(void)
{
    flash(0x30);
    fade_skip = 1; fade_lvl = 0;
    ppu_off();
    ctrl_swap = 0;
    scroll_x = 0; scroll_y = 0; scroll_nt = 0;
    chr_reload();
    bg_dirty = 0;
    swap_on = 0;
    bank_call(BANK_GAME, court_reload);  /* (rend aussi les palettes du court) */
    hl_mode = 0;
    bank_call(BANK_EXTRAS, rp_play);
    if (rp_done) { spr_begin(); spr_end(); }   /* (ralenti passé : pas de sprites du gros plan) */
    ppu_on();
}

#define SPR(px, py, t, at) { sx = (s16)(px) - hl_cam; if (sx >= 0 && sx < 248) spr1((u8)sx, (u8)(py), (u8)(t), (u8)(at)); }
static void closeup_tick(void)
{
    u8 i, k;
    s16 sx;
    ++hl_t;
    if (hl_t < 12) {                     /* image figée */
        if (hl_t == 9) { flash(0x30); sfx_perfect(); }
        if (hl_t == 11) cut_build(TZ_IMPACT);
        return;
    }
    hl_c = (u8)(hl_t - 11);             /* temps du plan */
    spr_begin();
    if (hl_t < 70) {
        /* plan moyen : travelling éclair, puis lente dérive */
        hl_cam = hl_c < 6 ? -(s16)whip_in[hl_c] : (s16)(hl_c >> 2);
        scroll_x = (u8)hl_cam; scroll_nt = (u8)((hl_cam >> 8) & 1);
        if ((hl_c & 3) == 0 && hl_c > 2) flash(0x01);
        /* la balle arrive sur le tamis, impact (flash), traînée */
        if (hl_c >= 8 && hl_c < 22) {
            k = (u8)(22 - hl_c);
            SPR(TZ_IMPACT_RACKET_X + k * 9, TZ_IMPACT_RACKET_Y - 4 + k * 5, ball_base + 8, 3);
        } else if (hl_c == 22) {
            flash(0x30); sfx_hit(3);
        } else if (hl_c > 22 && hl_c < 34) {
            for (i = 0; i < 3; ++i) {
                k = (u8)((hl_c - 22) * 10 - i * 6);
                if (k < 40) SPR(TZ_IMPACT_RACKET_X + 4 - k, TZ_IMPACT_RACKET_Y - 8 - (k >> 1), ball_base + 8, 3);
            }
        }
        /* titre, lettre par lettre */
        if (hl_c >= 26) label_draw(206, (u8)((hl_c - 26) >> 1));
        if (hl_c == 26) sfx_crowd(7);
        spr_end();
        if (hl_t == 69) { flash(0x30); sfx_winner(); }
        return;
    }
    if (hl_t == 70) { cut_build(TZ_VICTORY_Z); hl_shake = 6; }
    /* gros plan : la joie, l'écran tremble puis s'apaise ; confettis */
    if (hl_shake) { scroll_y = (u8)((hl_t & 1) ? hl_shake : 0); if ((hl_t & 3) == 0) --hl_shake; } else scroll_y = 0;
    for (i = 0; i < 10; ++i)
        spr1((u8)(i * 25 + 8), (u8)(((hl_t * (1 + (i & 1)) + i * 23) & 0x9F) + 8), confetti_tile, (u8)(i & 3));
    label_draw(206, 16);
    spr_end();
    if (hl_t >= 118) back_to_court();
}

/* ---------------------------------------------------------------------- */
/* Arrêt sur image zoomé                                                   */
/* ---------------------------------------------------------------------- */
static const u8 t_corner[16] = { 0xF0, 0x80, 0x80, 0x80, 0, 0, 0, 0,  0xF0, 0x80, 0x80, 0x80, 0, 0, 0, 0 };
static const u8 t_mark[16] =   { 0x00, 0x3C, 0x7E, 0xFF, 0xFF, 0x7E, 0x3C, 0x00,  0x00, 0x00, 0x3C, 0x7E, 0x7E, 0x3C, 0x00, 0x00 };
#define T_CORNER (swap_base + 20)
#define T_MARK   (swap_base + 21)
static u8 zx, zy, zwx, zwy;            /* point visé dans l'image agrandie ; coin de la fenêtre (px) */

static void zoom_build(void)
{
    register u8 i;
    u8 r, c, k, n, q, nq = 0, next = 4, ns = 0, src, a;
    u16 ad;
    u8 tx0, ty0;
    flash(0x30);
    fade_skip = 1; fade_lvl = 0;
    ppu_off();
    /* fenêtre de 128 x 120 px autour du point visé, alignée sur les tuiles,
       dans le court (rangées 4 à 29) */
    a = (u8)(hl_rx < 64 ? 0 : (u8)((hl_rx - 64) >> 3)); tx0 = a > 16 ? 16 : a;
    a = (u8)(hl_ry < 92 ? 4 : (u8)((hl_ry - 60) >> 3)); ty0 = a > 15 ? 15 : a;
    zwx = (u8)(tx0 << 3); zwy = (u8)(ty0 << 3);
    zx = (u8)((hl_rx - zwx) << 1); zy = (u8)((hl_ry - zwy) << 1);
    /* sprites de la trame figée dans la fenêtre (balle d'abord : elle est en
       tête), sans les coins du réticule (emplacements 60 à 63) */
    for (i = 0; i < 60 && ns < ZS; ++i) {
        u8 y = oam_buf[i * 4], x = oam_buf[i * 4 + 3];
        if (y >= 0xEF) continue;
        ++y;
        if ((u8)(x - zwx) >= 124 || x < zwx || (u8)(y - zwy) >= 116 || y < zwy) continue;
        zb_sx[ns] = (u8)(x - zwx); zb_sy[ns] = (u8)(y - zwy);
        zb_st[ns] = oam_buf[i * 4 + 1]; zb_sa[ns] = oam_buf[i * 4 + 2];
        vread(0x1000 + ((u16)zb_st[ns] << 4), zb_spr[ns], 16);
        ++ns;
    }
    /* décor : fenêtre et attributs */
    vread(0x23C0, zb_at, 64);
    for (r = 0; r < 15; ++r) vread(NTADR(tx0, ty0 + r), zb_nt + r * 16, 16);
    /* tuiles agrandies : écrites dans la table $1000 (le fond y est lu
       pendant le zoom, ctrl_swap), les sources restent lisibles en $0000 */
    for (c = 0; c < 4; ++c) {           /* 0 à 3 : tuiles unies */
        for (i = 0; i < 16; ++i) zb_out[i] = (u8)(((i < 8 ? c & 1 : c & 2)) ? 0xFF : 0);
        vwrite(0x1000 + ((u16)c << 4), zb_out, 16);
    }
    for (i = 0; ; ++i) { zb_map[i] = 0xFF; if (i == 255) break; }
    for (k = 0; k < 240; ++k) {
        src = zb_nt[k];
        if (zb_map[src] != 0xFF) continue;
        if (nq >= ZQ) { zb_map[src] = 0xFE; continue; }
        vread((u16)src << 4, zb_src, 16);
        for (q = 0; q < 4; ++q) {
            n = quarter(q);
            if (n == 0xFF) {
                if (next == 0) n = 0;    /* (plus de place : uni) */
                else { n = next; vwrite(0x1000 + ((u16)next << 4), zb_out, 16); if (++next == 0) next = 0; }
            }
            zb_q[nq][q] = n;
        }
        zb_map[src] = nq++;
    }
    /* table de noms agrandie (écran 0) */
    for (r = 0; r < 15; ++r) {
        ad = NTADR(0, r * 2);
        for (q = 0; q < 2; ++q) {
            vram_adr(ad + q * 32);
            for (c = 0; c < 16; ++c) {
                k = zb_map[zb_nt[r * 16 + c]];
                if (k >= ZQ) { vram_put(0); vram_put(0); }
                else { vram_put(zb_q[k][q * 2]); vram_put(zb_q[k][q * 2 + 1]); }
            }
        }
    }
    /* attributs : chaque tuile source devient un bloc de 16 x 16 */
    vram_adr(0x23C0);
    for (r = 0; r < 8; ++r)
        for (c = 0; c < 8; ++c) {
            u8 v = 0, qq;
            for (qq = 0; qq < 4; ++qq) {
                u8 sx = (u8)(tx0 + c * 2 + (qq & 1)), sy = (u8)(ty0 + r * 2 + (qq >> 1)), p;
                if (sy > 29) sy = 29;
                p = (u8)((zb_at[(sy >> 2) * 8 + (sx >> 2)] >> (((sy & 2) << 1) | (sx & 2))) & 3);
                v |= (u8)(p << (qq * 2));
            }
            vram_put(v);
        }
    /* sprites agrandis : tuiles en $0000 (table des sprites pendant le zoom) */
    spr_begin();
    next = 0;
    for (k = 0; k < ns; ++k) {
        u8 id[4], hf = (u8)(zb_sa[k] & 0x40), vf = (u8)(zb_sa[k] & 0x80);
        for (i = 0; i < 16; ++i) zb_src[i] = zb_spr[k][i];
        for (q = 0; q < 4; ++q) {
            quarter(q);
            for (i = 0, n = 0; i < 16; ++i) n |= zb_out[i];
            if (!n) { id[q] = 0xFF; continue; }
            id[q] = next;
            vwrite((u16)next << 4, zb_out, 16);
            ++next;
        }
        for (q = 0; q < 4; ++q) {
            /* quart affiché q : quart source selon les miroirs */
            u8 s = (u8)(((hf ? 1 : 0) ^ (q & 1)) | (((vf ? 2 : 0) ^ (q & 2))));
            u16 x = (u16)zb_sx[k] * 2 + (q & 1) * 8, y = (u16)zb_sy[k] * 2 + (q >> 1) * 8;
            if (id[s] == 0xFF || x > 248 || y > 230 || oam_i >= 208) continue;   /* (52 à 63 : trace et titre) */
            spr1((u8)x, (u8)y, id[s], zb_sa[k]);
        }
    }
    /* trace de la balle et titre (police) */
    vwrite((u16)next << 4, t_mark, 16);
    hl_n = next;
    for (i = 0; zl[i]; ++i)
        if ((u8)zl[i] > ' ' && (u8)zl[i] < 0x60)
            chr_copy(BANK_CHR, chr_data + ((u16)(u8)zl[i] << 4), (u16)(next + 1 + i) << 4, 16);
    lbl_n = i; lbl_brk = 0xFF;
    spr_end();
    ctrl_swap = 0x18;
    scroll_x = 0; scroll_y = 0; scroll_nt = 0;
    pal_buf[16 + 9] = 0x0F; pal_buf[16 + 10] = 0x0F; pal_buf[16 + 11] = 0x30;
    flash(hl_bk);
    ppu_on();
}

static void zoom_tick(void)
{
    u8 i, k, d, ob;
    ++hl_t;
    if (hl_t < 16) {
        /* réticule : 4 coins qui se resserrent sur le point visé, par-dessus
           l'image figée (emplacements 60 à 63 des sprites) */
        if (hl_t == 1) { hl_bk = pal_buf[0]; vbuf_raw((u16)(0x1000 + ((u16)T_CORNER << 4)), t_corner, 16); return; }
        d = (u8)(40 - hl_t * 2);
        ob = 240;
        for (k = 0; k < 4; ++k, ob += 4) {
            oam_buf[ob] = (u8)(hl_ry - 1 + ((k & 2) ? d - 8 : -(s8)d));
            oam_buf[ob + 1] = T_CORNER;
            oam_buf[ob + 2] = (u8)(2 | ((k & 1) ? 0x40 : 0) | ((k & 2) ? 0x80 : 0));
            oam_buf[ob + 3] = (u8)(hl_rx + ((k & 1) ? d - 8 : -(s8)d));
        }
        if (hl_t == 13) { flash(0x30); sfx_menu_ok(); }
        if (hl_t == 15) zoom_build();
        return;
    }
    /* image agrandie : trace de la balle qui clignote, titre */
    hl_c = (u8)(hl_t - 15);
    if (hl_c == 1) { if (hl_sbin) sfx_crowd(7); else sfx_out(); }
    /* (les sprites agrandis restent ; seuls la trace et le titre changent :
       on les écrit aux emplacements 52 à 63) */
    ob = 208;
    for (i = 0; i < 12; ++i) oam_buf[ob + i * 4] = 0xFF;
    if ((hl_c & 8) || hl_c > 48) {
        oam_buf[ob] = (u8)(zy - 5); oam_buf[ob + 1] = hl_n; oam_buf[ob + 2] = 3; oam_buf[ob + 3] = (u8)(zx - 4);
    }
    if (hl_c > 10) {
        /* titre sur une partie unie du court, loin du point visé */
        u8 x = (u8)(128 - lbl_n * 4), ly = (u8)(zy > 120 ? 79 : 159);
        for (i = 0, k = 1; zl[i] && k < 12; ++i, x += 8) {
            if (zl[i] == ' ') continue;
            oam_buf[ob + k * 4] = ly; oam_buf[ob + k * 4 + 1] = (u8)(hl_n + 1 + i);
            oam_buf[ob + k * 4 + 2] = 2; oam_buf[ob + k * 4 + 3] = x;
            ++k;
        }
    }
    if (hl_c >= 80) back_to_court();
}

void hl_tick(void)
{
    if (pad1_new & (PAD_A | PAD_B | PAD_START)) {   /* passer : fin du ralenti */
        back_to_court();
        rp_done = 1; hl_sok = hl_sbok = 0; hl_t = 0;
        return;
    }
    if (hl_t == 0 && hl_mode == 2) {
        /* point visé : le rebond, à l'écran (projeté en perspective) ; texte */
        hl_rx = hl_sbx; hl_ry = hl_sby;
        if (persp) { proj2(hl_sbx, hl_sby); hl_rx = proj_x; hl_ry = proj_y; }
        zl = hl_sbin ? "BONNE !" : "DEHORS !";
    }
    if (hl_mode == 1) closeup_tick(); else zoom_tick();
    if (!hl_mode) hl_t = 0;
}
