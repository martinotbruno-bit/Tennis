#!/usr/bin/env python3
"""Royal Martinot — vérification sur un second émulateur, FCEUX (version 2.69).

jsnes, qui fait tourner la suite de tests, est approximatif sur les effets en
cours de ligne (le décalage fin de l'affiche VS, par exemple). FCEUX, avec son
nouveau moteur PPU (--newppu 1, au point près), est bien plus proche de la
console. Ce script le pilote par un programme Lua (sans écran : xvfb-run) :

  1. affiche VS : pendant la glissade, le buste gauche sur les lignes paires
     seulement, le buste droit sur les impaires seulement ;
  2. match entre ordinateurs, à plat puis en perspective : un ralenti avec son
     gros plan (hl_mode 1) puis son zoom (hl_mode 2), et le retour au court.

Usage : python3 tools/fceux_check.py [build/royal_martinot.nes] [dossier des images]
Il faut fceux (paquet « fceux ») et xvfb-run (paquet « xvfb »).
"""
import os, subprocess, sys, shutil
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
rom = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'build', 'royal_martinot.nes')
out = os.path.abspath(sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, 'build', 'fceux'))
os.makedirs(out, exist_ok=True)
fceux = shutil.which('fceux') or '/usr/games/fceux'
lbl = {}
for l in open(rom.replace('.nes', '.lbl')):
    p = l.split()
    if len(p) >= 3:
        lbl[p[2].lstrip('.').lstrip('_')] = int(p[1], 16)
A = {k: lbl[k] for k in ('opt_val', 'cfg', 'scr_state', 'app_state', 'hl_mode', 'hl_t', 'split_o')}

def lua(view):
    return f'''
emu.speedmode("maximum")
local OPT={A['opt_val']} local CFG={A['cfg']} local SCR={A['scr_state']} local APP={A['app_state']}
local HLM={A['hl_mode']} local HLT={A['hl_t']} local SPO={A['split_o']}
local function run(n, pad) for i=1,n do if pad then joypad.set(1,pad) end emu.frameadvance() end end
local function rb(a) return memory.readbyte(a) end
local function shot(n) gui.savescreenshotas("{out}/"..n..".png") end
local log = io.open("{out}/v{view}.txt", "w")
run(40) run(2,{{start=true}}) run(4) run(2,{{start=true}}) run(40)
for i=1,60 do if rb(SCR)==0 then break end run(1) end
run(2,{{start=true}}) for i=1,90 do if rb(SCR)==1 then break end run(1) end run(10)
memory.writebyte(OPT+9,3) memory.writebyte(OPT+24,{view})
run(2,{{start=true}})
local k=0
for i=1,300 do
  run(1)
  local o = rb(SPO)
  if o>=24 and o<=72 and k<3 then shot(string.format("v{view}_vs_%d", k)) k=k+1 end
  if k>=3 then break end
end
run(60)
run(2,{{A=true}})
for i=1,400 do if rb(APP)==1 then break end run(1) end
for p=0,3 do memory.writebyte(CFG+2+p,2) end
local seq="" local last=0 local back=0
for i=1,60*60*8 do
  run(1)
  local m=rb(HLM)
  if m~=last then
    if m>0 then seq=seq..m end
    if m==2 and last==0 then end
    if m==0 and last>0 then back=back+1 run(6) shot("v{view}_back_"..back) end
    last=m
  end
  if m==2 and rb(HLT)==40 then shot("v{view}_zoom") end
  if m==1 and rb(HLT)==40 then shot("v{view}_gros_plan") end
  if back>=2 then break end
end
log:write(seq) log:close()
emu.exit()
'''

ok = True
for view in (0, 1):
    path = os.path.join(out, 'check_%d.lua' % view)
    open(path, 'w').write(lua(view))
    subprocess.run(['xvfb-run', '-a', fceux, '--newppu', '1', '--sound', '0', '--loadlua', path, rom],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=900)
    seq = open(os.path.join(out, 'v%d.txt' % view)).read()
    good = seq.startswith('12')
    print('%s : ralenti %s (gros plan puis zoom attendus : 1, 2)' % ('à plat' if view == 0 else 'perspective', seq), 'ok' if good else 'ÉCHEC')
    ok &= good
    if view == 0:
        # glissade : buste gauche (x < 100) seulement sur les lignes paires de
        # la bande, buste droit (x >= 156) seulement sur les impaires ; FCEUX
        # rogne 8 lignes en haut (image de 224 lignes)
        for k in range(3):
            im = Image.open(os.path.join(out, 'v0_vs_%d.png' % k)).convert('RGB')
            bg = im.getpixel((128, 100))
            ev = od = evr = odr = 0
            for y in range(32, 144):
                yy = y - 8
                L = sum(1 for x in range(8, 100) if im.getpixel((x, yy)) != bg)
                R = sum(1 for x in range(156, 248) if im.getpixel((x, yy)) != bg)
                if y % 2 == 0: ev += L; evr += R
                else: od += L; odr += R
            g = ev > 100 and od == 0 and odr > 100 and evr == 0
            print('affiche VS, image %d : gauche paires %d / impaires %d ; droite impaires %d / paires %d' % (k, ev, od, odr, evr), 'ok' if g else 'ÉCHEC')
            ok &= g
print('FCEUX :', 'tout est conforme' if ok else 'des écarts ; voir les images dans ' + out)
sys.exit(0 if ok else 1)
