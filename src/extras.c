/* Royal Martinot — fonctions annexes du match (banque BANK_EXTRAS) :
 * chargement du court, public animé, ralentis, exercices d'entraînement
 * et tutoriel. Appelées par le moteur via bank_call() ; elles ne peuvent
 * appeler que la banque fixe et échangent leurs données par game_int.h. */
#include "game_int.h"
#include "app.h"
#include "rules.h"
#include "court.h"
#include "gfx.h"
#include "audio.h"
#include "assets.h"

#define side_of(t) ((u8)(((t) & 1) ^ sc_swap))

#pragma static-locals(on)

/* ---------------------------------------------------------------------- */
/* Score : deux panneaux dans les coins supérieurs, au-dessus des tribunes
 * (les joueurs n'y vont jamais) ; annonces au centre du bandeau. */
/* nom d'équipe sur 4 lettres : banque fixe (gamefix.c) */
void team_name(u8 t, char *out);

static const char * const ex_short[NEX] = {
    "FRAP", "DEPL", "SERI", "PREC", "VOLE", "SERV", "ECHG", "PERS", "TUTO" };

/* Club nudiste : l'arbitre ajoute son grain de sel aux annonces. */
static u8 starts(const char *a, const char *b)
{
    while (*a == ' ') ++a;          /* annonces centrées */
    while (*b) if (*a++ != *b++) return 0;
    return 1;
}
static void set2(const char *s) { center14(msg2, s); }
static void nude_quip(void)
{
    u8 i, empty = 1;
    for (i = 0; i < 14; ++i) if (msg2[i] != ' ') empty = 0;
    if (!empty && !starts(msg1, "JEU, SET") && !starts(msg1, "CHANGEMENT")) return;
    if (starts(msg1, "CHANGEMENT")) set2("ET DE CREME !");
    if (starts(msg1, "JEU !")) set2("TENUE : AUCUNE");
    else if (starts(msg1, "FAUTE")) set2("PAS DE POCHE !");
    else if (starts(msg1, "ACE")) set2("ACE AU NATUREL");
    else if (starts(msg1, "COUP GAGNANT")) set2("BRONZAGE TOTAL");
    else if (starts(msg1, "SMASH")) set2("PLEIN SOLEIL !");
    else if (starts(msg1, "AMORTIE")) set2("EN DOUCEUR...");
    else if (starts(msg1, "LOB")) set2("VERS LE SOLEIL");
    else if (starts(msg1, "PASSING")) set2("RIEN A CACHER");
    else if (starts(msg1, "VOLEE")) set2("AIR FRAIS !");
    else if (starts(msg1, "EGALITE")) set2("NU A NU");
    else if (starts(msg1, "FILET")) set2("ATTENTION...");
    else if (starts(msg1, "DEHORS")) set2("COMME NOUS");
    else if (starts(msg1, "QUEL ECHANGE")) set2("TOUT EN NUDITE");
    else if (starts(msg1, "JEU, SET")) set2("ET SERVIETTE !");
    else if (starts(msg1, "BALLE DE MATCH")) set2("SANS FILET");
    else if (starts(msg1, "LET")) set2("CHATOUILLE");
}

