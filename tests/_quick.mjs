// Royal Martinot — tests de validation sur émulateur (jsnes).
// Usage : node emu_test.mjs ../build/royal_martinot.nes
import { Emu } from './emu.mjs';

const rom = process.argv[2] ?? '../build/royal_martinot.nes';
let fails = 0, passes = 0;
function check(cond, msg) {
  if (cond) { passes++; console.log('  ok   ' + msg); }
  else { fails++; console.log('  ECHEC ' + msg); }
}
const AS = { SCREENS: 0, MATCH: 1, MANUAL: 2 };
const SS = { TITLE: 0, MENU: 1, RESULTS: 2, PRESENT: 3, COMP: 4, CODE: 5, PALMARES: 6, TRAIN_RES: 7, COMP_END: 8, SETBREAK: 9 };
const O = { MODE: 1, TYPE: 2, P1: 3, P2: 4, LEVEL: 5, SURF: 6, ENV: 7, COND: 8, FMT: 9, REPLAY: 10, COMP: 11,
            CSIZE: 12, CPROG: 13, CVAR: 14, EX: 15, SIDE: 16, SPEED: 17, SPIN: 18, COUNT: 19, CPART: 20, GSPEED: 21, SOUND: 22, FATIG: 23 };
const MODE = { P1: 0, P2: 1, TEAM: 2, COMP: 3, TRAIN: 4, DEMO: 5 };
const CFG = { CTRL: 2, LEVEL: 10, GAMES: 17, REPLAYS: 19 };

// démarrage : le générique de lancement est passé avec START, puis écran titre
function boot() { const e = new Emu(rom); e.run(40); e.tap('START', 1, 1, 0); e.run(4); e.tap('START', 1, 1, 0); e.run(40); return e; }
const scr = (e) => e.peek('scr_state');
const app = (e) => e.peek('app_state');
function toMenu(e) { e.tap('START'); e.run(10); }
// ligne du menu où se trouve une option (la liste dépend du mode)
function rowOf(e, o) { for (let i = 0; i < e.peek('nvis'); i++) if (e.peek('vis', i) === o) return i; return -1; }
function setOpts(e, opts) { for (const [k, v] of Object.entries(opts)) e.poke('opt_val', v, O[k]); }
// depuis le titre : configure et lance (présentation -> A si nécessaire)
function startWith(e, opts) {
  toMenu(e); setOpts(e, opts); e.tap('START');
  // certains écrans (classements) se construisent en plusieurs trames
  for (let n = 0; n < 90 && app(e) === AS.SCREENS && scr(e) === SS.MENU; n++) e.run(1);
  e.run(10);
  // le court se charge sur quelques trames (fondu, décor, copie des tribunes)
  if (app(e) === AS.SCREENS && scr(e) === SS.PRESENT) { e.tap('A'); runUntil(e, () => app(e) === AS.MATCH, 60); e.run(2); }
}
function allCpu(e) { const c = e.addr('cfg'); for (let i = 0; i < 4; i++) e.nes.cpu.mem[c + CFG.CTRL + i] = 2; }
function runUntil(e, pred, max, each) { let n = 0; while (!pred() && n < max) { if (each) each(); e.run(1); n++; } return n < max; }
let spChecks = 0;
function spOk(e) {
  const wf = e.addr('wait_frame'), pc = e.nes.cpu.REG_PC + 1;
  if (pc < wf || pc > wf + 16) return true;
  spChecks++;
  return e.peek16('sp') === 0x0800;
}
const P = (e, n, i = 0) => e.peekS16(n, i * 2) / 64;
// joueur robot de l'équipe 1 (manette 1) : se place à côté de la balle et frappe
function botStep(e, btns = ['A']) {
  e.releaseAll();
  const ms = e.peek('ms');
  if (e.peek('sc_server') === 0 && e.peek('cfg', 0) === 0) {
    // service du joueur : lancer puis frappe au sommet
    if (ms === 0 && (e.frames % 16) === 0) { e.down('A'); return; }
    if (ms === 1) { if (e.peekS16('bvz') <= 0) e.down('B'); return; }
  }
  if (ms !== 2 || e.peek('hitter') !== 1) return;
  // en match, le robot laisse filer les échanges trop longs (sinon matchs interminables)
  if (e.peek('cfg', 0) === 0 && e.peek('rally_hits') >= 9) return;
  const px = P(e, 'plx'), py = P(e, 'ply'), b = e.peek('bounces');
  const tx = b === 0 ? e.peekS16('land_x') + e.peekS16('bvx') * 12 / 64 : P(e, 'bx') + e.peekS16('bvx') * 6 / 64;
  const ty = b === 0 ? e.peekS16('land_y') + e.peekS16('bvy') * 12 / 64 : P(e, 'by') + e.peekS16('bvy') * 6 / 64;
  const gx = tx + (px < tx ? -10 : 10);
  const near = Math.abs(P(e, 'by') - py) < 40;
  if (!near) {
    if (gx < px - 2) e.down('LEFT'); else if (gx > px + 2) e.down('RIGHT');
    if (ty < py - 2) e.down('UP'); else if (ty > py + 2) e.down('DOWN');
  }
  if (near && (b >= 1 || e.peek('cfg', 21) === 4)) {
    // bouton d'abord ; la direction seulement au dernier moment (sinon on s'éloigne)
    e.down(btns[0]);
    if (btns[1] && Math.abs(P(e, 'by') - py) < 22) e.down(btns[1]);
  }
}

