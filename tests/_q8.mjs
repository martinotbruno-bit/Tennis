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

console.log('Séquences animées du match');
{
  // entrée des joueurs (tunnel, salut, tirage au sort), changement de côté
  // (chaises), fin du match (à genoux, allongé, poignée de main, salut à
  // l'arbitre). Le banc d'essai les passe d'habitude (cerSkip) ; ici, non.
  const msg = (e) => { let t = ''; for (let i = 0; i < 14; i++) t += String.fromCharCode(e.peek('msg1', i)); return t.trim(); };
  const pos = (e, p) => [Math.round(e.peekS16('plx', p * 2) / 64), Math.round(e.peekS16('ply', p * 2) / 64)];
  const oamHas = (e, t) => { const o = e.addr('oam_buf'); for (let i = 0; i < 64; i++) if (e.nes.cpu.mem[o + i * 4] < 240 && e.nes.cpu.mem[o + i * 4 + 1] === t) return true; return false; };
  // tuile des accessoires : constante de la banque 13 (pas toujours visible
  // par le processeur), lue dans le fichier généré
  const SWAPC = (await import('fs')).readFileSync(new URL('../src/assets_swap.c', import.meta.url), 'utf8');
  const PROP = parseInt(SWAPC.match(/prop_tile13 = 0x([0-9A-F]+)/)[1], 16);      // gourde, serviette, raquette
  const CHAIR = parseInt(SWAPC.match(/chair_tile13 = 0x([0-9A-F]+)/)[1], 16);
  const prop = () => PROP;
  for (const view of [0, 1]) {
    const v = view ? 'perspective' : 'à plat';
    // la même partie, séquence passée : places attendues
    const r = boot(); toMenu(r); r.poke('opt_val', view, 24); setOpts(r, { FMT: 3, SURF: 1 }); r.tap('START'); r.run(10); r.tap('A');
    runUntil(r, () => app(r) === AS.MATCH, 300); r.run(5);
    const want = [pos(r, 0), pos(r, 1)];
    const e = boot(); e.cerSkip = false; toMenu(e); e.poke('opt_val', view, 24); setOpts(e, { FMT: 3, SURF: 1 }); e.tap('START'); e.run(10); e.tap('A');
    runUntil(e, () => app(e) === AS.MATCH, 300); e.run(2);
    const start = pos(e, 0);
    let wave = false, toss = '', coin = false, calls = '', upBack = false;
    const msg2 = () => { let t = ''; for (let i = 0; i < 14; i++) t += String.fromCharCode(e.peek('msg2', i)); return t.trim(); };
    for (let f = 0; f < 600 && e.peek('cer'); f++) {
      const y0 = e.peekS16('ply', 2);
      e.run(1);
      // le joueur du haut qui s'éloigne (vers le haut) est vu de dos
      if (e.peek('cer') === 1 && e.peekS16('ply', 2) < y0 && e.peek('view_x', 1) === 1) upBack = true;
      if (/ : (PILE|FACE)$/.test(msg(e)) && / : (PILE|FACE)$/.test(msg2())) calls = msg(e) + ' / ' + msg2();
      if (e.peek('pose_force', 0) === 5) wave = true;
      if (e.peek('cer') === 3 && (oamHas(e, e.peek('ball_base') + 8) || oamHas(e, e.peek('ball_base') + 12))) coin = true;
      if (/^(PILE|FACE) !$/.test(msg(e))) toss = msg(e);
    }
    check(e.peek('cer') === 0 && start[0] === 216 && start[1] >= 230 && wave && upBack && JSON.stringify([pos(e, 0), pos(e, 1)]) === JSON.stringify(want) && e.peek('ms') === 0,
      `${v} : entrée depuis le tunnel ${JSON.stringify(start)}, le joueur du fond vu de dos en s'éloignant, salut, puis chacun à sa place ${JSON.stringify(want)}`);
    // tirage au sort : pile ou face attribués aux deux joueurs, la pièce, puis
    // le résultat ; le gagnant (celui à qui on a attribué le côté sorti) sert
    const srvName = calls.split(' / ')[e.peek('sc_server')] ?? '';
    check(coin && calls !== '' && toss !== '' && srvName.endsWith(toss.replace(' !', '')),
      `${v} : tirage au sort (« ${calls} », puis « ${toss} ») : le gagnant sert`);
  }
  {
    // A passe l'entrée : chacun à sa place tout de suite
    const e = boot(); e.cerSkip = false; startWith(e, { FMT: 3, SURF: 1 }); e.run(30);
    const was = e.peek('cer'); e.tap('A', 1, 2, 2);
    check(was === 1 && e.peek('cer') === 0 && pos(e, 0)[0] !== 216, 'A passe l\'entrée des joueurs');
  }
  {
    // changement de côté (ordinateur contre ordinateur) : chaises, puis l'autre côté
    const e = boot(); startWith(e, { FMT: 3, SURF: 1 }); allCpu(e); e.run(20); e.cerSkip = false;
    // les chaises apparaissent dès la fin de l'échange qui amène le changement de côté
    runUntil(e, () => e.peek('cer') === 0xFD || e.peek('cer') === 4, 60 * 120);
    e.run(2);
    const early = e.peek('ms') === 3 && oamHas(e, CHAIR);
    runUntil(e, () => e.peek('cer') === 4, 1500);
    const before = [pos(e, 0)[1] < 132, pos(e, 1)[1] < 132];
    let seated = false, chair = false, bottle = false, towel = false;
    for (let f = 0; f < 600 && e.peek('cer'); f++) {
      e.run(1);
      if (e.peek('cer') === 5) {
        seated = seated || (e.peek('pose_force', 0) === 31 && e.peek('pose_force', 1) === 31 && pos(e, 0)[0] === 46);
        chair = chair || oamHas(e, CHAIR); bottle = bottle || oamHas(e, prop(e)); towel = towel || oamHas(e, prop(e) + 1);
      }
    }
    const after = [pos(e, 0)[1] < 132, pos(e, 1)[1] < 132];
    check(early, 'changement de côté : les chaises apparaissent dès la fin de l\'échange');
    check(seated && chair && bottle && towel, 'changement de côté : assis de profil sur leur chaise, près de l\'arbitre (chaise, gourde, serviette)');
    check(e.peek('cer') === 0 && before[0] !== after[0] && before[1] !== after[1], 'changement de côté : chacun rejoint l\'autre côté du court');
  }
  for (const dbl of [0, 1]) {
    // fin du match : à genoux, allongé (raquette au sol), poignée de main, salut
    const e = boot(); startWith(e, { FMT: 3, SURF: 1, TYPE: dbl }); allCpu(e); e.run(20);
    runUntil(e, () => e.peek('ms') === 4, 60 * 60 * 20, () => { if (e.peek('cer') >= 4 && e.peek('cer') <= 6) e.poke('cer_skip', 1); });
    e.cerSkip = false; e.poke('cer_skip', 0);
    const w = e.peek('sc_winner'), n = e.peek('npl');
    let kneel = false, supine = false, racket = false, shake = false, close = false, salute = false, frames = 0;
    for (let f = 0; f < 900 && app(e) === AS.MATCH; f++) {
      e.run(1); frames++;
      const c = e.peek('cer');
      if (c === 7) {
        if ((e.peek('pose_force', w) & 31) === 29) kneel = true;
        if ((e.peek('pose_force', w) & 31) === 30) { supine = true; racket = racket || oamHas(e, prop(e) + 2); }
      }
      if (c === 9) {
        shake = [...Array(n).keys()].every(p => (e.peek('pose_force', p) & 31) === 28);
        close = Math.abs(pos(e, 0)[0] - pos(e, 1)[0]) <= 16 && Math.abs(pos(e, 0)[1] - 132) <= 12 && Math.abs(pos(e, 1)[1] - 132) <= 12;
      }
      if (c === 10 && (e.peek('pose_force', 0) & 0x80)) salute = true;
    }
    check(kneel && supine && racket, `${dbl ? 'double' : 'simple'} : balle de match, vainqueur à genoux poings levés, puis allongé, raquette lâchée`);
    check(shake && close && salute && app(e) !== AS.MATCH && frames < 700, `${dbl ? 'double' : 'simple'} : poignée de main au filet, salut à l'arbitre, puis les résultats (${frames} trames)`);
  }
}
console.log("fin", passes, fails);
