// Pixel Open — tests de validation sur émulateur (jsnes).
// Usage : node emu_test.mjs ../build/pixel_open.nes
import { Emu } from './emu.mjs';

const rom = process.argv[2] ?? '../build/pixel_open.nes';
let fails = 0, passes = 0;
function check(cond, msg) {
  if (cond) { passes++; console.log('  ok   ' + msg); }
  else { fails++; console.log('  ECHEC ' + msg); }
}
const AS = { TITLE: 0, MENU: 1, MATCH: 2, RESULTS: 3, TOURNEY: 4, CODE: 5 };
const OPT = { MODE: 1, TYPE: 2, P1: 3, P2: 4, LEVEL: 5, SURF: 6, COND: 7, FMT: 8 };

function boot() { const e = new Emu(rom); e.run(30); return e; }
function toMenu(e) { e.tap('START'); e.run(10); }
function startWith(e, opts) {
  toMenu(e);
  for (const [k, v] of Object.entries(opts)) e.poke('opt_val', v, OPT[k]);
  e.tap('START'); e.run(5);
}
function allCpu(e) { const c = e.addr('cfg'); for (let i = 0; i < 4; i++) e.nes.cpu.mem[c + 2 + i] = 2; }
function runUntil(e, pred, max) { let n = 0; while (!pred() && n < max) { e.run(1); n++; } return n < max; }
// La pile C n'est vérifiée que lorsque le CPU attend la trame suivante
// (ailleurs, une lecture peut tomber au milieu d'une mise à jour de 'sp').
let spChecks = 0;
function spOk(e) {
  const wf = e.addr('wait_frame'), pc = e.nes.cpu.REG_PC + 1;
  if (pc < wf || pc > wf + 16) return true;
  spChecks++;
  return e.peek16('sp') === 0x0800;
}

console.log('Démarrage');
{
  const e = boot();
  check(e.peek('app_state') === AS.TITLE, 'écran titre affiché');
  const r = e.peek16('region_cnt');
  check(r > 1700 && r < 1970, `détection NTSC (compteur ${r})`);
  check(e.peek('is_pal') === 0, 'mode 60 Hz sélectionné');
  const f = e.peek('frame_cnt'); e.run(10);
  check(((e.peek('frame_cnt') - f) & 255) === 10, 'NMI à chaque trame');
}

console.log('Menu');
{
  const e = boot(); toMenu(e);
  check(e.peek('app_state') === AS.MENU, 'START ouvre la configuration');
  e.tap('DOWN'); e.tap('RIGHT');
  check(e.peek('opt_val', OPT.MODE) === 1, 'DROITE modifie le mode');
  e.tap('LEFT'); e.tap('DOWN'); e.tap('RIGHT');
  check(e.peek('opt_val', OPT.TYPE) === 1, 'choix SIMPLE/DOUBLE');
  e.tap('B');
  check(e.peek('app_state') === AS.TITLE, 'B revient au titre');
}

console.log('Service humain');
{
  const e = boot(); startWith(e, {});
  // forcer le service du joueur 1
  runUntil(e, () => e.peek('ms') === 0, 200);
  e.poke('sc_server', 0);
  e.run(2);
  e.tap('A', 1, 2, 1);
  check(e.peek('ms') === 1, 'A lance la balle');
  check(runUntil(e, () => e.peekS16('bvz') <= 0, 60), 'la balle monte puis redescend');
  e.tap('A', 1, 2, 1);
  check(e.peek('ms') === 2 || e.peek('ms') === 3, 'A frappe le service');
  check(e.peek('rally_hits') === 1 && e.peek('hitter') === 0, 'frappeur et compteur de frappes corrects');
}

console.log('Pause et abandon');
{
  const e = boot(); startWith(e, {}); e.run(30);
  e.tap('START');
  check(e.peek('paused') === 1, 'START met en pause');
  const bx = e.peek16('plx'); e.run(30);
  check(e.peek16('plx') === bx, 'le jeu est figé en pause');
  e.tap('SELECT'); e.run(5);
  check(e.peek('app_state') === AS.MENU, 'SELECT en pause quitte vers le menu');
}