console.log('Plongeon');
{
  // Le plongeon part AVANT la frappe ; il se décide au lâcher du bouton :
  // joueur bien placé, frappe normale ; balle trop loin sur le côté,
  // plongeon, et la frappe part en vol (lob de sauvetage). Joueur humain
  // (bas) : la balle de l'ordinateur arrive à OFF px sur sa gauche ; il
  // appuie sur A puis le lâche à N trames de l'arrivée (direction : facultative).
  const humanDive = (view, OFF, N, dir) => {
    const e = boot(); toMenu(e); e.poke('opt_val', view, 24); setOpts(e, { FMT: 3, SURF: 1 }); e.tap('START'); e.run(10); e.tap('A');
    runUntil(e, () => app(e) === AS.MATCH, 300);
    const out = [];
    for (let trial = 0; trial < 3; trial++) {
      let ok = false;
      for (let f = 0; f < 3000 && !ok; f++) {
        if (e.peek('ms') === 0 && e.peek('sc_server') === 0) { e.tap('A'); e.run(20); e.tap('A'); }
        e.run(1);
        ok = e.peek('ms') === 2 && e.peek('hitter') === 1 && e.peek('b_live') === 1 && e.peekS16('by') / 64 < 150 && e.peekS16('bvy') > 0;
      }
      if (!ok) continue;
      const pred = () => { let x = e.peekS16('bx'), y = e.peekS16('by'); const vx = e.peekS16('bvx'), vy = e.peekS16('bvy'), py = e.peekS16('ply', 0);
        for (let n = 0; n < 60; n++) { const d = y - py; if (d > -14 * 64 && d < 9 * 64) return { n, x: x / 64 }; x += vx; y += vy; } return null; };
      let placed = false, pressed = false, r = { dove: false, hit: false, hitAt: 0, before: false, shot: -1, kneel: false, still: true, startX: 0 };
      const rh0 = e.peek('rally_hits');
      for (let f = 0; f < 150; f++) {
        const p = pred();
        if (p && !placed) { const v = (Math.round(p.x) - OFF) * 64; e.poke('plx', v & 255, 0); e.poke('plx', (v >> 8) & 255, 1); placed = true; }
        if (p && placed && !pressed && p.n <= N) { e.releaseAll(); e.down('A'); if (dir) e.down('RIGHT'); pressed = true; }
        else if (pressed) e.releaseAll();
        e.run(1);
        const t = e.peek('dive_t', 0);
        if (t > 0 && !r.dove) { r.dove = true; r.before = e.peek('rally_hits') === rh0; }
        if (r.dove && t === 33) r.startX = e.peekS16('plx', 0);
        if (r.dove && t < 33 && t >= 8 && e.peekS16('plx', 0) !== r.startX) r.still = false;
        if ((e.peek('pose_now', 0) & 31) === 27) r.kneel = true;
        if (!r.hit && e.peek('rally_hits') !== rh0 && e.peek('hitter') === 0) { r.hit = true; r.hitAt = t; r.shot = e.peek('last_shot'); }
        if (r.dove && t === 0) break;
        if (!r.dove && (r.hit || e.peek('ms') !== 2)) break;
      }
      e.releaseAll();
      out.push(r);
      for (let f = 0; f < 400 && e.peek('ms') === 2; f++) e.run(1);
      for (let f = 0; f < 300 && e.peek('ms') !== 0; f++) e.run(1);
    }
    return out;
  };
  for (const view of [0, 1]) {
    const v = view ? 'perspective' : 'à plat';
    const a = humanDive(view, 34, 7, 0);
    check(a.length >= 2 && a.every(r => r.dove && r.before && r.hit && r.hitAt >= 30 && r.shot === 2),
      `${v} : balle à 34 px, bouton lâché : plongeon avant la frappe, balle touchée en vol (lob) [${a.map(r => r.hit ? 't=' + r.hitAt : 'raté').join(', ')}]`);
    check(a.every(r => r.kneel && r.still), `${v} : au sol puis un genou à terre, immobile jusqu'au relevé`);
    if (view) continue;
    const b = humanDive(view, 12, 7, 0);
    check(b.length >= 2 && b.every(r => !r.dove && r.hit && r.shot !== 2), 'joueur bien placé (balle à 12 px) : pas de plongeon, frappe normale');
    const c = humanDive(view, 48, 7, 1);
    check(c.length >= 2 && c.every(r => !r.hit), 'balle à 48 px : hors de portée, même en plongeant');
  }
  // ordinateur contre ordinateur : il plonge, touche la balle, jamais sur une balle qui sort
  let dives = 0, touched = 0, outDive = 0;
  for (const surf of [0, 1, 5]) {
  const e = boot(); startWith(e, { FMT: 3, SURF: surf }); allCpu(e);
  for (let i = 0; i < 4; i++) e.poke('cfg', 2, CFG.LEVEL + i);
  let prev = [0, 0], rh = 0, cur = -1;
  for (let f = 0; f < 30000 && app(e) === AS.MATCH; f++) {
    e.run(1);
    for (let p = 0; p < 2; p++) {
      const t = e.peek('dive_t', p);
      if (t > prev[p]) {
        dives++; cur = p; rh = e.peek('rally_hits');
        const lx = e.peekS16('land_x'), ly = e.peekS16('land_y');
        if (e.peek('bounces') === 0 && e.peek('ms') === 2 && (lx < 74 || lx > 182 || ly < 48 || ly > 216)) outDive++;
      }
      if (p === cur && t >= 30 && e.peek('rally_hits') !== rh && e.peek('hitter') === (p & 1)) { touched++; cur = -1; }
      prev[p] = t;
    }
  }
  }
  check(dives >= 5 && touched * 4 >= dives * 3 && outDive === 0, `ordinateur : ${dives} plongeons, ${touched} balles touchées, ${outDive} sur une balle qui sort`);
}
console.log("fin", passes, fails);
