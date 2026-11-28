#!/usr/bin/env python3
"""Royal Martinot — personnages en haute définition de la bande-annonce
(version 2.57) : dessinés en formes vectorielles (membres effilés, visage,
ombres, raquette cordée), rendus pixel par pixel puis découpés en tuiles du
décor, avec 3 palettes du décor par personnage (peau et son ombre, tenue et son
ombre, sombre : cheveux, short, semelles ; blanc : chaussettes, chaussures,
cordage). Les mêmes dessins, rendus à une échelle plus grande, donnent les
plans rapprochés (zoom).

Classes de pixels : 0 contour (couleur du fond), 1 peau, 2 peau ombrée,
3 tenue, 4 tenue ombrée, 5 sombre, 6 blanc.
Palettes du décor : P1 = peau, peau ombrée, sombre ; P2 = tenue, tenue
ombrée, sombre ; P3 = peau, tenue, blanc (P0 : bandes du fond).

Sortie : src/assets_tz.h (descripteurs, inclus par teaser.c) et
src/assets_tz<b>.c (tuiles, tables de noms, attributs) dans les banques
libres ; assets/preview_teaser.png (aperçu)."""
import math, os, sys
import numpy as np
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
PAL = [
 (84,84,84),(0,30,116),(8,16,144),(48,0,136),(68,0,100),(92,0,48),(84,4,0),(60,24,0),
 (32,42,0),(8,58,0),(0,64,0),(0,60,0),(0,50,60),(0,0,0),(0,0,0),(0,0,0),
 (152,150,152),(8,76,196),(48,50,236),(92,30,228),(136,20,176),(160,20,100),(152,34,32),(120,60,0),
 (84,90,0),(40,114,0),(8,124,0),(0,118,40),(0,102,120),(0,0,0),(0,0,0),(0,0,0),
 (236,238,236),(76,154,236),(120,124,236),(176,98,236),(228,84,236),(236,88,180),(236,106,100),(212,136,32),
 (160,170,0),(116,196,0),(76,208,32),(56,204,108),(56,180,204),(60,60,60),(0,0,0),(0,0,0),
 (236,238,236),(168,204,236),(188,188,236),(212,178,236),(236,174,236),(236,174,212),(236,180,176),(228,196,144),
 (204,210,120),(180,222,120),(168,226,144),(152,226,180),(160,214,228),(160,162,160),(0,0,0),(0,0,0)]

NUDE = [False]             # (2.79) club nudiste : corps sans vêtements (le torse en peau, ni jupe ni short)
GROUND_ROW = 28            # première rangée du sol (y = 224) : bande du bas
FIRST_TILE = 1             # tuiles du personnage : 1 à MAX_TILES
MAX_TILES = 0xF6           # $F7-$FF : bandes, sol, filet (teaser.c)
T_STREAK, T_GROUND = 0xF8, 0xF9

# ------------------------------------------------------------------ dessin
class Fig:
    """Personnage rendu dans un cadre de W x H pixels ; transformation
    (x, y) -> ((x - ox) * k, (y - oy) * k) des coordonnées de base."""
    def __init__(self, W, H, k=1.0, ox=0.0, oy=0.0):
        self.W, self.H, self.k, self.ox, self.oy = W, H, k, ox, oy
        self.cls = np.zeros((H, W), np.uint8)
        self.pid = np.zeros((H, W), np.int32)
        self.grp = {0: 0}
        self.n = 0

    def T(self, p):
        return ((p[0] - self.ox) * self.k, (p[1] - self.oy) * self.k)

    def mask(self, fn):
        im = Image.new('L', (self.W, self.H), 0)
        fn(ImageDraw.Draw(im))
        return np.array(im) > 0

    # primitives (coordonnées de base)
    def m_poly(self, pts):
        q = [self.T(p) for p in pts]
        return self.mask(lambda d: d.polygon(q, fill=255))

    def m_ell(self, c, rx, ry, ang=0.0):
        pts = []
        for i in range(48):
            t = 2 * math.pi * i / 48
            x, y = rx * math.cos(t), ry * math.sin(t)
            ca, sa = math.cos(ang), math.sin(ang)
            pts.append((c[0] + x * ca - y * sa, c[1] + x * sa + y * ca))
        return self.m_poly(pts)

    def m_caps(self, a, b, ra, rb):
        """membre effilé : de a (rayon ra) à b (rayon rb)"""
        dx, dy = b[0] - a[0], b[1] - a[1]
        L = math.hypot(dx, dy) or 1
        nx, ny = -dy / L, dx / L
        pts = [(a[0] + nx * ra, a[1] + ny * ra), (b[0] + nx * rb, b[1] + ny * rb),
               (b[0] - nx * rb, b[1] - ny * rb), (a[0] - nx * ra, a[1] - ny * ra)]
        return self.m_poly(pts) | self.m_ell(a, ra, ra) | self.m_ell(b, rb, rb)

    def m_line(self, pts, w=1.0):
        q = [self.T(p) for p in pts]
        ww = max(1, int(round(w * self.k)))
        return self.mask(lambda d: d.line(q, fill=255, width=ww, joint='curve'))

    def m_arc(self, c, rx, ry, a0, a1, w=1.0):
        pts = [(c[0] + rx * math.cos(math.radians(a)), c[1] + ry * math.sin(math.radians(a)))
               for a in np.linspace(a0, a1, 16)]
        return self.m_line(pts, w)

    # couches
    def part(self, m, base, shade=None, sh=(3, 1.5), group=None):
        """nouvelle pièce, devant les précédentes ; ombre sur le bord en bas à
        droite (lumière en haut à gauche) sur une épaisseur sh"""
        self.n += 1
        self.grp[self.n] = group if group is not None else self.n
        self.pid[m] = self.n
        self.cls[m] = base
        if shade is not None:
            dx, dy = int(round(sh[0] * self.k)), int(round(sh[1] * self.k))
            s = np.zeros_like(m)
            s[:self.H - dy, :self.W - dx] = m[dy:, dx:]
            self.cls[m & ~s] = shade
        return self.n

    def paint(self, m, c, within=None):
        """détail peint (sans nouvelle pièce), éventuellement limité à une pièce"""
        if within is not None:
            m = m & np.isin(self.pid, within)
        self.cls[m] = c

    def outline(self):
        """contour : un pixel touche une pièce située derrière (ou le fond)"""
        H, W = self.H, self.W
        g = np.vectorize(lambda v: self.grp.get(v, v))(self.pid)
        out = np.zeros((H, W), bool)
        for dy, dx in ((0, 1), (0, -1), (1, 0), (-1, 0)):
            sp = np.zeros_like(self.pid); sg = np.zeros_like(g)
            ys = slice(max(dy, 0), H + min(dy, 0)); yd = slice(max(-dy, 0), H + min(-dy, 0))
            xs = slice(max(dx, 0), W + min(dx, 0)); xd = slice(max(-dx, 0), W + min(-dx, 0))
            sp[yd, xd] = self.pid[ys, xs]; sg[yd, xd] = g[ys, xs]
            out |= (self.pid > 0) & (sp < self.pid) & (sg != g)
        self.cls[out] = 0


class HS:
    """dessin de la tête à l'échelle hs (rayons et épaisseurs)"""
    def __init__(self, f, hs):
        self.f, self.hs, self.k = f, hs, f.k
    def m_ell(self, c, rx, ry, ang=0.0): return self.f.m_ell(c, rx * self.hs, ry * self.hs, ang)
    def m_caps(self, a, b, ra, rb): return self.f.m_caps(a, b, ra * self.hs, rb * self.hs)
    def m_arc(self, c, rx, ry, a0, a1, w=1.0): return self.f.m_arc(c, rx * self.hs, ry * self.hs, a0, a1, w)
    def m_line(self, pts, w=1.0): return self.f.m_line(pts, w)
    def m_poly(self, pts): return self.f.m_poly(pts)
    def part(self, *a, **kw): return self.f.part(*a, **kw)
    def paint(self, *a, **kw): return self.f.paint(*a, **kw)
    def __getattr__(self, n): return getattr(self.f, n)


# ------------------------------------------------------------------ corps
CASUAL = [False]      # (2.74) tenue de ville : t-shirt sans col (public)