void hud_update(void)
{
    static char line[5];
    u8 t, x;
    vbuf_reset();
    if (cfg.mode == GM_MATCH) {
        for (t = 0; t < 2; ++t) {
            /* panneau de l'équipe affichée en bas à gauche, l'autre à droite */
            x = (u8)(side_of(t) == 0 ? 1 : 27);   /* colonne 0 : zone de débordement TV */
            team_name(t, line);
            vbuf_text(x, 1, line, 4);
            /* ligne 2 : jeux du set en cours (« J 3 ») + serveur (*) */
            line[0] = 'J';
            num2(line + 1, sc_games[t]);
            line[3] = (char)(sc_server == t ? '*' : ' ');
            vbuf_text(x, 2, line, 4);
            /* ligne 3 : points du jeu + un rond (@ = balle) par set gagné */
            rules_point_label(t, line);
            line[2] = (char)(sc_sets[t] >= 1 ? '@' : ' ');
            line[3] = (char)(sc_sets[t] >= 2 ? '@' : ' ');
            vbuf_text(x, 3, line, 4);
        }
    } else {
        vbuf_text(1, 1, ex_short[cfg.ex], 4);
        num2(line, tr_score); line[2] = 'P'; line[3] = ' ';
        vbuf_text(1, 2, line, 4);
        if (cfg.ex == EX_STREAK) { line[0] = 'X'; num2(line + 1, tr_streak); }
        else { line[0] = '/'; num2(line + 1, tr_total); }
        line[3] = ' ';
        vbuf_text(1, 3, line, 4);
    }
    if (paused) {
        if (cfg.env == ENV_NUDE) vbuf_text(9, 3, "CREME SOLAIRE?", 14);
        vbuf_text(9, 1, pause_cur ? "  CONTINUER" : "> CONTINUER", 14);
        vbuf_text(9, 2, pause_cur ? "> QUITTER" : "  QUITTER", 14);
    } else if (msg_timer) {
        if (cfg.env == ENV_NUDE) nude_quip();
        vbuf_text(9, 1, msg1, 14);
        vbuf_text(9, 2, msg2, 14);
    } else {
        vbuf_text(9, 1, "", 14);
        vbuf_text(9, 2, sc_tiebreak ? " JEU DECISIF" : "", 14);
    }
    /* panneau du club (masqué pendant la pause par la crème solaire) */
    /* (le nom du club est sur le panneau à côté du score) ; ligne 3 libérée
       après la pause */
    if (cfg.env == ENV_NUDE && !paused) vbuf_text(9, 3, "", 14);
    hud_dirty = 0;
}

/* ---------------------------------------------------------------------- */
/* Pluie : gouttes animées ; une sur deux par trame (8 sprites par ligne) */
#define NRAIN 10
static u8 rain_x[NRAIN], rain_y[NRAIN];
void rain_tick(void)
{
    u8 p;
    for (p = 0; p < NRAIN; ++p) {
        rain_y[p] += 5;
        rain_x[p] -= 1;
        if (rain_y[p] > 230 || rain_y[p] < 24) { rain_y[p] = (u8)(32 + (rand8() & 127)); rain_x[p] = (u8)(40 + (rand8() % 180)); }
    }
}
void rain_draw(void)
{
    u8 i;
    for (i = (u8)(frame_cnt & 1); i < NRAIN; i += 2) spr1(rain_x[i], rain_y[i], 4, 3);
}

/* ---------------------------------------------------------------------- */
extern u8 net_state;
/* copie n tuiles (16 octets chacune) en CHR-RAM, écran coupé */
static void tiles_load(const u8 *idx, u8 n, const u8 *src)
{
    u8 i, j;
    for (i = 0; i < n; ++i) {
        vram_adr((u16)idx[i] << 4);
        for (j = 0; j < 16; ++j) vram_put(*src++);
    }
}
static void crowd_load(void);
void load_court(void)
{
    u8 nude = (u8)(cfg.env == ENV_NUDE);
    vram_unrle(court_nt_rle[cfg.env]);
    people_tiles(nude);
    /* panneaux à côté du score, dans 8 tuiles distinctes inutilisées sur leur
       court (choisies par l'outil de génération : public d'un autre lieu) ;
       ailleurs, on rétablit leur dessin d'origine */
    tiles_load(sign_idx, 8, cfg.env == ENV_MARGAUX ? sign_chr : sign_orig);
    tiles_load(sign2_idx, 8, nude ? sign2_chr : sign2_orig);
    /* filet au repos (au cas où on aurait quitté un match pendant qu'il tremblait) */
    tiles_load(net_idx, 2, net_norm);
    /* arbitre de chaise : 3 tuiles prises à la coupe (inutile pendant le
       match), rétablies hors du match par chr_borrow_restore */
    {
        u8 j;
        vram_adr((u16)(0x1000 + ((u16)trophy_tile << 4)));
        for (j = 0; j < 48; ++j) vram_put(ump_chr[j]);
    }
    net_state = 0;
    net_shake = 0;
    crowd_load();                    /* public : copie des tribunes, regard vers le bas */
}

