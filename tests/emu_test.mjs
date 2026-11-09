// Pixel Open — tests de validation sur émulateur (jsnes).
// Usage : node emu_test.mjs ../build/pixel_open.nes
import { Emu } from './emu.mjs';

const rom = process.argv[2] ?? '../build/pixel_open.nes';
let fails = 0, passes = 0;
function check(cond, msg) {
  if (cond) { passes++; console.log('  ok   ' + msg); }
  else { fails++; console.log('  ECHEC ' + msg); }
}
const AS = { SCREENS: 0, MATCH: 1, MANUAL: 2 };
const SS = { TITLE: 0, MENU: 1, RESULTS: 2, PRESENT: 3, COMP: 4, CODE: 5, PALMARES: 6, TRAIN_RES: 7, COMP_END: 8 };
const O = { MODE: 1, TYPE: 2, P1: 3, P2: 4, LEVEL: 5, SURF: 6, ENV: 7, COND: 8, FMT: 9, REPLAY: 10, COMP: 11,
            CSIZE: 12, CPROG: 13, CVAR: 14, EX: 15, SIDE: 16, SPEED: 17, SPIN: 18, COUNT: 19, CPART: 20, GSPEED: 21, SOUND: 22 };
const MODE = { P1: 0, P2: 1, TEAM: 2, COMP: 3, TRAIN: 4, DEMO: 5 };
const CFG = { CTRL: 2, LEVEL: 10, GAMES: 17, REPLAYS: 19 };