def body(f, j, female, face, front=('armR', 'legL'), racket_hand='L', racket_dir=None,
         racket_len=50, racket_bend=0.0, ret=None):
    """j : articulations (repère de base, sol en y = 206). 'L'/'R' : côté de
    l'écran (le joueur, de face, tient la raquette dans sa main droite, à
    gauche de l'écran)."""
    S, s, O, o, D, W = 1, 2, 3, 4, 5, 6
    ret = {} if ret is None else ret
    j = dict(j)
    LW = 0.88 if female else 1.0
    AW = 0.82 if female else 1.0

    def leg(side):
        hip, kn, an, toe = j['hip' + side], j['kn' + side], j['an' + side], j['toe' + side]
        if j.get('kneel'):
            # (2.76) à genoux, assis sur les talons, vu de face : la cuisse
            # vient vers la caméra, le genou rond posé au sol ; tibia et pied
            # sont cachés sous le corps
            # (cuisses pleines, écrasées sur les talons : plus larges qu'en pied)
            thigh = f.m_caps(hip, kn, 11.0 if female else 12.5, 9.2 if female else 10.4)
            knee = f.m_ell(kn, 8.6 if female else 9.8, 6.8 if female else 7.6)
            g = f.part(thigh | knee, S, s, sh=(2, 1))
            ret['leg' + side] = g
            f.paint(f.m_arc((kn[0], kn[1] + 1.0), 4.0, 2.4, 200, 340, 0.7), 2, within=[g])   # rotule
            ret['foot' + side] = (kn[0], kn[1] + 6)
            return
        if female:
            # jambe fine et galbée, sans relief musculaire
            thigh = f.m_caps(hip, kn, 8.0, 4.8)
            cm = lerp(kn, an, 0.32)
            calf = f.m_caps(kn, cm, 4.5, 5.5) | f.m_caps(cm, lerp(cm, an, 0.55), 5.5, 3.9) | f.m_caps(lerp(cm, an, 0.55), an, 3.9, 2.9)
            g = f.part(thigh | calf, S, s, sh=(2, 1))
            ret['leg' + side] = g
        else:
            thigh = f.m_caps(hip, kn, 9.5, 7)
            calf = f.m_caps(kn, lerp(kn, an, 0.35), 7, 6.3) | f.m_caps(lerp(kn, an, 0.35), an, 6.3, 4.0)
            g = f.part(thigh | calf, S, s, sh=(3, 1))
            ret['leg' + side] = g
            d = (an[0] - kn[0], an[1] - kn[1]); L = math.hypot(*d) or 1; u = (d[0] / L, d[1] / L); n = (-u[1], u[0])
            c = (kn[0] + u[0] * 2.5, kn[1] + u[1] * 2.5)
            f.paint(f.m_line([(c[0] - n[0] * 3, c[1] - n[1] * 3), (c[0] + u[0] * 1.2, c[1] + u[1] * 1.2), (c[0] + n[0] * 3, c[1] + n[1] * 3)], 0.7), 2, within=[g])   # rotule
            f.paint(f.m_line([lerp(kn, an, 0.12), lerp(kn, an, 0.45)], 0.7) & shift_mask(f, (2.5, 0), calf), 2, within=[g])  # mollet
        # chaussette et chaussure
        sock = f.m_caps(lerp(an, kn, 0.14 if female else 0.18), an, 3.8 if female else 4.6, 3.8 if female else 4.6)
        f.part(sock, W, None, group=g)
        d = (toe[0] - an[0], toe[1] - an[1])
        L = math.hypot(*d) or 1
        u = (d[0] / L, d[1] / L)
        toe = (toe[0] + u[0] * 3.5, toe[1] + u[1] * 3.5)        # pied plus long (chaussure un peu plus grande, 2.75)
        n = (u[1], -u[0]) if u[0] >= 0 else (-u[1], u[0])          # vers le haut
        Q = lambda t, h: (an[0] + u[0] * t + n[0] * h, an[1] + u[1] * t + n[1] * h)
        L = math.hypot(toe[0] - an[0], toe[1] - an[1])
        shoe = f.m_poly([Q(-7, 6), Q(-1, 8.2), Q(L * 0.45, 5.4), Q(L + 1.5, 2.8), Q(L + 3.8, -1.2), Q(L + 1.5, -4.2), Q(-7, -4.2), Q(-8.6, 1)])
        shoe |= f.m_ell(Q(L, -0.7), 3.9, 3.4)
        sid = f.part(shoe, W, None)
        sole = shoe & ~shift_mask(f, (-n[0] * 2.2, -n[1] * 2.2), shoe)
        f.paint(sole, D, within=[sid])
        f.paint(f.m_line([Q(-4, 0.5), Q(L * 0.35, 2.2), Q(L * 0.7, 0.2)], 0.9), 3, within=[sid])   # bande de la chaussure
        for t in (0.25, 0.45):                                    # lacets
            f.paint(f.m_line([Q(L * t, 5.5), Q(L * t + 2.5, 2.5)], 0.5), 0, within=[sid])
        ret['foot' + side] = (toe[0], toe[1] + 5)

    def arm(side, hold):
        sh_, el, wr = j['sh' + side], j['el' + side], j['wr' + side]
        g = f.part(f.m_caps(sh_, el, 6.5 * AW, 5.2 * AW) | f.m_caps(el, wr, 5.2 * AW, 3.8 * AW), S, s, sh=(2.5, 1.5))
        if not female and not NUDE[0]:
            # manche courte du polo
            f.part(f.m_caps(sh_, lerp(sh_, el, 0.36), 7.4, 6.8), O, o, sh=(2.5, 1.5), group=g)
        hand = j.get('hd' + side, lerp(el, wr, 1.25))
        if hold:
            # poignet éponge blanc
            f.part(f.m_caps(lerp(el, wr, 0.78), wr, 4.6, 4.4), W, None, group=g)
        if hold:
            ret['hand'] = hand
        if True:
            # poing (2.73) : dans l'axe de l'avant-bras, plus large aux doigts,
            # trois doigts repliés marqués, pouce replié sur le côté (aussi la
            # main qui tient la raquette : ses doigts serrent le manche)
            d = (hand[0] - wr[0], hand[1] - wr[1]); L = math.hypot(*d) or 1; u = (d[0] / L, d[1] / L)
            n = (-u[1], u[0])
            ts = 1 if (side == 'R') == (u[1] < 0.3) else -1
            Pk = lambda t, k: (hand[0] + u[0] * t + n[0] * k, hand[1] + u[1] * t + n[1] * k)
            fist = f.m_caps(Pk(-2.5, 0), Pk(1.6, 0), 4.1 * AW, 5.0 * AW) | f.m_ell(Pk(1.8, 0), 5.0 * AW, 5.0 * AW)
            h = f.part(fist, S, s, sh=(1.5, 1.5), group=g)
            for k in (-2.2, 0.0, 2.2):                     # séparations des doigts
                f.paint(f.m_line([Pk(2.6 * AW, k * AW), Pk(6.2 * AW, k * AW)], 0.45), 2, within=[h])
            f.paint(f.m_line([Pk(-1.8, ts * 3.6 * AW), Pk(2.4 * AW, ts * 1.2 * AW)], 0.75), 2, within=[h])   # pouce

    def racket():
        hand = j['hd' + racket_hand]
        a = math.radians(racket_dir)
        u = (math.cos(a), math.sin(a))
        nrm = (-u[1], u[0])
        throat = (hand[0] + u[0] * 17, hand[1] + u[1] * 17)
        cen = (hand[0] + u[0] * (racket_len - 14), hand[1] + u[1] * (racket_len - 14))
        # manche (sombre), coeur (tenue), tamis (tenue), cordage blanc
        f.part(f.m_caps((hand[0] - u[0] * 7, hand[1] - u[1] * 7), throat, 2.4, 2.0), D, None)
        f.part(f.m_line([throat, (cen[0] + nrm[0] * 9 - u[0] * 13, cen[1] + nrm[1] * 9 - u[1] * 13)], 2.2)
               | f.m_line([throat, (cen[0] - nrm[0] * 9 - u[0] * 13, cen[1] - nrm[1] * 9 - u[1] * 13)], 2.2), O, None)
        outer = f.m_ell(cen, 15, 11.5, a)
        inner = f.m_ell(cen, 12.6, 9.2, a)
        rid = f.part(outer, O, o, sh=(1.5, 1.5))
        f.paint(inner, 0, within=[rid])
        st = np.zeros_like(inner)
        for t in range(-12, 13, 3):
            st |= f.m_line([(cen[0] + u[0] * t - nrm[0] * 12, cen[1] + u[1] * t - nrm[1] * 12),
                            (cen[0] + u[0] * t + nrm[0] * 12, cen[1] + u[1] * t + nrm[1] * 12)], 0.6)
        for t in range(-9, 10, 3):
            st |= f.m_line([(cen[0] + nrm[0] * t - u[0] * 15, cen[1] + nrm[1] * t - u[1] * 15),
                            (cen[0] + nrm[0] * t + u[0] * 15, cen[1] + nrm[1] * t + u[1] * 15)], 0.6)
        f.paint(st & inner, W, within=[rid])
        ret['racket'] = cen
        ret['racket_tip'] = (cen[0] + u[0] * 15, cen[1] + u[1] * 15)

    def torso():
        shL, shR, hpL, hpR = j['shL'], j['shR'], j['hipL'], j['hipR']
        nk = j['neck']
        # cou (ombre sous le menton)
        hd = j['head']; tl = j.get('tilt', 0.0)
        chin = (hd[0] - math.sin(tl) * 6, hd[1] + math.cos(tl) * 6)                # sous le menton, dans l'axe de la tête
        nid = f.part(f.m_caps(nk, chin, (4.6 if female else 6.0), (4.2 if female else 5.8)), S, s, sh=(2.5, 0))
        f.paint(f.m_ell((chin[0], chin[1] + 1.5), 5.2, 2.6), 2, within=[nid])          # ombre sous le menton
        ch_l = lerp(shL, hpL, 0.36); ch_r = lerp(shR, hpR, 0.36)
        wl = lerp(shL, hpL, 0.68); wr = lerp(shR, hpR, 0.68)
        if female:
            # épaules nues, robe de tennis sans manches : bretelles, encolure
            # arrondie, poitrine suggérée par un pli, taille marquée, jupe plissée
            f.part(f.m_poly([(shL[0] - 4, shL[1] + 1), (shL[0], shL[1] - 4), (nk[0], nk[1] - 2), (shR[0], shR[1] - 4),
                             (shR[0] + 4, shR[1] + 1), (ch_r[0], ch_r[1] - 6), (ch_l[0], ch_l[1] - 6)]), S, s, sh=(3, 1), group=nid)
            # profil en courbe douce : aisselle, poitrine, dessous de poitrine, taille, hanche
            def side(sh_, hp_, sx):
                P_ = lambda t, dx: (lerp(sh_, hp_, t)[0] + sx * dx, lerp(sh_, hp_, t)[1])
                # (le corsage s'arrête sous la taille, au milieu de la jupe : ses
                # coins, larges sur les hanches, dépassaient du haut de la jupe)
                if female == 2:     # fillette : buste droit
                    return [P_(0.13, -0.5), P_(0.25, 0.5), P_(0.4, 0.3), P_(0.6, -1.0), P_(0.8, 0.5), P_(0.88, -1.5)]
                return [P_(0.13, -0.5), P_(0.22, 3.2), P_(0.31, 4.4), P_(0.4, 3.0), P_(0.5, 0.0),
                        P_(0.6, -2.0), P_(0.7, -2.0), P_(0.8, 0.5), P_(0.88, -1.5)]
            # bretelles : elles montent jusqu'en haut de l'épaule, le long de sa pente, jusqu'au cou
            pts = [(shL[0] + 0.5, shL[1] - 4.8), (nk[0] - 6.6, nk[1] - 2.9), (nk[0] - 4.0, nk[1] - 1.6),
                   (nk[0] - 3.6, nk[1] + 7), (nk[0], nk[1] + 9), (nk[0] + 3.6, nk[1] + 7),
                   (nk[0] + 4.0, nk[1] - 1.6), (nk[0] + 6.6, nk[1] - 2.9), (shR[0] - 0.5, shR[1] - 4.8), (shR[0] + 1, shR[1] + 3)]
            pts += side(shR, hpR, 1) + list(reversed(side(shL, hpL, -1))) + [(shL[0] - 1, shL[1] + 3)]
            tid = f.part(f.m_poly(pts), S if NUDE[0] else O, s if NUDE[0] else o, sh=(4, 2))
            if NUDE[0]:                 # (club nudiste : torse nu, la poitrine sera masquée par la mosaïque)
                ret['dress'] = tid
                return
            # poitrine : volume rond, ombre douce sous chaque sein et sur le côté
            # (pas pour la fillette, female == 2 : Margaux)
            bx = abs(hpR[0] - hpL[0]) * 0.27
            by = lerp(lerp(shL, shR, 0.5), lerp(hpL, hpR, 0.5), 0.26)[1]
            for sx in ((-1, 1) if female == 1 else ()):
                c_ = (nk[0] + sx * bx, by)
                e_ = f.m_ell(c_, bx * 1.18, 7.2)
                low = ~f.m_poly([(c_[0] - 20, c_[1] - 20), (c_[0] + 20, c_[1] - 20), (c_[0] + 20, c_[1] + 1.5), (c_[0] - 20, c_[1] + 1.5)])
                mid = ~f.m_poly([(nk[0] - 1.6, c_[1] - 20), (nk[0] + 1.6, c_[1] - 20), (nk[0] + 1.6, c_[1] + 20), (nk[0] - 1.6, c_[1] + 20)])
                if VS_LINE:     # affiche VS : contour arrondi (côté et dessous du sein), lisible en gros plan
                    ring = e_ & ~f.m_ell((c_[0], c_[1] - 1.6), bx * 1.18 - 1.0, 7.2 - 1.0)
                    half = ~f.m_poly([(c_[0] - 20, c_[1] - 20), (c_[0] + 20, c_[1] - 20), (c_[0] + 20, c_[1] - 0.5), (c_[0] - 20, c_[1] - 0.5)])
                    yy, xx = np.mgrid[0:f.H, 0:f.W]
                    f.paint(ring & half & mid & ((xx + yy) % 2 == 0), 0, within=[tid])   # trait tramé : contour fondu
                else:
                    f.paint(e_ & ~shift_mask(f, (0, 2.2), e_) & low & mid, 4, within=[tid])   # ombre douce sous le sein
            ret['dress'] = tid
            return
        # short ample : jambes droites, plus larges que la cuisse, ourlet net
        m = f.m_poly([(hpL[0] - 6, hpL[1] - 8), (hpR[0] + 6, hpR[1] - 8), (hpR[0] + 9, hpR[1] + 2), (hpL[0] - 9, hpL[1] + 2)])
        hems = []
        for side, sx in (('L', -1), ('R', 1)):
            hp, kn = j['hip' + side], j['kn' + side]
            d = (kn[0] - hp[0], kn[1] - hp[1]); L = math.hypot(*d) or 1; u = (d[0] / L, d[1] / L); n = (-u[1], u[0])
            # (2.75) ourlet à la même distance de la hanche sur les deux jambes,
            # même quand une cuisse est raccourcie (genou levé vers la caméra)
            e = (hp[0] + u[0] * min(SEG['thigh'] * 1.06 * 0.48, L * 0.85), hp[1] + u[1] * min(SEG['thigh'] * 1.06 * 0.48, L * 0.85))
            leg_ = f.m_caps((hp[0] - u[0] * 4, hp[1] - u[1] * 4), e, 11.2, 11.6)
            keep = f.m_poly([(e[0] + n[0] * 40, e[1] + n[1] * 40), (e[0] - n[0] * 40, e[1] - n[1] * 40),
                             (e[0] - n[0] * 40 - u[0] * 90, e[1] - n[1] * 40 - u[1] * 90),
                             (e[0] + n[0] * 40 - u[0] * 90, e[1] + n[1] * 40 - u[1] * 90)])
            m |= leg_ & keep
            hems.append(((e[0] + n[0] * 11, e[1] + n[1] * 11), (e[0] - n[0] * 11, e[1] - n[1] * 11), u))
        if NUDE[0]:
            # club nudiste : ni short ni polo ; bassin et torse en peau
            sid = f.part(f.m_ell((lerp(hpL, hpR, 0.5)[0], lerp(hpL, hpR, 0.5)[1] + 2), abs(hpR[0] - hpL[0]) / 2 + 6, 8), S, s, sh=(3, 1))
            ret['shorts'] = sid
        else:
          sid = f.part(m, D, None)
        for (a_, b_, u_) in ([] if NUDE[0] else hems):
            f.paint(f.m_line([(a_[0] - u_[0] * 2.2, a_[1] - u_[1] * 2.2), (b_[0] - u_[0] * 2.2, b_[1] - u_[1] * 2.2)], 0.6), 0, within=[sid])   # ourlet
        cr = lerp(hpL, hpR, 0.5)
        if not NUDE[0]:
            f.paint(f.m_line([(cr[0], cr[1] + 2), lerp(hems[0][0], hems[1][1], 0.5)], 0.9), 0, within=[sid])   # entrejambe
        # polo, porté par-dessus le short ; flanc droit de la poitrine à la
        # hanche (2.73 : la taille creusée faisait une silhouette en sablier)
        def straight(ch_, dc, hp_, dh, w_):
            a_, b_ = (ch_[0] + dc, ch_[1]), (hp_[0] + dh, hp_[1] - 10)
            t = (w_[1] - a_[1]) / ((b_[1] - a_[1]) or 1)
            x = a_[0] + (b_[0] - a_[0]) * t
            x0 = w_[0] + (2 if dc > 0 else -2)
            x = x - (1.5 if dc > 0 else -1.5)          # (léger retrait au ventre : silhouette athlétique)
            return (x if (x - x0) * dc > 0 else x0, w_[1])
        top_l = (shL[0] - 2, shL[1] - 5); top_r = (shR[0] + 2, shR[1] - 5)
        pts = [top_l, (nk[0] - 6, nk[1] - 1), (nk[0] + 6, nk[1] - 1), top_r, (shR[0] + 5, shR[1] + 2),
               (ch_r[0] + 2, ch_r[1]), straight(ch_r, 2, hpR, 11, wr), (hpR[0] + 11, hpR[1] - 10), (hpR[0] + 11.5, hpR[1] - 5),
               (hpL[0] - 11.5, hpL[1] - 5), (hpL[0] - 11, hpL[1] - 10), straight(ch_l, -2, hpL, -11, wl), (ch_l[0] - 2, ch_l[1]), (shL[0] - 5, shL[1] + 2)]
        tm = f.m_poly(pts)
        if NUDE[0]:
            # torse nu : épaules, aisselles, poitrine, taille légèrement creusée,
            # hanches (pas la forme évasée du polo)
            nat = [(shL[0] - 2, shL[1] - 4.5), (nk[0] - 6, nk[1] - 1), (nk[0] + 6, nk[1] - 1), (shR[0] + 2, shR[1] - 4.5),
                   (shR[0] + 4.5, shR[1] + 2), (ch_r[0] + 1, ch_r[1]), (wr[0] - 0.5, wr[1]), (hpR[0] + 6.5, hpR[1] - 2),
                   (hpR[0] + 6, hpR[1] + 3), (hpL[0] - 6, hpL[1] + 3), (hpL[0] - 6.5, hpL[1] - 2), (wl[0] + 0.5, wl[1]),
                   (ch_l[0] - 1, ch_l[1]), (shL[0] - 4.5, shL[1] + 2)]
            f.part(f.m_poly(nat), S, s, sh=(5, 2))
            return
        tid = f.part(tm, O, o, sh=(5, 2))
        f.paint(tm & ~shift_mask(f, (0, -1.5), tm) & shift_mask(f, (0, 8), tm) | (tm & ~shift_mask(f, (0, -1.5), tm) & ~shift_mask(f, (0, 30), tm)), 4, within=[tid])   # ourlet du polo
        if CASUAL[0]:
            # t-shirt de spectateur (2.74) : encolure ronde, ni col ni boutons
            f.paint(f.m_ell((nk[0], nk[1] + 1), 5.5, 3.2), 1, within=[tid])
        else:
            # col du polo (rabattu) et patte de boutonnage
            f.part(f.m_poly([(nk[0] - 7, nk[1] - 2), (nk[0] - 1, nk[1] + 7), (nk[0] - 3, nk[1] + 9), (nk[0] - 8, nk[1] + 1)])
                   | f.m_poly([(nk[0] + 7, nk[1] - 2), (nk[0] + 1, nk[1] + 7), (nk[0] + 3, nk[1] + 9), (nk[0] + 8, nk[1] + 1)]),
                   O, o, sh=(1, 1), group=tid)
            f.paint(f.m_poly([(nk[0] - 1.5, nk[1] + 2), (nk[0] + 1.5, nk[1] + 2), (nk[0] + 0.5, nk[1] + 8), (nk[0] - 0.5, nk[1] + 8)]), 1, within=[tid])
            f.paint(f.m_line([(nk[0], nk[1] + 8), (nk[0], nk[1] + 15)], 0.8), 4, within=[tid])
        # plis du tissu
        f.paint(f.m_line([lerp(ch_l, wl, 0.2), (lerp(ch_l, wl, 0.2)[0] + 7, lerp(ch_l, wl, 0.2)[1] + 6)], 0.8), 4, within=[tid])
        f.paint(f.m_line([(lerp(wl, wr, 0.3)[0], lerp(wl, wr, 0.3)[1] - 2), (lerp(wl, wr, 0.6)[0], lerp(wl, wr, 0.6)[1] + 1)], 0.8), 4, within=[tid])
        ret['shorts'] = sid

    def skirt():
        shL, shR, hpL, hpR = j['shL'], j['shR'], j['hipL'], j['hipR']
        if NUDE[0]:                     # club nudiste : pas de jupe ; le bassin, en peau
            ret['shorts'] = f.part(f.m_ell(lerp(hpL, hpR, 0.5), abs(hpR[0] - hpL[0]) / 2 + 8, 10), S, s, sh=(3, 1), group=ret['dress'])
            return
        wl = lerp(shL, hpL, 0.68); wr = lerp(shR, hpR, 0.68)
        tid = ret['dress']
        # jupe plissée évasée (même tissu, d'un seul tenant avec le haut) :
        # elle couvre les hanches et le haut des cuisses, sans fente
        kl, kr = j['knL'], j['knR']
        hl, hr = lerp(hpL, kl, 0.4), lerp(hpR, kr, 0.4)
        top_l = lerp(wl, hpL, 0.12); top_r = lerp(wr, hpR, 0.12)
        dl = (hl[0] - hpL[0], hl[1] - hpL[1]); Ll = math.hypot(*dl) or 1; nl = (dl[1] / Ll, -dl[0] / Ll)
        dr = (hr[0] - hpR[0], hr[1] - hpR[1]); Lr = math.hypot(*dr) or 1; nr = (-dr[1] / Lr, dr[0] / Lr)
        if nl[0] > 0: nl = (-nl[0], -nl[1])
        if nr[0] < 0: nr = (-nr[0], -nr[1])
        hem_l = (hl[0] + nl[0] * 16, hl[1] + nl[1] * 16 + 1)
        hem_r = (hr[0] + nr[0] * 16, hr[1] + nr[1] * 16 + 1)
        in_l, in_r = (hl[0] - nl[0] * 6, hl[1] + 4), (hr[0] - nr[0] * 6, hr[1] + 4)
        # cuisse levée (genou à terre, 2.73) : la jupe repose sur la cuisse
        # jusqu'à mi-longueur et tombe un peu sous elle, au lieu de pendre droit
        # (l'autre côté, sur la cuisse au sol, s'évase peu : elle ne pend pas
        # en rideau)
        if abs(dl[1]) < 0.6 * Ll:
            m_ = lerp(hpL, kl, 0.65); hem_l = (m_[0], m_[1] + 6); in_l = (lerp(hpL, kl, 0.3)[0], lerp(hpL, kl, 0.3)[1] + 8)
            h2 = lerp(hpR, kr, 0.5); hem_r = (h2[0] + nr[0] * 6, h2[1] + nr[1] * 6); in_r = (h2[0] - nr[0] * 4, h2[1] + 2)
        elif abs(dr[1]) < 0.6 * Lr:
            m_ = lerp(hpR, kr, 0.65); hem_r = (m_[0], m_[1] + 6); in_r = (lerp(hpR, kr, 0.3)[0], lerp(hpR, kr, 0.3)[1] + 8)
            h2 = lerp(hpL, kl, 0.5); hem_l = (h2[0] + nl[0] * 6, h2[1] + nl[1] * 6); in_l = (h2[0] - nl[0] * 4, h2[1] + 2)
        # (2.77h) cuisse presque à l'horizontale (glissade, fente) : la jupe
        # repose SUR la cuisse : son bord suit le dessus de la cuisse jusqu'au
        # delà de mi-longueur (avant, le bord allait de la taille à l'ourlet
        # sous la cuisse, et le haut de la cuisse passait par-dessus le tissu)
        drape = []
        for hp_, kn_, d_, L_ in ((hpL, kl, dl, Ll), (hpR, kr, dr, Lr)):
            if abs(d_[1]) < 0.6 * L_:
                up = (d_[1] / L_, -d_[0] / L_)
                if up[1] > 0: up = (-up[0], -up[1])           # normale vers le haut
                for t_, w_ in ((0.2, 10.0), (0.4, 9.5), (0.6, 8.5), (0.72, 7.0)):
                    q = lerp(hp_, kn_, t_)
                    drape.append((q[0] + up[0] * w_, q[1] + up[1] * w_))
        # cuisse levée : le haut arrondi de la cuisse, à la hanche, déborde de
        # la jupe vers l'arrière ; elle l'englobe (sans monter au-dessus de la taille)
        cover = []
        for hp_, d_, L_ in ((hpL, dl, Ll), (hpR, dr, Lr)):
            if abs(d_[0]) > 0.3 * L_ and (abs(dl[1]) < 0.6 * Ll or abs(dr[1]) < 0.6 * Lr):   # (cuisse en biais, genou à terre)
                back = math.degrees(math.atan2(-d_[1], -d_[0]))      # à l'opposé du genou
                for a_ in range(-90, 11, 15):
                    q = (hp_[0] + math.cos(math.radians(back + a_)) * 8.6, hp_[1] + math.sin(math.radians(back + a_)) * 8.6)
                    if q[1] >= min(top_l[1], top_r[1]) + 2: cover.append(q)
        # profil arrondi : la jupe s'évase vite sous la taille puis plus
        # doucement jusqu'à l'ourlet (pas d'angle droit en haut sur les côtés)
        prof = []
        for (A, B) in (((top_l[0] + 2, top_l[1]), hem_l), ((top_r[0] - 2, top_r[1]), hem_r)):
            for fr in (0.1, 0.25, 0.42, 0.6, 0.8):
                prof.append((A[0] + (B[0] - A[0]) * fr ** 0.7, A[1] + (B[1] - A[1]) * fr))
        sk = hull([(top_l[0] + 2, top_l[1]), (top_r[0] - 2, top_r[1]), hem_l, hem_r,
                   in_l, in_r] + prof + cover + drape
                  + [(hp_[0] + math.cos(math.radians(a_)) * 8.5, hp_[1] + math.sin(math.radians(a_)) * 8.5)
                     for hp_ in (hpL, hpR) for a_ in range(0, 181, 30)])
        sm = f.m_poly(sk)
        sid = f.part(sm, O, o, sh=(3, 1), group=tid)
        # plis : du haut de la jupe vers l'ourlet, en éventail
        ys = [p[1] for p in sk]
        yb = max(ys)
        for t in (0.18, 0.36, 0.54, 0.72, 0.88):
            a_ = lerp((top_l[0] + 2, top_l[1]), (top_r[0] - 2, top_r[1]), t)
            b_ = lerp(hem_l, hem_r, t)
            f.paint(f.m_line([lerp(a_, b_, 0.35), (b_[0], b_[1] + 6)], 0.7), 4, within=[sid])
        f.paint(sm & ~shift_mask(f, (0, -1.6), sm), 4, within=[sid])          # ourlet

        ret['shorts'] = sid

    def head(f=f):
        hs = 0.86                                                                # tête de 22 px (canon)
        f = HS(f, hs)
        h = j['head']
        tilt = j.get('tilt', 0.0)
        turn = j.get('turn', 0.0)
        ca, sa = math.cos(tilt), math.sin(tilt)
        R = lambda dx, dy: (h[0] + hs * (dx * ca - dy * sa), h[1] + hs * (dx * sa + dy * ca))   # repère de la tête
        if female:
            # chevelure arrière (encadre le visage) et queue de cheval haute
            hair_back = [f.part(f.m_caps(R(6, -10), R(13, 7), 4, 2.2) | f.m_ell(R(6.5, -9.5), 4.5, 3.5, tilt), D, None)]
            back = f.m_ell(R(0, -1), 12.2, 12.5, tilt) & ~f.m_poly([R(-20, 6), R(20, 6), R(20, 30), R(-20, 30)])
            hair_back.append(f.part(back, D, None))
            ret['hair_back'] = hair_back
        ear_x = 10.2
        f.part(f.m_ell(R(-ear_x + turn * 0.3, 1.8), 2.1, 3.4, tilt) | f.m_ell(R(ear_x + turn * 0.3, 1.8), 2.1, 3.4, tilt), S, s, sh=(1, 1))
        # crâne et mâchoire : carrée chez l'homme, plus fine et arrondie chez la femme
        if female:
            jaw = [(-9.6, 0), (9.6, 0), (8.6, 5.5), (5.6, 10), (2, 12.4), (-2, 12.4), (-5.6, 10), (-8.6, 5.5)]
            hm = f.m_ell(R(0, -1.5), 10, 11.8, tilt)
        else:
            jaw = [(-10.2, 0), (10.2, 0), (9.6, 6), (7.2, 10.6), (3.4, 12.9), (-3.4, 12.9), (-7.2, 10.6), (-9.6, 6)]
            hm = f.m_ell(R(0, -1.5), 10.5, 12, tilt)
        hm |= f.m_poly([R(x + turn * 0.25 * (y / 12), y) for x, y in jaw])
        hid = f.part(hm, S, s, sh=(2, 0.8))
        # cheveux
        if female:
            hair = f.m_ell(R(0, -5.6), 11.6, 9.4, tilt) & ~f.m_ell(R(turn * 0.6, 3.2), 9.8, 8.2, tilt)
            hair |= f.m_poly([R(-9.5, -5), R(-1, -9), R(6, -6.5), R(1.5, -4.2), R(-5, -2.2)])        # frange de côté
        else:
            # coupe courte de joueur : côtés courts (pattes, oreilles dégagées),
            # volume sur le dessus coiffé vers le haut, frange en mèches
            top = f.m_ell(R(0, -6.2), 10.6, 8.6, tilt) & ~f.m_ell(R(turn * 0.6, 2.6), 10.2, 8.2, tilt)
            quiff = f.m_poly([R(-9.5, -8), R(-7, -13.2), R(-3.5, -14.6), R(0.5, -15.6), R(4.5, -15.0),
                              R(8, -13.4), R(10, -10.5), R(10.3, -7), R(-10.3, -7)])
            fringe = f.m_poly([R(-9.4, -6.5), R(-7.5, -2.6), R(-6.2, -4.6), R(-4.4, -2.0), R(-2.6, -4.4),
                               R(-0.6, -1.8), R(1.2, -4.2), R(3.2, -2.2), R(4.8, -4.6), R(7, -3.0), R(9.4, -6.5)])
            sides = f.m_poly([R(-10.4, -5), R(-9.9, 1.8), R(-9.0, 1.8), R(-8.8, -3.2)]) | f.m_poly([R(10.4, -5), R(9.9, 1.8), R(9.0, 1.8), R(8.8, -3.2)])
            hair = top | quiff | fringe | sides
        hr = f.part(hair, D, None, group=hid)
        ret['hair_pid'] = hr; ret['head_pid'] = hid
        if female:
            f.paint(f.m_line([R(-3.5, -12.5), R(-2.5, -8.5)], 0.6), 0, within=[hr])          # raie
        else:
            for pts in (((-6, -11.5), (-4.6, -9.5), (-4.2, -7)), ((-1, -13.5), (0.4, -11.2), (0.6, -8.5)), ((4, -12.5), (5.2, -10.4), (5.2, -8))):
                f.paint(f.m_line([R(*q) for q in pts], 0.5), 0, within=[hr])             # mèches
        ex, ey = R(turn, 1)
        ret['face'] = (ex, ey)
        face_features(f, ex, ey, face, hid, tilt, female, hs)

    order = list(j['order'])
    # le buste (et le short ou la robe) passe devant le haut des deux cuisses
    order.remove('torso')
    order.insert(max(order.index('legL'), order.index('legR')) + 1, 'torso')
    # les bras partent des épaules : toujours devant le buste (sinon le haut de
    # la robe ou du polo déborde sur l'épaule)
    for a_ in ('armL', 'armR'):
        if order.index(a_) < order.index('torso'):
            order.remove(a_)
            order.insert(order.index('torso') + 1, a_)
    if female:
        last = max(order.index(k_) for k_ in ('torso', 'legL', 'legR'))
        order.insert(last + 1, 'skirt')
    n_start, head_ids, arm_ids = f.n, set(), set()
    for p in order:
        if p == 'legL': leg('L')
        elif p == 'legR': leg('R')
        elif p in ('armL', 'armR'):
            n0 = f.n; arm(p[-1], racket_hand == p[-1]); arm_ids |= set(range(n0 + 1, f.n + 1))
        elif p == 'torso': torso()
        elif p == 'head':
            n0 = f.n; head(); head_ids = set(range(n0 + 1, f.n + 1))
        elif p == 'skirt': skirt()
        elif p == 'racket': racket()
    if NUDE[0]:
        # (2.79) club nudiste : un corps d'un seul tenant. Les pièces de peau
        # (cou, torse, bassin, bras, mains, jambes ; pas la tête) n'en font
        # qu'une : une seule couleur, une seule ombre sur le bord du corps,
        # aucun contour entre elles (sinon des marches à chaque jointure)
        ids = []
        for pid_ in range(n_start + 1, f.n + 1):
            if pid_ in head_ids or (VS_LINE and pid_ in arm_ids): continue   # (affiche : les bras croisés gardent leur contour)
            m_ = f.pid == pid_
            if m_.any() and np.isin(f.cls[m_], (S, s)).mean() > 0.6: ids.append(pid_)
        if ids:
            U = np.isin(f.pid, ids)
            for pid_ in ids: f.grp[pid_] = ids[0]
            f.cls[U] = S
            dx, dy = int(round(3 * f.k)), int(round(1.5 * f.k))
            sh_ = np.zeros_like(U); sh_[:f.H - dy, :f.W - dx] = U[dy:, dx:]
            f.cls[U & ~sh_] = s
    return ret


