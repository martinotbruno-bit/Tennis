#!/usr/bin/env python3
"""Royal Martinot — images des écrans d'annonce du Grand Martinot (version
2.44) : l'Opéra de Sydney (Open d'Augustralie), la tour Eiffel (Debogarros),
Big Ben (Martinbledon) et la statue de la Liberté, raquette au poing (US
Opaul).

Chaque scène est dessinée en couleurs NES sur 224 x 128 px (28 x 16 tuiles,
colonnes 2 à 29, rangées 6 à 21 de l'écran), puis :
  - chaque bloc de 16 x 16 px prend la palette (parmi 4) qui le rend le mieux ;
  - les tuiles identiques sont partagées ; au-delà de 160 tuiles (place libre
    après la police, tuiles $60 à $FF), les tuiles les plus proches sont
    fusionnées.
Sortie : src/assets_gm.c (banque 14), src/assets_gm2.c (banque 6 : images 4
à 6, version 2.47 : Margaux-les-Neiges, Rio de Bruneiro, Deborcy Masters) et
assets/preview_gm.png.
Usage : python3 tools/gen_gm.py"""
import os, math, random

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W, H = 224, 128
FIRST_TILE, MAX_TILES = 0x60, 160

# palette NES 2C02 (comme tools/contrast.py)
PAL = [
 (84,84,84),(0,30,116),(8,16,144),(48,0,136),(68,0,100),(92,0,48),(84,4,0),(60,24,0),
 (32,42,0),(8,58,0),(0,64,0),(0,60,0),(0,50,60),(0,0,0),(0,0,0),(0,0,0),
 (152,150,152),(8,76,196),(48,50,236),(92,30,228),(136,20,176),(160,20,100),(152,34,32),(120,60,0),
 (84,90,0),(40,114,0),(8,124,0),(0,118,40),(0,102,120),(0,0,0),(0,0,0),(0,0,0),
 (236,238,236),(76,154,236),(120,124,236),(176,98,236),(228,84,236),(236,88,180),(236,106,100),(212,136,32),
 (160,170,0),(116,196,0),(76,208,32),(56,204,108),(56,180,204),(60,60,60),(0,0,0),(0,0,0),
 (236,238,236),(168,204,236),(188,188,236),(212,178,236),(236,174,236),(236,174,212),(236,180,176),(228,196,144),
 (204,210,120),(180,222,120),(168,226,144),(152,226,180),(160,214,228),(160,162,160),(0,0,0),(0,0,0)]

def dist(a, b):
    return sum((x - y) ** 2 for x, y in zip(PAL[a], PAL[b]))

class Img:
    def __init__(self, bg):
        self.p = [[bg] * W for _ in range(H)]
    def put(self, x, y, c):
        x, y = int(x), int(y)
        if 0 <= x < W and 0 <= y < H:
            self.p[y][x] = c
    def rect(self, x0, y0, x1, y1, c):
        for y in range(int(y0), int(y1) + 1):
            for x in range(int(x0), int(x1) + 1):
                self.put(x, y, c)
    def ellipse(self, cx, cy, rx, ry, c, half=None):
        for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
            for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
                if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1:
                    if half == 'top' and y > cy: continue
                    self.put(x, y, c)
    def poly(self, pts, c):
        ys = [p[1] for p in pts]
        for y in range(int(min(ys)), int(max(ys)) + 1):
            xs = []
            n = len(pts)
            for i in range(n):
                (x0, y0), (x1, y1) = pts[i], pts[(i + 1) % n]
                if (y0 <= y + 0.5 < y1) or (y1 <= y + 0.5 < y0):
                    xs.append(x0 + (y + 0.5 - y0) * (x1 - x0) / (y1 - y0))
            xs.sort()
            for i in range(0, len(xs) - 1, 2):
                for x in range(int(round(xs[i])), int(round(xs[i + 1]))):
                    self.put(x, y, c)
    def line(self, x0, y0, x1, y1, c):
        n = int(max(abs(x1 - x0), abs(y1 - y0))) + 1
        for k in range(n + 1):
            t = k / n
            self.put(round(x0 + (x1 - x0) * t), round(y0 + (y1 - y0) * t), c)