function boot() { const e = new Emu(rom); e.run(30); return e; }
const scr = (e) => e.peek('scr_state');
const app = (e) => e.peek('app_state');
function toMenu(e) { e.tap('START'); e.run(10); }
function setOpts(e, opts) { for (const [k, v] of Object.entries(opts)) e.poke('opt_val', v, O[k]); }
// depuis le titre : configure et lance (présentation -> A si nécessaire)
function startWith(e, opts) {
  toMenu(e); setOpts(e, opts); e.tap('START');
  // certains écrans (classements) se construisent en plusieurs trames
  for (let n = 0; n < 90 && app(e) === AS.SCREENS && scr(e) === SS.MENU; n++) e.run(1);
  e.run(10);
  if (app(e) === AS.SCREENS && scr(e) === SS.PRESENT) { e.tap('A'); e.run(5); }
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

console.log('Démarrage et cartouche MMC1');
{
  const e = boot();
  check(app(e) === AS.SCREENS && scr(e) === SS.TITLE, 'écran titre affiché');
  const r = e.peek16('region_cnt');
  check(r > 1700 && r < 1970 && e.peek('is_pal') === 0, `détection NTSC (compteur ${r})`);
  const f = e.peek('frame_cnt'); e.run(10);
  check(((e.peek('frame_cnt') - f) & 255) === 10, 'NMI à chaque trame');
  check(e.peek('cur_bank') === 1, 'banque des écrans active sur le titre');
  // motifs copiés en CHR-RAM : la tuile 'A' de la police n'est pas vide
  const v = e.nes.ppu.vramMem; let nz = 0; for (let i = 0; i < 16; i++) nz += v[0x41 * 16 + i];
  check(nz > 0, 'police chargée en CHR-RAM depuis la banque 2');
  check(e.text(10, 16, 5) === 'JOUER', 'menu du titre affiché');
}

console.log("Mode d'emploi");
{
  const e = boot();
  e.tap('DOWN'); e.tap('A'); e.run(10);
  check(app(e) === AS.MANUAL && e.peek('cur_bank') === 4, "le mode d'emploi s'ouvre (banque 4)");
  check(e.text(2, 3, 17) === 'COMMANDES DE BASE', 'page 1 : commandes de base');
  for (let i = 0; i < 7; i++) { e.tap('RIGHT'); e.run(5); }
  check(e.peek('page') === 7 && e.text(2, 3, 8) === 'CONSEILS', '8 pages consultables');
  e.tap('RIGHT'); e.run(5);
  check(e.peek('page') === 0, 'navigation circulaire');
  e.tap('B'); e.run(10);
  check(app(e) === AS.SCREENS && scr(e) === SS.TITLE, 'B revient au titre');
}

console.log('Menu de configuration');
{
  const e = boot(); toMenu(e);
  check(scr(e) === SS.MENU && e.peek('nvis') === 13, 'configuration match : 13 lignes');
  e.tap('DOWN'); e.tap('RIGHT'); e.tap('RIGHT'); e.tap('RIGHT'); e.run(5);
  check(e.peek('opt_val', O.MODE) === MODE.COMP && e.peek('nvis') === 13, 'mode compétition : lignes adaptées');
  e.tap('RIGHT'); e.run(5);
  check(e.peek('opt_val', O.MODE) === MODE.TRAIN && e.peek('nvis') === 9, 'mode entraînement : lignes adaptées');
  e.tap('B'); e.run(5);
  check(scr(e) === SS.TITLE, 'B revient au titre');
}

console.log('Mode en équipe (double)');
{
  const e = boot(); toMenu(e); setOpts(e, { MODE: MODE.TEAM, TYPE: 0, CPART: 1 }); e.tap('B'); e.run(5); toMenu(e);
  check(e.peek('nvis') === 13 && e.text(3, 7, 10) === 'PARTENAIRE' && e.text(3, 8, 8) === 'JOUE PAR', 'pas de ligne JEU ; lignes PARTENAIRE et JOUE PAR');
  e.tap('START'); e.run(15);
  check(e.text(4, 4, 15) === 'MATCH DE DOUBLE' && e.text(18, 11, 11) === '+ MANETTE 2', 'toujours un double, manette 2 partenaire');
  e.tap('A'); e.run(30);
  const c = e.addr('cfg');
  check(e.nes.cpu.mem[c + 1] === 1 && e.nes.cpu.mem[c + 2] === 0 && e.nes.cpu.mem[c + 4] === 1 && e.nes.cpu.mem[c + 3] === 2,
    'manettes 1 et 2 dans la même équipe face à deux joueurs CPU');
  check(e.nes.cpu.mem[c + 8] === e.peek('opt_val', O.P2), 'PARTENAIRE = personnage de la manette 2');
}
{
  const e = boot(); toMenu(e); setOpts(e, { MODE: MODE.TEAM, CPART: 0 }); e.tap('START'); e.run(15); e.tap('A'); e.run(30);
  const c = e.addr('cfg');
  check(e.nes.cpu.mem[c + 1] === 1 && e.nes.cpu.mem[c + 2] === 0 && e.nes.cpu.mem[c + 4] === 2,
    'partenaire joué par l\'ordinateur (JOUE PAR : CPU)');
}

console.log('Court Margaux');
{
  const e = boot(); toMenu(e);
  for (let k = 0; k < 7; k++) e.tap('DOWN');
  const seen = new Set(); for (let i = 0; i < 6; i++) { e.tap('RIGHT'); seen.add(e.peek('opt_val', O.ENV)); }
  check(seen.has(4) && !seen.has(3), 'court Margaux proposé d\'emblée (club nudiste toujours verrouillé)');
  setOpts(e, { ENV: 4 }); e.tap('START'); e.run(15);
  check(e.text(4, 24, 22) === 'BIENVENUE SUR LE COURT', 'présentation du court Margaux');
  e.tap('A'); e.run(30);
  check(e.text(10, 3, 13) === 'COURT MARGAUX' && e.nes.cpu.mem[e.addr('pal_buf') + 1] === 0x25, 'court rose avec le panneau COURT MARGAUX');
  allCpu(e); e.poke('crowd_timer', 60); e.run(70);
  check(e.peek('crowd_timer') === 0, 'coeurs, fleurs et étoiles scintillent puis se reposent');
}

console.log('Présentation et pause');
{
  const e = boot(); toMenu(e); e.tap('START'); e.run(10);
  check(scr(e) === SS.PRESENT && e.text(4, 4, 12) === 'MATCH AMICAL', "présentation avant le match");
  e.tap('A'); e.run(30);
  check(app(e) === AS.MATCH && e.peek('cur_bank') === 0, 'A lance le match (banque 0)');
  e.tap('START');
  check(e.peek('paused') === 1, 'START ouvre le menu de pause');
  const x = e.peek16('plx'); e.run(30);
  check(e.peek16('plx') === x, 'le jeu est figé en pause');
  e.tap('DOWN'); e.tap('A'); e.run(10);
  check(app(e) === AS.SCREENS && scr(e) === SS.MENU, 'QUITTER ramène au menu');
}

console.log('Service : lancer visible pour le joueur du haut');
{
  const e = boot(); startWith(e, { MODE: MODE.P2 });
  runUntil(e, () => e.peek('ms') === 0, 300);
  e.poke('sc_server', 1); e.run(2);
  e.tap('A', 2, 2, 1);
  check(e.peek('ms') === 1, 'la manette 2 lance la balle (serveur du haut)');
  const ys = []; for (let i = 0; i < 40 && e.peek('ms') === 1; i++) { e.run(1); ys.push(e.nes.cpu.mem[0x200]); }
  const mn = Math.min(...ys), mx = Math.max(...ys);
  check(mn > 9 && mx - mn >= 8, `la balle monte et descend à l'écran (y ${mn}..${mx})`);
  runUntil(e, () => e.peekS16('bvz') <= 0, 40);
  e.tap('A', 2, 2, 1);
  check(e.peek('ms') === 2 || e.peek('ms') === 3, 'A (manette 2) frappe le service');
  check(e.peek('hitter') === 1, 'le serveur du haut est bien le frappeur');
}

console.log('Balle ratée : elle sort de l\'écran');
{
  const e = boot(); startWith(e, {});
  runUntil(e, () => e.peek('ms') === 0, 300);
  e.poke('sc_server', 1);            // l'ordinateur sert, le joueur ne bouge pas
  runUntil(e, () => e.peek('ms') === 2, 300);
  let maxY = 0, gone = false, stuckFrames = 0, lastY = -1;
  for (let i = 0; i < 400 && !gone; i++) {
    e.run(1);
    const y = e.peekS16('by') / 64;
    if (e.peek('b_vis') === 0) gone = true;
    else { maxY = Math.max(maxY, y); stuckFrames = (y === lastY && e.peekS16('bvy') === 0 && e.peek('ms') === 3) ? stuckFrames + 1 : 0; lastY = y; }
    if (stuckFrames > 30) break;
  }
  check(gone && maxY > 236, `la balle ratée continue jusqu'au bas de l'écran puis disparaît (y max ${maxY.toFixed(0)})`);
}

console.log('Filet effleuré au service (let)');
{
  const e = boot(); startWith(e, {});
  runUntil(e, () => e.peek('ms') === 0, 300);
  e.poke('sc_server', 0); e.run(1);
  // balle de service juste avant le filet, à hauteur de bande, vers le carré gauche
  const set16 = (n, v) => { e.poke(n, v & 255); e.poke(n, (v >> 8) & 255, 1); };
  e.poke('ms', 2); e.poke('b_vis', 1); e.poke('b_live', 1); e.poke('b_serve', 1); e.poke('hitter', 0);
  e.poke('bounces', 0); e.poke('faults', 0); e.poke('bgrav', 6);
  set16('bx', 110 * 64); set16('by', 134 * 64); set16('bz', 6 * 64);
  set16('bvx', 0); set16('bvy', -3 * 64); set16('bvz', 0);
  e.run(1);
  check(e.peek('b_netcord') === 1, 'la balle touche la bande et passe');
  runUntil(e, () => e.peek('ms') !== 2, 120);
  check(e.peek('end_reason') === 3 && e.peek('faults') === 0, 'service à remettre (LET), pas de faute');
}

function fullMatch(label, opts, maxFrames, extra) {
  const e = boot(); startWith(e, opts); allCpu(e);
  if (extra) extra(e);
  let lastMs = '', prevMs = -1, same = 0, worst = 0, spGood = true, replays = 0;
  const ok = runUntil(e, () => {
    const m = e.peek('ms');
    if (m === 5 && prevMs !== 5) replays++;
    prevMs = m;
    // blocage = ni l'état du match, ni la balle, ni les joueurs ne changent
    const sig = m + ',' + e.peek16('bx') + ',' + e.peek16('by') + ',' + e.peek16('plx') + ',' + e.peek16('plx', 2);
    if (sig === lastMs) { same++; worst = Math.max(worst, same); } else { same = 0; lastMs = sig; }
    if ((e.frames & 255) === 0 && !spOk(e)) spGood = false;
    return app(e) === AS.SCREENS;
  }, maxFrames);
  e.run(20);   // l'écran de résultats se construit sur quelques trames
  const pts = e.peek('st_points', 0) + e.peek('st_points', 1);
  check(ok && scr(e) === SS.RESULTS, `${label} : match terminé en ${(e.frames / 3600).toFixed(1)} min (${pts} points)`);
  const g = [e.peek('sc_hist', 0), e.peek('sc_hist', 1)];
  check(e.peek('sc_over') === 1 && Math.max(...g) >= e.peek('cfg', CFG.GAMES) && Math.abs(g[0] - g[1]) >= 1,
    `${label} : score final valide (${g[0]}-${g[1]})`);
  check(worst < 600 && spGood && spOk(e), `${label} : ni blocage (immobilité max ${worst} trames) ni pile corrompue`);
  return { e, replays };
}

console.log('Matchs complets CPU contre CPU (format rapide)');
const SURF = ['terre', 'gazon', 'dur', 'moquette', 'plage', 'glace'];
const ENV = ['stade', 'club', 'salle'];
const COND = ['jour', 'nuit', 'pluie'];
let totalReplays = 0;
for (let s = 0; s < 6; s++) {
  const r = fullMatch(`simple ${SURF[s]}/${ENV[s % 3]}/${COND[(s + 1) % 3]}`,
    { FMT: 3, SURF: s, ENV: s % 3, COND: (s + 1) % 3, LEVEL: s % 3, P2: (s + 4) % 10 }, 60 * 60 * 25);
  totalReplays += r.replays;
}
{
  const r = fullMatch('double dur/salle', { FMT: 3, TYPE: 1, SURF: 2, ENV: 2 }, 60 * 60 * 25);
  totalReplays += r.replays;
}
check(totalReplays > 0, `ralentis déclenchés sur les beaux points (${totalReplays})`);

console.log('Revanche et palmarès');
{
  const e = boot(); startWith(e, { FMT: 3 });
  runUntil(e, () => app(e) === AS.SCREENS, 60 * 60 * 25, () => botStep(e));
  e.releaseAll(); e.run(20);
  check(scr(e) === SS.RESULTS, 'match humain (robot) terminé');
  const pl = e.peek16('sv', 4);
  check(pl === 1, 'palmarès : 1 match joué enregistré');
  e.tap('A'); e.run(10);
  check(app(e) === AS.MATCH, 'A : revanche immédiate');
  allCpu(e);
  runUntil(e, () => app(e) === AS.SCREENS, 60 * 60 * 25); e.run(20);
  check(scr(e) === SS.RESULTS && spOk(e), 'second match enchaîné terminé');
  e.tap('B'); e.run(5); e.tap('B'); e.run(5);
  e.tap('DOWN'); e.tap('DOWN'); e.tap('A'); e.run(10);
  check(scr(e) === SS.PALMARES && e.text(3, 4, 5) === 'JOUES', 'écran palmarès');
  e.tap('RIGHT'); e.run(5); e.tap('RIGHT'); e.run(5);
  check(e.text(3, 3, 12) === "RECORDS D'EN", "page des records d'entraînement");
}

console.log('Entraînement : 9 exercices');
const EXN = ['frappes', 'déplacements', 'régularité', 'précision', 'volée', 'service', 'joueur fixe', 'personnalisé', 'tutoriel'];
for (let ex = 0; ex < 9; ex++) {
  const e = boot(); startWith(e, { MODE: MODE.TRAIN, EX: ex, COUNT: 0 });
  let tut = [ ['A'], ['B', 'DOWN'], ['B', 'UP'], ['A', 'LEFT'] ];
  const ok = runUntil(e, () => app(e) === AS.SCREENS, 60 * 60 * 8, () => {
    const ms = e.peek('ms');
    if ((ex === 5 || (ex === 8 && e.peek('tr_step') === 4)) && e.peek('sc_server') === 0 && (ms === 0 || ms === 1)) {
      e.releaseAll();
      if (ms === 0 && (e.frames % 20) === 0) e.down('A');
      if (ms === 1 && e.peekS16('bvz') <= 0) e.down('B');
      return;
    }
    if (ex === 8) botStep(e, tut[Math.min(e.peek('tr_step'), 3)]);
    else botStep(e);
  });
  e.releaseAll(); e.run(20);
  check(ok && scr(e) === SS.TRAIN_RES && e.peek('tr_score') > 0,
    `${EXN[ex]} : exercice terminé (${e.peek('tr_score')} pts / ${e.peek('tr_total')})`);
}

console.log('Compétitions');
function playComp(type, opts, wins) {
  const e = boot(); startWith(e, Object.assign({ MODE: MODE.COMP, COMP: type, FMT: 3 }, opts));
  let matches = 0, code = '';
  const CN = ['TOURNOI', 'POULES', 'CHAMPIONNAT', 'SAISON', 'PERSONNALISE'][type];
  check(scr(e) === SS.COMP && e.text(16, 1, CN.length) === CN, `${CN} : écran d'étape (titre lisible)`);
  while (scr(e) === SS.COMP && matches < 20) {
    if (matches === 1) code = e.text(10, 21, 15).replace(/ /g, '');
    e.tap('A'); e.run(5); e.tap('A'); e.run(30);
    if (app(e) !== AS.MATCH) break;
    // victoire forcée du joueur pour parcourir la compétition
    e.poke('sc_winner', wins(matches) ? 0 : 1); e.poke('sc_over', 1); e.poke('ms', 4); e.poke('ms_timer', 2);
    runUntil(e, () => app(e) === AS.SCREENS, 60); e.run(20);
    matches++;
    e.tap('A'); e.run(40);
  }
  return { e, matches, code };
}
{
  const { e, matches, code } = playComp(0, {}, () => true);
  check(matches === 3 && scr(e) === SS.COMP_END && e.peek('c_rank') === 1, `tournoi gagné en 3 matchs`);
  check(/^[A-Z2-9]{13}$/.test(code), `code de reprise affiché : ${code}`);
  check(e.peek('sv', 0x14 + 0) >= 0, 'palmarès mis à jour');
}
{
  const { e, matches } = playComp(0, {}, (m) => m < 1);
  check(matches === 2 && e.peek('c_rank') === 3, 'tournoi : élimination en demi-finale');
}
{
  const { e, matches } = playComp(1, {}, () => true);
  check(matches === 5 && e.peek('c_rank') === 1, 'poules : 3 journées + demi + finale gagnées');
}
{
  const { e, matches } = playComp(1, {}, () => false);
  check(matches === 3 && e.peek('c_rank') === 5, 'poules : élimination en phase de groupes');
}
{
  const { e, matches } = playComp(2, {}, () => true);
  check(matches === 5 && e.peek('c_rank') === 1, 'championnat : 5 journées, champion');
}
{
  const { e, matches } = playComp(3, {}, () => true);
  check(matches === 12 && e.peek('c_rank') === 1, 'saison : 4 tournois sur 4 surfaces, n°1');
}
{
  const { e, matches } = playComp(3, {}, (m) => m % 3 === 0);
  check(matches === 6 && e.peek('c_over') === 2, `saison avec défaites : classement final ${e.peek('c_rank')}e`);
}
{
  const { e, matches } = playComp(4, { CSIZE: 0, CPROG: 1, CVAR: 1 }, () => true);
  check(matches === 2 && e.peek('c_rank') === 1, 'personnalisé (4 joueurs, progressif, surfaces variées)');
}

console.log('Code de reprise');
{
  const e = boot(); startWith(e, { MODE: MODE.COMP, COMP: 2, P1: 6, SURF: 4, ENV: 1, COND: 2 });
  e.tap('A'); e.run(5); e.tap('A'); e.run(30);
  e.poke('sc_winner', 0); e.poke('sc_over', 1); e.poke('ms', 4); e.poke('ms_timer', 2);
  runUntil(e, () => app(e) === AS.SCREENS, 60); e.run(20); e.tap('A'); e.run(40);
  const code = e.text(10, 21, 15).replace(/ /g, '');
  const want = ['c_type', 'c_stage', 'c_seed', 'c_results', 'c_surf', 'c_env', 'c_cond'].map(n => e.peek(n));
  const f = boot(); f.tap('UP'); f.tap('A'); f.run(10);
  check(scr(f) === SS.CODE, 'saisie du code depuis le titre');
  const AL = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
  for (let i = 0; i < 13; i++) {
    const n = AL.indexOf(code[i]);
    for (let k = 0; k < n; k++) f.tap('UP', 1, 1, 1);
    if (i < 12) f.tap('RIGHT', 1, 1, 1);
  }
  f.tap('START'); f.run(40);
  const got = ['c_type', 'c_stage', 'c_seed', 'c_results', 'c_surf', 'c_env', 'c_cond'].map(n => f.peek(n));
  check(scr(f) === SS.COMP && JSON.stringify(got) === JSON.stringify(want), `code ${code} : championnat restauré (journée 2)`);
  check(f.text(16, 6, 5) === e.text(16, 6, 5), "même adversaire après reprise");
  const g = boot(); g.tap('UP'); g.tap('A'); g.run(10); g.tap('START'); g.run(10);
  check(scr(g) === SS.CODE, 'code invalide refusé');
}

console.log('Compétition en double, levée de coupe et club nudiste');
{
  const e = boot();
  check(e.peek('nude_unlocked') === 0, 'club nudiste verrouillé au départ');
  toMenu(e);
  let envs = new Set();
  for (let k = 0; k < 7; k++) e.tap('DOWN');
  for (let i = 0; i < 6; i++) { e.tap('RIGHT'); envs.add(e.peek('opt_val', O.ENV)); }
  check(!envs.has(3) && envs.size === 4, 'lieu « club nudiste » absent avant déblocage (4 lieux proposés)');
}
{
  const { e, matches, code } = playComp(0, { TYPE: 1, P2: 5 }, () => true);
  check(e.peek('c_dbl') === 1 && matches === 3 && e.peek('c_rank') === 1, 'tournoi en double gagné (3 matchs)');
  check(e.peek('c_pl2', 0) === 5, 'partenaire choisi (ZOE) conservé');
  const pl = new Set(); for (let i = 0; i < 8; i++) { pl.add(e.peek('c_pl', i)); pl.add(e.peek('c_pl2', i)); }
  check(pl.size === 16, '8 équipes de 2 : 16 personnages différents');
  // levée de coupe : les tuiles du trophée sont à l'écran
  e.run(80);
  const tt = e.peek('trophy_tile'), ct = e.peek('confetti_tile'); let cup = 0, conf = 0;
  for (let i = 0; i < 64; i++) { const t = e.nes.cpu.mem[0x201 + i * 4], y = e.nes.cpu.mem[0x200 + i * 4]; if (y < 0xEF) { if (t >= tt && t < tt + 4) cup++; if (t === ct) conf++; } }
  check(cup === 4 && conf >= 8, `levée de coupe affichée (coupe ${cup} tuiles, ${conf} confettis)`);
  check(e.peek('nude_unlocked') === 1 && e.text(3, 23, 21) === 'MODE NUDISTE DEBLOQUE', 'victoire : club nudiste débloqué');
  check(/^[A-Z2-9]{13}$/.test(code), `code de reprise en double : ${code}`);
  // le lieu « club nudiste » est maintenant proposé
  e.tap('A'); e.run(10); toMenu(e);
  setOpts(e, { MODE: MODE.P1, TYPE: 0 }); e.tap('B'); e.run(5); toMenu(e);
  for (let k = 0; k < 7; k++) e.tap('DOWN');
  let seen = false; for (let i = 0; i < 4; i++) { e.tap('RIGHT'); if (e.peek('opt_val', O.ENV) === 3) seen = true; }
  check(seen, 'lieu « club nudiste » proposé après déblocage');
  setOpts(e, { ENV: 3 }); e.tap('START'); e.run(15);
  check(e.text(4, 24, 24) === 'CLUB NUDISTE LES BRONZES', 'présentation du club nudiste');
  e.tap('A'); e.run(40); allCpu(e);
  const v = e.nes.ppu.vramMem;
  check(e.peek('cur_bank') === 0 && e.text(10, 3, 12) === 'CLUB NUDISTE', 'match au club nudiste (panneau du club)');
  // tuiles des personnages remplacées en CHR-RAM, puis restaurées au retour aux menus
  const nudeTile = [...v.slice(0x1100 + 2 * 16, 0x1100 + 3 * 16)].join(',');   // torse
  runUntil(e, () => app(e) === AS.SCREENS, 60 * 60 * 25); e.run(80);
  e.tap('B'); e.run(40);
  const normTile = [...e.nes.ppu.vramMem.slice(0x1100 + 2 * 16, 0x1100 + 3 * 16)].join(',');
  check(nudeTile !== normTile, 'tenues remplacées au club nudiste puis restaurées dans les menus');
  check(e.peek16('sv', 6) >= 1, 'palmarès : match de double compté');
}
{
  // un code portant un trophée débloque aussi le club nudiste sur une autre console
  const { e } = playComp(4, { CSIZE: 0 }, () => true);
  e.tap('A'); e.run(10); toMenu(e); setOpts(e, { MODE: MODE.COMP, COMP: 0 }); e.tap('START'); e.run(40);
  const code = e.text(10, 21, 15).replace(/ /g, '');
  check(e.peek('c_trophies') === 1, 'les trophées se cumulent d\'une compétition à l\'autre');
  const f = boot(); f.tap('UP'); f.tap('A'); f.run(10);
  const AL = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
  for (let i = 0; i < 13; i++) {
    const n = AL.indexOf(code[i]);
    for (let k = 0; k < n; k++) f.tap('UP', 1, 1, 1);
    if (i < 12) f.tap('RIGHT', 1, 1, 1);
  }
  f.tap('START'); f.run(40);
  check(scr(f) === SS.COMP && f.peek('nude_unlocked') === 1, `code ${code} (1 trophée) : club nudiste débloqué`);
}

console.log('Compétition en double à deux joueurs humains');
{
  const e = boot(); startWith(e, { MODE: MODE.COMP, COMP: 2, TYPE: 1, P2: 3, CPART: 1 });
  check(scr(e) === SS.COMP && e.text(3, 23, 18) === 'DOUBLE A 2 JOUEURS', 'championnat en double à 2 joueurs');
  e.tap('A'); e.run(10);
  check(e.text(18, 11, 11) === '+ MANETTE 2', 'présentation : partenaire à la manette 2');
  e.tap('A'); e.run(40);
  const c = e.addr('cfg');
  check(app(e) === AS.MATCH && e.nes.cpu.mem[c + 2] === 0 && e.nes.cpu.mem[c + 4] === 1 && e.nes.cpu.mem[c + 3] === 2 && e.nes.cpu.mem[c + 5] === 2,
    'manette 1 et manette 2 dans la même équipe, adversaires CPU');
  runUntil(e, () => e.peek('ms') === 2 || e.peek('ms') === 0, 300);
  const x0 = e.peekS16('plx', 4); e.down('RIGHT', 2); e.run(20); e.up('RIGHT', 2);
  check(e.peekS16('plx', 4) !== x0, 'la manette 2 déplace le partenaire');
  // victoire forcée puis reprise par code sur une autre console
  e.poke('sc_winner', 0); e.poke('sc_over', 1); e.poke('ms', 4); e.poke('ms_timer', 2);
  runUntil(e, () => app(e) === AS.SCREENS, 60); e.run(20); e.tap('A'); e.run(40);
  const code = e.text(10, 21, 15).replace(/ /g, '');
  const f = boot(); f.tap('UP'); f.tap('A'); f.run(10);
  const AL = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
  for (let i = 0; i < 13; i++) {
    const n = AL.indexOf(code[i]);
    for (let k = 0; k < n; k++) f.tap('UP', 1, 1, 1);
    if (i < 12) f.tap('RIGHT', 1, 1, 1);
  }
  f.tap('START'); f.run(40);
  check(scr(f) === SS.COMP && f.peek('c_hum2') === 1 && f.peek('c_dbl') === 1 && f.peek('c_pl2', 0) === 3,
    `code ${code} : double à 2 joueurs restauré`);
}

console.log('Vitesse du jeu, coups gagnants, smash sauté, musiques');
{
  // vitesse : nombre de trames logiques du match pour 120 trames affichées
  const rate = (gs) => {
    const e = boot(); startWith(e, { FMT: 3, GSPEED: gs }); allCpu(e);
    runUntil(e, () => e.peek('ms') === 0, 200);
    e.poke('ms', 4); e.poke('ms_timer', 250); e.run(120);    // fin de match : ms_timer décompte chaque trame logique
    return 250 - e.peek('ms_timer');
  };
  const r0 = rate(0), r1 = rate(1), r2 = rate(2);
  check(Math.abs(r0 - 90) <= 1 && Math.abs(r1 - 120) <= 1 && Math.abs(r2 - 150) <= 1, `vitesse du jeu : lent ${r0}, normal ${r1}, rapide ${r2} trames logiques / 120`);
  const e = boot(); startWith(e, { FMT: 3, GSPEED: 0 });
  check(e.peek('cfg', 26) === 0, 'option VITESSE JEU transmise au match');
  runUntil(e, () => e.peek('ms') === 0, 200);
  // un appui pendant une trame sautée n'est pas perdu (lent) : START ouvre la pause
  // appui d'une seule trame, à chacune des 4 phases (dont la trame sautée)
  let paused = 0;
  for (let k = 0; k < 4; k++) {
    e.run(k + 1); e.tap('START', 1, 1, 4); paused += e.peek('paused');
    e.tap('B', 1, 1, 6);
  }
  check(paused === 4, `mode lent : un appui bref n'est jamais perdu (${paused}/4)`);
}
{
  // coups gagnants nommés : on simule une balle gagnante qui sort après rebond
  const name = (shot, pass, jump) => {
    const e = boot(); startWith(e, { FMT: 3 }); allCpu(e);
    runUntil(e, () => e.peek('ms') === 0, 200);
    const set16 = (n, v) => { e.poke(n, v & 255); e.poke(n, (v >> 8) & 255, 1); };
    e.poke('ms', 2); e.poke('b_live', 1); e.poke('b_vis', 1); e.poke('b_serve', 0); e.poke('bounces', 1);
    e.poke('hitter', 0); e.poke('rally_hits', 4); e.poke('last_shot', shot); e.poke('hit_pass', pass); e.poke('hit_jump', jump);
    set16('bx', 128 * 64); set16('by', 236 * 64); set16('bz', 4 * 64); set16('bvx', 0); set16('bvy', 3 * 64); set16('bvz', 0);
    e.run(3);
    let t = ''; for (let i = 0; i < 14; i++) t += String.fromCharCode(e.peek('msg1', i));
    return t.trim();
  };
  const got = [name(7, 0, 1), name(7, 0, 0), name(3, 0, 0), name(2, 1, 0), name(6, 0, 0), name(0, 1, 0), name(0, 0, 0)];
  check(got.join('|') === 'SMASH SAUTE !|SMASH !|AMORTIE !|LOB GAGNANT !|VOLEE GAGNANTE|PASSING !|COUP GAGNANT !',
    `coups gagnants nommés : ${got.join(', ')}`);
}
{
  // smash sauté : balle très haute (50 px) au-dessus d'un joueur humain près du filet
  const e = boot(); startWith(e, { FMT: 3 });
  runUntil(e, () => e.peek('ms') === 0, 200);
  const set16 = (n, v, o = 0) => { e.poke(n, v & 255, o); e.poke(n, (v >> 8) & 255, o + 1); };
  e.poke('ms', 2); e.poke('b_live', 1); e.poke('b_vis', 1); e.poke('b_serve', 0); e.poke('bounces', 0);
  e.poke('hitter', 1); e.poke('rally_hits', 3); e.poke('bgrav', 6);
  set16('plx', 128 * 64, 0); set16('ply', 168 * 64, 0);
  set16('bx', 132 * 64); set16('by', 165 * 64); set16('bz', 52 * 64); set16('bvx', 0); set16('bvy', 0); set16('bvz', 0);
  e.down('A'); e.run(3); e.up('A');
  check(e.peek('hit_jump') === 1 && e.peek('last_shot') === 7 && e.peek('hitter') === 0 && e.peekS16('bvy') < 0,
    'smash sauté sur une balle à 52 px de haut');
  check(e.peek('pl_jump', 0) > 0, 'le joueur saute (sprite décalé vers le haut)');
  e.run(40);
  check(e.peek('pl_jump', 0) === 0, 'puis retombe');
  // une balle à 52 px reste hors de portée d'un CPU de niveau normal
  const f = boot(); startWith(f, { FMT: 3, LEVEL: 1 }); allCpu(f);
  runUntil(f, () => f.peek('ms') === 0, 200);
  const g16 = (n, v, o = 0) => { f.poke(n, v & 255, o); f.poke(n, (v >> 8) & 255, o + 1); };
  f.poke('ms', 2); f.poke('b_live', 1); f.poke('b_vis', 1); f.poke('b_serve', 0); f.poke('bounces', 0);
  f.poke('hitter', 1); f.poke('rally_hits', 3); f.poke('bgrav', 0); f.poke('ai_delay', 0, 0);
  g16('plx', 128 * 64, 0); g16('ply', 168 * 64, 0);
  g16('bx', 132 * 64); g16('by', 165 * 64); g16('bz', 52 * 64); g16('bvx', 0); g16('bvy', 0); g16('bvz', 0);
  f.run(4);
  check(f.peek('hitter') === 1, 'le smash sauté est réservé aux humains et au CPU expert');
}
{
  // musiques hors match : valse Margaux au menu, thème de la finale avant la finale
  const e = boot(); toMenu(e);
  e.poke('cursor', 7); e.poke('opt_val', 3, O.ENV); e.tap('RIGHT'); e.run(5);
  check(e.peek('opt_val', O.ENV) === 4 && e.peek('music_cur') === 7, 'menu : court Margaux choisi, sa musique démarre');
  e.tap('LEFT'); e.run(5);
  check(e.peek('music_cur') === 1, 'autre lieu : retour au thème du titre');
  e.tap('RIGHT'); e.run(5); e.tap('START'); e.run(20);
  check(scr(e) === SS.PRESENT && e.peek('music_cur') === 7, 'présentation au court Margaux : valse');
  e.tap('A'); e.run(10);
  check(app(e) === AS.MATCH && e.peek('music_cur') === 0, 'aucune musique pendant le match');
}
{
  const e = boot(); startWith(e, { MODE: MODE.COMP, COMP: 0, FMT: 3 });
  check(scr(e) === SS.COMP && e.peek('music_cur') !== 6, 'quart de finale : pas de thème de finale');
  for (let m = 0; m < 2; m++) {   // tournoi à 8 : quart puis demi gagnés
    e.tap('A'); e.run(5); e.tap('A'); e.run(30);
    e.poke('sc_winner', 0); e.poke('sc_over', 1); e.poke('ms', 4); e.poke('ms_timer', 2);
    runUntil(e, () => app(e) === AS.SCREENS, 60); e.run(20); e.tap('A'); e.run(40);
  }
  check(scr(e) === SS.COMP && e.text(3, 3, 6) === 'FINALE' && e.peek('music_cur') === 6, 'écran de la finale : thème de la finale');
  e.tap('A'); e.run(10);
  check(scr(e) === SS.PRESENT && e.peek('music_cur') === 6, 'présentation de la finale : le thème continue');
  e.tap('A'); e.run(10);
  check(app(e) === AS.MATCH && e.peek('music_cur') === 0, 'finale : silence pendant le match');
}

console.log('Finitions : ralentis, IA, effets, son, réglages mémorisés');
const oamTiles = (e) => { const t = new Set(); for (let i = 0; i < 64; i++) { const y = e.peek('oam_buf', i * 4); if (y < 0xEF) t.add(e.peek('oam_buf', i * 4 + 1)); } return t; };
const msgText = (e) => { let t = ''; for (let i = 0; i < 14; i++) t += String.fromCharCode(e.peek('msg1', i)); return t.trim(); };
{
  // annonces du club nudiste : jamais plus de 14 caractères (bandeau)
  const fs = await import('fs');
  const rd = (f) => fs.readFileSync(new URL('../src/' + f, import.meta.url), 'utf8');
  const src = rd('extras.c') + rd('game.c') + rd('gamefix.c');
  const lits = [...src.matchAll(/(?:set2|set_msg|return)\s*\(?\s*(?:[a-z]+\s*\?\s*)?"([^"]*)"/g)].map((m) => m[1]);
  const lits2 = [...src.matchAll(/:\s*"([^"]*)";/g)].map((m) => m[1]);
  const long = [...lits, ...lits2].filter((t) => t.length > 14);
  check(long.length === 0, `annonces du bandeau ≤ 14 caractères${long.length ? ' : ' + long.join(', ') : ''}`);
}
{
  // CPU expert : smash sauté sur un lob pas trop profond (graines aléatoires variées)
  const lobTrial = (lv, seed, t) => {
    const e = boot(); startWith(e, { FMT: 3, LEVEL: lv });
    runUntil(e, () => e.peek('ms') === 0, 300);
    const s16 = (n, v, o = 0) => { e.poke(n, v & 255, o); e.poke(n, (v >> 8) & 255, o + 1); };
    e.poke('ms', 2); e.poke('b_live', 1); e.poke('b_vis', 1); e.poke('b_serve', 0); e.poke('bounces', 1);
    e.poke('hitter', 1); e.poke('rally_hits', 3); e.poke('bgrav', 6);
    s16('plx', (100 + t * 5) * 64, 0); s16('ply', 200 * 64, 0); s16('plx', (110 + t * 3) * 64, 2); s16('ply', 104 * 64, 2);
    s16('bx', (104 + t * 5) * 64); s16('by', 197 * 64); s16('bz', 10 * 64); s16('bvx', 0); s16('bvy', 0); s16('bvz', 0);
    e.poke('rand_seed', seed); e.down('B'); e.down('UP'); e.run(3); e.releaseAll();
    for (let i = 0; i < 200; i++) { e.run(1); if (e.peek('hitter') === 1) return e.peek('hit_jump'); if (e.peek('ms') !== 2) return 0; }
    return 0;
  };
  let j2 = 0, j1 = 0;
  for (let t = 0; t < 8; t++) { j2 += lobTrial(2, t * 37 + 11, t); j1 += lobTrial(1, t * 37 + 11, t); }
  check(j2 >= 2 && j2 <= 7 && j1 === 0, `lob contre un CPU au filet : smash sauté ${j2}/8 en Expert, ${j1}/8 en Normal`);
}
{
  // le saut est enregistré pour les ralentis (bits 5-7 de pose_cache)
  const e = boot(); startWith(e, { FMT: 3 });
  runUntil(e, () => e.peek('ms') === 0, 200);
  const s16 = (n, v, o = 0) => { e.poke(n, v & 255, o); e.poke(n, (v >> 8) & 255, o + 1); };
  e.poke('ms', 2); e.poke('b_live', 1); e.poke('b_vis', 1); e.poke('b_serve', 0); e.poke('bounces', 0);
  e.poke('hitter', 1); e.poke('rally_hits', 3); e.poke('bgrav', 6);
  s16('plx', 128 * 64, 0); s16('ply', 168 * 64, 0);
  s16('bx', 132 * 64); s16('by', 165 * 64); s16('bz', 52 * 64); s16('bvx', 0); s16('bvy', 0); s16('bvz', 0);
  e.down('A'); e.run(4); e.up('A');
  check((e.peek('pose_cache', 0) >> 5) > 0, 'saut enregistré pour le ralenti');
}
{
  // effets : ombres (simple), arbitre (stade), poussière (terre), repère (lent)
  const e = boot(); startWith(e, { FMT: 3, SURF: 0, ENV: 0, GSPEED: 0 }); allCpu(e);
  e.run(60);
  let t = oamTiles(e);
  check(t.has(5) && t.has(13) && (t.has(11) || t.has(12)), 'ombres des joueurs et arbitre de chaise au stade');
  let dust = false, pb = 0;
  runUntil(e, () => { const b = e.peek('bounces'); if (b === 1 && pb === 0) { e.run(2); dust = dust || [...oamTiles(e)].some((x) => x === 6 || x === 7); } pb = b; return dust; }, 3000);
  check(dust, 'poussière au rebond sur la terre battue');
  const f = boot(); startWith(f, { FMT: 3, ENV: 1, GSPEED: 0 });
  let mark = false;   // joueur humain (robot) contre le CPU
  runUntil(f, () => { if (f.peek('hitter') === 1 && f.peek('ms') === 2 && f.peek('bounces') === 0) mark = mark || oamTiles(f).has(10); return mark; }, 6000, () => botStep(f));
  f.releaseAll();
  check(mark, 'mode lent : repère d\'atterrissage des balles adverses');
  const g = boot(); startWith(g, { FMT: 3, ENV: 1, GSPEED: 1 });
  let mark2 = false;
  runUntil(g, () => { mark2 = mark2 || oamTiles(g).has(10); return false; }, 1500);
  check(!mark2 && !oamTiles(g).has(13), 'vitesse normale : pas de repère ; pas d\'arbitre hors du stade');
}
{
  // réglage SON et réglages mémorisés dans la cartouche
  const e = boot(); toMenu(e);
  const row = (o) => { for (let i = 0; i < e.peek('nvis'); i++) if (e.peek('vis', i) === o) return i; return -1; };
  e.poke('cursor', row(O.SOUND)); e.tap('RIGHT'); e.run(5);
  check(e.peek('snd_mode') === 1 && e.peek16('mel_ptr') === 0, 'SON : bruitages seuls, musique arrêtée');
  e.tap('RIGHT'); e.run(5);
  check(e.peek('snd_mode') === 2 && e.text(16, 4 + row(O.SOUND), 5) === 'AUCUN', 'SON : aucun');
  e.tap('LEFT'); e.tap('LEFT'); e.run(5);
  check(e.peek('snd_mode') === 0 && e.peek16('mel_ptr') !== 0, 'SON : musique relancée');
  setOpts(e, { P1: 5, SURF: 3, GSPEED: 2, SOUND: 1, ENV: 4 }); e.tap('START'); e.run(30);
  // « rallumage » : une console neuve qui garde la RAM de cartouche
  const f = new Emu(rom);
  for (let a = 0x6000; a < 0x8000; a++) f.nes.cpu.mem[a] = e.nes.cpu.mem[a];
  f.run(30);
  check(f.peek('opt_val', O.P1) === 5 && f.peek('opt_val', O.SURF) === 3 && f.peek('opt_val', O.GSPEED) === 2
    && f.peek('opt_val', O.ENV) === 4 && f.peek('snd_mode') === 1, 'derniers réglages retrouvés au démarrage (cartouche à pile)');
  check(f.peek16('sv', 4) === e.peek16('sv', 4), 'palmarès conservé à côté des réglages');
}

console.log('Compensation 50 Hz (PAL)');
{
  const e = boot(); e.poke('is_pal', 1); startWith(e, { FMT: 3 }); allCpu(e);
  check(runUntil(e, () => app(e) === AS.SCREENS, 60 * 60 * 25), 'match complet en mode 50 Hz');
}

console.log(`\n${passes} vérifications réussies, ${fails} échec(s) ; pile contrôlée ${spChecks} fois`);
process.exit(fails ? 1 : 0);
