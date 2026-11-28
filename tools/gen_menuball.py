#!/usr/bin/env python3
"""Royal Martinot — balle de tennis en fond de l'écran des réglages (2.96).

Grande balle en bas à droite, coupée par le bord de l'écran, tramée (matrice
de Bayer 4 x 4 ; relief : 12, 25 ou 38 % selon la case, lumière en haut à
gauche, bord sombre à 50 % côté ombre) dans la couleur 1 des palettes du menu (noir :
la seule plus sombre que le fond, et commune à toutes les palettes du texte),
coutures en arcs « ) ( » laissées dans la couleur du fond. Une tuile par case
de l'écran (rangées 3 à 29) ; les lettres du menu masquent la balle dans
leur case, et le code remplace les espaces du texte par la tuile de la balle.

Tuiles rangées dans des places libres pendant l'écran des réglages : logo
de l'écran titre ($CE-$FB), traits et accolades ($A5-$B1), fin des lettres
du titre ($CA-$CD), puis tuiles vides de la police (caractères jamais
écrits) ; tout est rechargé en quittant l'écran (bg_dirty = 1).

Écrit src/assets_mball.h (lu par extra2.c)."""
import math, os
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CX, CY, R = 176, 150, 96
SEAM_W = 0.035
BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]
ROW0, ROW1 = 3, 30
# (2.96 : aussi les traits dorés retirés $A0-$A3, et « # », « @ », « = »
# et leurs versions surlignées, jamais écrits sur cet écran : mode
# d'emploi, intro et générique seulement)
SLOTS = (list(range(0xCE, 0xFC)) + list(range(0xA5, 0xB2)) + list(range(0xCA, 0xCE)) + list(range(0xA0, 0xA4)) +
         [0x22, 0x24, 0x26, 0x3B, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x62, 0x64, 0x66, 0x7B, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
          0x23, 0x40, 0x3D, 0x60, 0x63, 0x80, 0x7D])

LEVELS = (0.12, 0.25, 0.38)

def level(cx, cy):
    """relief : une densité par case (lumière en haut à gauche), 3 niveaux"""
    dx, dy = ((cx * 8 + 4) - CX) / R, ((cy * 8 + 4) - CY) / R
    z = math.sqrt(max(0.0, 1 - dx * dx - dy * dy))
    l = -dx * 0.5 - dy * 0.6 + z * 0.62
    d = 0.12 + 0.33 * (1 - max(0.0, min(1.0, (l + 0.2) / 1.1)))
    return min(LEVELS, key=lambda q: abs(q - d))

def pix(x, y, dens):
    dx, dy = (x - CX) / R, (y - CY) / R
    r2 = dx * dx + dy * dy
    if r2 >= 1:
        return 0
    if r2 > 0.9 and dx + dy > 0.3:       # bord sombre côté ombre (en bas à droite)
        dens = 0.5
    for sgn in (-1, 1):
        if abs(math.hypot(dx - sgn * 1.35, dy) - 1.0) < SEAM_W:
            return 0
    return 1 if BAYER[y % 4][x % 4] < dens * 16 else 0

tiles, tmap, grid = [], {}, []
for cy in range(ROW0, ROW1):
    row = []
    for cx in range(32):
        dl = level(cx, cy)
        t = tuple(pix(cx * 8 + i, cy * 8 + j, dl) for j in range(8) for i in range(8))
        if not any(t):
            row.append(0); continue
        if t not in tmap:
            assert len(tiles) < len(SLOTS), 'plus de place pour les tuiles de la balle'
            tmap[t] = SLOTS[len(tiles)]
            tiles.append(t)
        row.append(tmap[t])
    grid.append(row)

def chr_bytes(t):
    lo = [sum(t[j * 8 + i] << (7 - i) for i in range(8)) for j in range(8)]
    return lo + [0] * 8            # couleur 1 : plan bas seulement

out = ['/* Fichier généré par tools/gen_menuball.py — ne pas modifier. */',
       '/* Balle de tennis en fond de l\'écran des réglages (extra2.c). */',
       '#define MB_ROW0 %d' % ROW0, '#define MB_ROWS %d' % (ROW1 - ROW0), '#define MB_NT %d' % len(tiles)]
out.append('static const u8 mb_id[MB_NT] = { %s };' % ', '.join('0x%02X' % SLOTS[i] for i in range(len(tiles))))
data = []
for t in tiles: data += chr_bytes(t)
out.append('static const u8 mb_chr[%d] = {' % len(data))
for i in range(0, len(data), 16):
    out.append('    ' + ', '.join('0x%02X' % v for v in data[i:i + 16]) + ',')
out.append('};')
# carte : par rangée, première colonne non vide, nombre de cases, puis les tuiles
mp, first = [], []
for row in grid:
    nz = [i for i, v in enumerate(row) if v]
    if not nz:
        mp += [0, 0]; continue
    a, b = nz[0], nz[-1] + 1
    mp += [a, b - a] + row[a:b]
offs, o = [], 0
for row in grid:
    offs.append(o); o += 2 + (mp[o + 1])
out.append('static const u16 mb_off[MB_ROWS] = { %s };   /* début de chaque rangée dans mb_map */' % ', '.join(str(v) for v in offs))
out.append('static const u8 mb_map[%d] = {' % len(mp))
for i in range(0, len(mp), 16):
    out.append('    ' + ', '.join('%d' % v for v in mp[i:i + 16]) + ',')
out.append('};')
open(os.path.join(ROOT, 'src', 'assets_mball.h'), 'w').write('\n'.join(out) + '\n')
print('balle du menu : %d tuiles (places %d), carte %d octets' % (len(tiles), len(SLOTS), len(mp)))
