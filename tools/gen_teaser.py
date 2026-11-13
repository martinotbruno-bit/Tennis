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
        toe = (toe[0] + u[0] * 2, toe[1] + u[1] * 2)            # pied plus long
        n = (u[1], -u[0]) if u[0] >= 0 else (-u[1], u[0])          # vers le haut
        Q = lambda t, h: (an[0] + u[0] * t + n[0] * h, an[1] + u[1] * t + n[1] * h)
        L = math.hypot(toe[0] - an[0], toe[1] - an[1])
        shoe = f.m_poly([Q(-6, 5), Q(-1, 7), Q(L * 0.45, 4.5), Q(L + 1, 2.2), Q(L + 3, -1), Q(L + 1, -3.5), Q(-6, -3.5), Q(-7.5, 1)])
        shoe |= f.m_ell(Q(L, -0.6), 3.3, 2.9)
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
        if not female:
            # manche courte du polo
            f.part(f.m_caps(sh_, lerp(sh_, el, 0.36), 7.4, 6.8), O, o, sh=(2.5, 1.5), group=g)
        hand = j.get('hd' + side, lerp(el, wr, 1.25))
        if hold:
            # poignet éponge blanc
            f.part(f.m_caps(lerp(el, wr, 0.78), wr, 4.6, 4.4), W, None, group=g)
        h = f.part(f.m_ell(hand, 5.2 * AW, 5.2 * AW), S, s, sh=(2, 2), group=g)
        if hold:
            ret['hand'] = hand
        else:
            # doigts repliés
            f.paint(f.m_line([(hand[0] - 3, hand[1] + 1), (hand[0] + 2, hand[1] + 1)], 0.7), 2, within=[h])

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
                return [P_(0.13, -0.5), P_(0.22, 3.2), P_(0.31, 4.4), P_(0.4, 3.0), P_(0.5, 0.0),
                        P_(0.6, -2.0), P_(0.7, -2.0), P_(0.8, 0.5), (hp_[0] + sx * 7.5, hp_[1] - 7), (hp_[0] + sx * 8, hp_[1] - 2)]
            # bretelles : elles montent jusqu'en haut de l'épaule, le long de sa pente, jusqu'au cou
            pts = [(shL[0] + 0.5, shL[1] - 4.8), (nk[0] - 6.6, nk[1] - 2.9), (nk[0] - 4.0, nk[1] - 1.6),
                   (nk[0] - 3.6, nk[1] + 7), (nk[0], nk[1] + 9), (nk[0] + 3.6, nk[1] + 7),
                   (nk[0] + 4.0, nk[1] - 1.6), (nk[0] + 6.6, nk[1] - 2.9), (shR[0] - 0.5, shR[1] - 4.8), (shR[0] + 1, shR[1] + 3)]
            pts += side(shR, hpR, 1) + list(reversed(side(shL, hpL, -1))) + [(shL[0] - 1, shL[1] + 3)]
            tid = f.part(f.m_poly(pts), O, o, sh=(4, 2))
            # poitrine : volume rond, ombre douce sous chaque sein et sur le côté
            bx = abs(hpR[0] - hpL[0]) * 0.27
            by = lerp(lerp(shL, shR, 0.5), lerp(hpL, hpR, 0.5), 0.26)[1]
            for sx in (-1, 1):
                c_ = (nk[0] + sx * bx, by)
                e_ = f.m_ell(c_, bx * 1.18, 7.2)
                low = ~f.m_poly([(c_[0] - 20, c_[1] - 20), (c_[0] + 20, c_[1] - 20), (c_[0] + 20, c_[1] + 1.5), (c_[0] - 20, c_[1] + 1.5)])
                mid = ~f.m_poly([(nk[0] - 1.6, c_[1] - 20), (nk[0] + 1.6, c_[1] - 20), (nk[0] + 1.6, c_[1] + 20), (nk[0] - 1.6, c_[1] + 20)])
                f.paint(e_ & ~shift_mask(f, (0, 2.2), e_) & low & mid, 4, within=[tid])   # ombre douce sous le sein
            ret['dress'] = tid
            return
        # short ample : jambes droites, plus larges que la cuisse, ourlet net
        m = f.m_poly([(hpL[0] - 6, hpL[1] - 8), (hpR[0] + 6, hpR[1] - 8), (hpR[0] + 9, hpR[1] + 2), (hpL[0] - 9, hpL[1] + 2)])
        hems = []
        for side, sx in (('L', -1), ('R', 1)):
            hp, kn = j['hip' + side], j['kn' + side]
            d = (kn[0] - hp[0], kn[1] - hp[1]); L = math.hypot(*d) or 1; u = (d[0] / L, d[1] / L); n = (-u[1], u[0])
            e = lerp(hp, kn, 0.48)
            leg_ = f.m_caps((hp[0] - u[0] * 4, hp[1] - u[1] * 4), e, 11.2, 11.6)
            keep = f.m_poly([(e[0] + n[0] * 40, e[1] + n[1] * 40), (e[0] - n[0] * 40, e[1] - n[1] * 40),
                             (e[0] - n[0] * 40 - u[0] * 90, e[1] - n[1] * 40 - u[1] * 90),
                             (e[0] + n[0] * 40 - u[0] * 90, e[1] + n[1] * 40 - u[1] * 90)])
            m |= leg_ & keep
            hems.append(((e[0] + n[0] * 11, e[1] + n[1] * 11), (e[0] - n[0] * 11, e[1] - n[1] * 11), u))
        sid = f.part(m, D, None)
        for (a_, b_, u_) in hems:
            f.paint(f.m_line([(a_[0] - u_[0] * 2.2, a_[1] - u_[1] * 2.2), (b_[0] - u_[0] * 2.2, b_[1] - u_[1] * 2.2)], 0.6), 0, within=[sid])   # ourlet
        cr = lerp(hpL, hpR, 0.5)
        f.paint(f.m_line([(cr[0], cr[1] + 2), lerp(cr, lerp(lerp(hpL, j['knL'], 0.48), lerp(hpR, j['knR'], 0.48), 0.5), 1.0)], 0.9), 0, within=[sid])   # entrejambe
        # polo, porté par-dessus le short
        top_l = (shL[0] - 2, shL[1] - 5); top_r = (shR[0] + 2, shR[1] - 5)
        pts = [top_l, (nk[0] - 6, nk[1] - 1), (nk[0] + 6, nk[1] - 1), top_r, (shR[0] + 5, shR[1] + 2),
               (ch_r[0] + 2, ch_r[1]), (wr[0] + 2, wr[1]), (hpR[0] + 12, hpR[1] - 10), (hpR[0] + 12.5, hpR[1] - 5),
               (hpL[0] - 12.5, hpL[1] - 5), (hpL[0] - 12, hpL[1] - 10), (wl[0] - 2, wl[1]), (ch_l[0] - 2, ch_l[1]), (shL[0] - 5, shL[1] + 2)]
        tm = f.m_poly(pts)
        tid = f.part(tm, O, o, sh=(5, 2))
        f.paint(tm & ~shift_mask(f, (0, -1.5), tm) & shift_mask(f, (0, 8), tm) | (tm & ~shift_mask(f, (0, -1.5), tm) & ~shift_mask(f, (0, 30), tm)), 4, within=[tid])   # ourlet du polo
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
        # profil arrondi : la jupe s'évase vite sous la taille puis plus
        # doucement jusqu'à l'ourlet (pas d'angle droit en haut sur les côtés)
        prof = []
        for (A, B) in (((top_l[0] + 2, top_l[1]), hem_l), ((top_r[0] - 2, top_r[1]), hem_r)):
            for fr in (0.1, 0.25, 0.42, 0.6, 0.8):
                prof.append((A[0] + (B[0] - A[0]) * fr ** 0.7, A[1] + (B[1] - A[1]) * fr))
        sk = hull([(top_l[0] + 2, top_l[1]), (top_r[0] - 2, top_r[1]), hem_l, hem_r,
                   (hl[0] - nl[0] * 6, hl[1] + 4), (hr[0] - nr[0] * 6, hr[1] + 4)] + prof
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
            f.part(f.m_caps(R(6, -10), R(13, 7), 4, 2.2) | f.m_ell(R(6.5, -9.5), 4.5, 3.5, tilt), D, None)
            back = f.m_ell(R(0, -1), 12.2, 12.5, tilt) & ~f.m_poly([R(-20, 6), R(20, 6), R(20, 30), R(-20, 30)])
            f.part(back, D, None)
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
            hair = f.m_ell(R(0, -5.4), 11.1, 8.8, tilt) & ~f.m_ell(R(turn * 0.6, 3), 10.3, 8.4, tilt)
            hair |= f.m_poly([R(-10.6, -3), R(-9.6, 2.6), R(-8.7, -2.5)]) | f.m_poly([R(10.6, -3), R(9.6, 2.6), R(8.7, -2.5)])
        hr = f.part(hair, D, None, group=hid)
        f.paint(f.m_line([R(-3.5, -12.5), R(-2.5, -8.5)], 0.6), 0, within=[hr])          # raie
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
    for p in order:
        if p == 'legL': leg('L')
        elif p == 'legR': leg('R')
        elif p == 'armL': arm('L', racket_hand == 'L')
        elif p == 'armR': arm('R', racket_hand == 'R')
        elif p == 'torso': torso()
        elif p == 'head': head()
        elif p == 'skirt': skirt()
        elif p == 'racket': racket()
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

    def eye(sx, look=0.0, lid=0.0):
        c = R(sx * 4.4, -0.2)
        P(f.m_ell(c, 2.4 if female else 2.2, 1.35 - lid * 0.5, tilt), 6)
        P(f.m_ell((c[0] + look, c[1] + 0.15), 1.15, 1.15), 0)
        P(line([R(sx * 4.4 - 2.6, -1.3 + lid), R(sx * 4.4 + 2.6, -1.3 + lid)], 0.9 if female else 0.6), 5)   # paupière
        if female:
            P(line([R(sx * 6.8, -1.2 + lid), R(sx * 7.9, -2.3 + lid)], 0.7), 5)                 # cils
    def brow(sx, inner=-3.3, outer=-3.6, arch=0.0):
        if female:
            P(f.m_arc(R(sx * 4.6, -2.6 + arch), 3.0, 1.4, 200, 340, bw), 5)
        else:
            P(line([R(sx * 7.2, outer), R(sx * 2.2, inner)], bw), 5)
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
            P(line([R(-2.6, 8.9), R(0, 9.3), R(2.6, 8.9)], 0.7), 0)
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
             tilt=p.get('tilt', 0.0), turn=p.get('turn', 0.0), order=p['order'])
    j['shL'] = (shc[0] - perp[0] * sw, shc[1] - perp[1] * sw + (2 if female else 0))
    j['shR'] = (shc[0] + perp[0] * sw, shc[1] + perp[1] * sw + (2 if female else 0))
    j['hipL'] = (P[0] - perp[0] * hw, P[1] - perp[1] * hw)
    j['hipR'] = (P[0] + perp[0] * hw, P[1] + perp[1] * hw)
    for sd in ('L', 'R'):
        th, sn, tdir = p['leg' + sd]
        kn = add(j['hip' + sd], dvec(th, G['thigh']))
        an = add(kn, dvec(sn, G['shin']))
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