def face_features(f, x, y, face, hid, tilt=0.0, female=False, hs=1.0):
    """yeux (blanc, iris, paupière, cils), sourcils, nez, bouche (lèvres)
    selon l'expression : concentration, effort, joie, colère. Pupilles et
    intérieur de la bouche : couleur 0 (sombre du fond)."""
    P = lambda m, c: f.paint(m, c, within=[hid])
    line = f.m_line
    ca, sa = math.cos(tilt), math.sin(tilt)
    R = lambda dx, dy: (x + hs * (dx * ca - dy * sa), y + hs * (dx * sa + dy * ca))
    bw = 0.8 if female else 1.25                       # sourcils fins / épais
    # (2.75) petite échelle (smash, en pied) : sourcil, paupière, pupille et
    # frange se touchaient en une tache noire sur l'œil ; traits allégés,
    # sourcil remonté, pupille plus petite, œil un peu plus ouvert
    small = f.k * hs < 0.8
    # Margaux en petit (2.75) : son « sombre » est le blond des cheveux ; ses
    # traits (sourcils, paupières, bouche) passent dans l'ombre de la peau au
    # lieu de la couleur du fond, qui faisait des trous dans le visage
    gm = small and female == 2
    if small:
        bw = 0.6 if female else 0.8
    def eye(sx, look=0.0, lid=0.0):
        c = R(sx * 4.4, -0.2)
        if small:
            # (à cette taille, le blanc de l'œil ne tient qu'un pixel ou deux
            # et dépend du découpage en tuiles : œil sombre en amande ;
            # Margaux, dont le « sombre » est le blond : blanc de l'œil et
            # petite pupille de la couleur du fond, 2.75)
            if gm:
                P(f.m_ell(c, 1.7, 0.9, tilt), 2)
                P(f.m_ell((c[0] + look * 0.4, c[1] + 0.1), 0.7, 0.7), 0)
                return
            # (femme : œil plus petit et un peu plus bas, la frange de côté
            # ne s'y colle pas)
            P(f.m_ell((c[0] + look * 0.4, c[1] + (0.5 if female else 0.1)), 1.0 if female else 1.3, 0.75 if female else 0.9, tilt), 0)
            return
        P(f.m_ell(c, 2.4 if female else 2.2, 1.35 - lid * 0.5, tilt), 6)
        P(f.m_ell((c[0] + look, c[1] + 0.15), 1.15, 1.15), 0)
        if not small:
            P(line([R(sx * 4.4 - 2.6, -1.3 + lid), R(sx * 4.4 + 2.6, -1.3 + lid)], 0.9 if female else 0.6), 5)   # paupière
        if female and not small:
            P(line([R(sx * 6.8, -1.2 + lid), R(sx * 7.9, -2.3 + lid)], 0.7), 5)                 # cils
    def brow(sx, inner=-3.3, outer=-3.6, arch=0.0):
        up = -0.9 if small else 0.0
        if female:
            P(f.m_arc(R(sx * 4.6, -2.6 + arch + up), 3.0, 1.4, 200, 340, bw), 2 if gm else 5)
        else:
            P(line([R(sx * 7.2, outer + up), R(sx * 2.2, inner + up)], bw), 5)
    def nose():
        if female:
            P(line([R(0.8, 2.5), R(1.1, 4.8)], 0.6), 2)
            P(f.m_ell(R(-0.9, 5.4), 0.6, 0.45), 2); P(f.m_ell(R(1.1, 5.4), 0.6, 0.45), 2)
        else:
            P(line([R(0.6, 0.5), R(1.4, 5.2), R(-0.4, 5.9)], 0.8), 2)
            P(f.m_ell(R(-1.4, 5.7), 0.7, 0.5), 2)
    if face == 'focus':
        for sx in (-1, 1):
            eye(sx, look=-0.9, lid=0.25)
            brow(sx, -3.2, -3.9)
        nose()
        if female:
            P(line([R(-2.6, 8.9), R(0, 9.3), R(2.6, 8.9)], 0.7), 2 if gm else 0)
            P(f.m_ell(R(0, 10.0), 2.2, 0.8, tilt), 2)                                    # lèvre inférieure
        else:
            P(line([R(-3.2, 9.2), R(0, 9.5), R(3.2, 9.1)], 0.9), 0)
            P(line([R(-2, 10.8), R(2, 10.8)], 0.5), 2)
    elif face == 'effort':
        for sx in (-1, 1):
            P(line([R(sx * 6.6, -0.5), R(sx * 4.3, 0.4), R(sx * 2.3, -0.1)], 1.0), 5 if female else 0)   # yeux plissés
            brow(sx, -2.2, -4.4, 0.6)
        nose()
        P(f.m_ell(R(0, 9.4), 3.8 if female else 4.2, 1.7, tilt), 0)                        # dents serrées
        P(f.m_ell(R(0, 9.4), 3.1 if female else 3.5, 0.8, tilt), 6 if f.k >= 2 else 1)
    elif face == 'joy':
        for sx in (-1, 1):
            P(f.m_arc(R(sx * 4.4, 0.9), 2.4, 1.7, 195, 345, 1.0), 5 if female else 0)      # yeux plissés de joie
            brow(sx, -4.0, -4.2, -0.6)
        nose()
        P(f.m_ell(R(0, 8.9), 3.6 if female else 4.0, 2.3, tilt), 0)                        # bouche ouverte
        P(f.m_ell(R(0, 7.3), 3.0 if female else 3.3, 0.8, tilt), 6)                        # dents
        P(f.m_ell(R(0, 10.0), 1.7, 0.7, tilt), 2)                                          # langue
        if female:
            P(f.m_arc(R(0, 8.9), 3.9, 2.8, 20, 160, 0.6), 2)                                # lèvre
    elif face == 'sad':
        # (2.76) abattu : paupières baissées (arcs vers le bas), sourcils
        # relevés au milieu, bouche tombante
        for sx in (-1, 1):
            P(f.m_arc(R(sx * 4.4, -0.6), 2.4, 1.4, 20, 160, 1.0), 0)                       # yeux baissés
            P(line([R(sx * 7.0, -3.0), R(sx * 2.0, -5.0)], bw), 5)                           # sourcils en accent circonflexe
        nose()
        P(f.m_arc(R(0, 11.2), 3.4, 1.8, 200, 340, 0.9), 0)                                  # bouche tombante
    elif face == 'angry':
        for sx in (-1, 1):
            eye(sx, look=0.0, lid=0.6)
            P(line([R(sx * 7.6, -5.2), R(sx * 1.8, -2.0)], bw + 0.3), 5)                   # sourcils froncés
        P(line([R(-0.8, -3.4), R(-0.8, -1.6)], 0.5), 2)                                    # ride du front
        P(line([R(0.8, -3.2), R(0.8, -1.7)], 0.5), 2)
        nose()
        w0 = 3.6 if female else 4.2
        P(f.m_poly([R(-w0, 7.9), R(w0, 7.9), R(w0 - 1.2, 11.2), R(-w0 + 1.2, 11.2)]), 0)  # cri
        P(f.m_poly([R(-w0 + 0.6, 8.0), R(w0 - 0.6, 8.0), R(w0 - 0.9, 8.9), R(-w0 + 0.9, 8.9)]), 6)
        P(line([R(-6.2, 6), R(-5.2, 9.4)], 0.5), 2)                                         # plis
        P(line([R(6.2, 6), R(5.2, 9.4)], 0.5), 2)