/* Filet qui tremble quand la balle le touche : les tuiles des mailles
   alternent entre dessin « secoué » et « au repos » (net_shake : 14 -> 0) */
u8 net_shake, net_state;
void net_tick(void)
{
    u8 want = (u8)(net_shake > 1 && ((net_shake >> 2) & 1));
    if (want != net_state) {
        const u8 *s = want ? net_shk : net_norm;
        if (!vbuf_raw((u16)net_idx[0] << 4, s, 16)) return;      /* budget VBlank : trame suivante */
        if (!vbuf_raw((u16)net_idx[1] << 4, s + 16, 16)) return;
        net_state = want;
    }
    --net_shake;
}

/* ---------------------------------------------------------------------- */
/* Public.
 *  - Il suit la balle : les 6 tuiles de spectateurs passent de « regard vers
 *    le bas » à « regard vers le haut » (CHR-RAM, une tuile par trame) quand
 *    la balle change de moitié du court.
 *  - Ola après les beaux points (stade, club, salle) : une bande de 3
 *    rangées de spectateurs bras levés (6 tuiles propres) descend les deux
 *    tribunes, une rangée toutes les 2 trames, puis chaque rangée reprend
 *    son dessin d'origine (copié à l'écran du court par crowd_load).
 *  - Réactions : « OOOH » sur une balle dehors de peu, soupir sur une
 *    double faute, applaudissements rythmés avant une balle de match,
 *    silence au lancer du service.
 *  - Court Margaux et club nudiste : leurs motifs scintillent ou se lèvent,
 *    comme avant. */
#pragma bss-name(push, "WBSS")
static u8 st_map[26 * 8];            /* tribunes : rangées 4 à 29, colonnes 0-3 et 28-31 */
static u8 cr_cur[6], rowbuf[4];
static u8 ola_on, ola_o, cr_prev_t, cr_prev_ms, cr_df, cr_mp, cr_clap;
#pragma bss-name(pop)

#define PPU_DATA (*(volatile u8 *)0x2007)
/* écran du court construit (rendu coupé) : copie des tribunes, tuiles de
   spectateurs remises « regard vers le bas » */
static void crowd_load(void)
{
    u8 r, c, s;
    for (r = 0; r < 26; ++r)
        for (s = 0; s < 2; ++s) {
            vram_adr(NTADR(s ? 28 : 0, 4 + r));
            st_map[r * 8 + s * 4] = PPU_DATA;            /* lecture à vide */
            for (c = 0; c < 4; ++c) st_map[r * 8 + s * 4 + c] = PPU_DATA;
        }
    for (c = 0; c < 6; ++c) {
        vram_adr((u16)crowd_idx[c] << 4);
        for (r = 0; r < 16; ++r) vram_put(crowd_norm[c * 16 + r]);
        vram_adr((u16)crowdb_idx[c] << 4);
        for (r = 0; r < 16; ++r) vram_put(crowdb_norm[c * 16 + r]);
        cr_cur[c] = 0;
    }
    ola_on = 0; cr_prev_t = 0; cr_prev_ms = 0xFF; cr_mp = 0; cr_clap = 0;
    cr_df = (u8)(st_dfaults[0] + st_dfaults[1]);
}

