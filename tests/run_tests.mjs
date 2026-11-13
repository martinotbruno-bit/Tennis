// Royal Martinot — lanceur de la suite de tests sur émulateur.
//
//   node run_tests.mjs [rom] [--jobs=N] [--only=mot1,mot2]
//
// Sans --only : toutes les sections, réparties sur N processus en parallèle
// (par défaut un par cœur). La répartition équilibre les durées mesurées au
// passage précédent (section_times.json, mis à jour après chaque passage
// complet). Avec --only : seulement les sections dont le nom contient l'un des
// mots (contrôle rapide d'une modification localisée).
// La sortie est la même que celle de emu_test.mjs, dans l'ordre des sections.
import { spawn, spawnSync } from 'child_process';
import fs from 'fs';
import os from 'os';
import path from 'path';
import { fileURLToPath } from 'url';

const DIR = path.dirname(fileURLToPath(import.meta.url));
const TEST = path.join(DIR, 'emu_test.mjs');
const TIMES = path.join(DIR, 'section_times.json');
const args = process.argv.slice(2);
const opt = (k) => { const a = args.find(x => x.startsWith('--' + k + '=')); return a ? a.slice(k.length + 3) : null; };
const rom = path.resolve(args.find(a => !a.startsWith('--')) ?? path.join(DIR, '../build/royal_martinot.nes'));
const only = opt('only');
const jobs = Math.max(1, Number(opt('jobs') ?? os.cpus().length));

// liste des sections
const list = spawnSync(process.execPath, [TEST, rom, '--list'], { encoding: 'utf8' }).stdout
  .split('\n').filter(Boolean).map(l => { const [i, ...n] = l.split('\t'); return { i: Number(i), name: n.join('\t') }; });
let todo = list;
if (only) {
  const words = only.toLowerCase().split(',').map(t => t.trim()).filter(Boolean);
  todo = list.filter(s => words.some(w => s.name.toLowerCase().includes(w)));
  if (!todo.length) { console.log(`aucune section ne correspond à « ${only} »`); process.exit(2); }
}

// répartition : la plus longue d'abord, dans le processus le moins chargé
let times = {};
try { times = JSON.parse(fs.readFileSync(TIMES, 'utf8')); } catch { times = {}; }
const n = Math.min(jobs, todo.length);
const shards = Array.from({ length: n }, () => ({ load: 0, idx: [] }));
for (const s of [...todo].sort((a, b) => (times[b.name] ?? 30) - (times[a.name] ?? 30))) {
  const sh = shards.reduce((m, x) => (x.load < m.load ? x : m));
  sh.load += times[s.name] ?? 30; sh.idx.push(s.i);
}

const t0 = Date.now();
const outText = {}, measured = {};
let spChecks = 0, crashed = 0;
await Promise.all(shards.map(sh => new Promise(res => {
  const p = spawn(process.execPath, [TEST, rom, '--sections=' + sh.idx.join(',')], { env: { ...process.env, TEST_MARKS: '1' } });
  let buf = '';
  p.stdout.on('data', d => { buf += d; });
  p.stderr.on('data', d => { buf += d; });
  p.on('close', code => {
    let cur = null, done = false;
    for (const line of buf.split('\n')) {
      let m;
      if ((m = line.match(/^§S (\d+)$/))) { cur = Number(m[1]); outText[cur] = []; continue; }
      if ((m = line.match(/^§T (\d+) ([\d.]+)$/))) { measured[Number(m[1])] = Number(m[2]); continue; }
      if ((m = line.match(/pile contrôlée (\d+) fois/))) { spChecks += Number(m[1]); done = true; continue; }
      if (/vérifications réussies/.test(line) || line === '') continue;
      if (cur !== null) outText[cur].push(line);
      else if (line.trim()) { console.log(line); }
    }
    // un plantage (exception) : pas de bilan final ; la sortie est affichée
    if (!done) { crashed++; console.log(`processus interrompu (code ${code}) :\n` + buf.split('\n').filter(l => !l.startsWith('\u00a7')).slice(-15).join('\n')); }
    res();
  });
})));

let passes = 0, fails = 0;
for (const s of todo) {
  const lines = outText[s.i];
  if (!lines) { console.log(s.name); console.log('  ECHEC section non exécutée (processus interrompu)'); fails++; continue; }
  for (const l of lines) {
    console.log(l);
    if (l.startsWith('  ok ')) passes++;
    else if (l.startsWith('  ECHEC')) fails++;
  }
}
// mémorise les durées après un passage complet sans plantage
if (!only && !crashed) {
  const t = {};
  for (const s of list) if (measured[s.i] !== undefined) t[s.name] = measured[s.i];
  fs.writeFileSync(TIMES, JSON.stringify(t, null, 1) + '\n');
}
const dt = (Date.now() - t0) / 1000;
console.log(`\n${passes} vérifications réussies, ${fails} échec(s) ; pile contrôlée ${spChecks} fois`
  + ` ; ${todo.length} section(s), ${n} processus, ${dt < 120 ? dt.toFixed(0) + ' s' : (dt / 60).toFixed(1) + ' min'}`);
process.exit(fails || crashed ? 1 : 0);