# 4. raquette fracassée au sol : penché en avant (buste raccourci), bras abattu, cri de colère
POSES['angry'] = dict(
    s=dict(pelvis=(88, 124), torso=-96, torso_len=0.9, legL=(116, 96, 165), legR=(64, 84, 15),
           armL=(112, 78), armR=(60, 98), tilt=-0.1, turn=-0.5, head_bend=4,
           order=['legR', 'legL', 'torso', 'armR', 'racket', 'armL', 'head']),
    face='angry', hand='L', rdir=100, rlen=50)

# plans : (nom, pose, échelle, (x, y) de base visé au centre ; None : pieds au sol)
# zoom : True si le visage est assez grand pour sa palette (peau, sombre, blanc)
SHOTS = [
    ('impact', 'impact', 1.4, (50, 102), False),     # gros plan : buste, raquette à l'impact
    ('slide', 'slide', 1.12, None, False),           # glissade, en pied
    ('slide_z', 'slide', 1.3, (100, None), False),   # zoom : la chaussure qui glisse
    ('victory', 'victory', 1.0, (82, None), False),        # bras levés, en pied
    ('victory_z', 'victory', 2.0, (80, 62), True),   # zoom : buste et visage
    ('angry', 'angry', 1.12, None, False),           # raquette au sol, en pied
    ('angry_z', 'angry', 3.4, (83, 70), True),       # zoom : le visage qui crie
]


