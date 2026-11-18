// Royal Martinot — tests de validation sur émulateur (jsnes).
// Usage : node emu_test.mjs ../build/royal_martinot.nes
import fs from 'fs';
import { Emu } from './emu.mjs';

const ARGS = process.argv.slice(2);
const rom = ARGS.find(a => !a.startsWith('--')) ?? '../build/royal_martinot.nes';
// Sélection des sections (voir tests/run_tests.mjs) :
//   --only=bande,plongeon   sections dont le nom contient l'un de ces mots
//   --sections=0,4,7        sections par numéro (exécution en parallèle)
//   --list                  liste des sections, sans rien exécuter
const opt = (k) => { const a = ARGS.find(x => x.startsWith('--' + k + '=')); return a ? a.slice(k.length + 3) : null; };
const ONLY = (opt('only') ?? '').toLowerCase().split(',').map(t => t.trim()).filter(Boolean);
const SECTIONS = opt('sections') !== null ? new Set(opt('sections').split(',').filter(Boolean).map(Number)) : null;
const LIST = ARGS.includes('--list');
const MARK = !!process.env.TEST_MARKS;              // repères pour run_tests.mjs
let secIdx = -1, secT0 = 0;
function secEnd() { if (MARK && secT0) console.log(`\u00a7T ${secIdx} ${((Date.now() - secT0) / 1000).toFixed(1)}`); secT0 = 0; }
function sec(name) {
  secEnd();
  ++secIdx;
  if (LIST) { console.log(`${secIdx}\t${name}`); return false; }
  const n = name.toLowerCase();
  const run = (!SECTIONS || SECTIONS.has(secIdx)) && (!ONLY.length || ONLY.some(t => n.includes(t)));
  if (run) { if (MARK) console.log(`\u00a7S ${secIdx}`); secT0 = Date.now(); }
  return run;
}
let fails = 0, passes = 0;
function check(cond, msg) {
  if (cond) { passes++; console.log('  ok   ' + msg); }
  else { fails++; console.log('  ECHEC ' + msg); }
}
const AS = { SCREENS: 0, MATCH: 1, MANUAL: 2 };
const SS = { TITLE: 0, MENU: 1, RESULTS: 2, PRESENT: 3, COMP: 4, CODE: 5, PALMARES: 6, TRAIN_RES: 7, COMP_END: 8, SETBREAK: 9, CUSTOM: 13, CHAMP: 14, PICK: 15 };
const O = { MODE: 1, TYPE: 2, P1: 3, P2: 4, LEVEL: 5, SURF: 6, ENV: 7, COND: 8, FMT: 9, REPLAY: 10, COMP: 11,
            CSIZE: 12, CPROG: 13, CVAR: 14, EX: 15, SIDE: 16, SPEED: 17, SPIN: 18, COUNT: 19, CPART: 20, GSPEED: 21, SOUND: 22, FATIG: 23, VIEW: 24, CAREER: 25 };
const MODE = { P1: 0, P2: 1, TEAM: 2, COMP: 3, TRAIN: 4, DEMO: 5 };
const CFG = { CTRL: 2, LEVEL: 10, GAMES: 17, REPLAYS: 19 };

// démarrage : le générique de lancement est passé avec START, puis écran titre
function boot(render = false) { const e = new Emu(rom); e.setRender(render); seedHook(e); e.run(40); e.tap('START', 1, 1, 0); e.run(4); e.tap('START', 1, 1, 0); e.run(40); return e; }
// Le hasard du jeu (tirage au sort, trajectoires de l'ordinateur) dépend des
// trames : la moindre trame de plus dans un menu changeait les matchs des
// tests. La graine est donc fixée à l'appui qui lance le match (A sur la
// présentation ou l'image du tournoi, START sur les réglages) : chaque
// match des tests se déroule toujours de la même façon.
const SEED = Number(process.env.SEED ?? 0x5A3C);
function seedHook(e) {
  const down = e.down.bind(e), run = e.run.bind(e);
  let pending = false;
  e.down = (btn, pad = 1) => {
    const st = e.peek('scr_state');
    if (pad === 1 && e.peek('app_state') === 0 && ((btn === 'A' && (st === 3 || st === 10)) || (btn === 'START' && st === 1))) pending = true;
    down(btn, pad);
  };
  // (après la trame où l'appui est lu : il y mêle l'instant de l'appui)
  e.run = (n = 1, each) => {
    if (!pending) return run(n, each);
    run(1, each); e.poke('rand_seed', SEED & 255, 0); e.poke('rand_seed', SEED >> 8, 1); pending = false;
    if (n > 1) return run(n - 1, each);
  };
}
const scr = (e) => e.peek('scr_state');
const app = (e) => e.peek('app_state');
// (attend le titre, puis l'écran des réglages : la durée des transitions varie)
function toMenu(e) {
  for (let i = 0; i < 30 && e.peek('scr_state') !== 0; i++) e.run(1);
  e.tap('START');
  // (10 trames comme avant si l'écran est prêt : le hasard du tirage au sort dépend des trames)
  let n = 0;
  for (; n < 60 && e.peek('scr_state') !== 1; n++) e.run(1);
  e.run(Math.max(10 - n, 2));
}
// ligne du menu où se trouve une option (la liste dépend du mode)
// rangée de la ligne i des réglages (liste centrée, version 2.60)
const menuY = (e, i) => 4 + ((18 - e.peek('nvis')) >> 1) + i;
function rowOf(e, o) { for (let i = 0; i < e.peek('nvis'); i++) if (e.peek('vis', i) === o) return i; return -1; }
function setOpts(e, opts) { for (const [k, v] of Object.entries(opts)) e.poke('opt_val', v, O[k]); }
// depuis le titre : configure et lance (présentation -> A si nécessaire)
function startWith(e, opts) {
  toMenu(e); setOpts(e, opts); e.tap('START');
  // certains écrans (classements) se construisent en plusieurs trames
  for (let n = 0; n < 90 && app(e) === AS.SCREENS && scr(e) === SS.MENU; n++) e.run(1);
  // (2.75) compétition : écran du joueur personnalisé, passé avec START ;
  // (2.76) pages de choix (joueurs, lieu, surface), passées avec START
  passCustom(e);
  // bande-annonce du Grand Martinot (passée par le banc d'essai), puis l'annonce
  if (app(e) === AS.SCREENS && scr(e) === 11) runUntil(e, () => scr(e) !== 11, 120);
  e.run(10);
  // le court se charge sur quelques trames (fondu, décor, copie des tribunes)
  if (app(e) === AS.SCREENS && scr(e) === SS.PRESENT) { e.tap('A'); runUntil(e, () => app(e) === AS.MATCH, 60); e.run(2); }
}
function passCustom(e) {
  // (Emu.customSkip appuie sur START à chaque page : on attend la fin)
  // (2.76) puis les pages de choix illustrées (Emu.pickSkip : START aussi)
  for (let n = 0; n < 300 && app(e) === AS.SCREENS && (scr(e) === SS.CUSTOM || scr(e) === SS.MENU || scr(e) === SS.PICK); n++) e.run(1);
}
// (2.76) pages de choix : depuis les réglages, START puis A jusqu'à la page
// dont le titre (rangées 0 à 2) contient « mot » ; START du banc coupé
function toPick(e, mot) {
  e.pickSkip = false; e.run(1);   /* (pk_off remis à 0 avant l'appui) */
  e.tap('START');
  runUntil(e, () => scr(e) === SS.PICK, 120); e.run(40);
  for (let k = 0; k < 6; k++) {
    if ([0, 1, 2].some(y => e.text(0, y, 32).includes(mot))) return true;
    e.tap('A'); e.run(40);
  }
  return false;
}
function allCpu(e) { const c = e.addr('cfg'); for (let i = 0; i < 4; i++) e.nes.cpu.mem[c + CFG.CTRL + i] = 2; }
// Attentes à plafond adaptatif : chaque appel (repéré par sa ligne) retient
// le nombre de trames qu'il a pris quand il a abouti (until_frames.json) ; son
// plafond devient le double de ce maximum habituel (600 trames au moins, et
// jamais plus que le plafond écrit dans le test). Une attente qui ne vient pas
// échoue donc en quelques secondes au lieu de dérouler 10 à 30 minutes de jeu.
// UNTIL_FULL=1 : plafonds écrits dans le test (après un changement du jeu qui
// allonge une séquence, pour réapprendre les durées).
const UNTIL_FILE = new URL('./until_frames.json', import.meta.url);
let untilKnown = {};
try { untilKnown = JSON.parse(fs.readFileSync(UNTIL_FILE, 'utf8')); } catch { untilKnown = {}; }
const untilSeen = {};
// (clé : le texte de la ligne de l'appel, et non son numéro, qui change dès
// qu'on ajoute un test plus haut)
const SRC_LINES = fs.readFileSync(new URL(import.meta.url), 'utf8').split('\n');
function callSite() {
  const l = (new Error().stack || '').split('\n').find(s => s.includes('emu_test.mjs') && !s.includes('runUntil') && !s.includes('callSite'));
  const m = l && l.match(/emu_test\.mjs:(\d+)/);
  return m ? SRC_LINES[Number(m[1]) - 1].trim().slice(0, 160) : '?';
}
function runUntil(e, pred, max, each) {
  const site = callSite(), known = untilKnown[site];
  const cap = (!process.env.UNTIL_FULL && known !== undefined) ? Math.min(max, Math.max(600, known * 2)) : max;
  let n = 0;
  while (!pred() && n < cap) { if (each) each(); e.run(1); n++; }
  if (n < cap) { if (!(untilSeen[site] >= n)) untilSeen[site] = n; return true; }
  if (cap < max) console.log(`  (attente arrêtée à ${cap} trames, d'habitude ${known} au plus : ${site.slice(0, 70)})`);
  return false;
}
function untilSave() {
  if (!Object.keys(untilSeen).length) return;
  let cur = {};
  try { cur = JSON.parse(fs.readFileSync(UNTIL_FILE, 'utf8')); } catch { cur = {}; }
  for (const [k, v] of Object.entries(untilSeen)) if (!(cur[k] >= v)) cur[k] = v;
  const tmp = UNTIL_FILE.pathname + '.' + process.pid;
  fs.writeFileSync(tmp, JSON.stringify(cur, null, 0)); fs.renameSync(tmp, UNTIL_FILE.pathname);
}
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

// utilitaires partagés par plusieurs sections
function fullMatch(label, opts, maxFrames, extra) {
  const e = boot(); startWith(e, opts); allCpu(e);
  if (extra) extra(e);
  let lastMs = '', prevMs = -1, same = 0, worst = 0, spGood = true, replays = 0;
  const ok = runUntil(e, () => {
    const m = e.peek('ms');
    if (m === 5 && prevMs !== 5) replays++;
    prevMs = m;
    // blocage = ni l'état du match, ni la balle, ni les joueurs ne changent
    // (pendant le ralenti et sa mise en valeur, l'image bouge : sprites et temps de la séquence)
    const sig = m + ',' + e.peek16('bx') + ',' + e.peek16('by') + ',' + e.peek16('plx') + ',' + e.peek16('plx', 2)
      + (m === 5 ? ',' + e.peek('hl_t') + ',' + e.nes.cpu.mem[0x200] + ',' + e.nes.cpu.mem[0x203] + ',' + e.nes.cpu.mem[0x204] : '');
    if (sig === lastMs) { same++; worst = Math.max(worst, same); } else { same = 0; lastMs = sig; }
    // (pendant la construction d'un écran, la pile C contient l'argument de
    // screens_enter : contrôle seulement pendant le match)
    // (ni pendant un gros plan ou un zoom du ralenti : leurs constructions durent plusieurs trames)
    if ((e.frames & 255) === 0 && app(e) === AS.MATCH && !e.peek('hl_mode') && !spOk(e)) spGood = false;
    return app(e) === AS.SCREENS;
  }, maxFrames);
  e.run(60);   // l'écran de résultats se construit sur quelques trames
  const pts = e.peek('st_points', 0) + e.peek('st_points', 1);
  check(ok && scr(e) === SS.RESULTS, `${label} : match terminé en ${(e.frames / 3600).toFixed(1)} min (${pts} points)`);
  const g = [e.peek('sc_hist', 0), e.peek('sc_hist', 1)];
  check(e.peek('sc_over') === 1 && Math.max(...g) >= e.peek('cfg', CFG.GAMES) && Math.abs(g[0] - g[1]) >= 1,
    `${label} : score final valide (${g[0]}-${g[1]})`);
  check(worst < 600 && spGood && spOk(e), `${label} : ni blocage (immobilité max ${worst} trames) ni pile corrompue`);
  return { e, replays };
}

const SURF = ['terre', 'gazon', 'dur', 'moquette', 'plage', 'glace'];

const ENV = ['stade', 'club', 'salle'];

const COND = ['jour', 'nuit', 'pluie'];

let totalReplays = 0;

const EXN = ['frappes', 'déplacements', 'régularité', 'précision', 'volée', 'service', 'joueur fixe', 'personnalisé', 'tutoriel'];

const litScr = (e) => e.peek('ppu_mask') !== 0 && e.peek('fade_lvl') === 0;

const skipIntro = (e) => {
  runUntil(e, () => litScr(e), 90);
  if (scr(e) === 10) { e.tap('A'); runUntil(e, () => scr(e) !== 10 && litScr(e), 120); }
};

// de l'écran d'étape au match : présentation, A, puis (premier tour d'un
// tournoi du Grand Martinot) l'image du tournoi, juste avant le match, A.
// Renvoie la fanfare et les textes de l'image (0 / '' sans image).
// jusqu'à l'affiche VS ; avant une finale, le survol du stade (2.73) est passé avec A
function toPresent(e, max) {
  runUntil(e, () => scr(e) === SS.PRESENT || scr(e) === 12, max);
  if (scr(e) === 12) { e.run(10); e.tap('A'); runUntil(e, () => scr(e) === SS.PRESENT, 120); }
}
function toMatch(e) {
  const row = (y) => e.text(0, y, 32).replace(/\0/g, ' ').trim();
  e.tap('A'); runUntil(e, () => scr(e) === SS.PRESENT || scr(e) === 12, 60);
  if (scr(e) === 12) { e.run(10); e.tap('A'); runUntil(e, () => scr(e) === SS.PRESENT, 120); }   /* finale : survol du stade (2.73), A passe */
  e.run(3); e.tap('A');
  runUntil(e, () => app(e) === AS.MATCH || (scr(e) === 10 && litScr(e)), 120);
  const ann = { mus: 0, name: '', rank: '', head: '', tiles: 0 };
  if (app(e) !== AS.MATCH && scr(e) === 10) {
    const rom2 = e.nes.rom.rom[2];
    for (let i = 0x600; i < 0x1000; i++) if (e.nes.ppu.vramMem[i] !== rom2[i]) ann.tiles++;
    Object.assign(ann, { mus: e.peek('music_cur'), name: row(4), rank: row(23), head: row(2) });
    e.tap('A'); runUntil(e, () => app(e) === AS.MATCH, 120);
  }
  e.run(30);
  return ann;
}
function playComp(type, opts, wins) {
  const e = boot(); startWith(e, Object.assign({ MODE: MODE.COMP, COMP: type, FMT: 3 }, opts));
  skipIntro(e);
  let matches = 0, code = '';
  const CN = ['TOURNOI', 'POULES', 'CHAMPIONNAT', 'SAISON', 'PERSONNALISE', 'GRAND MARTINOT', 'MARTINWORLD'][type];
  check(scr(e) === SS.COMP && e.peek('c_type') === type && e.text(3, 5, 3).trim() !== '' && e.nes.ppu.vramMem[0x2000 + 32 + 16] !== 0, `${CN} : écran d'étape (titre lisible)`);
  while (scr(e) === SS.COMP && matches < 24) {
    if (matches === 1) code = e.text(10, 22, 15).replace(/ /g, '');
    toMatch(e);
    if (app(e) !== AS.MATCH) break;
    // victoire forcée du joueur pour parcourir la compétition
    e.poke('sc_winner', wins(matches) ? 0 : 1); e.poke('sc_over', 1); e.poke('ms', 4); e.poke('hl_fin', 1); e.poke('ms_timer', 2);
    runUntil(e, () => app(e) === AS.SCREENS, 60); e.run(20);
    matches++;
    e.tap('A'); e.run(40);
    skipIntro(e);
  }
  return { e, matches, code };
}

const oamTiles = (e) => { const t = new Set(); for (let i = 0; i < 64; i++) { const y = e.peek('oam_buf', i * 4); if (y < 0xEF) t.add(e.peek('oam_buf', i * 4 + 1)); } return t; };

const msgText = (e) => { let t = ''; for (let i = 0; i < 14; i++) t += String.fromCharCode(e.peek('msg1', i)); return t.trim(); };