function fullMatch(label, opts, maxFrames) {
  const e = boot(); startWith(e, opts); allCpu(e);
  let lastMs = -1, same = 0, worst = 0, spGood = true;
  const ok = runUntil(e, () => {
    const m = e.peek('ms');
    if (m === lastMs) { same++; worst = Math.max(worst, same); } else { same = 0; lastMs = m; }
    if ((e.frames & 255) === 0 && !spOk(e)) spGood = false;
    return e.peek('app_state') === AS.RESULTS;
  }, maxFrames);
  const pts = e.peek('st_points', 0) + e.peek('st_points', 1);
  const w = e.peek('sc_winner');
  check(ok, `${label} : match terminé en ${(e.frames / 3600).toFixed(1)} min de jeu (${pts} points)`);
  check(e.peek('sc_over') === 1 && e.peek('sc_sets', w) >= 1, `${label} : vainqueur ${w} cohérent`);
  const g = [e.peek('sc_hist', 0), e.peek('sc_hist', 1)];
  check(Math.max(...g) >= e.peek('cfg', 17) && Math.abs(g[0] - g[1]) >= 1, `${label} : score du set valide (${g[0]}-${g[1]})`);
  check(worst < 4000, `${label} : aucun état bloqué (max ${worst} trames)`);
  check(spGood && spOk(e), `${label} : pile C intacte`);
  return e;
}

console.log('Matchs complets CPU contre CPU (format rapide)');
const surfaces = ['terre', 'gazon', 'dur', 'moquette'];
for (let s = 0; s < 4; s++) fullMatch(`simple ${surfaces[s]}`, { FMT: 3, SURF: s, LEVEL: s % 3, COND: s % 3 }, 60 * 60 * 25);
fullMatch('double dur', { FMT: 3, TYPE: 1, SURF: 2 }, 60 * 60 * 25);

console.log('Revanche et longue session');
{
  const e = fullMatch('revanche 1', { FMT: 3, LEVEL: 2 }, 60 * 60 * 25);
  e.tap('A'); e.run(5);
  check(e.peek('app_state') === AS.MATCH, 'A sur les résultats relance un match');
  allCpu(e);
  check(runUntil(e, () => e.peek('app_state') === AS.RESULTS, 60 * 60 * 25), 'second match enchaîné terminé');
  check(spOk(e), 'pile intacte après deux matchs');
  e.tap('B'); e.run(5);
  check(e.peek('app_state') === AS.MENU, 'B sur les résultats revient au menu');
}

console.log('Entraînement');
{
  const e = boot(); startWith(e, { MODE: 4 });
  check(e.peek('cfg', 0) === 1, 'mode lanceur de balles');
  // joueur robot : se place et frappe
  const P = (n, i = 0) => e.peekS16(n, i * 2) / 64;
  runUntil(e, () => {
    e.releaseAll();
    if (e.peek('ms') === 2 && e.peek('hitter') === 1) {
      const px = P('plx'), py = P('ply'), b = e.peek('bounces');
      const tx = b === 0 ? e.peekS16('land_x') + e.peekS16('bvx') * 12 / 64 : P('bx') + e.peekS16('bvx') * 6 / 64;
      const ty = b === 0 ? e.peekS16('land_y') + e.peekS16('bvy') * 12 / 64 : P('by') + e.peekS16('bvy') * 6 / 64;
      const gx = tx - 10;
      if (gx < px - 2) e.down('LEFT'); else if (gx > px + 2) e.down('RIGHT');
      if (ty < py - 2) e.down('UP'); else if (ty > py + 2) e.down('DOWN');
      if (Math.abs(P('by') - py) < 40 && b >= 1) e.down('A');
    }
    return e.peek('app_state') === AS.RESULTS;
  }, 60 * 60 * 6);
  e.releaseAll();
  check(e.peek('app_state') === AS.RESULTS, 'exercice terminé après 20 balles');
  check(e.peek('tr_score') >= 10, `balles renvoyées comptées (${e.peek('tr_score')} pts / ${e.peek('tr_total')})`);
}
{
  const e = boot(); startWith(e, { MODE: 5 });
  for (let i = 0; i < 6; i++) {
    runUntil(e, () => e.peek('ms') === 0, 400);
    e.tap('A', 1, 2, 1);
    runUntil(e, () => e.peekS16('bvz') <= 0, 60);
    e.tap('B', 1, 2, 1);
    runUntil(e, () => e.peek('ms') === 3, 300);
  }
  check(e.peek('tr_total') >= 5, `services comptés (${e.peek('tr_score')}/${e.peek('tr_total')})`);
}

