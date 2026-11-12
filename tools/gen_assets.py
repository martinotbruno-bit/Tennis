#!/usr/bin/env python3
"""Génère toutes les ressources graphiques originales de Royal Martinot.

Sorties :
  assets/tiles.chr      8 Ko de CHR (table 0 = décors/texte, table 1 = sprites)
  src/assets_data.c     nametables compressées + tables de tuiles des sprites
  assets/preview_*.png  aperçus (si Pillow est disponible)

Tout est dessiné par programme : aucune ressource externe.
"""
import os, sys, random

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
random.seed(1234)  # génération déterministe (compilation reproductible)

# --------------------------------------------------------------------------
# Géométrie du court (doit rester identique à src/court.h)
# --------------------------------------------------------------------------
TOP_BASE, BOT_BASE, NET_Y = 48, 216, 132
TOP_SERV, BOT_SERV = 87, 177
SGL_L, SGL_R = 74, 182
DBL_L, DBL_R = 54, 202
CENTER_X = 128

# --------------------------------------------------------------------------
# Police 5x7 (majuscules sans accents, chiffres, ponctuation)
# --------------------------------------------------------------------------
FONT = {
 "'":["00100","00100","01000","00000","00000","00000","00000"],
 'A':["01110","10001","10001","11111","10001","10001","10001"],
 'B':["11110","10001","10001","11110","10001","10001","11110"],
 'C':["01110","10001","10000","10000","10000","10001","01110"],
 'D':["11110","10001","10001","10001","10001","10001","11110"],
 'E':["11111","10000","10000","11110","10000","10000","11111"],
 'F':["11111","10000","10000","11110","10000","10000","10000"],
 'G':["01110","10001","10000","10111","10001","10001","01111"],
 'H':["10001","10001","10001","11111","10001","10001","10001"],
 'I':["01110","00100","00100","00100","00100","00100","01110"],
 'J':["00111","00010","00010","00010","00010","10010","01100"],
 'K':["10001","10010","10100","11000","10100","10010","10001"],
 'L':["10000","10000","10000","10000","10000","10000","11111"],
 'M':["10001","11011","10101","10101","10001","10001","10001"],
 'N':["10001","11001","10101","10011","10001","10001","10001"],
 'O':["01110","10001","10001","10001","10001","10001","01110"],
 'P':["11110","10001","10001","11110","10000","10000","10000"],
 'Q':["01110","10001","10001","10001","10101","10010","01101"],
 'R':["11110","10001","10001","11110","10100","10010","10001"],
 'S':["01111","10000","10000","01110","00001","00001","11110"],
 'T':["11111","00100","00100","00100","00100","00100","00100"],
 'U':["10001","10001","10001","10001","10001","10001","01110"],
 'V':["10001","10001","10001","10001","10001","01010","00100"],
 'W':["10001","10001","10001","10101","10101","10101","01010"],
 'X':["10001","10001","01010","00100","01010","10001","10001"],
 'Y':["10001","10001","01010","00100","00100","00100","00100"],
 'Z':["11111","00001","00010","00100","01000","10000","11111"],
 '0':["01110","10001","10011","10101","11001","10001","01110"],
 '1':["00100","01100","00100","00100","00100","00100","01110"],
 '2':["01110","10001","00001","00010","00100","01000","11111"],
 '3':["11111","00010","00100","00010","00001","10001","01110"],
 '4':["00010","00110","01010","10010","11111","00010","00010"],
 '5':["11111","10000","11110","00001","00001","10001","01110"],
 '6':["00110","01000","10000","11110","10001","10001","01110"],
 '7':["11111","00001","00010","00100","01000","01000","01000"],
 '8':["01110","10001","10001","01110","10001","10001","01110"],
 '9':["01110","10001","10001","01111","00001","00010","01100"],
 ':':["00000","00100","00100","00000","00100","00100","00000"],
 '-':["00000","00000","00000","11111","00000","00000","00000"],
 '.':["00000","00000","00000","00000","00000","01100","01100"],
 '!':["00100","00100","00100","00100","00100","00000","00100"],
 '?':["01110","10001","00001","00010","00100","00000","00100"],
 '/':["00001","00010","00010","00100","01000","01000","10000"],
 "'":["00100","00100","01000","00000","00000","00000","00000"],
 '>':["01000","00100","00010","00001","00010","00100","01000"],
 '<':["00010","00100","01000","10000","01000","00100","00010"],
 '*':["00000","00100","10101","01110","10101","00100","00000"],
 '=':["00000","00000","11111","00000","11111","00000","00000"],
 '+':["00000","00100","00100","11111","00100","00100","00000"],
 '(':["00010","00100","01000","01000","01000","00100","00010"],
 ')':["01000","00100","00010","00010","00010","00100","01000"],
 '#':["01010","11111","01010","01010","11111","01010","00000"],
 ',':["00000","00000","00000","00000","01100","00100","01000"],
 '@':["00000","01110","11111","11111","11111","01110","00000"],   # rond plein : set gagné
 '%':["11001","11010","00010","00100","01000","01011","10011"],
}

def font_tile(ch):
    """Tuile 8x8 : texte couleur 3 avec ombre portée couleur 1."""
    t = [[0]*8 for _ in range(8)]
    g = FONT.get(ch)
    if not g:
        return t
    for y, row in enumerate(g):
        for x, c in enumerate(row):
            if c == '1':
                # ombre en bas à droite
                if x+2 < 8 and y+1 < 8 and t[y+1][x+2] == 0:
                    t[y+1][x+2] = 1
    for y, row in enumerate(g):
        for x, c in enumerate(row):
            if c == '1':
                t[y][x+1] = 3
    return t

def blank():
    return [[0]*8 for _ in range(8)]

# --------------------------------------------------------------------------
# Encodage CHR
# --------------------------------------------------------------------------
def encode_tile(t):
    lo = bytearray(8); hi = bytearray(8)
    for y in range(8):
        for x in range(8):
            v = t[y][x]
            if v & 1: lo[y] |= 0x80 >> x
            if v & 2: hi[y] |= 0x80 >> x
    return bytes(lo) + bytes(hi)

def tkey(t):
    return tuple(tuple(r) for r in t)

# --------------------------------------------------------------------------
# Table 0 : arrière-plans
# --------------------------------------------------------------------------
bg_tiles = [None]*256
for code in range(0x20, 0x60):
    bg_tiles[code] = font_tile(chr(code))
bg_tiles[0] = blank()
# Tuile 0x01 : bloc plein couleur 1 (cadres de menu), 0x02 : couleur 2, 0x03 : couleur 3
for i, c in ((1, 1), (2, 2), (3, 3)):
    bg_tiles[i] = [[c]*8 for _ in range(8)]
# 0x04 : flèche curseur (couleur 3)
cur = blank()
for y in range(7):
    w = 4 - abs(3 - y)
    for x in range(w + 1):
        cur[y][x+1] = 3
bg_tiles[4] = cur

free_bg = [i for i in range(256) if bg_tiles[i] is None]
bg_index = {}
for i, t in enumerate(bg_tiles):
    if t is not None:
        bg_index.setdefault(tkey(t), i)

def bg_alloc(t):
    k = tkey(t)
    if k in bg_index:
        return bg_index[k]
    if not free_bg:
        sys.exit("ERREUR : plus de tuiles libres dans la table d'arrière-plan")
    i = free_bg.pop(0)
    bg_tiles[i] = t
    bg_index[k] = i
    return i

def bitmap_to_nametable(bmp, attr_fn):
    """bmp[240][256] -> 960 octets de tuiles + 64 octets d'attributs."""
    nt = bytearray(960)
    for ty in range(30):
        for tx in range(32):
            t = [bmp[ty*8+y][tx*8:tx*8+8] for y in range(8)]
            nt[ty*32+tx] = bg_alloc([list(r) for r in t])
    at = bytearray(64)
    for ay in range(8):
        for ax in range(8):
            v = 0
            for q, (dx, dy) in enumerate(((0,0),(1,0),(0,1),(1,1))):
                bx, by = ax*2+dx, ay*2+dy
                v |= (attr_fn(bx, by) & 3) << (q*2)
            at[ay*8+ax] = v
    return bytes(nt) + bytes(at)

# ---------------------------- Court --------------------------------------
def draw_net(b, rect, shake=False):
    """Filet : bande blanche de 2 px (couleur 2), mailles aérées (couleur 3,
    on voit le court à travers), câble du bas, sangle centrale, ombre portée
    tramée sur le court, poteaux avec reflet et tête claire. shake : mailles
    décalées et bande affaissée d'un pixel (animation quand la balle touche)."""
    x0n, x1n = DBL_L - 6, DBL_R + 6
    vs = 2 if shake else 0
    hs = 1 if shake else 0
    for y in range(NET_Y - 5, NET_Y):
        for x in range(x0n, x1n + 1):
            strand = (x % 4 == vs) or ((y - NET_Y + hs) % 2 == 0)
            b[y][x] = 3 if strand else 1
    rect(x0n, NET_Y, x1n, NET_Y, 3)                         # câble du bas
    rect(x0n, NET_Y - 7 + hs, x1n, NET_Y - 6 + hs, 2)       # bande blanche
    if hs:
        rect(x0n, NET_Y - 7, x1n, NET_Y - 7, 1)              # (affaissée)
    rect(CENTER_X - 1, NET_Y - 5, CENTER_X, NET_Y, 2)        # sangle centrale
    for y in (NET_Y + 1, NET_Y + 2):                         # ombre portée
        for x in range(x0n + 2, x1n + 3):
            if (x + y) % 2 == 0 and (y == NET_Y + 1 or x % 4 < 2):
                b[y][x] = 3
    for px in (DBL_L - 6, DBL_R + 4):                        # poteaux (aux bouts du filet)
        rect(px, NET_Y - 11, px + 2, NET_Y + 1, 3)
        rect(px, NET_Y - 10, px, NET_Y, 2)                   # reflet
        rect(px, NET_Y - 11, px + 2, NET_Y - 11, 2)          # tête

# Chaise d'arbitre classique, dessinée dans le décor de chaque court, au
# niveau du filet : 16 x 48 px, x 32-47, y 112-159 (3 blocs d'attributs en
# palette 3, inutilisée ailleurs sur le court : 1 noir, 2 jaune, 3 blanc ;
# mêmes couleurs de jour, de nuit et sous la pluie).
CHAIR = [[0]*16 for _ in range(32)]
def cset(x0, y0, x1, y1, c):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            CHAIR[y][x] = c
# petite chaise de profil (environ 60 % de l'ancienne), l'arbitre regarde à
# droite (le court) ; ses jambes descendent du siège au repose-pieds
cset(1, 6, 2, 30, 1); cset(2, 7, 2, 30, 3); cset(1, 6, 3, 6, 1)   # dossier et pied arrière
cset(1, 17, 9, 17, 3); cset(1, 18, 9, 18, 1); cset(5, 17, 6, 17, 2)   # siège, plaque
cset(8, 19, 9, 23, 1); cset(8, 24, 11, 24, 1)                       # jambes, chaussures
cset(7, 25, 12, 25, 3); cset(7, 26, 12, 26, 1)                      # repose-pieds
cset(10, 27, 11, 30, 1); cset(11, 27, 11, 30, 3)                    # pied avant
cset(3, 23, 7, 23, 3); cset(3, 24, 7, 24, 1)                        # traverse
cset(0, 31, 3, 31, 1); cset(9, 31, 12, 31, 1)                       # patins
CHAIR_X, CHAIR_Y = 32, 112      # au bord du court, contre le poteau ; centrée sur le filet (bande : y 125-132)

CLUB_ROWS = (6, 7, 12, 13, 20, 21, 26, 27)     # bancs de spectateurs du club (rangées)
CASTLE_ROWS = (12, 13, 20, 21)                 # balcons du château (rangées)