/* rangée o des deux tribunes, debout (bras levés) ou d'origine */
static u8 ola_row(u8 o, u8 up)
{
    u8 s, c, k, t;
    const u8 *m = st_map + o * 8;
    for (s = 0; s < 2; ++s) {
        for (c = 0; c < 4; ++c) {
            t = m[s * 4 + c];
            if (up) for (k = 0; k < 6; ++k) {
                if (t == crowd_idx[k]) { t = cheer_idx[k]; break; }
                if (t == crowdb_idx[k]) { t = cheerb_idx[k]; break; }
            }
            rowbuf[c] = t;
        }
        if (!vbuf_raw(NTADR(s ? 28 : 0, 4 + o), rowbuf, 4)) return 0;
    }
    return 1;
}

/* distance (px) de la balle hors du court (0 : dedans) */
static s16 out_by(void)
{
    s16 x = PX(bx), y = PX(by), d = 0;
    s16 l = cfg.doubles ? DBL_L : SGL_L, r = cfg.doubles ? DBL_R : SGL_R;
    if (x < l) d = l - x; else if (x > r) d = x - r;
    if (y < TOP_BASE) d += TOP_BASE - y; else if (y > BOT_BASE) d += y - BOT_BASE;
    return d;
}
/* balle hors du court de peu (moins de 5 px) : le public retient son souffle */
static u8 near_miss(void)
{
    s16 d = out_by();
    return (u8)(d > 0 && d < 5);
}

void crowd_tick(void)
{
    u8 nude = (u8)(cfg.env == ENV_NUDE), girly = (u8)(cfg.env == ENV_MARGAUX), k, m;
    if (nude || girly) {
        /* court Margaux : les coeurs, fleurs et étoiles scintillent ; club
           nudiste : les spectateurs se lèvent */
        if (crowd_timer) {
            u8 n = (u8)(nude ? 18 : 3);
            u8 cheer = (u8)(crowd_timer >= n && ((crowd_timer >> 3) & 1));
            k = (u8)(crowd_timer % n);
            if (vbuf_raw((u16)(girly ? crowd_idx4 : crowd_idx3)[k] << 4,
                         (girly ? (cheer ? crowd_cheer4 : crowd_norm4) : (cheer ? crowd_cheer3 : crowd_norm3)) + (k << 4), 16))
                --crowd_timer;
        }
    } else {
        /* ola : relancée chaque fois que le jeu ranime le public */
        if (crowd_timer > cr_prev_t) { ola_on = 1; ola_o = 0; }
        if (crowd_timer) --crowd_timer;
        cr_prev_t = crowd_timer;
        if (ola_on && !(frame_cnt & 1)) {
            m = 1;
            if (ola_o < 26) m = ola_row(ola_o, 1);
            if (m && ola_o >= 3) m = ola_row((u8)(ola_o - 3), 0);
            if (m && ++ola_o == 29) ola_on = 0;
        }
        /* le public suit la balle du regard */
        if (b_vis) {
            /* jeu A au stade, jeu B au club et en salle */
            u8 a = (u8)(cfg.env == ENV_STADIUM);
            m = (u8)(by < NET_Y * FP);
            for (k = 0; k < 6; ++k)
                if (cr_cur[k] != m) {
                    if (vbuf_raw((u16)(a ? crowd_idx : crowdb_idx)[k] << 4,
                                 (a ? (m ? crowd_up : crowd_norm) : (m ? crowdb_up : crowdb_norm)) + (k << 4), 16))
                        cr_cur[k] = m;
                    break;
                }
        }
    }
    /* réactions */
    m = ms;
    if (m != cr_prev_ms) {
        if (m == MS_DEAD) {
            if (end_reason == END_POINT && ump_arm && out_by()) sfx_line();   /* balle dehors : bip */
            if (end_reason == END_POINT && ump_arm && near_miss()) sfx_ooh();
            k = (u8)(st_dfaults[0] + st_dfaults[1]);
            if (k != cr_df) { cr_df = k; sfx_groan(); }
        }
        if (m == MS_SERVE) { cr_mp = (u8)(cfg.mode == GM_MATCH && (rules_is_match_point(0) || rules_is_match_point(1))); cr_clap = 0; }
        if (m == MS_TOSS) sfx_hush();            /* silence au lancer */
        cr_prev_ms = m;
    }
    if (m == MS_SERVE && cr_mp && ++cr_clap >= 18) { cr_clap = 0; sfx_clap(); }   /* clap… clap… clap… */
}

