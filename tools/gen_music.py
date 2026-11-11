#!/usr/bin/env python3
"""Royal Martinot — musiques : génère src/music_data.c (banque 10).

Lit les partitions de tools/music_src.c (mélodie et basse de chaque morceau)
et ajoute deux voix :
  - l'HARMONIE (carré 1) : notes de l'accord dont la basse donne la
    fondamentale (tonalité de do majeur), en contretemps « oum-pa » pour la
    polka et la valse, tenues pour la marche, l'hymne et les airs calmes ;
    une note qui frotterait contre la mélodie (demi-ton, ton) est remplacée
    par une autre note de l'accord ;
  - les PERCUSSIONS (bruit) : grosse caisse, caisse claire, charleston.
Chaque voix a exactement la durée de la mélodie (les boucles restent
calées). Les jingles du match (point, jeu, défaite) restent à une voix.

Format d'une voix : paires (note, durée en trames) ; note 0..47 (do 2 à
si 5), $FE silence, $FF fin. Percussions : 0 grosse caisse, 1 caisse
claire, 2 charleston, $FE silence."""
import os, re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
src = open(os.path.join(ROOT, 'tools', 'music_src.c')).read()
src_nc = re.sub(r'/\*.*?\*/', '', src, flags=re.S)
defs = {k: int(v, 0) for k, v in re.findall(r'#define (\w+) (\w+)', src_nc)}

def arr(name):
    m = re.search(r'static const u8 ' + name + r'\[\]\s*=\s*\{(.*?)\};', src_nc, re.S)
    toks = [t.strip() for t in m.group(1).split(',') if t.strip()]
    vals = [defs[t] if t in defs else int(t, 0) for t in toks]
    pairs = list(zip(vals[0::2], vals[1::2]))
    assert pairs[-1][0] == 0xFF
    return pairs[:-1]

R = 0xFE
KICK, SNARE, HAT = 0, 1, 2
# tierce et quinte diatoniques en do majeur, par classe de hauteur
THIRD = {0: 4, 2: 3, 4: 3, 5: 4, 7: 4, 9: 3, 11: 3}
FIFTH = {0: 7, 2: 7, 4: 7, 5: 7, 7: 7, 9: 7, 11: 6}

def at(track, t):
    """note jouée à l'instant t (ou None)"""
    pos = 0
    for n, d in track:
        if pos <= t < pos + d:
            return None if n == R else n
        pos += d
    return None

def chord_tone(root, which, lo=27, hi=39):
    pc = root % 12
    off = {0: 0, 1: THIRD.get(pc, 4), 2: FIFTH.get(pc, 7)}[which]
    n = (root + off) % 12
    while n < lo:
        n += 12
    while n > hi:
        n -= 12
    return n

def clash(a, b):
    return b is not None and (abs(a - b) % 12) in (1, 2, 10, 11)

def pick(root, prefs, mel_note):
    for w in prefs:
        n = chord_tone(root, w)
        if not clash(n, mel_note):
            return n
    return None

def harmony(mel, bass, bar, slots, sustain=False):
    """slots : positions (en trames dans la mesure) des notes d'harmonie,
    avec la durée et la note de l'accord voulue (1 tierce, 2 quinte) ;
    sustain : notes tenues jusqu'au slot suivant"""
    total = sum(d for _, d in mel)
    out = []
    t = 0
    while t < total:
        b0 = t - (t % bar)
        root = at(bass, b0)
        if root is None:
            root = at(bass, t)
        placed = False
        for (pos, dur, which) in slots:
            if b0 + pos == t and root is not None:
                d = min(dur, total - t)
                n = pick(root, (which, 3 - which if which else 1, 0), at(mel, t))
                out.append((n if n is not None else R, d))
                t += d
                placed = True
                break
        if not placed:
            # silence jusqu'au prochain slot (ou fin de mesure)
            nxt = [b0 + p for (p, _, _) in slots if b0 + p > t] + [b0 + bar, total]
            d = min(nxt) - t
            out.append((R, d))
            t += d
    return merge(out)

def drums(total, bar, pattern):
    """pattern : liste (position dans la mesure, instrument)"""
    out = []
    t = 0
    while t < total:
        b0 = t - (t % bar)
        hits = sorted(p for p in pattern if b0 + p[0] >= t)
        if hits and b0 + hits[0][0] == t:
            inst = hits[0][1]
            nxt = [b0 + p for (p, _) in pattern if b0 + p > t] + [b0 + bar, total]
            d = min(nxt) - t
            out.append((inst, d))
            t += d
        else:
            nxt = [b0 + p for (p, _) in pattern if b0 + p > t] + [b0 + bar, total]
            d = min(nxt) - t
            out.append((R, d))
            t += d
    return merge(out)

def merge(track):
    """fusionne les silences consécutifs ; découpe les durées > 255"""
    out = []
    for n, d in track:
        if d <= 0:
            continue
        if out and n == R and out[-1][0] == R and out[-1][1] + d <= 255:
            out[-1] = (R, out[-1][1] + d)
        else:
            while d > 255:
                out.append((n, 255)); d -= 255
            out.append((n, d))
    return out

def length(tr):
    return sum(d for _, d in tr)

songs = []   # (id, nom, mel, bass, harm, drum, boucle, timbre mélodie, timbre harmonie)
def song(i, name, mel, bass=None, harm=None, drum=None, loop=0, mduty=0x86, hduty=0x72):
    if harm is not None:
        assert length(harm) == length(mel), (name, length(harm), length(mel))
    if drum is not None:
        assert length(drum) <= length(mel), (name, 'percussions')
    songs.append((i, name, mel, bass, harm, drum, loop, mduty, hduty))