# --------------------------------------------------------------------------
# Les 4 scènes. Chaque scène : image, 4 palettes (couleur 0 commune = ciel),
# et l'animation (palette, couleurs qui tournent).
def sydney():
    """Open d'Augustralie : l'Opéra de Sydney au coucher du soleil, le pont du
    port derrière, l'eau de la baie qui scintille"""
    bg = 0x26
    im = Img(bg)
    for y in range(0, 32):                     # haut du ciel : orange profond
        for x in range(W):
            if y < 16 or (x + y) % 2 == 0: im.put(x, y, 0x16)
    im.ellipse(176, 58, 16, 16, 0x38)          # soleil couchant, derrière le pont
    im.ellipse(176, 58, 11, 11, 0x37)
    # pont du port (à droite) : grande arche, suspentes, tablier, pylônes
    for x in range(128, 224):
        t = (x - 176) / 48
        y = 36 + 40 * t * t
        im.rect(x, y, x, y + 2, 0x0F)
        if x % 8 == 0 and y < 78:
            im.line(x, y + 3, x, 79, 0x0F)
    im.rect(124, 79, 223, 83, 0x0F)
    for px in (128, 208):
        im.rect(px, 62, px + 11, 87, 0x07)
        im.rect(px + 2, 66, px + 9, 68, 0x17)
    # eau de la baie : motif régulier (peu de tuiles), reflets en 3 tons
    for y in range(88, 128):
        for x in range(W):
            k = ((x % 16) + 3 * ((y // 4) % 4)) % 8
            im.put(x, y, 0x12 if k < 4 else 0x21 if k < 7 else 0x31)
    # Opéra : socle et voiles blanches (ombres grises)
    im.rect(8, 80, 112, 95, 0x17)
    im.rect(8, 80, 112, 81, 0x37)
    sails = [(12, 80, 32, 44, 52, 80), (32, 80, 54, 34, 76, 80), (56, 80, 74, 46, 92, 80), (76, 80, 90, 56, 106, 80)]
    for (x0, y0, xa, ya, x1, y1) in sails:
        im.poly([(x0, y0), (xa, ya), (x1, y1)], 0x30)
        for k in range(1, 4):                  # nervures des voiles
            im.line(xa, ya, x0 + (x1 - x0) * k / 4, y0, 0x10)
        im.line(xa, ya, x1, y1, 0x10)
    # (soleil et pont dans les mêmes blocs : noir, jaune pâle et jaune ensemble)
    pals = [[bg, 0x0F, 0x10, 0x30], [bg, 0x0F, 0x37, 0x38], [bg, 0x12, 0x21, 0x31], [bg, 0x16, 0x17, 0x07]]
    anim = (2, [0x12, 0x21, 0x31])             # l'eau scintille
    return im, pals, anim

def paris():
    """Debogarros : la tour Eiffel qui scintille au crépuscule, au-dessus
    d'un court de terre battue"""
    bg = 0x02
    im = Img(bg)
    for y in range(40, 92):                    # ciel du crépuscule : bleu, puis rose à l'horizon
        for x in range(W):
            if y > 72 or (x + y) % 2 == 0: im.put(x, y, 0x12 if y < 70 else 0x24)
    random.seed(7)
    for _ in range(24):                        # étoiles
        im.put(random.randrange(W), random.randrange(36), 0x30)
    # tour Eiffel (centrée), structure brune, lumières en deux groupes
    cx = 112
    def half_width(y):
        # de 2 px au sommet (y 6) à 34 px à la base (y 96), courbe
        t = (y - 6) / 90
        return 2 + 32 * t ** 2.2
    for y in range(6, 97):
        w = half_width(y)
        for x in range(int(cx - w), int(cx + w) + 1):
            edge = x <= cx - w + 2 or x >= cx + w - 2
            lattice = (x + y) % 4 == 0 or (x - y) % 4 == 0
            if y > 70 and abs(x - cx) < (y - 70) * 0.9:   # arche entre les pieds
                continue
            if edge or lattice or w < 6:
                im.put(x, y, 0x07)
    for y in (30, 58):                         # plateformes
        w = half_width(y) + 3
        im.rect(cx - w, y, cx + w, y + 2, 0x07)
    im.rect(cx - 1, 0, cx + 1, 6, 0x07)        # antenne
    random.seed(11)
    for k in range(70):                        # scintillement : 2 groupes de lumières
        y = random.randrange(8, 96)
        w = half_width(y)
        x = cx + random.uniform(-w, w)
        if y > 70 and abs(x - cx) < (y - 70) * 0.9: continue
        im.put(x, y, 0x38 if k % 2 else 0x28)
    # court de terre battue (premier plan), haies sombres de chaque côté
    im.rect(0, 96, 223, 127, 0x17)
    im.poly([(30, 127), (60, 98), (164, 98), (194, 127)], 0x26)
    for (x0, y0, x1, y1) in ((30, 127, 60, 98), (194, 127, 164, 98), (60, 98, 164, 98), (45, 112, 179, 112)):
        im.line(x0, y0, x1, y1, 0x30)
    im.line(112, 98, 112, 112, 0x30)
    pals = [[bg, 0x0F, 0x10, 0x30], [bg, 0x12, 0x24, 0x30], [bg, 0x07, 0x38, 0x28], [bg, 0x26, 0x17, 0x30]]
    anim = (2, [0x38, 0x28])                   # la tour scintille
    return im, pals, anim

def london():
    """Martinbledon : Londres par beau temps. Big Ben (clocheton gothique,
    flèche et pinacles, grand cadran : la grande aiguille est un sprite qui
    tourne), le Parlement et ses tourelles, la Victoria Tower, la grande roue
    du London Eye ; au premier plan, un court de Wimbledon : bâche violette,
    gazon rayé, lignes blanches et filet"""
    bg = 0x21
    im = Img(bg)
    for (cx, cy, rx, ry) in ((34, 12, 18, 6), (52, 9, 12, 6), (150, 16, 20, 6), (166, 12, 10, 5)):
        im.ellipse(cx, cy, rx, ry, 0x30)       # nuages
    # London Eye (à droite) : jante, rayons, nacelles, pieds en A
    ex, ey, er = 194, 50, 27
    for a in range(0, 360, 2):
        t = math.radians(a)
        im.put(ex + er * math.cos(t), ey + er * math.sin(t), 0x30)
        im.put(ex + (er - 1) * math.cos(t), ey + (er - 1) * math.sin(t), 0x30)
    for a in range(0, 360, 30):
        t = math.radians(a)
        im.line(ex, ey, ex + (er - 2) * math.cos(t), ey + (er - 2) * math.sin(t), 0x0F)
        im.rect(ex + er * math.cos(t) - 1, ey + er * math.sin(t) - 1, ex + er * math.cos(t) + 1, ey + er * math.sin(t) + 1, 0x30)
    im.line(ex, ey, ex - 14, 95, 0x0F); im.line(ex, ey, ex + 14, 95, 0x0F)
    # Victoria Tower (à gauche) : grosse tour carrée, tourelles d'angle, mât
    vx0, vx1, vtop = 8, 36, 30
    im.rect(vx0, vtop, vx1, 95, 0x37)
    for x in (vx0, vx1 - 3):
        im.rect(x, vtop - 8, x + 3, 95, 0x27)             # tourelles d'angle
        im.poly([(x - 1, vtop - 8), (x + 1.5, vtop - 14), (x + 4, vtop - 8)], 0x07)
    for y in range(vtop + 8, 92, 16):
        for x in range(16, vx1 - 4, 8):
            im.rect(x, y, x + 1, y + 7, 0x07)               # fenêtres en ogive
            im.put(x, y - 1, 0x07)
    im.line(22, vtop - 8, 22, vtop - 22, 0x07)              # mât et drapeau
    im.rect(23, vtop - 22, 30, vtop - 18, 0x07)
    # Parlement : façade gothique, tourelles régulières, fenêtres en ogive
    im.rect(36, 68, 172, 95, 0x37)
    for x in range(40, 172, 16):
        im.rect(x, 62, x + 2, 95, 0x27)                     # contreforts
        im.poly([(x - 1, 62), (x + 1, 55), (x + 3, 62)], 0x07)   # pinacles
    for x in range(44, 170, 4):
        if 94 <= x <= 132 or x % 16 == 8: continue
        for y in (72, 84):
            im.rect(x, y, x + 1, y + 7, 0x07)
            im.put(x, y - 1, 0x07); im.put(x + 1, y - 1, 0x07)
    im.rect(36, 92, 172, 95, 0x27)
    # Big Ben : tour, clocheton, cadran, flèche et pinacles
    tx0, tx1 = 98, 130
    im.rect(tx0, 30, tx1, 95, 0x37)
    for x in (tx0, tx1 - 2):
        im.rect(x, 30, x + 2, 95, 0x27)                     # arêtes
    for x in range(104, tx1 - 4, 8):
        im.line(x, 58, x, 92, 0x27)                         # nervures
    for y in range(64, 92, 8):
        for x in (108, 116):
            im.rect(x, y, x + 1, y + 4, 0x07); im.put(x, y - 1, 0x07)
    im.rect(tx0, 30, tx1, 63, 0x27)                         # étage de l'horloge : pierre dorée (même palette que le cadran)
    im.rect(tx0 - 3, 24, tx1 + 3, 30, 0x27)                 # corniche du beffroi
    im.ellipse(114, 42, 14, 14, 0x27)                       # cadran (doré) : la grande aiguille est un sprite
    im.ellipse(114, 42, 12, 12, 0x30)
    for k in range(12):
        a = k * math.pi / 6
        im.put(114 + 10 * math.cos(a), 42 + 10 * math.sin(a), 0x0F)
        if k % 3 == 0:
            im.put(114 + 9 * math.cos(a), 42 + 9 * math.sin(a), 0x0F)
    im.line(114, 42, 118, 46, 0x0F); im.line(115, 42, 119, 46, 0x0F)   # petite aiguille
    im.rect(tx0 + 2, 14, tx1 - 2, 24, 0x37)                 # clocheton (beffroi)
    for x in range(104, tx1 - 3, 8):
        im.rect(x, 16, x + 2, 22, 0x07)                     # baies du beffroi
    im.poly([(tx0, 14), (114, -2), (tx1, 14)], 0x07)        # flèche
    for x in (tx0 + 1, tx1 - 3):
        im.poly([(x - 1, 14), (x + 1, 6), (x + 3, 14)], 0x27)   # pinacles
    im.rect(113, -2, 115, 2, 0x27)                          # croix dorée
    # premier plan : bâche violette de Wimbledon, gazon rayé, lignes, filet
    im.rect(0, 96, 223, 111, 0x14)
    im.rect(0, 97, 223, 97, 0x30)                           # liserés blancs de la bâche
    im.rect(0, 110, 223, 110, 0x30)
    for y in range(112, 128):
        for x in range(W):
            im.put(x, y, 0x2A if (x // 16) % 2 else 0x1A)
    im.rect(0, 125, 223, 126, 0x30)                         # ligne de fond
    im.rect(16, 116, 207, 116, 0x30)                        # ligne de service
    im.rect(112, 116, 112, 126, 0x30)                       # ligne médiane
    pals = [[bg, 0x0F, 0x27, 0x30], [bg, 0x37, 0x27, 0x07], [bg, 0x1A, 0x2A, 0x30], [bg, 0x30, 0x14, 0x0F]]
    anim = None
    return im, pals, anim

def newyork():
    """US Opaul : la statue de la Liberté brandit une raquette devant les
    gratte-ciel de New York la nuit ; les fenêtres s'allument et s'éteignent"""
    bg = 0x01
    im = Img(bg)
    random.seed(3)
    for _ in range(12):
        im.put(random.randrange(W), random.randrange(40), 0x30)
    # gratte-ciel (à droite de la statue) : tours fines séparées par le ciel,
    # hauteurs variées, et trois silhouettes célèbres : l'Empire State
    # Building (gradins et antenne), le Chrysler Building (couronne en arcs,
    # flèche) et une tour effilée façon One World Trade Center
    G = 0x00
    def windows(x0, x1, top, bottom):
        # fenêtres de 1 x 2 px sur une grille fixe de l'écran (colonnes x = 4k + 1,
        # étages y = 4k + 1) ; chaque tuile prend l'un de 3 motifs d'éclairage :
        # peu de tuiles différentes. Deux groupes de couleurs (scintillement).
        for wy in range(top + 3, bottom - 2):
            if wy % 4 != 1: continue
            for wx in range(x0 + 2, x1 - 1):
                if wx % 4 != 1: continue
                pat = ((wx // 8) * 5 + (wy // 8) * 3) % 3
                lit = (pat == 0) or (pat == 1 and (wx // 4 + wy // 4) % 2 == 0) or (pat == 2 and (wy // 4) % 2 == 0)
                if lit:
                    im.rect(wx, wy, wx, wy + 1, 0x38 if (wx // 8) % 2 else 0x28)
    random.seed(9)
    towers = [(88, 12, 44, 'f'), (100, 12, 72, 'f'), (116, 8, 52, 'f'), (128, 20, 96, 'empire'),
              (152, 8, 60, 'f'), (164, 12, 88, 'chrysler'), (180, 8, 48, 'f'),
              (192, 16, 80, 'wtc'), (212, 12, 60, 'f')]
    for (x, w, h, kind) in towers:
        top = 112 - h
        x1 = x + w - 1
        if kind == 'empire':
            im.rect(x, top + 26, x1, 111, G)                  # base
            im.rect(x + 3, top + 14, x1 - 3, top + 26, G)     # premier gradin
            im.rect(x + 6, top + 6, x1 - 6, top + 14, G)      # second gradin
            im.rect(x + 9, top - 10, x1 - 9, top + 6, G)      # flèche et antenne
            windows(x, x1, top + 26, 111); windows(x + 3, x1 - 3, top + 14, top + 26)
            im.put(x + 10, top - 12, 0x38)                    # feu de l'antenne
        elif kind == 'chrysler':
            im.rect(x, top + 16, x1, 111, G)
            cx = (x + x1) / 2
            for k in range(4):                                # couronne en arcs, lumineuse
                r = (w // 2) - 2 * k
                for a in range(0, 181, 15):
                    t = math.radians(a)
                    im.put(cx + r * math.cos(t), top + 16 - k * 4 - r * 0.6 * math.sin(t), 0x38)
            im.line(cx, top + 2, cx, top - 12, G)              # flèche
            im.poly([(x + 2, top + 16), (cx, top), (x1 - 1, top + 16)], G)
            for k in range(4):
                r = (w // 2) - 2 * k - 1
                for a in range(0, 181, 30):
                    t = math.radians(a)
                    im.put(cx + r * math.cos(t), top + 14 - k * 4 - r * 0.5 * math.sin(t), 0x38)
            windows(x, x1, top + 16, 111)
        elif kind == 'wtc':
            im.poly([(x, 112), (x + 4, top), (x1 - 3, top), (x1 + 1, 112)], G)   # tour effilée
            im.line((x + x1) // 2, top, (x + x1) // 2, top - 14, G)             # antenne
            windows(x + 2, x1 - 2, top + 2, 111)
        else:
            im.rect(x, top, x1, 111, G)
            if h >= 60:
                im.rect(x + w // 2 - 1, top - 6, x + w // 2, top - 1, G)        # petite antenne
            windows(x, x1, top, 111)
    # eau de la baie (motif régulier)
    for y in range(112, 128):
        for x in range(W):
            im.put(x, y, 0x12 if (x % 8 == (y // 2) % 8) else 0x02)
    # statue (x 16 à 95) : socle, robe verte, tête couronnée, bras levé, raquette
    sx = 46
    im.poly([(sx - 22, 127), (sx - 16, 98), (sx + 16, 98), (sx + 22, 127)], 0x1B)   # socle
    im.rect(sx - 18, 96, sx + 18, 99, 0x2B)
    im.poly([(sx - 14, 96), (sx - 9, 50), (sx + 9, 50), (sx + 14, 96)], 0x2B)       # robe
    for k in range(5):
        im.line(sx - 8 + 4 * k, 56, sx - 11 + 5.5 * k, 95, 0x1B)                    # plis
    im.rect(sx - 11, 62, sx - 4, 70, 0x1B)                                         # tablette
    im.ellipse(sx, 43, 6, 7, 0x2B)                                                 # tête
    im.put(sx - 2, 42, 0x1B); im.put(sx + 2, 42, 0x1B)                             # yeux
    for k in range(7):                                                             # couronne
        a = math.pi * (1.1 + 0.13 * k)
        im.line(sx, 38, sx + 11 * math.cos(a), 38 + 11 * math.sin(a), 0x2B)
    for d in range(4):                                                             # bras levé (épais)
        im.line(sx + 5 + d, 54, sx + 18 + d, 30, 0x2B)
    im.rect(sx + 17, 24, sx + 20, 31, 0x1B)                                         # manche
    rcx, rcy = sx + 19, 13                                                         # tamis : de y 2 à 24
    for k in range(-6, 7, 3):                                                      # cordage blanc
        im.line(rcx + k, rcy - 8, rcx + k, rcy + 8, 0x30)
        im.line(rcx - 7, rcy + k, rcx + 7, rcy + k, 0x30)
    for y in range(0, 26):                                                         # cadre du tamis
        for x in range(rcx - 11, rcx + 12):
            r = ((x - rcx) / 10) ** 2 + ((y - rcy) / 11) ** 2
            r2 = ((x - rcx) / 8) ** 2 + ((y - rcy) / 9) ** 2
            if r <= 1 and r2 > 1: im.put(x, y, 0x1B)
            elif r2 <= 1 and im.p[y][x] != 0x30: im.put(x, y, bg)
    pals = [[bg, 0x0F, 0x10, 0x30], [bg, 0x00, 0x38, 0x28], [bg, 0x1B, 0x2B, 0x30], [bg, 0x02, 0x12, 0x30]]
    anim = (1, [0x38, 0x28])                   # les fenêtres scintillent
    return im, pals, anim

def alps():
    """Margaux-les-Neiges (glace) : les Alpes en hiver, un sommet pointu enneigé,
    des sapins, un chalet ; au premier plan, un court de tennis sur une
    patinoire. La neige tombe (sprites)"""
    bg = 0x21
    im = Img(bg)
    for (cx, cy, rx, ry) in ((30, 14, 16, 5), (46, 11, 10, 5)):
        im.ellipse(cx, cy, rx, ry, 0x30)        # nuages
    # montagnes (rangées de blocs 0 à 3) : roche grise, versant à l'ombre, neige
    def mountain(px, py, left, right, base=64):
        im.poly([(px - left, base), (px, py), (px + right, base)], 0x10)
        im.poly([(px, py), (px + right, base), (px + right * 0.35, base)], 0x00)   # versant à l'ombre
        # neige du sommet : bord dentelé
        for y in range(py, base):
            t = (y - py) / (base - py)
            if t > 0.42 + (0.06 if (y // 4) % 2 else 0): continue
            x0, x1 = px - left * t, px + right * t
            im.rect(x0, y, x1, y, 0x30)
    mountain(176, 22, 60, 50)
    mountain(40, 18, 52, 44)
    mountain(104, 2, 46, 38)                     # le grand sommet pointu, au centre
    # champ de neige et sapins (rangées 4 et 5)
    im.rect(0, 64, 223, 95, 0x30)
    def pine(x, y, h):
        for k in range(3):                       # trois étages de branches
            top = y + k * h // 4
            w = 3 + k * 3
            im.poly([(x - w, top + h // 3), (x, top), (x + w + 1, top + h // 3)], 0x09)
            im.rect(x - w + 1, top + h // 3 - 1, x + w - 1, top + h // 3 - 1, 0x1A)
        im.rect(x, y + h - 2, x + 1, y + h + 1, 0x09)
    for (x, y, h) in ((8, 66, 26), (22, 70, 22), (36, 64, 28), (52, 70, 22), (66, 67, 24),
                      (80, 72, 20), (138, 70, 22), (150, 66, 26), (214, 68, 24)):
        pine(x, y, h)
    # chalet (à droite) : bois, toit chargé de neige, balcon, fenêtres
    cx0, cx1 = 164, 206
    im.rect(cx0, 78, cx1, 95, 0x17)
    for y in range(80, 95, 3):
        im.rect(cx0, y, cx1, y, 0x07)            # planches
    im.poly([(cx0 - 6, 80), (185, 64), (cx1 + 7, 80)], 0x07)   # toit
    im.line(cx0 - 6, 79, 185, 64, 0x30); im.line(cx0 - 5, 79, 185, 65, 0x30)   # neige sur le bord du toit
    im.line(185, 64, cx1 + 7, 79, 0x30); im.line(185, 65, cx1 + 6, 79, 0x30)
    im.rect(182, 70, 188, 76, 0x30); im.rect(184, 72, 186, 75, 0x07)   # lucarne
    for x in (169, 195):
        im.rect(x, 82, x + 7, 89, 0x30)          # fenêtres (cadres blancs)
        im.rect(x + 2, 84, x + 5, 87, 0x07)
    im.rect(181, 84, 189, 95, 0x07)              # porte
    im.rect(183, 86, 187, 95, 0x17)
    # premier plan : bandes de la patinoire, glace, court de tennis tracé dessus
    im.rect(0, 96, 223, 111, 0x30)               # bandes blanches
    im.rect(0, 102, 223, 104, 0x16)              # liseré rouge
    im.rect(0, 110, 223, 111, 0x3C)
    for y in range(112, 128):
        for x in range(W):
            im.put(x, y, 0x3C)
    for x in range(0, W, 32):                    # reflets de la glace
        im.line(x + 2, 123, x + 6, 119, 0x30)
    im.rect(0, 125, 223, 126, 0x16)              # ligne de fond (rouge sur la glace)
    im.rect(16, 116, 207, 116, 0x16)             # ligne de service
    im.rect(112, 116, 112, 126, 0x16)            # ligne médiane
    pals = [[bg, 0x30, 0x10, 0x00], [bg, 0x09, 0x1A, 0x30], [bg, 0x07, 0x17, 0x30], [bg, 0x3C, 0x30, 0x16]]
    anim = None                                  # la neige tombe : sprites (gmintro.c)
    return im, pals, anim

def rio():
    """Rio de Bruneiro (plage) : la baie de Rio, le Pain de Sucre et son
    téléphérique, le Corcovado et sa statue ; la mer qui scintille, la
    plage et son filet, les palmiers, et la célèbre promenade aux vagues
    noires et blanches"""
    bg = 0x21
    im = Img(bg)
    for (cx, cy, rx, ry) in ((120, 12, 16, 5), (136, 9, 9, 4), (200, 20, 14, 4)):
        im.ellipse(cx, cy, rx, ry, 0x30)
    # Corcovado (à droite) : montagne et statue aux bras ouverts
    im.poly([(128, 80), (176, 30), (186, 26), (196, 30), (224, 52), (224, 80)], 0x0A)
    im.poly([(150, 80), (176, 34), (186, 30), (200, 40), (190, 80)], 0x1A)
    im.rect(184, 13, 187, 26, 0x17)              # statue
    im.rect(178, 16, 193, 17, 0x17)              # bras
    im.rect(184, 11, 187, 12, 0x17)              # tête
    # Morro da Urca (petit) et Pain de Sucre (grand dôme)
    im.ellipse(16, 70, 30, 22, 0x0A, half='top')
    im.poly([(44, 80), (52, 40), (62, 24), (76, 20), (88, 26), (98, 44), (108, 80)], 0x17)
    for (x0, y0, x1, y1) in ((60, 30, 56, 78), (70, 24, 68, 78), (82, 26, 86, 78), (92, 40, 98, 78)):
        im.line(x0, y0, x1, y1, 0x07)            # stries de la roche
    im.poly([(46, 80), (52, 58), (60, 66), (66, 80)], 0x0A)    # végétation au pied
    im.poly([(92, 80), (98, 60), (106, 80)], 0x0A)
    # téléphérique : câble du Morro au sommet, cabine
    im.line(18, 49, 74, 20, 0x0F)
    im.rect(42, 38, 47, 43, 0x0F); im.rect(43, 39, 46, 41, 0x30); im.line(44, 36, 44, 37, 0x0F)
    # mer (rangées 5 et 6) : reflets en 3 tons qui tournent
    for y in range(80, 100):
        for x in range(W):
            k = ((x % 16) + 5 * ((y // 3) % 3)) % 8
            im.put(x, y, 0x11 if k < 4 else 0x2C if k < 7 else 0x30)
    # plage, filet de beach-tennis
    im.rect(0, 100, 223, 111, 0x37)
    for x in range(0, W, 8):
        im.put(x + 3, 104, 0x27); im.put(x + 7, 108, 0x27)
    im.rect(64, 94, 65, 109, 0x27); im.rect(160, 94, 161, 109, 0x27)   # poteaux
    im.rect(66, 95, 159, 95, 0x30)
    for y in range(97, 104, 2):
        im.rect(66, y, 159, y, 0x30)              # mailles du filet
    for x in range(68, 159, 4):
        im.rect(x, 95, x, 103, 0x30)
    im.rect(16, 109, 207, 109, 0x30)             # ligne tracée dans le sable
    # palmiers (bords) : tronc courbe, palmes
    def palm(x, top, lean):
        for y in range(top, 112):
            t = (112 - y) / (112 - top)
            xx = x + lean * t * t
            im.rect(xx, y, xx + 2, y, 0x17)
            if y % 4 == 0: im.put(xx + 1, y, 0x07)
        hx = x + lean
        for (dx, dy) in ((-14, 6), (-10, -6), (0, -9), (10, -6), (14, 6), (-6, 10), (7, 10)):
            im.line(hx + 1, top, hx + 1 + dx, top + dy, 0x0A)
            im.line(hx + 1, top + 1, hx + 1 + dx, top + dy + 1, 0x1A)
    palm(8, 58, 8)
    palm(208, 54, -10)
    # promenade de Copacabana : vagues noires et blanches (rangées 7)
    for y in range(112, 128):
        for x in range(W):
            w = 4 * math.sin((x % 32) / 32 * 2 * math.pi)
            band = int((y - 112 + w + 8) // 4) % 2
            im.put(x, y, 0x0F if band else 0x30)
    # (la mer en dernier : à égalité, un bloc prend la première palette ; les
    # nuages ne doivent pas prendre celle de la mer, qui tourne)
    pals = [[bg, 0x0A, 0x1A, 0x17], [bg, 0x37, 0x27, 0x30], [bg, 0x0F, 0x30, 0x07], [bg, 0x11, 0x2C, 0x30]]
    anim = (3, [0x11, 0x2C, 0x30])               # la mer scintille
    return im, pals, anim

def bercy():
    """Deborcy Masters (moquette, en salle) : le palais omnisports de Bercy la
    nuit, ses pentes de gazon et sa charpente, les projecteurs qui balaient
    le ciel ; la Seine reflète les lumières"""
    bg = 0x0F
    im = Img(bg)
    random.seed(5)
    for _ in range(14):
        im.put(random.randrange(W), random.randrange(28), 0x30)
    # projecteurs : 3 faisceaux de 3 couleurs qui tournent (balayage)
    for (x0, x1, c) in ((60, 20, 0x31), (112, 112, 0x11), (164, 204, 0x01)):
        for k in range(-4, 5):
            im.line(112 + (x0 - 112) * 0.2 + k * 0.4, 30, x1 + k * 3, 0, c)
    # palais : pyramide tronquée, pentes de gazon rayées, charpente et verrière
    im.poly([(16, 96), (64, 40), (160, 40), (208, 96)], 0x0A)
    for k in range(0, 60, 6):                    # rayures du gazon
        y = 96 - k
        x0 = 16 + 48 * k / 56; x1 = 208 - 48 * k / 56
        im.rect(x0, y, x1, y + 2, 0x1A)
    im.rect(64, 32, 160, 47, 0x00)              # charpente
    for x in range(64, 161, 8):
        im.line(x, 32, x + 4, 47, 0x10)
        im.line(x + 8, 32, x + 4, 47, 0x10)
    im.rect(64, 32, 160, 33, 0x10)
    im.rect(56, 48, 168, 51, 0x00)
    for x in range(60, 168, 8):
        im.rect(x, 49, x + 3, 50, 0x38)          # lumières de la verrière
    # enseigne « BERCY » (lettres de 5 x 7, sur la pente)
    font = {'B': ['1110', '1001', '1110', '1001', '1110'], 'E': ['1111', '1000', '1110', '1000', '1111'],
            'R': ['1110', '1001', '1110', '1010', '1001'], 'C': ['0111', '1000', '1000', '1000', '0111'],
            'Y': ['1001', '1001', '0111', '0001', '0110']}
    x = 88
    for ch in 'BERCY':
        for j, row in enumerate(font[ch]):
            for i, b in enumerate(row):
                if b == '1': im.rect(x + i * 2, 66 + j * 2, x + i * 2 + 1, 66 + j * 2 + 1, 0x38)
        x += 10
    im.rect(84, 62, 139, 63, 0x00); im.rect(84, 77, 139, 78, 0x00)
    # entrées éclairées au pied
    for x in range(32, 196, 24):
        im.rect(x, 88, x + 9, 95, 0x38)
        im.rect(x + 2, 90, x + 7, 95, 0x00)
    # Seine : reflets jaunes et bleus
    for y in range(96, 128):
        for x in range(W):
            k = (x + 3 * (y // 2)) % 12
            im.put(x, y, 0x02 if k < 7 else 0x12 if k < 10 else 0x28 if (x // 24) % 2 == 0 and y < 112 else 0x02)
    pals = [[bg, 0x0A, 0x1A, 0x38], [bg, 0x00, 0x10, 0x38], [bg, 0x31, 0x11, 0x01], [bg, 0x02, 0x12, 0x28]]
    anim = (2, [0x31, 0x11, 0x01])               # les projecteurs balaient le ciel
    return im, pals, anim

# images 0 à 3 : le Grand Martinot ; 4 à 6 : tournois ajoutés par le Martinworld Tour
SCENES = [sydney, paris, london, newyork, alps, rio, bercy]
BANK2_FROM = 4      # tuiles, tables de noms et attributs des images 4 à 6 : banque 6

# --------------------------------------------------------------------------
def quantize(im, pals):
    """palette de chaque bloc 16 x 16 (index 0..3) et image en indices 0..3"""
    attr = [[0] * (W // 16) for _ in range(H // 16)]
    out = [[0] * W for _ in range(H)]
    for by in range(H // 16):
        for bx in range(W // 16):
            best, bp = None, 0
            for p, pal in enumerate(pals):
                err = 0
                for y in range(by * 16, by * 16 + 16):
                    for x in range(bx * 16, bx * 16 + 16):
                        c = im.p[y][x]
                        err += min(dist(c, q) for q in pal)
                if best is None or err < best:
                    best, bp = err, p
            attr[by][bx] = bp
            pal = pals[bp]
            for y in range(by * 16, by * 16 + 16):
                for x in range(bx * 16, bx * 16 + 16):
                    c = im.p[y][x]
                    out[y][x] = min(range(4), key=lambda i: dist(c, pal[i]))
    return attr, out

def tiles_of(out):
    tiles = []
    for ty in range(H // 8):
        for tx in range(W // 8):
            tiles.append(tuple(tuple(out[ty * 8 + y][tx * 8:tx * 8 + 8]) for y in range(8)))
    return tiles

def tdist(a, b):
    return sum(1 for y in range(8) for x in range(8) if a[y][x] != b[y][x])

def reduce_tiles(tiles):
    """tuiles uniques ; au-delà de MAX_TILES, fusion des plus proches (la
    moins utilisée remplacée par sa voisine la plus ressemblante)"""
    uniq = list(dict.fromkeys(tiles))
    use = {t: tiles.count(t) for t in uniq}
    repl = {}
    while len(uniq) > MAX_TILES:
        uniq.sort(key=lambda t: use[t])
        t = uniq.pop(0)
        near = min(uniq, key=lambda u: tdist(t, u))
        repl[t] = near
        use[near] += use[t]
        for k, v in list(repl.items()):
            if v == t: repl[k] = near
    idx = {t: FIRST_TILE + i for i, t in enumerate(uniq)}
    nt = [idx[repl.get(t, t)] for t in tiles]
    return uniq, nt, len(repl)

def encode(t):
    lo, hi = [], []
    for y in range(8):
        a = b = 0
        for x in range(8):
            a |= (t[y][x] & 1) << (7 - x)
            b |= ((t[y][x] >> 1) & 1) << (7 - x)
        lo.append(a); hi.append(b)
    return bytes(lo + hi)

def c_array(name, data):
    s = 'const unsigned char %s[%d] = {\n' % (name, len(data))
    for i in range(0, len(data), 16):
        s += '  ' + ','.join('0x%02X' % v for v in data[i:i + 16]) + ',\n'
    return s + '};\n'

src = ['/* Fichier généré par tools/gen_gm.py — ne pas modifier. */',
       '/* Images des écrans d\'annonce du Grand Martinot et du Martinworld Tour (banque 14 ;',
       '   tuiles, tables de noms et attributs des images 4 à 6 : assets_gm2.c, banque 6). */', '']
src2 = ['/* Fichier généré par tools/gen_gm.py — ne pas modifier. */',
        '/* Images 4 à 6 des écrans d\'annonce (Martinworld Tour) : banque 6. */', '']
ntiles = []
preview = []
for k, scene in enumerate(SCENES):
    im, pals, anim = scene()
    attr, out = quantize(im, pals)
    tiles = tiles_of(out)
    uniq, nt, merged = reduce_tiles(tiles)
    # attributs de l'écran entier (rangées 6 à 21, colonnes 2 à 29 : image)
    at = bytearray(64)
    for by in range(15):            # blocs 16 x 16 de l'écran (rangées de 2 tuiles)
        for bx in range(16):
            ty, tx = by * 2, bx * 2
            p = 0
            if 6 <= ty < 22 and 2 <= tx < 30:
                p = attr[(ty - 6) // 2][(tx - 2) // 2]
            i = (by // 2) * 8 + bx // 2
            q = (by & 1) * 2 + (bx & 1)
            at[i] |= p << (q * 2)
    dst = src if k < BANK2_FROM else src2
    dst.append(c_array('gm_chr%d' % k, b''.join(encode(t) for t in uniq)))
    dst.append(c_array('gm_nt%d' % k, bytes(nt)))
    dst.append(c_array('gm_at%d' % k, bytes(at)))
    if k >= BANK2_FROM:
        src.append('extern const unsigned char gm_chr%d[], gm_nt%d[], gm_at%d[];' % (k, k, k))
    ntiles.append(len(uniq))
    src.append(c_array('gm_pal%d' % k, bytes(c for p in pals for c in p)))
    a = anim or (0, [])
    src.append(c_array('gm_anim%d' % k, bytes([a[0], len(a[1])] + a[1] + [0] * (3 - len(a[1])))))
    print('%-8s %3d tuiles (%d fusionnées)' % (scene.__name__, len(uniq), merged))
    # aperçu : rendu avec les palettes, tuiles fusionnées comprises
    rgb = []
    for ty in range(16):
        for y in range(8):
            row = []
            for tx in range(28):
                t = uniq[nt[ty * 28 + tx] - FIRST_TILE]
                p = pals[attr[ty // 2][tx // 2]]
                row += [PAL[p[t[y][x]]] for x in range(8)]
            rgb.append(row)
    preview.append(rgb)
# grande aiguille de Big Ben : 3 tuiles de sprite (couleur 1), retournées
# pour 8 directions : verticale (colonne 4), diagonale, horizontale (rangée 4) ;
# puis un flocon de neige (Margaux-les-Neiges)
hand_v = [[1 if x == 4 else 0 for x in range(8)] for y in range(8)]
hand_d = [[1 if x == 7 - y else 0 for x in range(8)] for y in range(8)]
hand_h = [[1 if y == 4 else 0 for x in range(8)] for y in range(8)]
flake = [[1 if (x, y) in ((3, 2), (2, 3), (3, 3), (4, 3), (3, 4)) else 0 for x in range(8)] for y in range(8)]
src.append(c_array('gm_hand', b''.join(encode(tuple(map(tuple, t))) for t in (hand_v, hand_d, hand_h, flake))))
N = len(SCENES)
lst = lambda name: ', '.join('%s%d' % (name, k) for k in range(N))
src.append('const unsigned char gm_ntiles[%d] = { %s };' % (N, ', '.join(map(str, ntiles))))
src.append('const unsigned char gm_bank[%d] = { %s };' % (N, ', '.join('14' if k < BANK2_FROM else '6' for k in range(N))))
src.append('const unsigned char * const gm_chr[%d] = { %s };' % (N, lst('gm_chr')))
src.append('const unsigned char * const gm_nt[%d] = { %s };' % (N, lst('gm_nt')))
src.append('const unsigned char * const gm_at[%d] = { %s };' % (N, lst('gm_at')))
src.append('const unsigned char * const gm_pal[%d] = { %s };' % (N, lst('gm_pal')))
src.append('const unsigned char * const gm_anim[%d] = { %s };' % (N, lst('gm_anim')))
with open(os.path.join(ROOT, 'src', 'assets_gm.c'), 'w') as f:
    f.write('\n'.join(src) + '\n')
with open(os.path.join(ROOT, 'src', 'assets_gm2.c'), 'w') as f:
    f.write('\n'.join(src2) + '\n')
try:
    from PIL import Image
    img = Image.new('RGB', (W * 2 + 8, H * 4 + 24), (255, 255, 255))
    for k, rgb in enumerate(preview):
        ox, oy = (k % 2) * (W + 8), (k // 2) * (H + 8)
        for y in range(H):
            for x in range(W):
                img.putpixel((ox + x, oy + y), rgb[y][x])
    img = img.resize((img.size[0] * 2, img.size[1] * 2), Image.NEAREST)
    img.save(os.path.join(ROOT, 'assets', 'preview_gm.png'))
except ImportError:
    pass