/* ---------------------------------------------------------------------- */
/* Ralentis : tampon circulaire de 160 trames dans la RAM de cartouche.
 * Une trame = balle (x, y, z, visible) + 4 joueurs (x, y, pose). */
#pragma bss-name(push, "WBSS")
#define RP_LEN 160
#define RP_SHOW 140
u8 rp_buf[RP_LEN][16];
#pragma bss-name(pop)
u8 rp_head, rp_count;
static u8 rp_pos, rp_left, rp_slow;

/* rp_record() : en assembleur dans la banque fixe (crt0.s), appelée à chaque trame */

void rp_start(void)
{
    rp_left = rp_count < RP_SHOW ? rp_count : RP_SHOW;
    rp_pos = (u8)(rp_head >= rp_left ? rp_head - rp_left : rp_head + RP_LEN - rp_left);
    rp_slow = 0;
    rp_done = (u8)(rp_left == 0);
}

void rp_play(void)
{
    u8 *e = rp_buf[rp_pos];
    u8 ord[4], i, j, k, p, a;
    if (pad1_new & (PAD_A | PAD_B | PAD_START)) { rp_done = 1; rp_count = 0; return; }
    spr_begin();
    if (e[3]) spr_ball(e[0], (s16)e[1] - e[2] - 3, e[1], e[2]);
    for (i = 0; i < npl; ++i) ord[i] = i;
    for (i = 1; i < npl; ++i) {
        k = ord[i];
        for (j = i; j > 0 && e[5 + ord[j - 1] * 3] < e[5 + k * 3]; --j) ord[j] = ord[j - 1];
        ord[j] = k;
    }
    for (i = 0; i < npl; ++i) {
        p = ord[i];
        a = e[6 + p * 3];
        k = (u8)(a >> 5);           /* saut enregistré (j/2) : hauteur j²/16 */
        spr_person(e[4 + p * 3], (s16)e[5 + p * 3] - ((k * k) >> 2), cfg.chr[p], (u8)((a >> 4) & 1), (u8)(a & 7), (u8)((a >> 3) & 1), (u8)(p & 1));
    }
    ump_draw();                      /* l'arbitre reste sur sa chaise pendant le ralenti */
    spr_end();
    /* demi-vitesse : chaque trame enregistrée est montrée deux fois */
    rp_slow ^= 1;
    if (!rp_slow) {
        if (++rp_pos == RP_LEN) rp_pos = 0;
        if (--rp_left == 0) { rp_done = 1; rp_count = 0; }
    }
}

/* ---------------------------------------------------------------------- */
/* Entraînement (CDC 9) */
static u8 tr_count, tr_alt, tr_got;

static void tmsg(const char *a, const char *b, u8 t)
{
    center14(msg1, a);
    center14(msg2, b);
    msg_timer = t;
    hud_dirty = 1;
}

static s8 rnd(u8 spread) { return (s8)((s16)(rand8() % (u8)(spread * 2 + 1)) - spread); }

/* tutoriel : consignes et nombre de réussites demandées par étape */
static const char * const tut_l1[6] = {
    "FRAPPEZ AVEC A", "AMORTIE :", "LOB :", "VISEZ :", "SERVICE : A", "BRAVO !" };
static const char * const tut_l2[6] = {
    "DANS LE COURT", "B + BAS", "B + HAUT", "CROIX + A", "LANCER+FRAPPER", "TUTORIEL FINI" };
static const u8 tut_need[5] = { 3, 2, 2, 2, 2 };

static void tut_show(void)
{
    tmsg(tut_l1[tr_step], tut_l2[tr_step], 250);
}

