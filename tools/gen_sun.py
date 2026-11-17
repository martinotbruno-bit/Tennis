#!/usr/bin/env python3
"""Royal Martinot 2.76 : le soleil couchant du crépuscule, façon jeux de
course des années 80 (soleil rayé, ciel en dégradé, sol qui
mer et plage, reflet scintillant, palmiers en ombre chinoise).

Trois images plein écran (une par phase du crépuscule : soleil couchant,
soleil rouge, nuit tombante). Chaque image a son propre jeu de tuiles
(chargé en entier dans la table de fond pendant le plan) : 256 tuiles au
plus par phase.

Couleurs : 4 palettes de fond, couleur commune noire.
  palette 0 (lignes 0-47)   : noir, ciel 1, ciel 2, soleil haut
  palette 1 (lignes 48-95)  : noir, ciel 2, ciel 3, soleil milieu
  palette 2 (lignes 96-143) : noir, ciel 3, ciel 4, soleil bas
  palette 3 (sol, 144-239)  : noir, mer, reflet du soleil, sable
Les couleurs de ciel communes à deux palettes voisines permettent un
dégradé tramé continu. Le reflet du soleil sur la mer scintille pendant
le plan (couleur 2 de la palette 3).

Sortie : src/assets_sun.c (banque 22), docs/soleil_276.png (aperçu)."""
import math, os

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
W, H = 256, 240
HZ = 144                                    # horizon

# palette NES (FCEUX), RVB
NES = [(124,124,124), (0,0,252), (0,0,188), (68,40,188), (148,0,132), (168,0,32), (168,16,0), (136,20,0), (80,48,0), (0,120,0), (0,104,0), (0,88,0), (0,64,88), (0,0,0), (0,0,0), (0,0,0), (188,188,188), (0,120,248), (0,88,248), (104,68,252), (216,0,204), (228,0,88), (248,56,0), (228,92,16), (172,124,0), (0,184,0), (0,168,0), (0,168,68), (0,136,136), (0,0,0), (0,0,0), (0,0,0), (248,248,248), (60,188,252), (104,136,252), (152,120,248), (248,120,248), (248,88,152), (248,120,88), (252,160,68), (248,184,0), (184,248,24), (88,216,84), (88,248,152), (0,232,216), (120,120,120), (0,0,0), (0,0,0), (252,252,252), (164,228,252), (184,184,248), (216,184,248), (248,184,248), (248,164,192), (240,208,176), (252,224,168), (248,216,120), (216,248,120), (184,248,184), (184,248,216), (0,252,252), (248,216,248), (0,0,0), (0,0,0)]

# phases : ciel (5 niveaux, du haut vers l'horizon), soleil (haut, milieu,
# bas), sol (mer, reflet, sable), centre et rayon du soleil, étoiles
PHASES = [
    dict(sky=[0x0F, 0x02, 0x13, 0x24, 0x25], sun=[0x38, 0x28, 0x27], grid=[0x02, 0x28, 0x27],
         cy=86, r=54, stars=6),
    dict(sky=[0x0F, 0x01, 0x03, 0x14, 0x15], sun=[0x28, 0x27, 0x16], grid=[0x03, 0x27, 0x17],
         cy=124, r=58, stars=14),
    dict(sky=[0x0F, 0x0F, 0x02, 0x03, 0x04], sun=[0x16, 0x16, 0x06], grid=[0x02, 0x16, 0x07],
         cy=166, r=62, stars=30),
]

GJ, GS = 3, 80
BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]


def pal_row(y):
    return 0 if y < 48 else 1 if y < 96 else 2 if y < HZ else 3


def sky_level(y):
    """niveau de ciel continu (0..4) ; reste dans les niveaux de la palette"""
    if y < 48:
        return 2.0 * (y / 47.0) ** 1.3
    if y < 96:
        return 2.0 + (y - 48) / 47.0
    return 3.0 + (y - 96) / 47.0