def draw_court(env=0):
    W, H = 256, 240
    b = [[0]*W for _ in range(H)]
    def rect(x0, y0, x1, y1, c):
        for y in range(max(0,y0), min(H,y1+1)):
            for x in range(max(0,x0), min(W,x1+1)):
                b[y][x] = c
    # surface de jeu (couleur 1) avec une marge de dégagement
    # (6 px sur les côtés : le bloc d'attributs x 32-47, à gauche, reste
    # libre pour la chaise d'arbitre, au bord du court)
    rect(DBL_L-6, TOP_BASE-14, DBL_R+6, BOT_BASE+14, 1)
    # lignes (couleur 2)
    rect(DBL_L, TOP_BASE-1, DBL_R, TOP_BASE, 2)        # ligne de fond haute
    rect(DBL_L, BOT_BASE, DBL_R, BOT_BASE+1, 2)        # ligne de fond basse
    for x in (DBL_L, SGL_L, SGL_R, DBL_R):
        rect(x, TOP_BASE, x, BOT_BASE, 2)               # lignes de côté
    rect(SGL_L, TOP_SERV, SGL_R, TOP_SERV, 2)          # lignes de service
    rect(SGL_L, BOT_SERV, SGL_R, BOT_SERV, 2)
    rect(CENTER_X, TOP_SERV, CENTER_X, BOT_SERV, 2)    # ligne médiane de service
    rect(CENTER_X, TOP_BASE+1, CENTER_X, TOP_BASE+3, 2)  # marques centrales
    rect(CENTER_X, BOT_BASE-3, CENTER_X, BOT_BASE-1, 2)
    draw_net(b, rect)
    # bords du court (palette 2) selon l'environnement
    for side in (0, 1):
        x0 = 0 if side == 0 else 224
        for y in range(32, 240):
            for x in range(x0, x0+32):
                b[y][x] = 0
        if env == 0:
            # STADE : tribunes pleines de spectateurs
            for row in range(4, 30):
                for col in range(4):
                    pat = random.choice(CROWD)
                    for yy in range(8):
                        for xx in range(8):
                            b[row*8+yy][x0+col*8+xx] = pat[yy][xx]
        elif env == 1:
            # CLUB : haies, arbres et quelques spectateurs sur des bancs
            for y in range(32, 240):
                for x in range(x0, x0+32):
                    b[y][x] = 1 if (((x % 8)*7 + (y % 8)*3) % 11) else 2
            for k in range(6):
                cx = x0 + (8 if (k % 2) else 24)
                cy = 56 + k*32
                for y in range(cy-9, cy+10):
                    for x in range(cx-9, cx+10):
                        if 0 <= x < 256 and 32 <= y < 240 and (x-cx)**2 + (y-cy)**2 <= 72:
                            b[y][x] = 2 if ((x % 8) + (y % 8)) % 3 else 3
                for y in range(cy+7, cy+12):
                    if y < 240:
                        b[y][cx] = 3
            # bancs de spectateurs : 4 bandes de 2 rangées (palette 3 : visibles
            # sur la verdure)
            for row in CLUB_ROWS:
                for col in range(4):
                    pat = CROWDB[(row * 3 + col + side) % 6]
                    for yy in range(8):
                        for xx in range(8):
                            b[row*8+yy][x0+col*8+xx] = pat[yy][xx]
        elif env == 4:
            # COURT MARGAUX : fond rose pâle (1) parsemé de coeurs (2),
            # de fleurs et d'étoiles (3)
            for y in range(32, 240):
                for x in range(x0, x0+32):
                    b[y][x] = 1
            cols = (0, 1, 2) if side == 0 else (1, 2, 3)
            for row in range(4, 30):
                for col in cols:
                    k = (row + col * 3 + side * 3) % 6     # coeurs, fleurs et étoiles en alternance
                    pat = GIRLY[k // 2] if k % 2 == 0 else None
                    if pat:
                        for yy in range(8):
                            for xx in range(8):
                                b[row*8+yy][x0+col*8+xx] = pat[yy][xx]
        elif env == 5:
            # CHÂTEAU : murs de pierre (une seule tuile de briques répétée),
            # bannières royales, torches, deux balcons de spectateurs.
            # Couleurs (palette 2) : 1 pierre, 2 joints, 3 bannières et flammes
            for y in range(32, 240):
                for x in range(x0, x0+32):
                    ry = y % 8
                    off = 0 if ry < 4 else 4
                    b[y][x] = 2 if (ry % 4 == 3 or (x + off) % 8 == 7) else 1
            bcol = x0 + 8 if side == 0 else x0 + 16          # colonne des bannières
            for trow in (6, 16, 24):                          # bannières (8 x 16)
                y0 = trow * 8
                for yy in range(16):
                    for xx in range(8):
                        y, x = y0 + yy, bcol + xx
                        if yy == 0:
                            b[y][x] = 2                       # hampe
                        elif yy < 11 and 1 <= xx <= 6:
                            b[y][x] = 3 if not (yy in (4, 5) and 3 <= xx <= 4) else 1   # emblème
                        elif yy >= 11 and abs(xx * 2 - 7) <= (15 - yy) * 2 - 2:
                            b[y][x] = 3                       # pointe
            tcol = x0 + 16 if side == 0 else x0 + 8           # torches
            for trow in (10, 18, 28):
                y0 = trow * 8
                for (xx, yy, c) in ((3, 2, 3), (4, 1, 3), (3, 3, 3), (4, 3, 3), (4, 2, 3), (3, 4, 2), (4, 4, 2),
                                    (3, 5, 2), (4, 5, 2), (2, 6, 2), (5, 6, 2)):
                    b[y0 + yy][tcol + xx] = c
            # balcons : spectateurs (palette 3, comme au club)
            for row in CASTLE_ROWS:
                for col in range(4):
                    pat = CROWDB[(row * 3 + col + side) % 6]
                    for yy in range(8):
                        for xx in range(8):
                            b[row*8+yy][x0+col*8+xx] = pat[yy][xx]
        elif env == 3:
            # CLUB NUDISTE : pas de gradins, spectateurs (au naturel) sur des
            # serviettes dans l'herbe. Couleurs : 1 herbe, 2 peau, 3 serviette
            for y in range(32, 240):
                for x in range(x0, x0+32):
                    b[y][x] = 1
            # quelques spectateurs debout, centrés dans la bande latérale
            # (hors de la zone rognée par les téléviseurs) ; à gauche, la place
            # de la chaise d'arbitre reste libre
            for k in range(5):
                if side == 0 and k == 2:
                    continue
                ty0 = 40 + k * 40
                fan = NUDE_FANS[(k + side) % 3]
                for q in range(6):
                    ty = ty0 + (q // 2) * 8
                    tx = x0 + 8 + (q % 2) * 8
                    for yy in range(8):
                        for xx in range(8):
                            b[ty+yy][tx+xx] = fan[q][yy][xx]
        else:
            # SALLE : gradins côté extérieur (16 px, palette 3), murs et
            # panneaux publicitaires côté court
            xi = x0 + 16 if side == 0 else x0        # bande des panneaux
            xo = x0 if side == 0 else x0 + 16        # bande des gradins
            for y in range(32, 240):
                for x in range(xi, xi+16):
                    b[y][x] = 3 if (y // 8) % 6 == 0 else 1
            for k in range(5):
                y0 = 40 + k*40
                for y in range(y0, y0+20):
                    for x in range(xi+2, xi+14):
                        if 32 <= y < 240:
                            b[y][x] = 2
                for y in range(y0+6, y0+14, 2):
                    for x in range(xi+4, xi+12, 3):
                        if 32 <= y < 240:
                            b[y][x] = 3
            for row in range(4, 30):
                for col in range(2):
                    pat = CROWDB[(row * 5 + col * 2 + side) % 6]
                    for yy in range(8):
                        for xx in range(8):
                            b[row*8+yy][xo+col*8+xx] = pat[yy][xx]
        # rambarde (pas de barrière au club nudiste : simple haie basse)
        bx = 31 if side == 0 else 224
        for y in range(32, 240):
            b[y][bx] = 3 if env != 3 else (3 if y % 4 < 2 else 1)
    # chaise d'arbitre (palette 3, voir court_attr)
    for y in range(32):
        for x in range(16):
            b[CHAIR_Y + y][CHAIR_X + x] = CHAIR[y][x]
    # bandeau supérieur réservé au score (couleur 0)
    for y in range(0, 32):
        for x in range(W):
            b[y][x] = 0
    return b

# Spectateurs (version 2.42) : un par tuile, assis en rangées ; 4 modèles
# (cheveux courts, cheveux longs, casquette, cheveux clairs), décalés d'une
# place à l'autre pour casser la grille. Le jeu échange « regard vers le bas »
# et « regard vers le haut » quand la balle change de moitié ; bras levés :
# ola. Tailles : 'big' (vue à plat, premier plan en perspective), 'med',
# 'small' et 'tiny' (4 petites têtes par tuile, foule lointaine).
# Jeu A (stade, palette 2 : 1 peau, 2 gradins, 3 cheveux noirs) : marche des
# gradins (couleur 0) sur la ligne du haut ; maillots noirs, peau claire ou
# couleur des abords. Jeu B (club, salle, château, parc ; palette 3 : 1 noir,
# 2 peau, 3 blanc) : fond 0 (abords), cheveux noirs, maillots blancs ou noirs.
DESIGNS = 4
def spectator(d, view, b=False, size='big'):
    bg, skin, hair = (0, 2, 1) if b else (2, 1, 3)
    eye = hair
    shirts = (3, 1, 3, 3) if b else (3, 1, 3, 0)
    shirt = shirts[d]
    t = [[bg] * 8 for _ in range(8)]
    def px(x, y, v):
        if 0 <= x < 8 and 0 <= y < 8: t[y][x] = v
    if not b and size != 'tiny':
        for x in range(8): t[0][x] = 0                  # marche du gradin
    if size == 'tiny':
        # deux rangées de deux petites têtes (2 x 2) et bustes (3 x 2)
        for k, (ox, oy) in enumerate(((0, 0), (4, 0), (2, 4), (6, 4))):
            dd = (d + k) % DESIGNS
            sh = 3 if b else (1 if dd != 2 else 3)        # foule lointaine : maillots clairs, peu de noir
            hc = (3 if b else 2) if dd == 3 else hair       # cheveux clairs (blancs au jeu B)
            if view == 'up':
                px(ox + 1, oy, hc); px(ox + 2, oy, hc); px(ox + 1, oy + 1, hc); px(ox + 2, oy + 1, hc)
            else:
                px(ox + 1, oy, hc); px(ox + 2, oy, hc); px(ox + 1, oy + 1, skin); px(ox + 2, oy + 1, skin)
            for x in range(ox, ox + 4 if ox + 4 <= 8 else 8):
                if x != ox + 3: px(x, oy + 2, sh); px(x, oy + 3, sh)
            if view == 'cheer':
                px(ox, oy, skin); px(ox + 3, oy, skin)
        return t
    ox = (2, 1, 3, 2)[d]
    hw, top, hh, bw = {'big': (4, 1, 4, 6), 'med': (4, 2, 3, 5), 'small': (3, 3, 3, 5)}[size]
    hc = skin if (not b and d == 3) else hair             # modèle 3 : cheveux clairs (couleur de peau)
    if b and d == 3: hc = 3                               # jeu B : cheveux blancs
    # tête
    for k in range(hh):
        for xx in range(hw):
            x, y = ox + xx, top + k
            if view == 'up':
                v = hc if k < hh - 1 else skin            # nuque
            elif k == 0:
                v = hc
            elif k == 1 and xx in (1, hw - 2) and hw == 4:
                v = eye                                    # yeux
            elif k == 1 and hw == 3 and xx == 1:
                v = eye
            else:
                v = skin
            if size == 'big' and k == hh - 1 and xx in (0, hw - 1) and view != 'up':
                v = bg                                     # menton arrondi
            px(x, y, v)
    if d == 1 and view != 'up':                            # cheveux longs
        for k in range(1, hh):
            px(ox - 1, top + k, hc); px(ox + hw, top + k, hc)
    if d == 2:                                             # casquette (couleur du maillot)
        cap = 3 if not b else 1
        for xx in range(-1, hw + (1 if view != 'up' else 0)):
            px(ox + xx, top, cap)
    # épaules et buste
    by0 = top + hh
    for y in range(by0, 8):
        for xx in range(-(bw - hw) // 2 - (1 if bw - hw == 2 else 0), hw + (bw - hw + 1) // 2 - (1 if bw - hw == 2 else 0)):
            px(ox + xx, y, shirt)
    if view == 'cheer':                                    # bras levés
        for y in range(top - 1, top + hh):
            px(ox - 1, y, skin); px(ox + hw, y, skin)
    return t

ORDER = (0, 1, 2, 3, 1, 2)       # 6 places, 4 dessins (ola_row : tuiles 5-8 et 13-16)
CROWD = [spectator(d, 'down') for d in ORDER]
CROWD_UP = [spectator(d, 'up') for d in ORDER]
CHEER = [spectator(d, 'cheer') for d in ORDER]
CROWDB = [spectator(d, 'down', True) for d in ORDER]
CROWDB_UP = [spectator(d, 'up', True) for d in ORDER]
CHEERB = [spectator(d, 'cheer', True) for d in ORDER]

crowd_idx = [bg_alloc(t) for t in CROWD]
cheer_idx = [bg_alloc(t) for t in CHEER]     # ola : tuiles « bras levés » propres
crowdb_idx = [bg_alloc(t) for t in CROWDB]   # club et salle
cheerb_idx = [bg_alloc(t) for t in CHEERB]

def nude_fan(kind, cheer=False):
    """Spectateur du club nudiste, debout et en pied (16x24) : silhouette
    couleur peau (2) sans aucun détail anatomique ; cheveux, chapeau de soleil
    ou serviette en couleur 3 ; herbe (1). Bras levés quand il applaudit."""
    t = [[1]*16 for _ in range(24)]
    def f(x0, y0, x1, y1, v):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                if 0 <= x < 16 and 0 <= y < 24:
                    t[y][x] = v
    f(6, 1, 9, 5, 2)                      # tête
    f(5, 2, 10, 4, 2)
    if kind == 0:                         # cheveux courts
        f(6, 0, 9, 1, 3); f(5, 1, 5, 2, 3); f(10, 1, 10, 2, 3)
    elif kind == 1:                       # cheveux longs
        f(5, 0, 10, 1, 3); f(4, 1, 4, 6, 3); f(11, 1, 11, 6, 3)
    else:                                 # chapeau de soleil
        f(3, 1, 12, 1, 3); f(6, 0, 9, 0, 3)
    f(7, 6, 8, 6, 2)                      # cou
    f(5, 7, 10, 15, 2)                    # buste
    f(4, 8, 11, 9, 2)                     # épaules
    if kind == 0:
        for k in range(6):                # serviette en écharpe
            f(5 + k, 7 + k, 6 + k, 7 + k, 3)
    if cheer:
        f(3, 1, 3, 8, 2); f(12, 1, 12, 8, 2)      # bras levés
        f(2, 0, 3, 1, 2); f(12, 0, 13, 1, 2)       # mains
    else:
        f(3, 9, 3, 15, 2); f(12, 9, 12, 15, 2)     # bras le long du corps
        if kind == 2:
            f(12, 14, 14, 19, 3)                   # serviette à la main
    f(5, 16, 6, 22, 2); f(9, 16, 10, 22, 2)        # jambes
    f(4, 23, 6, 23, 2); f(9, 23, 11, 23, 2)        # pieds nus
    return t

def split6(t):
    return [[r[tx*8:tx*8+8] for r in t[ty*8:ty*8+8]] for ty in range(3) for tx in range(2)]

def girly_tile(kind, sparkle=False):
    """Décor du court Margaux : coeur (2), fleur (3, coeur 2), étoile (3) ;
    version « scintillante » pour l'animation après les beaux points."""
    t = [[1]*8 for _ in range(8)]
    if kind == 0:
        rows = ["........", ".##.##..", "#######.", "#######.", ".#####..", "..###...", "...#....", "........"]
        if sparkle:
            rows = [".##.##..", "#######.", "########", "########", ".######.", "..####..", "...##...", "........"]
        for y, r in enumerate(rows):
            for x, ch in enumerate(r):
                if ch == '#': t[y][x] = 2
    elif kind == 1:
        pts = [(3, 1), (1, 3), (5, 3), (3, 5)] if not sparkle else [(1, 1), (5, 1), (1, 5), (5, 5)]
        for (x, y) in pts:
            for dy in (0, 1):
                for dx in (0, 1):
                    t[y + dy][x + dx] = 3
        t[3][3] = t[3][4] = t[4][3] = t[4][4] = 2
    else:
        if sparkle:
            for k in range(8):
                t[k][3] = 3; t[3][k] = 3
            t[1][1] = t[5][5] = t[1][5] = t[5][1] = 3
        else:
            for (x, y) in ((3, 1), (2, 2), (3, 2), (4, 2), (1, 3), (2, 3), (3, 3), (4, 3), (5, 3), (2, 4), (3, 4), (4, 4), (2, 5), (4, 5)):
                t[y][x] = 3
    return t

GIRLY = [girly_tile(k) for k in range(3)]
GIRLY_SPARKLE = [girly_tile(k, True) for k in range(3)]
crowd_idx4 = [bg_alloc(t) for t in GIRLY]

NUDE_FANS = [split6(nude_fan(k)) for k in range(3)]
NUDE_FANS_CHEER = [split6(nude_fan(k, True)) for k in range(3)]
NUDE_CROWD = [tile for fan in NUDE_FANS for tile in fan]          # 18 tuiles
NUDE_CHEER = [tile for fan in NUDE_FANS_CHEER for tile in fan]
crowd_idx3 = [bg_alloc(t) for t in NUDE_CROWD]

def court_attr(bx, by, env=0):
    if by < 2:
        return 1                      # bandeau score
    if bx == CHAIR_X // 16 and CHAIR_Y // 16 <= by < CHAIR_Y // 16 + 2:
        return 3                      # chaise d'arbitre
    # spectateurs du club et de la salle : palette 3 (noir, jaune, blanc),
    # bien visibles sur la verdure et les murs
    if env == 1 and bx in (0, 1, 14, 15) and by * 2 in CLUB_ROWS:
        return 3
    if env == 2 and bx in (0, 15):
        return 3
    if env == 5 and bx in (0, 1, 14, 15) and by * 2 in CASTLE_ROWS:
        return 3
    if bx < 2 or bx >= 14:
        return 2                      # tribunes
    return 0

court_nts = [bitmap_to_nametable(draw_court(e), lambda bx, by, e=e: court_attr(bx, by, e)) for e in range(6)]
# panneau du club nudiste, ligne 3 du bandeau (police ASCII = n° de tuile)
# court Margaux : petit panneau (32 x 16) dans le bandeau du haut, juste à
# droite du score de gauche (colonnes 5 à 8, rangées 2 et 3). Palette des
# abords du court (2) : 1 rose clair, 2 magenta, 3 blanc ; 0 = fond du bandeau.
# Volume : liseré clair sur le dessus, face magenta, rebord clair, deux pieds.
# Lettres « MARGAUX » en petite police 3 x 5.
MINI = {
 'M': ["101", "111", "111", "101", "101"], 'A': ["010", "101", "111", "101", "101"],
 'R': ["110", "101", "110", "101", "101"], 'G': ["011", "100", "101", "101", "011"],
 'U': ["101", "101", "101", "101", "111"], 'X': ["101", "101", "010", "101", "101"],
}
SIGN_X, SIGN_Y = 5, 2          # en tuiles
def margaux_sign():
    c = [[0] * 32 for _ in range(16)]
    def put(x, y, v):
        if 0 <= x < 32 and 0 <= y < 16:
            c[y][x] = v
    for x in range(2, 30): put(x, 1, 1)                      # dessus (liseré clair)
    for y in range(2, 11):
        for x in range(1, 31): put(x, y, 2)                  # face magenta
    for x in range(1, 31): put(x, 11, 1)                     # rebord
    for x, ch in enumerate("MARGAUX"):
        for y, row in enumerate(MINI[ch]):
            for k, v in enumerate(row):
                if v == '1': put(3 + x * 4 + k, 4 + y, 3)    # lettres blanches
    for y in range(12, 15):                                  # pieds
        put(5, y, 1); put(6, y, 1); put(25, y, 1); put(26, y, 1)
    return [[c[ty * 8 + y][tx * 8:tx * 8 + 8] for y in range(8)] for ty in range(2) for tx in range(4)]
SIGN_TILES = margaux_sign()
def set_attr(nt, col, row, pal):
    i = 960 + (row // 4) * 8 + (col // 4)
    q = ((row // 2) & 1) * 2 + ((col // 2) & 1)
    nt[i] = (nt[i] & ~(3 << (q * 2))) | (pal << (q * 2))
# indices : les 8 premières tuiles du public du club nudiste (inutilisées sur
# le court Margaux) ; load_court y copie le panneau, et les rétablit ailleurs
SIGN_IDX = [i for i in dict.fromkeys(crowd_idx3) if i not in set(court_nts[4][:960])][:8]
assert len(SIGN_IDX) == 8
_nt4 = bytearray(court_nts[4])
for k in range(8):
    _nt4[(SIGN_Y + k // 4) * 32 + SIGN_X + k % 4] = SIGN_IDX[k]
for col in (4, 6, 8):                     # blocs 16 x 16 : palette des abords
    set_attr(_nt4, col, SIGN_Y, 2)
court_nts[4] = bytes(_nt4)

# club nudiste : même petit panneau à côté du score, « BRONZES » (club « Les
# Bronzés »), jaune soleil. Palette 3 du décor : 1 noir, 2 jaune, 3 blanc ;
# les ombres du score voisin y restent noires. Tuiles : celles du public du
# stade (6) et du court Margaux (2), inutilisées au club nudiste.
MINI.update({
 'B': ["110", "101", "110", "101", "110"], 'O': ["010", "101", "101", "101", "010"],
 'N': ["110", "101", "101", "101", "101"], 'Z': ["111", "001", "010", "100", "111"],
 'E': ["111", "100", "110", "100", "111"], 'S': ["011", "100", "010", "001", "110"],
})
def nude_sign():
    c = [[0] * 32 for _ in range(16)]
    def put(x, y, v):
        if 0 <= x < 32 and 0 <= y < 16:
            c[y][x] = v
    for x in range(2, 30): put(x, 1, 3)                      # dessus (liseré blanc)
    for y in range(2, 11):
        for x in range(1, 31): put(x, y, 2)                  # face jaune soleil
    for x in range(1, 31): put(x, 11, 1)                     # rebord sombre
    for x, ch in enumerate("BRONZES"):
        for y, row in enumerate(MINI[ch]):
            for k, v in enumerate(row):
                if v == '1': put(3 + x * 4 + k, 4 + y, 1)    # lettres noires
    for y in range(12, 15):                                  # pieds
        put(5, y, 1); put(6, y, 1); put(25, y, 1); put(26, y, 1)
    return [[c[ty * 8 + y][tx * 8:tx * 8 + 8] for y in range(8)] for ty in range(2) for tx in range(4)]
SIGN2_TILES = nude_sign()
def pick_free(env, pool, n=8):
    """n tuiles distinctes du réservoir, absentes du décor du court env."""
    used = set(court_nts[env][:960])
    out = [i for i in dict.fromkeys(pool) if i not in used]
    assert len(out) >= n, "pas assez de tuiles libres pour le panneau"
    return out[:n]
# réservoir : tuiles de décor des autres courts (public, arbres, murs...),
# hors police, hors décor du club nudiste et hors panneau Margaux
_pool = [i for e in (0, 1, 2) for i in court_nts[e][:960] if i >= 0x60 and i not in SIGN_IDX] + list(crowd_idx) + list(crowd_idx4)
SIGN2_IDX = pick_free(3, _pool)
_nt3 = bytearray(court_nts[3])
for k in range(8):
    _nt3[(SIGN_Y + k // 4) * 32 + SIGN_X + k % 4] = SIGN2_IDX[k]
for col in (4, 6, 8):
    set_attr(_nt3, col, SIGN_Y, 3)
court_nts[3] = bytes(_nt3)

# filet secoué : tuiles de mailles « au repos » et « secouées » (rangées 15
# et 16, colonne 12, sans ligne du court) ; l'animation remplace leur dessin
# en CHR-RAM pendant quelques trames quand la balle touche le filet
def net_tiles(shake):
    bm = [[1] * 256 for _ in range(240)]
    def rect(x0, y0, x1, y1, cc):
        for y in range(max(0, y0), min(240, y1 + 1)):
            for x in range(max(0, x0), min(256, x1 + 1)):
                bm[y][x] = cc
    draw_net(bm, rect, shake)
    return [[bm[r * 8 + y][96:104] for y in range(8)] for r in (15, 16)]
NET_IDX = [court_nts[0][15 * 32 + 12], court_nts[0][16 * 32 + 12]]
assert tkey(bg_tiles[NET_IDX[0]]) == tkey(net_tiles(False)[0]) and tkey(bg_tiles[NET_IDX[1]]) == tkey(net_tiles(False)[1])
court_nt = court_nts[0]

# ---------------------------- Titre --------------------------------------
LOGO = "ROYAL MARTINOT"   # ligne 1 : ROYAL, ligne 2 : MARTINOT
def big_letter_tiles(ch):
    """Lettre géante 16x16 (police x2) avec contour."""
    g = FONT[ch]
    px = [[0]*16 for _ in range(16)]
    for y, row in enumerate(g):
        for x, c in enumerate(row):
            if c == '1':
                for dy in range(2):
                    for dx in range(2):
                        px[1+y*2+dy][3+x*2+dx] = 3
    out = [[v for v in r] for r in px]
    for y in range(16):
        for x in range(16):
            if px[y][x] == 0:
                for dy in (-1,0,1):
                    for dx in (-1,0,1):
                        yy, xx = y+dy, x+dx
                        if 0 <= yy < 16 and 0 <= xx < 16 and px[yy][xx] == 3:
                            out[y][x] = 1
    # dégradé : moitié basse en couleur 2
    for y in range(9, 16):
        for x in range(16):
            if out[y][x] == 3:
                out[y][x] = 2
    tiles = []
    for ty in range(2):
        for tx in range(2):
            tiles.append([out[ty*8+y][tx*8:tx*8+8] for y in range(8)])
    return tiles

logo_tiles = {}
for ch in sorted(set(LOGO.replace(' ', ''))):   # ordre fixe (compilation reproductible)
    logo_tiles[ch] = [bg_alloc(t) for t in big_letter_tiles(ch)]

# Balle géante décorative 4x4 tuiles pour l'écran titre
def big_ball():
    px = [[0]*32 for _ in range(32)]
    for y in range(32):
        for x in range(32):
            d = (x-15.5)**2 + (y-15.5)**2
            if d < 15.5**2:
                px[y][x] = 2
                if d > 13.5**2:
                    px[y][x] = 1
    # coutures blanches
    import math
    for a in range(0, 360, 2):
        r = math.radians(a)
        for (cx, cy) in ((-14, 16), (46, 16)):
            x = int(cx + 20*math.cos(r)); y = int(cy + 20*math.sin(r))
            if 0 <= x < 32 and 0 <= y < 32 and px[y][x] == 2:
                px[y][x] = 3
    # reflet
    for y in range(7, 11):
        for x in range(9, 13):
            if px[y][x] == 2:
                px[y][x] = 3
    tiles = []
    for ty in range(4):
        for tx in range(4):
            tiles.append(bg_alloc([px[ty*8+y][tx*8:tx*8+8] for y in range(8)]))
    return tiles
ball_tiles = big_ball()

# ---------------------------- Sprites (table 1) --------------------------
spr_tiles = [None]*256
spr_tiles[0] = blank()
def spr_tile(i, t):
    spr_tiles[i] = t

# balle (couleur 1) et ombre (couleur 2)
ball = blank()
for (x, y) in ((2,1),(3,1),(1,2),(2,2),(3,2),(4,2),(1,3),(2,3),(3,3),(4,3),(2,4),(3,4)):
    ball[y][x] = 1
ball[2][2] = 3
spr_tile(1, ball)
shadow = blank()
for (x, y) in ((1,3),(2,3),(3,3),(4,3),(2,4),(3,4),(1,4),(4,4)):
    shadow[y][x] = 2
spr_tile(2, shadow)
# petite balle (loin / très haute) inutilisée pour l'instant : réserve
# curseur de visée / indicateur de serveur
star = blank()
for (x, y) in ((3,0),(3,1),(2,2),(3,2),(4,2),(0,3),(1,3),(2,3),(3,3),(4,3),(5,3),(6,3),(2,4),(3,4),(4,4),(1,5),(2,5),(4,5),(5,5),(1,6),(5,6)):
    star[y][x] = 3
spr_tile(3, star)
# gouttes de pluie (réservé pour la météo)
rain = blank()
for y in range(1, 6):
    rain[y][4 - y//3] = 2              # couleur 2 de la palette des effets : bleu
spr_tile(4, rain)
# ombre d'un joueur (couleur 2, noir) : ellipse 8 x 3, lignes 0 à 2
pshadow = blank()
for (y, x0, x1) in ((0, 2, 5), (1, 0, 7), (2, 2, 5)):
    for x in range(x0, x1 + 1):
        pshadow[y][x] = 2
spr_tile(5, pshadow)
# poussière au rebond (terre, plage) : petit nuage puis nuage plus large (couleur 1)
dust1 = blank()
for (x, y) in ((3, 5), (4, 5), (2, 6), (5, 6), (3, 4)):
    dust1[y][x] = 1
spr_tile(6, dust1)
dust2 = blank()
for (x, y) in ((1, 4), (2, 3), (5, 3), (6, 4), (0, 6), (7, 6), (3, 2), (4, 6)):
    dust2[y][x] = 1
spr_tile(7, dust2)
# éclaboussures sous la pluie (couleur 1 de la palette des effets)
spl1 = blank()
for (x, y) in ((3, 4), (4, 4), (2, 5), (5, 5), (3, 6), (4, 6)):
    spl1[y][x] = 2
spr_tile(8, spl1)
spl2 = blank()
for (x, y) in ((1, 2), (6, 2), (0, 4), (7, 4), (2, 6), (5, 6), (3, 3), (4, 3)):
    spl2[y][x] = 2
spr_tile(9, spl2)
# repère d'atterrissage (mode lent) : petite croix (couleur 3)
cross = blank()
for k in range(1, 6):
    cross[k][k + 1] = 1
    cross[k][7 - k] = 1
spr_tile(10, cross)
# Arbitre de chaise, de profil, tourné vers le court (sprites, palette 2 :
# 1 peau, 2 noir, 3 blanc). 11 tête, 12 buste au repos, 13 buste bras tendu
# (épaule) ; 3 tuiles chargées à la place de la coupe pendant un match : main
# du bras tendu, bras levé (au-dessus de l'épaule), buste bras levé.
def tile_from(rows):
    t = blank()
    for y, r in enumerate(rows):
        for x, c in enumerate(r):
            t[y][x] = int(c)
    return t
# petit arbitre (environ 60 % de l'ancien) : tête et épaules, puis buste et
# cuisses ; la tête regarde vers le bas (11) ou vers le haut (12) du court
ump_head_dn = tile_from(["00222000", "02222100", "02211100", "02211110", "00212110", "00011000", "00333300", "03333330"])
ump_head_up = tile_from(["00222000", "02222100", "02212110", "02211100", "00211100", "00011000", "00333300", "03333330"])
ump_body    = tile_from(["03333330", "03313330", "03313330", "03311110", "02222222", "00000022", "00000022", "00000022"])
ump_body_out = tile_from(["03333330", "03333111", "03333111", "03333330", "02222222", "00000022", "00000022", "00000022"])
ump_hand    = tile_from(["00000000", "11100000", "11100000", "00000000", "00000000", "00000000", "00000000", "00000000"])
ump_armup   = tile_from(["00000111", "00000111", "00000110", "00000110", "00000110", "00000110", "00001110", "00011110"])   # raccord à l'épaule
spr_tile(11, ump_head_dn)
spr_tile(12, ump_head_up)
spr_tile(13, ump_body)
UMP_EXTRA = [ump_body_out, ump_hand, ump_armup, blank()]   # chargées à la place de la coupe pendant un match
# court Margaux : l'arbitre est une princesse (couronne, cheveux longs, robe
# blanche, baguette magique à étoile). Mêmes couleurs que l'arbitre (palette 2 :
# 1 jaune pâle = peau, couronne, baguette dorée et étoile ; 2 noir =
# cheveux ; 3 = robe, rose au court Margaux : pal_set_court y remplace le
# blanc de la palette 2 par du rose). 4e tuile : la baguette tenue au repos.
prin_head_dn = tile_from(["01010100", "01111100", "22222100", "22211110", "22211210", "22211100", "22333300", "23333330"])
prin_head_up = tile_from(["01010100", "01111100", "22222100", "22211210", "22211110", "22211100", "22333300", "23333330"])
prin_body    = tile_from(["03333330", "03333311", "03333311", "33333333", "33333333", "00000033", "00000033", "00000022"])
prin_body_out = tile_from(["03333330", "03333111", "03333111", "33333333", "33333333", "00000033", "00000033", "00000022"])
prin_hand    = tile_from(["00000100", "11111111", "11001110", "00001010", "00000000", "00000000", "00000000", "00000000"])
prin_armup   = tile_from(["00000010", "00000111", "00000010", "00000010", "00000010", "00000110", "00001110", "00011110"])
prin_wand    = tile_from(["00010000", "00111000", "00010000", "00100000", "00100000", "00100000", "01100000", "01100000"])
UMP_SETS = [[ump_head_dn, ump_head_up, ump_body] + UMP_EXTRA,
            [prin_head_dn, prin_head_up, prin_body, prin_body_out, prin_hand, prin_armup, prin_wand]]

# goutte de sueur (fatigue), couleur 2 de la palette des effets (bleu)
sweat = blank()
for (x, y) in ((3, 1), (3, 2), (2, 3), (3, 3), (4, 3), (2, 4), (3, 4), (4, 4), (3, 5)):
    sweat[y][x] = 2
sweat[3][2] = 3
spr_tile(14, sweat)
# éclats de glace quand un joueur dérape (couleur 3 : blanc) ; aussi la
# rayure laissée au départ de la glissade (tuile ICE_TILE, plus loin)
ice = blank()
for (x, y) in ((1, 4), (2, 5), (5, 3), (6, 5), (3, 6), (4, 6), (0, 6), (7, 6), (4, 2)):
    ice[y][x] = 3
# (placée plus loin : ICE_TILE)

# Personnages 16x24 : 0 transparent, 1 peau, 2 tenue, 3 sombre
def new_canvas():
    return [[0]*16 for _ in range(24)]

def put(c, x, y, v):
    if 0 <= x < 16 and 0 <= y < 24:
        c[y][x] = v

def fill(c, x0, y0, x1, y1, v):
    for y in range(y0, y1+1):
        for x in range(x0, x1+1):
            put(c, x, y, v)

def line(c, x0, y0, x1, y1, v):
    n = max(abs(x1-x0), abs(y1-y0), 1)
    for i in range(n+1):
        put(c, round(x0 + (x1-x0)*i/n), round(y0 + (y1-y0)*i/n), v)

def racket(c, hx, hy, cx, cy, h=7):
    """Raquette bien visible : manche (couleur 3) de la main (hx, hy) jusqu'au
    tamis, puis tamis ovale 5 x h centré en (cx, cy) : cadre sombre et cordage
    en damier (pixels transparents entre les cordes)."""
    ry = (h - 1) / 2.0
    # manche : de la main vers le bord du tamis le plus proche
    ey = cy + (ry + 0.5 if hy > cy else -(ry + 0.5)) if abs(hy - cy) > ry else cy
    line(c, hx, hy, round(cx + (hx - cx) * 0.3), round(ey), 3)
    pts = set()
    for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
        for x in range(cx - 3, cx + 4):
            if ((x - cx) / 2.6) ** 2 + ((y - cy) / (ry + 0.6)) ** 2 <= 1.0:
                pts.add((x, y))
    for (x, y) in pts:
        edge = any((x + dx, y + dy) not in pts for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
        if edge:
            put(c, x, y, 3)
        else:
            put(c, x, y, 3 if (x + y) % 2 == 0 else 0)

def draw_dive(view, female, nude=False):
    """Plongeon : corps allongé à l'horizontale au ras du sol, tête vers la
    balle (à droite ; miroir à gauche), bras et raquette tendus devant la
    tête, l'autre main au sol, jambes tendues derrière."""
    c = new_canvas()
    # raquette tendue vers la balle, tamis vu de biais, en bout de bras
    line(c, 12, 18, 13, 17, 1)                          # bras tendu
    racket(c, 13, 17, 13, 13, 5)                        # manche puis tamis au-dessus de la main
    fill(c, 8, 16, 11, 19, 1)                           # tête
    fill(c, 8, 16, 11, 16, 3); fill(c, 8, 17, 8, 19, 3)  # cheveux
    put(c, 10, 18, 3)                                   # oeil
    if nude:
        fill(c, 2, 18, 8, 21, 1)
        for k in range(5):
            put(c, 3 + k, 18 + (k % 4), 2)
    else:
        fill(c, 3, 18, 8, 21, 2)                        # buste allongé
        fill(c, 1, 19, 3, 21, 3)                        # short
    fill(c, 0, 20, 1, 21, 1)                            # jambes tendues
    put(c, 0, 22, 1 if nude else 3)                     # pied
    line(c, 6, 22, 7, 23, 1)                            # main d'appui au sol
    return c

def draw_supine(view, female, nude=False):
    """Allongé sur le dos (victoire) : corps à l'horizontale, tête à
    gauche, visage vers le ciel, bras écartés, sans raquette (lâchée).
    Même dessin pour toutes et tous (place dans le jeu de tuiles F)."""
    c = new_canvas()
    fill(c, 1, 15, 4, 18, 1)                            # tête
    fill(c, 0, 15, 0, 18, 3)                            # cheveux (sur le sol)
    put(c, 3, 16, 3); put(c, 3, 17, 3)                  # yeux
    if nude:
        fill(c, 5, 15, 10, 19, 1)
        for k in range(5):
            put(c, 5 + k, 15 + (k % 4), 2)
    else:
        fill(c, 5, 15, 10, 19, 2)                       # buste
        fill(c, 11, 15, 12, 19, 3)                      # short (même dessin pour tous : 4 tuiles)
    fill(c, 13, 15, 15, 16, 1); fill(c, 13, 18, 15, 19, 1)   # jambes
    put(c, 15, 14, 1 if nude else 3); put(c, 15, 20, 1 if nude else 3)   # pieds
    line(c, 6, 14, 4, 10, 1); line(c, 6, 20, 4, 23, 1)  # bras écartés
    return c

HIDE_ARM = False
def draw_person(view, legs, arms, female, nude=False):
    if legs == 'dive':
        return draw_dive(view, female, nude)
    if legs == 'supine':
        return draw_supine(view, female, nude)
    c = new_canvas()
    # --- tête
    fill(c, 6, 1, 9, 5, 1)
    put(c, 5, 2, 1); put(c, 10, 2, 1); put(c, 5, 3, 1); put(c, 10, 3, 1); put(c, 5, 4, 1); put(c, 10, 4, 1)
    # cheveux
    fill(c, 6, 0, 9, 0, 3); fill(c, 5, 1, 10, 1, 3)
    if view == 'back':
        fill(c, 5, 1, 10, 4, 3); fill(c, 6, 5, 9, 5, 3)
        put(c, 6, 5, 1); put(c, 9, 5, 1)
        if female:
            fill(c, 7, 6, 8, 7, 3)  # queue de cheval (s'arrête à y 7 : bustes communs aux deux vues)
    else:
        put(c, 5, 2, 3); put(c, 10, 2, 3)
        put(c, 7, 3, 3); put(c, 9, 3, 3)  # yeux
        if female:
            fill(c, 4, 2, 4, 6, 3); fill(c, 11, 2, 11, 6, 3)
            put(c, 5, 5, 3); put(c, 10, 5, 3)
    # cou
    fill(c, 7, 6, 8, 6, 1)
    if female and view == 'back':
        put(c, 7, 6, 3); put(c, 8, 6, 3)
    # torse
    if nude:
        # au naturel : silhouette couleur peau, sans détail anatomique ;
        # seule une serviette en écharpe (couleur de l'équipe) distingue les camps
        fill(c, 5, 7, 10, 15, 1)
        fill(c, 4, 8, 11, 9, 1)
        for k in range(6):
            put(c, 5 + k, 7 + k, 2); put(c, 6 + k, 7 + k, 2)
    else:
        fill(c, 5, 7, 10, 13, 2)
        fill(c, 4, 8, 11, 9, 2)
        put(c, 7, 7, 1) if view == 'front' else None
    # short / jupe
    if nude:
        pass
    elif female:
        fill(c, 4, 14, 11, 15, 2); fill(c, 4, 15, 11, 15, 3)
    else:
        fill(c, 5, 14, 10, 15, 3)
    # jambes
    if legs == 'stand':
        fill(c, 5, 16, 6, 21, 1); fill(c, 9, 16, 10, 21, 1)
        fill(c, 4, 22, 6, 23, 3); fill(c, 9, 22, 11, 23, 3)
    elif legs == 'runA':
        fill(c, 4, 16, 5, 20, 1); fill(c, 10, 16, 11, 19, 1)
        fill(c, 3, 21, 5, 22, 3); fill(c, 10, 20, 12, 21, 3)
    elif legs == 'runB':
        fill(c, 5, 16, 6, 19, 1); fill(c, 9, 16, 10, 21, 1)
        fill(c, 4, 20, 6, 21, 3); fill(c, 9, 22, 11, 23, 3)
    elif legs == 'runC':
        # course, temps de passage : jambe gauche d'appui, genou droit levé
        fill(c, 5, 16, 6, 21, 1); fill(c, 4, 22, 6, 23, 3)
        fill(c, 9, 16, 10, 18, 1); fill(c, 10, 19, 11, 20, 1); fill(c, 10, 21, 12, 21, 3)
    elif legs == 'runD':
        fill(c, 9, 16, 10, 21, 1); fill(c, 9, 22, 11, 23, 3)
        fill(c, 5, 16, 6, 18, 1); fill(c, 4, 19, 5, 20, 1); fill(c, 3, 21, 5, 21, 3)
    elif legs == 'bent':
        # genoux fléchis (fatigue, mains sur les genoux)
        line(c, 5, 16, 4, 19, 1); line(c, 4, 19, 5, 21, 1); put(c, 6, 16, 1)
        line(c, 10, 16, 11, 19, 1); line(c, 11, 19, 10, 21, 1); put(c, 9, 16, 1)
        fill(c, 3, 22, 5, 23, 3); fill(c, 10, 22, 12, 23, 3)
    elif legs == 'lunge':
        # fente : jambe droite tendue loin sur le côté, jambe gauche fléchie
        fill(c, 5, 16, 6, 20, 1); fill(c, 4, 21, 5, 21, 1)
        line(c, 9, 16, 13, 21, 1); line(c, 10, 16, 14, 21, 1)
        fill(c, 3, 22, 5, 23, 3); fill(c, 13, 22, 15, 23, 3)
    elif legs == 'sit':
        # assis sur une chaise (changement de côté) : cuisses vers l'avant,
        # pieds posés (sol à y 19 : sprite abaissé de 4 px)
        fill(c, 5, 16, 6, 18, 1); fill(c, 9, 16, 10, 18, 1)
        fill(c, 4, 19, 6, 19, 3); fill(c, 9, 19, 11, 19, 3)
    elif legs == 'kneel':
        # genou gauche au sol, tibia couché vers l'arrière ; pied droit posé,
        # genou droit levé (sol à y 19 : sprite abaissé de 4 px)
        fill(c, 5, 16, 6, 19, 1); line(c, 4, 19, 2, 19, 1); put(c, 1, 19, 3)
        line(c, 9, 16, 12, 17, 1); line(c, 10, 16, 13, 17, 1); fill(c, 12, 18, 13, 19, 1)
        fill(c, 12, 19, 14, 19, 3)
    elif legs == 'wide':
        line(c, 5, 16, 3, 21, 1); line(c, 6, 16, 4, 21, 1)
        line(c, 9, 16, 11, 21, 1); line(c, 10, 16, 12, 21, 1)
        fill(c, 2, 22, 4, 23, 3); fill(c, 11, 22, 13, 23, 3)
    if nude:
        # pieds nus
        for y in range(20, 24):
            for x in range(16):
                if c[y][x] == 3:
                    c[y][x] = 1
    # bras + raquette (main droite côté écran droit)
    if arms == 'ready':
        fill(c, 3, 9, 3, 12, 1); fill(c, 12, 9, 12, 12, 1)
        put(c, 4, 13, 1); put(c, 12, 13, 1)
        racket(c, 12, 14, 13, 19)            # raquette tenue basse, devant soi
    elif arms == 'swing':
        fill(c, 3, 9, 3, 12, 1)
        line(c, 11, 9, 13, 9, 1)
        racket(c, 13, 8, 13, 3)              # bras tendu, tamis levé
    elif arms == 'follow':
        fill(c, 2, 8, 4, 8, 1); line(c, 11, 9, 12, 7, 1)
        racket(c, 12, 6, 13, 2, 5)           # accompagnement au-dessus de l'épaule
    elif arms == 'down':
        # déception : bras ballants, raquette vers le sol, tête baissée
        fill(c, 4, 9, 4, 14, 1); fill(c, 11, 9, 11, 14, 1)
        racket(c, 11, 15, 12, 20)
        if view == 'front':
            put(c, 7, 3, 1); put(c, 9, 3, 1); put(c, 7, 4, 3); put(c, 9, 4, 3)
    elif arms == 'serve':
        line(c, 4, 8, 3, 3, 1)          # bras de lancer levé
        line(c, 11, 8, 12, 6, 1)
        racket(c, 12, 5, 12, 2, 5)      # raquette armée au-dessus de la tête
    elif arms == 'serve_hit':
        # impact du service : bras tendu tout en haut, raquette inclinée vers
        # l'avant ; le bras de lancer redescend devant la poitrine
        line(c, 4, 8, 6, 11, 1); put(c, 7, 11, 1)
        line(c, 11, 7, 12, 3, 1)
        racket(c, 12, 3, 14, 2, 5)
    elif arms == 'backhand':
        # revers à deux mains (dessin vu en miroir : le sprite est retourné
        # quand la balle est à gauche) : les deux bras partent du même côté,
        # mains jointes sur le manche, raquette armée derrière l'épaule
        line(c, 5, 9, 10, 11, 1)
        line(c, 11, 9, 11, 11, 1)
        racket(c, 11, 11, 13, 5)
    # --- revers (dessins vus en miroir comme 'backhand' : le sprite est
    # retourné quand la balle est à gauche ; ici la balle est à droite et le
    # joueur frappe avec ses deux mains jointes du côté droit du dessin)
    elif arms == 'bprep':
        # préparation du revers : épaules tournées, les deux mains ramènent
        # la raquette basse de l'autre côté (gauche du dessin)
        line(c, 11, 9, 6, 12, 1)
        line(c, 4, 9, 5, 12, 1)
        racket(c, 5, 12, 2, 13, 5)
    elif arms == 'bimpact':
        # impact du revers à deux mains : bras tendus vers la balle, mains
        # jointes sur le manche devant la hanche, tamis au contact
        line(c, 4, 9, 11, 12, 1)
        line(c, 11, 9, 12, 12, 1)
        racket(c, 12, 12, 14, 13, 5)
    elif arms == 'bfollow':
        # fin du revers à deux mains : raquette au-dessus de l'épaule opposée,
        # les deux mains ensemble
        line(c, 11, 9, 5, 8, 1)
        line(c, 4, 9, 4, 8, 1)
        racket(c, 4, 7, 3, 2, 5)
    elif arms == 'bslice':
        # revers coupé à une main, armé : raquette haute au-dessus de l'épaule
        # opposée (même tête et même raquette que la fin du revers : rangée
        # du haut partagée), bras libre tendu en arrière pour l'équilibre
        line(c, 11, 9, 5, 8, 1)
        line(c, 4, 9, 2, 12, 1)
        racket(c, 4, 7, 3, 2, 5)
    elif arms == 'prep':
        # préparation : épaules tournées, bras gauche devant, raquette armée
        # en arrière et basse, côté coup droit
        line(c, 4, 9, 7, 12, 1)
        line(c, 11, 9, 13, 12, 1)
        racket(c, 13, 13, 14, 18, 5)
    elif arms == 'impact':
        # impact : bras tendu, tamis au contact à hauteur de hanche
        line(c, 4, 9, 2, 12, 1)
        line(c, 11, 9, 13, 11, 1)
        racket(c, 13, 11, 14, 13, 5)
    elif arms == 'volley':
        # volée : raquette devant le corps, tamis haut, petit geste sec
        line(c, 4, 9, 6, 11, 1)
        line(c, 11, 9, 12, 12, 1)
        racket(c, 12, 12, 13, 10, 5)
    elif arms == 'smash':
        # smash armé : bras gauche pointé vers la balle, raquette dans le dos
        line(c, 4, 8, 3, 2, 1)
        line(c, 11, 8, 13, 6, 1)
        racket(c, 13, 6, 13, 11, 5)
    elif arms == 'drop':
        # amortie : geste retenu, tamis bas et ouvert devant soi
        line(c, 4, 9, 2, 11, 1)
        line(c, 11, 9, 12, 13, 1)
        racket(c, 12, 14, 13, 18, 5)
    elif arms == 'lobf':
        # lob : la raquette finit vers le ciel, devant
        fill(c, 3, 9, 3, 12, 1)
        line(c, 11, 8, 12, 4, 1)
        racket(c, 12, 4, 13, 1, 3)
    elif arms == 'dribble':
        # fait rebondir la balle avant de servir : bras gauche tendu vers le sol
        line(c, 4, 9, 4, 14, 1)
        fill(c, 12, 9, 12, 12, 1); put(c, 12, 13, 1)
        racket(c, 12, 14, 13, 19)
    elif arms == 'fist':
        # poing serré, bras droit levé coude plié ; raquette dans la main gauche
        line(c, 11, 9, 13, 11, 1); line(c, 13, 11, 13, 9, 1); fill(c, 12, 8, 14, 8, 1)
        fill(c, 3, 9, 3, 12, 1)
        racket(c, 3, 13, 3, 18, 5)
    elif arms == 'angry':
        # colère : la raquette tape le sol à côté du pied, poing serré, tête
        # baissée (les bras ne passent pas devant le buste : même dessin de
        # face et de dos)
        line(c, 11, 9, 13, 14, 1); racket(c, 13, 15, 13, 20, 5)
        fill(c, 3, 9, 3, 12, 1); fill(c, 3, 13, 4, 13, 1)
    elif arms == 'tired':
        # fatigue : mains sur les genoux
        line(c, 4, 9, 3, 17, 1); line(c, 11, 9, 12, 17, 1)
        racket(c, 12, 18, 13, 21, 5)
    elif arms == 'reach':
        # balle lointaine : bras tendu au maximum, tamis vers le bas, l'autre
        # bras en arrière pour l'équilibre
        line(c, 4, 9, 1, 11, 1)
        line(c, 11, 9, 14, 11, 1)
        racket(c, 14, 12, 13, 17)
    elif arms == 'shake':
        # poignée de main : bras droit tendu sur le côté, main ouverte ;
        # raquette tenue basse dans la main gauche
        line(c, 11, 9, 14, 10, 1); fill(c, 14, 10, 15, 11, 1)
        fill(c, 3, 9, 3, 12, 1)
        racket(c, 3, 13, 2, 18, 5)
    elif arms == 'victory':
        # victoire à genoux : les deux poings levés au ciel, raquette lâchée
        line(c, 4, 9, 2, 3, 1); line(c, 3, 9, 1, 3, 1)
        line(c, 11, 9, 13, 3, 1); line(c, 12, 9, 14, 3, 1)
        fill(c, 1, 1, 2, 2, 1); fill(c, 13, 1, 14, 2, 1)
        # la raquette, lâchée, reste au sol près du genou : mêmes pixels que
        # la pose à genou (rangée des jambes commune, déjà chargée)
        tmp = new_canvas()
        racket(tmp, 12, 14, 13, 19)
        for y in range(16, 24):
            for x in range(16):
                if tmp[y][x]: c[y][x] = tmp[y][x]
    if HIDE_ARM and not nude:
        # variante « vu de dos » : un bras (ou le manche) qui passe devant le
        # buste est caché par le corps (sinon une barre couleur chair barre le
        # dos). Ces tuiles du buste sont chargées à la volée pour le joueur
        # vu de dos (slot_tick, cer.c) : le dessin commun garde le bras visible
        for y in range(8, 14):
            for x in range(5, 11):
                if c[y][x] == 1: c[y][x] = 2
    if legs in ('kneel', 'sit'):
        # un genou à terre (relevé après un plongeon) ou assis : le sprite est
        # dessiné 4 px plus bas (match_draw, rp_play) ; sous y 19 (le sol),
        # rien n'est dessiné
        for y in range(20, 24):
            for x in range(16):
                c[y][x] = 0
    return c

def canvas_tiles(c):
    out = []
    for ty in range(3):
        for tx in range(2):
            out.append([c[ty*8+y][tx*8:tx*8+8] for y in range(8)])
    return out

POSES = [  # (jambes, bras) — l'ordre est repris dans src/gfx.c
    ('stand', 'ready'),   # 0 attente
    ('runA', 'ready'),    # 1 course A
    ('runB', 'ready'),    # 2 course B
    ('wide', 'swing'),    # 3 frappe (armé)
    ('wide', 'follow'),   # 4 frappe (accompagnement)
    ('stand', 'serve'),   # 5 service
    ('wide', 'ready'),    # 6 joie / fente
    ('stand', 'down'),    # 7 déception
    ('wide', 'backhand'), # 8 revers à deux mains (armé)
    ('lunge', 'reach'),   # 9 extension (balle lointaine)
    ('stand', 'serve_hit'),  # 10 impact du service
    ('wide', 'prep'),     # 11 préparation (coup droit)
    ('wide', 'impact'),   # 12 impact
    ('wide', 'volley'),   # 13 volée
    ('wide', 'drop'),     # 14 amortie
    ('stand', 'dribble'), # 15 rebond de la balle avant le service
    ('stand', 'fist'),    # 16 poing serré
    ('wide', 'angry'),    # 17 colère (raquette au sol)
    ('bent', 'tired'),    # 18 fatigue (mains sur les genoux)
    ('runC', 'ready'),    # 19 course, passage (genou droit)
    ('runD', 'ready'),    # 20 course, passage (genou gauche)
    ('dive', 'dive'),     # 21 plongeon (dessin à part : draw_dive)
    ('wide', 'bprep'),    # 22 préparation du revers
    ('wide', 'bimpact'),  # 23 impact du revers à deux mains
    ('wide', 'bfollow'),  # 24 fin du revers à deux mains
    ('wide', 'bslice'),   # 25 revers coupé à une main (armé)
    ('wide', 'bimpact'),  # 26 volée de revers (même geste que l'impact à deux mains)
    ('kneel', 'ready'),   # 27 un genou à terre (relevé du plongeon), dessiné 4 px plus bas
    ('stand', 'shake'),   # 28 poignée de main au filet (fin du match)
    ('kneel', 'victory'), # 29 victoire à genoux, poings levés (balle de match), 4 px plus bas
    ('supine', 'supine'), # 30 allongé sur le dos (balle de match)
    ('sit', 'ready'),     # 31 assis (changement de côté), 4 px plus bas
]
# (le smash armé reprend la pose du service, et la fin du lob celle de
# l'impact du service : mêmes gestes, aucune tuile de plus)
NPOSE = len(POSES)
# Réserve commune de tuiles : beaucoup de rangées (tête, buste, jambes) se
# répètent d'une pose à l'autre. Chaque rangée de 2 tuiles n'est rangée
# qu'une fois ; une table donne, pour chaque pose, la tuile de chacune des
# 3 rangées. La version au naturel partage les mêmes numéros (une rangée est
# commune si elle l'est dans les deux versions).
next_spr = 0x10
POOL0 = next_spr
pool = {}
nude_people = bytearray()
# Tuiles des personnages : chaque pose = 3 rangées de 16 x 8 (2 tuiles),
# partagées entre poses identiques. Version 2.42 : les rangées propres aux
# célébrations (déception, rebond avant le service, poing serré, colère,
# fatigue) et celles propres au revers (préparation, impact, fin, revers
# coupé, volée) occupent les MÊMES tuiles (« jeu de tuiles par phase ») :
# load_court et swap_tick (extras.c) y chargent le jeu utile à la phase de
# jeu (célébrations hors échange, revers pendant l'échange).
SET_C = {7, 15, 16, 17, 18}
SET_R = {22, 23, 24, 25, 26}
SET_F = {28, 29, 30, 31}   # séquences animées (entrée, changement de côté, fin du match)
row_use = {}       # clé de rangée -> poses qui l'utilisent ('prof' : course de profil)
row_data = {}      # clé -> (2 tuiles, 2 tuiles au naturel)
person_keys = []   # clés dans l'ordre de person_map
def row_keys(cv, nv):
    t, tn = canvas_tiles(cv), canvas_tiles(nv)
    out = []
    for r in range(3):
        key = tuple(tuple(map(tuple, x)) for x in (t[2*r], t[2*r+1], tn[2*r], tn[2*r+1]))
        row_data.setdefault(key, ((t[2*r], t[2*r+1]), (tn[2*r], tn[2*r+1])))
        out.append(key)
    return out
def row_class(key):
    u = row_use[key]
    if u <= SET_C: return 'C'
    if u <= SET_R: return 'R'
    if u <= SET_F: return 'F'
    return 'res'
preview_people = []
for female in (0, 1):
    for view in ('back', 'front'):
        for k, (legs, arms) in enumerate(POSES):
            cv = draw_person(view, legs, arms, female)
            preview_people.append(cv)
            for key in row_keys(cv, draw_person(view, legs, arms, female, nude=True)):
                row_use.setdefault(key, set()).add(k)
                person_keys.append(key)
for female in (0, 1):
    for view in ('back', 'front'):
        for (legs, arms) in POSES:
            preview_people.append(draw_person(view, legs, arms, female, nude=True))

# --- course de profil (vue de côté, tournée vers la droite ; miroir à gauche)
def draw_profile(frame, female, nude=False):
    c = new_canvas()
    # tête : cheveux à l'arrière, visage tourné vers la droite
    fill(c, 6, 1, 10, 5, 1)
    fill(c, 5, 0, 9, 1, 3); fill(c, 5, 1, 6, 4, 3)
    put(c, 9, 2, 3)                     # oeil
    put(c, 11, 3, 1)                    # nez
    if female:
        fill(c, 3, 2, 4, 4, 3); put(c, 4, 1, 3)   # queue de cheval
    fill(c, 7, 6, 8, 6, 1)              # cou
    # buste de profil (plus étroit)
    if nude:
        fill(c, 6, 7, 9, 15, 1)
        for k in range(5):
            put(c, 6 + (k % 4), 7 + k, 2)
    else:
        fill(c, 6, 7, 9, 13, 2)
        if female:
            fill(c, 5, 14, 10, 15, 2); fill(c, 5, 15, 10, 15, 3)
        else:
            fill(c, 6, 14, 9, 15, 3)
    if frame == 0:
        # foulée : bras opposés, jambes écartées
        line(c, 6, 8, 4, 11, 1)                          # bras arrière
        line(c, 9, 8, 11, 11, 1)                         # bras avant
        racket(c, 11, 12, 13, 17, 5)                     # raquette tenue devant
        line(c, 8, 16, 11, 21, 1); line(c, 9, 16, 12, 21, 1)   # jambe avant
        line(c, 7, 16, 4, 21, 1); line(c, 6, 16, 3, 21, 1)     # jambe arrière
        fill(c, 11, 22, 13, 23, 1 if nude else 3); fill(c, 2, 22, 4, 23, 1 if nude else 3)
    elif frame == 2:
        # passage, genou avant levé, bras à mi-course
        line(c, 6, 8, 5, 11, 1)
        line(c, 9, 8, 11, 10, 1)
        racket(c, 11, 11, 13, 15, 5)
        fill(c, 6, 16, 7, 21, 1); fill(c, 6, 22, 8, 23, 1 if nude else 3)       # jambe d'appui
        line(c, 8, 16, 10, 18, 1); line(c, 10, 18, 10, 20, 1)                  # genou levé
        put(c, 11, 20, 1 if nude else 3)
    elif frame == 3:
        # passage, talon arrière qui remonte
        line(c, 6, 8, 5, 12, 1)
        line(c, 9, 8, 10, 11, 1)
        racket(c, 10, 12, 12, 17, 5)
        fill(c, 8, 16, 9, 21, 1); fill(c, 8, 22, 10, 23, 1 if nude else 3)      # jambe d'appui
        line(c, 7, 16, 5, 18, 1); line(c, 5, 18, 4, 19, 1)                     # talon qui remonte
        put(c, 3, 19, 1 if nude else 3)
    else:
        # jambes croisées (passage), bras le long du corps
        line(c, 6, 8, 5, 12, 1)
        line(c, 9, 8, 10, 12, 1)
        racket(c, 10, 13, 12, 18, 5)
        fill(c, 7, 16, 8, 21, 1)
        line(c, 7, 16, 5, 20, 1)
        fill(c, 7, 22, 9, 23, 1 if nude else 3); put(c, 4, 21, 1 if nude else 3)
    return c

prof_keys = []
for female in (0, 1):
    for frame in (0, 1, 2, 3):       # course 1, 2 (foulées), 3, 4 (passages)
        cv = draw_profile(frame, female)
        preview_people.append(cv)
        for key in row_keys(cv, draw_profile(frame, female, nude=True)):
            row_use.setdefault(key, set()).add('prof')
            prof_keys.append(key)
# rangées permanentes : à la suite (copiées au naturel par people_tiles)
pool = {}
for key in list(dict.fromkeys(person_keys + prof_keys)):
    if row_class(key) == 'res':
        pool[key] = next_spr
        (a0, a1), (n0, n1) = row_data[key]
        spr_tile(next_spr, a0); spr_tile(next_spr + 1, a1)
        nude_people.extend(encode_tile(n0)); nude_people.extend(encode_tile(n1))
        next_spr += 2
prof_map = [pool[k] for k in prof_keys]
POOL_END = next_spr

# coupe (2x2 tuiles, palette 2 : 1 or clair, 2 sombre, 3 reflet) et confettis
cup = [[0]*16 for _ in range(16)]
for y in range(0, 9):
    w = 6 if y < 6 else 6 - (y - 5) * 2
    for x in range(8 - w, 8 + w):
        cup[y][x] = 1
for y in range(1, 6):
    cup[y][1] = 1; cup[y][14] = 1          # anses
cup[0][1] = cup[0][14] = 1
for y in range(9, 13):
    cup[y][7] = cup[y][8] = 1               # pied
for x in range(4, 12):
    cup[13][x] = 2; cup[14][x] = 2          # socle
for y in range(1, 5):
    cup[y][4] = 3                           # reflet
for y in range(0, 9):
    for x in range(16):
        if cup[y][x] == 1 and (x == 0 or cup[y][x-1] == 0 or x == 15 or cup[y][x+1] == 0):
            cup[y][x] = 2 if y > 0 else cup[y][x]
TROPHY_TILE = next_spr
for ty in range(2):
    for tx in range(2):
        spr_tile(next_spr, [cup[ty*8+y][tx*8:tx*8+8] for y in range(8)]); next_spr += 1
BALL_MID_TILE = BALL_BIG_TILE = 0   # (anciennes tuiles de balle : remplacées par la série BALL_BASE)
CONFETTI_TILE = next_spr
conf = blank()
for (x, y) in ((3, 3), (4, 3), (3, 4), (4, 4)):
    conf[y][x] = 1
conf[3][4] = 3
spr_tile(next_spr, conf); next_spr += 1

# Balle (palette des effets : 1 couleur de la balle, 3 couture blanche).
# 3 tailles (petite, moyenne, grande) x 4 positions de la couture : elle
# tourne selon l'effet (lifté, coupé). Puis la balle écrasée au rebond, la
# balle vue à travers le filet (petite, moyenne) et l'ombre moyenne et petite.
def ball_mask(size):
    if size == 0:
        return {(x, y) for (x, y) in ((2,1),(3,1),(1,2),(2,2),(3,2),(4,2),(1,3),(2,3),(3,3),(4,3),(2,4),(3,4))}
    if size == 1:
        return {(x + 1, y + 1) for y in range(5) for x in range(5) if (x - 2) ** 2 + (y - 2) ** 2 <= 5}
    if size == 3:      # 6 x 6, entre la moyenne et la grande
        return {(x + 1, y + 1) for y in range(6) for x in range(6) if (x - 2.5) ** 2 + (y - 2.5) ** 2 <= 7.5}
    return {(x, y) for y in range(7) for x in range(7) if (x - 3) ** 2 + (y - 3) ** 2 <= 10}
BALL_BASE = next_spr
def ball_rot_tiles(size):
    global next_spr
    m = ball_mask(size)
    ys = sorted({y for _, y in m}); xs = sorted({x for x, _ in m})
    top, h = ys[0], len(ys); x0, w = xs[0], len(xs)
    for f in range(4):
        t = blank()
        for (x, y) in m: t[y][x] = 1
        r = top + (f * h) // 4 + (1 if h > 4 else 0) * (f > 0)
        r = min(r, ys[-1])
        for (x, y) in m:
            mid = x0 + w // 4 <= x <= x0 + w - 1 - w // 4
            if (y == r and mid) or (y == r - 1 and not mid and h > 4):
                t[y][x] = 3
        spr_tile(next_spr, t); next_spr += 1
for size in range(3):
    ball_rot_tiles(size)
sq = blank()
for (y, a0, a1) in ((4, 2, 5), (5, 1, 6), (6, 2, 5)):
    for x in range(a0, a1 + 1): sq[y][x] = 1
spr_tile(next_spr, sq); next_spr += 1                    # base + 12 : écrasée
for size in (0, 1):                                     # base + 13, 14 : derrière le filet
    t = blank()
    for (x, y) in ball_mask(size):
        if (x + y) % 2 == 0: t[y][x] = 1
    spr_tile(next_spr, t); next_spr += 1
shm = blank()
for (x, y) in ((2, 3), (3, 3), (1, 4), (2, 4), (3, 4), (4, 4)): shm[y][x] = 2
spr_tile(next_spr, shm); next_spr += 1                   # base + 15 : ombre moyenne
shs = blank()
for (x, y) in ((2, 4), (3, 4)): shs[y][x] = 2
spr_tile(next_spr, shs); next_spr += 1                   # base + 16 : petite ombre
ball_rot_tiles(3)                                       # base + 17 à 20 : balle de 6 x 6

assert next_spr <= 256, "trop de tuiles de sprites"

# --- Margaux : cheveux longs, blonds et bouclés (couleur 3 de sa palette).
# Seule la rangée du haut (tête, 2 tuiles) lui est propre ; le corps reste
# celui des joueuses. Une tête par vue et par pose (têtes identiques partagées)
# + 1 tête de profil pour la course.
def margaux_hair(c, view):
    hair = set()
    for x in range(5, 11): hair.add((x, 0))
    for x in range(4, 12): hair.add((x, 1))
    if view == 'back':
        for y in range(0, 8):
            for x in range(4, 12):
                hair.add((x, y))
    else:
        hair |= {(6, 2), (9, 2)}                          # frange bouclée
        for y in range(2, 8):
            hair |= {(4, y), (11, y)}                     # mèches qui tombent sur les épaules
        hair |= {(5, 5), (10, 5), (5, 6), (10, 6), (5, 7), (10, 7)}
    # boucles : petites bosses alternées sur le contour
    for y in (2, 4, 6):
        hair.add((3, y))
    for y in (3, 5, 7):
        hair.add((12, y))
    if view == 'back':
        for y in (3, 5, 7):
            hair.add((3, y))
        for y in (2, 4, 6):
            hair.add((12, y))
    for (x, y) in hair:
        # les bras et la raquette passent devant les cheveux ; de dos, la
        # chevelure couvre toute la tête et le haut du dos
        if c[y][x] in (0, 3) or (view == 'back' and (c[y][x] == 2 or (c[y][x] == 1 and 4 <= x <= 11))):
            c[y][x] = 3
        elif c[y][x] == 2 and y == 7 and x in (5, 10):
            c[y][x] = 3                                   # mèches sur le haut des épaules
    if view == 'back':
        # pointes bouclées : le haut du dos apparaît entre les boucles
        for x in range(4, 12, 2):
            if c[7][x] == 3:
                c[7][x] = 2 if 5 <= x <= 10 else 0
        c[6][4] = 0 if c[6][4] == 3 and c[6][3] == 3 else c[6][4]
    return c

def margaux_profile_hair(c):
    for x in range(5, 11): c[0][x] = 3
    for y in range(1, 8):
        for x in range(4, 7):
            if c[y][x] in (0, 3, 2):
                c[y][x] = 3
    c[1][9] = 3; c[1][10] = 3                             # frange
    for y in (2, 4, 6):
        c[y][3] = 3
    return c

# tête de Margaux pour chaque vue et chaque pose (les bras levés passent
# dans la rangée de la tête) ; les têtes identiques sont partagées
marg_previews = []
marg_pool = {}
marg_keys = []
marg_use = {}
marg_data = {}
for view in ('back', 'front'):
    for k, (legs, arms) in enumerate(POSES):
        cv = draw_person(view, legs, arms, 1)
        if any(cv[y][x] for y in range(8) for x in range(16)):
            cv = margaux_hair(cv, view)     # (rangée du haut vide : plongeon, allongée, rien à coiffer)
        t = canvas_tiles(cv)
        key = ('marg', tuple(map(tuple, t[0])), tuple(map(tuple, t[1])))
        if key not in marg_data:
            marg_data[key] = (t[0], t[1])
            marg_previews.append(cv)
        marg_use.setdefault(key, set()).add(k)
        marg_keys.append(key)
def marg_class(key):
    u = marg_use[key]
    if u <= SET_C: return 'C'
    if u <= SET_R: return 'R'
    if u <= SET_F: return 'F'
    return 'res'
for key in dict.fromkeys(marg_keys):
    if marg_class(key) == 'res':
        marg_pool[key] = next_spr
        spr_tile(next_spr, marg_data[key][0]); spr_tile(next_spr + 1, marg_data[key][1]); next_spr += 2
pv = margaux_profile_hair(draw_profile(0, 1))
marg_previews.append(pv)
tp = canvas_tiles(pv)
MARG_PROF_L, MARG_PROF_R = 0x0F, next_spr
spr_tile(MARG_PROF_L, tp[0]); spr_tile(MARG_PROF_R, tp[1]); next_spr += 1
# éclats de glace et grains de sable des glissades (couleur 3 : blanc) ; la
# tuile 15 est celle des cheveux de Margaux de profil
ICE_TILE = next_spr
spr_tile(ICE_TILE, ice); next_spr += 1
# accessoires des séquences animées (sprites 8 x 8, palette 2 : 1 beige,
# 2 noir, 3 blanc)
def prop_chair():
    """chaise vue de haut : dossier, assise blanche, pieds noirs"""
    t = [[0] * 8 for _ in range(8)]
    for x in range(1, 7): t[0][x] = 2; t[1][x] = 3
    for y in range(2, 6):
        for x in range(1, 7): t[y][x] = 3 if 1 < x < 6 else 2
    for x in (1, 6): t[6][x] = 2; t[7][x] = 2
    return t
def prop_bottle():
    """gourde : bouchon noir, corps blanc, étiquette beige"""
    t = [[0] * 8 for _ in range(8)]
    t[0][3] = t[0][4] = 2
    for y in range(1, 8):
        for x in range(2, 6): t[y][x] = 3
    for x in range(2, 6): t[4][x] = 1; t[5][x] = 1
    return t
def prop_towel():
    """serviette blanche à bandes noires, posée sur les épaules"""
    t = [[0] * 8 for _ in range(8)]
    for y in range(0, 5):
        for x in range(0, 8): t[y][x] = 3
    for x in range(0, 8): t[2][x] = 2
    for y in range(5, 8): t[y][0] = t[y][7] = 3
    return t
def prop_racket():
    """raquette posée au sol (lâchée), tamis en biais"""
    t = [[0] * 8 for _ in range(8)]
    for (x, y) in ((1, 7), (2, 6), (3, 5)): t[y][x] = 1          # manche
    for y in range(0, 5):
        for x in range(3, 8):
            if (x - 5) ** 2 + (y - 2) ** 2 <= 5:
                t[y][x] = 2 if (x - 5) ** 2 + (y - 2) ** 2 >= 3 else (3 if (x + y) % 2 else 0)
    return t
# zone partagée : jeu « célébrations » (chargé au départ : c'est lui que
# montrent les écrans) ou jeu « revers » (pendant l'échange)
SWAP0 = next_spr
swap_sets = {}
PROPS = [prop_chair(), prop_bottle(), prop_towel(), prop_racket()]
for cls in ('C', 'R', 'F'):
    tiles, nude, idx = [], [], SWAP0
    for key in dict.fromkeys(person_keys):
        if row_class(key) == cls:
            pool[key] = idx; idx += 2
            (a0, a1), (n0, n1) = row_data[key]
            tiles += [a0, a1]; nude += [n0, n1]
    for key in dict.fromkeys(marg_keys):
        if marg_class(key) == cls:
            marg_pool[key] = idx; idx += 2
            tiles += list(marg_data[key]); nude += list(marg_data[key])   # (Margaux jamais au club nudiste)
    if cls == 'F':
        # accessoires des séquences : chaise, gourde, serviette, raquette au sol
        PROP0 = idx
        tiles += PROPS; nude += PROPS; idx += len(PROPS)
    swap_sets[cls] = (tiles, nude)
NSWAP = max(len(swap_sets['C'][0]), len(swap_sets['R'][0]), len(swap_sets['F'][0]))
print('jeux de tuiles par phase : C %d, R %d, F %d (zone %d)' % (tuple(len(swap_sets[k][0]) for k in 'CRF') + (NSWAP,)))
for i, t in enumerate(swap_sets['C'][0]):
    spr_tile(SWAP0 + i, t)
next_spr = SWAP0 + NSWAP
person_map = [pool[k] for k in person_keys]
marg_map = [marg_pool[k] for k in marg_keys]
print('tuiles partagées : %d (célébrations %d, revers %d)' % (NSWAP, len(swap_sets['C'][0]), len(swap_sets['R'][0])))
assert next_spr <= 256, "trop de tuiles de sprites (%d)" % next_spr

# --------------------------------------------------------------------------
# Vue en perspective (option VUE du menu), 15 % : pour chaque lieu, le court
# (colonnes 4 à 27, rangées 4 à 29) est déformé comme vu par une caméra : le
# fond est 15 % plus étroit que le devant et la moitié du fond plus courte.
# Projection d'un point du court à plat (X, Y) : d = (216 - Y) / 168 (0 ligne
# de fond du bas, 1 ligne de fond du haut), s = 1 / (1 + a d) avec a = 1/0,85 - 1,
# x' = 128 + (X - 128) s, y' = 216 - 168 (1 + a) d s. Les lignes de fond restent
# à y 216 et 48 ; le filet remonte de 132 à 125. Les tribunes, le bandeau et la
# chaise d'arbitre ne bougent pas.
# Les tuiles nouvelles (lignes obliques) prennent, pendant le match, la place
# de tuiles inutilisées sur ce lieu (décors des autres lieux, titre...) ; la
# table d'origine est rétablie hors du match.
# --------------------------------------------------------------------------
PERSP_A = 1 / 0.85 - 1
PERSP_K = 168 * (1 + PERSP_A)
def persp_d(ys):
    u = (216 - ys) / PERSP_K
    return u / (1 - PERSP_A * u)
def persp_inv(xs, ys):
    # échelle horizontale constante sur chaque rangée de tuiles (8 px) : les
    # lignes obliques ne décalent d'un pixel qu'aux frontières de tuiles (la
    # pente est de moins d'un pixel par rangée), d'où peu de tuiles nouvelles
    sc = 1 / (1 + PERSP_A * persp_d((ys // 8) * 8 + 4))
    return 128 + (xs - 128) / sc, 216 - 168 * persp_d(ys)
def persp_net(w):
    """filet redessiné à sa place en perspective, avec ses mailles régulières
    (déformées, elles donnaient une tuile différente par colonne)"""
    ny = round(216 - PERSP_K * 0.5 / (1 + PERSP_A * 0.5))
    sc = 1 / (1 + PERSP_A * 0.5)
    x0n, x1n = round(128 + (DBL_L - 6 - 128) * sc), round(128 + (DBL_R + 6 - 128) * sc)
    # fond sous le filet : celui de la rangée juste au-dessous (le court, ou
    # les abords hors du court : sinon la couleur du court débordait jusqu'aux
    # poteaux, à hauteur du filet), sans les lignes
    base = [1 if w[ny + 6][x] == 2 else w[ny + 6][x] for x in range(256)]
    for y in range(ny - 9, ny + 4):                    # efface le filet déformé
        for x in range(32, 224):
            w[y][x] = base[x]
    for y in range(ny - 5, ny):
        for x in range(x0n, x1n + 1):
            w[y][x] = 3 if (x % 4 == 0 or (y - ny) % 2 == 0) else base[x]
    for x in range(x0n, x1n + 1):
        w[ny][x] = 3                                   # câble du bas
        w[ny - 7][x] = 2; w[ny - 6][x] = 2             # bande blanche
    for y in range(ny - 5, ny + 1):
        w[y][127] = 2; w[y][128] = 2                   # sangle centrale
    for y in (ny + 1, ny + 2):                         # ombre portée
        for x in range(x0n + 2, x1n + 3):
            if (x + y) % 2 == 0 and (y == ny + 1 or x % 4 < 2):
                w[y][x] = 3
    for px in (x0n - 2, x1n - 2):                      # poteaux
        for y in range(ny - 11, ny + 2):
            for x in range(px, px + 3):
                w[y][x] = 3
        for y in range(ny - 10, ny + 1):
            w[y][px] = 2
        for x in range(px, px + 3):
            w[ny - 11][x] = 2

# ola_row (extras.c) : bras levés = tuile + 4, et tuiles 5-8 (stade), 13-16 (club, salle)
assert sorted(set(crowd_idx)) == [5, 6, 7, 8] and all(c + 4 == h for c, h in zip(crowd_idx, cheer_idx))
assert sorted(set(crowdb_idx)) == [13, 14, 15, 16] and all(c + 4 == h for c, h in zip(crowdb_idx, cheerb_idx))
def persp_lines(w):
    """lignes verticales du court (côtés, médiane de service, marques
    centrales) redessinées en perspective, sans trou : l'échantillonnage du
    court à plat sautait parfois leur colonne d'un pixel au fond, où le court
    est rétréci"""
    verts = [(x, TOP_BASE, BOT_BASE) for x in (DBL_L, SGL_L, SGL_R, DBL_R)]
    verts += [(CENTER_X, TOP_SERV, BOT_SERV), (CENTER_X, TOP_BASE + 1, TOP_BASE + 3), (CENTER_X, BOT_BASE - 3, BOT_BASE - 1)]
    def lx(X, ty):
        sc = 1 / (1 + PERSP_A * persp_d(ty * 8 + 4))
        return int(round(128 + (X + 0.5 - 128) * sc - 0.5))
    # Au filet, chaque ligne doit ressortir là où elle entre : la rangée de
    # tuiles visible au-dessus (t - 2) et celle au-dessous (t + 1) du filet
    # (rangée t) prennent la même colonne. Sinon, la marche d'un pixel tombe
    # derrière le filet et la ligne paraît cassée. Au-dessus du filet,
    # l'escalier de la ligne est décalé d'une rangée, en remontant, jusqu'à
    # rejoindre sa place : chaque marche reste d'un pixel.
    ny = round(216 - PERSP_K * 0.5 / (1 + PERSP_A * 0.5))
    t = ny // 8
    fix = {}
    for (X, y0, y1) in verts[:4]:
        b = lx(X, t + 1)
        if lx(X, t - 2) == b: continue
        m = {t: b, t - 1: b, t - 2: b}
        ty = t - 3
        while ty >= TOP_BASE // 8:
            n, prev = lx(X, ty), m[ty + 1]
            if abs(n - prev) <= 1:
                break                      # l'escalier a rejoint sa place
            m[ty] = prev + (1 if n > prev else -1)
            ty -= 1
        fix[X] = m
    for ys in range(32, 240):
        Y = persp_inv(128, ys + 0.5)[1]
        sc = 1 / (1 + PERSP_A * persp_d((ys // 8) * 8 + 4))
        for (X, y0, y1) in verts:
            if y0 <= Y < y1 + 1:
                xs = int(round(128 + (X + 0.5 - 128) * sc - 0.5))
                f = fix.get(X, {}).get(ys // 8)
                if f is not None and f != xs:
                    w[ys][xs] = w[ys][xs + (2 if f < xs else -2)]   # efface la ligne échantillonnée
                    xs = f
                w[ys][xs] = 2

KEEP = set(range(0x60)) | set(crowd_idx) | set(cheer_idx) | set(crowdb_idx) | set(cheerb_idx) | \
       set(crowd_idx3) | set(crowd_idx4) | set(SIGN_IDX) | set(SIGN2_IDX) | set(NET_IDX)
persp_rles, persp_tiles, persp_ps = [], [], []

# Tribunes en perspective : la rambarde suit l'inclinaison des lignes de côté
# (point X = 31 ou 224 projeté, échelle constante par rangée de tuiles, pas de
# 2 px) et les tribunes s'avancent jusqu'à elle. Les colonnes ajoutées
# reprennent le décor du lieu ; une colonne « bord » (fond du décor, rambarde,
# puis abords) ferme la tribune. Dans la moitié du fond (rangées 4 à 15), les
# spectateurs sont plus petits (environ 85 %).
SIZES = ('med', 'small', 'tiny')
SMALL = {fam: {sz: {v: [spectator(d, v, fam == 'B', sz) for d in range(DESIGNS)] for v in ('down', 'up', 'cheer')}
               for sz in SIZES} for fam in ('A', 'B')}
def size_of_row(ty):
    """taille des spectateurs selon l'échelle de la rangée (perspective)"""
    sc = persp_row_scale(ty)
    if sc >= 0.965: return None          # premier plan : taille normale
    if sc >= 0.93: return 0              # 'med'
    if sc >= 0.895: return 1             # 'small'
    return 2                             # 'tiny' : foule lointaine
PERSP_FAM = {0: 'A', 1: 'B', 2: 'B', 5: 'B'}   # lieux avec du public animé

def persp_row_scale(ty):
    return 1 / (1 + PERSP_A * persp_d(ty * 8 + 4))
PERSP_STEP = 1
def persp_rail(ty, side):
    sc = persp_row_scale(ty)
    x = 128 + ((31 if side == 0 else 224) - 128) * sc
    if PERSP_STEP == 1:
        return round(x)
    return 2 * round(x / 2) + (1 if side == 0 else 0)   # gauche : impair (31, 33...) ; droite : pair

def tile_at(bmp, ty, tx):
    return [list(bmp[ty * 8 + y][tx * 8:tx * 8 + 8]) for y in range(8)]

def persp_sides(e, bmp, w):
    """Écrit dans w (image perspective) les tribunes des deux côtés.
    Renvoie {(ty, tx): palette} pour les blocs d'attributs à corriger."""
    fam = PERSP_FAM.get(e)
    rng = random.Random(100 + e)
    for side in (0, 1):
        for ty in range(4, 30):
            xr = persp_rail(ty, side)
            if side == 0 and CHAIR_Y // 8 <= ty < CHAIR_Y // 8 + 4:
                xr = 31                                   # chaise d'arbitre devant la tribune
            ce = xr // 8                                  # colonne du bord
            nins = (ce - 3) if side == 0 else (28 - ce)  # colonnes ajoutées
            outer = [tile_at(bmp, ty, (o if side == 0 else 31 - o)) for o in range(4)]
            crowd_row = (e == 0) or (e == 2) or (e == 1 and ty in CLUB_ROWS) or (e == 5 and ty in CASTLE_ROWS)
            # fond du décor (colonne du bord et colonnes ajoutées sans public)
            def filler(x, y):
                if e == 0: return 2
                if e in (1, 5) and crowd_row: return 0
                if e == 2: return 3 if (y // 8) % 6 == 0 else 1   # salle : mur et bandeau
                if e == 1: return 1 if (((x % 8) * 7 + (y % 8) * 3) % 11) else 2
                if e == 5:
                    ry = y % 8; off = 0 if ry < 4 else 4
                    return 2 if (ry % 4 == 3 or (x + off) % 8 == 7) else 1
                return 1                                  # salle (mur), club nudiste (herbe), Margaux
            def ext(k):
                col = 3 + k                               # colonne « virtuelle » du décor à plat
                if e == 0:
                    return [r[:] for r in CROWD[rng.randrange(6)]]
                if e == 2:                                # salle : gradins prolongés (côté extérieur)
                    return [r[:] for r in CROWDB[(ty * 5 + col * 2 + side) % 6]]
                if e in (1, 5) and crowd_row:
                    return [r[:] for r in CROWDB[(ty * 3 + col + side) % 6]]
                if e == 4:
                    kk = (ty + col * 3 + side * 3) % 6
                    if kk % 2 == 0:
                        return [r[:] for r in GIRLY[kk // 2]]
                return [[filler(x, y) for x in range(8)] for y in range(8)]
            ia = 2 if e == 2 else 3                       # salle : ajout entre gradins et panneaux
            shift = (not crowd_row and e != 4) or e == 2  # décor sans public : glissé vers le court
            if shift and nins == 0 and xr == (31 if side == 0 else 224):
                seq = outer[:]                            # comme à plat
                nins = -1
            elif shift and nins >= 1:
                # le décor à plat entier (arbres, bannières, torches, spectateurs
                # du club nudiste) glisse vers le court ; du fond est ajouté
                # côté écran, la rambarde d'origine devient du fond
                last = [r[:] for r in outer[3]]
                rx = 7 if side == 0 else 0
                for y in range(8):
                    last[y][rx] = filler((3 * 8 + rx) if side == 0 else (224 + rx), ty * 8 + y)
                fill = [[[filler(x, y) for x in range(8)] for y in range(8)] for _ in range(nins - 1)]
                if e == 2:
                    # salle : les gradins restent au bord de l'écran (palette 3,
                    # bloc de colonnes 0-1) ; le mur s'élargit avant les panneaux
                    seq = outer[:2] + fill + outer[2:3] + [last]
                else:
                    seq = fill + outer[:3] + [last]
            else:
                seq = outer[:ia] + [ext(k) for k in range(nins)] + outer[ia:3]
            edge = [[0] * 8 for _ in range(8)]
            off = xr - ce * 8
            for y in range(8):
                for x in range(8):
                    X = ce * 8 + x
                    inside = x < off if side == 0 else x > off
                    if x == off:
                        v = 3 if e != 3 else (3 if (ty * 8 + y) % 4 < 2 else 1)
                    elif inside:
                        v = filler(X, ty * 8 + y)
                    else:
                        v = 0
                    edge[y][x] = v
            if nins != -1:
                seq.append(edge)
            sz = size_of_row(ty)
            if fam and sz is not None:                    # plus petits vers le fond
                big = CROWD if fam == 'A' else CROWDB
                keys = [tkey(t) for t in big]
                for i, t in enumerate(seq):
                    if tkey(t) in keys:
                        seq[i] = [r[:] for r in SMALL[fam][SIZES[sz]]['down'][ORDER[keys.index(tkey(t))]]]
            for o, t in enumerate(seq):
                tx = o if side == 0 else 31 - o
                for y in range(8):
                    w[ty * 8 + y][tx * 8:tx * 8 + 8] = t[y]
    # blocs d'attributs des colonnes 4-5 et 26-27 : palette de la tribune là
    # où elle s'avance (jamais de surface de jeu dans ces blocs)
    fix = {}
    for by in range(2, 15):
        for side, bxo, bxi in ((0, 1, 2), (1, 14, 13)):
            if side == 0 and bxi == CHAIR_X // 16 and CHAIR_Y // 16 <= by < CHAIR_Y // 16 + 2:
                continue
            reach = False
            for ty in (by * 2, by * 2 + 1):
                if ty < 4: continue
                xr = persp_rail(ty, side)
                if (side == 0 and xr >= 32) or (side == 1 and xr <= 223):
                    reach = True
            if reach:
                # aucun pixel de la surface de jeu dans ce bloc
                for y in range(by * 16, by * 16 + 16):
                    sc = 1 / (1 + PERSP_A * persp_d(y))
                    xe = 128 + ((DBL_L - 6 if side == 0 else DBL_R + 6) - 128) * sc
                    assert (xe > bxi * 16 + 15) if side == 0 else (xe < bxi * 16), ('tribune et court dans un même bloc', e, by)
                fix[(bxi, by)] = court_attr(bxo, by, e)
    return fix

def persp_build(e):
    bmp = draw_court(e)
    w = [row[:] for row in bmp]
    for ys in range(32, 240):
        for xs in range(32, 224):
            X, Y = persp_inv(xs + 0.5, ys + 0.5)
            X, Y = int(X), int(min(239, max(32, Y)))
            w[ys][xs] = bmp[Y][X] if 32 <= X < 224 else 0     # au-delà : abords (couleur de fond)
    persp_lines(w)
    persp_net(w)
    fix = persp_sides(e, bmp, w)
    for y in range(32):                                   # chaise d'arbitre : à sa place
        for x in range(16):
            if CHAIR[y][x]:
                w[CHAIR_Y + y][CHAIR_X + x] = CHAIR[y][x]
    nt = bytearray(court_nts[e])
    for (bx, by), pal in fix.items():
        set_attr(nt, bx * 2, by * 2, pal)
    # tuiles du décor final : connues (déjà en table), ou nouvelles. Les tuiles
    # du court à plat absentes du décor final sont libres pendant le match.
    tiles = {}
    for ty in range(4, 30):
        for tx in range(32):
            tiles[(ty, tx)] = tile_at(w, ty, tx)
    known = {tkey(bg_tiles[k]): k for k in range(256) if bg_tiles[k] is not None}
    final_known = {known[tkey(t)] for t in tiles.values() if tkey(t) in known}
    keep = KEEP | set(nt[:128]) | final_known
    cand = [k for k in range(256) if k not in keep]
    new = {}
    fam = PERSP_FAM.get(e)
    ps = []
    if fam:
        # petits spectateurs : 4 dessins distincts à 4 places consécutives, et
        # leurs « bras levés » aux 4 suivantes (ola_row : t + 4, comme pour
        # crowd_idx / cheer_idx)
        # 3 tailles x (4 dessins, puis leurs 4 « bras levés ») à 24 places
        # consécutives : ola_row lève une tuile en lui ajoutant 4
        base = next((i for i in cand if all(i + j in cand for j in range(24))), None)
        if base is None: raise OverflowError
        for j in range(24):
            cand.remove(base + j)
        for si, sz in enumerate(SIZES):
            ds = SMALL[fam][sz]
            assert len({tkey(t) for t in ds['down']}) == 4 and len({tkey(t) for t in ds['cheer']}) == 4, (fam, sz)
            for d in range(4):
                new[tkey(ds['down'][d])] = (base + si * 8 + d, [list(x) for x in ds['down'][d]])
                new[tkey(ds['cheer'][d])] = (base + si * 8 + 4 + d, [list(x) for x in ds['cheer'][d]])
        ps = [base] + [0] * 11
    for (ty, tx), t in tiles.items():
        k = tkey(t)
        if k in known and known[k] in keep:
            nt[ty * 32 + tx] = known[k]
        else:
            if k not in new:
                if not cand: raise OverflowError
                new[k] = (cand.pop(0), [list(r) for r in t])
            nt[ty * 32 + tx] = new[k][0]
    return nt, new, ps, cand

for e in range(6):
    for PERSP_STEP in (1, 2):                             # rambarde au pixel près si la place suffit
        try:
            nt, new, ps, cand = persp_build(e)
            break
        except OverflowError:
            continue
    else:
        sys.exit('perspective : plus de place pour les tuiles (lieu %d)' % e)
    print('perspective, lieu %d : %d tuiles nouvelles, %d places restantes, rambarde au pas de %d px' % (e, len(new), len(cand), PERSP_STEP))
    persp_rles.append(bytes(nt))   # (RLE à l'écriture)
    persp_tiles.append(sorted(new.values()))
    persp_ps.append(ps if ps else [0] * 12)
proj_tab = bytearray(512)
for Y in range(256):
    d = (216 - Y) / 168
    sc = 1 / (1 + PERSP_A * d)
    proj_tab[Y] = max(0, min(255, round(216 - PERSP_K * d * sc)))
    m = max(-19, min(19, round(sc * 128) - 128))     # échelle x 128 - 128
    # octet tassé pour _pscale : bits 5-7 = (|m| x 32) & $E0, bits 0-1 = |m| / 8
    # (page de la table des produits), bit 2 = signe de m
    proj_tab[256 + Y] = ((abs(m) * 32) & 0xE0) | (abs(m) >> 3) | (4 if m < 0 else 0)
# table de produits : round(v x |m| / 128) pour v = 8 k + 4 (k 0..31) et |m| 0..19 ;
# _pscale y lit la correction au lieu de multiplier (erreur au plus 1 px)
mul_tab = bytearray(20 * 32)
for m in range(20):
    for k in range(32):
        mul_tab[m * 32 + k] = round((k * 8 + 4) * m / 128)
print('perspective : tuiles nouvelles par lieu', [len(t) for t in persp_tiles])

# --------------------------------------------------------------------------
# Sorties
# --------------------------------------------------------------------------
chr_data = bytearray()
for t in bg_tiles:
    chr_data += encode_tile(t if t is not None else blank())
for t in spr_tiles:
    chr_data += encode_tile(t if t is not None else blank())
assert len(chr_data) == 8192
os.makedirs(os.path.join(ROOT, 'assets'), exist_ok=True)
with open(os.path.join(ROOT, 'assets', 'tiles.chr'), 'wb') as f:
    f.write(chr_data)

def rle(data):
    """RLE simple : <octet> ou <MARK, octet, compte>. MARK = octet absent."""
    used = set(data)
    mark = next(i for i in range(256) if i not in used)
    out = bytearray([mark])
    i = 0
    while i < len(data):
        j = i
        while j < len(data) and data[j] == data[i] and j - i < 255:
            j += 1
        n = j - i
        if n >= 3:
            out += bytes([mark, data[i], n])
        else:
            out += data[i:j]
        i = j
    out += bytes([mark, 0, 0])  # MARK, valeur, compte 0 = fin
    return bytes(out)

def c_array(name, data):
    s = 'const unsigned char %s[%d] = {\n' % (name, len(data))
    for i in range(0, len(data), 16):
        s += '  ' + ','.join('0x%02X' % b for b in data[i:i+16]) + ',\n'
    return s + '};\n'

court_rles = [rle(n) for n in court_nts]

src = ['/* Fichier généré par tools/gen_assets.py — ne pas modifier. */',
       '#include "assets.h"', '']
logo = []
for ch in LOGO:
    if ch == ' ':
        logo += [0, 0, 0, 0]
    else:
        logo += logo_tiles[ch]
src.append(c_array('logo_tiles', bytes(logo)))
src.append(c_array('bigball_tiles', bytes(ball_tiles)))
src.append(c_array('person_map_m', bytes(person_map[:2 * NPOSE * 3])))
src.append(c_array('person_map_f', bytes(person_map[2 * NPOSE * 3:])))
src.append(c_array('prof_map', bytes(prof_map)))
src.append(c_array('marg_map', bytes(marg_map)))
src.append('const unsigned char person_row[2] = {0,%d};\n' % (NPOSE * 3))
src.append('#if NPOSE != %d\n#error "NPOSE (assets.h) ne correspond pas a tools/gen_assets.py"\n#endif\n' % NPOSE)
src.append('const unsigned int people_bytes = %d;\n' % ((POOL_END - POOL0) * 16))
src.append('const unsigned char marg_prof_l = 0x%02X;\nconst unsigned char marg_prof_r = 0x%02X;\n' % (MARG_PROF_L, MARG_PROF_R))
src.append('const unsigned char trophy_tile = 0x%02X;\nconst unsigned char confetti_tile = 0x%02X;\nconst unsigned char ball_base = 0x%02X;\n' % (TROPHY_TILE, CONFETTI_TILE, BALL_BASE))
with open(os.path.join(ROOT, 'src', 'assets_data.c'), 'w') as f:
    f.write('\n'.join(src))

# personnages au naturel (banque 6) : remplacent les tuiles $10.. en CHR-RAM
src = ['/* Fichier généré par tools/gen_assets.py — ne pas modifier. */',
       '#include "assets.h"', '']
src.append(c_array('nude_people', bytes(nude_people)))
with open(os.path.join(ROOT, 'src', 'assets_nude.c'), 'w') as f:
    f.write('\n'.join(src))

# données du court et du public : banque du moteur de jeu
src = ['/* Fichier généré par tools/gen_assets.py — ne pas modifier. */',
       '#include "assets.h"', '']
# tables de noms des courts : banque 12 (assets_courts12.c), pointeurs ici
for e in range(6):
    src.append('extern const unsigned char court_nt_rle%d[];' % e)
src.append('/* 6 : le parc reprend le dessin du club (autres couleurs) */\nconst unsigned char * const court_nt_rle[7] = { court_nt_rle0, court_nt_rle1, court_nt_rle2, court_nt_rle3, court_nt_rle4, court_nt_rle5, court_nt_rle1 };\n')
_c12 = ['/* Fichier généré par tools/gen_assets.py — ne pas modifier. */',
        '/* Tables de noms des courts, compressées (banque 12). */', '#include "assets.h"', '']
for e in range(6):
    _c12.append(c_array('court_nt_rle%d' % e, court_rles[e]))
with open(os.path.join(ROOT, 'src', 'assets_courts12.c'), 'w') as f:
    f.write('\n'.join(_c12) + '\n')
src.append(c_array('crowd_idx4', bytes(crowd_idx4)))
src.append(c_array('crowd_norm4', b''.join(encode_tile(t) for t in GIRLY)))
src.append(c_array('crowd_cheer4', b''.join(encode_tile(t) for t in GIRLY_SPARKLE)))
src.append(c_array('crowd_idx3', bytes(crowd_idx3)))
src.append(c_array('crowd_norm3', b''.join(encode_tile(t) for t in NUDE_CROWD)))  # 12 tuiles
src.append(c_array('crowd_cheer3', b''.join(encode_tile(t) for t in NUDE_CHEER)))
src.append(c_array('sign_chr', b''.join(encode_tile(t) for t in SIGN_TILES)))   # panneau du court Margaux
src.append(c_array('sign2_chr', b''.join(encode_tile(t) for t in SIGN2_TILES)))   # panneau du club nudiste
src.append(c_array('sign_idx', bytes(SIGN_IDX)))
src.append(c_array('sign_orig', b''.join(encode_tile(bg_tiles[i]) for i in SIGN_IDX)))
src.append(c_array('sign2_idx', bytes(SIGN2_IDX)))
src.append(c_array('sign2_orig', b''.join(encode_tile(bg_tiles[i]) for i in SIGN2_IDX)))
src.append(c_array('net_idx', bytes(NET_IDX)))
src.append(c_array('net_norm', b''.join(encode_tile(t) for t in net_tiles(False))))
src.append(c_array('net_shk', b''.join(encode_tile(t) for t in net_tiles(True))))
src.append(c_array('ump_chr', b''.join(encode_tile(t) for st in UMP_SETS for t in st)))
src.append(c_array('crowd_idx', bytes(crowd_idx)))
src.append(c_array('cheer_idx', bytes(cheer_idx)))
src.append(c_array('crowdb_idx', bytes(crowdb_idx)))
src.append(c_array('cheerb_idx', bytes(cheerb_idx)))
src.append(c_array('crowdb_norm', b''.join(encode_tile(t) for t in CROWDB)))
src.append(c_array('crowdb_up', b''.join(encode_tile(t) for t in CROWDB_UP)))
src.append(c_array('crowd_up', b''.join(encode_tile(t) for t in CROWD_UP)))
src.append(c_array('crowd_norm', b''.join(encode_tile(t) for t in CROWD)))
src.append(c_array('crowd_cheer', b''.join(encode_tile(t) for t in CHEER)))
# jeu de tuiles par phase : poses de chaque jeu, pose de secours tant que le
# jeu n'est pas chargé, place et taille de la zone partagée
POSE_FALLBACK = {7: 0, 15: 0, 16: 6, 17: 0, 18: 0, 22: 8, 23: 12, 24: 4, 25: 8, 26: 12,
                 28: 9, 29: 27, 30: 21, 31: 27}
src.append(c_array('pose_set', bytes([1 if k in SET_C else 2 if k in SET_R else 3 if k in SET_F else 0 for k in range(NPOSE)])))
src.append(c_array('pose_fb', bytes([POSE_FALLBACK.get(k, k) for k in range(NPOSE)])))
src.append('const unsigned char swap_base = 0x%02X, swap_ntile[3] = { %d, %d, %d };\nconst unsigned char ice_tile = 0x%02X, prop_tile = 0x%02X;\n' % (
    SWAP0, len(swap_sets['C'][0]), len(swap_sets['R'][0]), len(swap_sets['F'][0]), ICE_TILE, PROP0))
# petits spectateurs (vue en perspective) : regard vers le bas / vers le haut
for fam in ('A', 'B'):
    src.append(c_array('small_norm%s' % fam, b''.join(encode_tile(t) for sz in SIZES for t in SMALL[fam][sz]['down'])))
    src.append(c_array('small_up%s' % fam, b''.join(encode_tile(t) for sz in SIZES for t in SMALL[fam][sz]['up'])))
with open(os.path.join(ROOT, 'src', 'assets_court.c'), 'w') as f:
    f.write('\n'.join(src))

# jeux de tuiles par phase (banque 13) : célébrations et revers, normaux et
# au naturel
_sw = ['/* Fichier généré par tools/gen_assets.py — ne pas modifier. */',
       '/* Jeux de tuiles par phase : célébrations (C), revers (R) et séquences (F), banque 13. */', '#include "assets.h"', '']
for cls in ('C', 'R', 'F'):
    tiles, nude = swap_sets[cls]
    _sw.append(c_array('swap_chr%s' % cls, b''.join(encode_tile(t) for t in tiles)))
    _sw.append(c_array('swap_nude%s' % cls, b''.join(encode_tile(t) for t in nude)))
_sw.append('/* accessoires des séquences (cer.c, même banque) : chaise, gourde, serviette, raquette */\nconst unsigned char prop_tile13 = 0x%02X;' % PROP0)
# bustes « bras caché » pour la vue de dos (version 2.52) : les poses dont un
# bras passe devant le buste ; 2 tuiles (rangée du buste) par pose et par
# silhouette, chargées à la volée dans 2 cases (une par joueur vu de dos) à
# la fin de la zone partagée, libre pendant l'échange (jeu R : 28 tuiles)
def _row1(view, legs, arms, fem, hide):
    global HIDE_ARM
    HIDE_ARM = hide
    t = canvas_tiles(draw_person(view, legs, arms, fem))
    HIDE_ARM = False
    return (t[2], t[3])
XP = [k for k, (legs, arms) in enumerate(POSES) if legs not in ('dive', 'supine')
      and any(_row1('back', legs, arms, f, True) != _row1('back', legs, arms, f, False) for f in (0, 1))]
SLOT0 = SWAP0 + len(swap_sets['R'][0])
assert NSWAP - len(swap_sets['R'][0]) >= 4, 'pas de place pour les cases des bustes'
print('bustes vus de dos : poses', XP, ', cases', hex(SLOT0))
_x = []
for k in XP:
    for f in (0, 1):
        _x += list(_row1('back', POSES[k][0], POSES[k][1], f, True))
_sw.append(c_array('xrow1', b''.join(encode_tile(t) for t in _x)))
_sw.append(c_array('xpose_idx', bytes([XP.index(k) if k in XP else 255 for k in range(NPOSE)])))
_sw.append('const unsigned char slot_tile13 = 0x%02X;' % SLOT0)
with open(os.path.join(ROOT, 'src', 'assets_swap.c'), 'w') as f:
    f.write('\n'.join(_sw) + '\n')

# vue en perspective : courts déformés et tables de projection (banque 11) ;
# places des tuiles et pointeurs (banque 3, lus par load_court)
src = ['/* Fichier généré par tools/gen_assets.py — ne pas modifier. */',
       '/* Vue en perspective : courts déformés, tuiles et tables de projection (banque 11). */',
       '#include "assets.h"', '']
for e in range(6):
    src.append(c_array('persp_rle%d' % e, rle(persp_rles[e])))
    src.append(c_array('persp_chr%d' % e, b''.join(encode_tile(t) for _, t in persp_tiles[e])))
for e in range(6):
    src.append(c_array('persp_slot%d' % e, bytes([k for k, _ in persp_tiles[e]])))
    src.append(c_array('persp_ps%d' % e, bytes(persp_ps[e])))   # petits spectateurs : 6 places, puis 6 « bras levés »
src.append(c_array('proj_tab', bytes(proj_tab) + bytes(mul_tab)))
with open(os.path.join(ROOT, 'src', 'assets_persp.c'), 'w') as f:
    f.write('\n'.join(src) + '\n')
src = ['/* Fichier généré par tools/gen_assets.py — ne pas modifier. */',
       '/* Vue en perspective : places des tuiles et pointeurs vers la banque 11 (banque 3). */',
       '#include "assets.h"', '']
for e in range(6):
    src.append('extern const unsigned char persp_rle%d[], persp_chr%d[], persp_slot%d[], persp_ps%d[];' % (e, e, e, e))
src.append('/* 6 : le parc reprend le court du club */')
src.append('const unsigned char * const persp_rle[7] = { %s, persp_rle1 };' % ', '.join('persp_rle%d' % e for e in range(6)))
src.append('const unsigned char * const persp_chr[7] = { %s, persp_chr1 };' % ', '.join('persp_chr%d' % e for e in range(6)))
src.append('const unsigned char * const persp_slot[7] = { %s, persp_slot1 };' % ', '.join('persp_slot%d' % e for e in range(6)))
src.append('const unsigned char * const persp_ps[7] = { %s, persp_ps1 };' % ', '.join('persp_ps%d' % e for e in range(6)))
src.append('const unsigned char persp_ntile[7] = { %s, %d };' % (', '.join(str(len(t)) for t in persp_tiles), len(persp_tiles[1])))
with open(os.path.join(ROOT, 'src', 'assets_persp3.c'), 'w') as f:
    f.write('\n'.join(src) + '\n')
court_rle = court_rles[0]

used_bg = sum(1 for t in bg_tiles if t is not None)
print("tuiles BG utilisées : %d/256, sprites : %d/256, courts RLE : %s octets" % (
    used_bg, next_spr, [len(r) for r in court_rles]))

# aperçu facultatif
try:
    from PIL import Image
    NESPAL = {0: (200, 120, 60), 1: (230, 150, 90), 2: (255, 255, 255), 3: (30, 30, 30)}
    img = Image.new('RGB', (256, 240))
    for ty in range(30):
        for tx in range(32):
            t = bg_tiles[court_nt[ty*32+tx]]
            for y in range(8):
                for x in range(8):
                    img.putpixel((tx*8+x, ty*8+y), NESPAL[t[y][x]])
    img.save(os.path.join(ROOT, 'assets', 'preview_court.png'))
    for e in (1, 2, 3, 4, 5):
        im2 = Image.new('RGB', (256, 240))
        for ty in range(30):
            for tx in range(32):
                t = bg_tiles[court_nts[e][ty*32+tx]]
                for y in range(8):
                    for x in range(8):
                        im2.putpixel((tx*8+x, ty*8+y), NESPAL[t[y][x]])
        im2.save(os.path.join(ROOT, 'assets', 'preview_court%d.png' % e))
    pal = {0: (0, 120, 0), 1: (250, 190, 150), 2: (220, 40, 40), 3: (20, 20, 60)}
    sp = Image.new('RGB', (16*NPOSE*2+2*NPOSE*2, 24*2+2))
    for i, cv in enumerate(preview_people):
        ox = (i % (NPOSE*2)) * 18; oy = (i // (NPOSE*2)) * 26
        for y in range(24):
            for x in range(16):
                if oy+y < sp.size[1] and ox+x < sp.size[0]:
                    sp.putpixel((ox+x, oy+y), pal[cv[y][x]])
    sp = sp.resize((sp.size[0]*4, sp.size[1]*4), Image.NEAREST)
    sp.save(os.path.join(ROOT, 'assets', 'preview_sprites.png'))
except ImportError:
    pass