def render(pose, female, k, center):
    P = POSES[pose]
    W, H = 256, GROUND_ROW * 8
    cx, cy = center if center else (90, None)
    if cy is None:
        oy = 206.5 - (H - 1) / k          # pieds au sol (dernière rangée du cadre)
    else:
        oy = cy - (H / 2) / k
    f = Fig(W, H, k, cx - 128 / k, oy)
    r = body(f, skeleton(P['s'], female), female, P['face'], racket_hand=P['hand'], racket_dir=P['rdir'], racket_len=P['rlen'])
    f.outline()
    f.leg_ids = [r[k_] for k_ in ('legL', 'legR') if k_ in r]
    f.joints = skeleton(P['s'], female)
    r = {key: f.T(v) for key, v in r.items() if isinstance(v, tuple)}
    return f, r


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
SAMPLES = [(0x27, 0x30, 0x0F), (0x37, 0x1C, 0x0F), (0x07, 0x27, 0x0F), (0x27, 0x35, 0x28)]


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
        for female in (0, 1):
            f, r = render(pose, female, k, cen)
            res = build(f, zoom)
            res['r'] = r
            out[(name, female)] = res
            print('%-10s %s : %3d tuiles, colonnes %d-%d, rangées %d-%d' % (
                name, 'F' if female else 'H', len(res['tiles']), res['x0'], res['x1'] - 1, res['y0'], res['y1'] - 1))
            if not female:
                sheet.append([preview(res, c) for c in SAMPLES[:2]])
            else:
                sheet[-1] += [preview(res, c) for c in SAMPLES[2:]]
    return out, sheet