void train_start(void)
{
    tr_count = cfg.ex == EX_CUSTOM ? cfg.ex_count : 20;
    tr_alt = 0;
    tr_step = 0;
    tr_got = 0;
    tr_human_serves = (u8)(cfg.ex == EX_SERVE);
    if (cfg.ex == EX_VOLLEY) {
        feed_px_set = 1;
        feed_px = CENTER_X * FP;
        feed_py = (NET_Y + 30) * FP;
    }
    switch (cfg.ex) {
    case EX_TARGETS:   tmsg("VISEZ L'ETOILE", "ZONE : +2 PTS", 120); break;
    case EX_MOVE:      tmsg("DEPLACEMENTS", "COIN A COIN", 120); break;
    case EX_STREAK:    tmsg("REGULARITE :", "SANS FAUTE !", 120); break;
    case EX_PRECISION: tmsg("PRECISION :", "SEULE LA ZONE", 120); break;
    case EX_VOLLEY:    tmsg("VOLEE : JOUEZ", "AVANT REBOND", 120); break;
    case EX_SERVE:     tmsg("SERVICE :", "VISEZ L'ETOILE", 120); break;
    case EX_STATIC:    tmsg("ECHANGEZ AVEC", "LE JOUEUR FIXE", 120); break;
    case EX_CUSTOM:    tmsg("EXERCICE", "PERSONNALISE", 120); break;
    case EX_TUTORIAL:  tut_show(); break;
    }
    tgt_x = 0;
    if (cfg.ex == EX_SERVE) { tgt_x = 92; tgt_y = 100; }
}

/* cibles : 0 fond gauche, 1 fond droit, 2 court gauche, 3 court droit */
static const u8 zone_x[4] = { 94, 154, 94, 154 };
static const u8 zone_y[4] = { 58, 58, 100, 100 };

void train_feed(void)
{
    s16 px = PX(plx[0]);
    s16 tx, ty = BOT_BASE - 28;
    u8 T = 50, side;
    feed_x = (CENTER_X + rnd(20)) * FP;
    feed_y = (TOP_BASE - 4) * FP;
    feed_z = 22 * FP;
    feed_g = 9;
    feed_spin = SPIN_TOP;
    if (cfg.ex != EX_STATIC) ++tr_total;
    tx = CENTER_X + rnd(40);
    switch (cfg.ex) {
    case EX_TARGETS:
    case EX_PRECISION:
        tr_target = (u8)(rand8() & (cfg.ex == EX_TARGETS ? 1 : 3));
        tgt_x = zone_x[tr_target]; tgt_y = zone_y[tr_target];
        break;
    case EX_MOVE:
        tr_alt ^= 1;
        tx = tr_alt ? SGL_L + 12 : SGL_R - 12;
        ty = BOT_BASE - 20;
        T = 46;
        break;
    case EX_STREAK:
        tx = CENTER_X + rnd(30);
        T = 54;
        break;
    case EX_VOLLEY:
        tx = px + rnd(26);
        ty = NET_Y + 30;
        T = 32; feed_g = 4; feed_spin = SPIN_FLAT; feed_z = 16 * FP;
        break;
    case EX_CUSTOM:
        side = cfg.ex_side;
        if (side == 2) side = tr_alt ^= 1;
        else if (side == 3) side = (u8)(rand8() & 1);
        tx = side ? px - 28 : px + 28;        /* 0 coup droit (à droite), 1 revers */
        T = (u8)(cfg.ex_speed == 0 ? 62 : cfg.ex_speed == 1 ? 50 : 38);
        if (cfg.ex_spin == 1) { feed_spin = SPIN_SLICE; feed_g = 5; }
        else if (cfg.ex_spin == 2) { feed_g = 6; T += 22; ty = BOT_BASE - 14; }
        break;
    case EX_TUTORIAL:
        T = 60;
        break;
    }
    if (tx < SGL_L + 6) tx = SGL_L + 6;
    if (tx > SGL_R - 6) tx = SGL_R - 6;
    feed_tx = tx; feed_ty = ty; feed_T = T;
}

