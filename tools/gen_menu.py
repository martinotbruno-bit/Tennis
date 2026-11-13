#!/usr/bin/env python3
"""Royal Martinot — habillage des menus (version 2.57) : variante surlignée de
la police et ornements dorés, chargés en CHR-RAM (tuiles du décor $60-$A7,
libres pendant les menus) par menuart.c.

  $60-$9F : police surlignée (le texte, couleur 3, passe en couleur 2 ;
            l'ombre reste en couleur 1), pour la ligne choisie d'un menu ;
  $A0-$A7 : filet doré en relief, ornement en losange, extrémités, étoile.

Sortie : src/assets_menu.c (banque 8) et assets/preview_menu.png."""
import os
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
chr_ = open(os.path.join(ROOT, 'assets', 'tiles.chr'), 'rb').read()

# police surlignée : couleur 3 (plans 0 et 1) -> couleur 2 (plan 1 seul)
font2 = bytearray()
for t in range(0x20, 0x60):
    p0 = chr_[t * 16:t * 16 + 8]
    p1 = chr_[t * 16 + 8:t * 16 + 16]
    font2 += bytes(a & ~b & 255 for a, b in zip(p0, p1)) + bytes(p1)


def tile(rows):
    """8 chaînes de 8 caractères '0'-'3' -> 16 octets"""
    p0, p1 = [], []
    for r in rows:
        v0 = v1 = 0
        for ch in r:
            c = int(ch)
            v0 = (v0 << 1) | (c & 1)
            v1 = (v1 << 1) | (c >> 1)
        p0.append(v0); p1.append(v1)
    return bytes(p0 + p1)


DECO = [
    # $A0 filet doré en relief : reflet (2), or (3), ombre (1)
    ["00000000", "00000000", "00000000", "22222222", "33333333", "11111111", "00000000", "00000000"],
    # $A1 losange au milieu du filet
    ["00000000", "00033000", "00322300", "23333332", "33333333", "11333311", "00133100", "00011000"],
    # $A2 extrémité gauche (pointe)
    ["00000000", "00000000", "00000000", "00000222", "00233333", "00001111", "00000000", "00000000"],
    # $A3 extrémité droite
    ["00000000", "00000000", "00000000", "22200000", "33333200", "11110000", "00000000", "00000000"],
    # $A4 étoile (ornement de coin)
    ["00030000", "00030000", "02333200", "33333330", "01333100", "00313000", "03101300", "01000100"],
    # $A5 petit point doré (séparateur)
    ["00000000", "00000000", "00000000", "00023000", "00033100", "00011000", "00000000", "00000000"],
    # $A6 barre de sélection : bas souligné (reflet)
    ["00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "22222222", "00000000"],
    # $A7 case pleine (or)
    ["33333333"] * 8,
]
deco = b''.join(tile(r) for r in DECO)


def arr(name, data):
    lines = ['const unsigned char %s[%d] = {' % (name, len(data))]
    for i in range(0, len(data), 16):
        lines.append('  ' + ','.join('0x%02X' % v for v in data[i:i + 16]) + ',')
    lines.append('};')
    return lines


L = ['/* Fichier généré par tools/gen_menu.py — ne pas modifier. */',
     '/* Habillage des menus : police surlignée ($60-$9F) et ornements dorés ($A0-$A7), banque 8. */', '']
L += arr('menu_font2', font2) + [''] + arr('menu_deco', deco) + ['']
open(os.path.join(ROOT, 'src', 'assets_menu.c'), 'w').write('\n'.join(L))

# aperçu : texte normal et surligné, ornements
PAL = {0: (0, 30, 116), 1: (0, 0, 0), 2: (236, 238, 236), 3: (228, 196, 144)}
GOLD = {0: (0, 30, 116), 1: (120, 60, 0), 2: (228, 196, 144), 3: (212, 136, 32)}
im = Image.new('RGB', (8 * 16, 8 * 6), PAL[0])
px = im.load()


def put(t, data, x0, y0, pal):
    for y in range(8):
        for x in range(8):
            c = ((data[t * 16 + y] >> (7 - x)) & 1) | (((data[t * 16 + 8 + y] >> (7 - x)) & 1) << 1)
            if c:
                px[x0 + x, y0 + y] = pal[c]


for i, ch in enumerate('CONFIGURATION'):
    put(ord(ch), chr_, 8 * i, 0, {1: (0, 0, 0), 2: (255, 255, 255), 3: (168, 204, 236)})
    put(ord(ch) - 0x20, font2, 8 * i, 10, {1: (0, 0, 0), 2: (255, 255, 255), 3: (168, 204, 236)})
for i in range(8):
    put(i, deco, 8 * i, 24, GOLD)
im.resize((im.width * 4, im.height * 4), Image.NEAREST).save(os.path.join(ROOT, 'assets', 'preview_menu.png'))
print('menu : police surlignée %d octets, ornements %d octets' % (len(font2), len(deco)))
