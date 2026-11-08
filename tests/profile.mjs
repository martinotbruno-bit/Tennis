// Profileur : cycles CPU par fonction, en tenant compte des banques MMC1.
// Usage : node profile.mjs rom.nes [double=0|1] [trames]
import { Emu } from './emu.mjs';
import fs from 'fs';
const rom = process.argv[2], dbl = +(process.argv[3] ?? 0), N = +(process.argv[4] ?? 6000);
const dbg = fs.readFileSync(rom.replace(/\.nes$/, '.dbg'), 'utf8').split('\n');
const segBank = {};
for (const l of dbg) {
  const m = l.match(/^seg\tid=(\d+),name="([^"]+)"/);
  if (m) { const b = m[2].match(/^(?:BANK|RODATA)(\d)$/); segBank[m[1]] = b ? +b[1] : -1; }
}
const tables = {};   // banque -> [adresse, nom]
for (const l of dbg) {
  const m = l.match(/^sym\t.*name="([^"]+)".*val=0x([0-9A-F]+).*seg=(\d+).*type=lab/);
  if (!m || /^L[0-9A-F]{4}$/.test(m[1]) || m[1].includes('@')) continue;
  const v = parseInt(m[2], 16); if (v < 0x8000) continue;
  const b = v >= 0xC000 ? 'F' : segBank[m[3]];
  (tables[b] ??= []).push([v, m[1]]);
}
for (const l of fs.readFileSync(rom.replace(/\.nes$/, '.lbl'), 'utf8').split('\n')) {
  const m = l.match(/^al ([0-9A-F]+) \.(\S+)/);
  if (m && parseInt(m[1], 16) >= 0xC000 && !m[2].startsWith('__') && !/^L[0-9A-F]{4}$/.test(m[2]) && !m[2].includes('@'))
    (tables.F ??= []).push([parseInt(m[1], 16), m[2]]);
}
for (const k in tables) tables[k].sort((a, b) => a[0] - b[0]);
function sym(pc, bank) {
  const t = tables[pc >= 0xC000 ? 'F' : bank]; if (!t) return '?';
  let lo = 0, hi = t.length - 1, r = -1;
  while (lo <= hi) { const m = (lo + hi) >> 1; if (t[m][0] <= pc) { r = m; lo = m + 1; } else hi = m - 1; }
  return r < 0 ? '?' : t[r][1];
}
const e = new Emu(rom);
e.run(30); e.tap('START'); e.run(10); e.poke('opt_val', dbl, 2); e.poke('opt_val', 3, 9); e.tap('START'); e.run(10); e.tap('A'); e.run(10);
const c = e.addr('cfg'); for (let i = 0; i < 4; i++) e.nes.cpu.mem[c + 2 + i] = 2;
if (process.env.NOREPLAY) e.nes.cpu.mem[c + 19] = 0;
const cpu = e.nes.cpu, orig = cpu.emulate.bind(cpu), acc = {};
const cb = e.addr('cur_bank');
let total = 0, sinceWait = 0, inWait = false; const logicCost = [], overByMs = {}; let cur = {}; const spike = {};
cpu.emulate = function () {
  const pc = cpu.REG_PC + 1; const s = sym(pc, cpu.mem[cb]); const n = orig();
  acc[s] = (acc[s] || 0) + n; total += n;
  if (s === '_wait_frame' || s === 'nmi' || s === 'irq') {
    if (!inWait && s === '_wait_frame') {
      logicCost.push(sinceWait);
      if (sinceWait > 27000) { const m = e.peek('ms'); overByMs[m] = (overByMs[m] || 0) + 1; if (m === 2) for (const k in cur) spike[k] = (spike[k] || 0) + cur[k]; }
      cur = {}; sinceWait = 0; inWait = true;
    }
  } else { inWait = false; sinceWait += n; cur[s] = (cur[s] || 0) + n; }
  return n;
};
for (let i = 0; i < N; i++) e.run(1);
logicCost.shift();
const sorted = [...logicCost].sort((a, b) => a - b);
const wait = acc['_wait_frame'] || 0;
console.log('dépassements par état du match (0 service, 1 lancer, 2 échange, 3 point joué, 5 ralenti) :', JSON.stringify(overByMs));
console.log(`logique par trame : médiane ${sorted[sorted.length >> 1]}, max ${sorted[sorted.length - 1]} cycles ; trames > 27000 cycles : ${logicCost.filter(c => c > 27000).length}/${logicCost.length}`);
console.log(`charge CPU hors attente : ${(100 * (total - wait) / total).toFixed(1)} %`);
Object.entries(acc).sort((a, b) => b[1] - a[1]).slice(0, 18).forEach(([k, v]) => console.log(k.padEnd(24), (v / N).toFixed(0).padStart(7), 'cycles/trame'));
const ns = Object.values(overByMs).reduce((a, b) => a + b, 0) || 1;
console.log('--- trames d\'échange en dépassement : cycles moyens par fonction ---');
Object.entries(spike).sort((a, b) => b[1] - a[1]).slice(0, 16).forEach(([k, v]) => console.log(k.padEnd(24), (v / (overByMs[2] || 1)).toFixed(0).padStart(7)));
