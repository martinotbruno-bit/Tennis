#!/usr/bin/env python3
"""Royal Martinot — portraits des joueurs de l'écran RÉGLAGES (version 2.58).

Tête et épaules dessinées au pixel, 32 x 24 (4 x 3 sprites), aux couleurs
du joueur (palette de sprite : 1 peau, 2 tenue, 3 sombre ; 0 transparent) :
homme (polo), femme (débardeur, queue de cheval), fillette (Margaux :
cheveux longs bouclés). Le portrait de droite est retourné.
Sortie : src/assets_port.c (banque 13) et assets/preview_portraits.png."""
import os
import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
S, O, D = 1, 2, 3
W, H = 32, 24


def ell(g, cx, cy, rx, ry, v, ymax=99):
    for y in range(H):
        for x in range(W):
            if ((x + 0.5 - cx) / rx) ** 2 + ((y + 0.5 - cy) / ry) ** 2 <= 1 and y <= ymax:
                g[y, x] = v


def outline(g):
    """contour sombre autour de la peau et de la tenue"""
    o = g.copy()
    for y in range(H):
        for x in range(W):
            if g[y, x] == 0:
                for dy, dx in ((0, 1), (0, -1), (1, 0), (-1, 0)):
                    yy, xx = y + dy, x + dx
                    if 0 <= yy < H and 0 <= xx < W and g[yy, xx] in (S, O):
                        o[y, x] = D
                        break
    return o


def make(kind, light=False):
    """kind : 0 homme, 1 femme, 2 fillette ; light : cheveux clairs (Margaux,
    Papi), dessinés dans la couleur 2 (la tenue prend la même couleur), la
    couleur 3 restant noire pour les contours et les yeux"""
    g = np.zeros((H, W), np.uint8)
    if kind == 0:
        for y, w in zip(range(17, 24), (4, 8, 11, 13, 14, 15, 15)):      # polo : épaules
            g[y, 16 - w:16 + w] = O
        g[15:19, 14:18] = S                                                 # cou
        g[18:20, 15:17] = S                                                 # col ouvert
    else:
        for y in range(19, 24):                                             # épaules nues
            w = 8 + min(y - 19, 2) * 2 + (2 if kind == 1 else 0)
            g[y, 16 - w:16 + w] = S
        g[15:20, 14:18] = S
        g[20:24, 12:20] = O                                                 # débardeur
        if kind == 1:
            g[21:24, 11:21] = O
        g[19, 12] = g[19, 19] = O                                           # bretelles
        g[20, 14:18] = S                                                    # encolure
    ell(g, 16, 9.5, 5.6 if kind == 0 else 5.2, 6.6 if kind == 0 else 6.2, S)   # visage
    g[9:11, 10] = S; g[9:11, 21] = S                                        # oreilles
    g = outline(g)
    outline_mask = g == D
    if kind == 0:                                                           # cheveux courts
        ell(g, 16, 6.0, 6.4, 4.6, D, ymax=5)
        for x in range(10, 22):
            if x not in (13, 17):
                g[6, x] = D                                                 # frange
        g[7:9, 10] = D; g[7:9, 21] = D                                      # pattes
        g[0, 15] = 0
    else:
        ell(g, 16, 6.4, 6.4, 5.2, D, ymax=6)
        for x in (10, 11, 12, 13, 14, 19, 20, 21):
            g[7, x] = D                                                     # frange de côté
        lo = 13 if kind == 1 else 15
        g[7:lo + 1, 9:11] = D; g[7:lo + 1, 21:23] = D                       # cheveux qui encadrent le visage
        if kind == 1:
            ell(g, 23.5, 7.5, 1.8, 3.6, D)                                  # queue de cheval
        else:
            for y in range(8, lo + 2, 2):
                g[y, 8] = D; g[y, 23] = D                                   # boucles
    if light:
        hair = (g == D) & ~outline_mask
        g[hair] = O
    g[8, 12:14] = D; g[8, 18:20] = D                                        # sourcils
    g[10, 13] = D; g[10, 18] = D                                            # yeux
    if kind:
        g[9, 12] = D; g[9, 19] = D                                          # cils
    if kind == 0:
        g[13, 14:17] = D                                                    # bouche
    else:
        g[13, 15:17] = D
    return g


