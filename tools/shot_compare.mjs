// Royal Martinot — banc de comparaison des coups (version 2.72) : même situation,
// même manette, même graine du hasard ; la balle juste après la frappe.
// Usage : node tools/shot_compare.mjs rom.nes   (comparer la sortie de deux ROM)
import { Emu } from '../tests/emu.mjs';
const rom = process.argv[2];
const S16 = (e, n, v, i = 0) => { e.poke(n, v & 255, i * 2); e.poke(n, (v >> 8) & 255, i * 2 + 1); };
function start() {
  const e = new Emu(rom);
  e.run(40); e.tap('START', 1, 1, 0); e.run(4); e.tap('START', 1, 1, 0); e.run(40);
  for (let i = 0; i < 60 && e.peek('scr_state') !== 0; i++) e.run(1);
  e.tap('START'); for (let n = 0; n < 60 && e.peek('scr_state') !== 1; n++) e.run(1); e.run(10);
  e.poke('opt_val', 3, 9); e.poke('opt_val', 0, 10);
  e.tap('START'); for (let i = 0; i < 120 && e.peek('scr_state') !== 3; i++) e.run(1); e.run(60); e.tap('A');
  for (let i = 0; i < 300 && e.peek('app_state') !== 1; i++) e.run(1);
  for (let i = 0; i < 2000 && (e.peek('ms') !== 0 || e.peek('cer')); i++) { if (e.peek('cer')) e.poke('cer_skip', 1); e.run(1); }
  e.run(5);
  const c = e.addr('cfg'); e.nes.cpu.mem[c + 3] = 2;   // adversaire : manette 2, immobile (pas de hasard consommé par l'IA)
  return e;
}
const shots = [
  // nom, boutons, appui (trames), rebonds, dx, distance, hauteur, vy, vz
  ['lifte A', ['A'], 10, 1, 10, 50, 14, 192, 0],
  ['a plat A+HAUT', ['A', 'UP'], 10, 1, 10, 50, 14, 192, 0],
  ['croise court A+BAS', ['A', 'DOWN'], 10, 1, 10, 50, 14, 192, 0],
  ['coupe B', ['B'], 10, 1, 10, 50, 14, 192, 0],
  ['lob B+HAUT', ['B', 'UP'], 10, 1, 10, 50, 14, 192, 0],
  ['amortie B+BAS', ['B', 'DOWN'], 10, 1, 10, 50, 14, 192, 0],
  ['long de ligne A+GAUCHE', ['A', 'LEFT'], 10, 1, 10, 50, 14, 192, 0],
  ['revers A (balle a gauche)', ['A'], 10, 1, -10, 50, 14, 192, 0],
  ['revers coupe B', ['B'], 10, 1, -10, 50, 14, 192, 0],
  ['puissant A long', ['A'], 18, 1, 10, 76, 14, 192, 0],
  ['volee A', ['A'], 6, 0, 8, 40, 22, 192, -8],
  ['volee B', ['B'], 6, 0, 8, 40, 22, 192, -8],
  ['smash A', ['A'], 6, 0, 4, 40, 34, 160, -4],
  ['smash saute A', ['A'], 6, 0, 4, 40, 46, 160, -4],
  ['extension A (balle loin)', ['A'], 10, 1, 18, 50, 14, 192, 0],
];
const out = {};
for (const [name, btn, hold, bounces, dx, dist, h, vy, vz] of shots) {
  const e = start();
  const px = 128 * 64, py = 196 * 64; S16(e, 'plx', px); S16(e, 'ply', py); S16(e, 'plx', 128 * 64, 1); S16(e, 'ply', 30 * 64, 1);
  e.poke('ms', 2); e.poke('b_live', 1); e.poke('b_vis', 1); e.poke('b_serve', 0); e.poke('bounces', bounces); e.poke('hitter', 1);
  e.poke('bspin', 0); e.poke('bgrav', 9); e.poke('pl_state', 0, 0); e.poke('rally_hits', 3);
  S16(e, 'bx', px + dx * 64); S16(e, 'by', py - dist * 64); S16(e, 'bz', h * 64);
  S16(e, 'bvx', 0); S16(e, 'bvy', vy); S16(e, 'bvz', vz);
  e.poke('rand_seed', 0x34, 0); e.poke('rand_seed', 0x12, 1);
  e.down(btn[0]);
  let f = 0;
  for (; f < 80 && e.peek('hitter') !== 0; f++) { if (f === hold - 1) for (const b of btn.slice(1)) e.down(b); if (f === hold) e.up(btn[0]); e.run(1); }
  for (const b of btn) e.up(b);
  const r = f >= 80 ? 'pas de frappe' :
    ['bx', 'by', 'bz', 'bvx', 'bvy', 'bvz', 'land_x', 'land_y'].map(n => n + '=' + e.peekS16(n)).join(' ') +
    ` spin=${e.peek('bspin')} grav=${e.peek('bgrav')} coup=${e.peek('last_shot')} trame=${f}`;
  out[name] = r;
  console.log(name.padEnd(28), r);
}

// services : A, B, canon (A pile au sommet), vers l'extérieur
for (const [name, btn, dir, top] of [['service A', 'A', null, 2], ['service B', 'B', null, 2], ['service canon', 'A', null, 0], ['service canon exterieur', 'A', 'RIGHT', 0]]) {
  const e = start();
  e.poke('sc_server', 0); e.poke('ms', 0); e.poke('ms_timer', 0);
  S16(e, 'plx', 140 * 64); S16(e, 'ply', 206 * 64);
  e.run(2);
  e.down('A'); e.run(1); e.up('A');
  for (let i = 0; i < 40 && e.peek('ms') !== 1; i++) e.run(1);
  e.poke('rand_seed', 0x34, 0); e.poke('rand_seed', 0x12, 1);
  let f = 0;
  for (; f < 60 && e.peek('ms') === 1; f++) {
    const v = e.peekS16('bvz');
    if ((top === 0 && v < 8 && v > -8) || (top === 2 && v < 20 && v >= 8)) { e.down(btn); if (dir) e.down(dir); }
    e.run(1); e.up(btn); if (dir) e.up(dir);
  }
  const r = ['bx', 'by', 'bz', 'bvx', 'bvy', 'bvz', 'land_x', 'land_y'].map(n => n + '=' + e.peekS16(n)).join(' ') +
    ` spin=${e.peek('bspin')} grav=${e.peek('bgrav')} canon=${e.peek('srv_canon')}`;
  console.log(name.padEnd(28), r);
}
