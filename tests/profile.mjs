// Profileur : cycles CPU par fonction (à partir du fichier .lbl).
// Usage : node profile.mjs rom.nes [double=0|1] [trames]
import { Emu } from './emu.mjs';
import fs from 'fs';
const rom = process.argv[2], dbl = +(process.argv[3] ?? 0), N = +(process.argv[4] ?? 6000);
// symboles : fichier .dbg (contient aussi les fonctions statiques)
const lbl = [];
for (const l of fs.readFileSync(rom.replace(/\.nes$/, '.dbg'), 'utf8').split('\n')) {
  const m = l.match(/^sym\t.*name="([^"]+)".*val=0x([0-9A-F]+).*type=lab/);
  if (m && !/^L[0-9A-F]{4}$/.test(m[1]) && !m[1].includes('@')) {
    const v = parseInt(m[2], 16); if (v >= 0x8000) lbl.push([v, m[1]]);
  }
}
for (const l of fs.readFileSync(rom.replace(/\.nes$/, '.lbl'), 'utf8').split('\n')) {
  const m = l.match(/^al ([0-9A-F]+) \.(\S+)/);
  if (m && !m[2].startsWith('__') && !m[2].includes('@') && !/^L[0-9A-F]{4}$/.test(m[2]) && parseInt(m[1], 16) >= 0x8000) lbl.push([parseInt(m[1], 16), m[2]]);
}
lbl.sort((a, b) => a[0] - b[0]);
const starts = lbl.map(a => a[0]);
function sym(pc) { let lo = 0, hi = starts.length - 1, r = -1; while (lo <= hi) { const m = (lo + hi) >> 1; if (starts[m] <= pc) { r = m; lo = m + 1; } else hi = m - 1; } return r < 0 ? '?' : lbl[r][1]; }
const e = new Emu(rom);
e.run(30); e.tap('START'); e.run(10); e.poke('opt_val', dbl, 2); e.poke('opt_val', 3, 8); e.tap('START'); e.run(5);
const c = e.addr('cfg'); for (let i = 0; i < 4; i++) e.nes.cpu.mem[c + 2 + i] = 2;
const cpu = e.nes.cpu, orig = cpu.emulate.bind(cpu), acc = {};
let total = 0;
const overByMs = {}; let dumped = 0;
let sinceWait = 0, inWait = false; const logicCost = []; let cur = {}; const spike = {};
let wfStart = 0;
cpu.emulate = function () {
  const pc = cpu.REG_PC + 1; const s = sym(pc); const n = orig();
  acc[s] = (acc[s] || 0) + n; total += n;
  if (s === '_wait_frame' || s === 'nmi' || s === 'irq') { if (!inWait && s === '_wait_frame') { logicCost.push(sinceWait); if (sinceWait > 27000) { const m = e.peek('ms'); overByMs[m] = (overByMs[m] || 0) + 1; } if (sinceWait > 27000 && e.peek('ms') === 2 && (dumped++ < +(process.env.DUMP || 0))) console.log('TRAME', e.frames, sinceWait, Object.entries(cur).sort((a,b)=>b[1]-a[1]).slice(0,12).map(([k,v])=>k+':'+v).join(' '));
    if (sinceWait > 27000 && e.peek('ms') === 2) for (const k in cur) spike[k] = (spike[k] || 0) + cur[k]; cur = {}; sinceWait = 0; inWait = true; } }
  else { inWait = false; sinceWait += n; cur[s] = (cur[s] || 0) + n; }
  return n;
};
let late = 0;
const wf = lbl.find(a => a[1] === '_wait_frame')[0];
for (let i = 0; i < N; i++) { e.run(1); const pc = cpu.REG_PC + 1; if (pc < wf || pc > wf + 16) late++; }
const wait = acc['_wait_frame'] || 0;
logicCost.shift();
const over = logicCost.filter(c => c > 27000).length;
const sorted = [...logicCost].sort((a, b) => a - b);
console.log('dépassements par état du match (0 service, 1 lancer, 2 échange, 3 point joué) :', JSON.stringify(overByMs));
console.log(`logique par trame : médiane ${sorted[sorted.length >> 1]}, max ${sorted[sorted.length - 1]} cycles ; trames > 27000 cycles : ${over}/${logicCost.length}`);
console.log(`trames ${N}, charge CPU hors attente : ${(100 * (total - wait) / total).toFixed(1)} %`);
Object.entries(acc).sort((a, b) => b[1] - a[1]).slice(0, 25).forEach(([k, v]) => console.log(k.padEnd(24), (v / N).toFixed(0).padStart(7), 'cycles/trame'));
const ns = logicCost.filter(c => c > 27000).length || 1;
console.log('--- trames en dépassement : cycles moyens par fonction ---');
Object.entries(spike).sort((a, b) => b[1] - a[1]).slice(0, 22).forEach(([k, v]) => console.log(k.padEnd(24), (v / ns).toFixed(0).padStart(7)));
