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
    // bandeau du score (2.74) : il attend l'appui d'un joueur humain et
    // l'ordinateur ne sert pas avant ; il est fermé d'office à chaque trame,
    // sauf pour les tests qui regardent ce comportement (bandSkip = false)
    this.bandSkip = true;
    // (2.75) écran du joueur personnalisé avant une compétition : passé
    // d'office avec START (customSkip = false pour le tester)
    this.customSkip = true; this._cuPress = false;
    // (2.76) pages de choix illustrées (joueurs, tournoi, lieu, surface) :
    // validées d'office avec START (pickSkip = false pour les tester)
    this.pickSkip = true;
    this.tossSkip = true; this._tsPress = false;
    this.scrAddr = this.labels['_scr_state']; this.appAddr = this.labels['_app_state'];
    this.bandDis = this.labels['_band_dis']; this.bandHold = this.labels['_band_hold'];
    // dessin de l'image (fonctions d'origine de jsnes, gardées pour setRender)
    const ppu = this.nes.ppu;
    this._rbg = ppu.renderBgScanline; this._rfp = ppu.renderFramePartially;
    this.rendering = true;
  }
  // Dessin de l'image : coupé, l'émulation va ~1,5 fois plus vite (le décor
  // n'est plus dessiné pixel par pixel). Le jeu n'en dépend pas : il ne lit
  // jamais le « sprite 0 » (seul le bit de fin d'image de PPUSTATUS) ; la RAM
  // reste identique à l'octet près (vérifié sur 4000 trames de match).
  // frameBuf et screenshot() demandent le dessin : setRender(true) d'abord.
  setRender(on) {
    const ppu = this.nes.ppu;
    ppu.renderBgScanline = on ? this._rbg : function () {};
    ppu.renderFramePartially = on ? this._rfp : function () {};
    this.rendering = on;
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
      if (this.bandSkip && this.bandHold !== undefined && this.human()) {
        const m = this.nes.cpu.mem;
        if (m[this.bandHold] === 1) { m[this.bandDis] = 1; m[this.bandHold] = 0; }
        if (m[this.labels['_cer']] === 5) m[this.bandHold] = 2;   /* (2.75 : assis, statistiques passées d'office) */
      }
      if ((this.customSkip || this.pickSkip) && this.scrAddr !== undefined) {
        const m = this.nes.cpu.mem;
        if (this._cuPress) { this.nes.buttonUp(1, BTN.START); this._cuPress = false; }
        else if (((this.customSkip && m[this.scrAddr] === 13) || (this.pickSkip && m[this.scrAddr] === 15)) && m[this.appAddr] === 0 && !this.held[0].has('START')) { this.nes.buttonDown(1, BTN.START); this._cuPress = true; }
        if (this.labels['_pk_off'] !== undefined) m[this.labels['_pk_off']] = this.pickSkip ? 1 : 0;   /* (2.76 : pages de choix sautées, comme avant) */
      }
      // (2.76) tirage au sort : le gagnant humain choisit de servir (A, sur
      // ses deux manettes possibles), sauf pour les tests qui le regardent
      if (this.tossSkip && this.cerAddr !== undefined && this.appAddr !== undefined) {
        const m = this.nes.cpu.mem;
        if (this._tsPress) { this.nes.buttonUp(1, BTN.A); this.nes.buttonUp(2, BTN.A); this._tsPress = false; }
        else if (m[this.appAddr] === 1 && (m[this.labels['_cer']] === 11 || m[this.labels['_cer']] === 13) && !this.held[0].has('A')) { this.nes.buttonDown(1, BTN.A); this.nes.buttonDown(2, BTN.A); this._tsPress = true; }
      }
      if (each) each(this);
    }
  }
  // un joueur à la manette (seul cas où le bandeau attend indéfiniment)
  human() { const c = this.labels['_cfg'], n = this.nes.cpu.mem[this.labels['_npl']]; for (let i = 0; i < n; i++) if (this.nes.cpu.mem[c + 2 + i] !== 2) return true; return false; }
  tap(btn, pad = 1, hold = 3, after = 6) { this.down(btn, pad); this.run(hold); this.up(btn, pad); this.run(after); }
  // texte affiché à l'écran (table des noms $2000)
  text(x, y, len) {
    let t = '';
    // police surlignée des menus ($60-$9F) : lue comme le texte normal
    for (let i = 0; i < len; i++) { const c = this.nes.ppu.vramMem[0x2000 + y * 32 + x + i]; t += String.fromCharCode(c >= 0x61 && c < 0xA0 ? c - 0x40 : c); }
    return t;
  }
  screenshot(file) {
    if (!this.rendering) throw new Error('screenshot : dessin coupé (setRender(true) avant de jouer les trames)');
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