console.log('Tournoi et code de reprise');
{
  const e = boot(); startWith(e, { MODE: 3, P1: 2, SURF: 1, COND: 2 });
  check(e.peek('app_state') === AS.TOURNEY, 'écran du tournoi');
  const v = e.nes.ppu.vramMem;
  let code = ''; for (let i = 0; i < 6; i++) code += String.fromCharCode(v[0x2000 + 19 * 32 + 12 + i]);
  check(/^[BCDFGHJKLMNPRSTV]{6}$/.test(code), `code affiché : ${code}`);
  // victoire simulée au premier tour
  e.tap('A'); e.run(5);
  check(e.peek('app_state') === AS.MATCH, 'A lance le quart de finale');
  allCpu(e);
  runUntil(e, () => e.peek('app_state') === AS.RESULTS, 60 * 60 * 40);
  e.poke('sc_winner', 0);
  e.tap('A'); e.run(5);
  check(e.peek('tour_round') === 1 && e.peek('app_state') === AS.TOURNEY, 'qualification pour la demi-finale');
  let code2 = ''; for (let i = 0; i < 6; i++) code2 += String.fromCharCode(e.nes.ppu.vramMem[0x2000 + 19 * 32 + 12 + i]);
  // nouvelle partie : saisie du code
  const f = boot(); f.tap('SELECT'); f.run(10);
  check(f.peek('app_state') === AS.CODE, 'SELECT ouvre la saisie du code');
  const AL = 'BCDFGHJKLMNPRSTV';
  for (let i = 0; i < 6; i++) {
    const n = AL.indexOf(code2[i]);
    for (let k = 0; k < n; k++) f.tap('UP', 1, 2, 2);
    if (i < 5) f.tap('RIGHT', 1, 2, 2);
  }
  f.tap('START'); f.run(10);
  check(f.peek('app_state') === AS.TOURNEY, `code ${code2} accepté`);
  check(f.peek('tour_round') === 1 && f.peek('opt_val', OPT.P1) === 2 && f.peek('opt_val', OPT.SURF) === 1 && f.peek('opt_val', OPT.COND) === 2,
    'progression restaurée (manche, joueur, surface, conditions)');
  const g = boot(); g.tap('SELECT'); g.run(10); g.tap('START'); g.run(10);
  check(g.peek('app_state') === AS.CODE, 'code invalide refusé');
}

console.log('Compensation 50 Hz (PAL)');
{
  const e = boot(); e.poke('is_pal', 1); startWith(e, { FMT: 3 }); allCpu(e);
  const l0 = e.frames; let ticks = 0, prev = e.peek('ms_timer');
  check(runUntil(e, () => e.peek('app_state') === AS.RESULTS, 60 * 60 * 25), 'match complet en mode 50 Hz');
  console.log(`    (${((e.frames - l0) / 3000).toFixed(1)} min à 50 Hz)`);
}

console.log(`\n${passes} vérifications réussies, ${fails} échec(s)`);
process.exit(fails ? 1 : 0);
