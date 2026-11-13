// Petit banc d'essai autour de jsnes : chargement de ROM, manettes,
// lecture de la RAM par symbole (fichier .lbl de ld65) et captures PNG.
import fs from 'fs';
import path from 'path';
import * as jsnes from 'jsnes';
import pngjs from 'pngjs';
const { PNG } = pngjs;

const BTN = {
  A: jsnes.Controller.BUTTON_A, B: jsnes.Controller.BUTTON_B,
  SELECT: jsnes.Controller.BUTTON_SELECT, START: jsnes.Controller.BUTTON_START,
  UP: jsnes.Controller.BUTTON_UP, DOWN: jsnes.Controller.BUTTON_DOWN,
  LEFT: jsnes.Controller.BUTTON_LEFT, RIGHT: jsnes.Controller.BUTTON_RIGHT,
};

class Emu {
  constructor(romPath) {
    this.frameBuf = null;
    this.nes = new jsnes.NES({
      onFrame: (b) => { this.frameBuf = b; },
      onAudioSample: () => {},
    });
    this.nes.loadROM(fs.readFileSync(romPath).toString('binary'));
    this.labels = {};
    const lbl = romPath.replace(/\.nes$/, '.lbl');
    for (const line of fs.readFileSync(lbl, 'utf8').split('\n')) {
      const m = line.match(/^al ([0-9A-F]+) \.(\S+)/);
      if (m) this.labels[m[2]] = parseInt(m[1], 16);
    }
    this.frames = 0;
    this.held = [new Set(), new Set()];
    // séquences animées du match (entrée, changement de côté, fin) : passées
    // à chaque trame, sauf pour les tests qui les regardent (cerSkip = false)
    this.cerSkip = true;
    this.cerAddr = this.labels['_cer_skip'];
  }
  addr(sym) {
    const a = this.labels['_' + sym] ?? this.labels[sym];
    if (a === undefined) throw new Error('symbole inconnu : ' + sym);
    return a;
  }
  peek(sym, off = 0) { return this.nes.cpu.mem[this.addr(sym) + off]; }
  peek16(sym, off = 0) { return this.peek(sym, off) | (this.peek(sym, off + 1) << 8); }
  peekS16(sym, off = 0) { const v = this.peek16(sym, off); return v >= 0x8000 ? v - 0x10000 : v; }
  poke(sym, v, off = 0) { this.nes.cpu.mem[this.addr(sym) + off] = v & 0xFF; }
  down(btn, pad = 1) { this.nes.buttonDown(pad, BTN[btn]); this.held[pad - 1].add(btn); }
  up(btn, pad = 1) { this.nes.buttonUp(pad, BTN[btn]); this.held[pad - 1].delete(btn); }
  releaseAll() { for (const p of [1, 2]) for (const b of [...this.held[p - 1]]) this.up(b, p); }
  run(n = 1, each) {
    for (let i = 0; i < n; i++) {
      this.nes.frame(); this.frames++;
      if (this.cerAddr !== undefined) this.nes.cpu.mem[this.cerAddr] = this.cerSkip ? 1 : 0;
      if (each) each(this);
    }
  }
  tap(btn, pad = 1, hold = 3, after = 6) { this.down(btn, pad); this.run(hold); this.up(btn, pad); this.run(after); }
  // texte affiché à l'écran (table des noms $2000)
  text(x, y, len) {
    let t = '';
    // police surlignée des menus ($60-$9F) : lue comme le texte normal
    for (let i = 0; i < len; i++) { const c = this.nes.ppu.vramMem[0x2000 + y * 32 + x + i]; t += String.fromCharCode(c >= 0x61 && c < 0xA0 ? c - 0x40 : c); }
    return t;
  }
  screenshot(file) {
    const png = new PNG({ width: 256, height: 240 });
    for (let i = 0; i < 256 * 240; i++) {
      const c = this.frameBuf[i];
      png.data[i * 4] = c & 0xFF; png.data[i * 4 + 1] = (c >> 8) & 0xFF;
      png.data[i * 4 + 2] = (c >> 16) & 0xFF; png.data[i * 4 + 3] = 255;
    }
    fs.mkdirSync(path.dirname(file), { recursive: true });
    fs.writeFileSync(file, PNG.sync.write(png));
  }
}
export { Emu, BTN };
