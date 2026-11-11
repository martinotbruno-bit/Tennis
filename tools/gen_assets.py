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
    for px in (DBL_L - 8, DBL_R + 6):                        # poteaux
        rect(px, NET_Y - 11, px + 2, NET_Y + 1, 3)
        rect(px, NET_Y - 10, px, NET_Y, 2)                   # reflet
        rect(px, NET_Y - 11, px + 2, NET_Y - 11, 2)          # tête

# Chaise d'arbitre classique, dessinée dans le décor de chaque court, au
# niveau du filet : 16 x 48 px, x 16-31, y 112-159 (3 blocs d'attributs en
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
CHAIR_X, CHAIR_Y = 16, 112      # centrée sur le filet (bande du filet : y 125-132)

CLUB_ROWS = (6, 7, 12, 13, 20, 21, 26, 27)     # bancs de spectateurs du club (rangées)

def draw_court(env=0):
    W, H = 256, 240
    b = [[0]*W for _ in range(H)]
    def rect(x0, y0, x1, y1, c):
        for y in range(max(0,y0), min(H,y1+1)):
            for x in range(max(0,x0), min(W,x1+1)):
                b[y][x] = c
    # surface de jeu (couleur 1) avec une marge de dégagement
    rect(DBL_L-10, TOP_BASE-14, DBL_R+10, BOT_BASE+14, 1)
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

# Spectateurs : un par tuile, assis en rangées. Tête 4 x 4 : de face
# (regard vers le bas du court) chevelure puis visage avec deux yeux ; de dos
# (regard vers le haut) chevelure entière et nuque. Épaules et buste en
# dessous. Le jeu échange les deux versions quand la balle change de moitié :
# toute la tribune tourne la tête. Bras levés : ola.
# Jeu A (stade, palette des tribunes) : fond 2 (gradins), peau 1, yeux 2,
# cheveux 3, maillots 3 ou 1. Jeu B (club, salle ; palette 3 noir, jaune,
# blanc) : fond 0, peau 2, yeux et cheveux 1, maillots 3 ou 1.
def spectators(seed, view, b=False):
    r = random.Random(seed)
    bg, skin, eye, hair = (0, 2, 1, 1) if b else (2, 1, 2, 3)
    t = [[bg] * 8 for _ in range(8)]
    ox = 2 + r.choice((-1, 0, 0, 1))              # léger décalage d'une place à l'autre
    shirt = r.choice((3, 3, 1))
    if view == 'up':
        head = ((hair,) * 4, (hair,) * 4, (hair,) * 4, (skin,) * 4)
    else:
        head = ((hair,) * 4, (skin, eye, eye, skin), (skin,) * 4, (bg, skin, skin, bg))
    for k in range(4):
        for xx in range(4):
            t[1 + k][ox + xx] = head[k][xx]
    for yy in range(5, 8):                           # épaules et buste
        for xx in range(ox - 1, ox + 5):
            if 0 <= xx < 8:
                t[yy][xx] = shirt
    if view == 'cheer':                              # bras levés de part et d'autre de la tête
        for yy in range(1, 5):
            if ox - 1 >= 0: t[yy][ox - 1] = skin
            if ox + 4 < 8: t[yy][ox + 4] = skin
    return t

CROWD = [spectators(i, 'down') for i in range(6)]
CROWD_UP = [spectators(i, 'up') for i in range(6)]
CHEER = [spectators(i, 'cheer') for i in range(6)]
CROWDB = [spectators(i, 'down', True) for i in range(6)]
CROWDB_UP = [spectators(i, 'up', True) for i in range(6)]
CHEERB = [spectators(i, 'cheer', True) for i in range(6)]

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
    if bx < 2 or bx >= 14:
        return 2                      # tribunes
    return 0

court_nts = [bitmap_to_nametable(draw_court(e), lambda bx, by, e=e: court_attr(bx, by, e)) for e in range(5)]
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
for ch in set(LOGO.replace(' ', '')):
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
    rain[y][4 - y//3] = 3
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
    spl1[y][x] = 1
spr_tile(8, spl1)
spl2 = blank()
for (x, y) in ((1, 2), (6, 2), (0, 4), (7, 4), (2, 6), (5, 6), (3, 3), (4, 3)):
    spl2[y][x] = 1
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
sweat[3][2] = 1
spr_tile(14, sweat)

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

def draw_person(view, legs, arms, female, nude=False):
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
            fill(c, 7, 6, 8, 8, 3)  # queue de cheval
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
    elif legs == 'lunge':
        # fente : jambe droite tendue loin sur le côté, jambe gauche fléchie
        fill(c, 5, 16, 6, 20, 1); fill(c, 4, 21, 5, 21, 1)
        line(c, 9, 16, 13, 21, 1); line(c, 10, 16, 14, 21, 1)
        fill(c, 3, 22, 5, 23, 3); fill(c, 13, 22, 15, 23, 3)
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
    elif arms == 'reach':
        # balle lointaine : bras tendu au maximum, tamis vers le bas, l'autre
        # bras en arrière pour l'équilibre
        line(c, 4, 9, 1, 11, 1)
        line(c, 11, 9, 14, 11, 1)
        racket(c, 14, 12, 13, 17)
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
]
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
def pool_rows(cv, nv):
    global next_spr
    t, tn = canvas_tiles(cv), canvas_tiles(nv)
    out = []
    for r in range(3):
        key = tuple(tuple(map(tuple, x)) for x in (t[2*r], t[2*r+1], tn[2*r], tn[2*r+1]))
        if key not in pool:
            pool[key] = next_spr
            spr_tile(next_spr, t[2*r]); spr_tile(next_spr + 1, t[2*r+1])
            nude_people.extend(encode_tile(tn[2*r])); nude_people.extend(encode_tile(tn[2*r+1]))
            next_spr += 2
        out.append(pool[key])
    return out
