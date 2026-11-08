// Captures d'écran de référence pour la documentation.
import { Emu } from './emu.mjs';
const rom = process.argv[2], out = process.argv[3] ?? 'shots';
const O = { MODE: 1, TYPE: 2, P1: 3, P2: 4, LEVEL: 5, SURF: 6, ENV: 7, COND: 8, FMT: 9, COMP: 11, EX: 15 };
function boot() { const e = new Emu(rom); e.run(30); return e; }
function menu(e, opts) { e.tap('START'); e.run(10); for (const [k, v] of Object.entries(opts)) e.poke('opt_val', v, O[k]); }
function allCpu(e) { const c = e.addr('cfg'); for (let i = 0; i < 4; i++) e.nes.cpu.mem[c + 2 + i] = 2; }
function match(opts, frames, name) {
  const e = boot(); menu(e, opts); e.tap('START'); e.run(10); e.tap('A'); e.run(5); allCpu(e);
  e.run(frames); e.screenshot(`${out}/${name}.png`); return e;
}
let e = boot(); e.run(30); e.screenshot(`${out}/titre.png`);
menu(e, {}); e.run(5); e.tap('START'); e.run(5); e.tap('B'); e.run(15); e.screenshot(`${out}/menu.png`);
e.tap('START'); e.run(30); e.screenshot(`${out}/presentation.png`);
e = boot(); e.tap('DOWN'); e.tap('A'); e.run(15); e.tap('RIGHT'); e.run(15); e.screenshot(`${out}/mode_emploi.png`);
match({ SURF: 0, ENV: 0 }, 420, 'simple_terre_stade');
match({ TYPE: 1, SURF: 2, ENV: 2 }, 520, 'double_dur_salle');
match({ SURF: 1, ENV: 1, COND: 2 }, 450, 'pluie_gazon_club');
match({ SURF: 4, ENV: 1 }, 450, 'plage_club');
match({ SURF: 5, ENV: 0, COND: 1 }, 450, 'glace_nuit');
// ralenti
e = boot(); menu(e, { FMT: 3 }); e.tap('START'); e.run(10); e.tap('A'); e.run(5); allCpu(e);
for (let i = 0; i < 60 * 60 * 10 && e.peek('ms') !== 5; i++) e.run(1);
e.run(40); e.screenshot(`${out}/ralenti.png`);
for (let i = 0; i < 60 * 60 * 20 && e.peek('app_state') === 1; i++) e.run(1);
e.run(40); e.screenshot(`${out}/resultats.png`);
// entraînement
e = boot(); menu(e, { MODE: 4, EX: 0 }); e.tap('START'); e.run(130); e.screenshot(`${out}/entrainement.png`);
// compétition
e = boot(); menu(e, { MODE: 3, COMP: 1 }); e.tap('START'); e.run(20); e.screenshot(`${out}/competition.png`);
e = boot(); menu(e, { MODE: 3, COMP: 3 }); e.tap('START'); e.run(20); e.screenshot(`${out}/saison.png`);
// palmarès
e = boot(); e.tap('DOWN'); e.tap('DOWN'); e.tap('A'); e.run(20); e.screenshot(`${out}/palmares.png`);
