#!/usr/bin/env python3
"""Occupation de chaque banque de la ROM et de la RAM, à partir du fichier .map."""
import re, sys
mp, rom = sys.argv[1], sys.argv[2]
txt = open(mp).read()
m = re.search(r"Segment list:.*?-{20,}\n(.*?)\n\n", txt, re.S)
seg = {}
for line in (m.group(1).splitlines() if m else []):
    p = line.split()
    if len(p) >= 4 and p[0] != "Name":
        seg[p[0]] = int(p[3], 16)
fixed = sum(seg.get(s, 0) for s in ("STARTUP", "LOWCODE", "ONCE", "CODE", "RODATA", "DATA"))
parts = ["fixe %d" % fixed] + ["B%d %d" % (i, seg.get("BANK%d" % i, 0) + seg.get("RODATA%d" % i, 0)) for i in range(7) if seg.get("BANK%d" % i)]
print("ROM %s : octets par banque (max 16368) : %s" % (rom, ", ".join(parts)))
print("RAM %d/768, RAM cartouche %d/6144 + sauvegarde %d/2048, ZP %d/240" % (
    seg.get("DATA", 0) + seg.get("BSS", 0), seg.get("WBSS", 0), seg.get("SAVEDATA", 0), seg.get("ZEROPAGE", 0)))