def palms(img):
    """deux palmiers en ombre chinoise (couleur 0 : noir), en bord d'écran
    (couronnes en partie hors champ : moins de tuiles)"""
    def disc(cx, cy, r):
        for yy in range(int(cy - r) - 1, int(cy + r) + 2):
            for xx in range(int(cx - r) - 1, int(cx + r) + 2):
                if 0 <= xx < W and 0 <= yy < H and (xx - cx) ** 2 + (yy - cy) ** 2 <= r * r:
                    img[yy][xx] = 0
    for side in (0, 1):
        sg = 1 if side == 0 else -1
        bx, by = (14, 236) if side == 0 else (244, 236)
        ty = 64 if side == 0 else 82
        lean = PALM_LEAN
        cx = cy = 0
        for y in range(by, ty - 1, -1):
            t = (by - y) / float(by - ty)
            x = bx + sg * lean * t * t
            w = 3.2 - 1.2 * t
            for xx in range(int(x - w), int(x + w) + 1):
                if 0 <= xx < W:
                    img[y][xx] = 0
            cx, cy = x, y
        for ang, ln, droop in PALM_FRONDS:
            a = math.radians(ang)
            ca, sa = math.cos(a) * sg, math.sin(a)
            for i in range(0, 81):
                t = i / 80.0
                x = cx + ca * ln * t
                y = cy + sa * ln * t + droop * ln * t * t
                disc(x, y, PALM_W * (1 - t) + 0.5)
        disc(cx, cy, 3.5)


PALM_LEAN = 18
PALM_W = 2.2
PALM_FRONDS = [(-160, 34, 0.6), (-125, 30, 0.55), (-70, 26, 0.45), (-25, 34, 0.6), (10, 32, 0.8), (170, 30, 0.9), (45, 22, 1.0)]


def build(ph, sf):
    img = [[0] * W for _ in range(H)]
    sky = ph['sky']
    # ciel tramé
    for y in range(HZ):
        p = pal_row(y)
        lv = sky_level(y)
        base = int(lv)
        f = lv - base
        lo, hi = base, min(base + 1, 4)
        for x in range(W):
            v = hi if f * 16 > BAYER[y & 3][x & 3] else lo
            # niveau -> indice dans la palette de la rangée
            first = [0, 2, 3][p]                  # niveau de l'indice 1 (0 : noir, indices 1 et 2)
            if p == 0:
                img[y][x] = v                     # niveaux 0, 1, 2 = indices 0, 1, 2
            else:
                img[y][x] = 1 if v <= first else 2
    # étoiles (en haut, sur le noir)
    rs = 12345
    n = 0
    while n < ph['stars']:
        rs = (rs * 1103515245 + 12345) & 0x7FFFFFFF
        x = rs % W
        rs = (rs * 1103515245 + 12345) & 0x7FFFFFFF
        y = rs % (70 if ph['stars'] > 20 else 40)
        if img[y][x] == 0:
            img[y][x] = 2 if y >= 48 else 2
            n += 1
    # soleil rayé
    cx, cy, r = 128, ph['cy'], ph['r']
    gaps = [(6, 8), (15, 18), (24, 28), (33, 38), (42, 48), (51, 58)]
    for y in range(max(0, cy - r), min(HZ, cy + r + 1)):
        dy = y - cy
        if any(a <= dy < b for a, b in gaps):
            continue
        hw = math.sqrt(max(0, r * r - dy * dy))
        for x in range(int(cx - hw + 0.5), int(cx + hw + 0.5)):
            img[y][x] = 3
    landmark(img, sf)
    if sf == 4:
        ground_beach(img, ph)
        palms(img)
    else:
        ground_court(img, sf)
    return img