# --- polka du titre et du menu : 2 temps de 24 trames ; « oum-pa » en
# contretemps (tierce puis quinte), grosse caisse sur les temps, caisse claire
# sur les contretemps
m, b = arr('mus_title_mel'), arr('mus_title_bass')
song(1, 'title', m, b,
     harmony(m, b, 48, [(12, 6, 1), (36, 6, 2)]),
     drums(length(m), 48, [(0, KICK), (12, SNARE), (24, KICK), (36, SNARE)]),
     loop=1, mduty=0x36, hduty=0x73)
song(2, 'point', arr('mus_point_mel'))
song(3, 'game', arr('mus_game_mel'))
# --- victoire : tierces tenues, roulement de caisse claire et grosse caisse
m, b = arr('mus_vict_mel'), arr('mus_vict_bass')
song(4, 'vict', m, b,
     harmony(m, b, 24, [(0, 24, 1)]),
     drums(length(m), 24, [(0, KICK), (6, SNARE), (12, SNARE), (18, SNARE)]),
     mduty=0xB8, hduty=0x74)
song(5, 'def', arr('mus_def_mel'))
# --- marche de la finale : 4 temps de 20 trames ; accord tenu par demi-mesure,
# grosse caisse sur 1 et 3, caisse claire sur 2 et 4, charleston aux croches
m, b = arr('mus_final_mel'), arr('mus_final_bass')
song(6, 'final', m, b,
     harmony(m, b, 40, [(0, 40, 1)]),
     drums(length(m), 80, [(0, KICK), (10, HAT), (20, SNARE), (30, HAT), (40, KICK), (50, HAT), (60, SNARE), (70, HAT)]),
     loop=1, mduty=0xB7, hduty=0x74)
# --- valse du court Margaux : 3 temps de 12 trames ; « oum-pa-pa » en boîte à
# musique (tierce et quinte sur les temps 2 et 3), charleston léger
m, b = arr('mus_margaux_mel'), arr('mus_margaux_bass')
song(7, 'margaux', m, b,
     harmony(m, b, 36, [(12, 6, 1), (24, 6, 2)]),
     drums(length(m), 36, [(0, KICK), (12, HAT), (24, HAT)]),
     loop=1, mduty=0x77, hduty=0x73)
# --- interlude de fin de set : air calme, tierces tenues, charleston discret
m, b = arr('mus_break_mel'), arr('mus_break_bass')
song(8, 'break', m, b,
     harmony(m, b, 24, [(0, 24, 1)]),
     drums(length(m), 48, [(12, HAT), (36, HAT)]),
     loop=1, mduty=0x76, hduty=0x72)
# --- générique : percussions seulement pendant la partie majestueuse
# (timbales : grosse caisse sur les temps), puis roulement quand ça s'emballe
m, b = arr('mus_intro_mel'), arr('mus_intro_bass')
maj = drums(480, 60, [(0, KICK), (30, HAT)])
emb = drums(240, 30, [(0, KICK), (15, SNARE)])
roll = drums(160, 16, [(0, SNARE), (8, SNARE)])
intro_dr = merge(maj + emb + roll + [(R, length(m) - 880)])
song(9, 'intro', m, b, None, intro_dr, mduty=0xB8)
# --- hymne du générique de fin : tierces tenues (blanches), timbales sur 1 et 3
m, b = arr('mus_cred_mel'), arr('mus_cred_bass')
song(10, 'cred', m, b,
     harmony(m, b, 48, [(0, 48, 1)]),
     drums(length(m), 96, [(0, KICK), (48, KICK)]),
     loop=1, mduty=0x76, hduty=0x74)

def c_bytes(name, tr):
    data = []
    for n, d in tr:
        data += [n, d]
    data += [0xFF, 0]
    s = 'static const u8 %s[] = {' % name
    for i in range(0, len(data), 16):
        s += '\n  ' + ', '.join('0x%02X' % v for v in data[i:i + 16]) + ','
    return s + '\n};\n', len(data)

out = ['/* Fichier généré par tools/gen_music.py — ne pas modifier. */',
       '/* Musiques (banque 10) : copiées en RAM de cartouche par music_play. */',
       '#include "audio.h"', '']
tab = ['const u8 * const song_trk[11][4] = {', '  { 0, 0, 0, 0 },']
lens = ['  { 0, 0, 0, 0 },']
flags = ['0']
duty = ['0, 0']
total = 0
biggest = 0
for (i, name, mel, bass, harm, drum, loop, md, hd) in songs:
    ptrs = []
    ln = []
    size = 0
    for kind, tr in (('mel', mel), ('bass', bass), ('harm', harm), ('drum', drum)):
        if tr is None:
            ptrs.append('0')
            ln.append('0')
            continue
        txt, n = c_bytes('mus_%s_%s' % (name, kind), tr)
        out.append(txt)
        ptrs.append('mus_%s_%s' % (name, kind))
        ln.append(str(n))
        size += n
    total += size
    biggest = max(biggest, size)
    tab.append('  { %s },' % ', '.join(ptrs))
    lens.append('  { %s },' % ', '.join(ln))
    flags.append(str(loop))
    duty.append('0x%02X, 0x%02X' % (md, hd))
tab.append('};')
out += tab
out.append('const u16 song_len[11][4] = {')
out += lens
out.append('};')
out.append('const u8 song_loop[11] = { %s };' % ', '.join(flags))
out.append('const u8 song_duty[11][2] = { %s };' % ', '.join('{ %s }' % d for d in duty))
out.append('/* plus gros morceau : %d octets (tampon MUS_BUF de audio.c) ; total %d octets */' % (biggest, total))
open(os.path.join(ROOT, 'src', 'music_data.c'), 'w').write('\n'.join(out) + '\n')
print('musiques : %d octets, plus gros morceau %d octets' % (total, biggest))
