#!/usr/bin/env python3
"""Royal Martinot — échantillons DPCM (canal DMC de la NES) : voix de
l'arbitre (« Out! », « Fault! », « Let! ») et « pok » de la balle.

Voix synthétisées par espeak-ng (voix anglaise, comme à Wimbledon) ; le
« pok » est calculé (rebond amorti de 30 ms). Les échantillons sont écrits
dans src/dpcm.s (segment DPCM de la banque fixe : le DMC ne lit que
$C000-$FFFF, à des adresses multiples de 64).

Encodage DMC : 1 bit par pas, +2 ou -2 sur un compteur de 0 à 127. Chaque
échantillon part de 0 et y revient (montée et descente douces) : entre deux
sons, $4011 reste à 0 et le triangle et le bruit gardent leur volume.

Usage : python3 tools/gen_voice.py   (nécessite espeak-ng ; src/dpcm.s est
versionné, la compilation n'en a pas besoin)."""
import os, subprocess, tempfile, wave, struct, math

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# fréquences du DMC (NTSC) par indice de $4010
RATES = [4181.71, 4709.93, 5264.04, 5593.04, 6257.95, 7046.35, 7919.35, 8363.42,
         9419.86, 11186.1, 12604.0, 13982.6, 16884.6, 21306.8, 24858.0, 33143.9]

def speak(word, speed=165, pitch=38):
    with tempfile.TemporaryDirectory() as d:
        f = os.path.join(d, 'v.wav')
        subprocess.run(['espeak-ng', '-v', 'en-gb', '-s', str(speed), '-p', str(pitch), '-a', '200', '-w', f, word],
                       check=True)
        w = wave.open(f)
        n, sr = w.getnframes(), w.getframerate()
        data = struct.unpack('<%dh' % n, w.readframes(n))
    x = [v / 32768.0 for v in data]
    # coupe les silences du début et de la fin
    th = 0.03
    a = next(i for i, v in enumerate(x) if abs(v) > th)
    b = len(x) - next(i for i, v in enumerate(reversed(x)) if abs(v) > th)
    return x[max(0, a - 50):b + 200], sr

def resample(x, sr, to):
    n = int(len(x) * to / sr)
    out = []
    for i in range(n):
        t = i * sr / to
        k = int(t)
        f = t - k
        a = x[k] if k < len(x) else 0
        b = x[k + 1] if k + 1 < len(x) else 0
        out.append(a + (b - a) * f)
    return out

def encode(x, gain=1.0, mid=36):
    """x : signal -1..1 ; renvoie les octets DMC (bits de poids faible en
    premier). Le compteur part de 0, monte jusqu'à mid, suit le signal puis
    redescend à 0."""
    peak = max(1e-6, max(abs(v) for v in x))
    tgt = []
    ramp = mid // 2
    tgt += [2 * k for k in range(ramp)]                       # montée douce
    tgt += [mid + v / peak * gain * (mid - 4) for v in x]
    tgt += [mid - 2 * k for k in range(ramp + 1)]             # descente
    bits = []
    c = 0
    for t in tgt:
        if t > c and c <= 125:
            bits.append(1); c += 2
        else:
            if c >= 2:
                bits.append(0); c -= 2
            else:
                bits.append(1 if len(bits) % 2 == 0 else 0); c += 2 if bits[-1] else -2
    # longueur : 16 L + 1 octets ; le reste alterne +2/-2 (silence à 0)
    while len(bits) % 8:
        bits.append(0 if c >= 2 else 1); c += -2 if bits[-1] == 0 else 2
    data = bytearray()
    for i in range(0, len(bits), 8):
        v = 0
        for k in range(8):
            v |= bits[i + k] << k
        data.append(v)
    while (len(data) - 1) % 16:
        data.append(0x55 if c < 2 else 0x00)
    return bytes(data)

samples = []
# voix : 7 919 Hz (indice 6), assez pour être compréhensible, et peu de place
VOICE_RATE = 6
for name, word, speed in (('out', 'Out!', 160), ('fault', 'Fault!', 175), ('let', 'Let!', 170)):
    x, sr = speak(word, speed)
    samples.append((name, encode(resample(x, sr, RATES[VOICE_RATE]), 1.0), VOICE_RATE))
# « pok » : rebond amorti (sinus de 900 Hz et une pointe de bruit), 30 ms
POK_RATE = 13
sr = RATES[POK_RATE]
pok = []
seed = 1
for i in range(int(sr * 0.03)):
    t = i / sr
    seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF
    nz = (seed / 0x7FFFFFFF - 0.5) * (0.6 if t < 0.004 else 0)
    pok.append((math.sin(2 * math.pi * 900 * t) + nz) * math.exp(-t * 120))
samples.append(('pok', encode(pok, 0.9, 28), POK_RATE))

out = ['; Fichier généré par tools/gen_voice.py — ne pas modifier.',
       '; Échantillons DPCM (voix de l\'arbitre, « pok »), banque fixe.',
       '.export _dpcm_addr, _dpcm_len, _dpcm_rate', '',
       '.segment "DPCM"']
total = 0
for name, data, rate in samples:
    out.append('.align 64')
    out.append('dpcm_%s:' % name)
    for i in range(0, len(data), 16):
        out.append('    .byte ' + ','.join('$%02X' % v for v in data[i:i + 16]))
    total += len(data) + (-len(data)) % 64
out += ['', '.segment "RODATA"',
        '; $4012 = (adresse - $C000) / 64, $4013 = (longueur - 1) / 16, $4010 = fréquence',
        '_dpcm_addr: .byte ' + ', '.join('<((dpcm_%s - $C000) >> 6)' % n for n, _, _ in samples),
        '_dpcm_len:  .byte ' + ', '.join('%d' % ((len(d) - 1) // 16) for _, d, _ in samples),
        '_dpcm_rate: .byte ' + ', '.join('%d' % r for _, _, r in samples)]
open(os.path.join(ROOT, 'src', 'dpcm.s'), 'w').write('\n'.join(out) + '\n')
for name, data, rate in samples:
    print('%-6s %4d octets, %.2f s' % (name, len(data), len(data) * 8 / RATES[rate]))
print('total (avec alignement) : %d octets' % total)