# ------------------------------------------------------------------ export
# banques libres (octets disponibles, marge comprise)
BANKS = [(9, 13600), (12, 11900), (4, 12200), (10, 11400), (2, 7900)]


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
        for female in (0, 1):
            res = out[(name, female)]
            chr_ = pack(b''.join(res['tiles']))
            mp = bytes(res['nt'][y][x] for y in range(res['y0'], res['y1']) for x in range(res['x0'], res['x1']))
            key = (chr_, mp, bytes(res['at']))
            same = [i for i, b in enumerate(blobs) if b['key'] == key]
            if same:
                arts[(name, female)] = same[0]
                continue
            blobs.append(dict(key=key, size=len(chr_) + len(mp) + 64, id='%s_%s' % (name, 'f' if female else 'm'), res=res))
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
    H.append('static const tz_art_t tz_art[TZ_N][2] = {')
    for name, pose, k, cen, zoom in SHOTS:
        row = []
        for female in (0, 1):
            b = blobs[arts[(name, female)]]; r = b['res']
            row.append('{ %d, %d, %d, %d, %d, %d, %d, tz_chr_%s, tz_map_%s, tz_at_%s }' % (
                b['bank'], len(r['tiles']), r['x0'], r['x1'] - r['x0'], r['y0'], r['y1'] - r['y0'], 1 if zoom else 0, b['id'], b['id'], b['id']))
        H.append('    { %s,\n      %s },' % tuple(row))
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
    tot = sum(b['size'] for b in blobs)
    print('export : %d images, %d octets, banques %s' % (len(blobs), tot,
          ', '.join('%d (%d libres)' % (k, free[k]) for k in used)))
    return used


