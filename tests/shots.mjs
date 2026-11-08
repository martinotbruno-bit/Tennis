// Captures d'écran de référence (titre, menu, simple, double, pluie, nuit)
import { Emu } from './emu.mjs';
const rom = process.argv[2], out = process.argv[3] ?? 'shots';
function match(opts, frames, name) {
  const e = new Emu(rom);
  e.run(30); e.tap('START'); e.run(10);
  for (const [i, v] of Object.entries(opts)) e.poke('opt_val', v, +i);
  e.tap('START'); e.run(5);
  const cfg = e.addr('cfg'); e.nes.cpu.mem[cfg + 2] = 2;
  e.run(frames); e.screenshot(`${out}/${name}.png`);
  return e;
}
const e = new Emu(rom); e.run(60); e.screenshot(`${out}/titre.png`);
e.tap('START'); e.run(20); e.screenshot(`${out}/menu.png`);
match({}, 420, 'simple_terre');
match({ 2: 1, 6: 2 }, 520, 'double_dur');
match({ 6: 1, 7: 2 }, 450, 'pluie_gazon');
match({ 6: 3, 7: 1 }, 450, 'nuit_moquette');
{
  const r = new Emu(rom); r.run(30); r.tap('START'); r.run(10);
  r.poke('opt_val', 3, 8); r.tap('START'); r.run(5);
  const c = r.addr('cfg'); for (let i = 0; i < 4; i++) r.nes.cpu.mem[c + 2 + i] = 2;
  let n = 0; while (r.peek('app_state') !== 3 && n++ < 60 * 60 * 30) r.run(1);
  r.run(20); r.screenshot(`${out}/resultats.png`);
  const t = new Emu(rom); t.run(30); t.tap('START'); t.run(10);
  t.poke('opt_val', 3, 1); t.tap('START'); t.run(20); t.screenshot(`${out}/tournoi.png`);
}