static void check_end(void)
{
    if (cfg.ex != EX_STREAK && cfg.ex != EX_TUTORIAL && tr_total >= tr_count) {
        tr_done = 1;
        tgt_x = 0;
        tmsg("EXERCICE", "TERMINE !", 200);
    }
}

void train_event(void)
{
    u8 zone;
    hud_dirty = 1;
    if (cfg.ex == EX_TUTORIAL) {
        if ((tr_ev == TEV_LAND_IN && (tr_step == 0 || (tr_step == 1 && last_shot == SH_DROP)
             || (tr_step == 2 && last_shot == SH_LOB) || (tr_step == 3 && last_aim)))
            || (tr_ev == TEV_SERVE_IN && tr_step == 4)) {
            ++tr_score;
            if (++tr_got >= tut_need[tr_step]) {
                tr_got = 0;
                ++tr_step;
                if (tr_step == 4) { tr_human_serves = 1; sc_server = 0; }
                if (tr_step == 5) { tr_done = 1; music_play(MUS_VICTORY); }
                sfx_menu_ok();
            }
            tut_show();
        } else if (tr_ev != TEV_LAND_IN && tr_ev != TEV_SERVE_IN) {
            tut_show();
        }
        if (tr_ev == TEV_SERVE_IN || tr_ev == TEV_SERVE_FAULT) ++tr_total;
        return;
    }
    /* joueur fixe : l'exercice compte les frappes du joueur, pas les balles lancées */
    if (cfg.ex == EX_STATIC && (tr_ev == TEV_LAND_IN || tr_ev == TEV_LAND_OUT || tr_ev == TEV_MISS)) ++tr_total;
    switch (tr_ev) {
    case TEV_LAND_IN:
        if (cfg.ex == EX_STREAK) {
            ++tr_streak;
            if (tr_streak > tr_score) tr_score = tr_streak;
            tmsg("SERIE", "", 30);
            msg1[6] = (char)(tr_streak >= 10 ? '0' + tr_streak / 10 : ' ');
            msg1[7] = (char)('0' + tr_streak % 10);
            if (tr_streak >= 50) { tr_done = 1; }
            break;
        }
        if (cfg.ex == EX_TARGETS || cfg.ex == EX_PRECISION) {
            zone = (u8)((ev_x >= CENTER_X ? 1 : 0) | (ev_y >= TOP_SERV + 6 ? 2 : 0));
            if (zone == tr_target) { tr_score += 2; tmsg("CIBLE ! +2", "", 40); sfx_crowd(4); }
            else if (cfg.ex == EX_TARGETS) { ++tr_score; tmsg("BONNE +1", "", 40); }
            else tmsg("HORS ZONE", "", 40);
            break;
        }
        ++tr_score;
        tmsg("BONNE +1", "", 40);
        break;
    case TEV_LAND_OUT:
    case TEV_MISS:
        if (cfg.ex == EX_STREAK) {
            tr_done = 1;
            tmsg("FIN DE SERIE", "", 200);
            return;
        }
        tmsg(tr_ev == TEV_MISS ? "MANQUE" : "DEHORS", "", 40);
        break;
    case TEV_SERVE_IN:
        ++tr_total;
        ++tr_score;
        /* bonus si le service tombe dans le tiers visé (extérieur ou T) */
        if (ev_x < SGL_L + 18 || ev_x > SGL_R - 18 || (ev_x > CENTER_X - 18 && ev_x < CENTER_X + 18)) {
            ++tr_score; tmsg("ACE ? +2", "", 40);
        } else tmsg("BON +1", "", 40);
        break;
    case TEV_SERVE_FAULT:
        ++tr_total;
        break;
    }
    if (cfg.ex == EX_SERVE) {
        /* étoile : alterne la cible à chaque service selon le côté */
        tgt_x = (u8)(((sc_points[0] ^ 1) & 1) ? 160 : 92);   /* côté du prochain service */
    }
    check_end();
}