def ground_beach(img, ph):
    # sol (2.76, à la demande : un vrai sol plutôt qu'une grille) : la mer
    # jusqu'au rivage, avec le reflet du soleil en traits brisés (indice 2,
    # scintillant pendant le plan), puis le sable (indice 3), plus sombre
    # au premier plan (tramé de noir)
    rs = 777
    for y in range(HZ, H):
        d = (y - HZ) / 96.0
        shore = 196 + int(5 * math.sin(y * 0.0 + 0.0))
        for x in range(W):
            sh = 194 + int(4 * math.sin(x / 23.0) + 3 * math.sin(x / 9.0 + 1.0))
            if y < sh:
                img[y][x] = 1                             # mer
            else:
                k = (y - sh) / float(H - sh)              # 0 au rivage, 1 en bas
                img[y][x] = 0 if k * 12 > BAYER[y & 3][x & 3] + 4 else 3
            if y == sh or y == sh - 1:
                img[y][x] = 2 if (x // 3 + y) % 5 else 1   # écume
    # vaguelettes : traits courts, plus longs et espacés vers le rivage
    for y in range(HZ + 2, 192, 3):
        d = (y - HZ) / 48.0
        ln = int(2 + 6 * d)
        rs = (rs * 1103515245 + 12345) & 0x7FFFFFFF
        x = rs % 48
        while x < W:
            for i in range(ln):
                if 0 <= x + i < W and img[y][x + i] == 1:
                    img[y][x + i] = 0
            x += int(30 + 50 * d) + (rs >> 8) % 23
    # reflet du soleil : colonne de traits brisés, plus large au rivage
    hw0 = ph['r'] * 0.9 if ph['cy'] - ph['r'] < HZ else 0
    if ph['cy'] >= HZ:
        hw0 = math.sqrt(max(0, ph['r'] ** 2 - (HZ - ph['cy']) ** 2)) * 0.9
    for y in range(HZ + 1, 192):
        d = (y - HZ) / 48.0
        hw = hw0 * (0.55 + 0.45 * d)
        if (y - HZ) % 3 == 2:
            continue
        rs = (rs * 1103515245 + 12345) & 0x7FFFFFFF
        x = int(128 - hw)
        while x < 128 + hw:
            seg = 3 + (rs >> 5) % 10
            for i in range(seg):
                if x + i < 128 + hw and img[y][x + i] in (0, 1):
                    img[y][x + i] = 2
            rs = (rs * 1103515245 + 12345) & 0x7FFFFFFF
            x += seg + 1 + (rs >> 9) % 5


def ground_court(img, sf):
    """court vu du fond, derrière la ligne de fond (perspective), abords en
    couleur sombre (1), surface (2), lignes et bande du filet (3)"""
    F, CH, Z0 = 230.0, 2.5, 6.0              # focale, hauteur de l'oeil, recul

    def py(z):
        return HZ + F * CH / z

    def px(X, z):
        return 128 + F * X / z
    zb0, zb1, zn = Z0, Z0 + 23.77, Z0 + 11.885
    yb1 = py(zb1)
    for y in range(HZ, H):
        for x in range(W):
            img[y][x] = 1 if (y < HZ + 3 or BAYER[y & 3][x & 3] > 1 or sf == 5) else 0
    # surface du court (couloirs compris), de la ligne de fond lointaine à nous
    for y in range(int(yb1), H):
        z = F * CH / (y - HZ)
        xl, xr = px(-5.485 - 1.5, z), px(5.485 + 1.5, z)
        for x in range(max(0, int(xl)), min(W, int(xr) + 1)):
            img[y][x] = 2
    # lignes (3) : longitudinales, puis transversales
    def lng(X):
        for y in range(int(yb1), H):
            z = F * CH / (y - HZ)
            x = int(round(px(X, z)))
            if 0 <= x < W:
                img[y][x] = 3
    for X in (-5.485, -4.115, 4.115, 5.485):
        lng(X)
    for y in range(int(py(zb1 - 6.40)), int(py(zb0 + 6.40)) + 1):
        z = F * CH / (y - HZ)
        x = int(round(px(0, z)))
        img[y][x] = 3
    def tr(zz, X0, X1):
        y = int(round(py(zz)))
        if HZ < y < H:
            for x in range(max(0, int(px(X0, zz))), min(W, int(px(X1, zz)) + 1)):
                img[y][x] = 3
    tr(zb1, -5.485, 5.485)
    tr(zb1 - 6.40, -4.115, 4.115)
    tr(zb0 + 6.40, -4.115, 4.115)
    tr(zb0 + 0.02, -5.485, 5.485)
    # filet : bande tramée (noir / abords), bande blanche en haut, poteaux
    yb = py(zn)
    yt = HZ + F * (CH - 0.914) / zn
    xl, xr = px(-6.4, zn), px(6.4, zn)
    for y in range(int(yt), int(yb) + 1):
        for x in range(int(xl), int(xr) + 1):
            img[y][x] = 3 if y == int(yt) else (0 if (x + y) & 1 else img[y][x] if img[y][x] != 3 else 1)
    for xp in (int(xl), int(xr)):
        for y in range(int(yt) - 1, int(yb) + 1):
            img[y][xp] = 3


def landmark(img, sf):
    """repère du lieu à l'horizon, en ombre chinoise (0), fenêtres ou neige
    en couleur 3 (soleil) ou 2 (ciel clair)"""
    def rect(x0, y0, x1, y1, c=0):
        for y in range(max(0, int(y0)), min(HZ, int(y1))):
            for x in range(max(0, int(x0)), min(W, int(x1))):
                img[y][x] = c
    if sf == 0:                                    # Debogarros : la tour Eiffel
        cx, base, top = 206, HZ, 50
        for y in range(top, base):
            t = (y - top) / float(base - top)
            hw = 1 + 30 * t ** 2.2
            if 0.62 < t < 0.98 and abs((y - top) - 0.80 * (base - top)) < 0.18 * (base - top):
                # grande arche
                ax = hw * (1 - ((y - top - 0.98 * (base - top)) / (0.36 * (base - top))) ** 2) ** 0.5 * 0.7
                rect(cx - hw, y, cx - ax, y + 1)
                rect(cx + ax, y, cx + hw + 1, y + 1)
            else:
                rect(cx - hw, y, cx + hw + 1, y + 1)
        rect(cx - 12, top + 0.55 * (base - top), cx + 13, top + 0.55 * (base - top) + 2)
        rect(cx - 21, top + 0.78 * (base - top), cx + 22, top + 0.78 * (base - top) + 3)
        rect(cx, top - 8, cx + 1, top)
        rect(0, HZ - 6, W, HZ)                     # toits de Paris
        for x in range(0, W, 9):
            rect(x, HZ - 9 - (x * 7 % 5), x + 6, HZ)
    elif sf == 1:                                  # Martinbledon : Big Ben
        cx = 40
        rect(cx - 7, 60, cx + 8, HZ)
        rect(cx - 9, 70, cx + 10, 88)
        for y in range(40, 60):
            hw = (y - 40) * 7 / 20.0
            rect(cx - hw, y, cx + hw + 1, y + 1)
        rect(cx, 32, cx + 1, 40)
        for y in range(73, 85):                    # cadran
            for x in range(cx - 6, cx + 7):
                if (x - cx) ** 2 + (y - 79) ** 2 <= 26:
                    img[y][x] = 3
        rect(cx + 8, HZ - 30, cx + 90, HZ)         # parlement
        for x in range(cx + 10, cx + 90, 8):
            rect(x, HZ - 36, x + 3, HZ - 30)
        rect(0, HZ - 14, W, HZ)                    # arbres
        for x in range(0, W, 7):
            r = 5 + (x * 13) % 6
            for y in range(HZ - 14 - r, HZ - 14):
                for xx in range(x - r, x + r):
                    if 0 <= xx < W and (xx - x) ** 2 + (y - (HZ - 14)) ** 2 <= r * r:
                        img[y][xx] = 0
    elif sf in (2, 3):                             # US Opaul, Augustralie : gratte-ciel
        x, k = 0, 3
        while x < W:
            k = (k * 37 + 11) % 101
            w = 10 + k % 14
            h = 14 + (k * 7) % 46
            if 96 < x < 150:
                h = h // 2                         # (le soleil reste visible)
            rect(x, HZ - h, x + w, HZ)
            if k % 3 == 0:
                rect(x + w // 2, HZ - h - 8, x + w // 2 + 1, HZ - h)
            for wy in range(HZ - h + 3, HZ - 3, 5):    # fenêtres allumées
                for wx in range(x + 2, x + w - 2, 4):
                    if wx < W and (wx * 3 + wy * 5 + k) % 7 == 0:
                        img[wy][wx] = 3
            x += w + 1
    elif sf == 5:                                  # Margaux-les-Neiges : montagnes
        for x in range(W):
            h1 = 40 + 26 * math.sin(x / 31.0 + 0.5) + 14 * math.sin(x / 11.0)
            h2 = 22 + 10 * math.sin(x / 17.0 + 2.0)
            cut = 0.4 + 0.6 * min(1.0, abs(x - 128) / 80.0)   # (col au centre : le soleil reste visible)
            h1, h2 = h1 * cut, h2 * cut
            h = max(h1, h2)
            for y in range(int(HZ - h), HZ):
                img[y][x] = 0
            if h1 > 48:                            # neiges : couleur 2 (lueur du ciel)
                for y in range(int(HZ - h1), int(HZ - h1 + (h1 - 46) * 0.6)):
                    if y >= 96:
                        img[y][x] = 2
                    elif y >= 48:
                        img[y][x] = 2
    elif sf == 4:                                  # Rio de Bruneiro : le Pain de Sucre
        for x in range(W):
            h = 0
            d = (x - 52) / 22.0
            if abs(d) < 1.6:
                h = max(h, 54 * max(0, 1 - d * d / 2.6))
            d = (x - 92) / 26.0
            if abs(d) < 1.5:
                h = max(h, 26 * max(0, 1 - d * d / 2.2))
            for y in range(int(HZ - h), HZ):
                img[y][x] = 0



def tiles_of(img, limit=250):
    """tuiles uniques ; au-delà de la limite, fusion des tuiles les plus
    proches (pixels différents, pondérés par l'usage), comme un encodeur"""
    import numpy as np
    tiles, idx, nt, use = [], {}, [], []
    for ty in range(30):
        for tx in range(32):
            t = tuple(tuple(img[ty * 8 + y][tx * 8 + x] for x in range(8)) for y in range(8))
            if t not in idx:
                idx[t] = len(tiles)
                tiles.append(t)
                use.append(0)
            use[idx[t]] += 1
            nt.append(idx[t])
    alive = list(range(len(tiles)))
    remap = list(range(len(tiles)))
    arr = np.array([np.array(t).reshape(64) for t in tiles])
    while len(alive) > limit:
        A = arr[alive]
        d = (A[:, None, :] != A[None, :, :]).sum(axis=2).astype(float)
        u = np.array([use[i] for i in alive], dtype=float)
        cost = d * np.minimum(u[:, None], u[None, :])
        np.fill_diagonal(cost, 1e18)
        k = int(np.argmin(cost))
        i, j = divmod(k, len(alive))
        a, b = alive[i], alive[j]
        if use[a] < use[b]:
            a, b = b, a                     # b (moins utilisée) disparaît dans a
        use[a] += use[b]
        for q in range(len(remap)):
            if remap[q] == b:
                remap[q] = a
        alive.remove(b)
    pos = {t: n for n, t in enumerate(alive)}
    out = [tiles[t] for t in alive]
    nt = [pos[remap[i]] for i in nt]
    return out, nt


def attrs():
    at = []
    for ay in range(8):
        for ax in range(8):
            v = 0
            for q, (dx, dy) in enumerate(((0, 0), (1, 0), (0, 1), (1, 1))):
                y = (ay * 32 + dy * 16)
                p = pal_row(min(y, 239)) if y < 240 else 3
                v |= p << (q * 2)
            at.append(v)
    return at


# sol des courts : abords (sombres), surface, lignes ; le soir les assombrit
COURT = {0: (0x05, 0x26), 1: (0x0A, 0x2A), 2: (0x02, 0x12), 3: (0x03, 0x13), 5: (0x12, 0x21)}


def ground_pal(k, ph, sf):
    if sf == 4:
        return ph['grid']                          # plage : mer, reflet, sable
    d, c = COURT[sf]
    if k == 0:
        return [d, c, 0x30]
    if k == 1:
        return [d, c - 0x10 if c >= 0x10 else c, 0x30]
    return [0x0F, d, 0x10]


def palette(ph, k=0, sf=4):
    s, u, g = ph['sky'], ph['sun'], ground_pal(k, ph, sf)
    return [0x0F, s[1], s[2], u[0],
            0x0F, s[2], s[3], u[1],
            0x0F, s[3], s[4], u[2],
            0x0F, g[0], g[1], g[2]]


def chr_bytes(t):
    out = []
    for plane in (0, 1):
        for y in range(8):
            b = 0
            for x in range(8):
                if (t[y][x] >> plane) & 1:
                    b |= 0x80 >> x
            out.append(b)
    return out


NSF = 6                                         # surfaces (gfx.c : surf_name)
SURF_NM = ['terre battue', 'gazon', 'dur', 'moquette', 'plage', 'glace']
BANK0, PER_BANK = 22, 3                         # images par banque (~5 Ko chacune)


def main():
    files = {}
    idx = []                                    # (banque, nom, tuiles, palette)
    prev = []
    for sf in range(NSF):
        for k, ph in enumerate(PHASES):
            n = sf * 3 + k
            bank = BANK0 + n // PER_BANK
            img = build(ph, sf)
            tiles, nt = tiles_of(img)
            assert len(tiles) <= 250, (sf, k, len(tiles))
            data = []
            for t in tiles:
                data += chr_bytes(t)
            nm = 'sun_c%d' % n
            files.setdefault(bank, []).append('const u8 %s[%d] = {%s};' % (nm, len(data), ','.join(str(v) for v in data)))
            files[bank].append('const u8 sun_n%d[960] = {%s};' % (n, ','.join(str(v) for v in nt)))
            idx.append((bank, n, len(tiles), palette(ph, k, sf)))
            prev.append(([[tiles[nt[(y >> 3) * 32 + (x >> 3)]][y & 7][x & 7] for x in range(W)] for y in range(H)], palette(ph, k, sf)))
            print('soleil %-12s phase %d : %d tuiles (banque %d)' % (SURF_NM[sf], k, len(tiles), bank))
    hdr = ['/* généré par tools/gen_sun.py : ne pas modifier */', '#define u8 unsigned char', '']
    for bank, lines in files.items():
        fn = 'assets_sun.c' if bank == BANK0 else 'assets_sun%d.c' % bank
        out = list(hdr) + lines
        if bank == BANK0:
            n = len(idx)
            out += ['extern const u8 %s[];' % ('sun_c%d' % i) for i in range(n) if idx[i][0] != BANK0]
            out += ['extern const u8 %s[];' % ('sun_n%d' % i) for i in range(n) if idx[i][0] != BANK0]
            out.append('const u8 sun_at[64] = {%s};' % ','.join(str(b) for b in attrs()))
            out.append('const u8 sun_pal[%d][16] = {%s};' % (n, ','.join('{' + ','.join('0x%02X' % c for c in e[3]) + '}' for e in idx)))
            out.append('const u8 sun_ntl[%d] = {%s};' % (n, ','.join(str(e[2]) for e in idx)))
            out.append('const u8 sun_bk[%d] = {%s};' % (n, ','.join(str(e[0]) for e in idx)))
            out.append('const u8 * const sun_chr[%d] = {%s};' % (n, ','.join('sun_c%d' % i for i in range(n))))
            out.append('const u8 * const sun_nt[%d] = {%s};' % (n, ','.join('sun_n%d' % i for i in range(n))))
        with open(os.path.join(ROOT, 'src', fn), 'w') as f:
            f.write('\n'.join(out) + '\n')
    try:
        from PIL import Image
        im = Image.new('RGB', ((W + 8) * 3, (H + 8) * NSF), (255, 255, 255))
        for n, (img, pal) in enumerate(prev):
            ox, oy = (n % 3) * (W + 8), (n // 3) * (H + 8)
            for y in range(H):
                p = pal_row(y)
                for x in range(W):
                    c = img[y][x]
                    im.putpixel((ox + x, oy + y), NES[pal[p * 4 + c] if c else 0x0F])
        im.save(os.path.join(ROOT, 'docs', 'soleil_276.png'))
    except ImportError:
        pass


if __name__ == '__main__':
    main()
