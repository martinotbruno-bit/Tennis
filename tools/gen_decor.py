#!/usr/bin/env python3
"""Petites animations des lieux (2.80) : 2 tuiles de sprites par lieu,
écrites au chargement du court (hors pluie) à la place des tuiles 4 (goutte)
et 8 (éclaboussure), inutilisées sans pluie. Écrit src/assets_decor.h."""
import os
T = {
 # stade et salle : flash d'appareil photo (palette 3 : 1 jaune, 3 blanc), puis sa lueur
 'flash': ["...3....",
           ".3.3.3..",
           "..333...",
           "3333333.",
           "..333...",
           ".3.3.3..",
           "...3....",
           "........"],
 'glow':  ["........",
           "........",
           "...1....",
           "..131...",
           "...1....",
           "........",
           "........",
           "........"],
 
 # court Margaux : cœur (palette 2 : 1 reflet beige, 3 rose : $25, gfx.c) et petit cœur
 'heart': [".33.33..",
           "3133333.",
           "3133333.",
           "3333333.",
           ".33333..",
           "..333...",
           "...3....",
           "........"],
 'heart2':["........",
           "........",
           ".3.3....",
           "33333...",
           ".333....",
           "..3.....",
           "........",
           "........"],
 # club nudiste : papillon blanc (piéride) ailes ouvertes / repliées (palette 3 : 1 taches jaunes, 2 corps bleu, 3 ailes blanches)
 'bfly1': ["........",
           "33....33",
           "313..313",
           "3332.333",
           ".332233.",
           "33.2..33",
           "33....33",
           "........"],
 'bfly2': ["........",
           "..3..3..",
           "..31.13.",
           "..3223..",
           "...22...",
           "..3..3..",
           "........",
           "........"],
 # château : oiseau ailes hautes / basses (palette 2 : 2 noir)
 'bird1': ["........",
           "2......2",
           ".2....2.",
           "..2..2..",
           "...22...",
           "........",
           "........",
           "........"],
 'bird2': ["........",
           "........",
           "........",
           "...22...",
           ".22..22.",
           "2......2",
           "........",
           "........"],
 # club : petit oiseau (passereau) ailes hautes / basses (palette 2 : 2 noir)
 'sbird1':["........",
           "........",
           ".2...2..",
           "..2.2...",
           "...2....",
           "........",
           "........",
           "........"],
 'sbird2':["........",
           "........",
           "........",
           "...2....",
           "..2.2...",
           ".2...2..",
           "........",
           "........"],
 # parc : écureuil de profil, deux temps de course (palette 3 du parc :
 # 1 jaune, 2 orange, 3 blanc)
 'sq1':   [".22.....",
           "2112....",
           "21.2..2.",
           ".2.2.222",
           "..222232",
           "..22222.",
           "..2...2.",
           ".2.....2"],
 'sq2':   [".22.....",
           "2112....",
           "21.2..2.",
           ".2.2.222",
           "..222232",
           "..22222.",
           "...2.2..",
           "...2.2.."],
}
SETS = [('flash', 'glow'), ('sbird1', 'sbird2'), ('flash', 'glow'), ('bfly1', 'bfly2'), ('heart', 'heart2'), ('bird1', 'bird2'), ('sq1', 'sq2')]
def chr_bytes(rows):
    lo, hi = [], []
    for r in rows:
        a = b = 0
        for c in r:
            v = 0 if c == '.' else int(c)
            a = (a << 1) | (v & 1); b = (b << 1) | (v >> 1)
        lo.append(a); hi.append(b)
    return lo + hi
out = ['/* généré par tools/gen_decor.py : tuiles des petites animations des lieux */\n',
       '/* (par lieu : tuile 4 puis tuile 8 de la table des sprites ; 0 = rien) */\n',
       'static const u8 dk_tiles[%d] = {\n' % (7 * 32)]
for s in SETS:
    data = (chr_bytes(T[s[0]]) + chr_bytes(T[s[1]])) if s else [0] * 32
    out.append('    ' + ', '.join('0x%02X' % v for v in data) + ',\n')
out.append('};\n')
open(os.path.join(os.path.dirname(__file__), '..', 'src', 'assets_decor.h'), 'w').write(''.join(out))
print('assets_decor.h : %d tuiles' % sum(2 for s in SETS if s))