def hull(pts):
    """enveloppe convexe (chaîne monotone)"""
    P = sorted(set(pts))
    def cr(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lo, up = [], []
    for p in P:
        while len(lo) >= 2 and cr(lo[-2], lo[-1], p) <= 0: lo.pop()
        lo.append(p)
    for p in reversed(P):
        while len(up) >= 2 and cr(up[-2], up[-1], p) <= 0: up.pop()
        up.append(p)
    return lo[:-1] + up[:-1]


def lerp(a, b, t):
    return (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t)


def shift_mask(f, d, m=None):
    """masque décalé (d en coordonnées de base) ; sans m : demi-plan utile
    pour ne garder qu'un côté (par rapport au masque courant : tout vrai)"""
    dx, dy = int(round(d[0] * f.k)), int(round(d[1] * f.k))
    s = np.zeros_like(m)
    ys = slice(max(dy, 0), f.H + min(dy, 0)); yd = slice(max(-dy, 0), f.H + min(-dy, 0))
    xs = slice(max(dx, 0), f.W + min(dx, 0)); xd = slice(max(-dx, 0), f.W + min(-dx, 0))
    s[yd, xd] = m[ys, xs]
    return s


# ------------------------------------------------------------------ poses
# repère de base : sol en y = 206. Proportions du canon de dessin de figure
# (Loomis, Richer) : environ 7,5 têtes de haut (tête HU = 22 px, de la
# calotte au menton), entrejambe à mi-hauteur, cuisse 1,9 tête, tibia 1,8,
# bras 1,4 + avant-bras 1,1 ; épaules : 2,3 têtes chez l'homme, 1,8 chez la
# femme ; hanches : 1,5 tête (un peu plus que les épaules chez la femme).
HU = 22.0
SEG = dict(torso=48, neck=18.5, thigh=42, shin=40, upper=31, fore=24)


def dvec(a, L):
    r = math.radians(a)
    return (math.cos(r) * L, math.sin(r) * L)


def skeleton(p, female):
    """articulations à partir du bassin, de l'angle du buste et des angles
    des membres (degrés à l'écran : 0 droite, 90 bas, -90 haut)"""
    add = lambda a, b: (a[0] + b[0], a[1] + b[1])
    P = p['pelvis']
    at = p['torso']
    # homme : plus grand (environ 7,75 têtes), membres plus longs
    G = dict(SEG) if female else dict(torso=SEG['torso'] * 1.05, neck=SEG['neck'], thigh=SEG['thigh'] * 1.06,
                                       shin=SEG['shin'] * 1.06, upper=SEG['upper'] * 1.06, fore=SEG['fore'] * 1.06)
    tl = G['torso'] * p.get('torso_len', 1.0)
    neck = add(P, dvec(at, tl))
    perp = dvec(at + 90, 1.0)                      # vers la droite de l'écran (buste vertical)
    if perp[0] < 0: perp = (-perp[0], -perp[1])
    sw = 13.5 if female else 19.0                  # demi-largeur des épaules (articulations)
    hw = 11.0 if female else 7.5                   # demi-largeur des hanches (homme : 1,5 tête avec les cuisses)
    shc = add(neck, dvec(at + 180, 3.5))
    j = dict(neck=neck, head=add(neck, dvec(at + p.get('head_bend', 0), SEG['neck'])),
             tilt=p.get('tilt', 0.0), turn=p.get('turn', 0.0), order=p['order'], kneel=p.get('kneel', False))
    j['shL'] = (shc[0] - perp[0] * sw, shc[1] - perp[1] * sw + (2 if female else 0))
    j['shR'] = (shc[0] + perp[0] * sw, shc[1] + perp[1] * sw + (2 if female else 0))
    j['hipL'] = (P[0] - perp[0] * hw, P[1] - perp[1] * hw)
    j['hipR'] = (P[0] + perp[0] * hw, P[1] + perp[1] * hw)
    for sd in ('L', 'R'):
        lg = p['leg' + sd]
        th, sn, tdir = lg[:3]
        # (2.75) facteurs de longueur facultatifs (cuisse, tibia) : membre
        # qui vient vers la caméra ou s'en éloigne (genou levé, vu de face)
        tk = lg[3] if len(lg) > 3 else 1.0
        sk_ = lg[4] if len(lg) > 4 else 1.0
        kn = add(j['hip' + sd], dvec(th, G['thigh'] * tk))
        an = add(kn, dvec(sn, G['shin'] * sk_))
        j['kn' + sd], j['an' + sd] = kn, an
        j['toe' + sd] = add(an, dvec(tdir, 12))
        ua, fa = p['arm' + sd]
        el = add(j['sh' + sd], dvec(ua, G['upper']))
        wr = add(el, dvec(fa, G['fore']))
        j['el' + sd], j['wr' + sd], j['hd' + sd] = el, wr, add(wr, dvec(fa, 5))
    # pieds posés au sol : la cheville la plus basse en y = 200
    dy = 200 - max(j['anL'][1], j['anR'][1])
    for k_, v in list(j.items()):
        if isinstance(v, tuple):
            j[k_] = (v[0], v[1] + dy)
    return j


POSES = {}

# 1. frappe de coup droit, à l'impact : bras tendu vers la gauche de l'écran
POSES['impact'] = dict(
    s=dict(pelvis=(88, 121), torso=-93, legL=(113, 104, 160), legR=(68, 76, 20),
           armL=(176, 183), armR=(62, 102), tilt=-0.08, turn=-1.0,
           order=['racket', 'legR', 'armR', 'legL', 'torso', 'armL', 'head']),
    face='focus', hand='L', rdir=200, rlen=52)

# 2. glissade sur terre battue : jambe gauche fléchie, jambe droite tendue, très bas
POSES['slide'] = dict(
    s=dict(pelvis=(62, 146), torso=-78, legL=(158, 95, 175), legR=(47, 38, 5),
           armL=(-140, -112), armR=(22, 14), tilt=0.15, turn=1.5, head_bend=-8,
           order=['armL', 'legL', 'torso', 'legR', 'head', 'armR', 'racket']),
    face='effort', hand='R', rdir=-10, rlen=50)

# 3. bras levés au ciel, raquette au poing, cri de joie
POSES['victory'] = dict(
    s=dict(pelvis=(80, 118), torso=-90, legL=(97, 94, 165), legR=(83, 86, 15),
           armL=(-128, -108), armR=(-52, -72), tilt=0.0, turn=0.0,
           order=['racket', 'legL', 'legR', 'torso', 'armL', 'armR', 'head']),
    face='joy', hand='L', rdir=-140, rlen=50)

# 3 ter. le champion (2.76, fin d'une compétition gagnée) : la coupe
#    brandie à deux mains au-dessus de la tête, cri de joie. Coupe d'argent
#    (blanc, reflets sombres) : la seule couleur commune à toutes les palettes.
def _trophy_extra(f, j, female, r):
    hl, hr = j['hdL'], j['hdR']
    cx = (hl[0] + hr[0]) / 2
    hy = (hl[1] + hr[1]) / 2
    base = min(hy + 13, j['head'][1] - (30 if female else 26))   # (au-dessus de la tête : les joueuses, plus petites, avaient les cheveux coupés)
    bot, top = base - 10, base - 42
    mid = (bot + top) / 2
    bowl = f.m_poly([(cx - 17, top), (cx + 17, top), (cx + 10, bot), (cx - 10, bot)]) | f.m_ell((cx, bot), 10, 5)
    cup = (bowl | f.m_ell((cx, top), 17, 4) | f.m_caps((cx, bot), (cx, base - 2), 3.2, 3.6)
           | f.m_ell((cx, base), 10, 3.2)
           | f.m_arc((cx - 16, mid), 7, 9, 90, 270, 2.4) | f.m_arc((cx + 16, mid), 7, 9, -90, 90, 2.4))
    g = f.part(cup, 6, None, sh=(1.5, 1.5))
    f.paint(f.m_line([(cx - 9, top + 6), (cx - 6, bot - 2)], 1.1), 5, within=[g])      # reflet sombre
    f.paint(f.m_line([(cx + 8, top + 7), (cx + 6, bot - 4)], 0.6), 5, within=[g])
    f.paint(f.m_ell((cx, top), 15, 2.2), 5, within=[g])                               # intérieur
    f.paint(f.m_line([(cx - 12, top + 14), (cx + 12, top + 14)], 0.6), 5, within=[g])  # gravure
    for h in (hl, hr):                                                                 # poings par-dessus
        f.part(f.m_ell((h[0], h[1]), 5.6 * (0.82 if female else 1.0), 5.2), 1, 2, sh=(1.5, 1.5))
POSES['trophy'] = dict(
    s=dict(pelvis=(88, 128), torso=-90, legL=(97, 94, 165), legR=(83, 86, 15),
           armL=(-104, -78), armR=(-76, -102), tilt=0.0, turn=0.0,
           order=['legL', 'legR', 'torso', 'armL', 'armR', 'head']),
    face='joy', hand='X', rdir=0, rlen=50, extra=_trophy_extra)

# 3 bis. défaite (2.76, fin du match perdu par le joueur humain) :
#    à genoux, bras ballants, tête penchée, raquette lâchée à ses pieds
POSES['defeat'] = dict(
    # (à genoux : cuisses vers la caméra, tibias repliés derrière, presque cachés)
    s=dict(pelvis=(88, 182), torso=-90, torso_len=0.86, legL=(122, 90, 170, 0.42, 0.1), legR=(58, 90, 10, 0.42, 0.1),
           armL=(104, 72), armR=(76, 108), tilt=0.14, turn=0.0, head_bend=0, kneel=True,
           order=['racket', 'legL', 'legR', 'torso', 'armL', 'armR', 'head']),
    face='sad', hand='L', rdir=170, rlen=44)

# 4. raquette fracassée au sol : penché en avant (buste raccourci), bras abattu, cri de colère
POSES['angry'] = dict(
    s=dict(pelvis=(88, 124), torso=-96, torso_len=0.9, legL=(116, 96, 165), legR=(64, 84, 15),
           armL=(112, 78), armR=(60, 98), tilt=-0.1, turn=-0.5, head_bend=4,
           order=['legR', 'legL', 'torso', 'armR', 'racket', 'armL', 'head']),
    face='angry', hand='L', rdir=100, rlen=50)

# 5. smash (et service) : raquette au-dessus de la tête, bras libre pointé
#    vers la balle, buste cambré, jambe arrière sur la pointe (version 2.68,
#    gros plan du ralenti)
POSES['smash'] = dict(
    s=dict(pelvis=(96, 118), torso=-104, legL=(110, 98, 170), legR=(70, 80, 15),
           armL=(-118, -104), armR=(52, 160), tilt=-0.08, turn=-0.7, head_bend=-14,
           order=['legR', 'legL', 'torso', 'armR', 'head', 'armL', 'racket']),
    face='focus', hand='L', rdir=-122, rlen=50)

# 5 bis. smash en plein vol (version 2.75, gros plan du smash au ralenti) :
#    même haut du corps que le smash, mais les deux pieds décollés (ciseau :
#    jambe avant repliée, jambe arrière lancée derrière), bras libre tendu
#    vers la balle ; ombre au sol, loin sous les pieds
def _smash_air_extra(f, j, female, r):
    f.part(f.m_ell((96, 229.3), 15, 3), 0 if female == 2 else 5, None)   # (Margaux : son « sombre » est blond)
POSES['smash_air'] = dict(
    # (2.75) ciseau du smash sauté vu de face : genou avant levé vers la
    # caméra (cuisse raccourcie), tibia vertical, pied pointé vers le bas ;
    # jambe arrière pendante, tibia replié vers l'arrière (raccourci) ; buste
    # presque droit, hanches de niveau (short et jupe sans débord)
    s=dict(pelvis=(96, 96), torso=-98, legL=(100, 115, 125, 1.0, 0.75), legR=(75, 95, 105, 0.6, 1.0),
           armL=(-118, -104), armR=(-55, -75), tilt=0.0, turn=0.0, head_bend=-8,   # (2.76 : visage de face, droit : il paraissait décalé)
          
           order=['legR', 'legL', 'torso', 'armR', 'head', 'armL', 'racket']),
    face='focus', hand='L', rdir=-122, rlen=50, extra=_smash_air_extra)

# 6. revers à deux mains, à l'impact (version 2.68, gros plan du ralenti) :
#    miroir du coup droit, les deux bras tendus vers la droite de l'écran,
#    mains jointes sur le manche
POSES['backhand'] = dict(
    s=dict(pelvis=(112, 121), torso=-87, legL=(112, 104, 160), legR=(67, 76, 20),
           armR=(95, -35), armL=(12, 8), tilt=0.08, turn=1.0,
           # (2.73) le bras tendu traverse le buste DEVANT lui jusqu'à la main
           # du haut du manche ; le bras plié reste derrière, main du bas : en
           # 2.68-2.72 c'était l'inverse et les bras paraissaient emmêlés
           order=['racket', 'legL', 'legR', 'armR', 'torso', 'armL', 'head']),
    face='focus', hand='R', rdir=-20, rlen=52)

# 7. ramasseur de balles (version 2.73, changement de côté) : un genou à terre
#    près du poteau, une balle levée dans la main, l'autre main sur le genou
def _ballkid_extra(f, j, female, r):
    hd = j['hdL']                       # (2.73 : main gauche, côté éloigné : l'autre main sur le genou)
    c = (hd[0], hd[1] - 6)
    bid = f.part(f.m_ell(c, 6.5, 6.5), 6, None)                     # balle (blanc)
    f.paint(f.m_arc(c, 4.5, 4.5, 200, 340, 0.8), 5, within=[bid])   # couture
    r['ball'] = c
POSES['ballkid'] = dict(
    # (2.73) genou à terre comme une fente : jambe avant (droite, hanche de
    # devant) cuisse horizontale, tibia vertical, pied à plat ; jambe arrière
    # (gauche) en biais vers l'arrière, genou au sol, tibia à plat ; bras
    # gauche levé avec la balle, main droite sur le genou avant
    s=dict(pelvis=(80, 157), torso=-84, legL=(112, 177, 178), legR=(0, 90, 0),
           armL=(-120, -88), armR=(50, 60), tilt=0.05, turn=0.4,
           order=['armL', 'legL', 'torso', 'legR', 'head', 'armR']),
    face='joy', hand='X', rdir=0, rlen=50, extra=_ballkid_extra)

# 8. poignée de main au filet (version 2.73, fin du match) : le vainqueur
#    tend la main droite de l'écran ; le bras de l'adversaire entre par la
#    droite (peau, poignet blanc) ; la bande du filet devant, à la taille
def _shake_extra(f, j, female, r):
    hd = j['hdR']
    a = (300, hd[1] + 14)
    el = (232, hd[1] + 8)
    g = f.part(f.m_caps(a, el, 7.5, 6.5) | f.m_caps(el, (hd[0] + 7, hd[1] + 1), 6.5, 5.0), 1, 2, sh=(2.5, 1.5))
    f.part(f.m_caps(lerp(el, (hd[0] + 7, hd[1] + 1), 0.62), lerp(el, (hd[0] + 7, hd[1] + 1), 0.78), 5.4, 5.2), 6, None, group=g)   # poignet blanc
    # mains serrées (2.73) : celle de l'adversaire enveloppe celle du joueur
    # (doigts repliés sur le dos de sa main), le pouce du joueur par-dessus
    hm = f.m_ell((hd[0] + 3, hd[1] + 1), 6.6, 5.4)
    hid = f.part(hm, 1, 2, sh=(2, 2), group=g)
    for dy in (-1.6, 0.6, 2.8):
        f.paint(f.m_line([(hd[0] - 2.5, hd[1] + 1 + dy), (hd[0] + 2.5, hd[1] + 1 + dy * 1.1)], 0.45), 2, within=[hid])
    f.part(f.m_caps((hd[0] - 3, hd[1] - 3.2), (hd[0] + 4.5, hd[1] - 4.6), 1.9, 1.6), 1, 2, sh=(1, 1))   # pouce du joueur
    y0 = j['hipL'][1] - 4
    # (2.84) bande entière dans une même rangée de blocs de couleurs (16 px de
    # l'écran) : sinon ses dernières lignes tombaient dans les blocs du short,
    # sans blanc dans leur palette, et devenaient une ligne couleur peau
    ts, tb = (y0 - 6 - f.oy) * f.k, (y0 + 1 - f.oy) * f.k
    lim = (int(ts) // 16 + 1) * 16
    if tb > lim:
        y0 -= (tb - lim + 0.5) / f.k
    yb = lim / f.k + f.oy - 0.01          # bas de la bande : la limite du bloc (pas de mailles blanches isolées juste dessous)
    net = f.m_poly([(-50, yb), (400, yb), (400, y0 + 90), (-50, y0 + 90)])
    yy, xx = np.mgrid[0:f.H, 0:f.W]
    f.part(net & (((xx + yy) % 8 == 0) | ((xx - yy) % 8 == 0)), 6, None)   # mailles blanches (seules : le joueur se voit derrière ; en couleur sombre, celle des cheveux de Margaux les délavait)
    tape = f.m_poly([(-50, y0 - 6), (400, y0 - 6), (400, yb), (-50, yb)])
    f.part(tape, 6, None)                                             # bande blanche
POSES['handshake'] = dict(
    s=dict(pelvis=(82, 122), torso=-90, legL=(97, 94, 165), legR=(83, 86, 15),
           armL=(100, 95), armR=(30, -8), tilt=0.0, turn=0.3,
           order=['legL', 'legR', 'armL', 'racket', 'torso', 'head', 'armR']),
    face='joy', hand='L', rdir=95, rlen=50, extra=_shake_extra)
# (2.80) la même poignée de main, visage triste (le joueur humain qui a
# perdu) : seules les tuiles qui changent sont gardées (sad_patch)
POSES['handshake_sad'] = dict(POSES['handshake'], face='sad')

# 9. public en gros plan (version 2.73, après un très long échange) : trois
#    spectateurs debout qui applaudissent, bras levés, maillots différents
POSES['crowd'] = dict(figures=[
    dict(s=dict(pelvis=(28, 126), torso=-92, legL=(97, 94, 165), legR=(83, 86, 15),
                armL=(-120, -100), armR=(-60, -80), tilt=-0.06, turn=0.2,
                order=['legL', 'legR', 'torso', 'armL', 'armR', 'head']), female=0, face='joy', shirt=6, shorts=3, casual=True, tag='1'),
    # (2.75 : vue resserrée sur deux spectateurs ; le troisième, à droite, retiré : place des tuiles)
    dict(s=dict(pelvis=(82, 122), torso=-90, legL=(97, 94, 165), legR=(83, 86, 15),
                armL=(-100, -70), armR=(-80, -110), tilt=0.0, turn=0.0,
                order=['legL', 'legR', 'torso', 'armL', 'armR', 'head']), female=1, face='joy', tag='2'),
    ], face='joy', hand='X', rdir=0, rlen=50, extra=lambda f, j, female, r: _crowd_extra(f, r))

def _crowd_extra(f, r):
    # (2.74) casquette (couleur de la tenue) pour le spectateur de gauche ;
    # rangée de sièges devant, assez haute pour cacher le bas du corps (ni
    # short ni jupe de tennis : ce sont des spectateurs, pas des joueurs)
    ex, ey = r['face1']
    crown = f.m_ell((ex, ey - 8.5), 10.5, 6.5) & f.m_poly([(ex - 20, ey - 30), (ex + 20, ey - 30), (ex + 20, ey - 5.5), (ex - 20, ey - 5.5)])
    brim = f.m_poly([(ex - 3, ey - 6.5), (ex + 14, ey - 6.5), (ex + 14, ey - 4.5), (ex - 3, ey - 4.5)])
    f.part(crown | brim, 3, 4, sh=(1, 1))
    # (2.75) gradins à l'échelle des spectateurs : un dossier est large comme
    # des épaules (32 px) ; la rangée de devant cache le bas du corps, la
    # rangée du dessus se voit derrière eux, entre les spectateurs : ils sont
    # dans la tribune. Motif de 40 x 40 pixels calé sur les tuiles, rangées
    # décalées d'un demi-siège : béton clair (teinte de la peau claire),
    # dossier orangé arrondi (ombre de la peau), reflet et arête clairs,
    # assise, ombre sous l'assise ; couleurs différentes des maillots.
    yy, xx = np.mgrid[0:f.H, 0:f.W]
    y0 = int(f.T((0, 110))[1]) // 16 * 16                      # (limite de bloc de couleurs : le haut des sièges ne partage pas un bloc avec les maillots)
    free = f.pid == 0                                           # (derrière les spectateurs : le fond seulement)
    # (motif à l'échelle de l'image : 40 px pour k = 0,9, multiple de 8)
    C = max(40, int(round(40 * f.k / 0.9 / 8)) * 8)
    B = C * 4 // 5 // 8 * 8                                     # (rangée du dessus : sous les épaules)
    front = yy >= y0
    behind = (yy < 0)          # (rangée du dessus retirée en vue resserrée : elle faisait des carrés pâles sur les maillots)
    r_ = np.where(front, (yy - y0) // C, -1)
    cy = np.where(front, (yy - y0) % C, yy - (y0 - B)) * 40 // C
    cx = ((xx - (r_ % 2) * (C // 2)) % C) * 40 // C
    zone = front | behind
    f.part(front, 1, None)                                      # béton (devant : cache le bas du corps)
    f.part(behind, 1, None)
    back = zone & (cy <= 22) & (cx >= 4) & (cx <= 35) & ~((cy <= 1) & ((cx <= 6) | (cx >= 33))) & ~((cy == 2) & ((cx == 4) | (cx == 35)))
    sid_ = f.part(back, 2, None)
    f.paint(back & (cy == 3) & (cx >= 9) & (cx <= 26), 1, within=[sid_])     # reflet
    f.paint(back & (cx == 33) & (cy >= 5) & (cy <= 19), 1, within=[sid_])    # arête
    seat = zone & (cy >= 24) & (cy <= 28) & (cx >= 1) & (cx <= 38)
    sid2 = f.part(seat, 2, None)
    f.paint(seat & (cy == 24), 1, within=[sid2])
    f.part(zone & (((cy >= 29) & (cy <= 31) & (cx >= 3) & (cx <= 36)) | (cy == 39)), 5, None)   # ombre sous l'assise, pied de marche

# images identiques quel que soit le joueur (une seule copie en ROM) : la
# fillette pour le ramasseur, le public
FIXED_FEMALE = {'ballkid': 2, 'crowd': 0}

# plans : (nom, pose, échelle, (x, y) de base visé au centre ; None : pieds au sol)
# zoom : True si le visage est assez grand pour sa palette (peau, sombre, blanc)
SHOTS = [
    ('impact', 'impact', 1.4, (54.3, 102), False),     # gros plan : buste, raquette à l'impact  # (2.76 : décalé de +6 px, blocs de couleurs mieux placés)
    ('slide', 'slide', 1.12, (88.2, None), False),           # glissade, en pied  # (2.76 : décalé de -2 px, blocs de couleurs mieux placés)
    ('slide_z', 'slide', 1.3, (101.5, None), False),   # zoom : la chaussure qui glisse  # (2.76 : décalé de +2 px, blocs de couleurs mieux placés)
    ('victory', 'victory', 1.0, (82, None), False),        # bras levés, en pied
    ('victory_z', 'victory', 2.0, (83, 62), True),   # zoom : buste et visage  # (2.76 : décalé de +6 px, blocs de couleurs mieux placés)
    ('defeat', 'defeat', 1.5, (88, 150), False),       # défaite : à genoux, abattu, plan rapproché (2.76)
    ('angry', 'angry', 1.12, (91.8, None), False),           # raquette au sol, en pied  # (2.76 : décalé de +2 px, blocs de couleurs mieux placés)
    ('angry_z', 'angry', 3.4, (84.8, 70), True),       # zoom : le visage qui crie  # (2.76 : décalé de +6 px, blocs de couleurs mieux placés)
    ('smash', 'smash', 0.86, (87.3, None), False),  # (2.76 : décalé de -4 px, blocs de couleurs mieux placés)
    ('smash_air', 'smash_air', 0.86, (90, 104), False),   # smash en plein vol (ralenti du smash, 2.75)
    ('backhand', 'backhand', 1.4, (148.6, 102), False),   # revers à deux mains, gros plan (ralenti, 2.68)      # smash et service, en pied (ralenti, 2.68)  # (2.76 : décalé de -2 px, blocs de couleurs mieux placés)
    ('ballkid', 'ballkid', 1.3, (97.1, 123), False),     # ramasseur de balles (changement de côté, 2.73)  # (2.76 : décalé de +4 px, blocs de couleurs mieux placés)
    ('handshake', 'handshake', 1.3, (114.6, 96), False),  # poignée de main au filet (fin du match, 2.73)  # (2.76 : décalé de +6 px, blocs de couleurs mieux placés)
    ('crowd', 'crowd', 1.3, (48.5, 86), False),
    ('trophy', 'trophy', 0.9, (88, 92.5), False),           # le champion brandit la coupe (fin de compétition, 2.76)           # public en gros plan (long échange, 2.73)  # (2.76 : décalé de -2 px, blocs de couleurs mieux placés)
]


def render(pose, female, k, center, dark=True):
    P = POSES[pose]
    W, H = 256, GROUND_ROW * 8
    cx, cy = center if center else (90, None)
    if cy is None:
        oy = 206.5 - (H - 1) / k          # pieds au sol (dernière rangée du cadre)
    else:
        oy = cy - (H / 2) / k
    f = Fig(W, H, k, cx - 128 / k, oy)
    if 'figures' in P:
        # plusieurs personnages (public, version 2.73) : de l'arrière à l'avant ;
        # chacun peut changer la couleur de sa tenue (classes 3-4 -> autre)
        r = {}
        for fs in P['figures']:
            n0 = f.n
            CASUAL[0] = bool(fs.get('casual'))
            rr = body(f, skeleton(fs['s'], fs['female']), fs['female'], fs['face'], racket_hand='X', racket_dir=0, racket_len=50)
            CASUAL[0] = False
            if fs.get('shirt'):
                m = (f.pid > n0) & np.isin(f.cls, (3, 4))
                f.cls[m] = fs['shirt']
            if fs.get('shorts') and isinstance(rr.get('shorts'), (int, np.integer)):
                # short d'une autre couleur : un polo blanc sur un short sombre
                # ne tient dans aucune palette (peau, blanc, sombre)
                m = (f.pid == rr['shorts']) & (f.cls == 5)
                f.cls[m] = fs['shorts']
            r.update({k_ + fs.get('tag', ''): v for k_, v in rr.items()})
    else:
        r = body(f, skeleton(P['s'], female), female, P['face'], racket_hand=P['hand'], racket_dir=P['rdir'], racket_len=P['rlen'])
    if 'extra' in P:                     # dessin complémentaire (balle, bras de l'adversaire, filet : 2.73)
        P['extra'](f, skeleton(P['s'], female) if 's' in P else None, female, r)
    f.outline()
    # contours et yeux dans la couleur sombre (version 2.61), et non dans
    # celle du fond ; sauf Margaux (cheveux blonds : ses yeux seraient jaunes)
    # (2.76) contours et yeux : toujours dans la couleur du fond (sombre, et
    # présente dans toutes les palettes) ; la couleur sombre du joueur ne
    # sert plus qu'aux cheveux, sourcils, short, semelles et raquette : des
    # cheveux blonds ou blancs ne font plus de contours clairs
    if False and female != 2 and dark:
        f.cls[(f.cls == 0) & (f.pid > 0)] = 5
    # Margaux (2.73) : la couleur sombre est celle de ses cheveux blonds ; les
    # autres détails sombres (manche de la raquette, semelles, sourcils...)
    # passent dans la couleur du fond, sombre elle aussi, au lieu de jaunir
    if female == 2 and isinstance(r.get('head_pid'), (int, np.integer)):
        hid_ = r['head_pid']
        hair_ids = [p_ for p_ in range(1, f.n + 1) if f.grp.get(p_) == hid_ and p_ != hid_] + list(r.get('hair_back', []))
        f.cls[(f.cls == 5) & ~np.isin(f.pid, hair_ids)] = 0
    f.leg_ids = [r[k_] for k_ in ('legL', 'legR') if k_ in r]
    f.joints = skeleton(P['s'] if 's' in P else P['figures'][-1]['s'], female)
    # (2.79) squelettes des personnages dessinés : zones masquées au club nudiste
    f.skels = ([(skeleton(fs['s'], fs['female']), fs['female']) for fs in P['figures']] if 'figures' in P
               else [(skeleton(P['s'], female), female)])
    r = {key: f.T(v) for key, v in r.items() if isinstance(v, tuple) and len(v) == 2 and not isinstance(v[0], (list, tuple))}
    return f, r


# ------------------------------------------------------------------ censure
def cens_rects(T, skels, cols, rows):
    """(2.79) club nudiste : rectangles de tuiles (x, y, l, h) à couvrir d'une
    mosaïque couleur peau : le bassin de chacun, la poitrine des femmes.
    T : coordonnées du personnage -> pixels de l'image."""
    out = []
    for j, fem in skels:
        H = [(a + b) / 2 for a, b in zip(T(j['hipL']), T(j['hipR']))]
        S = [(a + b) / 2 for a, b in zip(T(j['shL']), T(j['shR']))]
        tl = math.hypot(S[0] - H[0], S[1] - H[1])
        zones = [((H[0], H[1] + 0.14 * tl), 0.30 * tl, 0.24 * tl)]          # bassin
        if fem:
            zones.append(((S[0] + (H[0] - S[0]) * 0.32, S[1] + (H[1] - S[1]) * 0.32), 0.40 * tl, 0.17 * tl))   # poitrine
        for (cx, cy), hx, hy in zones:
            x0 = max(0, int((cx - hx) // 8)); x1 = min(cols - 1, int((cx + hx) // 8))
            y0 = max(0, int((cy - hy) // 8)); y1 = min(rows - 1, int((cy + hy) // 8))
            if x1 >= x0 and y1 >= y0:
                out.append((x0, y0, x1 - x0 + 1, y1 - y0 + 1))
    return out[:4]

# ------------------------------------------------------------------ tuiles
COST = {  # coût de remplacement d'une classe par une autre (0..6)
    (1, 2): 2, (1, 6): 4, (1, 3): 7, (1, 5): 8, (1, 4): 7,
    (2, 1): 2, (2, 5): 4, (2, 4): 4, (2, 3): 6,
    (3, 4): 2, (3, 6): 4, (3, 1): 7, (3, 5): 6,
    (4, 3): 2, (4, 5): 3, (4, 2): 4, (4, 1): 6,
    (5, 0): 3, (5, 2): 4, (5, 4): 4,
    (6, 1): 3, (6, 3): 3, (6, 2): 5,
}
PALS = {1: (1, 2, 5), 2: (3, 4, 5), 3: (1, 3, 6)}
PALS_Z = {0: (1, 5, 6), 1: (1, 2, 5), 2: (3, 4, 5), 3: (1, 2, 3)}       # plans rapprochés : P0 = peau, sombre, blanc


COST_X = {}      # surcharges (affiche VS)


def cost(a, b):
    if a == b:
        return 0
    if (a, b) in COST_X:
        return COST_X[(a, b)]
    if b == 0:
        if not COST_X:
            # (2.73, images du ralenti) la peau et la tenue ne deviennent jamais
            # du fond (des trous dans les bras ou les jambes) : elles prennent
            # la couleur la plus proche du carré. (Le sombre garde sa règle :
            # le protéger aussi faisait perdre le blanc des chaussures.)
            if a in (1, 2, 3, 4): return 14
        return COST.get((a, 0), 6)
    return COST.get((a, b), 9)


def quantize(f, pals):
    """attribut de chaque bloc 16 x 16 (palette la moins coûteuse), puis
    indices 0-3 des pixels"""
    H, W = f.H, f.W
    idx = np.zeros((H, W), np.uint8)
    attr = np.full((H // 16 + 1, W // 16), 255, np.uint8)
    for by in range(0, H, 16):
        for bx in range(0, W, 16):
            blk = f.cls[by:by + 16, bx:bx + 16]
            if not blk.any() and not (f.pid[by:by + 16, bx:bx + 16] > 0).any():
                continue
            vals, cnt = np.unique(blk, return_counts=True)
            best = None
            for p, cols in pals.items():
                tgt = (0,) + cols
                c = sum(min(cost(v, t) for t in tgt) * n for v, n in zip(vals, cnt))
                if best is None or c < best[0]:
                    best = (c, p)
            p = best[1]
            attr[by // 16, bx // 16] = p
            tgt = (0,) + pals[p]
            lut = {v: min(range(4), key=lambda i: cost(v, tgt[i])) for v in vals}
            sub = idx[by:by + 16, bx:bx + 16]
            for v in vals:
                sub[blk == v] = lut[v]
    return idx, attr


def tile_bytes(idx, tx, ty):
    b = []
    for plane in (0, 1):
        for y in range(8):
            v = 0
            for x in range(8):
                v = (v << 1) | ((int(idx[ty * 8 + y, tx * 8 + x]) >> plane) & 1)
            b.append(v)
    return bytes(b)


def build(f, zoom):
    idx, attr = quantize(f, PALS_Z if zoom else PALS)
    rows = f.H // 8
    tiles, tmap = [], {}
    nt = [[None] * 32 for _ in range(rows)]
    for ty in range(rows):
        for tx in range(32):
            a = attr[ty // 2, tx // 2]
            t = tile_bytes(idx, tx, ty)
            if a == 255:
                nt[ty][tx] = 0 if zoom else T_STREAK
            elif not any(t):
                nt[ty][tx] = 0
            else:
                if t not in tmap:
                    tmap[t] = FIRST_TILE + len(tiles)
                    tiles.append(t)
                nt[ty][tx] = tmap[t]
    # rectangle utile (rangées / colonnes qui ne sont pas que du fond)
    used = [(x, y) for y in range(rows) for x in range(32) if attr[y // 2, x // 2] != 255]
    x0 = min(x for x, _ in used) & ~1; x1 = (max(x for x, _ in used) | 1) + 1
    y0 = min(y for _, y in used) & ~1; y1 = rows
    # table des attributs complète (64 octets) : 2 bits par bloc 16 x 16
    at = []
    for ay in range(8):
        for ax in range(8):
            v = 0
            for q, (dy, dx) in enumerate(((0, 0), (0, 1), (1, 0), (1, 1))):
                by, bx = ay * 2 + dy, ax * 2 + dx
                p = attr[by, bx] if by < attr.shape[0] and by * 16 < f.H and bx < attr.shape[1] else 255
                v |= (0 if p == 255 else int(p)) << (q * 2)
            at.append(v)
    assert len(tiles) <= MAX_TILES, len(tiles)
    return dict(tiles=tiles, nt=nt, x0=x0, x1=x1, y0=y0, y1=y1, at=at, idx=idx, attr=attr, zoom=zoom)


# ------------------------------------------------------------------ aperçu
SAMPLES = [(0x27, 0x30, 0x0F), (0x37, 0x1C, 0x0F), (0x07, 0x27, 0x0F), (0x27, 0x35, 0x37)]


def shades(S, O, D):
    sl, sd = (S, S - 0x10) if S >= 0x10 else (S + 0x10, S)
    ol, od = (O, O - 0x10) if O >= 0x10 else (O + 0x10, O)
    if O == 0x30: od = 0x10
    return sl, sd, ol, od


def bg_pals(c, back=0x01, zoom=False):
    S, O, D = c
    sl, sd, ol, od = shades(S, O, D)
    p0 = (back, sl, D, 0x30) if zoom else (back, 0x11, 0x21, 0x12)
    p3 = (back, sl, sd, ol) if zoom else (back, sl, ol, 0x30)
    return [p0, (back, sl, sd, D), (back, ol, od, D), p3]


def preview(res, c, back=0x01):
    pals = bg_pals(c, back, res['zoom'])
    H = GROUND_ROW * 8 + 32
    im = Image.new('RGB', (256, H), PAL[back])
    px = im.load()
    idx, attr = res['idx'], res['attr']
    for y in range(GROUND_ROW * 8):
        for x in range(256):
            a = attr[y // 16, x // 16]
            v = idx[y, x]
            if a == 255:
                if res['zoom']: continue
                if (x + y) % 8 == 0: px[x, y] = PAL[0x11]
                continue
            if v:
                px[x, y] = PAL[pals[a][v]]
    for y in range(GROUND_ROW * 8, H):
        for x in range(256):
            px[x, y] = PAL[0x12]
    return im


def main():
    out = {}
    sheet = []
    for name, pose, k, cen, zoom in SHOTS:
        for female in (0, 1, 2):
            f, r = render(pose, FIXED_FEMALE.get(name, female), k, cen)
            try:
                res = build(f, zoom)
            except AssertionError:      # trop de tuiles différentes : contours dans la couleur du fond
                f, r = render(pose, FIXED_FEMALE.get(name, female), k, cen, dark=False)
                res = build(f, zoom)
                print('  (%s %d : contours dans la couleur du fond, faute de tuiles)' % (name, female))
            res['r'] = r
            res['cens'] = cens_rects(f.T, f.skels, 32, GROUND_ROW)
            out[(name, female)] = res
            print('%-10s %s : %3d tuiles, colonnes %d-%d, rangées %d-%d' % (
                name, 'HFG'[female], len(res['tiles']), res['x0'], res['x1'] - 1, res['y0'], res['y1'] - 1))
            # (ramasseuse, public : toujours la tenue de JADE en jeu, highlight.c)
            smp = [(0x36, 0x2B, 0x0F)] * 4 if name in FIXED_FEMALE else SAMPLES
            if female == 0:
                sheet.append([preview(res, c) for c in smp[:2]])
            elif female == 1:
                sheet[-1] += [preview(res, c) for c in smp[2:]]
            # (2.79) club nudiste : la même image sans vêtements (homme, femme ;
            # ni Margaux ni le public)
            if female < 2 and name != 'crowd':
                NUDE[0] = True
                try:
                    f, r = render(pose, FIXED_FEMALE.get(name, female), k, cen)
                    res = build(f, zoom)
                finally:
                    NUDE[0] = False
                out[(name, female, 'n')] = res
    return out, sheet


# ------------------------------------------------------------------ export
# banques libres (octets disponibles, marge comprise)
# (2.79) banques des images sans vêtements du club nudiste (place libre, marge déduite)
NZ_BANKS = [(2, 4390), (6, 4320), (7, 4830), (9, 4670), (11, 4720), (16, 4840), (17, 4510), (18, 5710), (19, 5210),
            (21, 4780), (23, 4600), (24, 2440), (25, 2440), (27, 4240), (28, 5170), (10, 3320), (12, 3290), (26, 1360)]
NZ_VS = {(0, 0): 2, (0, 1): 6, (1, 0): 27, (1, 1): 10}     # bustes sans vêtements de l'affiche (banques avec ~1,4 Ko libres après les images HD)
BANKS = [(9, 13600), (12, 11500), (4, 12200), (10, 7050), (2, 7900), (7, 6800), (6, 5800), (17, 16000), (18, 13500), (20, 9000), (21, 15500), (16, 12500), (28, 15000)]


def pack(chr_):
    """par tuile : masque 16 bits (octet répété = 1), puis octets nouveaux"""
    out = bytearray()
    for t in range(0, len(chr_), 16):
        m, prev, new = 0, 0, bytearray()
        for i, b in enumerate(chr_[t:t + 16]):
            if b == prev:
                m |= 1 << i
            else:
                new.append(b)
            prev = b
        out += bytes((m & 255, m >> 8)) + new
    return bytes(out)


def export(out):
    blobs, arts = [], {}
    for name, pose, k, cen, zoom in SHOTS:
        for female in (0, 1, 2):
            res = out[(name, female)]
            chr_ = pack(b''.join(res['tiles']))
            mp = bytes(res['nt'][y][x] for y in range(res['y0'], res['y1']) for x in range(res['x0'], res['x1']))
            key = (chr_, mp, bytes(res['at']))
            same = [i for i, b in enumerate(blobs) if b['key'] == key]
            if same:
                arts[(name, female)] = same[0]
                continue
            blobs.append(dict(key=key, size=len(chr_) + len(mp) + 64, id='%s_%s' % (name, 'mfg'[female]), res=res))
            arts[(name, female)] = len(blobs) - 1
    # rangement : la plus grosse d'abord, dans la banque la moins remplie qui l'accepte
    free = {b: n for b, n in BANKS}
    for b in sorted(blobs, key=lambda b: -b['size']):
        cand = [k for k in free if free[k] >= b['size']]
        assert cand, 'plus de place pour ' + b['id']
        k = max(cand, key=lambda k: free[k])
        free[k] -= b['size']
        b['bank'] = k
    used = sorted(k for k, _ in BANKS)
    def arr(name, data):
        lines = ['const unsigned char %s[%d] = {' % (name, len(data))]
        for i in range(0, len(data), 16):
            lines.append('  ' + ','.join('0x%02X' % v for v in data[i:i + 16]) + ',')
        lines.append('};')
        return lines
    for bk in used:
        L = ['/* Fichier généré par tools/gen_teaser.py — ne pas modifier. */',
             '/* Bande-annonce : personnages en haute définition (banque %d). */' % bk, '']
        for b in blobs:
            if b['bank'] != bk:
                continue
            chr_, mp, at = b['key']
            L += arr('tz_chr_' + b['id'], chr_) + arr('tz_map_' + b['id'], mp) + arr('tz_at_' + b['id'], at) + ['']
        open(os.path.join(ROOT, 'src', 'assets_tz%d.c' % bk), 'w').write('\n'.join(L))
    H = ['/* Fichier généré par tools/gen_teaser.py — ne pas modifier. */',
         '/* Bande-annonce : descripteurs des images en haute définition (teaser.c). */', '']
    for b in blobs:
        for t in ('chr', 'map', 'at'):
            H.append('extern const unsigned char tz_%s_%s[];' % (t, b['id']))
    H += ['', 'typedef struct { unsigned char bank, ntile, x0, w, y0, h, zoom; const unsigned char *chr, *map, *at; } tz_art_t;', '']
    names = [sh[0] for sh in SHOTS]
    for i, n in enumerate(names):
        H.append('#define TZ_%s %d' % (n.upper(), i))
    H.append('#define TZ_N %d' % len(names))
    H.append('static const tz_art_t tz_art[TZ_N][3] = {   /* homme, femme, fillette (Margaux) */')
    for name, pose, k, cen, zoom in SHOTS:
        row = []
        for female in (0, 1, 2):
            b = blobs[arts[(name, female)]]; r = b['res']
            row.append('{ %d, %d, %d, %d, %d, %d, %d, tz_chr_%s, tz_map_%s, tz_at_%s }' % (
                b['bank'], len(r['tiles']), r['x0'], r['x1'] - r['x0'], r['y0'], r['y1'] - r['y0'], 1 if zoom else 0, b['id'], b['id'], b['id']))
        H.append('    { %s,\n      %s,\n      %s },' % tuple(row))
    H.append('};')
    # repères (pixels de l'écran) : centre du tamis, pied qui glisse, visage
    for name in names:
        r = out[(name, 0)]['r']
        for key in ('racket', 'footR', 'face', 'racket_tip'):
            if key in r:
                x, y = r[key]
                H.append('#define TZ_%s_%s_X %d' % (name.upper(), key.upper(), max(0, min(255, int(round(x))))))
                H.append('#define TZ_%s_%s_Y %d' % (name.upper(), key.upper(), max(0, min(239, int(round(y))))))
    H.append('#define TZ_GROUND_ROW %d' % GROUND_ROW)
    H.append('#define TZ_FIRST_TILE %d' % FIRST_TILE)
    open(os.path.join(ROOT, 'src', 'assets_tz.h'), 'w').write('\n'.join(H) + '\n')
    # (2.79) zones masquées au club nudiste : 4 rectangles (x, y, l, h ; l = 0 : aucun) par image, homme et femme
    C = ['/* Fichier généré par tools/gen_teaser.py — ne pas modifier. */',
         '/* Club nudiste : rectangles de tuiles couverts d\'une mosaïque (images HD, extra2.c). */', '',
         'static const unsigned char tz_cens[%d][2][16] = {' % len(names)]
    for name in names:
        row = []
        for female in (0, 1):
            rr = out[(name, female)]['cens'] + [(0, 0, 0, 0)] * 4
            row.append('{ ' + ', '.join('%d' % v for q in rr[:4] for v in q) + ' }')
        C.append('    { %s },   /* %s */' % (', '.join(row), name))
    C.append('};')
    open(os.path.join(ROOT, 'src', 'assets_cens.h'), 'w').write('\n'.join(C) + '\n')
    # (2.79) variantes sans vêtements : leurs propres données, dans les banques
    # libres (assets_nz<b>.c), et leurs descripteurs (assets_cens.h, lus par
    # extra2.c) ; une variante identique à l'image habillée la réutilise
    nb, narts = [], {}
    for name in names:
        for female in (0, 1):
            if (name, female, 'n') not in out:
                continue
            res = out[(name, female, 'n')]
            chr_ = pack(b''.join(res['tiles']))
            mp = bytes(res['nt'][y][x] for y in range(res['y0'], res['y1']) for x in range(res['x0'], res['x1']))
            key = (chr_, mp, bytes(res['at']))
            ref = [b for b in blobs if b['key'] == key]
            if ref:
                narts[(name, female)] = ('tz', ref[0]); continue
            same = [b for b in nb if b['key'] == key]
            if same:
                narts[(name, female)] = ('nz', same[0]); continue
            b = dict(key=key, size=len(chr_) + len(mp) + 64, id='%s_%sn' % (name, 'mf'[female]), res=res)
            nb.append(b); narts[(name, female)] = ('nz', b)
    nfree = dict(NZ_BANKS)
    for b in sorted(nb, key=lambda b: -b['size']):
        cand = [k for k in nfree if nfree[k] >= b['size']]
        assert cand, 'plus de place pour ' + b['id']
        k = max(cand, key=lambda k: nfree[k])
        nfree[k] -= b['size']; b['bank'] = k
    for bk, _ in NZ_BANKS:
        L = ['/* Fichier généré par tools/gen_teaser.py — ne pas modifier. */',
             '/* Club nudiste : images HD sans vêtements (banque %d). */' % bk, '']
        for b in nb:
            if b['bank'] == bk:
                chr_, mp, at = b['key']
                L += arr('tz_chr_' + b['id'], chr_) + arr('tz_map_' + b['id'], mp) + arr('tz_at_' + b['id'], at) + ['']
        open(os.path.join(ROOT, 'src', 'assets_nz%d.c' % bk), 'w').write('\n'.join(L) + '\n')
    C = ['', 'typedef struct { unsigned char bank, ntile, x0, w, y0, h, zoom; const unsigned char *chr, *map, *at; } tzn_t;']
    for b in nb + blobs:
        C.append('extern const unsigned char tz_chr_%s[], tz_map_%s[], tz_at_%s[];' % (b['id'], b['id'], b['id']))
    C.append('static const tzn_t tz_nude[%d][2] = {   /* homme, femme ; public : l\'image habillée */' % len(names))
    for name, pose, k, cen, zoom in SHOTS:
        row = []
        for female in (0, 1):
            src, b = narts.get((name, female), ('tz', blobs[arts[(name, female)]]))
            r = b['res']
            row.append('{ %d, %d, %d, %d, %d, %d, %d, tz_chr_%s, tz_map_%s, tz_at_%s }' % (
                b['bank'], len(r['tiles']), r['x0'], r['x1'] - r['x0'], r['y0'], r['y1'] - r['y0'], 1 if zoom else 0, b['id'], b['id'], b['id']))
        C.append('    { %s,\n      %s },' % tuple(row))
    C.append('};')
    open(os.path.join(ROOT, 'src', 'assets_cens.h'), 'a').write('\n'.join(C) + '\n')
    print('club nudiste : %d images sans vêtements, %d octets ; place restante %s' % (
        len(nb), sum(b['size'] for b in nb), ', '.join('%d:%d' % kv for kv in sorted(nfree.items()))))
    tot = sum(b['size'] for b in blobs)
    print('export : %d images, %d octets, banques %s' % (len(blobs), tot,
          ', '.join('%d (%d libres)' % (k, free[k]) for k in used)))
    return used


# ------------------------------------------------------------------ affiche VS
# présentation du match : bustes face à face, bras croisés, 2 palettes par
# joueur (A : sombre, peau ombrée, peau ; B : tenue ombrée, tenue, blanc ;
# le texte blanc de l'écran s'écrit dans les blocs B)
VS_LINE = False
POSES['bust'] = dict(
    s=dict(pelvis=(80, 118), torso=-90, legL=(97, 94, 165), legR=(83, 86, 15),
           armL=(98, -12), armR=(82, 192), tilt=0.0, turn=1.2,
           order=['legL', 'legR', 'torso', 'armR', 'armL', 'head']),
    face='focus', hand='X', rdir=0, rlen=50)
VS_W, VS_H = 12, 14                    # tuiles
VS_PALS_G = {0: (5, 1, 6), 1: (5, 1, 3)}  # Margaux : A (noir, peau, cheveux) ; B (noir, peau, tenue)
VS_PALS = {0: (5, 1, 3), 1: (5, 3, 6)}  # A : (sombre, peau, tenue) ; B : (sombre, tenue, blanc : texte blanc ombré de sombre)
# la peau ne devient jamais blanche ; le blanc des yeux se fond dans la peau
VS_COST = {(1, 6): 12, (2, 6): 12, (6, 1): 1, (6, 3): 6, (4, 3): 1}
VS_MAX = 95
VS_BANK = 10


def vs_render(female, mirror, k=1.38):
    W, H = VS_W * 8, VS_H * 8
    global VS_LINE
    f = Fig(W, H, k, 80 - (W / 2) / k, 36.0 if female in (1, 2) else 32.5)   # homme plus grand : tête entière dans le cadre
    VS_LINE = True
    light = female in (2, 3)        # cheveux clairs (Margaux, Papi) : palettes à part
    if female == 3:
        female = 0
    sk = skeleton(POSES['bust']['s'], female)
    body(f, sk, female, 'focus', racket_hand='X', racket_dir=0, racket_len=50)
    VS_LINE = False
    # (2.79) club nudiste : la poitrine des joueuses, masquée (rectangle de tuiles)
    cens = [r_ for r_ in cens_rects(f.T, [(sk, female in (1, 2))], VS_W, VS_H) if female in (1, 2)][1:2]
    if mirror:
        cens = [(VS_W - x - w, y, w, h) for x, y, w, h in cens]
    f.outline()
    # coupé au-dessus de l'ourlet du polo (ni short ni bas du vêtement)
    cut = VS_H * 8 - 10
    f.cls[cut:, :] = 0; f.pid[cut:, :] = 0
    # contours et yeux : couleur sombre (la couleur 0 est celle du fond de l'écran)
    pals = VS_PALS
    if light:
        # Margaux (blonde), Papi (gris) : les cheveux (classe 6 ici) ne partagent pas la
        # couleur des contours. Palettes A (noir, peau, cheveux), B (noir, peau, tenue)
        f.cls[f.cls == 6] = 1
        f.cls[f.cls == 2] = 1
        f.cls[f.cls == 4] = 3
        f.cls[f.cls == 5] = 6
        pals = VS_PALS_G
    f.cls[(f.cls == 0) & (f.pid > 0)] = 5
    if mirror:
        f.cls = np.ascontiguousarray(np.fliplr(f.cls)); f.pid = np.ascontiguousarray(np.fliplr(f.pid))
    COST_X.clear(); COST_X.update(VS_COST)
    idx, attr = quantize(f, pals)
    COST_X.clear()
    tiles, tmap, nt = [], {}, []
    for ty in range(VS_H):
        for tx in range(VS_W):
            t = tile_bytes(idx, tx, ty)
            if not any(t):
                nt.append(0); continue
            if t not in tmap:
                tmap[t] = len(tiles); tiles.append(t)
            nt.append(1 + tmap[t])
    assert len(tiles) <= VS_MAX, ('buste', female, mirror, len(tiles))
    # attributs : 1 bit par bloc 16 x 16 (B), 6 x 7 blocs
    ab = []
    for by in range(VS_H // 2):
        for bx in range(VS_W // 2):
            a_ = attr[by, bx]
            ab.append(1 if a_ == 1 else 0)
    return dict(tiles=tiles, nt=nt, ab=ab, idx=idx, attr=attr, cens=(cens + [(0, 0, 0, 0)])[0])


def vs_export():
    arts = {}
    for female in (0, 1, 2, 3):
        for mirror in (0, 1):
            arts[(female, mirror)] = vs_render(female, mirror)
    # (2.79) club nudiste : bustes sans vêtements (homme, femme), rangés avec
    # les images HD sans vêtements (assets_nz<b>.c, NZ_VS)
    nude = {}
    NUDE[0] = True
    try:
        for female in (0, 1):
            for mirror in (0, 1):
                nude[(female, mirror)] = vs_render(female, mirror)
    finally:
        NUDE[0] = False
    L = ['/* Fichier généré par tools/gen_teaser.py — ne pas modifier. */',
         '/* Affiche VS : bustes des joueurs (gauche, droite en miroir ; homme, femme), banque %d. */' % VS_BANK, '']
    H = ['/* Fichier généré par tools/gen_teaser.py — ne pas modifier. */', '',
         '#define VS_BANK %d' % VS_BANK, '#define VS_W %d' % VS_W, '#define VS_H %d' % VS_H]
    tot = 0
    for (female, mirror), a in arts.items():
        nm = 'vs_%s%s' % ('mfgp'[female], 'r' if mirror else 'l')
        chr_ = pack(b''.join(a['tiles']))
        L += arr_c(nm + '_chr', chr_) + arr_c(nm + '_map', bytes(a['nt'])) + arr_c(nm + '_ab', bytes(a['ab'])) + ['']
        H += ['extern const unsigned char %s_chr[], %s_map[], %s_ab[];' % (nm, nm, nm), '#define %s_N %d' % (nm.upper(), len(a['tiles']))]
        H += ['#define %s_CENS %d, %d, %d, %d   /* poitrine masquée au club nudiste (x, y, l, h en tuiles du buste) */' % ((nm.upper(),) + tuple(a['cens']))]
        tot += len(chr_) + len(a['nt']) + len(a['ab'])
        print('buste %s : %d tuiles' % (nm, len(a['tiles'])))
    for (female, mirror), a in nude.items():
        nm = 'vs_%s%sn' % ('mf'[female], 'r' if mirror else 'l')
        bk = NZ_VS[(female, mirror)]
        chr_ = pack(b''.join(a['tiles']))
        Lb = arr_c(nm + '_chr', chr_) + arr_c(nm + '_map', bytes(a['nt'])) + arr_c(nm + '_ab', bytes(a['ab'])) + ['']
        open(os.path.join(ROOT, 'src', 'assets_nz%d.c' % bk), 'a').write('\n'.join(Lb) + '\n')
        H += ['extern const unsigned char %s_chr[], %s_map[], %s_ab[];' % (nm, nm, nm),
              '#define %s_N %d' % (nm.upper(), len(a['tiles'])), '#define %s_BANK %d' % (nm.upper(), bk)]
        print('buste %s (sans vêtements, banque %d) : %d tuiles, %d octets' % (nm, bk, len(a['tiles']), len(chr_) + len(a['nt']) + len(a['ab'])))
    open(os.path.join(ROOT, 'src', 'assets_vs.c'), 'w').write('\n'.join(L))
    open(os.path.join(ROOT, 'src', 'assets_vs.h'), 'w').write('\n'.join(H) + '\n')
    print('affiche VS : %d octets' % tot)
    return arts


def arr_c(name, data):
    lines = ['const unsigned char %s[%d] = {' % (name, len(data))]
    for i in range(0, len(data), 16):
        lines.append('  ' + ','.join('0x%02X' % v for v in data[i:i + 16]) + ',')
    lines.append('};')
    return lines


def vs_preview(arts, dst):
    from PIL import Image as I
    sheet = I.new('RGB', (256 * 2, 112), (0, 0, 0))
    combos = [((0, 0), (1, 1), (0x27, 0x30, 0x0F), (0x37, 0x35, 0x0F)), ((1, 0), (0, 1), (0x17, 0x1C, 0x0F), (0x07, 0x27, 0x0F))]
    for i, (la, ra, cl, cr) in enumerate(combos):
        im = I.new('RGB', (256, 112), PAL[0x07])
        for side, key, c, x0 in ((0, la, cl, 0), (1, ra, cr, 160)):
            a = arts[key]
            sl, sd, ol, od = shades(*c)
            pa = {1: PAL[c[2]], 2: PAL[sl], 3: PAL[ol]}
            pb = {1: PAL[c[2]], 2: PAL[ol], 3: PAL[0x30]}
            for y in range(VS_H * 8):
                for x in range(VS_W * 8):
                    v = a['idx'][y, x]
                    if v:
                        im.putpixel((x0 + x, y), (pb if a['attr'][y // 16, x // 16] == 1 else pa)[v])
        sheet.paste(im, (i * 256, 0))
    sheet.resize((sheet.width * 3, sheet.height * 3), I.NEAREST).save(dst)


# (2.80) poignée de main du perdant : visage triste. Plutôt qu'une image
# entière (8 Ko de plus), les cases de 8 x 8 qui diffèrent de l'image
# souriante : (case de l'écran, tuile, 16 octets) ; une tuile propre à la
# case est réécrite, sinon une tuile libre (après celles de l'image) est
# prise et la case pointée dessus (extra2.c, shake_sad_f)
def sad_patch(out):
    name, pose, k, cen, zoom = [sh for sh in SHOTS if sh[0] == 'handshake'][0]
    blobs, tot = [], 0
    for key in ((0,), (1,), (2,), (0, 'n'), (1, 'n')):
        female, nude = key[0], len(key) > 1
        base = out[('handshake', female) + (('n',) if nude else ())]
        NUDE[0] = nude
        try:
            f, _ = render('handshake_sad', female, k, cen)
            sad = build(f, zoom)
        finally:
            NUDE[0] = False
        cnt = {}
        for row in base['nt']:
            for v in row:
                cnt[v] = cnt.get(v, 0) + 1
        spare = FIRST_TILE + len(base['tiles'])
        data = []
        for ty in range(len(base['nt'])):
            for tx in range(32):
                tj, ts = tile_bytes(base['idx'], tx, ty), tile_bytes(sad['idx'], tx, ty)
                if tj == ts:
                    continue
                assert base['attr'][ty // 2, tx // 2] == sad['attr'][ty // 2, tx // 2], ('palette', key, tx, ty)
                t = base['nt'][ty][tx]
                if not t or t >= T_STREAK or cnt[t] > 1:
                    t = spare; spare += 1
                    assert spare <= MAX_TILES + 1, 'plus de tuile libre'
                ad = 0x2000 + ty * 32 + tx
                data += [ad & 255, ad >> 8, t] + list(ts)
        blobs.append(data); tot += len(data)
        print('poignée de main triste %s%s : %d cases' % ('HFG'[female], ' nu' if nude else '', len(data) // 19))
    L = ['/* Fichier généré par tools/gen_teaser.py — ne pas modifier. */',
         '/* Poignée de main du perdant : cases du visage triste (extra2.c). */', '']
    for i, d in enumerate(blobs):
        L.append('static const unsigned char sad_p%d[%d] = { %s };' % (i, len(d) + 1, ', '.join(str(v) for v in [len(d) // 19] + d)))
    L.append('#define SAD_ART %d   /* TZ_HANDSHAKE */' % [sh[0] for sh in SHOTS].index('handshake'))
    L.append('static const unsigned char * const sad_pt[5] = { sad_p0, sad_p1, sad_p2, sad_p3, sad_p4 };   /* homme, femme, Margaux, homme nu, femme nue */')
    open(os.path.join(ROOT, 'src', 'assets_sad.h'), 'w').write('\n'.join(L) + '\n')
    print('poignée de main triste : %d octets' % tot)


if __name__ == '__main__':
    out, sheet = main()
    export(out)
    sad_patch(out)
    vs_preview(vs_export(), os.path.join(ROOT, 'assets', 'preview_vs.png'))
    cols = len(sheet[0])
    w, h = sheet[0][0].size
    big = Image.new('RGB', (w * cols, h * len(sheet)))
    for r, row in enumerate(sheet):
        for c, im in enumerate(row):
            big.paste(im, (c * w, r * h))
    dst = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'assets', 'preview_teaser.png')
    big.resize((big.width * 2, big.height * 2), Image.NEAREST).save(dst)
    print('aperçu :', dst)