def tiles(g):
    out = b''
    for ty in range(3):
        for tx in range(4):
            for plane in (0, 1):
                for y in range(8):
                    v = 0
                    for x in range(8):
                        v = (v << 1) | ((int(g[ty * 8 + y, tx * 8 + x]) >> plane) & 1)
                    out += bytes([v])
    return out


grids = [make(0), make(1), make(2, True), make(0, True)]     # homme, femme, Margaux, Papi
data = b''.join(tiles(g) for g in grids)

# vignettes des lieux (16 x 8, 2 sprites ; palette de sprite 3 : 1 blanc,
# 2 couleur du lieu, 3 or), tuiles $40-$4D
ICONS = [
    # stade (2 : vert)
    ["................", "..333333333333..", ".33222222222233.", "3322222112222233",
     "3322222112222233", ".33222222222233.", "..333333333333..", "................"],
    # club (vert) : arbres et clôture
    ["..22......22....", ".2222....2222...", "222222..222222..", ".2222....2222...",
     "..33......33....", "1111111111111111", "1.1.1.1.1.1.1.1.", "................"],
    # salle (gris) : toit en voûte, fenêtres, porte
    ["....22222222....", "..222222222222..", ".22222222222222.", ".21121121121122.",
     ".22222222222222.", ".22222233222222.", ".22222233222222.", "................"],
    # club nudiste (rouge) : parasol rayé et serviette
    ["...2121212121...", "..212121212121..", ".21212121212121.", ".......3........",
     ".......3........", ".......3........", "..1111111111....", "................"],
    # court Margaux (rose) : cœur et étoile
    [".22.22.......3..", "2222222.....333.", "2122222......3..", "2222222.........",
     ".22222.....3....", "..222.....333...", "...2.......3....", "................"],
    # château (gris) : tours, créneaux, drapeaux
    [".3...........3..", ".33..........33.", ".2...........2..", "2.2.2.2.2.2.2.2.",
     "222222222222222.", "222112222211222.", "222222233222222.", "222222233222222."],
    # parc (orange) : arbres d'automne et allée
    ["..22......22....", ".2222....2222...", "222222..222222..", ".2222....2222...",
     "..33......33....", "..33......33....", "1111111111111111", "................"],
]
for rows in ICONS:
    assert len(rows) == 8 and all(len(r) == 16 for r in rows)
    g = np.array([[0 if ch == '.' else int(ch) for ch in r] for r in rows], np.uint8)
    for tx in range(2):
        for plane in (0, 1):
            for y in range(8):
                v = 0
                for x in range(8):
                    v = (v << 1) | ((int(g[y, tx * 8 + x]) >> plane) & 1)
                data += bytes([v])
L = ['/* Fichier généré par tools/gen_port.py — ne pas modifier. */',
     '/* Portraits des réglages : homme, femme, Margaux, Papi (12 tuiles de sprites chacun), puis vignettes des 7 lieux (2 tuiles), banque 13. */', '',
     'const unsigned char port_chr[%d] = {' % len(data)]
for i in range(0, len(data), 16):
    L.append('  ' + ','.join('0x%02X' % v for v in data[i:i + 16]) + ',')
L += ['};', '']
open(os.path.join(ROOT, 'src', 'assets_port.c'), 'w').write('\n'.join(L))
COL = {1: (240, 188, 60), 2: (60, 90, 200), 3: (20, 20, 20)}
im = Image.new('RGB', (4 * 36, H), (0, 100, 0))
for i, g in enumerate(grids):
    for y in range(H):
        for x in range(W):
            if g[y, x]:
                c = COL[int(g[y, x])]
                if i >= 2:
                    c = {1: COL[1], 2: (248, 216, 120) if i == 2 else (230, 230, 230), 3: (0, 0, 0)}[int(g[y, x])]
                im.putpixel((i * 36 + x, y), c)
im.resize((im.width * 6, im.height * 6), Image.NEAREST).save(os.path.join(ROOT, 'assets', 'preview_portraits.png'))
print('portraits : %d octets' % len(data))
