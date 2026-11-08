import { Emu } from './emu.mjs';
const e = new Emu(process.argv[2]);
const lv = +(process.argv[3] ?? 1), surf = +(process.argv[4] ?? 0);
e.run(30); e.tap('START'); e.run(10);
// niveau / surface via opt_val
e.poke('opt_val', lv, 5); e.poke('opt_val', surf, 6); e.poke('opt_val', +(process.argv[5] ?? 0), 2);
e.tap('START'); e.run(5);
// CPU des deux côtés
const cfg = e.addr('cfg');
e.nes.cpu.mem[cfg + 2] = 2; e.nes.cpu.mem[cfg + 10] = lv;
let lastPts = [0,0], points = 0, rallies = [], f0 = e.frames;
const ms = () => e.nes.cpu.mem[e.addr('ms')];
let stuck = 0, lastMs = -1, maxSame = 0;
while (e.peek('app_state') === 2 && e.frames - f0 < 60*60*40) {
  e.run(1);
  const m = ms();
  if (m === lastMs) { stuck++; maxSame = Math.max(maxSame, stuck); } else { stuck = 0; lastMs = m; }
  const p = [e.peek('st_points',0), e.peek('st_points',1)];
  if (p[0] !== lastPts[0] || p[1] !== lastPts[1]) { points++; rallies.push(e.peek('rally_hits')); lastPts = p; }
}
const hist = [0,1,2].map(i => [e.peek('sc_hist', i*2), e.peek('sc_hist', i*2+1)]);
const hits = rallies.reduce((a,b)=>a+b,0)/rallies.length;
console.log(`état=${e.peek('app_state')} durée=${((e.frames-f0)/60/60).toFixed(1)}min points=${points} vainqueur=${e.peek('sc_winner')} sets=${JSON.stringify(hist)}`);
console.log(`frappes moy/point=${hits.toFixed(2)} max=${Math.max(...rallies)} aces=${e.peek('st_aces',0)}/${e.peek('st_aces',1)} DF=${e.peek('st_dfaults',0)}/${e.peek('st_dfaults',1)} gagnants=${e.peek('st_winners',0)}/${e.peek('st_winners',1)} fautes=${e.peek('st_errors',0)}/${e.peek('st_errors',1)} 1eres=${e.peek('st_first_in',0)}/${e.peek('st_first_tot',0)} maxEtatFige=${maxSame}`);
const dist = {}; rallies.forEach(r => dist[r] = (dist[r]||0)+1); console.log(JSON.stringify(dist));
