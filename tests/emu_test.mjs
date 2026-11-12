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
  check(scr(e) === SS.MENU && e.peek('nvis') === 14, 'configuration match : 14 lignes (pas de ligne JEU contre le CPU)');
  e.tap('DOWN'); e.tap('RIGHT'); e.tap('RIGHT'); e.tap('RIGHT'); e.run(5);
  check(e.peek('opt_val', O.MODE) === MODE.COMP && e.peek('nvis') === 15, 'mode compétition : lignes adaptées');
  e.tap('RIGHT'); e.run(5);
  check(e.peek('opt_val', O.MODE) === MODE.TRAIN && e.peek('nvis') === 10, 'mode entraînement : lignes adaptées');
  e.tap('B'); e.run(5);
  check(scr(e) === SS.TITLE, 'B revient au titre');
}

console.log('Mode en équipe (double)');
{
  const e = boot(); toMenu(e); setOpts(e, { MODE: MODE.TEAM, TYPE: 0, CPART: 1 }); e.tap('B'); e.run(5); toMenu(e);
  check(e.peek('nvis') === 15 && e.text(3, 7, 10) === 'PARTENAIRE' && e.text(3, 8, 8) === 'JOUE PAR', 'pas de ligne JEU ; lignes PARTENAIRE et JOUE PAR');
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
  e.poke('cursor', rowOf(e, O.ENV));
  const seen = new Set(); for (let i = 0; i < 6; i++) { e.tap('RIGHT'); seen.add(e.peek('opt_val', O.ENV)); }
  check(seen.has(4) && !seen.has(3), 'court Margaux proposé d\'emblée (club nudiste toujours verrouillé)');
  setOpts(e, { ENV: 4 }); e.tap('START'); e.run(15);
  check(e.text(4, 24, 22) === 'BIENVENUE SUR LE COURT', 'présentation du court Margaux');
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
    // (pendant la construction d'un écran, la pile C contient l'argument de
    // screens_enter : contrôle seulement pendant le match)
    if ((e.frames & 255) === 0 && app(e) === AS.MATCH && !spOk(e)) spGood = false;
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
  const r = fullMatch('double dur/salle', { MODE: MODE.P2, FMT: 3, TYPE: 1, SURF: 2, ENV: 2 }, 60 * 60 * 25);
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
  check(scr(e) === SS.PALMARES && e.text(3, 4, 5) === 'JOUES', 'écran palmarès');
  e.tap('RIGHT'); e.run(8); e.tap('RIGHT'); e.run(8);
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
// Grand Martinot : écran d'annonce d'un tournoi (état 10) avant son écran d'étape
const litScr = (e) => e.peek('ppu_mask') !== 0 && e.peek('fade_lvl') === 0;
const skipIntro = (e) => {
  runUntil(e, () => litScr(e), 90);
  if (scr(e) === 10) { e.tap('A'); runUntil(e, () => scr(e) !== 10 && litScr(e), 120); }
};
function playComp(type, opts, wins) {
  const e = boot(); startWith(e, Object.assign({ MODE: MODE.COMP, COMP: type, FMT: 3 }, opts));
  skipIntro(e);
  let matches = 0, code = '';
  const CN = ['TOURNOI', 'POULES', 'CHAMPIONNAT', 'SAISON', 'PERSONNALISE', 'GRAND MARTINOT', 'MARTINWORLD'][type];
  check(scr(e) === SS.COMP && (type === 5 ? e.text(3, 1, 17) === 'LE GRAND MARTINOT' : type === 6 ? e.text(3, 1, 16) === 'MARTINWORLD TOUR' : e.text(18, 1, CN.length) === CN), `${CN} : écran d'étape (titre lisible)`);
  while (scr(e) === SS.COMP && matches < 24) {
    if (matches === 1) code = e.text(10, 21, 15).replace(/ /g, '');
    e.tap('A'); runUntil(e, () => scr(e) === SS.PRESENT, 60); e.run(3); e.tap('A'); e.run(30);
    if (app(e) !== AS.MATCH) break;
    // victoire forcée du joueur pour parcourir la compétition
    e.poke('sc_winner', wins(matches) ? 0 : 1); e.poke('sc_over', 1); e.poke('ms', 4); e.poke('ms_timer', 2);
    runUntil(e, () => app(e) === AS.SCREENS, 60); e.run(20);
    matches++;
    e.tap('A'); e.run(40);
    skipIntro(e);
  }
  return { e, matches, code };
}
{
  // le Grand Martinot : 4 tournois de 16 (huitièmes, quarts, demis, finale),
  // dur, terre battue, gazon, dur ; les 16 matchs gagnés : Grand Chelem
  const g = boot(); startWith(g, { MODE: MODE.COMP, COMP: 5, FMT: 3, ENV: 2, COND: 2 });
  // écran d'annonce : image (tuiles $60 et suivantes), nom, fanfare ; puis l'écran d'étape, décor rétabli
  const rom2 = g.nes.rom.rom[2];
  let tiles = 0; for (let i = 0x600; i < 0x1000; i++) if (g.nes.ppu.vramMem[i] !== rom2[i]) tiles++;
  check(scr(g) === 10 && g.peek('music_cur') === 11 && g.text(7, 4, 18) === "OPEN D'AUGUSTRALIE" && tiles > 500,
    `Grand Martinot : annonce de l'Open d'Augustralie (image, fanfare ${g.peek('music_cur')})`);
  runUntil(g, () => litScr(g), 90); g.tap('A'); runUntil(g, () => scr(g) === SS.COMP && litScr(g), 120);
  let back = 0; for (let i = 0x600; i < 0x1000; i++) if (g.nes.ppu.vramMem[i] !== rom2[i]) back++;
  check(scr(g) === SS.COMP && back === 0, `Grand Martinot : écran d'étape ensuite, décor rétabli (${back} octet(s))`);
  check(g.peek('c_env') === 0 && g.peek('c_cond') === 0, 'Grand Martinot : toujours au stade, de jour (lieu et conditions imposés)');
  check(scr(g) === SS.COMP && g.peek('c_n') === 16 && g.text(3, 3, 18) === "OPEN D'AUGUSTRALIE" && g.text(3, 4, 18) === 'HUITIEME DE FINALE',
    `Grand Martinot : 16 joueurs, « ${g.text(3, 3, 18)} », ${g.text(3, 4, 18)}`);
  const names = new Set(); for (let i = 0; i < 16; i++) names.add(g.peek('c_pl', i));
  check(names.size === 16, `Grand Martinot : 16 joueurs différents`);
  {
    const h = boot(); startWith(h, { MODE: MODE.COMP, COMP: 5, FMT: 3 }); skipIntro(h);
    const seen = [];
    for (let m = 0; m < 5; m++) {
      h.tap('A'); runUntil(h, () => scr(h) === SS.PRESENT, 60); h.run(3); h.tap('A'); h.run(30);
      h.poke('sc_winner', 0); h.poke('sc_over', 1); h.poke('ms', 4); h.poke('ms_timer', 2);
      runUntil(h, () => app(h) === AS.SCREENS, 60); runUntil(h, () => scr(h) === SS.RESULTS && litScr(h), 120);
      h.tap('A'); runUntil(h, () => scr(h) !== SS.RESULTS && litScr(h), 120);
      seen.push(scr(h) === 10 ? h.peek('music_cur') : 0); skipIntro(h);
    }
    check(seen.join(',') === '0,0,0,12,0', `annonce au premier tour de chaque tournoi seulement (Debogarros après 4 victoires : ${seen})`);
  }
  const { e, matches } = playComp(5, {}, () => true);
  for (let k = 0; k < 5 && scr(e) !== SS.COMP_END; k++) { e.tap('A'); e.run(60); }
  e.run(30);
  const podTxt = [...Array(30).keys()].map((y) => e.text(0, y, 32)).join('|');
  check(matches === 16 && scr(e) === SS.COMP_END && e.peek('c_rank') === 1 && e.peek('c_slam') === 1 && podTxt.includes('GRAND CHELEM !') && podTxt.includes('LE GRAND MARTINOT'),
    `Grand Martinot : 16 matchs gagnés, Grand Chelem et podium (${matches} matchs)`);
  // en double : 8 équipes ; une défaite ne termine que le tournoi en cours
  const r = playComp(5, { TYPE: 1, P2: 3 }, (m) => m !== 1);
  check(r.e.peek('c_n') === 8 && r.matches === 11 && r.e.peek('c_slam') === 0 && scr(r.e) === SS.COMP_END,
    `Grand Martinot en double : 8 équipes, défaite en demi au 1er tournoi puis 3 tournois (${r.matches} matchs), pas de Grand Chelem`);
  // menu : explication de la compétition sous le curseur
  const m = boot(); toMenu(m); setOpts(m, { MODE: MODE.COMP, COMP: 5 }); m.tap('B'); m.run(30); toMenu(m); m.run(10);
  m.poke('cursor', rowOf(m, O.COMP)); m.run(8);
  const h = m.text(2, 24, 28).trim(), h4 = m.text(2, 27, 28).trim();
  m.poke('cursor', rowOf(m, O.P1)); m.run(8);
  check(rowOf(m, O.ENV) < 0 && rowOf(m, O.COND) < 0 && rowOf(m, O.SURF) < 0, 'menu du Grand Martinot : ni lieu, ni conditions, ni surface (imposés)');
  check(h.startsWith('4 TOURNOIS DE 16') && h4.includes('CHELEM') && m.text(3, 24, 5) === 'STYLE' && m.text(2, 25, 8) === 'HAUT/BAS',
    `menu : explication de la compétition (« ${h} »), puis style et aide`);
}
{
  // le Martinworld Tour : 7 tournois de 8 (quarts, demis, finale), un par surface
  const g = boot(); startWith(g, { MODE: MODE.COMP, COMP: 6, FMT: 3, ENV: 3, COND: 2, SURF: 4 });
  const row = (e, y) => e.text(0, y, 32).replace(/\0/g, ' ').trim();
  check(scr(g) === 10 && g.peek('music_cur') === 11 && row(g, 4) === "OPEN D'AUGUSTRALIE" && row(g, 2) === 'MARTINWORLD TOUR'
    && row(g, 23) === 'TOURNOI 1/7 : DUR', `Martinworld Tour : annonce du 1er tournoi (« ${row(g, 23)} »)`);
  runUntil(g, () => litScr(g), 90); g.tap('A'); runUntil(g, () => scr(g) === SS.COMP && litScr(g), 120);
  check(g.peek('c_env') === 0 && g.peek('c_cond') === 0 && g.peek('c_n') === 8 && g.text(3, 1, 16) === 'MARTINWORLD TOUR',
    'Martinworld Tour : 8 joueurs, au stade, de jour');
  // chaque tournoi : annonce (image, fanfare, rang), surface, nom ; une défaite au 1er tour à chaque fois
  const h = boot(); startWith(h, { MODE: MODE.COMP, COMP: 6, FMT: 3 });
  const seen = [], surfs = [], names = [], ranks = [];
  for (let m = 0; m < 7; m++) {
    if (scr(h) === 10) {
      seen.push(h.peek('music_cur')); ranks.push(row(h, 23)); names.push(row(h, 4));
      runUntil(h, () => litScr(h), 90); h.tap('A'); runUntil(h, () => scr(h) === SS.COMP && litScr(h), 120);
    } else seen.push(0);
    h.tap('A'); runUntil(h, () => scr(h) === SS.PRESENT, 60); h.run(3); h.tap('A'); h.run(30);
    surfs.push(h.peek('cfg', 14));
    h.poke('sc_winner', 1); h.poke('sc_over', 1); h.poke('ms', 4); h.poke('ms_timer', 2);
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
      r.tap('A'); runUntil(r, () => scr(r) === SS.PRESENT, 60); r.run(3); r.tap('A'); r.run(30);
      r.poke('sc_winner', 0); r.poke('sc_over', 1); r.poke('ms', 4); r.poke('ms_timer', 2);
      runUntil(r, () => app(r) === AS.SCREENS, 60); r.run(20); r.tap('A'); r.run(40); skipIntro(r);
    }
    const rc = r.text(10, 21, 15).replace(/ /g, '');
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
      && f.text(3, 3, 18) === 'MARGAUX-LES-NEIGES' && f.peek('c_pl', 0) === 16,
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
  e.tap('A'); runUntil(e, () => scr(e) === SS.TITLE && e.peek('ppu_mask') !== 0, 200); e.run(10);
  const rom2 = e.nes.rom.rom[2]; let diff = 0; for (let i = 0; i < 8192; i++) if (chr[i] !== rom2[i]) diff++;
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
  check(scr(e) === SS.TITLE && e.peek('scroll_y') === 0 && e.peek('scroll_nt') === 0 && e.text(4, 13, 24) === 'PLATINUM DIAMOND EDITION',
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
  e.poke('cursor', rowOf(e, O.ENV));
  const names = new Set();
  for (let i = 0; i < 8; i++) { e.tap('RIGHT'); e.run(3); envs.add(e.peek('opt_val', O.ENV)); names.add(e.text(16, 4 + rowOf(e, O.ENV), 7).replace(/\0/g, ' ').trim()); }
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
  e.tap('A'); runUntil(e, () => scr(e) === SS.TITLE && e.peek('ppu_mask') !== 0, 200); e.run(10); toMenu(e);
  setOpts(e, { MODE: MODE.P1, TYPE: 0 }); e.tap('B'); e.run(5); toMenu(e);
  e.poke('cursor', rowOf(e, O.ENV));
  let seen = false; for (let i = 0; i < 4; i++) { e.tap('RIGHT'); if (e.peek('opt_val', O.ENV) === 3) seen = true; }
  check(seen, 'lieu « club nudiste » proposé après déblocage');
  // (chargement des tuiles au naturel + fondu : écran construit en ~25 trames)
  setOpts(e, { ENV: 3 }); e.tap('START'); e.run(40);
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
  e.tap('A'); runUntil(e, () => scr(e) === SS.TITLE && e.peek('ppu_mask') !== 0, 200); e.run(10); toMenu(e); setOpts(e, { MODE: MODE.COMP, COMP: 0 }); e.tap('START'); e.run(40);
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
    // fin de match : ms_timer décompte chaque trame logique ; le CPU (équipe 0)
    // bat un humain (équipe 1) : pas de fête, qui fausserait la mesure
    e.nes.cpu.mem[e.addr('cfg') + CFG.CTRL + 1] = 0; e.poke('sc_winner', 0);
    e.poke('ms', 4); e.poke('ms_timer', 250); e.run(120);
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
  const f = boot(); startWith(f, { FMT: 3, LEVEL: 1 }); allCpu(f);
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
  e.poke('cursor', rowOf(e, O.ENV)); e.poke('opt_val', 3, O.ENV); e.tap('RIGHT'); e.run(5);
  check(e.peek('opt_val', O.ENV) === 4 && e.peek('music_cur') === 7, 'menu : court Margaux choisi, sa musique démarre');
  { const mm = e.nes.mmap, w = mm.write.bind(mm), vol = new Set();
    mm.write = (a, v) => { if (a === 0x4004) vol.add(v); return w(a, v); };
    e.run(600); mm.write = w;
    check([...vol].every((v) => v === 0x30 || (v & 0x30) === 0x30), `valse Margaux : notes tenues à volume constant (${[...vol].map((v) => v.toString(16))})`); }
  e.tap('LEFT'); e.run(5);
  check(e.peek('music_cur') === 1, 'autre lieu : retour à la polka');
  e.tap('RIGHT'); e.run(5); e.tap('START'); e.run(20);
  check(scr(e) === SS.PRESENT && e.peek('music_cur') === 7, 'présentation au court Margaux : valse');
  e.tap('A'); runUntil(e, () => app(e) === AS.MATCH, 60);
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
  e.tap('A'); runUntil(e, () => app(e) === AS.MATCH, 60);
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
    e.poke('hitter', 1); e.poke('hitter_p', 1); e.poke('rally_hits', 3); e.poke('bgrav', 6);
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
  check(e.peek('snd_mode') === 2 && e.text(16, 4 + row(O.SOUND), 5) === 'AUCUN', 'SON : aucun');
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

console.log('Margaux jouable et fatigue');
{
  // 1 joueur contre CPU : toujours en simple (le double avec partenaire CPU
  // est le mode en équipe) ; la ligne JEU n'existe qu'à 2 joueurs adversaires
  const t = boot(); toMenu(t);
  check(t.peek('opt_val', O.MODE) === MODE.P1 && t.text(3, 6, 8) === 'JOUEUR 1', '1 joueur contre CPU : pas de ligne JEU (simple seulement)');
  t.poke('cursor', 1); t.tap('RIGHT');
  let dark = 0; for (let n = 0; n < 12; n++) { t.run(1); if (!t.nes.ppu.f_bgVisibility) dark++; }
  check(t.peek('opt_val', O.MODE) === MODE.P2 && t.text(3, 6, 3) === 'JEU' && dark === 0, `2 joueurs adversaires : ligne JEU ; menu redessiné sans couper l'écran (${dark} trame(s) noire(s))`);
  t.tap('LEFT'); t.run(12);
  check(t.text(3, 6, 8) === 'JOUEUR 1' && t.text(3, 4 + t.peek('nvis'), 13).replace(/[\0 ]/g, '') === '', 'retour à 1 joueur : lignes réécrites, ancienne ligne en trop effacée');
  const u = boot(); startWith(u, { MODE: MODE.P1, TYPE: 1 });
  check(app(u) === AS.MATCH && u.peek('npl') === 2, `1 joueur contre CPU avec un ancien réglage « double » : match en simple (${u.peek('npl')} joueurs)`);
  // Margaux (personnage 16) : menu, présentation, code de reprise
  const e = boot(); toMenu(e);
  e.poke('cursor', rowOf(e, O.P1)); e.poke('opt_val', 15, O.P1); e.tap('RIGHT'); e.run(5);
  check(e.peek('opt_val', O.P1) === 16 && e.text(16, 4 + rowOf(e, O.P1), 7) === 'MARGAUX', 'MARGAUX sélectionnable (17e personnage)');
  const nn = [];
  for (let k = 0; k < 4; k++) { e.tap('RIGHT'); e.run(5); nn.push(e.text(16, 4 + rowOf(e, O.P1), 5).replace(/\0/g, ' ').trim()); }
  check(nn.join(',') === 'MAX,LOLA,PAPI,TITOU', `nouveaux personnages après Margaux : ${nn.join(', ')}`);
  e.tap('RIGHT'); e.run(5);
  check(e.peek('opt_val', O.P1) === 0, 'retour au premier personnage après TITOU');
  const g = boot(); toMenu(g); setOpts(g, { MODE: MODE.P2, P1: 16, TYPE: 1, P2: 3 }); g.tap('START'); g.run(30);
  check(scr(g) === SS.PRESENT && g.text(6, 10, 7) === 'MARGAUX' && g.text(6, 11, 1) === '+' && g.text(8, 11, 3) !== '   ',
    `présentation en double : MARGAUX, partenaire sur la ligne suivante (${g.text(6, 11, 6).replace(/\0/g, " ")})`);
  const c = boot(); startWith(c, { MODE: MODE.COMP, COMP: 0, FMT: 3, P1: 16, TYPE: 1, P2: 7 });
  const code = c.text(10, 21, 15).replace(/ /g, '');
  check(scr(c) === SS.COMP && c.text(3, 6, 5) === 'MX/EM', `équipe abrégée « MX/EM » (pas de confusion avec MAYA « MA ») ; code ${code}`);
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
  const er = (() => { for (let i = 0; i < n.peek('nvis'); i++) if (n.peek('vis', i) === O.ENV) return i; return -1; })();
  n.poke('cursor', er); n.tap('RIGHT'); n.run(5);
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
  const e = boot(); startWith(e, { ENV: 0, SURF: 2 }); allCpu(e); e.run(200);
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
  check(seen > 20 && stands().join() === before, `ola : la vague descend les tribunes (${seen} trames) puis tout est rétabli`);
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
  e.poke('ms', 2); e.run(1); e.poke('ms', 0); e.poke('ms_timer', 250); log.length = 0; e.run(60);
  const claps = log.filter((x) => x === '1/5').length;
  log.length = 0; e.tap('A', 1, 1, 0); e.run(3);
  const hush = log.includes('silence');
  // balle dehors de 2 px (couloir gauche du simple)
  e.poke('ms', 2); e.run(1); log.length = 0;
  s16('bx', 72 * 64); s16('by', 100 * 64); e.poke('end_reason', 2); e.poke('ump_arm', 1); e.poke('ms', 3); e.poke('ms_timer', 80); e.run(40);
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
  const e = boot(); startWith(e, { FMT: 3 }); allCpu(e);
  const mm = e.nes.mmap, w = mm.write.bind(mm); let beep = false;
  mm.write = (a, v) => { if (a === 0x4002 && v === 0x5C) beep = true; return w(a, v); };
  let faults = 0, beeps = 0, prev = e.peek('ms');
  for (let f = 0; f < 60 * 60 * 6 && faults < 3; f++) {
    beep = false; const srv = e.peek('b_serve'); e.run(1);
    const ms = e.peek('ms');
    if (prev !== 3 && ms === 3 && srv && e.peek('rally_hits') <= 1 && e.peek('ump_arm')) { faults++; if (beep) beeps++; }
    prev = ms;
  }
  check(faults > 0 && beeps === faults, `service faux : bip (${beeps}/${faults})`);
}
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
  const f = boot(); startWith(f, { FMT: 3, LEVEL: 1 }); allCpu(f);
  let cpu = 0, serves = 0, prev = 0;
  runUntil(f, () => { const m = f.peek('ms'); if (m === 2 && prev === 1) { serves++; cpu += f.peek('srv_canon'); } prev = m; return serves >= 40 || app(f) !== AS.MATCH; }, 60 * 60 * 10);
  check(serves >= 20 && cpu === 0, `CPU Normal : aucun service canon sur ${serves} services`);
}

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
  check(scr(e) === SS.SETBREAK && tx(8, 2, 14) === 'FIN DU 1ER SET' && tx(8, 6, 4) === 'SETS', 'fin du 1er set : écran récapitulatif (tableau des sets, statistiques)');
  const l0 = tx(8, 7, 9), l1 = tx(8, 8, 9);
  check(/^[A-Z]{4}\s+\d/.test(l0) && /^[A-Z]{4}\s+\d/.test(l1), `tableau des sets : « ${l0.trim()} » / « ${l1.trim()} »`);
  check(e.peek('music_cur') === 8, 'petite musique sur l\'écran de fin de set');
  // n'importe quel bouton (ici B) ferme l'écran aussitôt, sans fondu
  e.tap('B', 1, 1, 0); let n = 0;
  while (app(e) !== AS.MATCH && n < 30) { e.run(1); n++; }
  check(app(e) === AS.MATCH && n <= 16 && e.peek('music_cur') === 0, `B : retour au court en ${n + 1} trames, musique coupée`);
  e.run(30);
  check(app(e) === AS.MATCH && /^[A-Z]{4} \d \d/.test(msgText(e)), `reprise du match, score des sets dans le bandeau : « ${msgText(e)} »`);
  // un jeu plus tard : récapitulatif après le jeu
  let after = '';
  runUntil(e, () => {
    if (e.peek('ms') === 0 && e.peek('sc_games', 0) + e.peek('sc_games', 1) === 0) { e.poke('sc_points', 3, 0); e.poke('sc_points', 0, 1); }
    const t = msgText(e); if (/^[A-Z]{4} \d \d/.test(t) && e.peek('sc_games', 0) + e.peek('sc_games', 1) > 0) after = t; return after !== '';
  }, 60 * 60 * 3);
  check(after !== '', `après un jeu : « ${after} »`);
}

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

console.log('Rotation des sprites (double)');
{
  // 4 joueurs près du filet : jusqu'à 12 sprites sur une ligne. Avec la rotation, chaque
  // sprite est parmi les 8 premiers de ses lignes au moins une trame sur deux
  const e = boot(); startWith(e, { MODE: MODE.P2, FMT: 3, TYPE: 1 }); allCpu(e);
  runUntil(e, () => e.peek('ms') === 0, 300);
  e.tap('START', 1, 1, 2);
  const s16 = (n, v, o = 0) => { e.poke(n, v & 255, o); e.poke(n, (v >> 8) & 255, o + 1); };
  [[96, 146], [104, 136], [150, 144], [158, 138]].forEach(([x, y], i) => { s16('plx', x * 64, i * 2); s16('ply', y * 64, i * 2); });
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

console.log('Victoire : sauts de joie et confettis');
{
  const confetti = boot().peek('confetti_tile');
  // le joueur humain (équipe 0) gagne : confettis et bonds du vainqueur
  const e = boot(); startWith(e, { MODE: MODE.P1 }); e.run(60);
  e.poke('sc_winner', 0); e.poke('ms_timer', 160); e.poke('ms', 4); e.run(2);
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
  g.poke('sc_winner', 0); g.poke('ms_timer', 160); g.poke('ms', 4);
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
  d.poke('sc_winner', 0); d.poke('ms', 4); d.poke('ms_timer', 250); d.run(120);
  check(250 - d.peek('ms_timer') >= 118, `fête en double : ${250 - d.peek('ms_timer')} trames logiques sur 120 (au plus 1 perdue, invisible)`);
  // le CPU bat le joueur humain : pas de fête
  const f = boot(); startWith(f, { MODE: MODE.P1 }); f.run(60);
  f.poke('sc_winner', 1); f.poke('ms_timer', 160); f.poke('ms', 4); f.run(10);
  check(!oamTiles(f).has(confetti), 'défaite contre le CPU : pas de confettis');
}
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
console.log('Animation des joueurs');
{
  // tuiles : réserve commune (rangées partagées), versions normale et au naturel de même taille
  const e = boot();
  const nb = e.peek16('people_bytes');
  let okm = true;
  // (version 2.43 : rangées des célébrations et du revers dans la zone partagée, swap_base)
  const rb3 = (sym, off = 0) => e.nes.rom.rom[3][e.addr(sym) - 0x8000 + off];
  const sb = rb3('swap_base'), inPool = (t) => (t >= 0x10 && t + 1 < 0x10 + nb / 16) || (t >= sb && t + 1 < sb + 34);
  for (const mp of ['person_map_m', 'person_map_f']) for (let i = 0; i < 162; i++) { if (!inPool(e.peek(mp, i))) okm = false; }
  for (let i = 0; i < 24; i++) { const t = e.peek('prof_map', i); if (t < 0x10 || t + 1 >= 0x10 + nb / 16) okm = false; }
  check(okm && nb % 32 === 0, `poses dans la réserve de tuiles des joueurs (${nb / 16} tuiles permanentes et 34 partagées pour 27 poses x 4 vues + 4 images de profil)`);
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
  for (let f = 0; f < 60 * 60 * 3; f++) {
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
  check([11, 12, 15, 19, 20].every((k) => poses.has(k)) && [14, 13].some((k) => poses.has(k)), 'préparation, impact, volée ou amortie, rebonds avant le service, course en 4 temps');
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
console.log('IA : tactique selon le score');
{
  // mené de 3 jeux : le CPU prend des risques ; devant : il joue placé ;
  // balle de break contre lui au service : première balle assurée
  const e = boot(); startWith(e, { FMT: 0 }); allCpu(e);
  runUntil(e, () => e.peek('ms') === 2, 600);
  e.poke('sc_games', 1, 0); e.poke('sc_games', 4, 1);
  runUntil(e, () => e.peek('ms') === 3, 3000); runUntil(e, () => e.peek('ms') === 0, 400);
  const g0 = e.peek('sc_games', 0), g1 = e.peek('sc_games', 1);
  const pw = (t) => { const v = e.peek('tac_pow', t); return v > 127 ? v - 256 : v; };
  const mg = (t) => { const v = e.peek('tac_margin', t); return v > 127 ? v - 256 : v; };
  const trailing = g1 - g0 >= 2;
  check(trailing && pw(0) >= 5 && mg(0) < 0 && e.peek('tac_net', 0) >= 50 && mg(1) >= 3 && e.peek('tac_err', 1) === 0x10,
    `mené ${g0}-${g1} : le CPU attaque (force +${pw(0)}, marge ${mg(0)}, filet ${e.peek('tac_net', 0)}) ; le meneur joue placé (marge +${mg(1)})`);
  // balle de break contre le serveur
  const srv = e.peek('sc_server');
  e.poke('sc_points', 3, srv ^ 1); e.poke('sc_points', 1, srv); e.poke('sc_games', 2, 0); e.poke('sc_games', 2, 1);
  runUntil(e, () => e.peek('ms') === 2, 600); runUntil(e, () => e.peek('ms') === 3, 3000); runUntil(e, () => e.peek('ms') === 0, 400);
  const s2 = e.peek('sc_server'), bp = e.peek('sc_points', s2 ^ 1) >= 3 && e.peek('sc_points', s2 ^ 1) > e.peek('sc_points', s2);
  check(!bp || e.peek('tac_safe', s2) === 1, `balle de break contre le serveur : première balle assurée (${bp ? 'balle de break' : 'situation passée'})`);
}
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
  e.poke('sc_winner', 0); e.poke('ms_timer', 160); e.poke('ms', 4); e.run(170);   // feux d'artifice
  check(bad.length === 0, `aucune note coupée par l'unité de balayage sur console réelle${bad.length ? ' (périodes ' + [...new Set(bad)].join(', ') + ')' : ''}`);
}
console.log('Vue en perspective (option VUE)');
{
  // menu : ligne VUE, à plat par défaut
  const m = boot(); toMenu(m);
  const r = rowOf(m, 24);
  check(r > 0 && m.text(3, 4 + r, 3) === 'VUE' && m.text(16, 4 + r, 6) === 'A PLAT', 'menu : ligne VUE (à plat par défaut)');
  m.poke('cursor', r); m.tap('RIGHT'); m.run(5);
  check(m.peek('opt_val', 24) === 1 && m.text(16, 4 + r, 11) === 'PERSPECTIVE', 'menu : VUE PERSPECTIVE');
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
  const rom2 = f.nes.rom.rom[2]; let d2 = 0; for (let i = 0; i < 4096; i++) if (f.nes.ppu.vramMem[i] !== rom2[i]) d2++;
  check(f.peek('persp') === 0 && d2 === 0, `hors du match : tuiles de décor d'origine rétablies (${d2} octet(s) différent(s)), projection coupée`);
  // match complet en perspective (double, château)
  const d = boot(); toMenu(d); setOpts(d, { MODE: MODE.P2, TYPE: 1, ENV: 5, FMT: 3 }); d.poke('opt_val', 1, 24); d.tap('START'); d.run(30); d.tap('A');
  runUntil(d, () => app(d) === AS.MATCH, 300); allCpu(d);
  const done = runUntil(d, () => app(d) === AS.SCREENS, 60 * 60 * 25); d.run(30);
  check(done && d.peek('persp') === 0, 'match complet en perspective (double, château), projection coupée ensuite');
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
  const PROP = parseInt((await import('fs')).readFileSync(new URL('../src/assets_swap.c', import.meta.url), 'utf8').match(/prop_tile13 = 0x([0-9A-F]+)/)[1], 16);
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
    let wave = false, toss = '', coin = false;
    for (let f = 0; f < 500 && e.peek('cer'); f++) {
      e.run(1);
      if (e.peek('pose_force', 0) === 5) wave = true;
      if (e.peek('cer') === 3 && (oamHas(e, e.peek('ball_base') + 8) || oamHas(e, e.peek('ball_base') + 12))) coin = true;
      if (/^(PILE|FACE) !$/.test(msg(e))) toss = msg(e);
    }
    check(e.peek('cer') === 0 && start[0] === 216 && start[1] >= 230 && wave && coin && toss !== '' && JSON.stringify([pos(e, 0), pos(e, 1)]) === JSON.stringify(want) && e.peek('ms') === 0,
      `${v} : entrée depuis le tunnel ${JSON.stringify(start)}, salut, pièce de l'arbitre, « ${toss} », puis chacun à sa place ${JSON.stringify(want)}`);
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
    runUntil(e, () => e.peek('cer') === 4, 60 * 120);
    const before = [pos(e, 0)[1] < 132, pos(e, 1)[1] < 132];
    let seated = false, chair = false, bottle = false, towel = false;
    for (let f = 0; f < 600 && e.peek('cer'); f++) {
      e.run(1);
      if (e.peek('cer') === 5) {
        seated = seated || (e.peek('pose_force', 0) === 31 && e.peek('pose_force', 1) === 31 && pos(e, 0)[0] === 46);
        chair = chair || oamHas(e, prop(e)); bottle = bottle || oamHas(e, prop(e) + 1); towel = towel || oamHas(e, prop(e) + 2);
      }
    }
    const after = [pos(e, 0)[1] < 132, pos(e, 1)[1] < 132];
    check(seated && chair && bottle && towel, 'changement de côté : assis sur leur chaise, près de l\'arbitre (chaise, gourde, serviette)');
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
        if ((e.peek('pose_force', w) & 31) === 30) { supine = true; racket = racket || oamHas(e, prop(e) + 3); }
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
  e.tap('A'); runUntil(e, () => app(e) === 4, 180);
  check(app(e) === 4, `${name} : puis le générique de fin`);
}
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
  runUntil(e, () => app(e) === AS.SCREENS, 60 * 60 * 25); e.run(60);
  const base = 0x1000 + e.peek('swap_base') * 16, rom2 = e.nes.rom.rom[2];
  let d = 0; for (let i = 0; i < 34 * 16; i++) if (e.nes.ppu.vramMem[base + i] !== rom2[base + i]) d++;
  check(d === 0, `écrans : tuiles des célébrations rétablies (${d} octet(s) différent(s))`);
}
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
console.log('Compensation 50 Hz (PAL)');
{
  const e = boot(); e.poke('is_pal', 1); startWith(e, { FMT: 3 }); allCpu(e);
  check(runUntil(e, () => app(e) === AS.SCREENS, 60 * 60 * 25), 'match complet en mode 50 Hz');
}

console.log(`\n${passes} vérifications réussies, ${fails} échec(s) ; pile contrôlée ${spChecks} fois`);
process.exit(fails ? 1 : 0);
