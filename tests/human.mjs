// Joueur humain simulé : utilise uniquement la manette 1.
import { Emu } from './emu.mjs';
const e = new Emu(process.argv[2]);
const N = +(process.argv[3] ?? 20000);
e.run(30); e.tap('START'); e.run(10); e.poke('opt_val', +(process.argv[4] ?? 0), 5); e.tap('START'); e.run(5);
const P = (n, i = 0) => e.peekS16(n, i * 2) / 64;
let returns = 0, swings = 0, serves = 0, lastRh = 0, aHeld = 0;
for (let i = 0; i < N && e.peek('app_state') === 2; i++) {
  e.releaseAll();
  const ms = e.peek('ms'), srv = e.peek('sc_server');
  const swap = e.peek('sc_swap');
  if ((ms === 0 || ms === 1) && srv === 0) {
    // service : lancer, puis frappe au sommet
    if (ms === 0 && i % 30 === 0) { e.down('A'); serves++; }
    if (ms === 1 && e.peekS16('bvz') <= 0) e.down('A');
  } else if (ms === 2 && e.peek('hitter') === 1) {
    const bx = P('bx'), by = P('by'), px = P('plx'), py = P('ply');
    const b = e.peek('bounces');
    const tx = b === 0 ? e.peekS16('land_x') + e.peekS16('bvx') * 12 / 64 : bx + e.peekS16('bvx') * 6 / 64;
    const ty = b === 0 ? e.peekS16('land_y') + e.peekS16('bvy') * 12 / 64 : by + e.peekS16('bvy') * 6 / 64;
    const gx = tx + (px < tx ? -10 : 10);
    if (gx < px - 2) e.down('LEFT'); else if (gx > px + 2) e.down('RIGHT');
    if (ty < py - 2) e.down('UP'); else if (ty > py + 2) e.down('DOWN');
    // armer quand la balle approche, frapper en la laissant entrer dans la zone
    if (Math.abs(by - py) < 40 && b >= 1) { e.down('A'); aHeld++; } else aHeld = 0;
  }
  e.run(1);
  const rh = e.peek('rally_hits');
  if (rh > lastRh && e.peek('hitter') === 0 && rh > 1) returns++;
  lastRh = rh;
}
console.log(`état=${e.peek('app_state')} trames=${e.frames} retours humains=${returns} services=${serves} points=${e.peek('st_points',0)}-${e.peek('st_points',1)} jeux=${e.peek('sc_games',0)}-${e.peek('sc_games',1)} sets=${e.peek('sc_sets',0)}-${e.peek('sc_sets',1)}`);