# ------------------------------------------------------------------ affiche VS
# présentation du match : bustes face à face, bras croisés, 2 palettes par
# joueur (A : sombre, peau ombrée, peau ; B : tenue ombrée, tenue, blanc ;
# le texte blanc de l'écran s'écrit dans les blocs B)
POSES['bust'] = dict(
    s=dict(pelvis=(80, 118), torso=-90, legL=(97, 94, 165), legR=(83, 86, 15),
           armL=(98, -12), armR=(82, 192), tilt=0.0, turn=1.2,
           order=['legL', 'legR', 'torso', 'armR', 'armL', 'head']),
    face='focus', hand='X', rdir=0, rlen=50)
VS_W, VS_H = 12, 14                    # tuiles
VS_PALS = {0: (5, 1, 3), 1: (5, 3, 6)}  # A : (sombre, peau, tenue) ; B : (sombre, tenue, blanc : texte blanc ombré de sombre)
# la peau ne devient jamais blanche ; le blanc des yeux se fond dans la peau
VS_COST = {(1, 6): 12, (2, 6): 12, (6, 1): 1, (6, 3): 6, (4, 3): 1}
VS_MAX = 95
VS_BANK = 10


def vs_render(female, mirror, k=1.38):
    W, H = VS_W * 8, VS_H * 8
    f = Fig(W, H, k, 80 - (W / 2) / k, 36.0)
    body(f, skeleton(POSES['bust']['s'], female), female, 'focus', racket_hand='X', racket_dir=0, racket_len=50)
    f.outline()
    # coupé au-dessus de l'ourlet du polo (ni short ni bas du vêtement)
    cut = VS_H * 8 - 10
    f.cls[cut:, :] = 0; f.pid[cut:, :] = 0
    if mirror:
        f.cls = np.ascontiguousarray(np.fliplr(f.cls)); f.pid = np.ascontiguousarray(np.fliplr(f.pid))
    COST_X.clear(); COST_X.update(VS_COST)
    idx, attr = quantize(f, VS_PALS)
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
    return dict(tiles=tiles, nt=nt, ab=ab, idx=idx, attr=attr)


def vs_export():
    arts = {}
    for female in (0, 1):
        for mirror in (0, 1):
            arts[(female, mirror)] = vs_render(female, mirror)
    L = ['/* Fichier généré par tools/gen_teaser.py — ne pas modifier. */',
         '/* Affiche VS : bustes des joueurs (gauche, droite en miroir ; homme, femme), banque %d. */' % VS_BANK, '']
    H = ['/* Fichier généré par tools/gen_teaser.py — ne pas modifier. */', '',
         '#define VS_BANK %d' % VS_BANK, '#define VS_W %d' % VS_W, '#define VS_H %d' % VS_H]
    tot = 0
    for (female, mirror), a in arts.items():
        nm = 'vs_%s%s' % ('f' if female else 'm', 'r' if mirror else 'l')
        chr_ = pack(b''.join(a['tiles']))
        L += arr_c(nm + '_chr', chr_) + arr_c(nm + '_map', bytes(a['nt'])) + arr_c(nm + '_ab', bytes(a['ab'])) + ['']
        H += ['extern const unsigned char %s_chr[], %s_map[], %s_ab[];' % (nm, nm, nm), '#define %s_N %d' % (nm.upper(), len(a['tiles']))]
        tot += len(chr_) + len(a['nt']) + len(a['ab'])
        print('buste %s : %d tuiles' % (nm, len(a['tiles'])))
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


if __name__ == '__main__':
    out, sheet = main()
    export(out)
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