person_map = []
preview_people = []
for female in (0, 1):
    for view in ('back', 'front'):
        for (legs, arms) in POSES:
            cv = draw_person(view, legs, arms, female)
            preview_people.append(cv)
            person_map += pool_rows(cv, draw_person(view, legs, arms, female, nude=True))
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
    else:
        # jambes croisées (passage), bras le long du corps
        line(c, 6, 8, 5, 12, 1)
        line(c, 9, 8, 10, 12, 1)
        racket(c, 10, 13, 12, 18, 5)
        fill(c, 7, 16, 8, 21, 1)
        line(c, 7, 16, 5, 20, 1)
        fill(c, 7, 22, 9, 23, 1 if nude else 3); put(c, 4, 21, 1 if nude else 3)
    return c

prof_map = []
for female in (0, 1):
    for frame in (0, 1):
        cv = draw_profile(frame, female)
        preview_people.append(cv)
        prof_map += pool_rows(cv, draw_profile(frame, female, nude=True))
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
# balle moyenne (5x5) et grande (7x7) : la taille suit la hauteur de la balle
BALL_MID_TILE = next_spr
bm = blank()
for y in range(5):
    for x in range(5):
        if (x - 2) ** 2 + (y - 2) ** 2 <= 5:
            bm[y + 1][x + 1] = 1
bm[2][2] = 3
spr_tile(next_spr, bm); next_spr += 1
BALL_BIG_TILE = next_spr
bb = blank()
for y in range(7):
    for x in range(7):
        if (x - 3) ** 2 + (y - 3) ** 2 <= 10:
            bb[y][x] = 1
bb[1][2] = 3; bb[2][1] = 3; bb[2][2] = 3
spr_tile(next_spr, bb); next_spr += 1
CONFETTI_TILE = next_spr
conf = blank()
for (x, y) in ((3, 3), (4, 3), (3, 4), (4, 4)):
    conf[y][x] = 1
conf[3][4] = 3
spr_tile(next_spr, conf); next_spr += 1

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
marg_map = []
for view in ('back', 'front'):
    for (legs, arms) in POSES:
        cv = margaux_hair(draw_person(view, legs, arms, 1), view)
        t = canvas_tiles(cv)
        key = (tuple(map(tuple, t[0])), tuple(map(tuple, t[1])))
        if key not in marg_pool:
            marg_pool[key] = next_spr
            marg_previews.append(cv)
            spr_tile(next_spr, t[0]); spr_tile(next_spr + 1, t[1]); next_spr += 2
        marg_map.append(marg_pool[key])
pv = margaux_profile_hair(draw_profile(0, 1))
marg_previews.append(pv)
tp = canvas_tiles(pv)
MARG_PROF_L, MARG_PROF_R = 0x0F, next_spr
spr_tile(MARG_PROF_L, tp[0]); spr_tile(MARG_PROF_R, tp[1]); next_spr += 1
assert next_spr <= 256, "trop de tuiles de sprites (Margaux)"

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
src.append(c_array('person_map', bytes(person_map)))
src.append(c_array('prof_map', bytes(prof_map)))
src.append(c_array('marg_map', bytes(marg_map)))
src.append('const unsigned char person_row[4] = {%d,%d,%d,%d};\n' % tuple(k * NPOSE * 3 for k in range(4)))
src.append('const unsigned int people_bytes = %d;\n' % ((POOL_END - POOL0) * 16))
src.append('const unsigned char marg_prof_l = 0x%02X;\nconst unsigned char marg_prof_r = 0x%02X;\n' % (MARG_PROF_L, MARG_PROF_R))
src.append('const unsigned char trophy_tile = 0x%02X;\nconst unsigned char confetti_tile = 0x%02X;\nconst unsigned char ball_mid_tile = 0x%02X;\nconst unsigned char ball_big_tile = 0x%02X;\n' % (TROPHY_TILE, CONFETTI_TILE, BALL_MID_TILE, BALL_BIG_TILE))
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
for e in range(5):
    src.append(c_array('court_nt_rle%d' % e, court_rles[e]))
src.append('const unsigned char * const court_nt_rle[5] = { court_nt_rle0, court_nt_rle1, court_nt_rle2, court_nt_rle3, court_nt_rle4 };\n')
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
with open(os.path.join(ROOT, 'src', 'assets_court.c'), 'w') as f:
    f.write('\n'.join(src))
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
    for e in (1, 2, 3, 4):
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
