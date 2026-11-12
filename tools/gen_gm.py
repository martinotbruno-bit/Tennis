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
Sortie : src/assets_gm.c (banque 14) et assets/preview_gm.png.
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
    """Martinbledon : Big Ben et le Parlement par beau temps, gazon rayé au
    premier plan ; l'aiguille de l'horloge tourne (sprite)"""
    bg = 0x21
    im = Img(bg)
    for (cx, cy, rx, ry) in ((40, 18, 22, 7), (60, 14, 16, 8), (170, 26, 26, 8), (190, 20, 14, 7)):
        im.ellipse(cx, cy, rx, ry, 0x30)       # nuages
    # Parlement (bas, sur toute la largeur), fenêtres en ogive
    im.rect(0, 70, 223, 111, 0x37)
    for x in range(0, 224, 8):
        im.rect(x, 66, x + 3, 69, 0x37)        # créneaux
        if 92 <= x <= 140: continue
        im.rect(x + 2, 76, x + 4, 84, 0x07)
        im.rect(x + 2, 92, x + 4, 100, 0x07)
    im.rect(0, 106, 223, 111, 0x27)
    # Big Ben (tour), horloge, flèche
    tx0, tx1 = 96, 136
    im.rect(tx0, 22, tx1, 111, 0x37)
    im.rect(tx0, 22, tx0 + 3, 111, 0x27); im.rect(tx1 - 3, 22, tx1, 111, 0x27)
    for y in range(60, 100, 8):
        im.rect(tx0 + 8, y, tx0 + 10, y + 5, 0x07); im.rect(tx1 - 10, y, tx1 - 8, y + 5, 0x07)
    im.ellipse(116, 40, 15, 15, 0x27)
    im.ellipse(116, 40, 13, 13, 0x30)          # cadran (la grande aiguille est un sprite qui tourne)
    for k in range(12):
        a = k * math.pi / 6
        im.put(116 + 11 * math.cos(a), 40 + 11 * math.sin(a), 0x0F)
    im.line(116, 40, 121, 45, 0x0F); im.line(117, 40, 122, 45, 0x0F)   # petite aiguille
    im.poly([(tx0 - 2, 22), (116, 0), (tx1 + 2, 22)], 0x07)   # flèche
    im.rect(tx0 - 2, 20, tx1 + 2, 22, 0x27)
    # gazon rayé et ligne de fond
    for y in range(112, 128):
        for x in range(W):
            im.put(x, y, 0x2A if (x // 16) % 2 else 0x1A)
    im.rect(0, 120, 223, 121, 0x30)
    pals = [[bg, 0x0F, 0x27, 0x30], [bg, 0x37, 0x27, 0x07], [bg, 0x1A, 0x2A, 0x30], [bg, 0x30, 0x10, 0x0F]]
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
    # gratte-ciel (à droite de la statue), alignés sur les tuiles ; fenêtres
    # régulières (2 par tuile), allumées par étage, en deux groupes
    random.seed(5)
    x = 96
    heights = [72, 48, 96, 56, 84, 64, 100, 52, 76]
    widths = [16, 24, 16, 24, 16, 16, 24, 16]
    n = 0
    while x < W:
        w = widths[n % len(widths)]
        h = heights[n % len(heights)]
        n += 1
        top = 112 - h
        im.rect(x, top, x + w - 1, 111, 0x00)
        if w == 24 and random.random() < 0.5:
            im.rect(x + 10, top - 8, x + 13, top - 1, 0x00)    # antenne
        for wy in range(top + 2, 108, 8):
            on = random.random()
            for wx in range(x + 1, x + w - 1, 4):
                if on < 0.75:
                    im.rect(wx, wy, wx + 1, wy + 3, 0x38 if ((wy // 8) + (x // 8)) % 2 else 0x28)
        x += w
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

SCENES = [sydney, paris, london, newyork]

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
       '/* Images des écrans d\'annonce du Grand Martinot (banque 14). */', '']
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
    src.append(c_array('gm_chr%d' % k, b''.join(encode(t) for t in uniq)))
    src.append(c_array('gm_nt%d' % k, bytes(nt)))
    src.append(c_array('gm_at%d' % k, bytes(at)))
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
# pour 8 directions : verticale (colonne 4), diagonale, horizontale (rangée 4)
hand_v = [[1 if x == 4 else 0 for x in range(8)] for y in range(8)]
hand_d = [[1 if x == 7 - y else 0 for x in range(8)] for y in range(8)]
hand_h = [[1 if y == 4 else 0 for x in range(8)] for y in range(8)]
src.append(c_array('gm_hand', b''.join(encode(tuple(map(tuple, t))) for t in (hand_v, hand_d, hand_h))))
src.append('const unsigned char gm_ntiles[4] = { %s };' % ', '.join(
    str(len(reduce_tiles(tiles_of(quantize(*SCENES[k]()[:2])[1]))[0])) for k in range(4)))
src.append('const unsigned char * const gm_chr[4] = { gm_chr0, gm_chr1, gm_chr2, gm_chr3 };')
src.append('const unsigned char * const gm_nt[4] = { gm_nt0, gm_nt1, gm_nt2, gm_nt3 };')
src.append('const unsigned char * const gm_at[4] = { gm_at0, gm_at1, gm_at2, gm_at3 };')
src.append('const unsigned char * const gm_pal[4] = { gm_pal0, gm_pal1, gm_pal2, gm_pal3 };')
src.append('const unsigned char * const gm_anim[4] = { gm_anim0, gm_anim1, gm_anim2, gm_anim3 };')
with open(os.path.join(ROOT, 'src', 'assets_gm.c'), 'w') as f:
    f.write('\n'.join(src) + '\n')
try:
    from PIL import Image
    img = Image.new('RGB', (W * 2 + 8, H * 2 + 8), (255, 255, 255))
    for k, rgb in enumerate(preview):
        ox, oy = (k % 2) * (W + 8), (k // 2) * (H + 8)
        for y in range(H):
            for x in range(W):
                img.putpixel((ox + x, oy + y), rgb[y][x])
    img = img.resize((img.size[0] * 2, img.size[1] * 2), Image.NEAREST)
    img.save(os.path.join(ROOT, 'assets', 'preview_gm.png'))
except ImportError:
    pass
