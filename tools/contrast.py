#!/usr/bin/env python3
"""Contrôle de lisibilité : écart de couleur perçu (CIE76 ΔE, palette NES 2C02)
entre les couleurs des personnages et celles des courts (surface et abords),
de jour, de nuit et sous la pluie. Usage : python3 tools/contrast.py"""
import re, os, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# palette NES 2C02 (valeurs couramment utilisées)
PAL = [
 (84,84,84),(0,30,116),(8,16,144),(48,0,136),(68,0,100),(92,0,48),(84,4,0),(60,24,0),
 (32,42,0),(8,58,0),(0,64,0),(0,60,0),(0,50,60),(0,0,0),(0,0,0),(0,0,0),
 (152,150,152),(8,76,196),(48,50,236),(92,30,228),(136,20,176),(160,20,100),(152,34,32),(120,60,0),
 (84,90,0),(40,114,0),(8,124,0),(0,118,40),(0,102,120),(0,0,0),(0,0,0),(0,0,0),
 (236,238,236),(76,154,236),(120,124,236),(176,98,236),(228,84,236),(236,88,180),(236,106,100),(212,136,32),
 (160,170,0),(116,196,0),(76,208,32),(56,204,108),(56,180,204),(60,60,60),(0,0,0),(0,0,0),
 (236,238,236),(168,204,236),(188,188,236),(212,178,236),(236,174,236),(236,174,212),(236,180,176),(228,196,144),
 (204,210,120),(180,222,120),(168,226,144),(152,226,180),(160,214,228),(160,162,160),(0,0,0),(0,0,0)]
def lab(c):
    r, g, b = [v / 255 for v in PAL[c & 0x3F]]
    f = lambda u: ((u + 0.055) / 1.055) ** 2.4 if u > 0.04045 else u / 12.92
    r, g, b = f(r), f(g), f(b)
    X = (0.4124*r + 0.3576*g + 0.1805*b) / 0.95047
    Y = (0.2126*r + 0.7152*g + 0.0722*b)
    Z = (0.0193*r + 0.1192*g + 0.9505*b) / 1.08883
    t = lambda u: u ** (1/3) if u > 0.008856 else 7.787*u + 16/116
    return (116*t(Y) - 16, 500*(t(X) - t(Y)), 200*(t(Y) - t(Z)))
def dE(a, b):
    A, B = lab(a), lab(b)
    return sum((x - y) ** 2 for x, y in zip(A, B)) ** 0.5
def darken(c):          # même règle que gfx.c
    if c in (0x0F, 0x30): return c
    return c - 0x10 if c >= 0x10 else c

src = open(os.path.join(ROOT, 'src', 'gfx.c')).read()
names = re.findall(r'"(\w+) ?"', src[src.index('char_name'):src.index('char_female')])
cp = [tuple(int(x, 16) for x in m) for m in re.findall(r'\{ 0x(\w\w), 0x(\w\w), 0x(\w\w) \}', src[src.index('char_pal'):src.index('style_name')])]
sp = [tuple(int(x, 16) for x in m) for m in re.findall(r'\{ 0x(\w\w), 0x(\w\w), 0x(\w\w), 0x(\w\w) \}', src[src.index('surf_pal'):src.index('env_pal')])]
surf = ['TERRE', 'GAZON', 'DUR', 'MOQUETTE', 'PLAGE', 'GLACE', 'MARGAUX']
SEUIL = float(sys.argv[1]) if len(sys.argv) > 1 else 20.0
worst = []
for si, (univ, court, lines, net) in enumerate(sp):
    for cond in ('jour', 'nuit', 'pluie'):
        u, c = univ, court
        if cond == 'pluie': u, c = darken(univ), darken(court)
        if cond == 'nuit': u = darken(univ)
        for ci, (skin, outfit, dark) in enumerate(cp):
            if names[ci] == 'MARGAUX' and surf[si] == 'PLAGE':
                pass
            for part, col in (('peau', skin), ('tenue', outfit), ('sombre', dark)):
                for bgn, bg in (('court', c), ('abords', u)):
                    d = dE(col, bg)
                    if d < SEUIL:
                        worst.append((d, surf[si], cond, names[ci], part, '%02X' % col, bgn, '%02X' % bg))
# balle (palette des effets, couleur 1) : couleur propre à chaque court
# (table ball_col de gfx.c), contre le court et ses abords, de jour, de nuit
# et sous la pluie
bc = [int(x, 16) for x in re.findall(r'0x(\w\w)', src[src.index('ball_col[NSURF + 1] ='):].split(';')[0])]
for si, (univ, court, lines, net) in enumerate(sp):
    ball = bc[si]
    for cond in ('jour', 'nuit', 'pluie'):
        c = darken(court) if cond == 'pluie' else court
        u = darken(univ) if cond != 'jour' else univ
        for bgn, bg in (('court', c), ('abords', u)):
            d = dE(ball, bg)
            if d < SEUIL:
                worst.append((d, surf[si], cond, 'balle', '-', '%02X' % ball, bgn, '%02X' % bg))
# tenues entre elles : la tenue de Margaux (la seule rose) doit se distinguer
# de toutes les autres ; les autres paires proches sont signalées pour
# information (elles diffèrent aussi par la peau, les cheveux ou le sexe)
mi = names.index('MARGAUX') if 'MARGAUX' in names else -1
for ci, (skin, outfit, dark) in enumerate(cp):
    for cj in range(ci + 1, len(cp)):
        d = dE(outfit, cp[cj][1])
        if (ci == mi or cj == mi) and d < SEUIL:
            worst.append((d, '-', '-', names[ci], 'tenue', '%02X' % outfit, names[cj], '%02X' % cp[cj][1]))
worst.sort()
for w in worst:
    print('ΔE %5.1f  %-8s %-5s %-4s %-6s $%s sur %-6s $%s' % w)
print('%d paires sous le seuil ΔE %.0f' % (len(worst), SEUIL))
sys.exit(1 if worst else 0)
