#!/usr/bin/env python3
"""Affiche l'occupation mémoire de la ROM et de la RAM à partir du fichier .map."""
import re, sys
mp, rom = sys.argv[1], sys.argv[2]
txt = open(mp).read()
m = re.search(r"Segment list:.*?-{20,}\n(.*?)\n\n", txt, re.S)
seg = {}
for line in (m.group(1).splitlines() if m else []):
    p = line.split()
    if len(p) >= 4 and p[0] != "Name":
        seg[p[0]] = int(p[3], 16)
prg = sum(seg.get(s, 0) for s in ("STARTUP", "LOWCODE", "ONCE", "CODE", "RODATA", "DATA"))
ram = sum(seg.get(s, 0) for s in ("DATA", "BSS"))
zp = seg.get("ZEROPAGE", 0)
print("ROM %s : PRG %d/32762 octets (%.0f%%), RAM %d/768 octets, ZP %d/240 octets" % (
    rom, prg, prg * 100 / 32762, ram, zp))