if (sec('Générique de lancement')) {
console.log('Générique de lancement');
{
  const e = new Emu(rom); e.run(100);
  const tx = (x, y, n) => e.text(x, y, n).replace(/\0/g, ' ');
  check(e.peek('app_state') === 3 && e.peek('music_cur') === 9 && tx(9, 11, 14) === 'BRUNO MARTINOT', 'générique : BRUNO MARTINOT PRESENTS, fanfare');
  let seen = new Set();
  const ok = runUntil(e, () => {
    if (tx(7, 12, 17) === 'AUGUSTIN MARTINOT') seen.add('augustin');
    if (tx(13, 14, 3) === '117') seen.add('117 %');
    if (tx(10, 7, 11) === 'MAGNIFIQUE.') seen.add('critique');
    if (tx(6, 10, 20) === 'RATED A FOR AUGUSTIN') seen.add('mentions');
    if (tx(13, 16, 6) === '*CLAP*') seen.add('clap');
    if (tx(10, 26, 12) === 'OOPS. SORRY.') seen.add('stagiaire');
    if (tx(6, 26, 19) === '(PRESTIGE RESTORED)') seen.add('restauré');
    return e.peek('app_state') === 0;
  }, 2400);
  e.run(30);
  check(ok && seen.size === 7 && e.peek('scr_state') === 0 && e.text(10, 16, 5) === 'JOUER', `générique complet (${[...seen].join(', ')}) puis écran titre`);
  // le N de MARTINOT revient bien à sa place (colonne 18) : même tuile que dans le logo de l'écran titre
  check(e.nes.ppu.vramMem[0x2000 + 4 * 32 + 18] === e.peek('logo_tiles', 11 * 4), 'logo complet après le générique');
  // bouton « passer » vexé : demande de confirmation, puis « FINE. » au second appui
  const f = new Emu(rom); f.run(60); f.tap('B', 1, 1, 2);
  const asked = f.text(5, 26, 21) === 'ARE YOU SURE? THIS IS' && f.peek('app_state') === 3;
  f.tap('A', 1, 1, 0);
  let n = 0; while (f.peek('app_state') !== 0 && n < 40) { f.run(1); n++; }
  check(asked && f.peek('app_state') === 0 && n < 25, `passer : « ARE YOU SURE? » puis « FINE. » (${n + 1} trames)`);
}

{
  // fanfare du générique : chaque note commence exactement à sa trame (la
  // musique avance dans la NMI, même pendant la construction des scènes)
  const e = new Emu(rom), m = e.nes.cpu.mem, mm = e.nes.mmap, w = mm.write.bind(mm);
  let fr = 0; const on = { 0x4006: [], 0x400b: [] }; const vol = new Set(); let w7 = 0;
  mm.write = (a, v) => { if (a === 0x4006 || a === 0x400b) on[a].push(fr); if (a === 0x4004) vol.add(v); if (a === 0x4007) w7++; return w(a, v); };
  for (; fr < 1300; fr++) e.run(1);
  // partitions : banque 10 (copiées en RAM de cartouche par music_play)
  const b10 = e.nes.rom.rom[10];
  const score = (sym) => { let a = e.addr(sym) - 0x8000, out = [], t = 0; while (b10[a] !== 0xFF) { if (b10[a] !== 0xFE) out.push(t); t += b10[a + 1]; a += 2; } return out; };
  let worst = 0, cnt = 0;
  for (const [reg, sym] of [[0x4006, 'mus_intro_mel'], [0x400b, 'mus_intro_bass']]) {
    const got = on[reg], exp = score(sym);
    cnt += Math.min(got.length, exp.length) === exp.length ? 0 : 1;
    for (let i = 0; i < exp.length && i < got.length; i++) worst = Math.max(worst, Math.abs(got[i] - got[0] - exp[i]));
  }
  // notes tenues : volume constant (rien ne s'éteint avant la note suivante),
  // et chaque note est relancée de la même façon ($4007 à chaque note)
  const held = [...vol].every((v) => v === 0x30 || (v & 0x30) === 0x30);
  check(held && w7 === score('mus_intro_mel').length, `générique : notes tenues sans coupure, chaque note relancée de la même façon (volumes ${[...vol].map((v) => v.toString(16))}, ${w7} relances pour ${score('mus_intro_mel').length} notes)`);
  check(cnt === 0 && worst === 0, `générique : les ${score('mus_intro_mel').length} notes de la mélodie et ${score('mus_intro_bass').length} de la basse tombent à la trame près (écart max ${worst})`);
}

{
  // aucune écriture de palette pendant que l'image se dessine (rendu coupé,
  // la NES afficherait la couleur écrite : traits de couleur aux transitions)
  const e = new Emu(rom), ppu = e.nes.ppu, mm = e.nes.mmap, w = mm.regWrite.bind(mm), ld = mm.load.bind(mm);
  let latch = 0, hi = 0, adr = 0, bad = 0;
  mm.load = (a) => { if (a === 0x2002) latch = 0; return ld(a); };
  mm.regWrite = (a, v) => {
    if (a === 0x2006) { if (!latch) { hi = v; latch = 1; } else { adr = ((hi & 0x3F) << 8) | v; latch = 0; } }
    if (a === 0x2007) { if (adr >= 0x3F00 && ppu.scanline >= 21 && ppu.scanline <= 260) bad++; adr = (adr + 1) & 0x3FFF; }
    return w(a, v);
  };
  e.run(1300); e.tap('START'); e.run(40); e.tap('DOWN'); e.run(3);
  for (let i = 0; i < 6; i++) { e.tap('RIGHT'); e.run(12); }
  e.tap('UP'); e.run(3); e.tap('START'); e.run(100); e.tap('A'); e.run(200);
  check(bad === 0, `générique, menus, transitions : palette toujours écrite hors de l'affichage (${bad} écriture(s) fautive(s))`);
}

}
if (sec('Démarrage et cartouche MMC3')) {
console.log('Démarrage et cartouche MMC3');
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
  // cartouche MMC3 (version 2.62) : en-tête, et banque de 16 Ko n = blocs de 8 Ko 2n ($8000) et 2n + 1 ($A000)
  const fs = await import('fs'); const hdr = fs.readFileSync(rom).subarray(0, 8);
  check(hdr[4] === 32 && hdr[5] === 0 && ((hdr[6] >> 4) | (hdr[7] & 0xF0)) === 4 && (hdr[6] & 2) === 2, `en-tête : MMC3 (mapper 4), 512 Ko, CHR-RAM, sauvegarde (${[...hdr].map(b => b.toString(16)).join(' ')})`);
  let okb = true;
  for (const b of [1, 4, 9, 13, 20, 30]) {
    e.nes.cpu.mem[e.addr('cur_bank')] = 0;
    // set_bank(b) appelé comme par le jeu : on écrit les registres comme lui
    e.nes.mmap.write(0x8000, 6); e.nes.mmap.write(0x8001, b * 2); e.nes.mmap.write(0x8000, 7); e.nes.mmap.write(0x8001, b * 2 + 1);
    const bank = e.nes.rom.rom[b];
    for (const off of [0x0000, 0x1FF0, 0x2000, 0x3F00]) if (e.nes.mmap.load(0x8000 + off) !== bank[off]) okb = false;
  }
  check(okb, 'banques de 16 Ko en deux blocs de 8 Ko ($8000, $A000)');
}

}
if (sec("Mode d'emploi")) {
console.log("Mode d'emploi");
{
  const e = boot();
  e.tap('DOWN'); e.tap('A'); e.run(10);
  check(app(e) === AS.MANUAL && e.peek('cur_bank') === 4, "le mode d'emploi s'ouvre (banque 4)");
  check(e.text(2, 3, 9) === 'COMMANDES', 'page 1 : commandes');
  for (let i = 0; i < 12; i++) { e.tap('RIGHT'); e.run(5); }
  check(e.peek('page') === 12 && e.text(2, 3, 8) === 'CONSEILS', '13 pages consultables');
  e.tap('RIGHT'); e.run(5);
  check(e.peek('page') === 0, 'navigation circulaire');
  e.tap('B'); e.run(10);
  check(app(e) === AS.SCREENS && scr(e) === SS.TITLE, 'B revient au titre');
}

}
if (sec('Menu de configuration')) {
console.log('Menu de configuration');
{
  const e = boot(); toMenu(e);
  check(scr(e) === SS.MENU && e.peek('nvis') === 9, 'configuration match : 9 lignes (pas de ligne JEU contre le CPU ; joueurs, lieu, surface et météo sur des pages, 2.76-2.77)');
  e.tap('DOWN'); e.tap('RIGHT'); e.tap('RIGHT'); e.tap('RIGHT'); e.run(5);
  check(e.peek('opt_val', O.MODE) === MODE.COMP && e.peek('nvis') === 13, 'mode compétition : lignes adaptées (avec CARRIERE, 2.72 ; JOUEUR 2, 2.76)');
  e.tap('RIGHT'); e.run(5);
  check(e.peek('opt_val', O.MODE) === MODE.TRAIN && e.peek('nvis') === 6, 'mode entraînement : lignes adaptées');
  e.tap('B'); e.run(10);   // (les tuiles des personnages, remplacées par les portraits, sont rechargées)
  check(scr(e) === SS.TITLE, 'B revient au titre');
}

}
if (sec('Mode en équipe (double)')) {
console.log('Mode en équipe (double)');
{
  const e = boot(); toMenu(e); setOpts(e, { MODE: MODE.TEAM, TYPE: 0, CPART: 1 }); e.tap('B'); e.run(5); toMenu(e);
  check(e.peek('nvis') === 10 && rowOf(e, O.TYPE) < 0 && e.text(3, menuY(e, 2), 8) === 'JOUEUR 2', 'pas de ligne JEU ; ligne JOUEUR 2 (le partenaire : sur la page des joueurs, 2.76)');
  e.tap('START'); runUntil(e, () => scr(e) === SS.PRESENT && litScr(e), 90);
  check(e.text(3, 1, 15) === 'MATCH DE DOUBLE' && e.text(2, 22, 11) === '+ MANETTE 2', 'toujours un double, manette 2 partenaire');
  e.tap('A'); e.run(30);
  const c = e.addr('cfg');
  check(e.nes.cpu.mem[c + 1] === 1 && e.nes.cpu.mem[c + 2] === 0 && e.nes.cpu.mem[c + 4] === 1 && e.nes.cpu.mem[c + 3] === 2,
    'manettes 1 et 2 dans la même équipe face à deux joueurs CPU');
  check(e.nes.cpu.mem[c + 8] === e.peek('opt_val', O.P2), 'PARTENAIRE = personnage de la manette 2');
}
{
  const e = boot(); toMenu(e); setOpts(e, { MODE: MODE.TEAM, CPART: 0 }); e.tap('START'); runUntil(e, () => scr(e) === SS.PRESENT && litScr(e), 90); e.tap('A'); e.run(30);
  const c = e.addr('cfg');
  check(e.nes.cpu.mem[c + 1] === 1 && e.nes.cpu.mem[c + 2] === 0 && e.nes.cpu.mem[c + 4] === 2,
    'partenaire joué par l\'ordinateur (JOUE PAR : CPU)');
}

}
if (sec('Court Margaux')) {
console.log('Court Margaux');
{
  const e = boot(); toMenu(e); setOpts(e, { ENV: 0 });
  toPick(e, 'LIEU');
  const seen = new Set(); for (let i = 0; i < 6; i++) { e.tap('RIGHT'); e.run(30); seen.add(e.peek('opt_val', O.ENV)); }
  check(seen.has(4) && !seen.has(3), 'court Margaux proposé d\'emblée (club nudiste toujours verrouillé)');
  for (let k = 0; k < 10 && e.peek('opt_val', O.ENV) !== 4; k++) { e.tap('RIGHT'); e.run(30); }   /* (borné : jamais de boucle sans fin) */
  e.tap('START'); runUntil(e, () => scr(e) === SS.PRESENT && litScr(e), 120);
  check(e.text(2, 3, 26) === 'BIENVENUE AU COURT MARGAUX', 'présentation du court Margaux');
  e.tap('A'); e.run(30);
  // petit panneau dans le bandeau, à droite du score de gauche (rangées 2-3, colonnes 5 à 8) :
  // ses 8 tuiles, chargées en CHR-RAM à la place du public du club nudiste ; zone des annonces libre
  const nt = (x, y) => e.nes.ppu.vramMem[0x2000 + y * 32 + x];
  // données en banque 3 : lues directement dans la ROM (16 Ko par banque)
  const rb = (sym, off) => e.nes.rom.rom[3][e.addr(sym) - 0x8000 + off];
  const idx = [...Array(8).keys()].map((k) => rb('sign_idx', k));
  const placed = idx.every((t, k) => nt(5 + (k % 4), 2 + (k >> 2)) === t);
  let same = true; for (let b = 0; b < 128; b++) if (e.nes.ppu.vramMem[idx[b >> 4] * 16 + (b & 15)] !== rb('sign_chr', b)) same = false;
  check(placed && same && !/[A-Z]/.test(e.text(9, 3, 14)) && e.nes.cpu.mem[e.addr('pal_buf') + 1] === 0x25,
    'court rose avec le petit panneau MARGAUX à côté du score');
  allCpu(e); e.poke('crowd_timer', 60); e.run(70);
  check(e.peek('crowd_timer') === 0, 'coeurs, fleurs et étoiles scintillent puis se reposent');
}

}
if (sec('Version 2.71 : bandeau du score, crépuscule, voix, record')) {
console.log('Version 2.71 : bandeau du score, crépuscule, voix, record');
{
  // bandeau du score (MMC3 : bas de l'écran 1 dès la ligne 200) et voix de
  // l'arbitre ; crépuscule (accentuation des couleurs au fil des jeux)
  const e = boot(); startWith(e, { FMT: 0, COND: 3 }); allCpu(e);
  const apu = e.nes.papu, orig = apu.writeReg.bind(apu), said = [];
  let la = -1;
  apu.writeReg = (a, v) => { if (a === 0x4012) la = v; if (a === 0x4015 && (v & 0x10) && la >= 0) said.push(la); return orig(a, v); };
  const names = {}; ['out', 'fault', 'let', 'pok', 'deuce', 'game', 'adv'].forEach((n, i) => names[e.peek('dpcm_addr', i)] = n);
  const vm = e.nes.ppu.vramMem;
  let shown = 0, okText = 0, okSpr = 0, badServe = 0, pm = -1, forced = 0, prevBv = false;
  const emph = new Set(), stages = new Set();
  for (let f = 0; f < 60 * 60 * 20 && app(e) === AS.MATCH; f++) {
    if (e.peek('cer') >= 1 && e.peek('cer') <= 3) e.poke('cer_skip', 1);
    // égalité forcée de temps en temps (annonces « Deuce! », « Advantage! »)
    const m = e.peek('ms');
    if (m === 0 && pm !== 0 && !e.peek('sc_tiebreak') && forced < 6 && (f & 1)) { e.poke('sc_points', 3, 0); e.poke('sc_points', 3, 1); forced++; }
    pm = m;
    e.run(1);
    if (!e.peek('hl_mode') && e.peek('ppu_mask')) { stages.add(e.peek('sun_st')); emph.add(e.peek('sun_st') * 256 + e.peek('ppu_emph')); }   /* (gros plan : sans accentuation) */
    const bv = e.peek('band_vis') === 3 && !e.peek('hl_mode') && e.peek('ms') !== 5;   /* (gros plan : band_vis n'est plus tenu à jour) */
    const steady = bv && prevBv; prevBv = bv;
    if (steady) {   /* (l'OAM envoyée au PPU est celle de la trame précédente) */
      shown++;
      const row = String.fromCharCode(...vm.slice(0x2400 + 26 * 32 + 4, 0x2400 + 26 * 32 + 8));
      if (/^[A-Z][A-Z ]{3}$/.test(row)) okText++;
      let low = 0; for (let i = 0; i < 64; i++) { const y = e.nes.ppu.spriteMem[i * 4]; if (y >= 192 && y < 240) low++; }   /* (OAM envoyée au PPU)*/
      if (!low) okSpr++; else if (process.env.DBG) { const l = []; for (let i = 0; i < 64; i++) { const y = e.nes.ppu.spriteMem[i * 4]; if (y >= 192 && y < 240) l.push(y + ':' + e.nes.ppu.spriteMem[i * 4 + 1].toString(16)); } console.log('spr', f, e.peek('cer'), e.peek('ms'), e.peek('ms_timer'), l.join(' ')); }
      if (e.peek('ms') === 0 && e.peek('ms_timer') === 0 && e.peek('cer') === 0) badServe++;
    }
  }
  check(shown > 100 && okText === shown && okSpr === shown, `bandeau du score entre les points (${shown} trames, texte ${okText}, sprites ${okSpr}) : noms et score dans l'écran 1, aucun sprite dessous`);
  check(badServe === 0, `bandeau caché quand le serveur est prêt (${badServe})`);
  const seq = said.map(a => names[a] ?? '?').filter(n => n !== 'pok');
  check(['deuce', 'adv', 'game'].every(n => seq.includes(n)), `annonces de l'arbitre : ${['deuce', 'adv', 'game'].map(n => n + ' ' + seq.filter(x => x === n).length).join(', ')}`);
  const red = 0x20;
  const okEmph = [...emph].every(v => { const st = v >> 8, em = v & 255; return em === (st === 1 ? red : st === 2 ? (red | 0x80) : 0); });
  check(stages.has(1) && stages.has(2) && okEmph, `crépuscule : paliers ${[...stages].sort().join(',')}, accentuation rouge puis rouge et bleu, projecteurs (${[...emph].map(v => (v >> 8) + ':' + (v & 255).toString(16)).join(' ')})`);
  runUntil(e, () => scr(e) === SS.RESULTS, 600);
  check(e.peek('ppu_emph') === 0 && e.peek('band_vis') === 0 || scr(e) === SS.RESULTS && e.peek('ppu_emph') === 0, `hors du court : pas d'accentuation (${e.peek('ppu_emph')})`);
}
for (const view of [0, 1]) {
  // (2.72) aucune écriture dans la VRAM pendant l'affichage : en 2.71, la fin
  // de chaque jeu réécrivait le bandeau et la tuile de son filet écran
  // allumé, et abîmait des tuiles de la police et du court
  const e = boot(); startWith(e, { FMT: 0, COND: 3, VIEW: view, REPLAY: 0 }); allCpu(e);
  const vm = e.nes.ppu.vramMem, font = () => Array.from(vm.slice(0x300, 0x3A0)).join(',');   /* chiffres 0 à 9 */
  let ref = null, bad = 0, games = 0, pg = 0;
  for (let f = 0; f < 60 * 60 * 8 && app(e) === AS.MATCH; f++) {
    e.poke('cer_skip', e.peek('cer') >= 1 && e.peek('cer') <= 3 ? 1 : 0);
    e.run(1);
    const g = e.peek('sc_games', 0) + e.peek('sc_games', 1); if (g !== pg) { games++; pg = g; }
    if (e.peek('hl_mode')) continue;
    const s = font();
    if (ref === null) ref = s; else if (s !== ref) bad++;
  }
  check(games >= 4 && bad === 0, `${view ? 'perspective' : 'à plat'} : chiffres de la police intacts pendant tout le match (${games} jeux, ${bad} trames abîmées)`);
}
{
  // record du service le plus rapide (palmarès), « KM/H : RECORD ! »
  const e = boot(); startWith(e, { FMT: 3 });
  for (let f = 0; f < 60 * 60 * 15 && app(e) === AS.MATCH; f++) {
    if (e.peek('ms') === 0 && e.peek('sc_server') === 0) { e.tap('A'); e.run(14); e.tap('A'); }
    if (e.peek('cer') >= 4) e.poke('cer_skip', 1);
    if (e.peek('hl_mode') === 3) e.tap('A');
    e.run(1);
  }
  runUntil(e, () => scr(e) === SS.RESULTS, 600); e.run(20);
  const v = e.peek('st_vmax', 0) + 80, rec = e.peek('sw_spd', 1) + 80;
  check(e.peek('sp_new') === 1 && rec === v && e.text(2, 19, 16).startsWith('KM/H : RECORD'), `record de vitesse au palmarès (${rec} km/h), annoncé aux résultats (« ${e.text(2, 19, 16).trim()} »)`);
  // (2.75) échange le plus long : ligne à part, sous le tableau
  const L = e.peek('st_longest'), lg = [20, 21, 22].map(r => e.text(8, r, 16).replace(/\0/g, ' ').trim()).join(' / ');
  check(lg === `PLUS LONG / ECHANGE / ${L} FRAPPE${L > 1 ? 'S' : ''}`, `résultats : « ${lg} », à part, entre les deux joueurs`);
}
}
if (sec('Version 2.72 : carrière, fiche, reprise, cérémonie')) {
console.log('Version 2.72 : carrière, fiche, reprise, cérémonie');
{
  // blocs sauvegardés : mêmes adresses qu'en 2.71 (sinon les palmarès et
  // réglages des cartouches existantes seraient perdus)
  const e = boot();
  const want = { sv: 0x7800, sw_best: 0x7851, so: 0x7855, sw_view: 0x787B, sw_spd: 0x787E };   /* (sx : nom aussi pris par une variable statique ; entre so et sw_view) */
  const bad = Object.entries(want).filter(([k, a]) => e.addr(k) !== a).map(([k]) => k);
  check(bad.length === 0 && e.addr('sw_h2h') > 0x787E && e.addr('sw_car') > 0x787E, `sauvegardes : adresses des versions précédentes conservées${bad.length ? ' (' + bad.join(', ') + ')' : ''}`);
}
{
  // fiche des joueurs sur l'affiche VS (alterne avec les textes d'origine)
  const e = boot(); toMenu(e); setOpts(e, { MODE: 0 }); e.tap('START');
  runUntil(e, () => scr(e) === SS.PRESENT, 120); e.run(100);
  const a = e.text(2, 21, 9), b0 = e.text(2, 24, 12);
  e.run(160);
  const h = e.text(2, 20, 6), m = e.text(2, 21, 8), f = e.text(2, 24, 19);
  check(a === 'MANETTE 1' && /^\d,\d\d M$/.test(h) && /^(DROITIERE?|GAUCHERE?)/.test(m.trim()) && (f.startsWith('FACE A FACE') || f === 'PREMIER FACE A FACE'),
    `fiche : « ${a} » puis « ${h} / ${m.trim()} », « ${f.trim()} »`);
}
{
  // carrière : tournoi gagné = points, classement, ligne au palmarès ; bonus en match
  const { e } = playComp(0, { CAREER: 1 }, () => true);
  const pts = e.peek('sw_car', 1) | (e.peek('sw_car', 2) << 8);
  check(pts === 400 && e.peek('sw_car', 3) === 1 && e.peek('sw_car', 4) === 1 && e.peek('sw_car', 5) === 60, `carrière : tournoi gagné, ${pts} points, ${e.peek('sw_car', 5)}e`);
  const f = boot(); toMenu(f); setOpts(f, { MODE: MODE.COMP, COMP: 0, CAREER: 1 }); f.tap('B'); f.run(30); toMenu(f); f.run(10);
  check(rowOf(f, 25) > 0, 'carrière : ligne CARRIERE dans les réglages d\'une compétition en simple');
  f.poke('sw_car', 0x43, 0); f.poke('sw_car', 2000 & 255, 1); f.poke('sw_car', 2000 >> 8, 2);
  let c = 0x3B; for (let i = 0; i < 11; i++) c = (((c << 1) | (c >> 7)) & 255) ^ f.peek('sw_car', i); f.poke('sw_car', c, 11);
  f.tap('START'); f.run(60); skipIntro(f); toMatch(f);
  check(f.peek('car_srv') === 4 && f.peek('car_spd') === 4 && f.peek('car_acc') === 1, `carrière : 2 000 points, service +${f.peek('car_srv')}, vitesse +${f.peek('car_spd')}, précision ${f.peek('car_acc')}`);
}
{
  // reprise d'un match après extinction (RAM de cartouche gardée par la pile)
  const e = boot(); startWith(e, { FMT: 1, REPLAY: 0 }); allCpu(e);
  runUntil(e, () => e.peek('sc_games', 0) + e.peek('sc_games', 1) >= 3 && e.peek('ms') === 0, 60 * 60 * 10, () => e.poke('cer_skip', e.peek('cer') >= 1 && e.peek('cer') <= 3 ? 1 : 0));
  e.run(30);
  const snap = (x) => ['sc_points', 'sc_games', 'sc_sets', 'st_points'].map(n => x.peek(n, 0) + '-' + x.peek(n, 1)).join(' ') + ' ' + x.peek('sc_server') + x.peek('sc_swap');
  const before = snap(e), sram = e.nes.cpu.mem.slice(0x6000, 0x8000);
  const f = new Emu(rom); for (let i = 0; i < 0x2000; i++) f.nes.cpu.mem[0x6000 + i] = sram[i];
  f.run(40); f.tap('START', 1, 1, 0); f.run(4); f.tap('START', 1, 1, 0); f.run(40);
  runUntil(f, () => scr(f) === SS.TITLE, 200); f.run(30);
  const t = f.text(4, 25, 22);
  f.tap('B'); f.run(60); const pr = scr(f);
  f.tap('A'); runUntil(f, () => app(f) === AS.MATCH, 300); f.run(20);
  check(t === 'B : REPRENDRE LE MATCH' && pr === SS.PRESENT && snap(f) === before && !f.peek('cer'), `reprise du match : « ${t} », même score (${snap(f)})`);
  // match fini : plus de reprise
  f.poke('sc_winner', 0); f.poke('sc_over', 1); f.poke('ms', 4); f.poke('hl_fin', 1); f.poke('ms_timer', 2);
  runUntil(f, () => app(f) === AS.SCREENS, 120); f.run(30);
  check(f.peek('pod_buf', 0) !== 0x4D, 'match terminé : la sauvegarde est effacée');
}
{
  // changement de côté : le bandeau montre le score puis 5 statistiques (FAUTES : 2.76)
  const e = boot(); startWith(e, { FMT: 0, REPLAY: 0 }); allCpu(e);
  const want = { 1: '1ERE', 2: 'ACES', 3: 'COUP', 4: 'FAUT', 5: 'SERV' }, seen = new Set();
  for (let f = 0; f < 60 * 60 * 10 && seen.size < 5 && app(e) === AS.MATCH; f++) {
    e.poke('cer_skip', e.peek('cer') >= 1 && e.peek('cer') <= 3 ? 1 : 0);
    e.run(1);
    const pg = e.peek('band_pg');
    if (e.peek('cer') === 5 && e.peek('band_vis') === 3 && pg && String.fromCharCode(...e.nes.ppu.vramMem.slice(0x2400 + 27 * 32 + 2, 0x2400 + 27 * 32 + 6)) === want[pg]) seen.add(pg);
  }
  check(seen.size === 5, `changement de côté : le bandeau montre les 5 statistiques (${[...seen].sort().join(',')})`);
  // (2.75) avec un joueur humain : elles restent affichées et défilent à l'appui ;
  // les joueurs ne repartent qu'après la dernière page
  const h = boot(); h.bandSkip = false; startWith(h, { FMT: 3, REPLAY: 0 }); allCpu(h);
  h.cerSkip = false; h.poke('cer_skip', 0); h.poke('sc_games', 2); h.poke('sc_points', 3); h.poke('sc_points', 0, 1);
  runUntil(h, () => h.peek('cer') === 5, 60 * 60 * 3);
  h.poke('cfg', 0, CFG.CTRL);                      /* (un joueur prend la manette) */
  h.run(400);
  const stay = h.peek('cer') === 5 && h.peek('band_pg') === 0;
  const pages = [];
  for (let k = 0; k < 5; k++) { h.tap('A'); pages.push(h.peek('band_pg')); }
  const still = h.peek('cer') === 5;
  h.tap('A'); h.run(4);
  check(stay && pages.join() === '1,2,3,4,5' && still && h.peek('cer') === 6, `changement de côté : statistiques affichées jusqu'à l'appui, une page par appui (${pages}), puis retour des joueurs (${h.peek('cer')})`);
}
{
  // plongeon de dernière chance sur un service canon
  const e = boot(); startWith(e, { FMT: 2, REPLAY: 1 });
  const cfga = e.addr('cfg'); for (let i = 1; i < 4; i++) { e.nes.cpu.mem[cfga + 2 + i] = 2; e.nes.cpu.mem[cfga + 10 + i] = 2; }
  let dives = 0, pd = 0, close = 0;
  for (let f = 0; f < 60 * 60 * 20 && !close && app(e) === AS.MATCH; f++) {
    e.poke('cer_skip', e.peek('cer') >= 1 && e.peek('cer') <= 3 ? 1 : 0);
    const ms = e.peek('ms');
    if (e.peek('sc_server') === 0 && ms === 0 && !e.peek('cer')) { e.down('A'); e.run(1); e.up('A'); }
    if (e.peek('sc_server') === 0 && ms === 1) { const v = e.peekS16('bvz'); if (v < 8 && v > -8) { e.down('A'); e.down(f & 2 ? 'LEFT' : 'RIGHT'); } else { e.up('A'); e.up('LEFT'); e.up('RIGHT'); } } else if (ms !== 0) { e.up('A'); e.up('LEFT'); e.up('RIGHT'); }
    if (e.peek('srv_canon') && ms === 2 && e.peek('bounces') === 0 && e.peek('b_serve')) {
      const bxp = e.peek16('bx') >> 6, x = Math.max(20, Math.min(230, bxp + (bxp < 128 ? 34 : -34)));   /* (34 px : il faut plonger, avec une marge pour la dérive de la balle) */
      e.poke('plx', (x << 6) & 255, 2); e.poke('plx', (x << 6) >> 8, 3);
    }
    /* (2.74) retour en plongeon réussi : le joueur 1 est écarté, le plongeon
       gagne le point (sans dépendre du hasard du match) */
    if (e.peek('dive_canon') && ms === 2 && e.peek('hitter') === 1 && !e.peek('b_serve')) { e.poke('plx', 0, 0); e.poke('plx', (20 << 6) >> 8, 1); }
    e.run(1);
    const d = e.peek('dive_canon'); if (d && !pd) dives++; pd = d;
    if (e.peek('hl_mode') === 1 && e.peek('hl_sk') === 4) close = 1;
  }
  check(dives > 0 && close, `service canon : plongeons de dernière chance (${dives}), gros plan du plongeon gagnant`);
}
{
  // remise du trophée : tribunes, public debout, plaque du finaliste
  const { e } = playComp(0, {}, () => true);
  while (e.peek16('pod_t') < 330) e.run(1);
  const nt = e.nes.ppu.vramMem;
  const fans = nt[0x2000 + 12 * 32] >= 0xB1 && nt[0x2000 + 12 * 32] <= 0xB2 && nt[0x2000 + 12 * 32 + 31] >= 0xB1;
  check(fans && e.peek('oam_buf', 249) === 0x3C && e.peek('oam_buf', 248) < 0xF0, `podium : tribunes, plaque du finaliste (${fans}, ${e.peek('oam_buf', 249).toString(16)}, ${e.peek('oam_buf', 248)}, coupe ${e.peek('cup_t')}, t ${e.peek16('pod_t')}, écran ${scr(e)})`);
}
{
  // mode d'emploi illustré
  const e = boot(); runUntil(e, () => scr(e) === SS.TITLE, 100); e.run(30);
  e.tap('DOWN'); e.tap('A'); e.run(60);
  let ok = 0;
  for (let p = 0; p < 13; p++) { e.run(20); let n = 0; for (let i = 0; i < 64; i++) if (e.peek('oam_buf', i * 4) < 0xF0) n++; if (n >= 12) ok++; e.tap('RIGHT'); e.run(10); }
  check(ok === 13, `mode d'emploi : deux joueurs animés sur le mini-court de chaque page (${ok}/13)`);
}
}
if (sec('Version 2.73 : bandes du ralenti, survol, bandeau, dehors de peu')) {
console.log('Version 2.73 : bandes du ralenti, survol, bandeau, dehors de peu');
{
  // bandes « cinéma » du ralenti (écran 1, rangées 0-1 et 28-29) ; bandeau
  // du score jusqu'à l'appui ; haut de l'écran sans les jeux ; zoom rapide
  // sur une balle dehors de peu
  const e = boot(); startWith(e, { FMT: 2, REPLAY: 1 }); allCpu(e);
  const vm = e.nes.ppu.vramMem;
  let endF = -1, minDelay = 999, rise = new Set(), q4 = 0, q4ok = 0, rpF = 0, bars = 0, quick = 0, quickIn = 0, quickLen = 0, pm = 0, start = 0, modes = '', bandServe = 0, hud = '', early = 0;
  for (let f = 0; f < 60 * 60 * 12 && app(e) === AS.MATCH; f++) {
    e.poke('cer_skip', e.peek('cer') >= 1 && e.peek('cer') <= 3 ? 1 : 0);
    e.run(1);
    const m = e.peek('ms'), hm = e.peek('hl_mode');
    if (m === 5 && !hm) { rpF++; if (e.peek('split_on') === 3 || e.peek('lb_st')) bars++; }   /* (2.74 : plus de bandes noires) */
    if (m === 3 && e.peek('rp_pending') && e.peek('band_vis')) early++;                      /* (2.74 : bandeau après le ralenti) */
    if (hm === 2 && e.peek('hl_quick') && e.peek('hl_t') === 1 && e.peek('hl_sbin')) quickIn++;
    if (hm === 2 && e.peek('hl_quick') && e.peek('hl_c') === 36) { q4++; if (e.peek('ctrl_swap') === 0 && e.peek('scroll_nt') === 1 && e.peek('hl_n') === 0) q4ok++; }   /* (2.74 : second temps x 4) */
    if (m === 5 && pm !== 5) { start = f; modes = ''; }
    if (m === 5 && hm && !modes.endsWith(String(hm))) modes += hm;
    if (m !== 5 && pm === 5 && modes === '2') { quick++; quickLen = Math.max(quickLen, f - start); }
    if (m === 0 && !e.peek('cer') && e.peek('band_vis') === 3) bandServe++;
    if (m === 3 && pm !== 3 && pm !== 5 && !e.peek('rp_pending')) endF = f;   /* (2.76 : après un ralenti, le bandeau vient sans délai : non compté)                        /* (2.74 : délai, puis montée) */
    if (endF >= 0 && (e.peek('hl_mode') || e.peek('rp_pending'))) endF = -1;           /* (2.76 : après un ralenti ou un zoom, pas de délai) */
    if (endF >= 0 && e.peek('band_vis') === 3 && !e.peek('cer')) { minDelay = Math.min(minDelay, f - endF); endF = -1; }
    if (e.peek('band_vis') === 3 && !e.peek('cer')) rise.add(e.peek('split_l'));
    if (!hud && m === 0 && f > 600) hud = e.text(1, 2, 4);
    pm = m;
  }
  check(rpF > 50 && bars === 0, `ralenti sans bandes noires (${rpF} trames, ${bars} avec bandes)`);
  check(early === 0, `bandeau du score après le ralenti seulement (${early} trames avant)`);
  check(quick > 0 && quickLen < 290, `balle limite : zoom seul, rapide (${quick} fois dont ${quickIn} bonnes sur la ligne, ${quickLen} trames au plus)`);
  check(q4 > 0 && q4ok === q4, `balle limite : second temps x 4 sur l'impact (${q4ok}/${q4})`);
  check(bandServe > 0, `bandeau du score pendant la mise en place du service (${bandServe} trames)`);
  check(minDelay >= 48 && minDelay < 120 && rise.size >= 8 && Math.max(...rise) > 230 && Math.min(...rise) === 199, `bandeau : apparaît ${minDelay} trames après le point, en montant (lignes ${Math.max(...rise)} à ${Math.min(...rise)})`);
  check(!hud.startsWith('J') && /^( 0|15|30|40|AV| -)/.test(hud), `haut de l'écran : les points, sans les jeux (« ${hud} »)`);
  // (2.74) le bandeau ne part qu'à l'appui d'un joueur humain : l'ordinateur
  // attend pour servir ; l'appui ferme le bandeau sans lancer la balle
  const g = boot(); g.bandSkip = false; startWith(g, { FMT: 3 });
  let humOk = null, cpuWait = null, cpuGo = null;
  for (let f = 0; f < 60 * 60 * 6 && (humOk === null || cpuWait === null) && app(g) === AS.MATCH; f++) {
    g.poke('cer_skip', g.peek('cer') >= 1 && g.peek('cer') <= 3 ? 1 : 0);
    g.run(1);
    if (g.peek('cer') === 5) { g.tap('A'); continue; }   /* (2.75 : assis, les statistiques défilent à l'appui) */
    if (g.peek('ms') === 0 && !g.peek('cer') && g.peek('band_dis') && g.peek('sc_server') === 0) { g.tap('A'); g.run(14); g.tap('A'); continue; }   /* (bandeau fermé : le joueur sert) */
    if (g.peek('ms') !== 0 || g.peek('cer') || g.peek('band_vis') !== 3) continue;
    if (g.peek('sc_server') === 1) {           /* l'ordinateur sert */
      if (cpuWait !== null) { g.tap('B'); continue; }
      g.run(300); cpuWait = g.peek('ms') === 0 && g.peek('band_vis') === 3;
      g.tap('B'); g.run(4); const gone = g.peek('band_vis') === 0;
      cpuGo = gone && runUntil(g, () => g.peek('ms') !== 0, 200);
    } else {                                   /* le joueur sert */
      g.tap('A'); g.run(4);
      if (humOk === null) humOk = g.peek('ms') === 0 && g.peek('band_vis') === 0;
    }
  }
  check(cpuWait === true && cpuGo === true, `bandeau : l'ordinateur attend l'appui pour servir (attente ${cpuWait}, service après l'appui ${cpuGo})`);
  check(humOk === true, `bandeau : l'appui du serveur ferme le bandeau sans lancer la balle (${humOk})`);
}
{
  // (2.74) public : drapeaux aux couleurs des joueurs au changement de côté ;
  // standing ovation (toutes les tribunes debout) après un très long échange
  const e = boot(); startWith(e, { FMT: 3, REPLAY: 0 }); allCpu(e);
  e.cerSkip = false; e.poke('cer_skip', 0); e.poke('sc_games', 2); e.poke('sc_points', 3); e.poke('sc_points', 0, 1);
  const ft = e.peek('flag_tile');
  let flags = 0;
  for (let f = 0; f < 60 * 60 * 3 && !flags; f++) {
    e.run(1);
    if (e.peek('cer') === 5 && e.peek('cer_t') === 60) { for (let i = 0; i < 64; i++) if (e.peek('oam_buf', i * 4 + 1) === ft && e.peek('oam_buf', i * 4) < 0xEF) flags++; }
  }
  check(flags === 6, `changement de côté : 6 drapeaux des supporters dans les tribunes (${flags})`);
  const o = boot(); startWith(o, { FMT: 2, REPLAY: 0 }); allCpu(o);
  let seq = '', up = 0;
  for (let f = 0; f < 60 * 60 * 6 && !seq.endsWith('2340'); f++) {
    if (f === 600) o.poke('rally_hits', 12);
    o.run(1);
    const v = o.peek('ola_on'); if (!seq.endsWith(String(v))) seq += v;
    if (v === 3 && !up) { const vm = o.nes.ppu.vramMem; for (let r = 4; r < 30; r++) for (const c of [0, 1, 2, 3, 28, 29, 30, 31]) { const t = vm[0x2000 + r * 32 + c]; if (t >= 9 && t <= 12) up++; } }
  }
  check(seq.includes('2340') && up > 100, `standing ovation : les tribunes se lèvent, restent debout, se rassoient (${seq}, ${up} spectateurs debout)`);
}
{
  // (2.75) coup selon la hauteur de la balle : avant le rebond, smash au-dessus
  // de 24 px, volée en dessous ; après le rebond, smash au-dessus de 30 px
  const e = boot(); startWith(e, { FMT: 2, REPLAY: 0 }); allCpu(e);
  let bad = [], hi = 0;
  for (let f = 0; f < 60 * 60 * 6; f++) {
    const z = e.peekS16('bz') >> 6, b = e.peek('bounces'), h0 = e.peek('hitter'), inc = e.peek('last_shot');
    e.run(1);
    if (e.peek('hitter') !== h0 && e.peek('ms') === 2 && !e.peek('b_serve')) {
      // (après le rebond, B garde le lob ou l'amortie demandés ; 2.76 : smash
      // après le rebond seulement sur un lob reçu)
      const ls = e.peek('last_shot'), smash = ls === 7, want = z > (b ? 30 : 24) && !(b && (ls === 2 || ls === 3)) && !(b && inc !== 2);
      if (want) hi++;
      if (smash !== want && Math.abs(z - (b ? 30 : 24)) > 2) bad.push(`${b ? 'après' : 'avant'} rebond ${z} px : ${e.peek('last_shot')}`);
    }
  }
  check(hi > 0 && bad.length === 0, `coup selon la hauteur : smash sur balle haute avant ou après le rebond (${hi}), volée ou coup normal plus bas${bad.length ? ' — ' + bad.slice(0, 3).join(', ') : ''}`);
}
{
  // (2.74) lob : après son rebond, il reste frappable (62 px de haut au plus)
  // jusqu'à ce qu'il quitte l'écran : il se reprend aussi juste avant le
  // second rebond (avant : 90 à 110 px, hors d'atteinte et hors de l'écran)
  const e = boot(); startWith(e, { FMT: 2, REPLAY: 0 }); allCpu(e);
  let n = 0, zmax = 0, seen = false;
  for (let f = 0; f < 60 * 60 * 12 && n < 4 && app(e) === AS.MATCH; f++) {
    e.run(1);
    if (e.peek('ms') === 2 && e.peek('last_shot') === 2 && e.peek('bounces') <= 1) {
      for (let q = 0; q < e.peek('npl'); q++) if ((q & 1) !== e.peek('hitter')) e.poke('plx', 0, q * 2), e.poke('plx', 48 >> 2, q * 2 + 1);   /* (receveur écarté : la balle va au bout de sa course) */
      const y = e.peekS16('by') >> 6;
      if (e.peek('bounces') === 1 && y > 8 && y < 240) { zmax = Math.max(zmax, e.peekS16('bz') >> 6); seen = true; }
    } else if (seen) { n++; seen = false; }
  }
  check(n >= 2 && zmax <= 62, `lob : après le rebond, toujours à portée (${n} lobs, ${zmax} px de haut au plus)`);
}
{
  // perspective : la tuile $5F (vide, bord du court) n'est plus prise par le
  // filet du bandeau (traits horizontaux entre le public et le court, 2.71-2.72)
  const e = boot(); startWith(e, { FMT: 3, VIEW: 1 }); allCpu(e); e.run(600);
  const vm = e.nes.ppu.vramMem;
  check(vm.slice(0x5F0, 0x600).every(v => v === 0) && vm.slice(0xFF0, 0xFF8).some(v => v), 'perspective : tuile $5F vide (bord du court), filet du bandeau sur $FF');
}
{
  // survol du stade avant une finale : 3 bandes à des vitesses différentes
  const e = boot(); startWith(e, { MODE: MODE.COMP, COMP: 0, FMT: 3 }); skipIntro(e);
  let m = 0;
  while (scr(e) === SS.COMP && m < 2) {
    toMatch(e);
    e.poke('sc_winner', 0); e.poke('sc_over', 1); e.poke('ms', 4); e.poke('hl_fin', 1); e.poke('ms_timer', 2);
    runUntil(e, () => app(e) === AS.SCREENS, 60); e.run(20); m++;
    e.tap('A'); e.run(40); skipIntro(e);
  }
  e.tap('A'); e.run(5);
  const fly = scr(e) === 12;
  runUntil(e, () => e.peek16('fly_t') >= 120, 300);
  const sky = e.peek('scroll_x'), st = e.peek('fly_x', 0), ct = e.peek('fly_x', 1);
  runUntil(e, () => scr(e) === SS.PRESENT, 400);
  check(fly && sky < st && st < ct && scr(e) === SS.PRESENT, `finale : survol du stade (ciel ${sky}, tribunes ${st}, court ${ct}), puis l'affiche`);
}
}
if (sec('Ralenti : gros plan et zoom')) {
console.log('Ralenti : gros plan et zoom');
{
  // version 2.67 : pendant le ralenti, gros plan de la frappe décisive puis
  // arrêt sur image zoomé (x 2) au rebond ; retour au court à l'identique
  const court = (e) => { const vm = e.nes.ppu.vramMem, o = []; for (let ty = 4; ty < 30; ty++) for (let tx = 6; tx < 26; tx++) { const t = vm[0x2000 + ty * 32 + tx]; o.push(t, ...vm.slice(t * 16, t * 16 + 16)); } return o.join(','); };
  for (const view of [0, 1]) {
    const e = boot(); startWith(e, { FMT: 3, VIEW: view }); allCpu(e);
    let ref = null, seq = [], zoomOk = false, closeOk = false, back = [], f;
    for (f = 0; f < 60 * 60 * 10 && (back.length < 2 || !seq.join().includes('1,2')); f++) {   // (2.77 : la séquence gros plan + zoom, où qu'elle soit ; un zoom rapide peut venir avant)
      const m0 = e.peek('hl_mode');
      e.run(1);
      const m = e.peek('hl_mode');
      if (e.peek('ms') === 5 && !m && !m0 && ref === null) ref = court(e);
      if (m && !m0) seq.push(m);
      if (m === 1 && e.peek('hl_t') === 40) closeOk = e.nes.ppu.vramMem[0x2000 + 15 * 32 + 16] !== 0 && e.peek('ctrl_swap') === 0;
      if (m === 2 && e.peek('hl_t') === 40) {
        const used = new Set(e.nes.ppu.vramMem.slice(0x2000, 0x23C0));
        zoomOk = e.peek('ctrl_swap') === 0x18 && used.size > 4 && used.size < 256;
      }
      if (!m && m0) { e.run(1); back.push(court(e) === ref && e.peek('ctrl_swap') === 0); }
      if (e.peek('ms') !== 5) ref = null;
    }
    check(seq.join().includes('1,2') && closeOk && zoomOk, `${view ? 'perspective' : 'à plat'} : gros plan de la frappe décisive, puis zoom x 2 au rebond (${seq.join(',')})`);
    check(back.length >= 2 && back.every(Boolean), `${view ? 'perspective' : 'à plat'} : retour au court à l'identique après chaque séquence (${back.join(',')})`);
  }
  // A pendant le gros plan : la séquence et le ralenti s'arrêtent
  const g = boot(); startWith(g, { FMT: 3 }); allCpu(g);
  runUntil(g, () => g.peek('hl_mode') === 1 && g.peek('hl_t') === 30, 60 * 60 * 6);
  g.tap('A'); runUntil(g, () => g.peek('hl_mode') === 0, 120); g.run(10);   /* (retour au court : une quinzaine de trames de rechargement) */
  check(g.peek('hl_mode') === 0 && g.peek('ctrl_swap') === 0 && g.peek('ms') !== 5, `A pendant le gros plan : retour au court, fin du ralenti (ms ${g.peek('ms')}, mode ${g.peek('hl_mode')}, t ${g.peek('hl_t')})`);
  // fin d'un vrai double (2.69) : portrait du vainqueur en pied, puis gros plan
  // de son partenaire, « VICTOIRE ! », puis les résultats
  {
    const d = boot(); startWith(d, { MODE: 1, TYPE: 1, FMT: 3, REPLAY: 0 }); allCpu(d);
    runUntil(d, () => d.peek('hl_mode') === 3, 60 * 60 * 30, () => { if (d.peek('cer') >= 4) d.poke('cer_skip', 1); });
    const w = d.peek('sc_winner'), np = d.peek('npl');
    runUntil(d, () => d.peek('hl_t') === 40, 100); const a = d.peek('hl_sp');
    const tx = []; for (let i = 0; i < 15 && d.peek('hl_txt', i); i++) tx.push(d.peek('hl_txt', i));
    runUntil(d, () => d.peek('hl_t') === 180, 200); const b = d.peek('hl_sp');
    runUntil(d, () => scr(d) === SS.RESULTS, 400);
    check(np === 4 && a === w && b === w + 2 && String.fromCharCode(...tx) === 'VICTOIRE !' && scr(d) === SS.RESULTS,
      `double : portrait du vainqueur, puis gros plan de son partenaire (« ${String.fromCharCode(...tx)} »), puis les résultats`);
  }
  // club nudiste : ni gros plan ni zoom
  const n = boot(); startWith(n, { FMT: 3, ENV: 3 }); allCpu(n);
  let rep = 0, hl = 0, pm = 0;
  for (let f = 0; f < 60 * 60 * 4; f++) { n.run(1); const m = n.peek('ms'); if (m === 5 && pm !== 5) rep++; if (n.peek('hl_mode')) hl++; pm = m; }
  check(rep > 0 && hl === 0, `club nudiste : ralentis (${rep}) sans gros plan ni zoom`);
  // réglage RALENTIS « OUI, SIMPLE » (2.69) : ralentis sans gros plan ni zoom
  const q = boot(); startWith(q, { FMT: 3, REPLAY: 2 }); allCpu(q);
  let rq = 0, hq = 0, zq = 0, pq = 0;
  for (let f = 0; f < 60 * 60 * 4; f++) { q.run(1); const m = q.peek('ms'); if (m === 5 && pq !== 5) rq++; if (q.peek('hl_mode') === 1) hq++; if (q.peek('hl_mode') === 2) zq++; pq = m; }
  check(rq > 0 && hq === 0 && zq > 0, `RALENTIS « OUI, SIMPLE » : ralentis (${rq}) avec zoom (${zq} trames), sans gros plan`);
  // plans télé HD (2.73) : ramasseuse au changement de côté (un sur deux), public après un long échange
  {
    const k = boot(); startWith(k, { FMT: 3, REPLAY: 1 }); allCpu(k);
    k.cerSkip = false; k.poke('cer_skip', 0); k.poke('sc_games', 2); k.poke('sc_points', 3); k.poke('sc_points', 0, 1);
    let kid = false, pal = false, txt = '', back = false;
    for (let f = 0; f < 60 * 60 * 3 && !back; f++) {
      k.run(1);
      if (k.peek('cer') === 5 && k.peek('cer_t') < 240) k.poke('cer_t', 245);
      if (k.peek('hl_mode') === 4) {
        kid = true;
        if (k.peek('hl_t') === 60) { pal = k.peek('pal_buf', 5) === 0x36 && k.peek('pal_buf', 9) === 0x2B; const a = []; for (let i = 0; i < 15 && k.peek('hl_txt', i); i++) a.push(k.peek('hl_txt', i)); txt = String.fromCharCode(...a); }
      } else if (kid) { k.run(20); back = k.peek('cer') === 6 && k.peek('ctrl_swap') === 0 && k.peek('scroll_x') === 0 && app(k) === AS.MATCH; }
    }
    check(kid && pal && txt === 'LES BALLES' && back, `ramasseuse au changement de côté (« ${txt} », tenue verte ${pal}), puis retour au court (${back})`);
    const c = boot(); startWith(c, { FMT: 3, REPLAY: 1 }); allCpu(c);
    runUntil(c, () => c.peek('hl_mode') === 1, 60 * 60 * 15); c.poke('rally_hits', 22);   /* (20 frappes au moins depuis la 2.75) */
    runUntil(c, () => c.peek('hl_t') === 90, 200);
    const cp = c.peek('pal_buf', 5) === 0x36 && c.peek('pal_buf', 9) === 0x2B;
    runUntil(c, () => c.peek('hl_mode') === 0, 200);
    check(cp && c.peek('ctrl_swap') === 0, `long échange : le public debout à la place du gros plan de joie (${cp})`);
  }
  // ace (2.70) : vitesse du service affichée façon télé, « ACE ! / xxx KM/H »
  {
    const a = boot(); startWith(a, { FMT: 3 });
    const tx = (s) => String.fromCharCode(...Array.from({ length: 14 }, (_, i) => a.peek(s, i)).filter(v => v)).trim();
    let m2 = '';
    for (let f = 0; f < 60 * 60 * 3 && !m2; f++) {
      if (a.peek('ms') === 0 && a.peek('sc_server') === 0) { a.tap('A'); a.run(14); a.tap('A'); }
      if (a.peek('ms') === 2) { a.poke('plx', 0, 2); a.poke('plx', 5, 3); }   /* receveur tenu loin de la balle */
      a.run(1);
      if (tx('msg1') === 'ACE !') m2 = tx('msg2');
    }
    const k = parseInt(m2, 10), vm = a.peek('st_vmax', 0) + 80;
    check(/^\d{3} KM\/H$/.test(m2) && k >= 100 && k <= 260, `ace : vitesse du service affichée (« ${m2} »)`);
    check(vm >= k && vm <= 260, `service le plus rapide du match retenu pour les statistiques (${vm} km/h)`);
  }
}
}
if (sec('Présentation et pause')) {
console.log('Présentation et pause');
{
  const e = boot(); toMenu(e); e.tap('START'); e.run(10);
  check(scr(e) === SS.PRESENT && e.text(3, 1, 12) === 'MATCH AMICAL', "présentation avant le match");
  e.tap('A'); e.run(30);
  check(app(e) === AS.MATCH && e.peek('cur_bank') === 0, 'A lance le match (banque 0)');
  e.tap('START');
  check(e.peek('paused') === 1, 'START ouvre le menu de pause');
  const x = e.peek16('plx'); e.run(30);
  check(e.peek16('plx') === x, 'le jeu est figé en pause');
  e.tap('DOWN'); e.tap('A'); e.run(10);
  check(app(e) === AS.SCREENS && scr(e) === SS.MENU, 'QUITTER ramène au menu');
}
{
  // affiche VS (2.63) : glissade des bustes par l'interruption par ligne de la MMC3
  const e = boot(true); toMenu(e); e.tap('START');   /* (cette vérification lit l'image) */
  for (let i = 0; i < 80 && !(e.peek('split_on') && e.peek('split_o') > 20 && e.peek('split_o') < 60); i++) e.run(1);
  e.run(1);
  const fb = e.frameBuf, bg = fb[100 * 256 + 128];
  const cnt = (y, x0, x1) => { let n = 0; for (let x = x0; x < x1; x++) if (fb[y * 256 + x] !== bg) n++; return n; };
  let ev = 0, od = 0, evR = 0, odR = 0;
  for (let y = 32; y < 144; y += 2) { ev += cnt(y, 8, 100); od += cnt(y + 1, 8, 100); evR += cnt(y, 156, 248); odR += cnt(y + 1, 156, 248); }
  check(e.peek('split_on') === 1 && ev > 300 && od === 0 && odR > 300 && evR === 0,   // (bustes en cours d'entrée : couleurs sombres proches du fond selon le personnage)
    `glissade : buste gauche sur les lignes paires, droit sur les impaires (${ev}/${od}, ${odR}/${evR})`);
  const os = new Set(); for (let i = 0; i < 40; i++) { e.run(1); if (e.peek('split_on')) os.add(e.peek('split_o')); }
  check([...os].every((o) => o % 8 === 0) && os.size >= 4, `glissade par pas de 8 px (décalage fin toujours nul : rien ne bouge en cours de ligne) : ${[...os].join(' ')}`);
  let gap = 0; for (let y = 40; y < 140; y++) if (cnt(y, 0, 100) === 0 && cnt(y + 1, 0, 100) > 0 && cnt(y - 1, 0, 100) > 0) gap++;
  check(e.peek('split_on') === 0 && e.nes.cpu.F_INTERRUPT === 1 && gap === 0, `fin de l'entrée : interruption coupée, bustes pleins (${gap} lignes vides)`);
  const f = boot(); toMenu(f); f.tap('START'); f.run(12); f.tap('B'); f.run(40);
  check(scr(f) === SS.MENU && f.peek('split_on') === 0 && f.nes.cpu.F_INTERRUPT === 1, 'B pendant la glissade : retour au menu, interruption coupée');
}

}
if (sec('Service : lancer visible pour le joueur du haut')) {
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

}
if (sec('Balle ratée : elle sort de l\'écran')) {
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

}
if (sec('Filet effleuré au service (let)')) {
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


}
if (sec('Matchs complets CPU contre CPU (format rapide)')) {
console.log('Matchs complets CPU contre CPU (format rapide)');
for (let s = 0; s < 6; s++) {
  const r = fullMatch(`simple ${SURF[s]}/${ENV[s % 3]}/${COND[(s + 1) % 3]}`,
    { FMT: 3, SURF: s, ENV: s % 3, COND: (s + 1) % 3, LEVEL: s % 3, P2: (s + 4) % 10 }, 60 * 60 * 25);
  totalReplays += r.replays;
}
{
  const r = fullMatch('double dur/salle', { MODE: MODE.P2, FMT: 3, TYPE: 1, SURF: 2, ENV: 2 }, 60 * 60 * 25);
  totalReplays += r.replays;
}
check(totalReplays > 0, `ralentis déclenchés sur les beaux points (${totalReplays})`);

}
if (sec('Revanche et palmarès')) {
console.log('Revanche et palmarès');
{
  const e = boot(); startWith(e, { FMT: 3 });
  runUntil(e, () => app(e) === AS.SCREENS, 60 * 60 * 25, () => botStep(e));
  e.releaseAll(); e.run(20);
  check(scr(e) === SS.RESULTS, 'match humain (robot) terminé');
  const pl = e.peek16('sv', 4);
  check(pl === 1, 'palmarès : 1 match joué enregistré');
  e.tap('A'); runUntil(e, () => app(e) === AS.MATCH, 60);
  check(app(e) === AS.MATCH, 'A : revanche immédiate');
  allCpu(e);
  runUntil(e, () => app(e) === AS.SCREENS, 60 * 60 * 25); e.run(20);
  check(scr(e) === SS.RESULTS && spOk(e), 'second match enchaîné terminé');
  // chaque écran : attendre qu'il soit allumé avant d'appuyer (pendant un
  // fondu, la boucle principale ne lit pas les manettes)
  const lit = (st) => runUntil(e, () => scr(e) === st && e.peek('ppu_mask') !== 0 && e.peek('fade_lvl') === 0, 120);
  e.tap('B'); lit(SS.MENU); e.tap('B'); lit(SS.TITLE);
  e.tap('DOWN'); e.tap('DOWN'); e.tap('A'); lit(SS.PALMARES); e.run(2);
  check(scr(e) === SS.PALMARES && e.text(3, 6, 5) === 'JOUES', 'écran palmarès');
  e.tap('RIGHT'); e.run(8); e.tap('RIGHT'); e.run(8);
  check(e.text(3, 5, 12) === "RECORDS D'EN", "page des records d'entraînement");
}

}
if (sec('Entraînement : 9 exercices')) {
console.log('Entraînement : 9 exercices');
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

}
if (sec('Compétitions')) {
console.log('Compétitions');
// Grand Martinot : écran d'annonce d'un tournoi (état 10) avant son écran d'étape
{
  // le Grand Martinot : 4 tournois de 16 (huitièmes, quarts, demis, finale),
  // dur, terre battue, gazon, dur ; les 16 matchs gagnés : Grand Chelem
  const g = boot(); startWith(g, { MODE: MODE.COMP, COMP: 5, FMT: 3, ENV: 2, COND: 2 });
  // l'écran d'étape d'abord (plus d'image juste après la bande-annonce)
  runUntil(g, () => litScr(g), 90);
  check(scr(g) === SS.COMP && g.text(3, 5, 18) === "OPEN D'AUGUSTRALIE", "Grand Martinot : l'écran d'étape directement après la bande-annonce");
  check(g.peek('c_env') === 0 && g.peek('c_cond') === 0, 'Grand Martinot : toujours au stade, de jour (lieu et conditions imposés)');
  check(scr(g) === SS.COMP && g.peek('c_n') === 16 && g.text(3, 5, 18) === "OPEN D'AUGUSTRALIE" && g.text(3, 6, 18) === 'HUITIEME DE FINALE',
    `Grand Martinot : 16 joueurs, « ${g.text(3, 5, 18)} », ${g.text(3, 6, 18)}`);
  const names = new Set(); for (let i = 0; i < 16; i++) names.add(g.peek('c_pl', i));
  check(names.size === 16, `Grand Martinot : 16 joueurs différents`);
  // présentation du match, puis l'image du tournoi (fanfare), puis le match
  const ga = toMatch(g);
  check(ga.mus === 11 && ga.name === "OPEN D'AUGUSTRALIE" && ga.tiles > 500 && app(g) === AS.MATCH,
    `Grand Martinot : image de l'Open d'Augustralie (fanfare ${ga.mus}, ${ga.tiles} octets d'image) juste avant le match, puis le match`);
  {
    const h = boot(); startWith(h, { MODE: MODE.COMP, COMP: 5, FMT: 3 }); skipIntro(h);
    const seen = [];
    for (let m = 0; m < 5; m++) {
      seen.push(toMatch(h).mus);
      h.poke('sc_winner', 0); h.poke('sc_over', 1); h.poke('ms', 4); h.poke('hl_fin', 1); h.poke('ms_timer', 2);
      runUntil(h, () => app(h) === AS.SCREENS, 60); runUntil(h, () => scr(h) === SS.RESULTS && litScr(h), 120);
      h.tap('A'); runUntil(h, () => scr(h) !== SS.RESULTS && litScr(h), 120);
    }
    check(seen.join(',') === '11,0,0,0,12', `image au premier match de chaque tournoi seulement (Debogarros au 5e match : ${seen})`);
  }
  const { e, matches } = playComp(5, {}, () => true);
  for (let k = 0; k < 5 && scr(e) !== SS.COMP_END; k++) { e.tap('A'); e.run(60); }
  e.run(30);
  const podTxt = [...Array(30).keys()].map((y) => e.text(0, y, 32)).join('|');
  check(matches === 16 && scr(e) === SS.COMP_END && e.peek('c_rank') === 1 && e.peek('c_slam') === 1 && podTxt.includes('GRAND CHELEM !') && podTxt.includes('LE GRAND MARTINOT'),
    `Grand Martinot : 16 matchs gagnés, Grand Chelem et podium (${matches} matchs)`);
  // en double : 8 équipes ; une défaite ne termine que le tournoi en cours
  const r = playComp(5, { TYPE: 1, P2: 3 }, (m) => m !== 1);
  for (let k = 0; k < 5 && scr(r.e) !== SS.COMP_END; k++) { r.e.tap('A'); r.e.run(60); }   // (comme en simple : jusqu'à l'écran de fin)
  check(r.e.peek('c_n') === 8 && r.matches === 11 && r.e.peek('c_slam') === 0 && scr(r.e) === SS.COMP_END,
    `Grand Martinot en double : 8 équipes, défaite en demi au 1er tournoi puis 3 tournois (${r.matches} matchs), pas de Grand Chelem [c_n ${r.e.peek('c_n')}, slam ${r.e.peek('c_slam')}, écran ${scr(r.e)}, rang ${r.e.peek('c_rank')}]`);
  // menu : explication de la compétition sous le curseur
  const m = boot(); toMenu(m); setOpts(m, { MODE: MODE.COMP, COMP: 5 }); m.tap('B'); m.run(30); toMenu(m); m.run(10);
  m.poke('cursor', rowOf(m, O.COMP)); m.run(8);
  const h = m.text(2, 24, 28).trim(), h4 = m.text(2, 27, 28).trim();
  m.poke('cursor', rowOf(m, O.LEVEL)); m.run(8);
  check(rowOf(m, O.ENV) < 0 && rowOf(m, O.COND) < 0 && rowOf(m, O.SURF) < 0, 'menu du Grand Martinot : ni lieu, ni conditions, ni surface (imposés)');
  check(h.startsWith('4 TOURNOIS DE 16') && h4.includes('CHELEM') && m.text(2, 24, 28).trim() !== h && m.text(2, 26, 15) === 'HAUT/BAS: LIGNE',
    `menu : explication de la compétition (« ${h} »), puis aide de la ligne et touches`);
}
{
  // le Martinworld Tour : 7 tournois de 8 (quarts, demis, finale), un par surface
  const g = boot(); startWith(g, { MODE: MODE.COMP, COMP: 6, FMT: 3, ENV: 3, COND: 2, SURF: 4 });
  const row = (e, y) => e.text(0, y, 32).replace(/\0/g, ' ').trim();
  runUntil(g, () => litScr(g), 90);
  check(g.peek('c_env') === 0 && g.peek('c_cond') === 0 && g.peek('c_n') === 8 && g.peek('c_type') === 6,
    'Martinworld Tour : 8 joueurs, au stade, de jour');
  {
    const ga = toMatch(g);
    check(ga.mus === 11 && ga.name === "OPEN D'AUGUSTRALIE" && ga.head === 'MARTINWORLD TOUR' && ga.rank === 'TOURNOI 1/7 : DUR' && app(g) === AS.MATCH,
      `Martinworld Tour : image du 1er tournoi juste avant le match (« ${ga.rank} »)`);
  }
  // chaque tournoi : annonce (image, fanfare, rang), surface, nom ; une défaite au 1er tour à chaque fois
  const h = boot(); startWith(h, { MODE: MODE.COMP, COMP: 6, FMT: 3 });
  const seen = [], surfs = [], names = [], ranks = [];
  for (let m = 0; m < 7; m++) {
    const an = toMatch(h);
    seen.push(an.mus); if (an.mus) { ranks.push(an.rank); names.push(an.name); }
    surfs.push(h.peek('cfg', 14));
    h.poke('sc_winner', 1); h.poke('sc_over', 1); h.poke('ms', 4); h.poke('hl_fin', 1); h.poke('ms_timer', 2);
    runUntil(h, () => app(h) === AS.SCREENS, 60); runUntil(h, () => scr(h) === SS.RESULTS && litScr(h), 120);
    h.tap('A'); runUntil(h, () => scr(h) !== SS.RESULTS && litScr(h), 120);
  }
  check(seen.join(',') === '11,15,12,13,16,14,17', `Martinworld Tour : 7 annonces et 7 fanfares (${seen.join(',')})`);
  check(names.join('/') === "OPEN D'AUGUSTRALIE/MARGAUX-LES-NEIGES/DEBOGARROS/MARTINBLEDON/RIO DE BRUNEIRO/US OPAUL/DEBORCY MASTERS",
    `Martinworld Tour : ${names.join(', ')}`);
  check(ranks.join(',') === 'TOURNOI 1/7 : DUR,TOURNOI 2/7 : GLACE,TOURNOI 3/7 : TERRE BATTUE,TOURNOI 4/7 : GAZON,TOURNOI 5/7 : PLAGE,TOURNOI 6/7 : DUR,TOURNOI 7/7 : MOQUETTE',
    `rangs et surfaces annoncés (${ranks.join(', ')})`);
  check(surfs.join(',') === '2,5,0,1,4,2,3', `Martinworld Tour : toutes les surfaces (${surfs.join(',')})`);
  check(scr(h) === SS.COMP_END && h.peek('c_rank') > 1 && h.peek('c_slam') === 0, `7 défaites au 1er tour : fin, ${h.peek('c_rank')}e`);
  check(h.peek('sw_best', 0) === 66 && h.peek('sw_best', 2) === h.peek('c_rank'),
    `palmarès : meilleur rang au Martinworld Tour (${h.peek('sw_best', 2)}e), rangé à part`);
  // les 21 matchs gagnés : Grand Chelem, podium, 14 000 points
  const { e, matches, code } = playComp(6, {}, () => true);
  for (let k = 0; k < 5 && scr(e) !== SS.COMP_END; k++) { e.tap('A'); e.run(60); }
  e.run(30);
  const podTxt = [...Array(30).keys()].map((y) => e.text(0, y, 32)).join('|');
  check(matches === 21 && scr(e) === SS.COMP_END && e.peek('c_rank') === 1 && e.peek('c_slam') === 1 && podTxt.includes('GRAND CHELEM !') && podTxt.includes('MARTINWORLD TOUR'),
    `Martinworld Tour : 21 matchs gagnés, Grand Chelem et podium (${matches} matchs)`);
  check(/^[A-Z2-9]{13}$/.test(code), `Martinworld Tour : code de reprise (${code})`);
  // reprise par le code : 4 matchs gagnés (Margaux-les-Neiges, demi-finale)
  {
    const r = boot(); startWith(r, { MODE: MODE.COMP, COMP: 6, FMT: 3, P1: 16 }); skipIntro(r);
    for (let m = 0; m < 4; m++) {
      toMatch(r);
      r.poke('sc_winner', 0); r.poke('sc_over', 1); r.poke('ms', 4); r.poke('hl_fin', 1); r.poke('ms_timer', 2);
      runUntil(r, () => app(r) === AS.SCREENS, 60); r.run(20); r.tap('A'); r.run(40); skipIntro(r);
    }
    const rc = r.text(10, 22, 15).replace(/ /g, '');
    const want = ['c_type', 'c_stage', 'c_seed', 'c_n'].map(n => r.peek(n)).concat([r.peek16('c_gres')]);
    const f = boot(); f.tap('UP'); f.tap('A'); f.run(10);
    const AL = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
    for (let i = 0; i < 13; i++) {
      const n = AL.indexOf(rc[i]);
      for (let k = 0; k < n; k++) f.tap('UP', 1, 1, 1);
      if (i < 12) f.tap('RIGHT', 1, 1, 1);
    }
    f.tap('START'); f.run(40); skipIntro(f);
    const got = ['c_type', 'c_stage', 'c_seed', 'c_n'].map(n => f.peek(n)).concat([f.peek16('c_gres')]);
    check(scr(f) === SS.COMP && JSON.stringify(got) === JSON.stringify(want) && want[1] === 4 && want[4] === 3
      && f.text(3, 5, 18) === 'MARGAUX-LES-NEIGES' && f.peek('c_pl', 0) === 16,
      `Martinworld Tour : code ${rc} relu (Margaux-les-Neiges, demi-finale, ${JSON.stringify(got)})`);
  }
  // menu : explication ; ni lieu, ni conditions, ni surface
  const m = boot(); toMenu(m); setOpts(m, { MODE: MODE.COMP, COMP: 6 }); m.tap('B'); m.run(30); toMenu(m); m.run(10);
  m.poke('cursor', rowOf(m, O.COMP)); m.run(8);
  const t1 = m.text(2, 24, 28).trim(), t4 = m.text(2, 27, 28).trim();
  check(rowOf(m, O.ENV) < 0 && rowOf(m, O.COND) < 0 && rowOf(m, O.SURF) < 0 && t1.startsWith('7 TOURNOIS DE 8') && t4.includes('CHELEM'),
    `menu du Martinworld Tour : explication (« ${t1} ») ; lieu, conditions, surface imposés`);
}
{
  const { e, matches, code } = playComp(0, {}, () => true);
  check(matches === 3 && scr(e) === SS.COMP_END && e.peek('c_rank') === 1, `tournoi gagné en 3 matchs`);
  check(/^[A-Z2-9]{13}$/.test(code), `code de reprise affiché : ${code}`);
  check(e.peek('sv', 0x14 + 0) >= 0, 'palmarès mis à jour');
  // podium : 3 joueurs agrandis dans le décor, marches, noms, textes centrés
  const nt = (x, y) => e.nes.ppu.vramMem[0x2000 + y * 32 + x];
  const pod = [0, 1, 2].map((k) => e.peek('c_pod', k));
  check(pod[0] === 0 && pod[1] > 0 && pod[2] > 0 && pod[1] !== pod[2], `podium : 1er le joueur, 2e et 3e distincts (${pod})`);
  check(nt(14, 12) === 0x60 && nt(17, 17) === 0x60 + 23 && nt(8, 14) === 0x60 + 24 && nt(20, 16) === 0x60 + 48,
    'podium : 3 joueurs en taille double (24 tuiles chacun), 1er au centre, 2e à gauche, 3e à droite');
  check(nt(13, 18) === 0xA8 && nt(7, 20) === 0xA8 && nt(19, 22) === 0xA8 && nt(15, 19) === 0xAA && nt(9, 21) === 0xAB && nt(21, 23) === 0xAC,
    'podium : marches 1 (milieu, la plus haute), 2 (gauche) et 3 (droite), centrées');
  const nm = (c) => { const p = e.peek16('char_name', c * 2); let t = ''; for (let k = 0; k < 4; k++) t += String.fromCharCode(e.nes.cpu.mem[p + k]); return t; };
  const under = (x0) => e.text(x0, 26, 6).replace(/[\0 ]/g, '');   // nom sous une marche de 6 cases
  check(under(13) === nm(e.peek('c_pl', 0)).trim() && under(7) === nm(e.peek('c_pl', pod[1])).trim() && under(19) === nm(e.peek('c_pl', pod[2])).trim(),
    `podium : noms sous les marches (${e.text(7, 26, 18)})`);
  check(e.text(10, 4, 11) === 'VAINQUEUR !' && e.text(4, 7, 23) === 'MODE NUDISTE DEBLOQUE !', 'podium : « VAINQUEUR ! » et déblocage centrés');
  // big player : chaque tuile agrandie = tuile du personnage dont chaque pixel est doublé
  const chr = e.nes.ppu.vramMem, f0 = e.nes.cpu.mem;
  let okz = true;
  { const c = e.peek('c_pl', 0), fem = e.peek('char_female', c), base = e.peek(fem ? 'person_map_f' : 'person_map_m', e.peek('person_row', 1) + 6 * 3);
    for (let y = 0; y < 8; y++) { const s0 = chr[0x1000 + base * 16 + (y >> 1)], d0 = chr[0x60 * 16 + y];
      let exp = 0; for (let b = 0; b < 4; b++) if (s0 & (0x80 >> b)) exp |= 0xC0 >> (b * 2);
      if (d0 !== exp) okz = false; } }
  check(okz, 'podium : agrandissement exact (pixel doublé, sans décalage de lecture)');
  // (2.76 : l'écran du champion, puis la conférence de presse, entre le podium et le titre)
  for (let k = 0; k < 8 && !(scr(e) === SS.TITLE && e.peek('ppu_mask') !== 0); k++) { e.tap('A'); runUntil(e, () => scr(e) === SS.TITLE && e.peek('ppu_mask') !== 0, 200); }
  e.run(10);
  const rom2 = e.nes.rom.rom[2]; let diff = 0; for (let i = 0; i < 8192; i++) if (chr[i] !== rom2[i] && !(i >= 0x600 && i < 0xCE0)) diff++;   // ($60-$CD : tuiles des menus)
  check(scr(e) === SS.TITLE && diff === 0, `retour au titre : motifs d'origine rétablis (${diff} octet(s) différent(s))`);
}
{
  // Margaux vainqueur : sa chevelure agrandie, son nom en entier
  const { e } = playComp(0, { P1: 16 }, () => true);
  check(e.text(13, 26, 7) === 'MARGAUX' && e.nes.ppu.vramMem[0x2000 + 12 * 32 + 14] === 0x60, 'podium : Margaux vainqueur, « MARGAUX » en entier sous la marche');
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
  // saison gagnée : après le podium, le générique de fin défile
  e.run(60); e.tap('A');
  runUntil(e, () => app(e) === 4 && e.peek('ppu_mask') !== 0, 200);
  const both = () => { let t = ''; for (const b of [0x2000, 0x2800]) for (let i = 0; i < 960; i++) t += String.fromCharCode(e.nes.ppu.vramMem[b + i]); return t; };
  let seen = new Set(), scr0 = new Set();
  const want = ['SEASON CHAMPION', 'BRUNO MARTINOT', 'AUGUSTIN MARTINOT', 'THE INTERN', 'THE END', '(PROBABLY NOT)'];
  runUntil(e, () => { if ((e.peek('frame_cnt') & 15) === 0) { const t = both(); for (const w of want) if (t.includes(w)) seen.add(w); scr0.add(e.peek('scroll_nt')); } return e.peek('halt') === 1; }, 4000);
  check(app(e) === 4 && seen.size === want.length && scr0.has(0) && scr0.has(2),
    `générique de fin : défilement sur 2 écrans empilés, ${seen.size}/${want.length} textes vus (${[...seen].join(', ')})`);
  e.run(80); e.tap('A'); runUntil(e, () => scr(e) === SS.TITLE && app(e) === 0 && e.peek('ppu_mask') !== 0, 200); e.run(10);
  check(scr(e) === SS.TITLE && e.peek('scroll_y') === 0 && e.peek('scroll_nt') === 0 && e.text(4, 14, 24) === 'PLATINUM DIAMOND EDITION',
    'fin du générique : A ramène à l\'écran titre, défilement et miroir rétablis');
}
{
  const { e, matches } = playComp(3, {}, (m) => m % 3 === 0);
  check(matches === 6 && e.peek('c_over') === 2, `saison avec défaites : classement final ${e.peek('c_rank')}e`);
}
{
  const { e, matches } = playComp(4, { CSIZE: 0, CPROG: 1, CVAR: 1 }, () => true);
  check(matches === 2 && e.peek('c_rank') === 1, 'personnalisé (4 joueurs, progressif, surfaces variées)');
}

}
if (sec('Code de reprise')) {
console.log('Code de reprise');
{
  const e = boot(); startWith(e, { MODE: MODE.COMP, COMP: 2, P1: 6, SURF: 4, ENV: 1, COND: 2 });
  toMatch(e);
  e.poke('sc_winner', 0); e.poke('sc_over', 1); e.poke('ms', 4); e.poke('hl_fin', 1); e.poke('ms_timer', 2);
  runUntil(e, () => app(e) === AS.SCREENS, 60); e.run(20); e.tap('A'); e.run(40);
  const code = e.text(10, 22, 15).replace(/ /g, '');
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
  check(f.text(16, 11, 5) === e.text(16, 11, 5), "même adversaire après reprise");
  const g = boot(); g.tap('UP'); g.tap('A'); g.run(10); g.tap('START'); g.run(10);
  check(scr(g) === SS.CODE, 'code invalide refusé');
}

}
if (sec('Compétition en double, levée de coupe et club nudiste')) {
console.log('Compétition en double, levée de coupe et club nudiste');
{
  const e = boot();
  check(e.peek('nude_unlocked') === 0, 'club nudiste verrouillé au départ');
  toMenu(e); toPick(e, 'LIEU');
  let envs = new Set();
  const names = new Set();
  for (let i = 0; i < 8; i++) { e.tap('RIGHT'); e.run(30); envs.add(e.peek('opt_val', O.ENV)); const t = e.text(0, 1, 32); names.add(t.slice(t.indexOf('<') + 1, t.lastIndexOf('>')).trim()); }
  check(!envs.has(3) && envs.size === 6, `lieu « club nudiste » absent avant déblocage (6 lieux proposés : ${[...names].join(', ')})`);
  check(envs.has(5) && envs.has(6) && names.has('CHATEAU') && names.has('PARC'), 'nouveaux lieux : CHATEAU et PARC');
}
{
  const { e, matches, code } = playComp(0, { TYPE: 1, P2: 5 }, () => true);
  check(e.peek('c_dbl') === 1 && matches === 3 && e.peek('c_rank') === 1, 'tournoi en double gagné (3 matchs)');
  check(e.peek('c_pl2', 0) === 5, 'partenaire choisi (ZOE) conservé');
  const pl = new Set(); for (let i = 0; i < 8; i++) { pl.add(e.peek('c_pl', i)); pl.add(e.peek('c_pl2', i)); }
  check(pl.size === 16, '8 équipes de 2 : 16 personnages différents');
  // levée de coupe : la coupe agrandie (16 sprites) au-dessus des vainqueurs
  // (après la remise des médailles, version 2.52)
  runUntil(e, () => e.peek16('pod_t') >= 290, 400);
  const ct = e.peek('confetti_tile'); let cup = 0, conf = 0, third = 0;
  for (let i = 0; i < 64; i++) { const t = e.nes.cpu.mem[0x201 + i * 4], y = e.nes.cpu.mem[0x200 + i * 4]; if (y < 0xEF) { if (t >= 0x10 && t < 0x20) cup++; if (t >= 0x20 && t < 0x38) third++; if (t === ct) conf++; } }
  check(third === 24, `double : le capitaine 3e en sprites agrandis (${third}/24)`);
  const ntd = (x, y) => e.nes.ppu.vramMem[0x2000 + y * 32 + x];
  check(ntd(12, 12) === 0x60 && ntd(16, 12) === 0x60 + 24 && ntd(6, 14) === 0x60 + 48 && e.text(11, 26, 10).replace(/[\0 ]/g, '') === 'AL/ZO',
    'double : les 2 vainqueurs côte à côte au centre, finaliste à gauche, équipe « AL/ZO » sous la marche');
  check(cup === 16 && conf >= 8, `levée de coupe affichée (coupe agrandie ${cup}/16 sprites, ${conf} confettis)`);
  // feux d'artifice pendant la levée de coupe : 5 gerbes, puis plus rien
  e.poke('cup_t', 0);                       // on rejoue la levée de coupe depuis le début
  let bursts = 0, prev = 0, late = 0;
  for (let n = 0; n < 260; n++) {
    e.run(1);
    let st = 0; for (let i = 0; i < 64; i++) if (e.nes.cpu.mem[0x200 + i * 4] < 0xEF && e.nes.cpu.mem[0x201 + i * 4] === 3) st++;
    if (st >= 4 && prev < 4) bursts++;
    if (n > 230 && st) late++;
    prev = st;
  }
  check(bursts === 5 && late === 0, `levée de coupe : ${bursts} feux d'artifice, puis le calme`);
  check(e.peek('nude_unlocked') === 1 && e.text(4, 7, 21) === 'MODE NUDISTE DEBLOQUE', 'victoire : club nudiste débloqué');
  check(/^[A-Z2-9]{13}$/.test(code), `code de reprise en double : ${code}`);
  // le lieu « club nudiste » est maintenant proposé
  // retour au titre après le podium (CHR-RAM rechargée : ~47 trames)
  e.tap('A'); runUntil(e, () => scr(e) === SS.CHAMP, 60); e.run(70); e.tap('A');   /* (2.76 : le champion brandit la coupe) */
  runUntil(e, () => scr(e) === SS.TITLE && e.peek('ppu_mask') !== 0, 200); e.run(10); toMenu(e);
  setOpts(e, { MODE: MODE.P1, TYPE: 0, ENV: 2 }); e.tap('B'); e.run(5); toMenu(e); toPick(e, 'LIEU');
  let seen = false; for (let i = 0; i < 4; i++) { e.tap('RIGHT'); e.run(30); if (e.peek('opt_val', O.ENV) === 3) { seen = true; break; } }
  check(seen, 'lieu « club nudiste » proposé après déblocage');
  // (chargement des tuiles au naturel + fondu : écran construit en ~25 trames)
  e.tap('START'); runUntil(e, () => scr(e) === SS.PRESENT, 120); e.run(40);
  check(e.text(4, 24, 24) === 'CLUB NUDISTE LES BRONZES', 'présentation du club nudiste');
  e.tap('A'); e.run(70); allCpu(e);
  const v = e.nes.ppu.vramMem;
  {
    // panneau « BRONZES » à côté du score : tuiles du panneau en CHR-RAM et placées (rangées 2-3, col. 5-8)
    const rb = (sym, off) => e.nes.rom.rom[3][e.addr(sym) - 0x8000 + off];
    const idx = [...Array(8).keys()].map((k) => rb('sign2_idx', k));
    const placed = idx.every((t, k) => e.nes.ppu.vramMem[0x2000 + (2 + (k >> 2)) * 32 + 5 + (k % 4)] === t);
    let same = true; for (let b = 0; b < 128; b++) if (e.nes.ppu.vramMem[idx[b >> 4] * 16 + (b & 15)] !== rb('sign2_chr', b)) same = false;
    check(e.peek('cur_bank') === 0 && placed && same, 'match au club nudiste (panneau BRONZES à côté du score)');
  }
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
  // retour au titre après le podium (CHR-RAM rechargée : ~47 trames)
  e.tap('A'); runUntil(e, () => scr(e) === SS.CHAMP, 60); e.run(70); e.tap('A');   /* (2.76 : le champion brandit la coupe) */
  runUntil(e, () => scr(e) === SS.TITLE && e.peek('ppu_mask') !== 0, 200); e.run(10); toMenu(e); setOpts(e, { MODE: MODE.COMP, COMP: 0 }); e.tap('START'); passCustom(e); e.run(40);
  e.tap('LEFT'); e.run(20);   // (2.77 : ouverture sur le tableau ; le code est sur la page du match)
  const code = e.text(10, 22, 15).replace(/ /g, '');
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

}
if (sec('Compétition en double à deux joueurs humains')) {
console.log('Compétition en double à deux joueurs humains');
{
  const e = boot(); startWith(e, { MODE: MODE.COMP, COMP: 2, TYPE: 1, P2: 3, CPART: 1 });
  const opening = e.peek('c_pg') === 1; e.tap('LEFT'); e.run(20);   // (2.77 : ouverture sur le tableau)
  check(opening && scr(e) === SS.COMP && e.text(3, 23, 18) === 'DOUBLE A 2 JOUEURS', 'championnat en double à 2 joueurs : ouverture sur le tableau, puis la page du match');
  e.tap('A'); runUntil(e, () => scr(e) === SS.PRESENT && litScr(e), 90);
  check(e.text(2, 22, 11) === '+ MANETTE 2', 'présentation : partenaire à la manette 2');
  e.tap('A'); e.run(40);
  const c = e.addr('cfg');
  check(app(e) === AS.MATCH && e.nes.cpu.mem[c + 2] === 0 && e.nes.cpu.mem[c + 4] === 1 && e.nes.cpu.mem[c + 3] === 2 && e.nes.cpu.mem[c + 5] === 2,
    'manette 1 et manette 2 dans la même équipe, adversaires CPU');
  runUntil(e, () => e.peek('ms') === 2 || e.peek('ms') === 0, 300);
  const x0 = e.peekS16('plx', 4); e.down('RIGHT', 2); e.run(20); e.up('RIGHT', 2);
  check(e.peekS16('plx', 4) !== x0, 'la manette 2 déplace le partenaire');
  // victoire forcée puis reprise par code sur une autre console
  e.poke('sc_winner', 0); e.poke('sc_over', 1); e.poke('ms', 4); e.poke('hl_fin', 1); e.poke('ms_timer', 2);
  runUntil(e, () => app(e) === AS.SCREENS, 60); e.run(20); e.tap('A'); e.run(40);
  const code = e.text(10, 22, 15).replace(/ /g, '');
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

}
if (sec('Vitesse du jeu, coups gagnants, smash sauté, musiques')) {
console.log('Vitesse du jeu, coups gagnants, smash sauté, musiques');
{
  // vitesse : nombre de trames logiques du match pour 120 trames affichées
  const rate = (gs) => {
    const e = boot(); startWith(e, { FMT: 3, GSPEED: gs }); allCpu(e);
    runUntil(e, () => e.peek('ms') === 0, 200);
    // fin de match : ms_timer décompte chaque trame logique ; le CPU (équipe 0)
    // bat un humain (équipe 1) : pas de fête, qui fausserait la mesure
    e.nes.cpu.mem[e.addr('cfg') + CFG.CTRL + 1] = 0; e.poke('sc_winner', 0);
    e.poke('ms', 4); e.poke('hl_fin', 1); e.poke('ms_timer', 250); e.run(120);
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
  e.poke('hitter', 1); e.poke('hitter_p', 1); e.poke('rally_hits', 3); e.poke('bgrav', 6);
  set16('plx', 128 * 64, 0); set16('ply', 168 * 64, 0);
  set16('bx', 132 * 64); set16('by', 165 * 64); set16('bz', 52 * 64); set16('bvx', 0); set16('bvy', 0); set16('bvz', 0);
  e.down('A'); e.run(3); e.up('A');
  check(e.peek('hit_jump') === 1 && e.peek('last_shot') === 7 && e.peek('hitter') === 0 && e.peekS16('bvy') < 0,
    'smash sauté sur une balle à 52 px de haut');
  check(e.peek('pl_jump', 0) > 0, 'le joueur saute (sprite décalé vers le haut)');
  e.run(40);
  check(e.peek('pl_jump', 0) === 0, 'puis retombe');
  // une balle à 52 px reste hors de portée d'un CPU de niveau normal
  const f = boot(); startWith(f, { FMT: 0, LEVEL: 1 }); allCpu(f);   // (un set : assez de services)
  runUntil(f, () => f.peek('ms') === 0, 200);
  const g16 = (n, v, o = 0) => { f.poke(n, v & 255, o); f.poke(n, (v >> 8) & 255, o + 1); };
  f.poke('ms', 2); f.poke('b_live', 1); f.poke('b_vis', 1); f.poke('b_serve', 0); f.poke('bounces', 0);
  f.poke('hitter', 1); f.poke('hitter_p', 1); f.poke('rally_hits', 3); f.poke('bgrav', 0); f.poke('ai_delay', 0, 0);
  g16('plx', 128 * 64, 0); g16('ply', 168 * 64, 0);
  g16('bx', 132 * 64); g16('by', 165 * 64); g16('bz', 52 * 64); g16('bvx', 0); g16('bvy', 0); g16('bvz', 0);
  f.run(4);
  check(f.peek('hitter') === 1, 'le smash sauté est réservé aux humains et au CPU expert');
}
{
  // smash dirigé, charge, volée haute : balle immobile au-dessus d'un joueur
  // humain près du filet ; la charge se fait balle hors de portée
  const hit = (z, btn, dirs = [], charge = 0, bnc = 0) => {
    const e = boot(); startWith(e, { FMT: 3 });
    runUntil(e, () => e.peek('ms') === 0, 200);
    const set16 = (n, v, o = 0) => { e.poke(n, v & 255, o); e.poke(n, (v >> 8) & 255, o + 1); };
    e.poke('ms', 2); e.poke('b_live', 1); e.poke('b_vis', 1); e.poke('b_serve', 0); e.poke('bounces', bnc);
    e.poke('hitter', 1); e.poke('hitter_p', 1); e.poke('rally_hits', 3); e.poke('bgrav', 0);
    set16('plx', 128 * 64, 0); set16('ply', 168 * 64, 0);
    set16('bx', (charge ? 180 : 132) * 64); set16('by', 165 * 64); set16('bz', z * 64); set16('bvx', 0); set16('bvy', 0); set16('bvz', 0);
    e.down(btn); for (const d of dirs) e.down(d);
    if (charge) { e.run(charge); set16('plx', 128 * 64, 0); set16('ply', 168 * 64, 0); set16('bx', 132 * 64); set16('by', 165 * 64); }
    e.run(3); e.releaseAll(); e.run(1);
    return { shot: e.peek('last_shot'), spin: e.peek('bspin'), vy: -e.peekS16('bvy'), land: e.peekS16('land_y'), hitter: e.peek('hitter') };
  };
  const plain = hit(30, 'A'), deep = hit(30, 'A', ['UP']), short = hit(30, 'A', ['DOWN']);
  check(plain.shot === 7 && plain.hitter === 0 && deep.land < plain.land - 6 && short.land > plain.land + 10,
    `smash dirigé : HAUT vers la ligne de fond (y ${deep.land} contre ${plain.land}), BAS dans l'angle court (y ${short.land})`);
  const sl = hit(30, 'B'), dr = hit(30, 'B', ['DOWN']);
  check(sl.shot === 7 && sl.spin === 2 && dr.shot === 7 && dr.spin === 3 && dr.land > plain.land + 20,
    `smash coupé (B) et smash amorti (B + BAS, y ${dr.land})`);
  const armed = hit(30, 'A', [], 30);
  check(armed.shot === 7 && armed.vy > plain.vy, `smash armé plus rapide (vitesse ${armed.vy} contre ${plain.vy})`);
  const high = hit(22, 'A');
  check(high.shot !== 7, 'balle à 22 px (hauteur d\'épaule) avant le rebond : volée haute, pas un smash');
  const vf = hit(12, 'A', ['UP']), vfa = hit(12, 'A', ['UP'], 30), vd = hit(12, 'B', ['DOWN']);
  check(vf.shot === 6 && vfa.vy > vf.vy && vd.shot === 3 && vd.land > 110,
    `volée à plat appuyée plus vive (${vfa.vy} contre ${vf.vy}), volée amortie près du filet (y ${vd.land})`);
  const after = hit(30, 'A', [], 0, 1);
  check(after.shot !== 7, 'balle haute après le rebond : coup normal, pas de smash');
}
{
  // musiques hors match : valse Margaux au menu, thème de la finale avant la finale
  const e = boot();
  check(e.peek('music_cur') === 1, 'écran titre : polka');
  e.run(30);
  toMenu(e);
  check(e.peek('music_cur') === 1 && e.peek16('mel_ptr') !== e.peek16('mel_start'), 'menu : même polka, qui continue sans repartir du début');
  e.run(800);
  check(e.peek('music_cur') === 1, 'menu : la polka reboucle');
  e.poke('opt_val', 2, O.ENV); toPick(e, 'LIEU'); e.tap('RIGHT'); e.run(30);
  check(e.peek('opt_val', O.ENV) === 4 && e.peek('music_cur') === 7, 'page du lieu : court Margaux choisi, sa musique démarre');
  { const mm = e.nes.mmap, w = mm.write.bind(mm), vol = new Set();
    mm.write = (a, v) => { if (a === 0x4004) vol.add(v); return w(a, v); };
    e.run(600); mm.write = w;
    check([...vol].every((v) => v === 0x30 || (v & 0x30) === 0x30), `valse Margaux : notes tenues à volume constant (${[...vol].map((v) => v.toString(16))})`); }
  e.tap('LEFT'); e.run(30);
  check(e.peek('music_cur') === 1, 'autre lieu : retour à la polka');
  e.tap('RIGHT'); e.run(30); e.tap('START'); runUntil(e, () => scr(e) === SS.PRESENT, 120); e.run(5);
  check(scr(e) === SS.PRESENT && e.peek('music_cur') === 7, 'présentation au court Margaux : valse');
  e.tap('A'); runUntil(e, () => app(e) === AS.MATCH, 60);
  check(app(e) === AS.MATCH && e.peek('music_cur') === 0, 'aucune musique pendant le match');
}
{
  const e = boot(); startWith(e, { MODE: MODE.COMP, COMP: 0, FMT: 3 });
  check(scr(e) === SS.COMP && e.peek('music_cur') !== 6, 'quart de finale : pas de thème de finale');
  for (let m = 0; m < 2; m++) {   // tournoi à 8 : quart puis demi gagnés
    toMatch(e);
    e.poke('sc_winner', 0); e.poke('sc_over', 1); e.poke('ms', 4); e.poke('hl_fin', 1); e.poke('ms_timer', 2);
    runUntil(e, () => app(e) === AS.SCREENS, 60); e.run(20); e.tap('A'); e.run(40);
  }
  check(scr(e) === SS.COMP && e.text(3, 5, 6) === 'FINALE' && e.peek('music_cur') === 6, 'écran de la finale : thème de la finale');
  e.tap('A'); toPresent(e, 90); runUntil(e, () => litScr(e), 90);
  check(scr(e) === SS.PRESENT && e.peek('music_cur') === 6, 'présentation de la finale : le thème continue');
  e.tap('A'); runUntil(e, () => app(e) === AS.MATCH, 60);
  check(app(e) === AS.MATCH && e.peek('music_cur') === 0, 'finale : silence pendant le match');
}

}
if (sec('Finitions : ralentis, IA, effets, son, réglages mémorisés')) {
console.log('Finitions : ralentis, IA, effets, son, réglages mémorisés');
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
    e.poke('hitter', 1); e.poke('hitter_p', 1); e.poke('rally_hits', 3); e.poke('bgrav', 6);
    s16('plx', (100 + t * 5) * 64, 0); s16('ply', 200 * 64, 0); s16('plx', (110 + t * 3) * 64, 2); s16('ply', 104 * 64, 2);
    s16('bx', (104 + t * 5) * 64); s16('by', 197 * 64); s16('bz', 10 * 64); s16('bvx', 0); s16('bvy', 0); s16('bvz', 0);
    e.poke('rand_seed', seed); e.down('B'); e.down('UP'); e.run(3); e.releaseAll();
    for (let i = 0; i < 200; i++) { e.run(1); if (e.peek('hitter') === 1) return e.peek('hit_jump'); if (e.peek('ms') !== 2) return 0; }
    return 0;
  };
  let j2 = 0, j1 = 0;
  for (let t = 0; t < 8; t++) { j2 += lobTrial(2, t * 37 + 11, t); j1 += lobTrial(1, t * 37 + 11, t); }
  // (version 2.59 : un lob relâché aussitôt est court ; l'Expert peut le smasher à chaque fois)
  check(j2 >= 2 && j1 === 0, `lob court contre un CPU au filet : smash sauté ${j2}/8 en Expert, ${j1}/8 en Normal`);
}
{
  // le saut est enregistré pour les ralentis (bits 6-7 de pose_cache)
  const e = boot(); startWith(e, { FMT: 3 });
  runUntil(e, () => e.peek('ms') === 0, 200);
  const s16 = (n, v, o = 0) => { e.poke(n, v & 255, o); e.poke(n, (v >> 8) & 255, o + 1); };
  e.poke('ms', 2); e.poke('b_live', 1); e.poke('b_vis', 1); e.poke('b_serve', 0); e.poke('bounces', 0);
  e.poke('hitter', 1); e.poke('hitter_p', 1); e.poke('rally_hits', 3); e.poke('bgrav', 6);
  s16('plx', 128 * 64, 0); s16('ply', 168 * 64, 0);
  s16('bx', 132 * 64); s16('by', 165 * 64); s16('bz', 52 * 64); s16('bvx', 0); s16('bvy', 0); s16('bvz', 0);
  e.down('A'); e.run(4); e.up('A');
  check((e.peek('pose_cache', 0) >> 6) > 0, 'saut enregistré pour le ralenti');
}
{
  // effets : ombres (simple), arbitre (tous les lieux), poussière (terre), repère (lent)
  const e = boot(); startWith(e, { FMT: 3, SURF: 0, ENV: 0, GSPEED: 0 }); allCpu(e);
  e.run(60);
  let t = oamTiles(e);
  check(t.has(5) && t.has(13) && (t.has(11) || t.has(12)), 'ombres des joueurs et arbitre de chaise (tête, buste)');
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
  check(!mark2 && oamTiles(g).has(13), 'vitesse normale : pas de repère ; arbitre aussi au club');
}
{
  // chaise d'arbitre de profil, au bord du court contre le poteau du filet
  // (x 32-47), dans le décor de chaque lieu (palette 3 :
  // mêmes couleurs de jour, de nuit et sous la pluie) ; gestes de l'arbitre
  const nt = (e, x, y) => e.nes.ppu.vramMem[0x2000 + y * 32 + x];
  const at = (e, a) => e.nes.ppu.vramMem[a];
  let tiles = null, okall = true;
  for (const env of [0, 1, 2, 4]) for (const cond of [0, 1, 2]) {
    const e = boot(); startWith(e, { ENV: env, COND: cond, SURF: 0 }); e.run(30);
    const t = []; for (let r = 14; r < 18; r++) t.push(nt(e, 4, r), nt(e, 5, r));
    if (!tiles) tiles = t.join(); else if (tiles !== t.join()) okall = false;
    if (((at(e, 0x23D9) >> 4) & 3) !== 3 || (at(e, 0x23E1) & 3) !== 3) okall = false;
    if (cond === 0 && env === 0) { const p = e.addr('pal_buf'); if (e.nes.cpu.mem[p + 13] !== 0x0F || e.nes.cpu.mem[p + 15] !== 0x30) okall = false; }
    { const p = e.addr('pal_buf'); if (e.nes.cpu.mem[p + 14] !== 0x37 || e.nes.cpu.mem[p + 16 + 9] !== 0x37) okall = false; }   // peau beige (arbitre, spectateurs)
  }
  check(okall, 'chaise d\'arbitre identique dans les 4 lieux, de jour, de nuit et sous la pluie (palette 3 noir/blanc) ; peau beige de l\'arbitre et des spectateurs');
  const g = boot(); startWith(g, { ENV: 0, SURF: 0 }); g.run(100);
  const tt = g.peek('trophy_tile');
  g.poke('ump_arm', 1); g.poke('end_reason', 1); g.poke('ms', 3); g.poke('ms_timer', 60); g.run(2);
  const out = oamTiles(g).has(tt) && oamTiles(g).has(tt + 1);
  g.poke('end_reason', 3); g.poke('ms_timer', 60); g.run(2);
  const let_ = oamTiles(g).has(13) && oamTiles(g).has(tt + 2);
  g.poke('ms', 2); g.poke('ump_arm', 0); g.run(2);
  const rest = oamTiles(g).has(13) && !oamTiles(g).has(tt) && !oamTiles(g).has(tt + 2);
  // la tête suit la balle : vers le haut ou vers le bas du court
  const by = g.addr('by'); g.nes.cpu.mem[by] = 0; g.nes.cpu.mem[by + 1] = 0x10; g.run(1); const up = oamTiles(g).has(12);
  g.nes.cpu.mem[by + 1] = 0x30; g.run(1); const dn = oamTiles(g).has(11);
  check(out && let_ && rest && up && dn, 'arbitre : tête qui suit la balle, bras tendu sur une faute, bras levé sur un let, au repos sinon');
  g.tap('START'); g.run(5); g.tap('DOWN'); g.tap('A'); runUntil(g, () => scr(g) === SS.RESULTS || scr(g) === SS.MENU || scr(g) === SS.TITLE, 200); g.run(30);
  const rom2 = g.nes.rom.rom[2]; let diff = 0; for (let i = 0; i < 48; i++) if (g.nes.ppu.vramMem[0x1000 + tt * 16 + i] !== rom2[0x1000 + tt * 16 + i]) diff++;
  check(app(g) === AS.SCREENS && diff === 0, `hors du match : tuiles de la coupe rétablies (${diff} octet(s) différent(s))`);
}
{
  // court Margaux : l'arbitre est une princesse (couronne, robe rose, baguette à étoile)
  const g = boot(); startWith(g, { ENV: 4 }); g.run(60);
  const rom2 = g.nes.rom.rom[2], v = g.nes.ppu.vramMem, tt = g.peek('trophy_tile');
  const tileIs = (t, src) => { for (let i = 0; i < 16; i++) if (v[0x1000 + t * 16 + i] !== src[i]) return false; return true; };
  const rb = (sym, off) => g.nes.rom.rom[3][g.addr(sym) - 0x8000 + off];
  const prin = [...Array(112).keys()].map((k) => rb('ump_chr', 112 + k));
  const loaded = tileIs(11, prin.slice(0, 16)) && tileIs(13, prin.slice(32, 48)) && tileIs(tt + 3, prin.slice(96, 112));
  g.poke('ms', 2); g.poke('ump_arm', 0); g.run(2);
  const wand = oamTiles(g).has(tt + 3) && oamTiles(g).has(13);
  const pink = g.peek('pal_buf', 16 + 11) === 0x34;
  // ailleurs : l'arbitre habituel, sans baguette
  const n = boot(); startWith(n, { ENV: 0 }); n.run(60); n.poke('ms', 2); n.poke('ump_arm', 0); n.run(2);
  let same = true; for (let i = 0; i < 48; i++) if (n.nes.ppu.vramMem[0x1000 + 11 * 16 + i] !== rom2[0x1000 + 11 * 16 + i]) same = false;
  const nowand = !oamTiles(n).has(n.peek('trophy_tile') + 3) && n.peek('pal_buf', 16 + 11) === 0x30;
  check(loaded && wand && pink && same && nowand, `court Margaux : arbitre princesse (couronne, robe rose, baguette) ; ailleurs l'arbitre habituel`);
}
{
  // l'arbitre reste sur sa chaise pendant les ralentis
  const e = boot(); startWith(e, { FMT: 3 }); allCpu(e);
  let seen = false, rep = 0;
  runUntil(e, () => { if (e.peek('ms') === 5 && ++rep === 6) seen = oamTiles(e).has(13); return rep >= 6; }, 60 * 60 * 6);
  check(rep && seen, 'ralenti : l\'arbitre reste visible');
}
{
  // réglage SON et réglages mémorisés dans la cartouche
  const e = boot(); toMenu(e);
  const row = (o) => { for (let i = 0; i < e.peek('nvis'); i++) if (e.peek('vis', i) === o) return i; return -1; };
  e.poke('cursor', row(O.SOUND)); e.tap('RIGHT'); e.run(5);
  check(e.peek('snd_mode') === 1 && e.peek16('mel_ptr') === 0, 'SON : bruitages seuls, musique arrêtée');
  e.tap('RIGHT'); e.run(5);
  check(e.peek('snd_mode') === 2 && e.text(16, menuY(e, row(O.SOUND)), 5) === 'AUCUN', 'SON : aucun');
  e.tap('LEFT'); e.tap('LEFT'); e.run(5);
  check(e.peek('snd_mode') === 0 && e.peek16('mel_ptr') !== 0, 'SON : musique relancée');
  setOpts(e, { P1: 5, SURF: 3, GSPEED: 2, SOUND: 1, ENV: 4 }); e.tap('START'); e.run(30);
  // « rallumage » : une console neuve qui garde la RAM de cartouche
  const f = new Emu(rom);
  for (let a = 0x6000; a < 0x8000; a++) f.nes.cpu.mem[a] = e.nes.cpu.mem[a];
  f.run(40);
  const sndIntro = f.peek('snd_mode');               // réglage SON appliqué dès le générique
  f.tap('START', 1, 1, 0); f.run(4); f.tap('START', 1, 1, 0); f.run(40);
  check(f.peek('opt_val', O.P1) === 5 && f.peek('opt_val', O.SURF) === 3 && f.peek('opt_val', O.GSPEED) === 2
    && f.peek('opt_val', O.ENV) === 4 && f.peek('snd_mode') === 1 && sndIntro === 1, 'derniers réglages retrouvés au démarrage (cartouche à pile), SON dès le générique');
  check(f.peek16('sv', 4) === e.peek16('sv', 4), 'palmarès conservé à côté des réglages');
}

}
if (sec('Margaux jouable et fatigue')) {
console.log('Margaux jouable et fatigue');
{
  // 1 joueur contre CPU : toujours en simple (le double avec partenaire CPU
  // est le mode en équipe) ; la ligne JEU n'existe qu'à 2 joueurs adversaires
  const t = boot(); toMenu(t);
  check(t.peek('opt_val', O.MODE) === MODE.P1 && rowOf(t, O.TYPE) < 0 && t.text(3, menuY(t, 2), 10) === 'NIVEAU CPU', '1 joueur contre CPU : pas de ligne JEU (simple seulement)');
  t.poke('cursor', 1); t.tap('RIGHT');
  let dark = 0; for (let n = 0; n < 20; n++) { t.run(1); if (!t.nes.ppu.f_bgVisibility) dark++; }
  check(t.peek('opt_val', O.MODE) === MODE.P2 && t.text(3, menuY(t, 2), 3) === 'JEU' && dark === 0, `2 joueurs adversaires : ligne JEU ; menu redessiné sans couper l'écran (${dark} trame(s) noire(s))`);
  t.tap('LEFT'); t.run(20);
  check(t.text(3, menuY(t, 2), 10) === 'NIVEAU CPU' && t.text(3, menuY(t, t.peek('nvis')), 13).replace(/[\0 ]/g, '') === '' && t.text(3, menuY(t, -1), 13).replace(/[\0 ]/g, '') === '', 'retour à 1 joueur : lignes réécrites et recentrées, anciennes lignes effacées');
  const u = boot(); startWith(u, { MODE: MODE.P1, TYPE: 1 });
  check(app(u) === AS.MATCH && u.peek('npl') === 2, `1 joueur contre CPU avec un ancien réglage « double » : match en simple (${u.peek('npl')} joueurs)`);
  // Margaux (personnage 16) : menu, présentation, code de reprise
  const e = boot(); toMenu(e); setOpts(e, { P1: 15, P2: 1 }); toPick(e, 'TON JOUEUR');
  const nm19 = () => e.text(2, 19, 8).replace(/[\0<> ]/g, '');
  e.tap('RIGHT'); e.run(30);
  check(e.peek('opt_val', O.P1) === 16 && nm19() === 'MARGAUX', 'MARGAUX sélectionnable (17e personnage)');
  const nn = [];
  for (let k = 0; k < 4; k++) { e.tap('RIGHT'); e.run(30); nn.push(nm19()); }
  check(nn.join(',') === 'MAX,LOLA,PAPI,TITOU', `nouveaux personnages après Margaux : ${nn.join(', ')}`);
  e.tap('RIGHT'); e.run(30);
  check(e.peek('opt_val', O.P1) === 0, 'retour au premier personnage après TITOU');
  const g = boot(); toMenu(g); setOpts(g, { MODE: MODE.P2, P1: 16, TYPE: 1, P2: 3 }); g.tap('START'); passCustom(g); runUntil(g, () => scr(g) === SS.PRESENT, 120); g.run(30);
  check(scr(g) === SS.PRESENT && g.text(2, 19, 7) === 'MARGAUX' && g.text(2, 20, 1) === '+' && g.text(4, 20, 3) !== '   ',
    `présentation en double : MARGAUX, partenaire sur la ligne suivante (${g.text(2, 20, 6).replace(/\0/g, " ")})`);
  const c = boot(); startWith(c, { MODE: MODE.COMP, COMP: 0, FMT: 3, P1: 16, TYPE: 1, P2: 7 });
  c.tap('LEFT'); c.run(20);   // (2.77 : ouverture sur le tableau ; le code est sur la page du match)
  const code = c.text(10, 22, 15).replace(/ /g, '');
  check(scr(c) === SS.COMP && c.text(3, 11, 5) === 'MX/EM', `équipe abrégée « MX/EM » (pas de confusion avec MAYA « MA ») ; code ${code}`);
  const h = boot(); h.tap('UP'); h.tap('A'); h.run(10);
  const AL = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
  for (let i = 0; i < 13; i++) { const n = AL.indexOf(code[i]); for (let k = 0; k < n; k++) h.tap('UP', 1, 1, 1); if (i < 12) h.tap('RIGHT', 1, 1, 1); }
  h.tap('START'); h.run(40);
  check(scr(h) === SS.COMP && h.peek('c_pl', 0) === 16 && h.peek('c_pl2', 0) === 7, 'code de reprise avec Margaux relu');
  // tête propre à Margaux (cheveux longs, blonds, bouclés) en match
  const m = boot(); startWith(m, { FMT: 3, P1: 16 }); allCpu(m); m.run(60);
  const mt = new Set(); for (let i = 0; i < 44; i++) { mt.add(m.peek('marg_map', i)); mt.add(m.peek('marg_map', i) + 1); }
  mt.add(m.peek('marg_prof_l')); mt.add(m.peek('marg_prof_r'));
  const tl = oamTiles(m);
  check([...tl].some((x) => mt.has(x)), 'Margaux affichée avec sa propre chevelure');
  // jamais au club nudiste : lieu sauté au menu
  const n = boot(); n.poke('nude_unlocked', 1); toMenu(n);
  n.poke('opt_val', 16, O.P1); n.poke('opt_val', 2, O.ENV);
  toPick(n, 'LIEU'); n.tap('RIGHT'); n.run(30);
  check(n.peek('opt_val', O.ENV) === 4, 'avec Margaux, le club nudiste est sauté (lieu suivant : court Margaux)');
}
{
  // fatigue : déplacement d'un joueur humain en début d'échange et après 20 frappes
  const dist = (fat, hits) => {
    const e = boot(); startWith(e, { FMT: 3, FATIG: fat, SURF: 1 });   // gazon : sans glissade
    runUntil(e, () => e.peek('ms') === 0, 300);
    const s16 = (n, v, o = 0) => { e.poke(n, v & 255, o); e.poke(n, (v >> 8) & 255, o + 1); };
    e.poke('ms', 2); e.poke('b_live', 1); e.poke('b_vis', 0); e.poke('hitter', 0); e.poke('hitter_p', 2); e.poke('rally_hits', hits);
    s16('plx', 80 * 64, 0); s16('ply', 200 * 64, 0);
    // la goutte clignote (16 trames allumée, 16 éteinte) : on la cherche sur toute la course
    const x0 = e.peekS16('plx', 0); let t = false;
    e.down('RIGHT'); for (let f = 0; f < 20; f++) { e.run(1); t = t || oamTiles(e).has(14); } e.up('RIGHT');
    for (let f = 0; f < 16 && !t; f++) { e.run(1); t = oamTiles(e).has(14); }
    return [e.peekS16('plx', 0) - x0, t];
  };
  const [d0] = dist(1, 2), [d1, sweat] = dist(1, 20), [d2] = dist(0, 20);
  check(d1 < d0 && d1 > d0 * 0.7 && d2 === d0, `fatigue : ${d0} -> ${d1} (option activée), ${d2} (désactivée)`);
  check(sweat, 'goutte de sueur au-dessus des joueurs fatigués');
}

{
  // A + HAUT : à plat nettement plus rapide que le lifté ; A + BAS : croisé
  // court en angle, dans le carré de service près de la ligne de côté (sur
  // gazon : le robot se place sans glissade)
  const play = (dir) => {
    const e = boot(); startWith(e, { MODE: MODE.TRAIN, EX: 2, COUNT: 0, SURF: 1 });
    const shots = []; let cur = null, pr = 0, pb = 0;
    for (let n = 0; n < 60 * 90 && shots.length < 16; n++) {
      botStep(e, dir ? ['A', dir] : ['A']); e.run(1);
      const r = e.peek('rally_hits'), h = e.peek('hitter'), b = e.peek('bounces');
      if (r !== pr && h === 0) { if (cur) shots.push(cur); cur = { v: Math.hypot(e.peekS16('bvx'), e.peekS16('bvy')), x: null, y: null }; }
      if (cur && b === 1 && pb === 0 && h === 0 && cur.x === null) { cur.x = e.peekS16('bx') >> 6; cur.y = e.peekS16('by') >> 6; }
      pr = r; pb = b;
    }
    e.releaseAll();
    return shots;
  };
  const avg = (s) => s.reduce((a, x) => a + x.v, 0) / (s.length || 1);
  const lift = play(''), flat = play('UP'), short = play('DOWN');
  const landed = short.filter((s) => s.x !== null), angle = landed.filter((s) => s.y >= 87 && s.y <= 132 && (s.x < 92 || s.x > 164));
  check(avg(flat) > avg(lift) * 1.15, `à plat nettement plus rapide que le lifté (${avg(flat).toFixed(0)} contre ${avg(lift).toFixed(0)})`);
  check(landed.length >= 5 && angle.length >= landed.length * 0.7, `croisé court : ${angle.length}/${landed.length} balles en angle dans le carré de service`);
}

{
  // public : suit la balle du regard (tuiles en CHR-RAM), ola (tribunes
  // réécrites puis rétablies), gradins en salle et bancs au club (palette 3)
  const e = boot(); startWith(e, { ENV: 0, SURF: 2, REPLAY: 0 }); allCpu(e); e.run(200);   /* (sans ralentis : un zoom en plein écran masquerait les tribunes) */
  runUntil(e, () => e.peek('ms') === 2, 600);   // un échange en cours (le public suit la balle)
  const r3 = (sym, off) => e.nes.rom.rom[3][e.addr(sym) - 0x8000 + off];
  const ci = [...Array(6).keys()].map((k) => r3('crowd_idx', k)), chi = [...Array(6).keys()].map((k) => r3('cheer_idx', k));
  const tileIs = (k, sym) => { for (let b = 0; b < 16; b++) if (e.nes.ppu.vramMem[ci[k] * 16 + b] !== r3(sym, k * 16 + b)) return false; return true; };
  const by = e.addr('by'), hold = (hi) => { for (let n = 0; n < 12; n++) { e.nes.cpu.mem[by + 1] = hi; e.poke('b_vis', 1); e.run(1); } };
  hold(0x10); const up = [0, 1, 2, 3, 4, 5].every((k) => tileIs(k, 'crowd_up'));
  hold(0x30); const dn = [0, 1, 2, 3, 4, 5].every((k) => tileIs(k, 'crowd_norm'));
  check(up && dn, 'public : regarde vers le haut puis vers le bas selon la balle');
  const stands = () => { const t = []; for (let r = 4; r < 30; r++) for (const c of [0, 1, 2, 3, 28, 29, 30, 31]) t.push(e.nes.ppu.vramMem[0x2000 + r * 32 + c]); return t; };
  const before = stands().join();
  e.poke('crowd_timer', 64); let seen = 0;
  for (let n = 0; n < 40; n++) { e.run(1); if (stands().some((t) => chi.includes(t))) seen++; }
  e.run(60);
  check(seen > 20 && stands().join() === before, `ola : la vague descend les tribunes (${seen} trames) puis tout est rétabli (état ${e.peek('ola_on')}, échange ${e.peek('rally_hits')})`);
  const g = boot(); startWith(g, { ENV: 2, SURF: 2 });
  const h = boot(); startWith(h, { ENV: 1, SURF: 1 });
  const cb = [...Array(6).keys()].map((k) => r3('crowdb_idx', k));   // jeu B : club et salle
  const crowdIn = (x) => { let n = 0; for (let r = 4; r < 30; r++) for (const c of [0, 1, 30, 31]) if (cb.includes(x.nes.ppu.vramMem[0x2000 + r * 32 + c])) n++; return n; };
  check(crowdIn(g) >= 80 && crowdIn(h) >= 24 && (g.nes.ppu.vramMem[0x23C0 + 2 * 8] & 3) === 3,
    `spectateurs en salle (${crowdIn(g)} tuiles) et au club (${crowdIn(h)}), bien visibles (palette 3)`);
}
{
  // réactions du public (canal de bruit) : applaudissements rythmés avant une
  // balle de match, silence au lancer, « OOOH » sur une balle dehors de peu,
  // soupir sur une double faute
  const e = boot(); startWith(e, { FMT: 3 }); e.run(60);
  const mm = e.nes.mmap, w = mm.write.bind(mm); let env = 0; const log = [];
  mm.write = (a, v) => { if (a === 0x400C) { env = v; if (v === 0x30) log.push('silence'); } if (a === 0x400E) log.push(env.toString(16) + '/' + v.toString(16)); return w(a, v); };
  const s16 = (n, v) => { e.poke(n, v & 255); e.poke(n, (v >> 8) & 255, 1); };
  e.poke('sc_games', 3, 0); e.poke('sc_points', 3, 0); e.poke('sc_points', 0, 1); e.poke('sc_server', 0);
  e.poke('sc_crit', 4 | 1);   /* (2.71 : calculé à la fin de chaque point ; ici le score est posé à la main) */
  e.poke('ms', 2); e.run(1); e.poke('ms', 0); e.poke('ms_timer', 250); log.length = 0; e.run(60);
  const claps = log.filter((x) => x === '1/5').length;
  log.length = 0; e.tap('A', 1, 1, 0); e.run(3);
  const hush = log.includes('silence');
  // balle dehors de 2 px (couloir gauche du simple)
  e.poke('ms', 2); e.run(1); log.length = 0;
  s16('bx', 70 * 64); s16('by', 100 * 64); e.poke('end_reason', 2); e.poke('ump_arm', 1); e.poke('ms', 3); e.poke('ms_timer', 80); e.run(40);   // (2 px dehors : ligne et rayon de la balle compris, 2.76)
  const ooh = log.includes('b/5');
  e.poke('ms', 2); e.run(1); log.length = 0; e.poke('st_dfaults', e.peek('st_dfaults', 0) + 1, 0); e.poke('ump_arm', 0); e.poke('ms', 3); e.run(40);
  const groan = log.includes('9/9');
  check(claps >= 2 && hush && ooh && groan, `réactions : ${claps} applaudissements sur balle de match, silence au lancer${hush ? '' : ' (absent)'}, « OOOH »${ooh ? '' : ' (absent)'}, soupir${groan ? '' : ' (absent)'}`);
}

{
  // bruitages : « pok » de la frappe avec un corps au triangle, souffle du geste,
  // bip du juge électronique sur une balle dehors, crissement sur dur, et
  // priorités du canal de bruit (un rebond ne coupe plus l'ovation)
  const e = boot(); startWith(e, { FMT: 3, SURF: 2 }); e.run(60);
  const mm = e.nes.mmap, w = mm.write.bind(mm); const log = [];
  mm.write = (a, v) => { if (a >= 0x4000 && a <= 0x400F) log.push([a, v]); return w(a, v); };
  const has = (a, v) => log.some(([x, y]) => x === a && (v === undefined || y === v));
  const s16 = (n, v, o = 0) => { e.poke(n, v & 255, o); e.poke(n, (v >> 8) & 255, o + 1); };
  // frappe du joueur : geste (souffle) puis frappe (triangle)
  e.poke('ms', 2); e.poke('b_live', 1); e.poke('b_vis', 1); e.poke('b_serve', 0); e.poke('bounces', 1);
  e.poke('hitter', 1); e.poke('hitter_p', 1); e.poke('rally_hits', 3); e.poke('bgrav', 0);
  s16('plx', 128 * 64, 0); s16('ply', 200 * 64, 0);
  s16('bx', 138 * 64); s16('by', 155 * 64); s16('bz', 6 * 64); s16('bvx', 0); s16('bvy', 128); s16('bvz', 0);
  log.length = 0; e.down('A'); e.run(8); e.up('A');
  for (let i = 0; i < 40 && e.peek('hitter') !== 0; i++) e.run(1);
  const whoosh = has(0x400E, 0x02);
  const pok = has(0x400B) && has(0x4008, 0x0A);
  // balle dehors pendant l'échange : bip
  e.poke('ms', 2); e.run(1); log.length = 0;
  s16('bx', 40 * 64); s16('by', 100 * 64); e.poke('end_reason', 2); e.poke('ump_arm', 1); e.poke('ms', 3); e.poke('ms_timer', 80); e.run(2);
  const beep = has(0x4002, 0x5C);
  // ovation (priorité 2) puis rebond (priorité 1) : le rebond ne la coupe pas
  e.poke('ms', 2); e.run(1); log.length = 0;
  e.poke('crowd_timer', 0); e.poke('ms', 3); e.run(1);
  e.poke('noise_hold', 40); e.poke('noise_pri', 2); log.length = 0;
  s16('bz', 1 * 64); s16('bvz', -64); e.poke('b_live', 0); e.run(3);
  const kept = !has(0x400E, 0x0A);       // rebond sur dur : période $0A
  // plus de crissement (supprimé à la demande) : un demi-tour en pleine course sur dur reste muet sur le carré 2
  e.poke('ms', 2); e.poke('b_live', 1); e.poke('hitter', 0); log.length = 0;
  e.down('RIGHT'); e.run(10); e.up('RIGHT'); e.down('LEFT'); e.run(10); e.up('LEFT');
  const quiet = !has(0x4007) && !has(0x4005);
  check(whoosh && pok && beep && kept && quiet, `bruitages : souffle${whoosh ? '' : ' (absent)'}, « pok » au triangle${pok ? '' : ' (absent)'}, bip dehors${beep ? '' : ' (absent)'}, ovation non coupée${kept ? '' : ' (coupée)'}, pas de crissement${quiet ? '' : ' (présent)'}`);
}

{
  // service faux : bip du juge électronique, comme une balle dehors
  // (un set complet : en format rapide, il arrive qu'aucun service ne soit faux)
  const e = boot(); startWith(e, { FMT: 0 }); allCpu(e);
  const mm = e.nes.mmap, w = mm.write.bind(mm); let beep = false;
  mm.write = (a, v) => { if (a === 0x4002 && v === 0x5C) beep = true; return w(a, v); };
  let faults = 0, beeps = 0, prev = e.peek('ms');
  for (let f = 0; f < 60 * 60 * 12 && faults < 3 && app(e) === AS.MATCH; f++) {
    beep = false; const srv = e.peek('b_serve'); e.run(1);
    const ms = e.peek('ms');
    if (prev !== 3 && ms === 3 && srv && e.peek('rally_hits') <= 1 && e.peek('ump_arm')) { faults++; if (beep) beeps++; }
    prev = ms;
  }
  check(faults > 0 && beeps === faults, `service faux : bip (${beeps}/${faults})`);
}
}
if (sec('Service canon')) {
console.log('Service canon');
{
  // frappe A k trames après le lancer : seules 2 trames consécutives (le sommet) donnent un service canon
  const trial = (k, lv = 1) => {
    const e = boot(); startWith(e, { FMT: 3, LEVEL: lv });
    runUntil(e, () => e.peek('ms') === 0 && e.peek('sc_server') === 0, 600);
    // la première trame d'un point (mise en place du service) déborde : un
    // appui d'une seule trame peut y être perdu ; un joueur appuie plus longtemps
    e.run(6);
    e.poke('ms_timer', 200);
    e.tap('A', 1, 1, 0);
    runUntil(e, () => e.peek('ms') === 1, 30);
    e.run(k - 1); e.tap('A', 1, 1, 0); e.run(3);
    return { canon: e.peek('srv_canon'), vy: Math.abs(e.peekS16('bvy')), msg: msgText(e), ok: e.peek('st_canon', 0), tried: e.peek('st_ctry', 0) };
  };
  const res = []; for (let k = 22; k <= 34; k++) res.push([k, trial(k)]);
  const hits = res.filter(([, r]) => r.canon).map(([k]) => k);
  check(hits.length === 2 && hits[1] === hits[0] + 1, `fenêtre du service canon : 2 trames consécutives (${hits.join(', ')})`);
  const c = res.find(([, r]) => r.canon)[1], n = res.find(([k, r]) => !r.canon && k === hits[0] - 3)[1];
  check(c.vy > n.vy * 1.4 && c.msg === 'SERVICE CANON' && c.ok === 1 && c.tried === 1, `service canon plus rapide (${c.vy} contre ${n.vy}), annoncé et compté`);
  // le CPU Normal ne le réussit jamais
  const f = boot(); startWith(f, { FMT: 0, LEVEL: 1 }); allCpu(f);   // (un set : assez de services)
  let cpu = 0, serves = 0, prev = 0;
  runUntil(f, () => { const m = f.peek('ms'); if (m === 2 && prev === 1) { serves++; cpu += f.peek('srv_canon'); } prev = m; return serves >= 40 || app(f) !== AS.MATCH; }, 60 * 60 * 10);
  check(serves >= 20 && cpu === 0, `CPU Normal : aucun service canon sur ${serves} services`);
}

}
if (sec('Frappe parfaite')) {
console.log('Frappe parfaite');
{
  // balle qui arrive sur le joueur du bas après rebond ; A maintenu puis relâché k trames avant
  // son entrée dans la zone de frappe : seules 3 trames consécutives donnent une frappe parfaite
  const trial = (k, x = 138) => {
    const e = boot(); startWith(e, { FMT: 3, SURF: 1 });   // gazon : le joueur ne glisse pas après sa mise en place
    runUntil(e, () => e.peek('ms') === 0, 300);
    const s16 = (n, v, o = 0) => { e.poke(n, v & 255, o); e.poke(n, (v >> 8) & 255, o + 1); };
    e.poke('ms', 2); e.poke('b_live', 1); e.poke('b_vis', 1); e.poke('b_serve', 0); e.poke('bounces', 1);
    e.poke('hitter', 1); e.poke('hitter_p', 1); e.poke('rally_hits', 3); e.poke('bgrav', 0);
    s16('plx', 128 * 64, 0); s16('ply', 200 * 64, 0);
    s16('bx', x * 64); s16('by', 155 * 64); s16('bz', 6 * 64); s16('bvx', 0); s16('bvy', 128); s16('bvz', 0);
    // la balle entre dans la zone (dy >= -15) quand by >= 185 : 15 trames
    e.down('A'); e.run(Math.max(1, 15 - k)); e.up('A');
    for (let i = 0; i < 40 && e.peek('hitter') !== 0; i++) e.run(1);
    return { hit: e.peek('hitter') === 0, perf: e.peek('hit_perf'), vy: Math.abs(e.peekS16('bvy')), n: e.peek('st_perf', 0), tries: e.peek('st_ptry', 0) };
  };
  const res = []; for (let k = 0; k <= 8; k++) res.push([k, trial(k)]);
  const ks = res.filter(([, r]) => r.perf).map(([k]) => k);
  check(ks.length === 3 && ks[2] === ks[0] + 2 && res.every(([, r]) => r.hit), `fenêtre de la frappe parfaite : 3 trames consécutives (relâché ${ks.join(', ')} trames avant)`);
  const p = res.find(([, r]) => r.perf)[1];
  check(p.n === 1 && p.tries === 1, 'frappe parfaite comptée');
  // nettement plus rapide qu'une frappe normale relâchée une trame plus tôt
  const nrm = res.find(([k]) => k === ks[2] + 1)[1];
  check(p.vy > nrm.vy * 1.3, `frappe parfaite plus puissante (${p.vy} contre ${nrm.vy}, +${Math.round((p.vy / nrm.vy - 1) * 100)} %)`);
  // fatigue (échange de plus de 20 frappes) : frappe normale plus rapide
  const tiredHit = (rh) => {
    const e = boot(); startWith(e, { FMT: 3 });
    runUntil(e, () => e.peek('ms') === 0, 300);
    const s16 = (n, v, o = 0) => { e.poke(n, v & 255, o); e.poke(n, (v >> 8) & 255, o + 1); };
    e.poke('ms', 2); e.poke('b_live', 1); e.poke('b_vis', 1); e.poke('b_serve', 0); e.poke('bounces', 1);
    e.poke('hitter', 1); e.poke('hitter_p', 1); e.poke('rally_hits', rh); e.poke('bgrav', 0);
    s16('plx', 128 * 64, 0); s16('ply', 200 * 64, 0);
    s16('bx', 138 * 64); s16('by', 155 * 64); s16('bz', 6 * 64); s16('bvx', 0); s16('bvy', 128); s16('bvz', 0);
    e.down('A'); e.run(14);
    for (let i = 0; i < 40 && e.peek('hitter') !== 0; i++) e.run(1);
    e.up('A');
    return { vy: Math.abs(e.peekS16('bvy')), perf: e.peek('hit_perf') };
  };
  const fresh = tiredHit(3), worn = tiredHit(25);
  check(!fresh.perf && !worn.perf && worn.vy > fresh.vy * 1.15, `fatigue : frappe plus rapide en fin de long échange (${worn.vy} contre ${fresh.vy})`);
  // HAUT / BAS relatifs au joueur : « vers l'adversaire » = coup long, « vers
  // soi » = coup court, que le joueur soit en bas ou en haut de l'écran
  const shot = (top, dir, btn = 'A') => {
    const e = boot(); startWith(e, { FMT: 3 });
    runUntil(e, () => e.peek('ms') === 0, 300);
    const s16 = (n, v, o = 0) => { e.poke(n, v & 255, o); e.poke(n, (v >> 8) & 255, o + 1); };
    e.poke('sc_swap', top ? 1 : 0);
    e.poke('ms', 2); e.poke('b_live', 1); e.poke('b_vis', 1); e.poke('b_serve', 0); e.poke('bounces', 1);
    e.poke('hitter', 1); e.poke('hitter_p', 1); e.poke('rally_hits', 3); e.poke('bgrav', 0);
    s16('plx', 128 * 64, 0); s16('ply', (top ? 40 : 200) * 64, 0);
    s16('bx', 138 * 64); s16('by', (top ? 90 : 155) * 64); s16('bz', 6 * 64); s16('bvx', 0); s16('bvy', top ? -128 : 128); s16('bvz', 0);
    e.down(btn); e.run(top ? 17 : 12); e.down(dir);
    for (let i = 0; i < 40 && e.peek('hitter') !== 0; i++) e.run(1);
    e.releaseAll();
    return e.peek('last_shot');
  };
  const sb = [shot(0, 'UP'), shot(0, 'DOWN'), shot(1, 'UP'), shot(1, 'DOWN')];
  check(sb[0] === 4 && sb[1] === 5 && sb[2] === 5 && sb[3] === 4,
    `commandes relatives : en bas, HAUT à plat / BAS croisé court ; en haut, HAUT croisé court / BAS à plat (${sb.join(',')})`);
  const bb = [shot(0, 'UP', 'B'), shot(0, 'DOWN', 'B'), shot(1, 'UP', 'B'), shot(1, 'DOWN', 'B')];
  check(bb[0] === 2 && bb[1] === 3 && bb[2] === 3 && bb[3] === 2,
    `commandes relatives avec B : en bas, HAUT lob / BAS amortie ; en haut, l'inverse (${bb.join(',')})`);
  const collee = trial(ks[1], 130), ext = trial(ks[1], 148);
  check(!collee.perf && !ext.perf && collee.hit && ext.hit, 'balle collée au corps ou en extension : frappe normale');
}

}
if (sec('Entraînement : retour sur le timing')) {
console.log('Entraînement : retour sur le timing');
{
  const msg2 = (e) => { let t = ''; for (let i = 0; i < 14; i++) t += String.fromCharCode(e.peek('msg2', i)); return t.replace(/\0/g, ' ').trim(); };
  // service (exercice SERVICE) : frappe A k trames après le lancer
  const serve = (k) => {
    const e = boot(); startWith(e, { MODE: MODE.TRAIN, EX: 5 });
    runUntil(e, () => e.peek('ms') === 0 && e.peek('sc_server') === 0, 600);
    e.tap('A', 1, 1, 0); runUntil(e, () => e.peek('ms') === 1, 30);
    e.run(k - 1); e.tap('A', 1, 1, 0); e.run(2);
    return msgText(e) + ' / ' + msg2(e);
  };
  const s1 = serve(25), s2 = serve(31), s3 = serve(28);
  check(s1 === 'TROP TOT DE / 3 IMAGES' && s2 === 'TROP TARD DE / 2 IMAGES' && s3.startsWith('SERVICE CANON'), `service : « ${s1} », « ${s2} », « ${s3} »`);
  // coup droit : relâché 5 trames avant l'arrivée, ou bouton gardé enfoncé, ou balle collée
  const stroke = (k, x = 138) => {
    const e = boot(); startWith(e, { MODE: MODE.TRAIN, EX: 6 });
    runUntil(e, () => e.peek('ms') === 2 || e.peek('ms') === 0, 300);
    const s16 = (n, v, o = 0) => { e.poke(n, v & 255, o); e.poke(n, (v >> 8) & 255, o + 1); };
    e.poke('ms', 2); e.poke('b_live', 1); e.poke('b_vis', 1); e.poke('b_serve', 0); e.poke('bounces', 1);
    e.poke('hitter', 1); e.poke('hitter_p', 1); e.poke('rally_hits', 3); e.poke('bgrav', 0);
    s16('plx', 128 * 64, 0); s16('ply', 200 * 64, 0);
    s16('bx', x * 64); s16('by', 155 * 64); s16('bz', 6 * 64); s16('bvx', 0); s16('bvy', 128); s16('bvz', 0);
    e.down('A'); e.run(Math.max(1, 15 - k)); if (k >= 0) e.up('A');
    for (let i = 0; i < 40 && e.peek('hitter') !== 0; i++) e.run(1);
    e.releaseAll(); e.run(1);
    return msgText(e) + ' / ' + msg2(e);
  };
  const a = stroke(5), b = stroke(-1), c = stroke(1, 131), d = stroke(1);
  check(a === 'TROP TOT DE / 3 IMAGES' && b === 'RELACHEZ LE / BOUTON AVANT' && c === 'BALLE TROP / PRES DU CORPS' && d.startsWith('COUP PARFAIT'),
    `coup droit : « ${a} », « ${b} », « ${c} », « ${d} »`);
}

}
if (sec('Fondus entre les écrans')) {
console.log('Fondus entre les écrans');
{
  const e = boot(); e.run(20);
  const seen = [];
  e.down('START'); e.run(1); e.up('START');
  for (let i = 0; i < 30; i++) { e.run(1); seen.push(e.peek('fade_lvl')); }
  const max = Math.max(...seen), last = seen[seen.length - 1];
  const down = seen.indexOf(max), steps = new Set(seen.slice(down));
  check(max === 64 && last === 0 && steps.size >= 4 && scr(e) === SS.MENU, `fondu au noir puis retour progressif (${[...new Set(seen)].join(' > ')})`);
}

}
if (sec('Récapitulatif du score')) {
console.log('Récapitulatif du score');
{
  // match en 3 sets ; on avance le score à 5-0 pour atteindre vite la fin du 1er set
  const e = boot(); startWith(e, { FMT: 1 }); allCpu(e);
  runUntil(e, () => e.peek('ms') === 0, 300);
  // balle de set (5-0, 40-0) remise à chaque service jusqu'à la fin du 1er set
  runUntil(e, () => {
    if (app(e) === AS.MATCH && e.peek('ms') === 0 && e.peek('sc_set_no') === 0) {
      e.poke('sc_games', 5, 0); e.poke('sc_games', 0, 1); e.poke('sc_points', 3, 0); e.poke('sc_points', 0, 1);
    }
    return scr(e) === SS.SETBREAK && app(e) === AS.SCREENS;
  }, 60 * 60 * 4);
  e.run(30);
  const tx = (x, y, n) => e.text(x, y, n).replace(/\0/g, ' ');
  check(scr(e) === SS.SETBREAK && tx(8, 4, 12) === '1ER SET POUR' && tx(8, 7, 4) === 'SETS', 'fin du 1er set : écran récapitulatif (titre, tableau des sets, statistiques)');
  const l0 = tx(8, 7, 9), l1 = tx(8, 8, 9);
  check(/^[A-Z]{4}\s+\d/.test(l0) && /^[A-Z]{4}\s+\d/.test(l1), `tableau des sets : « ${l0.trim()} » / « ${l1.trim()} »`);
  check(e.peek('music_cur') === 8, 'petite musique sur l\'écran de fin de set');
  // n'importe quel bouton (ici B) ferme l'écran aussitôt, sans fondu
  e.tap('B', 1, 1, 0); let n = 0;
  while (app(e) !== AS.MATCH && n < 30) { e.run(1); n++; }
  check(app(e) === AS.MATCH && n <= 16 && e.peek('music_cur') === 0, `B : retour au court en ${n + 1} trames, musique coupée`);
  e.run(30);
  check(app(e) === AS.MATCH && /^[A-Z]{4} \d \d/.test(msgText(e)), `reprise du match, score des sets dans le bandeau : « ${msgText(e)} »`);
  // un jeu plus tard : (2.76) plus de rappel des jeux en haut (le bandeau les donne)
  let after = '', played = false;
  runUntil(e, () => {
    if (e.peek('ms') === 0 && e.peek('sc_games', 0) + e.peek('sc_games', 1) === 0) { e.poke('sc_points', 3, 0); e.poke('sc_points', 0, 1); }
    const t = msgText(e); if (/^[A-Z]{4} \d \d/.test(t) && e.peek('sc_games', 0) + e.peek('sc_games', 1) > 0) after = t;
    if (e.peek('sc_games', 0) + e.peek('sc_games', 1) > 0 && e.peek('ms') === 2) played = true;
    return played;
  }, 60 * 60 * 3);
  check(played && after === '', `après un jeu : pas de rappel des jeux en haut (« ${after} »)`);
}

}
if (sec('Bandeau devant le joueur du haut')) {
console.log('Bandeau devant le joueur du haut');
{
  const e = boot(); startWith(e, { FMT: 3 }); allCpu(e);
  let ok = false, bad = false;
  runUntil(e, () => {
    if (e.peek('ms') === 0 && e.peekS16('ply', 2) / 64 < 50) {
      e.run(2);
      for (let i = 0; i < 64; i++) {
        const y = e.peek('oam_buf', i * 4) + 1, a = e.peek('oam_buf', i * 4 + 2);
        if (y >= 0xF0) continue;
        if (y + 7 <= 31 && (e.peek('oam_buf', i * 4 + 1) >= 0x10) && (a & 0x20)) ok = true;   // tête dans le bandeau : derrière le texte
        if (y >= 32 && (a & 0x20)) bad = true;                                               // jamais derrière le court
      }
      return ok;
    }
    return false;
  }, 60 * 60 * 3);
  check(ok && !bad, 'joueur du haut au service : sa tête passe derrière le texte du bandeau, le reste du sprite devant le court');
}

}
if (sec('Panneaux : tuiles empruntées rétablies')) {
console.log('Panneaux : tuiles empruntées rétablies');
{
  // club nudiste, puis stade dans la même partie : les tuiles du panneau retrouvent leur dessin
  const e = boot(); e.poke('nude_unlocked', 1);
  startWith(e, { FMT: 3, ENV: 3 }); allCpu(e); e.run(60);
  const rb = (sym, off) => e.nes.rom.rom[3][e.addr(sym) - 0x8000 + off];
  const idx = [...Array(8).keys()].map((k) => rb('sign2_idx', k));
  const chr = (src) => { for (let b = 0; b < 128; b++) if (e.nes.ppu.vramMem[idx[b >> 4] * 16 + (b & 15)] !== rb(src, b)) return false; return true; };
  const atNude = chr('sign2_chr');
  e.tap('START', 1, 1, 2); e.tap('SELECT', 1, 1, 2);
  runUntil(e, () => app(e) === AS.SCREENS, 120); e.run(30);
  setOpts(e, { ENV: 0 }); e.tap('START'); e.run(40);
  if (scr(e) === SS.PRESENT) { e.tap('A'); e.run(70); }
  check(atNude && app(e) === AS.MATCH && chr('sign2_orig'), 'club nudiste puis stade : décor d\'origine rétabli');
}

}
if (sec('Filet qui tremble')) {
console.log('Filet qui tremble');
{
  const e = boot(); startWith(e, { FMT: 3 }); allCpu(e);
  runUntil(e, () => e.peek('ms') === 0, 300);
  const s16 = (n, v, o = 0) => { e.poke(n, v & 255, o); e.poke(n, (v >> 8) & 255, o + 1); };
  // balle basse qui arrive dans le filet
  e.poke('ms', 2); e.poke('b_live', 1); e.poke('b_vis', 1); e.poke('b_serve', 0); e.poke('bounces', 0);
  e.poke('hitter', 0); e.poke('rally_hits', 3); e.poke('bgrav', 0);
  s16('bx', 100 * 64); s16('by', 140 * 64); s16('bz', 2 * 64); s16('bvx', 0); s16('bvy', -128); s16('bvz', 0);
  runUntil(e, () => e.peek('net_shake') > 0, 30);
  const rb = (sym, off) => e.nes.rom.rom[3][e.addr(sym) - 0x8000 + off];
  const ni = rb('net_idx', 0);
  let shaken = false;
  for (let f = 0; f < 16; f++) {
    e.run(1);
    let same = true; for (let b = 0; b < 16; b++) if (e.nes.ppu.vramMem[ni * 16 + b] !== rb('net_shk', b)) same = false;
    if (same) shaken = true;
  }
  e.run(10);
  let rest = true; for (let b = 0; b < 16; b++) if (e.nes.ppu.vramMem[ni * 16 + b] !== rb('net_norm', b)) rest = false;
  check(shaken && rest && e.peek('net_shake') === 0, 'balle dans le filet : les mailles tremblent puis reviennent au repos');
}
{
  // perspective (2.68) : les tuiles du filet listées par le générateur (banque
  // 16) prennent leur dessin secoué, puis reviennent au repos, dans 5 lieux
  let bad = [];
  for (const env of [0, 1, 2, 4, 5]) {
    const e = boot(); startWith(e, { FMT: 3, VIEW: 1, ENV: env }); allCpu(e);
    runUntil(e, () => e.peek('ms') === 0, 300);
    const rb = (off) => e.nes.rom.rom[16][off];
    const tab = e.addr('persp_net') - 0x8000, ptr = rb(tab + env * 2) | (rb(tab + env * 2 + 1) << 8);
    const base = ptr - 0x8000, n = rb(base), vm = e.nes.ppu.vramMem;
    const state = (k) => { let ok = 0; for (let i = 0; i < n; i++) { const t = rb(base + 1 + i * 33); let same = true; for (let b = 0; b < 16; b++) if (vm[t * 16 + b] !== rb(base + 2 + i * 33 + k * 16 + b)) same = false; if (same) ok++; } return ok; };
    const rest0 = state(0);
    e.poke('net_shake', 14);
    let shaken = 0; for (let f = 0; f < 16; f++) { e.run(1); shaken = Math.max(shaken, state(1)); }
    e.run(4);
    if (rest0 !== n || shaken !== n || state(0) !== n || e.peek('net_shake') !== 0) bad.push(`${env}:${rest0}/${shaken}/${state(0)}/${n}`);
  }
  check(bad.length === 0, `perspective : le filet tremble (toutes ses tuiles secouées), puis revient au repos, dans 5 lieux ${bad.join(' ')}`);
}

}
if (sec('Rotation des sprites (double)')) {
console.log('Rotation des sprites (double)');
{
  // 4 joueurs près du filet : jusqu'à 12 sprites sur une ligne. Avec la rotation, chaque
  // sprite est parmi les 8 premiers de ses lignes au moins une trame sur deux
  const e = boot(); startWith(e, { MODE: MODE.P2, FMT: 3, TYPE: 1 }); allCpu(e);
  runUntil(e, () => e.peek('ms') === 0, 300);
  e.tap('START', 1, 1, 2);
  const s16 = (n, v, o = 0) => { e.poke(n, v & 255, o); e.poke(n, (v >> 8) & 255, o + 1); };
  // (2.75 : sous le bandeau de pause, rangées 12 à 17, les sprites sont cachés : joueurs plus bas)
  [[96, 186], [104, 176], [150, 184], [158, 178]].forEach(([x, y], i) => { s16('plx', x * 64, i * 2); s16('ply', y * 64, i * 2); });
  const seen = new Map(); let rot = new Set(), maxLine = 0, frames = 0;
  for (let f = 0; f < 8; f++) {
    e.run(1); frames++;
    rot.add(e.peek('spr_rot'));
    const spr = []; for (let i = 0; i < 64; i++) { const y = e.peek('oam_buf', i * 4); if (y < 0xEF) spr.push({ y: y + 1, key: e.peek('oam_buf', i * 4 + 1) + ',' + e.peek('oam_buf', i * 4 + 3) }); }
    const count = new Array(256).fill(0), vis = new Set();
    for (const sp of spr) { let ok = true; for (let l = sp.y; l < sp.y + 8 && l < 256; l++) { if (count[l] >= 8) ok = false; count[l]++; } if (ok) vis.add(sp.key); seen.set(sp.key, (seen.get(sp.key) || 0) + (ok ? 1 : 0)); }
    maxLine = Math.max(maxLine, ...count);
  }
  const never = [...seen].filter(([, n]) => n === 0).length;
  check(rot.size > 4 && maxLine > 8 && never === 0, `rotation active (${rot.size} départs différents), jusqu'à ${maxLine} sprites par ligne, aucun sprite masqué en permanence`);
  // en simple : pas de rotation
  const g = boot(); startWith(g, { FMT: 3 }); allCpu(g); g.run(200);
  check(g.peek('spr_rot') === 0, 'simple : ordre des sprites fixe (pas de rotation inutile)');
}

}
if (sec('Victoire : sauts de joie et confettis')) {
console.log('Victoire : sauts de joie et confettis');
{
  const confetti = boot().peek('confetti_tile');
  // le joueur humain (équipe 0) gagne : confettis et bonds du vainqueur
  const e = boot(); startWith(e, { MODE: MODE.P1 }); e.run(60);
  e.poke('sc_winner', 0); e.poke('ms_timer', 160); e.poke('ms', 4); e.poke('hl_fin', 1); e.run(2);
  let conf = 0, ys = new Set();
  for (let n = 0; n < 40; n++) {
    e.run(1);
    const t = oamTiles(e); if (t.has(confetti) && t.has(3)) conf++;
    let top = 255; for (let i = 0; i < 64; i++) { const y = e.peek('oam_buf', i * 4), tl = e.peek('oam_buf', i * 4 + 1); if (y < 0xEF && tl !== confetti && tl !== 3 && y > 120 && e.peek('oam_buf', i * 4 + 3) >= 40) top = Math.min(top, y); /* hors arbitre (x < 40) */ }
    ys.add(top);
  }
  check(conf >= 38, `victoire d'un humain : pluie d'étoiles et de confettis (${conf}/40 trames)`);
  check(ys.size >= 4, `le vainqueur (en bas) sautille de joie (${ys.size} hauteurs différentes)`);
  check(e.peek('app_state') === AS.MATCH, 'la fête ne bloque pas la fin du match');
  runUntil(e, () => e.peek('app_state') !== AS.MATCH, 300);
  check(e.peek('app_state') === AS.SCREENS, 'puis écran des résultats');
  // fanfare de victoire : exactement la durée de la fête (160 trames), mélodie et basse
  const dur = (sym) => { const b10 = e.nes.rom.rom[10]; let a = e.addr(sym) - 0x8000, n = 0; while (b10[a] !== 0xFF) { n += b10[a + 1]; a += 2; } return n; };
  check(dur('mus_vict_mel') === 160 && dur('mus_vict_bass') === 160, `fanfare de victoire : ${dur('mus_vict_mel')}/${dur('mus_vict_bass')} trames (durée de la fête)`);
  // feux d'artifice : 3 gerbes de 4 étoiles qui s'écartent
  const g = boot(); startWith(g, { MODE: MODE.P1 }); g.run(60);
  g.poke('sc_winner', 0); g.poke('ms_timer', 160); g.poke('ms', 4); g.poke('hl_fin', 1);
  let bursts = 0, prev = 0;
  for (let n = 0; n < 158; n++) {
    g.run(1);
    let st = 0; for (let i = 0; i < 64; i++) if (g.peek('oam_buf', i * 4) < 0xEF && g.peek('oam_buf', i * 4 + 1) === 3) st++;
    if (st >= 9 && prev < 9) bursts++;
    prev = st;
  }
  check(bursts === 3, `3 feux d'artifice pendant la fête (${bursts})`);
  // double à 4 joueurs (cas le plus chargé) : aucune trame perdue pendant la fête
  const d = boot(); startWith(d, { MODE: MODE.P2, TYPE: 1, FMT: 3 }); allCpu(d);
  runUntil(d, () => d.peek('ms') === 0, 200);
  d.poke('sc_winner', 0); d.poke('ms', 4); d.poke('hl_fin', 1); d.poke('ms_timer', 250); d.run(120);
  check(250 - d.peek('ms_timer') >= 118, `fête en double : ${250 - d.peek('ms_timer')} trames logiques sur 120 (au plus 1 perdue, invisible)`);
  // le CPU bat le joueur humain : pas de fête
  const f = boot(); startWith(f, { MODE: MODE.P1 }); f.run(60);
  f.poke('sc_winner', 1); f.poke('ms_timer', 160); f.poke('ms', 4); f.poke('hl_fin', 1); f.run(10);
  check(!oamTiles(f).has(confetti), 'défaite contre le CPU : pas de confettis');
}
}
if (sec('Balle : couleur, ombre, rebond, effet, filet')) {
console.log('Balle : couleur, ombre, rebond, effet, filet');
{
  const ballSpr = (e) => { const r = []; for (let i = 0; i < 64; i++) { const y = e.peek('oam_buf', i * 4); if (y < 0xEF) r.push({ t: e.peek('oam_buf', i * 4 + 1), a: e.peek('oam_buf', i * 4 + 2) & 3, x: e.peek('oam_buf', i * 4 + 3), y: y + 1 }); } return r; };
  const setBall = (e, x, y, z, vz, vy, spin) => {
    const s16 = (n, v) => { e.poke(n, v & 255); e.poke(n, (v >> 8) & 255, 1); };
    e.poke('ms', 2); e.poke('b_live', 0); e.poke('b_vis', 1); e.poke('bounces', 1); e.poke('bgrav', 0); e.poke('bspin', spin);
    s16('bx', x * 64); s16('by', y * 64); s16('bz', z * 64); s16('bvx', 0); s16('bvy', vy); s16('bvz', vz);
  };
  // couleur selon le court : jaune de balle de tennis, vert-jaune sur le sable
  const cols = [];
  for (const surf of [0, 1, 2, 3, 4, 5]) { const e = boot(); startWith(e, { SURF: surf, ENV: 0, FMT: 3 }); cols.push(e.peek('pal_buf', 16 + 13)); }
  const m = boot(); startWith(m, { ENV: 4, FMT: 3 }); cols.push(m.peek('pal_buf', 16 + 13));
  check(cols.join() === [0x28, 0x28, 0x28, 0x28, 0x2A, 0x28, 0x28].join(), `couleur de la balle par court : ${cols.map((c) => c.toString(16))}`);
  // ombre de plus en plus petite avec la hauteur ; balle sur la palette des effets
  const e = boot(); startWith(e, { SURF: 2, FMT: 3 });
  runUntil(e, () => e.peek('ms') === 0, 300); e.run(10);
  const base = e.peek('ball_base');
  const sh = []; let pal3 = true;
  for (const z of [4, 20, 45]) { setBall(e, 150, 190, z, 0, 0, 0); e.run(3); const sp = ballSpr(e);
    const b = sp.find((q) => q.a === 3); if (!b || b.a !== 3) pal3 = false;
    sh.push(sp.some((q) => q.t === 2 && q.a === 2) ? 'G' : sp.some((q) => q.t === base + 15) ? 'M' : sp.some((q) => q.t === base + 16) ? 'P' : '?'); }
  check(sh.join('') === 'GMP' && pal3, `ombre qui rétrécit quand la balle monte (${sh.join(' ')}), balle sur la palette des effets`);
  // 4 tailles : 4, 5, 6 et 7 px (grossit en douceur ; la grande pour les vrais lobs)
  const sz = [];
  for (const z of [4, 15, 25, 45]) { setBall(e, 150, 190, z, 0, 0, 0); e.run(3); const b = ballSpr(e).find((q) => q.a === 3);
    const t = b ? b.t - base : -1; sz.push(t < 4 ? 4 : t < 8 ? 5 : t >= 17 && t < 21 ? 6 : t < 12 ? 7 : 0); }
  check(sz.join() === '4,5,6,7', `balle de ${sz.join(', ')} px selon la hauteur (4, 15, 25, 45 px)`);
  // rebond : balle écrasée
  setBall(e, 150, 190, 8, -40, 0, 0); e.poke('bgrav', 4);
  let sq = 0; for (let f = 0; f < 20; f++) { e.run(1); if (ballSpr(e).some((q) => q.t === base + 12)) sq++; }
  check(sq >= 2 && sq <= 4, `balle écrasée au rebond (${sq} trames)`);
  // effet : la couture tourne avec le lifté, pas avec la balle à plat
  const turn = (spin) => { setBall(e, 150, 190, 15, 0, -64, spin); const seen = new Set();
    for (let f = 0; f < 16; f++) { e.run(1); for (const q of ballSpr(e)) if (q.t >= base + 4 && q.t < base + 8) seen.add(q.t); } return seen.size; };
  const top = turn(1), flat = turn(0);
  check(top === 4 && flat === 1, `couture qui tourne selon l'effet (lifté : ${top} positions, à plat : ${flat})`);
  // camp du haut, à hauteur du filet : balle vue à travers les mailles ; dans le camp du bas : devant
  setBall(e, 140, 129, 2, 0, 0, 0); e.run(3); const behind = ballSpr(e).some((q) => q.t === base + 13);
  setBall(e, 140, 136, 4, 0, 0, 0); e.run(3); const front = ballSpr(e).some((q) => q.t >= base && q.t < base + 4);
  check(behind && front, 'balle derrière le filet vue à travers les mailles (camp du haut), devant dans le camp du bas');
}
}
if (sec('Animation des joueurs')) {
console.log('Animation des joueurs');
{
  // tuiles : réserve commune (rangées partagées), versions normale et au naturel de même taille
  const e = boot();
  const nb = e.peek16('people_bytes');
  let okm = true;
  // (version 2.43 : rangées des célébrations et du revers dans la zone partagée, swap_base)
  const rb3 = (sym, off = 0) => e.nes.rom.rom[3][e.addr(sym) - 0x8000 + off];
  const sb = rb3('swap_base'), inPool = (t) => (t >= 0x10 && t + 1 < 0x10 + nb / 16) || (t >= sb && t + 1 < sb + 34);
  // (version 2.54 : 32 poses ; 5 images de profil, la 5e — assis — dans la zone partagée)
  for (const mp of ['person_map_m', 'person_map_f']) for (let i = 0; i < 192; i++) { if (!inPool(e.peek(mp, i))) okm = false; }
  for (let i = 0; i < 30; i++) { if (!inPool(e.peek('prof_map', i))) okm = false; }
  check(okm && nb % 32 === 0, `poses dans la réserve de tuiles des joueurs (${nb / 16} tuiles permanentes et 34 partagées pour 32 poses x 4 vues + 5 images de profil)`);
  // match CPU : poses jouées, ordre du service, split-step, rebond d'attente
  startWith(e, { FMT: 3 }); allCpu(e);
  const poses = new Set(), seq = [[], []];
  let split = 0, bob = [new Set(), new Set()];
  const headY = (p) => {
    const px = e.peekS16('plx', p * 2) >> 6, py = e.peekS16('ply', p * 2) >> 6;
    let m = 999;
    for (let i = 0; i < 64; i++) {
      const y = e.peek('oam_buf', i * 4), x = e.peek('oam_buf', i * 4 + 3);
      if (y < 0xEF && x >= px - 9 && x <= px + 1 && y + 1 > py - 30 && y + 1 < py) m = Math.min(m, y + 1);
    }
    return m === 999 ? null : m - py;
  };
  let last = [-1, -1], still = [0, 0], prev = [0, 0];
  // (jusqu'à 6 min si aucune volée ni amortie : depuis la 2.61, le CPU ne joue l'amortie que de l'intérieur du court)
  for (let f = 0; f < 60 * 60 * 3 || (f < 60 * 60 * 10 && !poses.has(13) && !poses.has(14)); f++) {
    /* (2.74) volée et amortie sont rares (selon le hasard du match) : après
       3 minutes sans elles, le joueur 0 monte au filet */
    if (f > 60 * 60 * 3 && e.peek('ms') === 2) e.poke('ai_net', 1, 0);
    e.run(1);
    for (let p = 0; p < 2; p++) {
      const k = e.peek('pose_cache', p) & 31;
      poses.add(k);
      if (k !== last[p]) { seq[p].push(k); last[p] = k; }
      const j = e.peek('pl_jump', p);
      if (j === 6 && (k === 0 || k === 6) && e.peek('ms') === 2) split++;
      const pos = e.peekS16('plx', p * 2) * 65536 + e.peekS16('ply', p * 2);
      still[p] = pos === prev[p] ? still[p] + 1 : 0; prev[p] = pos;
      if (still[p] > 2 && k === 0 && j === 0 && e.peek('ms') === 3 && e.peek('ms_timer') > 62) { const h = headY(p); if (h !== null) bob[p].add(h); }
    }
  }
  const has = (a, b) => a.join(',').includes(b);
  check((poses.has(8) || poses.has(22)) && poses.has(23) && poses.has(24) && poses.has(10), `revers à deux mains (préparation, impact, fin) et impact du service joués (${[...poses].sort((a, b) => a - b)})`);
  check([11, 12, 15, 19, 20].every((k) => poses.has(k)) && [14, 13].some((k) => poses.has(k)), `préparation, impact, volée ou amortie, rebonds avant le service, course en 4 temps (manque : ${[11, 12, 15, 19, 20, 13, 14].filter((k) => !poses.has(k))})`);
  check([16, 17, 18].filter((k) => poses.has(k)).length >= 2 && poses.has(7), 'après le point : joie, déception, colère ou fatigue');
  check(has(seq[0], '5,10,4') || has(seq[1], '5,10,4'), 'service en 3 temps : lancer, impact bras tendu, accompagnement');
  check(split > 3, `split-step quand l'adversaire frappe (${split})`);
  const okb = [0, 1].some((p) => { const v = [...bob[p]]; return v.length === 2 && Math.abs(v[0] - v[1]) === 1; });
  check(okb, `rebond d'attente de 1 px (${[...bob[0]]} / ${[...bob[1]]})`);
  // course en 4 temps : deux foulées, chacune basse puis relevée d'1 px
  const r = boot(); startWith(r, { FMT: 3 });
  runUntil(r, () => r.peek('ms') === 0, 300);
  r.poke('ms', 2); r.poke('b_live', 1); r.poke('b_vis', 0); r.poke('hitter', 0);
  const hs = new Set(), rp = new Set();
  r.down('RIGHT');
  for (let f = 0; f < 32; f++) { r.run(1); rp.add(r.peek('pose_cache', 0) & 31);
    const px = r.peekS16('plx', 0) >> 6, py = r.peekS16('ply', 0) >> 6;
    let m = 999; for (let i = 0; i < 64; i++) { const y = r.peek('oam_buf', i * 4), x = r.peek('oam_buf', i * 4 + 3);
      if (y < 0xEF && x >= px - 9 && x <= px + 1 && y + 1 > py - 30 && y + 1 < py) m = Math.min(m, y + 1); }
    if (m !== 999) hs.add(m - py); }
  r.up('RIGHT');
  check(rp.has(1) && rp.has(2) && rp.has(19) && rp.has(20) && hs.size === 2, `course en 4 images (${[...rp].sort((a, b) => a - b)}) et 2 hauteurs (${[...hs]})`);
}
{
  // service : la balle rebondit à côté du serveur ; le receveur fléchit les genoux
  const e = boot(); startWith(e, { FMT: 3 }); runUntil(e, () => e.peek('ms') === 0, 300);
  const srv = e.peek('sc_server'), base = e.peek('ball_base');
  const hs = new Set(); let crouch = 0;
  for (let f = 0; f < 64; f++) { e.run(1);
    for (let i = 0; i < 64; i++) { const y = e.peek('oam_buf', i * 4), t = e.peek('oam_buf', i * 4 + 1); if (y < 0xEF && t === base && (e.peek('oam_buf', i * 4 + 2) & 3) === 3) hs.add(y); }
    if (e.peek('pose_now', srv ^ 1) === (6 | 64)) crouch++; }
  check(e.peek('pose_now', srv) === 15 && hs.size >= 6 && crouch > 10, `avant le service : balle qui rebondit (${hs.size} hauteurs), receveur genoux fléchis`);
  // ralenti : la pose est gardée sur 5 bits (poses jusqu'à 21)
  e.poke('pose_cache', 21, 0); e.poke('flip_cache', 1, 0);
  const rb = e.addr('rp_buf'), h = e.peek('rp_head'); e.nes.cpu.mem[0x4000]; 
  e.poke('ms', 2); e.run(2);
  let found = false; for (let i = 0; i < 160 * 16; i += 16) if ((e.nes.cpu.mem[rb + i + 6] & 31) <= 21) found = true;
  check(found, 'ralentis : poses sur 5 bits');
}
}
if (sec('IA : tactique selon le score')) {
console.log('IA : tactique selon le score');
{
  // mené de 3 jeux : le CPU prend des risques ; devant : il joue placé ;
  // balle de break contre lui au service : première balle assurée
  const e = boot(); startWith(e, { FMT: 0 }); allCpu(e);
  runUntil(e, () => e.peek('ms') === 2, 600);
  e.poke('sc_games', 1, 0); e.poke('sc_games', 4, 1);
  runUntil(e, () => e.peek('ms') === 3, 3000); runUntil(e, () => e.peek('ms') === 0, 1500);   /* (ralenti mis en valeur : plus long, 2.67) */
  const g0 = e.peek('sc_games', 0), g1 = e.peek('sc_games', 1);
  const pw = (t) => { const v = e.peek('tac_pow', t); return v > 127 ? v - 256 : v; };
  const mg = (t) => { const v = e.peek('tac_margin', t); return v > 127 ? v - 256 : v; };
  const trailing = g1 - g0 >= 2;
  check(trailing && pw(0) >= 5 && mg(0) < 0 && e.peek('tac_net', 0) >= 50 && mg(1) >= 3 && e.peek('tac_err', 1) === 0x10,
    `mené ${g0}-${g1} : le CPU attaque (force +${pw(0)}, marge ${mg(0)}, filet ${e.peek('tac_net', 0)}) ; le meneur joue placé (marge +${mg(1)})`);
  // balle de break contre le serveur
  const srv = e.peek('sc_server');
  e.poke('sc_points', 3, srv ^ 1); e.poke('sc_points', 1, srv); e.poke('sc_games', 2, 0); e.poke('sc_games', 2, 1);
  runUntil(e, () => e.peek('ms') === 2, 600); runUntil(e, () => e.peek('ms') === 3, 3000); runUntil(e, () => e.peek('ms') === 0, 1500);   /* (bandeau du score et ralenti : plus de 400 trames) */
  const s2 = e.peek('sc_server'), bp = e.peek('sc_points', s2 ^ 1) >= 3 && e.peek('sc_points', s2 ^ 1) > e.peek('sc_points', s2);
  check(!bp || e.peek('tac_safe', s2) === 1, `balle de break contre le serveur : première balle assurée (${bp ? 'balle de break' : 'situation passée'})`);
}
}
if (sec('Son sur console réelle : balayage des carrés')) {
console.log('Son sur console réelle : balayage des carrés');
{
  // Sur la vraie NES, l'unité de balayage d'un canal carré coupe la note si
  // sa période cible (période + période >> décalage, sens montant) dépasse
  // $7FF, même désactivée. jsnes ne le fait pas : on le vérifie ici sur toutes
  // les notes jouées (générique passé, menus, match, fête et feux d'artifice).
  const watch = (e) => { const mm = e.nes.mmap, w = mm.write.bind(mm), reg = {}, bad = [];
    mm.write = (a, v) => { if (a >= 0x4000 && a <= 0x4007) { reg[a] = v;
        if (a === 0x4003 || a === 0x4007) { const b = a - 3, per = reg[b + 2] | ((v & 7) << 8), sw = reg[b + 1] ?? 0;
          const tgt = (sw & 8) ? 0 : per + (per >> (sw & 7));
          if (per >= 8 && tgt > 0x7FF) bad.push(per); } }
      return w(a, v); };
    return bad; };
  const e = new Emu(rom); const bad = watch(e);
  e.run(40); e.tap('START', 1, 1, 0); e.run(4); e.tap('START', 1, 1, 0); e.run(60);   // « ARE YOU SURE? » (bourdon grave)
  toMenu(e); e.tap('START'); e.run(30); e.tap('A');
  runUntil(e, () => app(e) === AS.MATCH, 300); allCpu(e); e.run(1500);
  e.poke('sc_winner', 0); e.poke('ms_timer', 160); e.poke('ms', 4); e.poke('hl_fin', 1); e.run(170);   // feux d'artifice
  check(bad.length === 0, `aucune note coupée par l'unité de balayage sur console réelle${bad.length ? ' (périodes ' + [...new Set(bad)].join(', ') + ')' : ''}`);
}
}
if (sec('Vue en perspective (option VUE)')) {
console.log('Vue en perspective (option VUE)');
{
  // ralentissements (2.66) : double en perspective, 4 joueurs de l'ordinateur ;
  // une trame est en retard si le programme n'attend pas déjà la suivante
  // (1 056 sur 15 000 mesurées en 2.66, 2 942 en 2.63)
  const g = boot(); toMenu(g); setOpts(g, { MODE: 1, TYPE: 1, VIEW: 1, FMT: 3, REPLAY: 0 }); g.tap('START');   /* (sans ralentis : le jeu seul) */
  runUntil(g, () => scr(g) === SS.PRESENT, 120); g.run(40); g.tap('A');
  runUntil(g, () => app(g) === AS.MATCH, 300); allCpu(g);
  // (2.76) depuis le rattrapage (main.c : une trame perdue est rattrapée par
  // une trame logique sans dessin), ce qui compte est la vitesse du jeu :
  // trames logiques (match_tick) par trame affichée pendant les échanges
  const wf = g.addr('wait_frame'), mt = g.addr('match_tick'), cb = g.addr('cur_bank');
  let late = 0, n = 0, ticks = 0, rf = 0, rt = 0;
  const cpu = g.nes.cpu, emu0 = cpu.emulate.bind(cpu);
  cpu.emulate = function () { if (cpu.REG_PC + 1 === mt && cpu.mem[cb] === 0) ticks++; return emu0(); };
  for (let f = 0; f < 3000 && app(g) === AS.MATCH; f++) {
    const t0 = ticks; g.run(1); n++;
    const pc = g.nes.cpu.REG_PC + 1; if (pc < wf || pc > wf + 16) late++;
    if (g.peek('ms') === 2) { rf++; rt += ticks - t0; }
  }
  cpu.emulate = emu0;
  check(n === 3000 && g.peek('npl') === 4 && g.peek('persp') === 1 && rf > 300 && rt * 100 >= rf * 97,
    `double en perspective : vitesse du jeu ${(rt / rf).toFixed(3)} (${rt} trames logiques pour ${rf} trames d'échange, 0,97 au moins ; ${late} trames rattrapées sur ${n})`);
}
{
  // lignes fines (2.64) : les tuiles des lignes prennent la place des panneaux et
  // du public de la vue à plat absents du court ; le jeu ne doit jamais les
  // réécrire (regard du public, ola, panneaux) pendant le match
  let bad = [];
  for (const env of [0, 1, 2, 5, 6]) {
    const g = boot(); toMenu(g); setOpts(g, { VIEW: 1, ENV: env, FMT: 3 }); g.tap('START');
    runUntil(g, () => scr(g) === SS.PRESENT, 120); g.run(40); g.tap('A');
    runUntil(g, () => app(g) === AS.MATCH, 300); allCpu(g); g.run(20);
    for (let k = 0; k < 6; k++) { g.poke('crowd_timer', 60); g.run(150); }
    runUntil(g, () => !g.peek('net_shake'), 60);   /* (filet au repos : en tremblant, ses tuiles sont secouées, 2.68) */
    // tuiles attendues : données de la ROM (banque 11, compressées pour tz_unpack)
    const v = env === 6 ? 1 : env, rb = (b, sym, off) => g.nes.rom.rom[b][g.addr(sym) - 0x8000 + off];
    const nt = rb(3, 'persp_ntile', env), vm = g.nes.ppu.vramMem, ps = rb(11, 'persp_ps' + v, 0);
    let p = 0, n = 0;
    for (let i = 0; i < nt; i++) {
      const slot = rb(11, 'persp_slot' + v, i), m = rb(11, 'persp_chr' + v, p) | (rb(11, 'persp_chr' + v, p + 1) << 8);
      p += 2; let prev = 0, ok = true;
      for (let j = 0; j < 16; j++) {
        if (!((m >> j) & 1)) prev = rb(11, 'persp_chr' + v, p++);
        if (vm[slot * 16 + j] !== prev) ok = false;
      }
      if (!ok && !(ps && slot >= ps && slot < ps + 24)) n++;     /* (petits spectateurs : animés, regard vers le haut) */
    }
    if (n || app(g) !== AS.MATCH || !g.peek('persp')) bad.push(env + ':' + n + '/' + nt + (g.peek('persp') ? '' : ' (pas en perspective)'));
  }
  check(bad.length === 0, `perspective, 5 lieux : tuiles du court jamais réécrites pendant le match (ola, public, panneaux) ${bad.join(' ')}`);
}
{
  // menu : ligne VUE, à plat par défaut
  const m = boot(); toMenu(m);
  const r = rowOf(m, 24);
  check(r > 0 && m.text(3, menuY(m, r), 3) === 'VUE' && m.text(16, menuY(m, r), 6) === 'A PLAT', 'menu : ligne VUE (à plat par défaut)');
  m.poke('cursor', r); m.tap('RIGHT'); m.run(5);
  check(m.peek('opt_val', 24) === 1 && m.text(16, menuY(m, r), 11) === 'PERSPECTIVE', 'menu : VUE PERSPECTIVE');
  // match en perspective : court déformé, tuiles chargées, projection
  const e = boot(); startWith(e, { FMT: 3, ENV: 0 }); e.poke('opt_val', 1, 24);
  const f = boot(); toMenu(f); f.poke('opt_val', 1, 24); f.poke('opt_val', 3, O.FMT); f.tap('START'); f.run(30); f.tap('A');
  runUntil(f, () => app(f) === AS.MATCH, 300); f.run(30);
  const nt = (g, x, y) => g.nes.ppu.vramMem[0x2000 + y * 32 + x];
  const flat = boot(); startWith(flat, { FMT: 3, ENV: 0 }); flat.run(30);
  let diff = 0; for (let y = 4; y < 30; y++) for (let x = 4; x < 28; x++) if (nt(f, x, y) !== nt(flat, x, y)) diff++;
  check(f.peek('persp') === 1 && flat.peek('persp') === 0 && diff > 100,
    `court en perspective (${diff} tuiles du court changées) ; vue à plat inchangée`);
  // tribunes en perspective : elles s'avancent vers le court au fond, petits
  // spectateurs dans la moitié du fond, ola jusqu'aux colonnes ajoutées
  // 3 tailles plus petites vers le fond : 24 places dès ps_idx (4 dessins, puis 4 bras levés)
  const base = f.peek('ps_idx'), sizeOf = (t) => ((t - base) & 255) < 24 && !((t - base) & 4) ? ((t - base) >> 3) : -1;
  const isUp = (t) => ((t - base) & 255) < 24 && ((t - base) & 4);
  const sizes = [0, 0, 0]; let wide = 0;
  for (let y = 4; y < 30; y++) for (let x = 0; x < 32; x++) { const k = sizeOf(nt(f, x, y)); if (k >= 0) sizes[k]++; }
  for (let y = 4; y < 8; y++) for (const x of [4, 27]) if (nt(f, x, y) !== nt(flat, x, y)) wide++;
  f.poke('crowd_timer', 60); f.run(20);
  let cheer = 0; for (let y = 4; y < 30; y++) for (let x = 0; x < 32; x++) if (isUp(nt(f, x, y))) cheer++;
  f.run(80);
  let rest = 0; for (let y = 4; y < 30; y++) for (let x = 0; x < 32; x++) if (isUp(nt(f, x, y))) rest++;
  check(base > 0 && sizes.every((n) => n > 10) && wide === 8 && cheer > 0 && rest === 0,
    `tribunes en perspective : spectateurs de 3 tailles réduites (${sizes.join(', ')}), tribunes avancées, ola (${cheer} debout) puis rétablie`);
  // projection : un joueur placé au fond, près du couloir, apparaît plus près du centre qu'à plat
  const s16 = (g, n, v, o = 0) => { g.poke(n, v & 255, o); g.poke(n, (v >> 8) & 255, o + 1); };
  for (const g of [f, flat]) { g.poke('ms', 2); g.poke('b_live', 0); g.poke('b_vis', 0); s16(g, 'plx', 70 * 64, 2); s16(g, 'ply', 50 * 64, 2); g.run(3); }
  const xOf = (g) => { let best = 999; for (let i = 0; i < 64; i++) { const y = g.peek('oam_buf', i * 4), t = g.peek('oam_buf', i * 4 + 1);
    if (y < 0xEF && y < 60 && (g.peek('oam_buf', i * 4 + 2) & 3) === 1) best = Math.min(best, g.peek('oam_buf', i * 4 + 3)); } return best; };
  const xp = xOf(f), xf = xOf(flat);
  check(xp > xf + 5, `projection : joueur du fond plus près du centre en perspective (x ${xp} contre ${xf} à plat)`);
  // retour aux menus : motifs d'origine rétablis, projection coupée
  f.tap('START'); f.run(5); f.tap('DOWN'); f.tap('A'); runUntil(f, () => app(f) === AS.SCREENS, 200); f.run(40);
  const rom2 = f.nes.rom.rom[2]; let d2 = 0; for (let i = 0; i < 4096; i++) if (f.nes.ppu.vramMem[i] !== rom2[i] && !(i >= 0x600 && i < 0xCE0)) d2++;   // ($60-$CD : tuiles des menus)
  check(f.peek('persp') === 0 && d2 === 0, `hors du match : tuiles de décor d'origine rétablies (${d2} octet(s) différent(s)), projection coupée`);
  // match complet en perspective (double, château)
  const d = boot(); toMenu(d); setOpts(d, { MODE: MODE.P2, TYPE: 1, ENV: 5, FMT: 3 }); d.poke('opt_val', 1, 24); d.tap('START'); d.run(30); d.tap('A');
  runUntil(d, () => app(d) === AS.MATCH, 300); allCpu(d);
  const done = runUntil(d, () => app(d) === AS.SCREENS, 60 * 60 * 25); d.run(30);
  check(done && d.peek('persp') === 0, 'match complet en perspective (double, château), projection coupée ensuite');
}
}
if (sec('Plongeon')) {
console.log('Plongeon');
{
  // Le plongeon part AVANT la frappe ; il se décide au lâcher du bouton :
  // joueur bien placé, frappe normale ; balle trop loin sur le côté,
  // plongeon, et la frappe part en vol (lob de sauvetage). Joueur humain
  // (bas) : la balle de l'ordinateur arrive à OFF px sur sa gauche ; il
  // appuie sur A puis le lâche à N trames de l'arrivée (direction : facultative).
  const humanDive = (view, OFF, N, dir, trials = 3) => {
    const e = boot(); toMenu(e); e.poke('opt_val', view, 24); setOpts(e, { FMT: 3, SURF: 1 }); e.tap('START'); e.run(10); e.tap('A');
    runUntil(e, () => app(e) === AS.MATCH, 300);
    const out = [];
    for (let trial = 0; trial < trials; trial++) {
      let ok = false;
      for (let f = 0; f < 3000 && !ok; f++) {
        if (e.peek('ms') === 0 && e.peek('sc_server') === 0) { e.tap('A'); e.run(20); e.tap('A'); }
        e.run(1);
        ok = e.peek('ms') === 2 && e.peek('hitter') === 1 && e.peek('b_live') === 1 && e.peekS16('by') / 64 < 150 && e.peekS16('bvy') > 0;
      }
      if (!ok) continue;
      const pred = () => { let x = e.peekS16('bx'), y = e.peekS16('by'); const vx = e.peekS16('bvx'), vy = e.peekS16('bvy'), py = e.peekS16('ply', 0);
        for (let n = 0; n < 60; n++) { const d = y - py; if (d > -14 * 64 && d < 9 * 64) return { n, x: x / 64 }; x += vx; y += vy; } return null; };
      let placed = false, pressed = false, r = { dove: false, hit: false, hitAt: 0, before: false, shot: -1, kneel: false, still: true, startX: 0, rel: -1 };
      const rh0 = e.peek('rally_hits');
      for (let f = 0; f < 150; f++) {
        const p = pred();
        if (p && !placed) { const v = (Math.round(p.x) - OFF) * 64; e.poke('plx', v & 255, 0); e.poke('plx', (v >> 8) & 255, 1); placed = true; }
        if (p && placed && !pressed && p.n <= N) { e.releaseAll(); e.down('A'); if (dir) e.down('RIGHT'); pressed = true; }
        else if (pressed) {
          // distance réelle de la balle au lâcher (la joueuse peut être
          // arrêtée par le bord du court et se retrouver plus près)
          if (r.rel < 0) { const q = pred(); r.rel = q ? Math.round(Math.abs(q.x - e.peekS16('plx', 0) / 64)) : 0; }
          e.releaseAll();
        }
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
    const a = humanDive(view, 34, 7, 0, 5);
    // (au moins 2 essais sur 5 : selon la trajectoire, un lâcher fixe à 7 trames peut rater, comme pour un joueur)
    const okDive = (r) => r.dove && r.before && r.hit && r.hitAt >= 30 && r.shot === 2;
    check(a.filter(okDive).length >= 2 && a.every(r => !r.hit || okDive(r)),
      `${v} : balle à 34 px, bouton lâché : plongeon avant la frappe, balle touchée en vol (lob) [${a.map(r => r.hit ? 't=' + r.hitAt : 'raté').join(', ')}]`);
    check(a.filter(r => r.dove).every(r => r.kneel && r.still), `${v} : au sol puis un genou à terre, immobile jusqu'au relevé`);
    if (view) continue;
    const b = humanDive(view, 12, 7, 0);
    check(b.length >= 2 && b.every(r => !r.dove) && b.filter(r => r.hit && r.shot !== 2).length >= 2, `joueur bien placé (balle à 12 px) : pas de plongeon, frappe normale (${b.map(r => (r.dove ? 'plongeon' : 'debout') + '/' + (r.hit ? 'frappe ' + r.shot : 'ratée')).join(', ')})`);
    const c = humanDive(view, 48, 7, 1, 6).filter(r => r.rel >= 46);
    check(c.length >= 2 && c.every(r => !r.hit), `balle à 46 px ou plus au lâcher : hors de portée, même en plongeant (${c.length} essai(s) : ${c.map(r => r.rel + ' px').join(', ')})`);
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
        if (e.peek('bounces') === 0 && e.peek('ms') === 2 && (lx < 72 || lx > 184 || ly < 45 || ly > 219)) outDive++;   /* (limites des règles, 2.76 : ligne et rayon de la balle compris) */
      }
      if (p === cur && t >= 30 && e.peek('rally_hits') !== rh && e.peek('hitter') === (p & 1)) { touched++; cur = -1; }
      prev[p] = t;
    }
  }
  }
  check(dives >= 5 && touched * 4 >= dives * 3 && outDive === 0, `ordinateur : ${dives} plongeons, ${touched} balles touchées, ${outDive} sur une balle qui sort`);
}
}
if (sec('Séquences animées du match')) {
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
    // (2.77 : l'ordinateur qui gagne le tirage ne sert plus toujours : les
    // places dépendent de son choix ; chacun sur sa ligne de fond, de part et
    // d'autre du filet, comme dans une partie sans séquence)
    const got = [pos(e, 0), pos(e, 1)], base = (p) => p[1] >= 216 || p[1] <= 50;
    const placed = (JSON.stringify(got) === JSON.stringify(want)) || (base(got[0]) && base(got[1]) && (got[0][1] > 132) !== (got[1][1] > 132));
    check(e.peek('cer') === 0 && start[0] === 216 && start[1] >= 230 && wave && upBack && placed && e.peek('ms') === 0,
      `${v} : entrée depuis le tunnel ${JSON.stringify(start)}, le joueur du fond vu de dos en s'éloignant, salut, puis chacun à sa place ${JSON.stringify(got)}`);
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
    // (sans ralentis : avec un ralenti, les chaises n'arrivent qu'après lui)
    const e = boot(); startWith(e, { FMT: 3, SURF: 1, REPLAY: 0 }); allCpu(e); e.run(20); e.cerSkip = false;
    // (2.76) les chaises apparaissent dès que le bandeau du score est entièrement monté, pas avant
    runUntil(e, () => e.peek('cer') === 0xFD || e.peek('cer') === 4, 60 * 240);   /* (2.74 : bandeau entre les points, jeux plus longs) */
    let noChairBefore = true;
    runUntil(e, () => { if (!(e.peek('hl_flags') & 4) && oamHas(e, CHAIR)) noChairBefore = false; return (e.peek('hl_flags') & 4) !== 0 || e.peek('cer') === 4; }, 200);
    e.run(2);
    const early = noChairBefore && oamHas(e, CHAIR);
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
    check(early, 'changement de côté : les chaises apparaissent avec le bandeau entièrement monté, pas avant');
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
    let portrait = false, txt = '', first = -1, second = -1;
    for (let f = 0; f < 1100 && app(e) === AS.MATCH; f++) {
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
      // portrait du vainqueur (2.69) : en pied puis gros plan, son nom
      if (e.peek('hl_mode') === 3) {
        portrait = true;
        const t = e.peek('hl_t');
        if (t >= 40 && first < 0) { const a = []; for (let i = 0; i < 15 && e.peek('hl_txt', i); i++) a.push(e.peek('hl_txt', i)); txt = String.fromCharCode(...a); first = e.peek('hl_sp'); }
        if (t >= 180 && second < 0) second = e.peek('hl_sp');   // (hl_t avance de 2 en 2 : valeurs impaires)
      }
    }
    check(kneel && supine && racket, `${dbl ? 'double' : 'simple'} : balle de match, vainqueur à genoux poings levés, puis allongé, raquette lâchée`);
    check(shake && close && salute && app(e) !== AS.MATCH && frames < 1010, `${dbl ? 'double' : 'simple'} : poignée de main au filet, salut à l'arbitre, portrait du vainqueur, puis les résultats (${frames} trames)`);
    e.run(60);
    const four = n === 4, want = four ? 'VICTOIRE !' : (e.text(20, 4, 8).match(/^[A-Z]+/) || [''])[0] + ' GAGNE !';
    check(portrait && first === w && second === (four ? w + 2 : w) && txt === want && scr(e) === SS.RESULTS && e.peek('ctrl_swap') === 0,
      `${dbl ? 'double' : 'simple'} : portrait du vainqueur (« ${txt} »), en pied puis gros plan${four ? ' du partenaire' : ''}, puis les résultats [${portrait},${first},${second},${w},« ${want} »,${scr(e)},${e.peek('ctrl_swap')}]`);
  }
}
}
if (sec('Bras devant le buste : vue de dos')) {
console.log('Bras devant le buste : vue de dos');
{
  // gestes où un bras passe devant le buste : de face, le bras est visible ;
  // de dos, le buste « bras caché » est chargé à la volée (case de la zone
  // partagée) et utilisé pour ce joueur seulement
  const X = new Set([8, 10, 11, 13, 22, 23, 24, 25, 26]);
  const e = boot(); startWith(e, { FMT: 3, SURF: 1 }); allCpu(e);
  let back = 0, backOk = 0, front = 0, frontOk = 0;
  const o = e.addr('oam_buf');
  for (let f = 0; f < 20000 && (back < 5 || front < 5); f++) {
    e.run(1);
    if (e.peek('swap_on') !== 2) continue;
    for (let p = 0; p < 2; p++) {
      const k = e.peek('pose_now', p) & 31;
      if (!X.has(k)) continue;
      const isBack = ((p & 1) ^ e.peek('sc_swap')) === 0, ovr = e.peek('row1_ovr', p);
      if (isBack) {
        back++;
        // la case est utilisée dans l'OAM (rangée du buste du joueur)
        let used = false;
        for (let i = 0; i < 64; i++) if (e.nes.cpu.mem[o + i * 4 + 1] === ovr || e.nes.cpu.mem[o + i * 4 + 1] === ovr + 1) used = true;
        if (ovr && used) backOk++;
      } else { front++; if (ovr === 0) frontOk++; }
    }
  }
  check(back >= 5 && backOk === back && front >= 5 && frontOk === front,
    `vu de dos : buste « bras caché » chargé à la volée (${backOk}/${back}) ; vu de face : bras visible (${frontOk}/${front})`);
}
}
if (sec('Format du Grand Martinot et du Martinworld Tour')) {
console.log('Format du Grand Martinot et du Martinworld Tour');
for (const type of [5, 6]) {
  // 3 sets, et la finale de chaque tournoi en 5 sets ; ligne FORMAT absente du menu
  const e = boot(); startWith(e, { MODE: MODE.COMP, COMP: type, FMT: 0 }); skipIntro(e);
  const seen = [];
  for (let m = 0; m < 8 && scr(e) === SS.COMP; m++) {
    const fin = e.text(0, 6, 32).replace(/\0/g, ' ').trim() === 'FINALE';
    e.tap('A'); toPresent(e, 60); e.run(3);
    seen.push(`${fin ? 'F' : '-'}${e.peek('cfg', 16)}:${e.text(18, 25, 6).trim()}`);
    e.tap('A'); runUntil(e, () => app(e) === AS.MATCH || (scr(e) === 10 && litScr(e)), 120);
    if (app(e) !== AS.MATCH) { e.tap('A'); runUntil(e, () => app(e) === AS.MATCH, 120); }   /* image du tournoi */
    e.run(30);
    e.poke('sc_winner', 0); e.poke('sc_over', 1); e.poke('ms', 4); e.poke('hl_fin', 1); e.poke('ms_timer', 2);
    runUntil(e, () => app(e) === AS.SCREENS, 60); e.run(20); e.tap('A'); e.run(40); skipIntro(e);
  }
  const ok = seen.length >= 4 && seen.every(x => x[0] === 'F' ? x.endsWith('3:5 SETS') : x.endsWith('2:3 SETS'));
  const m = boot(); toMenu(m); setOpts(m, { MODE: MODE.COMP, COMP: type }); m.tap('B'); m.run(30); toMenu(m); m.run(10);
  check(ok && rowOf(m, O.FMT) < 0, `${type === 5 ? 'Grand Martinot' : 'Martinworld Tour'} : 3 sets, finales en 5 sets, FORMAT imposé (${seen.join(' ')})`);
}
}
if (sec('Bande-annonce du Grand Martinot et du Martinworld Tour')) {
console.log('Bande-annonce du Grand Martinot et du Martinworld Tour');
for (const type of [5, 6]) {
  // au lancement : 4 plans en gros plan, puis le titre, sur le riff de rock ;
  // environ 7 secondes (titre tenu une seconde de plus) ; puis l'écran de la compétition
  const e = boot(); e.cerSkip = false; toMenu(e); setOpts(e, { MODE: MODE.COMP, COMP: type, FMT: 3 }); e.tap('START'); passCustom(e);
  runUntil(e, () => scr(e) === 11, 120);
  const t0 = e.frames, cuts = new Set(); let rock = false, big = 0, cam = false, title = '', dur = 0;
  runUntil(e, () => {
    if (scr(e) !== 11) return true;
    const c = e.peek('ts_cut');
    cuts.add(c);
    if (e.peek('music_cur') === 19) rock = true;
    if (e.peek('scroll_x') !== 0) cam = true;                                // travellings
    if (c < 7 && e.peek('ts_t') === 12) {                                    // image en haute définition
      const t = new Set(); for (let i = 0; i < 28 * 32; i++) t.add(e.nes.ppu.vramMem[0x2000 + i]);
      big = Math.max(big, t.size);
    }
    if (!title && c === 7 && e.peek('ts_t') > 120) title = e.text(0, 21, 32).trim();
    if (!dur && e.peek('teaser_done')) dur = (e.frames - t0) / 60;
    return false;
  }, 700);
  runUntil(e, () => scr(e) === SS.COMP && litScr(e), 120);
  const name = type === 5 ? 'Grand Martinot' : 'Martinworld Tour';
  check(cuts.size === 8 && rock && big > 140 && cam && dur > 6.5 && dur < 8 && title.includes('TOURNOIS') && scr(e) === SS.COMP,
    `${name} : bande-annonce (7 plans en haute définition jusqu'à ${big} tuiles, travellings, riff de rock, ${dur.toFixed(1)} s, titre tenu une seconde de plus, « ${title} »), puis l'écran de la compétition`);
  const a = boot(); a.cerSkip = false; toMenu(a); setOpts(a, { MODE: MODE.COMP, COMP: type, FMT: 3 }); a.tap('START'); passCustom(a);
  runUntil(a, () => scr(a) === 11, 120); a.run(20); a.tap('A'); runUntil(a, () => scr(a) === SS.COMP, 60);
  check(scr(a) === SS.COMP, `${name} : A passe la bande-annonce`);
}
}
if (sec('Cérémonies de fin de compétition')) {
console.log('Cérémonies de fin de compétition');
{
  // compétition classique gagnée : remise des médailles (3e, 2e, puis 1er
  // apparaissent ; médailles bronze, argent, or ; hymne), puis la coupe
  const { e } = playComp(0, {}, () => true);
  for (let k = 0; k < 5 && scr(e) !== SS.COMP_END; k++) { e.tap('A'); e.run(30); }
  const pal = (i) => e.nes.ppu.vramMem[0x3F00 + i] ?? 0;
  const t0 = e.peek16('pod_t');
  // (arrivée sur l'écran vers la trame 40 : le 3e est déjà là, pas le vainqueur)
  const hidden = e.peek('pal_buf', 5) === e.peek('pal_buf', 0) && e.peek('pal_buf', 13) !== e.peek('pal_buf', 0);
  runUntil(e, () => e.peek16('pod_t') >= 205, 400);
  const shown = e.peek('pal_buf', 5) !== e.peek('pal_buf', 0) && e.peek('pal_buf', 9) !== e.peek('pal_buf', 0) && e.peek('pal_buf', 13) !== e.peek('pal_buf', 0);
  const o = e.addr('oam_buf'); const tiles = new Set();
  for (let i = 0; i < 64; i++) if (e.nes.cpu.mem[o + i * 4] < 240 && e.nes.cpu.mem[o + i * 4 + 2] === 3) tiles.add(e.nes.cpu.mem[o + i * 4 + 1]);
  check(t0 < 100 && hidden && shown && [0x39, 0x3A, 0x3B].every(t => tiles.has(t)) && e.peek('music_cur') === 18,
    `remise des médailles : joueurs qui apparaissent un à un, médailles or, argent, bronze, hymne (${e.peek('music_cur')})`);
  runUntil(e, () => e.peek('cup_t') > 60, 200);
  check(e.peek('cup_t') > 60, 'puis la coupe est brandie');
}
for (const type of [5, 6]) {
  // Grand Martinot, Martinworld Tour : remise du trophée, puis conférence de presse
  const { e } = playComp(type, {}, () => true);
  for (let k = 0; k < 5 && scr(e) !== SS.COMP_END; k++) { e.tap('A'); e.run(30); }
  const name = type === 5 ? 'Grand Martinot' : 'Martinworld Tour';
  const st = e.peek('gm_st'), busy = e.peek('pod_busy');
  runUntil(e, () => e.peek16('pod_t') >= 520, 700);
  const sprTiles = new Set(); const o = e.addr('oam_buf');
  for (let i = 0; i < 64; i++) if (e.nes.cpu.mem[o + i * 4] < 240) sprTiles.add(e.nes.cpu.mem[o + i * 4 + 1]);
  check(st === 1 && busy === 1 && e.text(0, 2, 32).includes(type === 5 ? 'LE GRAND MARTINOT' : 'MARTINWORLD TOUR') && sprTiles.has(e.peek('trophy_tile')),
    `${name} : remise du trophée (tapis rouge, coupe)`);
  e.tap('A'); e.run(10);
  check(scr(e) === SS.COMP_END && e.peek('pod_busy') === 1, `${name} : A n'interrompt pas la cérémonie`);
  e.tap('A'); e.run(20);
  const q1 = e.text(0, 7, 32).trim();
  check(e.peek('gm_st') === 2 && e.text(0, 2, 32).includes('CONFERENCE DE PRESSE') && q1.length > 5, `${name} : conférence de presse (« ${q1} »)`);
  e.tap('A'); e.run(10); e.tap('A'); e.run(10);
  check(e.peek('pod_busy') === 0 && e.text(0, 27, 32).includes('A : CONTINUER'), `${name} : trois questions, puis A pour continuer`);
  // (2.76) le champion brandit la coupe (image HD), puis le générique
  e.tap('A'); runUntil(e, () => scr(e) === 14, 60); e.run(80);
  const vmc = e.nes.ppu.vramMem; let used = new Set(); for (let i = 0; i < 960; i++) used.add(vmc[0x2000 + i]);
  if (process.env.SHOT) { e.setRender(true); e.run(2); e.screenshot(process.env.SHOT + '_' + type + '.png'); }
  check(scr(e) === 14 && used.size > 80, `${name} : le champion brandit la coupe (image HD, ${used.size} tuiles)`);
  e.tap('A'); runUntil(e, () => app(e) === 4, 180);
  check(app(e) === 4, `${name} : puis le générique de fin`);
}
}
if (sec('Revers et jeu de tuiles par phase')) {
console.log('Revers et jeu de tuiles par phase');
{
  // match CPU : préparation, impact et fin du revers à deux mains ; le jeu
  // « revers » est chargé pendant l'échange, le jeu « célébrations » ensuite ;
  // une pose n'est jamais montrée sans ses tuiles ; aux écrans, célébrations
  const e = boot(); startWith(e, { FMT: 3 }); allCpu(e);
  // (pendant un ralenti et la trame qui le suit, pose_now n'est pas dessinée :
  // le ralenti dessine ses poses enregistrées, elles aussi vérifiées)
  const seen = new Set(); let bad = 0, onR = 0, onC = 0, prevMs = 0;
  const set = (k) => (k === 7 || (k >= 15 && k <= 18)) ? 1 : (k >= 22 && k <= 26) ? 2 : 0;
  for (let f = 0; f < 20000 && app(e) === AS.MATCH; f++) {
    e.run(1);
    const sw = e.peek('swap_on'), ms = e.peek('ms');
    if (sw === 2) onR++; if (sw === 1) onC++;
    if (ms !== 5 && prevMs !== 5)
      for (let p = 0; p < 2; p++) { const k = e.peek('pose_now', p) & 31; seen.add(k); if (set(k) && set(k) !== sw) bad++; }
    prevMs = ms;
  }
  check([22, 23, 24].every((k) => seen.has(k)) && bad === 0 && onR > 0 && onC > 0,
    `revers à deux mains (préparation, impact, fin) ; jamais une pose sans ses tuiles (${bad}) ; jeux chargés selon la phase`);
  // double en perspective (4 joueurs) : jamais une pose sans ses tuiles, y
  // compris le rebond de balle avant le service (version 2.62)
  {
    const d = boot(); startWith(d, { FMT: 3, TYPE: 1, MODE: MODE.P2, VIEW: 1 }); allCpu(d);
    let bad4 = 0, pm = 0;
    for (let f = 0; f < 12000 && app(d) === AS.MATCH; f++) {
      d.run(1);
      const sw = d.peek('swap_on'), ms = d.peek('ms');
      if (ms !== 5 && pm !== 5) for (let p = 0; p < 4; p++) { const k = d.peek('pose_now', p) & 31; if (set(k) && set(k) !== sw) bad4++; }
      pm = ms;
    }
    check(bad4 === 0, `double en perspective : jamais une pose sans ses tuiles (${bad4})`);
  }
  runUntil(e, () => app(e) === AS.SCREENS, 60 * 60 * 25); e.run(60);
  const base = 0x1000 + e.peek('swap_base') * 16, rom2 = e.nes.rom.rom[2];
  let d = 0; for (let i = 0; i < 34 * 16; i++) if (e.nes.ppu.vramMem[base + i] !== rom2[base + i]) d++;
  check(d === 0, `écrans : tuiles des célébrations rétablies (${d} octet(s) différent(s))`);
}
}
if (sec('Dérapages selon la surface')) {
console.log('Dérapages selon la surface');
{
  // le joueur court 12 trames vers la droite puis lâche la croix
  const run = (surf) => {
    const e = boot(); toMenu(e); setOpts(e, { FMT: 3, SURF: surf }); e.tap('START'); e.run(10); e.tap('A');
    runUntil(e, () => app(e) === AS.MATCH, 300); e.run(30);
    e.poke('ms', 2); e.poke('b_live', 0); e.poke('b_vis', 0);
    const x = () => e.peekS16('plx', 0) / 64, xs = [x()];
    let fx = 0;
    e.down('RIGHT'); for (let i = 0; i < 12; i++) { e.run(1); xs.push(x()); }
    e.up('RIGHT'); for (let i = 0; i < 12; i++) { e.run(1); xs.push(x()); if (e.peek('slide_t')) fx++; }
    return { start: xs[2] - xs[0], glide: xs[24] - xs[12], fx };
  };
  const g = run(1), c = run(0), s = run(4), i = run(5);
  check(g.glide === 0 && g.fx === 0, `gazon : arrêt net (${g.glide.toFixed(1)} px), aucun effet`);
  check(c.glide > 4 && c.glide < 9 && c.fx > 0 && Math.abs(c.start - g.start) < 0.1, `terre battue : glissade de ${c.glide.toFixed(1)} px à l'arrêt, poussière`);
  check(s.glide === 0 && s.start < g.start - 1 && s.fx > 0, `sable : départ lourd (${s.start.toFixed(1)} px en 2 trames contre ${g.start.toFixed(1)}), arrêt net, gerbe`);
  check(i.glide > 3 && i.glide < 10 && i.start < g.start - 1 && i.fx > 0, `glace : départ progressif, glissade de ${i.glide.toFixed(1)} px, éclats`);
}
}
if (sec('Bandeau de pause et balle de match (2.75)')) {
console.log('Bandeau de pause et balle de match (2.75)');
for (const view of [0, 1]) {
  const e = boot(); startWith(e, { FMT: 3, REPLAY: 0, VIEW: view }); allCpu(e);
  e.run(900);
  const nt = () => Array.from(e.nes.ppu.vramMem.slice(0x2000 + 12 * 32, 0x2000 + 18 * 32)).join();
  const before = nt();
  e.tap('START'); e.run(10);
  const t = [14, 15].map(r => String.fromCharCode(...Array.from(e.nes.ppu.vramMem.slice(0x2400 + r * 32, 0x2400 + r * 32 + 32)))).join('');
  const paused = e.peek('paused');
  e.tap('START'); e.run(10);
  check(paused && /ALEX|MAYA/.test(t) && nt() === before, `pause (vue ${view}) : score au centre de l'écran, court rétabli à la reprise`);
}
{
  const m = boot(); startWith(m, { FMT: 3, REPLAY: 0 }); allCpu(m);
  m.poke('sc_games', 5); m.poke('sc_points', 2);
  const nt1 = (r) => String.fromCharCode(...Array.from(m.nes.ppu.vramMem.slice(0x2400 + r * 32, 0x2400 + r * 32 + 32)).map(c => c < 32 ? 32 : c));
  const said = runUntil(m, () => m.peek('mp_t') >= 20, 60 * 60 * 3);
  const txt = nt1(14) + '|' + nt1(15), served = m.peek('ms');
  const gone = runUntil(m, () => m.peek('mp_t') === 0, 200);
  check(said && (m.peek('sc_crit') & 4) && txt.includes('BALLE DE MATCH') && txt.includes('POUR ALEX') && served === 0 && gone,
        `balle de match : bandeau au centre avant le service (« ${txt.replace(/ +/g, ' ').trim()} »), disparu seul`);
}
}
if (sec('Joueur personnalisé en compétition (2.75)')) {
console.log('Joueur personnalisé en compétition (2.75)');
{
  // (2.77) ordre : TON JOUEUR (joueur 1, puis joueur 2 humain), puis l'affiche
  // VS, qui montre le joueur conçu, puis le tableau
  const e = boot(); e.customSkip = false; e.pickSkip = false; toMenu(e);
  setOpts(e, { MODE: MODE.COMP, COMP: 1, TYPE: 0, CPART: 1, P1: 0, P2: 1, FMT: 3 }); e.poke('pk_off', 0); e.run(1);   // (sans saut automatique : pages de choix montrées)
  e.tap('START'); e.run(20);
  const first = scr(e), t1 = e.text(4, 4, 20);
  e.tap('DOWN'); e.tap('A'); let n = 0;
  while (e.peek('cu_sav', 1 + e.peek('cu_st', 2)) !== 'Z'.charCodeAt(0) && n++ < 40) e.tap('UP', 1, 1, 2);
  e.tap('A');
  e.tap('START'); e.run(20);
  const second = scr(e), t2 = e.text(4, 4, 20);
  e.tap('START'); runUntil(e, () => scr(e) === SS.PICK, 120); e.run(40);
  // (2.77h) plus de page de choix du joueur en compétition : la page du court
  const poster = scr(e), t3 = e.text(0, 0, 32) + e.text(0, 1, 32) + e.text(0, 2, 32);
  e.tap('A'); e.tap('A'); e.tap('A'); runUntil(e, () => scr(e) === SS.COMP, 200);
  const name = [...Array(30).keys()].map(r => e.text(0, r, 32)).filter(l => /\bZ/.test(l)).length;
  check(first === SS.CUSTOM && t1.startsWith('JOUEUR 1') && second === SS.CUSTOM && t2.startsWith('JOUEUR 2') && poster === SS.PICK && !t3.includes('TON JOUEUR') && t3.includes('LIEU') && scr(e) === SS.COMP && name >= 1,
    `compétition : TON JOUEUR (« ${t1.trim()} », « ${t2.trim()} »), puis la page du court sans choix du joueur (${poster}, « ${t3.replace(/\s+/g, ' ').trim()} »), puis le tableau (${scr(e)}, ${name})`);
  // (profil remis d'origine pour la suite)
  e.poke('cu_sav', 0, 0);
}
{
  const e = boot(); e.customSkip = false; toMenu(e); setOpts(e, { MODE: MODE.COMP, COMP: 0, P1: 0, FMT: 3 }); e.tap('START'); e.run(20);
  check(scr(e) === SS.CUSTOM && e.text(3, 7, 6) === 'PROFIL' && e.text(13, 7, 9) === "D'ORIGINE", 'compétition : écran « TON JOUEUR » avant le tableau, profil d\'origine par défaut');
  // nom BRUNO, femme, peau ébène, cheveux blonds, tenue rose
  e.tap('DOWN'); e.tap('A');
  for (const L of 'BRUNO') { let n = 0; while (e.peek('cu_sav', 1 + e.peek('cu_st', 2)) !== L.charCodeAt(0) && n++ < 40) e.tap('UP', 1, 1, 2); e.tap('RIGHT', 1, 1, 2); }
  e.tap('A');
  e.tap('DOWN'); e.tap('RIGHT');
  e.tap('DOWN'); e.tap('RIGHT'); e.tap('RIGHT');
  e.tap('DOWN'); e.tap('RIGHT'); e.tap('RIGHT'); e.tap('RIGHT');
  e.tap('DOWN'); e.tap('RIGHT'); e.tap('RIGHT'); e.tap('RIGHT'); e.tap('RIGHT');
  const sav = Array.from({ length: 12 }, (_, i) => e.peek('cu_sav', 1 + i));
  check(e.text(13, 7, 9) === 'PERSO 1/1' && String.fromCharCode(...sav.slice(0, 7)) === 'BRUNO  ' && sav[8] === 3 && sav[9] === 0x07 && sav[10] === 0x35 && sav[11] === 0x37,
    `profil : BRUNO, femme, peau ébène, tenue rose, cheveux blonds (${sav.map(v => v.toString(16)).join(' ')})`);
  e.tap('START'); e.run(40);
  check(scr(e) === SS.COMP && e.peek('cu_map', 0) === 1 && e.peek('char_female', 0) === 1 && e.text(0, 0, 32 * 30 > 0 ? 32 : 32) !== null &&
        [...Array(30).keys()].some(r => e.text(0, r, 32).includes('BRUN')), 'tableau de la compétition : le joueur personnalisé à la place d\'ALEX');
  e.tap('A'); for (let n = 0; n < 400 && app(e) !== AS.MATCH; n++) { e.run(1); if (scr(e) === SS.PRESENT && n % 30 === 0) e.tap('A'); }
  e.run(60);
  const pl = [17, 18, 19].map(i => e.peek('pal_buf', i));
  check(app(e) === AS.MATCH && pl[0] === 0x07 && pl[1] === 0x35 && pl[2] === 0x0F &&   // (2.77 : en match, contour, raquette et cheveux noirs)
        [0, 1, 2, 3].some(r => e.text(0, r, 32).includes('BRUN')),
    `en match : couleurs du profil (${pl.map(v => v.toString(16)).join(' ')}), nom « BRUN » en haut`);
  // retour au menu : personnages d'origine ; le profil reste gardé
  e.tap('START', 1, 1, 2); e.tap('SELECT', 1, 1, 2);
  runUntil(e, () => app(e) === AS.SCREENS, 400); e.run(30);
  if (scr(e) !== SS.MENU) { e.tap('B'); runUntil(e, () => scr(e) === SS.MENU, 200); e.run(20); }
  check(e.peek('cu_map', 0) === 0 && e.peek('char_female', 0) === 0 && e.peek('cu_sav', 0) === 0xC5, 'hors compétition : ALEX d\'origine, profil gardé dans la sauvegarde');
  e.tap('START'); e.run(20);
  check(scr(e) === SS.CUSTOM && e.text(13, 9, 5) === 'BRUNO' && e.text(13, 7, 9) === 'PERSO 1/1', 'compétition suivante : le profil revient');
  e.tap('LEFT');                        // (curseur sur PROFIL : PERSO 1/1 -> D'ORIGINE, 2.77)
  e.tap('START'); e.run(40);
  check(scr(e) === SS.COMP && e.peek('cu_map', 0) === 0 && [...Array(30).keys()].some(r => e.text(0, r, 32).includes('ALEX')), 'profil « D\'ORIGINE » : ALEX tel quel (sans obligation)');
}
{
  // (2.77) bibliothèque des profils : NOUVEAU, création à la première
  // modification, défilement, suppression par SELECT
  const e = boot(); e.customSkip = false; toMenu(e); setOpts(e, { MODE: MODE.COMP, COMP: 1, P1: 0, FMT: 3 }); e.tap('START'); e.run(30);
  const row = () => e.text(13, 7, 10).replace(/\0/g, ' ').trim(), nm = () => e.text(13, 9, 7).replace(/\0/g, ' ').trim();
  const seen = [];
  e.tap('RIGHT'); seen.push(row());                                           // NOUVEAU
  for (let i = 0; i < 5; i++) e.tap('DOWN'); e.tap('RIGHT'); for (let i = 0; i < 5; i++) e.tap('UP'); seen.push(row());   // tenue : PERSO 1/1
  e.tap('RIGHT'); e.tap('RIGHT'); seen.push(row());                          // D'ORIGINE
  e.tap('DOWN'); e.tap('A'); let n = 0; while (e.peek('cu_sav', 1) !== 'Z'.charCodeAt(0) && n++ < 40) e.tap('UP', 1, 1, 2); e.tap('A'); e.tap('UP');
  seen.push(row() + ' ' + nm());                                              // PERSO 2/2 ZLEX
  e.tap('RIGHT'); e.tap('RIGHT'); e.tap('RIGHT'); seen.push(row() + ' ' + nm());   // PERSO 1/2 ALEX
  e.tap('RIGHT'); seen.push(row() + ' ' + nm());                              // PERSO 2/2 ZLEX
  e.tap('SELECT'); seen.push(row());                                          // D'ORIGINE
  e.tap('RIGHT'); seen.push(row());                                           // PERSO 1/1
  const want = ['NOUVEAU', 'PERSO 1/1', "D'ORIGINE", 'PERSO 2/2 ZLEX', 'PERSO 1/2 ALEX', 'PERSO 2/2 ZLEX', "D'ORIGINE", 'PERSO 1/1'];
  // (2.77) NOM : GAUCHE/DROITE fait défiler les joueurs existants, puis les
  // profils rangés (qui reprennent leur personnage de base)
  e.tap('LEFT'); e.tap('DOWN');                                               // D'ORIGINE, curseur sur NOM
  const names = []; for (let k = 0; k < 22; k++) { e.tap('RIGHT'); names.push(e.text(13, 9, 7).replace(/\0/g, ' ').trim() + (e.text(13, 7, 5) === 'PERSO' ? '*' + e.peek('opt_val', O.P1) : '')); }
  check(names[0] === 'MAYA' && names[19] === 'TITOU' && names[20] === 'ALEX*0' && names[21] === 'ALEX' && e.text(13, 7, 9) === "D'ORIGINE",
    `NOM : défilement des joueurs puis des profils rangés (${names.join(' ')})`);
  check(seen.join('|') === want.join('|'), `profils : nouveau, création, défilement, suppression (${seen.join(', ')})`);
}
}
if (sec('Version 2.76 : lignes, bandeau, smash')) {
console.log('Version 2.76 : lignes, bandeau, smash');
{
  // balle qui touche la ligne de fond en milieu de trame : jugée au point de
  // contact (sur la ligne, bonne), et non là où elle est en fin de trame (2 px plus loin)
  const e = boot(); startWith(e, { FMT: 3, REPLAY: 0 }); allCpu(e);
  runUntil(e, () => e.peek('ms') === 2 && e.peek('bounces') === 0 && !e.peek('b_serve') && e.peek('hitter') === 1 && e.peek('sc_swap') === 0, 60 * 60 * 3);
  const s16 = (n, v) => { e.poke(n, v & 255, 0); e.poke(n, (v >> 8) & 255, 1); };
  s16('bx', 128 * 64); s16('by', 13773); s16('bvx', 0); s16('bvy', 256); s16('bz', 30); s16('bvz', -40);
  e.run(1);
  const endY = (13773 + 256) >> 6;
  check(e.peek('bounces') === 1 && e.peek('hl_bin') === 1 && e.peek('hl_by') === 217 && endY === 219 && e.peek('ms') === 2,
    `balle sur la ligne de fond : jugée au point de contact (y ${e.peek('hl_by')}, bonne) et non en fin de trame (y ${endY}, dehors)`);
}
{
  // après un ralenti, le bandeau du score monte aussitôt (pas de délai de lecture)
  const e = boot(); e.bandSkip = false; startWith(e, { FMT: 3, REPLAY: 1 }); allCpu(e);
  runUntil(e, () => e.peek('hl_mode') !== 0, 60 * 60 * 15);
  runUntil(e, () => e.peek('hl_mode') === 0 && !e.peek('rp_pending'), 60 * 30);
  const t0 = e.frames;
  runUntil(e, () => e.peek('band_vis') === 3, 200);
  check(e.frames - t0 < 30, `après le ralenti : bandeau du score en ${e.frames - t0} trames (avant : environ 60)`);
}
}
if (sec('Version 2.76 : pause')) {
console.log('Version 2.76 : pause');
{
  // pause : 3e ligne RALENTI (NON / OUI / SIMPLE), retenue pour les matchs suivants
  const e = boot(); startWith(e, { FMT: 3, REPLAY: 0 }); allCpu(e); e.run(200);
  e.tap('START'); e.run(6); e.tap('DOWN'); e.tap('DOWN'); e.tap('RIGHT'); e.tap('RIGHT'); e.run(4);
  const r = e.peek('cfg', CFG.REPLAYS), o = e.peek('opt_val', O.REPLAY), still = e.peek('paused');
  e.tap('UP'); e.tap('UP'); e.tap('A'); e.run(4);
  check(r === 2 && o === 2 && still && !e.peek('paused'), `pause : RALENTI réglé à SIMPLE (${r}, réglage ${o}), puis CONTINUER`);
}
{
  // (2.77) reprise : la ligne RALENTI est effacée ; pause juste après une
  // frappe : le bandeau se pose quand même, bords compris, sans sprite dessus
  const e = boot(); startWith(e, { FMT: 3, REPLAY: 0, VIEW: 1 }); allCpu(e); e.run(200);
  e.tap('START'); e.run(6); e.tap('START'); e.run(6);
  const l3 = e.text(9, 3, 16);
  check(!e.peek('paused') && !l3.includes('RALENTI'), `pause : ligne RALENTI effacée à la reprise (« ${l3.replace(/\0/g, ' ')} »)`);
  runUntil(e, () => e.peek('hit_now') === 1, 60 * 60);
  e.down('START'); e.run(1); e.up('START'); e.run(20);
  const v = e.nes.ppu.vramMem, edge = [14, 15].flatMap(y => [0, 1, 30, 31].map(x => v[0x2400 + y * 32 + x]));
  let over = 0;
  for (let i = 0; i < 64; i++) { const y = e.peek('oam_buf', i * 4); if (y >= 88 && y < 144) over++; }
  check(e.peek('paused') && (e.peek('hl_flags') & 1) && e.peek('mp_t') === 8 && edge.every(t => t === 0) && over === 0,
    `pause sur une trame de frappe : bandeau posé (${e.peek('hl_flags') & 1}, ${e.peek('mp_t')}), bords vides [${edge}], ${over} sprite(s) dessus`);
}
}
if (sec('Version 2.76 : balle de match et changement de côté')) {
console.log('Version 2.76 : balle de match et changement de côté');
{
  // ralentis désactivés : le bandeau reste affiché de la fin du point aux
  // statistiques ; les chaises n'apparaissent qu'avec le bandeau entièrement monté
  const e = boot(); startWith(e, { FMT: 3, REPLAY: 0 }); allCpu(e);
  e.cerSkip = false; e.poke('cer_skip', 0);
  e.poke('sc_games', 0); e.poke('sc_points', 3); e.poke('sc_points', 0, 1);
  let started = false, shown = false, gaps = 0, chairEarly = 0;
  const chair = e.addr('chair_tile13') !== undefined ? null : null;
  for (let f = 0; f < 60 * 60 * 2; f++) {
    e.run(1);
    const cer = e.peek('cer'), bv = e.peek('band_vis');
    if (cer === 0xFD || cer === 4 || cer === 5) started = true;
    if (!started) continue;
    if (bv === 3) shown = true; else if (shown && cer !== 6) gaps++;
    if (cer === 0xFD && !(e.peek('hl_flags') & 4)) { for (let i = 0; i < 64; i++) { const y = e.peek('oam_buf', i * 4), x = e.peek('oam_buf', i * 4 + 3); if (y < 0xF0 && x >= 36 && x <= 46 && (y === 91 || y === 149 || y === 79 || y === 161)) chairEarly++; } }
    if (cer === 6) break;
  }
  check(shown && gaps === 0 && chairEarly === 0, `changement de côté sans ralenti : bandeau continu jusqu'aux statistiques (${gaps} trames sans), chaises après le bandeau (${chairEarly})`);
}
{
  // START assis : toutes les statistiques passées, reprise, sans pause
  const h = boot(); h.bandSkip = false; startWith(h, { FMT: 3, REPLAY: 0 }); allCpu(h);
  h.cerSkip = false; h.poke('cer_skip', 0); h.poke('sc_games', 2); h.poke('sc_points', 3); h.poke('sc_points', 0, 1);
  runUntil(h, () => h.peek('cer') === 5, 60 * 60 * 3);
  h.poke('cfg', 0, CFG.CTRL);
  h.run(60);
  h.tap('START'); h.run(4);
  check(h.peek('cer') === 6 && !h.peek('paused'), `assis : START reprend aussitôt (cer ${h.peek('cer')}), sans pause (${h.peek('paused')})`);
}
{
  // balle de match gagnée : pas de ralenti, la fête tout de suite
  const m = boot(); startWith(m, { FMT: 3, REPLAY: 1 }); allCpu(m);
  m.poke('sc_games', 5); m.poke('sc_points', 2);
  let rpAfterMp = 0, over = false, rp = 0;
  for (let f = 0; f < 60 * 60 * 20 && app(m) === AS.MATCH && !over; f++) {
    m.run(1);
    const ms = m.peek('ms');
    if (ms === 2) rp = 0;                       /* (nouveau point) */
    if (ms === 5) rp++;
    if (ms === 4) { over = true; rpAfterMp = rp; }
  }
  check(over && rpAfterMp === 0, `balle de match gagnée : fin du match sans ralenti (${rpAfterMp} trames de ralenti, fin ${over})`);
}
}
if (sec('Version 2.76 : crépuscule et balle faute renvoyée')) {
console.log('Version 2.76 : crépuscule et balle faute renvoyée');
{
  // balle faute (dehors, service faux) : le joueur humain armé et à portée
  // la renvoie quand même ; l'échange reste terminé (b_live = 0)
  const e = boot(); startWith(e, { FMT: 0 }); allCpu(e);
  const c = e.addr('cfg');
  const rd16 = (n) => { const v = e.peek(n) | (e.peek(n, 1) << 8); return v > 32767 ? v - 65536 : v; };
  const s16 = (n, v) => { e.poke(n, v & 255); e.poke(n, (v >> 8) & 255, 1); };
  let tries = 0, hits = 0, live = 0, scoreOk = 0;
  for (let f = 0; f < 60 * 60 * 15 && app(e) === AS.MATCH && tries < 5 && hits < 3; f++) {   // (placement au jugé : un essai peut tomber hors de portée)
    e.run(1);
    if (e.peek('ms') !== 3 || !e.peek('b_vis')) continue;
    const by = rd16('by'), bvy = rd16('bvy'), top = (rd16('ply') >> 6) < 110;
    if (!((top && bvy < 0) || (!top && bvy > 0))) continue;
    const pts = e.peek('sc_points', 0) + e.peek('sc_points', 1) + e.peek('sc_games', 0) + e.peek('sc_games', 1);
    s16('plx', rd16('bx')); s16('ply', by + bvy * 8 + (top ? -192 : 192));
    e.nes.cpu.mem[c + CFG.CTRL] = 0; e.down('A'); e.run(4); e.up('A');
    let hit = false;
    for (let k = 0; k < 20 && !hit; k++) { e.run(1); const v = rd16('bvy'); hit = (top && v > 0) || (!top && v < 0); }
    tries++; if (hit) hits++; if (e.peek('b_live')) live++;
    runUntil(e, () => e.peek('ms') !== 3, 200);
    const pts2 = e.peek('sc_points', 0) + e.peek('sc_points', 1) + e.peek('sc_games', 0) + e.peek('sc_games', 1);
    if (pts2 !== pts || e.peek('ms') === 0) scoreOk++;   /* (point déjà compté : rien de plus) */
    e.nes.cpu.mem[c + CFG.CTRL] = 2;
  }
  check(hits === 3 && live === 0, `balle faute renvoyée par le joueur armé (${hits}/${tries}), hors jeu (b_live ${live})`);
}
{
  // crépuscule : la lumière change au changement de côté, après un plan du
  // soleil couchant (hl_mode 5, image selon la surface)
  const e = boot(); startWith(e, { FMT: 0, COND: 3, SURF: 1 }); allCpu(e);
  let shown = 0, okNt = 0, stBefore = -1, stAfter = -1, prev = 0;
  for (let f = 0; f < 60 * 60 * 20 && app(e) === AS.MATCH && !shown; f++) {
    e.run(1);
    const h = e.peek('hl_mode');
    if (h === 5 && prev !== 5) {
      stBefore = e.peek('sun_st');
      e.run(40);
      const vm = e.nes.ppu.vramMem;
      let used = new Set(); for (let i = 0; i < 960; i++) used.add(vm[0x2000 + i]);
      if (used.size > 100) okNt = 1;
      runUntil(e, () => e.peek('hl_mode') === 0, 300);
      stAfter = e.peek('sun_st'); shown = 1;
    }
    prev = h;
  }
  check(shown && okNt && stAfter === stBefore + 1, `crépuscule : plan du soleil couchant au changement de côté (image ${okNt ? 'chargée' : 'absente'}), lumière ${stBefore} -> ${stAfter}`);
}
}
if (sec('Version 2.77 : château au crépuscule, victoire sans bandeau')) {
console.log('Version 2.77 : château au crépuscule, victoire sans bandeau');
{
  // château au couchant : pierre dorée ($37/$27), plus la pierre grise qui
  // virait au rose sous l'accentuation rouge
  const e = boot(); startWith(e, { FMT: 0, ENV: 5, COND: 3 }); allCpu(e); e.run(20);
  e.poke('sc_games', 2, 0); e.poke('sc_games', 0, 1);   // (1 set de 6 jeux : 3e jeu, phase 1)
  let sun = false;
  runUntil(e, () => { if (e.peek('hl_mode') === 5) sun = true; return sun && e.peek('hl_mode') === 0; }, 60 * 60 * 6, () => { if (e.peek('cer') >= 4 && e.peek('cer') <= 6) e.poke('cer_skip', 1); });
  e.run(60);   /* (fondu du retour au court terminé) */
  const st = e.peek('sun_st'), c = [e.peek('pal_buf', 9), e.peek('pal_buf', 10)];
  check((st === 1 && c[0] === 0x37 && c[1] === 0x27) || (st === 2 && c[0] === 0x27 && c[1] === 0x17), `château au couchant : pierre dorée puis brun orangé (phase ${st}, couleurs ${c.map(x => x.toString(16))})`);
}
{
  // balle de match gagnée : aucun bandeau du score entre le point et la fête
  const e = boot(); startWith(e, { FMT: 3, REPLAY: 1 }); allCpu(e); e.run(20);
  e.poke('sc_sets', 1, 0); e.poke('sc_sets', 0, 1); e.poke('sc_games', 5, 0); e.poke('sc_games', 0, 1);
  let band = 0, done = false;
  for (let f = 0; f < 60 * 60 * 6 && !done; f++) {
    if (e.peek('ms') === 0 && e.peek('sc_points', 0) < 3) { e.poke('sc_points', 3, 0); e.poke('sc_points', 0, 1); }
    e.run(1);
    const crit = e.peek('sc_crit'), pw = e.peek('pending_winner');
    if (e.peek('ms') === 3 && (crit & 4) && ((crit >> pw) & 1) && e.peek('band_vis')) band++;
    if (e.peek('sc_over')) done = true;
  }
  check(done && band === 0, `balle de match gagnée : pas de bandeau du score avant la fête (${band} trame(s), fin ${done})`);
}
{
  // (2.77) zoom x 4 des balles limites : la trace est l'empreinte exacte de
  // la balle jugée (rayon 2 px du court) ; bonne de justesse, elle mord sur
  // la ligne ; dehors de justesse, elle ne la touche pas (à plat et en
  // perspective : l'image agrandie y est celle du court à plat)
  const res = [];
  for (const view of [0, 1]) for (const [x, y, want] of [[72, 160, 1], [71, 160, 0], [128, 219, 1], [128, 220, 0]]) {
    const e = boot(); startWith(e, { FMT: 3, REPLAY: 1, VIEW: view }); allCpu(e);
    runUntil(e, () => e.peek('ms') === 0 && e.peek('cer') === 0, 3000); e.run(30);
    e.poke('hl_sbx', x); e.poke('hl_sby', y); e.poke('hl_sbin', 1); e.poke('hl_quick', 1); e.poke('hl_t', 0); e.poke('rp_done', 0);
    e.poke('hl_mode', 2); e.poke('ms', 5);
    runUntil(e, () => e.peek('hl_c') === 40, 300); e.run(1);
    const vm = e.nes.ppu.vramMem;
    const white = (px, py) => { const t = vm[0x2400 + (py >> 3) * 32 + (px >> 3)], a = t * 16 + (py & 7); const c = ((vm[a] >> (7 - (px & 7))) & 1) | (((vm[a + 8] >> (7 - (px & 7))) & 1) << 1); const at = vm[0x27C0 + (py >> 5) * 8 + (px >> 5)], pal = (at >> ((((py >> 4) & 1) << 2) | (((px >> 4) & 1) << 1))) & 3; const col = c ? vm[0x3F00 + pal * 4 + c] : vm[0x3F00]; return col === 0x30 || col === 0x20; };
    let touch = 0, spr = 0;
    for (let k = 52; k < 56; k++) { const sy = e.peek('oam_buf', k * 4) + 1, t = e.peek('oam_buf', k * 4 + 1), sx = e.peek('oam_buf', k * 4 + 3);
      for (let j = 0; j < 8; j++) for (let i = 0; i < 8; i++) { const a = 0x1000 + t * 16 + j; if (((vm[a] >> (7 - i)) & 1) | ((vm[a + 8] >> (7 - i)) & 1)) { spr++; if (white(sx + i, sy + j)) touch++; } } }
    res.push({ view, x, y, want, touch, spr, ok: spr > 100 && (want ? touch > 0 : touch === 0) });
  }
  check(res.every(r => r.ok), `zoom x 4 : trace fidèle à l'arbitrage, bonne = sur la ligne, dehors = à côté (${res.map(r => `${r.view ? 'persp' : 'plat'} ${r.x},${r.y} ${r.want ? 'bonne' : 'dehors'} ${r.touch}`).join(' ; ')})`);
}
}
if (sec('Version 2.77 : compétition à 4 joueurs')) {
console.log('Version 2.77 : compétition à 4 joueurs');
{
  // (2.77h) en passant le mode à COMPETITION, la ligne s'appelle tout de suite JOUEURS
  const e = boot(); toMenu(e); setOpts(e, { MODE: MODE.TEAM, TYPE: 0, CPART: 0 }); e.tap('B'); e.run(5); toMenu(e);
  e.tap('DOWN'); e.run(5); e.tap('RIGHT'); e.run(60);
  const r = rowOf(e, O.CPART), lbl = r >= 0 ? e.text(3, menuY(e, r), 7) : '';
  check(e.peek('opt_val', O.MODE) === MODE.COMP && lbl === 'JOUEURS', `réglages : passage en compétition, ligne « ${lbl} » aussitôt`);
}
{
  // réglages : ligne JOUEURS (1 à 4) en compétition en simple ; TON JOUEUR pour
  // chacun ; tableau : les 4 joueurs répartis dans les quarts ; à tour de rôle
  // (manettes 1, 2, 1, 2), matchs entre joueurs humains à deux manettes,
  // enregistrés pour les deux ; à la fin, le mieux classé (podium)
  const e = boot(); e.customSkip = false; toMenu(e);
  setOpts(e, { MODE: MODE.COMP, COMP: 0, TYPE: 0, CPART: 3, P1: 0, P2: 1, FMT: 3 }); e.tap('B'); e.run(5); toMenu(e);   // (lignes refaites)
  const lbl = rowOf(e, O.CPART) >= 0 ? e.text(3, menuY(e, rowOf(e, O.CPART)), 7) : '';
  e.tap('START'); e.run(30);
  const pages = [];
  for (let k = 0; k < 5 && scr(e) === SS.CUSTOM; k++) { pages.push(e.text(4, 4, 20).replace(/\0/g, ' ').trim()); e.tap('START'); e.run(30); }
  runUntil(e, () => scr(e) === SS.COMP, 600); skipIntro(e);
  const hum = [0, 1, 2, 3].map(k => e.peek('hum_e', k));
  const log = [];
  for (let m = 0; m < 12 && scr(e) === SS.COMP; m++) {
    const me = e.peek('c_me'), st = e.peek('c_stage');
    toMatch(e);
    if (app(e) !== AS.MATCH) break;
    const c = e.addr('cfg');
    log.push(`J${me + 1}t${st}:${e.nes.cpu.mem[c + 2]}${e.nes.cpu.mem[c + 3]}`);
    e.poke('sc_winner', 0); e.poke('sc_over', 1); e.poke('ms', 4); e.poke('hl_fin', 1); e.poke('ms_timer', 2);
    runUntil(e, () => app(e) === AS.SCREENS, 120); e.run(20); e.tap('A'); e.run(40); skipIntro(e);
  }
  const want = 'J1t0:02 J2t0:12 J3t0:02 J4t0:12 J1t1:01 J2t1:10 J1t2:01';
  const ranks = [0, 1, 2, 3].map(k => e.peek('os_rk', k)).join('');
  check(lbl === 'JOUEURS' && pages.length === 4 && pages[3].startsWith('JOUEUR 4 : MANETTE 2') && hum.join() === '0,4,2,6' &&
        log.join(' ') === want && scr(e) === SS.COMP_END && e.peek('c_rank') === 1 && ranks === '1233',
    `4 joueurs : ligne ${lbl}, ${pages.length} pages TON JOUEUR, inscrits ${hum}, tours ${log.join(' ')}, rangs ${ranks}, écran ${scr(e)}`);
}
}
if (sec('Version 2.76 : parcours, tirage, fin de set, compétition à deux')) {
console.log('Version 2.76 : parcours, tirage, fin de set, compétition à deux');
{
  // pages de choix après les réglages : joueurs (affiche VS), puis le court :
  // lieu, surface et météo sur la même page (2.77), HAUT/BAS pour la ligne
  const e = boot(); e.pickSkip = false; toMenu(e); setOpts(e, { MODE: MODE.P1, P1: 0, P2: 1, ENV: 0, SURF: 0, COND: 0 });
  check(rowOf(e, O.P1) < 0 && rowOf(e, O.SURF) < 0 && rowOf(e, O.ENV) < 0 && rowOf(e, O.COND) < 0, 'réglages : plus de lignes joueurs, lieu, surface, météo');
  e.tap('START'); runUntil(e, () => scr(e) === SS.PICK, 60); e.run(40);
  const t1 = e.text(3, 1, 10);
  e.tap('RIGHT'); e.run(40);
  const p1 = e.peek('opt_val', O.P1);
  e.tap('A'); e.run(40); const t2 = e.text(3, 1, 14);
  e.tap('A'); e.run(30); const rows = [1, 2, 3].map(y => e.text(2, y, 28));
  e.tap('RIGHT'); e.run(30); const env = e.peek('opt_val', O.ENV);
  e.tap('DOWN'); e.run(4); const cur = e.text(2, 2, 2);
  e.tap('RIGHT'); e.run(30); const surf = e.peek('opt_val', O.SURF);
  e.tap('DOWN'); e.tap('RIGHT'); e.run(30); const cond = e.peek('opt_val', O.COND), emph = e.peek('ppu_emph');
  e.tap('RIGHT'); e.run(30); const cond2 = e.peek('opt_val', O.COND), rain = e.peek('cfg', 0) >= 0 && [...Array(64).keys()].filter(i => e.peek('oam_buf', i * 4 + 1) === 4 && e.peek('oam_buf', i * 4) < 0xF0).length;
  e.tap('B'); e.run(40); const back = e.text(3, 1, 14);
  e.tap('A'); e.run(30);
  e.tap('START'); runUntil(e, () => scr(e) === SS.PRESENT, 120);
  check(t1 === 'TON JOUEUR' && p1 !== 0 && p1 !== 1 && t2.startsWith('TON ADVERSAIRE') && rows[0].includes('LIEU') && rows[1].includes('SURFACE') && rows[2].includes('METEO') &&
    env === 1 && cur.includes('>') && surf === 1 && cond === 1 && cond2 === 2 && rain >= 5 && back.startsWith('TON ADVERSAIRE') && scr(e) === SS.PRESENT && e.peek('cfg', 0) >= 0,
    `pages : joueur (${p1}), adversaire, puis le court : lieu (${env}), surface (${surf}), météo (nuit ${cond}, pluie ${cond2} : ${rain} gouttes) sur la même page ; B revient, START valide (« ${rows.map(r => r.trim()).join(' / ')} »)`);
  void emph;
}
{
  // tirage au sort : devant l'arbitre, le gagnant (humain) choisit sur le bandeau
  const e = boot(); e.cerSkip = false; e.tossSkip = false; toMenu(e); setOpts(e, { MODE: MODE.P1, FMT: 0 }); e.tap('START');
  passCustom(e); runUntil(e, () => scr(e) === SS.PRESENT, 120); e.run(10); e.tap('A'); runUntil(e, () => app(e) === AS.MATCH, 100);
  const c = e.addr('cfg'); e.nes.cpu.mem[c + CFG.CTRL + 1] = 0;   // (les deux joueurs à la manette 1 : le gagnant est humain)
  let atNet = false, band = false;
  runUntil(e, () => { if (e.peek('cer') === 3 && Math.abs((e.peek('ply', 0) | (e.peek('ply', 1) << 8)) / 64 - 132) < 20) atNet = true; return e.peek('cer') === 11 && e.peek('band_vis') === 3; }, 900);
  const nt1 = (y) => String.fromCharCode(...e.nes.ppu.vramMem.slice(0x2400 + y * 32 + 2, 0x2400 + y * 32 + 30));
  band = e.peek('band_vis') === 3 && nt1(26).includes('GAGNE LE TIRAGE');
  const w = e.peek('sc_server');
  e.tap('B'); e.run(20); const q2 = nt1(27);                       // le gagnant choisit le côté
  e.tap('A'); e.run(20); const q4 = nt1(27);                       // EN HAUT
  const topOk = (((w & 1) ^ e.peek('sc_swap')) === 1);
  e.tap('A');                                                      // le perdant sert
  runUntil(e, () => e.peek('cer') === 13 && e.peek('band_vis') === 3, 400); e.run(10);
  const rc = nt1(26), rc2 = nt1(27); e.run(120); const waits = e.peek('cer') === 13;
  e.tap('A'); runUntil(e, () => e.peek('cer') === 0, 60);
  check(atNet && band && q2.includes('EN HAUT') && q4.includes('RECEVOIR') && topOk && e.peek('sc_server') === (w ^ 1) && e.peek('ms') === 0,
    `tirage : au filet, « GAGNE LE TIRAGE » ; le gagnant choisit le côté (EN HAUT), le perdant sert (« ${q2.trim()} », « ${q4.trim()} »)`);
  check(rc.includes('SERT EN') && rc2.includes('RECOIT') && rc2.includes('A : JOUER') && waits, `récapitulatif avant le service (« ${rc.trim()} », « ${rc2.trim()} »), attend A`);
}
{
  // fin de set : les joueurs s'assoient, bandeau du set, puis l'écran du set ;
  // au retour, ils repartent des chaises
  const e = boot(); e.cerSkip = false; startWith(e, { FMT: 1, REPLAY: 0 }); allCpu(e);
  let st = 0, seated = false, toScreen = false;
  runUntil(e, () => { if (e.peek('brk_pend') && e.peek('st_t') === 8) st = 1; if (e.peek('cer') === 5 && e.peek('brk_pend')) seated = true; return app(e) === AS.SCREENS && scr(e) === SS.SETBREAK; }, 60 * 60 * 15,
    () => { if (e.peek('cer') === 1 || e.peek('cer') === 2 || e.peek('cer') === 3 || e.peek('cer') === 11) e.poke('cer_skip', 1); });
  toScreen = scr(e) === SS.SETBREAK && e.text(8, 7, 4) === 'SETS';
  e.run(30); e.tap('A'); runUntil(e, () => app(e) === AS.MATCH, 60); e.run(2);
  check(st && seated && toScreen && e.peek('cer') === 6, `fin de set : bandeau du set (${st}), assis (${seated}), écran du set (${toScreen}), retour aux places depuis les chaises (cer ${e.peek('cer')})`);
}
{
  // (2.77) avec un joueur humain, le bandeau du set attend un appui avant
  // l'écran du set
  const e = boot(); e.cerSkip = false; e.bandSkip = false; startWith(e, { FMT: 1, REPLAY: 0 }); allCpu(e);
  runUntil(e, () => e.peek('ms') === 2, 3000, () => { if ([1, 2, 3, 11, 13].includes(e.peek('cer'))) e.poke('cer_skip', 1); });
  e.poke('cer_skip', 0); e.poke('sc_games', 5, 0); e.poke('sc_games', 0, 1); e.poke('sc_points', 3, 0); e.poke('sc_points', 0, 1);
  const shown = runUntil(e, () => e.peek('cer') === 5 && e.peek('brk_pend') === 1 && e.peek('st_t') === 8, 60 * 60 * 6);
  e.poke('cfg', 0, CFG.CTRL);                     /* (un joueur prend la manette) */
  e.run(300);
  const waits = e.peek('cer') === 5 && e.peek('brk_pend') === 1 && app(e) === AS.MATCH;
  e.tap('A'); runUntil(e, () => app(e) === AS.SCREENS && scr(e) === SS.SETBREAK, 300);
  check(shown && waits && scr(e) === SS.SETBREAK, `fin de set avec un joueur : le bandeau du set attend l'appui (${shown}, ${waits}), puis l'écran du set (${scr(e)})`);
}
{
  // compétition à deux joueurs humains : chacun ses matchs, puis la finale à deux manettes
  const e = boot(); startWith(e, { MODE: MODE.COMP, COMP: 0, TYPE: 0, CPART: 1, P1: 0, P2: 5, FMT: 3 });
  skipIntro(e);
  const seq = []; let duo = 0;
  for (let m = 0; m < 8 && scr(e) === SS.COMP; m++) {
    seq.push(e.peek('c_me'));
    toMatch(e);
    if (app(e) !== AS.MATCH) break;
    const c = e.addr('cfg');
    if (e.nes.cpu.mem[c + CFG.CTRL] !== 2 && e.nes.cpu.mem[c + CFG.CTRL + 1] !== 2) duo++;
    e.poke('sc_winner', 0); e.poke('sc_over', 1); e.poke('ms', 4); e.poke('hl_fin', 1); e.poke('ms_timer', 2);
    runUntil(e, () => app(e) === AS.SCREENS, 60); e.run(20);
    e.tap('A'); e.run(40); skipIntro(e);
  }
  check(e.peek('c_h2') === 4 && seq.join('') === '01100' && duo === 1 && scr(e) === SS.COMP_END && e.peek('c_rank') === 1,
    `compétition à deux : tours ${seq.join('')} (0 joueur 1, 1 joueur 2), finale à deux manettes (${duo}), podium`);
}
}
if (sec('Compensation 50 Hz (PAL)')) {
console.log('Compensation 50 Hz (PAL)');
{
  const e = boot(); e.poke('is_pal', 1); startWith(e, { FMT: 3 }); allCpu(e);
  check(runUntil(e, () => app(e) === AS.SCREENS, 60 * 60 * 25), 'match complet en mode 50 Hz');
}

}
secEnd();
if (LIST) process.exit(0);
console.log(`\n${passes} vérifications réussies, ${fails} échec(s) ; pile contrôlée ${spChecks} fois`);
untilSave();
process.exit(fails ? 1 : 0);
