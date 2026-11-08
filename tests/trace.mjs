import { Emu } from './emu.mjs';
const e = new Emu(process.argv[2]);
const N = +(process.argv[3] ?? 3000);
e.run(30); e.tap('START'); e.run(10); e.tap('START'); e.run(5);
const cfg = e.addr('cfg'); e.nes.cpu.mem[cfg + 1] = 2;
const s16 = (n, i=0) => e.peekS16(n, i*2) / 64;
let last = '';
for (let i = 0; i < N; i++) {
  e.run(1);
  const st = `ms=${e.peek('ms')} hit=${e.peek('hitter')} b=${e.peek('bounces')} live=${e.peek('b_live')} rh=${e.peek('rally_hits')} pts=${e.peek('st_points',0)}-${e.peek('st_points',1)}`;
  if (st !== last) {
    console.log(e.frames, st, `ball=(${s16('bx').toFixed(0)},${s16('by').toFixed(0)},${s16('bz').toFixed(0)}) p0=(${s16('plx',0).toFixed(0)},${s16('ply',0).toFixed(0)}) p1=(${s16('plx',1).toFixed(0)},${s16('ply',1).toFixed(0)})`);
    last = st;
  }
  if (process.argv[4] && i % 20 === 0 && i < 400) e.screenshot(`/tmp/claude-0/-home-user-Tennis/47547f90-df0e-5d9d-aa0b-91079a38b536/scratchpad/f${String(i).padStart(4,'0')}.png`);
}
