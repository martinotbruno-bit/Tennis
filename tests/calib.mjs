// Calibrage de la difficulté : un joueur robot « humain » (réaction retardée,
// placement imprécis, frappes variées) affronte l'IA d'un niveau donné.
// Usage : node calib.mjs rom.nes niveau [graine] [trames]
import { Emu } from './emu.mjs';
const rom = process.argv[2], lv = +process.argv[3], seed0 = +(process.argv[4] ?? 1), N = +(process.argv[5] ?? 60 * 60 * 12);
let rs = seed0 * 7919 + 13;
const rnd = () => { rs = (rs * 1103515245 + 12345) & 0x7fffffff; return rs / 0x7fffffff; };
// générique passé (deux appuis : « ARE YOU SURE? », puis « FINE. »)
const e = new Emu(rom); e.run(40); e.tap('START', 1, 1, 0); e.run(4); e.tap('START', 1, 1, 0); e.run(40 + seed0 * 7);
// SURF=n : surface du court (0 terre battue par défaut)
e.tap('START'); e.run(10); e.poke('opt_val', lv, 5); e.poke('opt_val', 0, 10); e.poke('opt_val', +(process.env.SURF ?? 0), 6);
e.tap('START'); e.run(20); e.tap('A');
for (let k = 0; k < 300 && e.peek('app_state') !== 1; k++) e.run(1);
const P = (n, i = 0) => e.peekS16(n, i * 2) / 64;
let lastRh = -1, wait = 0, errX = 0, errY = 0, aim = [], charge = 0, holding = false;
for (let f = 0; f < N; f++) {
  e.releaseAll();
  const ms = e.peek('ms');
  if (e.peek('sc_server') === 0 && e.peek('sc_swap') === 0 || e.peek('sc_server') === 0) {
    if (ms === 0 && rnd() < 0.05) { e.down('A'); e.run(1); continue; }
    if (ms === 1) { if (e.peekS16('bvz') <= 8 && rnd() < 0.5) e.down(rnd() < 0.7 ? 'A' : 'B'); e.run(1); continue; }
  }
  const rh = e.peek('rally_hits');
  if (rh !== lastRh) {               // nouvelle frappe : nouveau temps de réaction et nouvelle erreur
    lastRh = rh;
    wait = 9 + Math.floor(rnd() * 8);
    errX = (rnd() - 0.5) * 20; errY = (rnd() - 0.5) * 16;
    aim = rnd() < 0.35 ? [] : [rnd() < 0.5 ? 'LEFT' : 'RIGHT'];
    charge = Math.floor(rnd() * 20); holding = false;
  }
  if (ms === 2 && e.peek('hitter') === 1) {
    if (wait > 0) { wait--; e.run(1); continue; }
    const side = e.peek('sc_swap');   // 0 : équipe 1 en bas
    const px = P('plx'), py = P('ply'), b = e.peek('bounces');
    const dy = Math.abs(P('by') - py);
    // l'erreur de placement se réduit à mesure que la balle approche (correction visuelle)
    const k = Math.min(1, Math.max(0.25, (dy - 20) / 80));
    const tx = (b === 0 ? e.peekS16('land_x') + e.peekS16('bvx') * 12 / 64 : P('bx') + e.peekS16('bvx') * 6 / 64) + errX * k;
    const ty = (b === 0 ? e.peekS16('land_y') + e.peekS16('bvy') * 12 / 64 : P('by') + e.peekS16('bvy') * 6 / 64) + errY * k;
    const gx = tx + (px < tx ? -10 : 10);
    if (dy > 30) {
      if (gx < px - 3) e.down('LEFT'); else if (gx > px + 3) e.down('RIGHT');
      if (ty < py - 3) e.down('UP'); else if (ty > py + 3) e.down('DOWN');
    } else if (b >= 1 || dy < 14) {
      if (gx < px - 3) e.down('LEFT'); else if (gx > px + 3) e.down('RIGHT');
      e.down('A'); if (dy < 12) { e.releaseAll(); e.down('A'); for (const a of aim) e.down(a); }
    }
  }
  e.run(1);
  if (e.peek('app_state') !== 1) break;
}
const w = e.peek('st_points', 0), l = e.peek('st_points', 1);
console.log(JSON.stringify({ lv, seed: seed0, pts: [w, l], pct: +(100 * w / Math.max(1, w + l)).toFixed(1), jeux: [e.peek('sc_games', 0), e.peek('sc_games', 1)], sets: [e.peek('sc_sets', 0), e.peek('sc_sets', 1)] }));
