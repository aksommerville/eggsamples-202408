import { Font } from "./Font";

/**
 * Trivial pick-from-list menu.
 * You must supply either a 256-tile sheet or one of our Font objects.
 * If using a Font, you must cleanup() when done.
 */
export class SingleSelectionMenu {
  private texid: number;
  private font: Font | null;
  private prompt: string;
  private options: [number, string][];
  private optionp: number;
  private screenw: number;
  private screenh: number;
  
  private glyphw: number;
  private rowh: number;
  private tiles: ArrayBuffer;
  private tileCount: number;
  private hilitey0: number;
  
  private labels: { dstx: number; dsty: number; w: number; h: number; texid: number; }[] = [];

  constructor(
    texidOrFont: number | Font,
    prompt: string,
    options: [number, string][], // [key, value]
    initial: number // key (not index!)
  ) {
    this.prompt = prompt;
    this.options = options;
    this.optionp = this.options.findIndex(o => o[0] === initial);
    
    if (texidOrFont instanceof Font) {
      this.texid = 0;
      this.font = texidOrFont;
    } else {
      this.texid = texidOrFont;
      this.font = null;
    }
    
    const fb = egg.texture_get_header(1);
    this.screenw = fb.w;
    this.screenh = fb.h;
    
    if (this.texid) {
      const font = egg.texture_get_header(this.texid);
      this.glyphw = font.w >> 4;
      this.rowh = this.glyphw + 2;
      const tileLimit = this.prompt.length + this.options.reduce((a, v) => a + v[1].length, 0);
      const tiles = new Uint8Array(tileLimit * 6);
      const totalh = this.rowh * (this.options.length + 2); // +2 = Prompt, and a blank line between.
      this.hilitey0 = (this.screenh >> 1) - (totalh >> 1) + this.rowh * 2 - 1;
      let y = (this.screenh >> 1) - (totalh >> 1) + (this.glyphw >> 1);
      let tilep = this.addTile(tiles, 0, this.prompt, y);
      y += this.rowh * 2;
      for (const [lang, name] of this.options) {
        tilep = this.addTile(tiles, tilep, name, y);
        y += this.rowh;
      }
      this.tileCount = tilep / 6;
      this.tiles = tiles.buffer;
      
    } else if (this.font) {
      this.rowh = this.font.getRowHeight();
      this.addLabel(this.prompt);
      for (const [lang, name] of this.options) {
        this.addLabel(name);
      }
      this.centerLabels();
      
    } else throw new Error(`Must supply texid or font`);
  }
  
  cleanup(): void {
    for (const label of this.labels) egg.texture_del(label.texid);
  }
  
  private addTile(dst: Uint8Array, p: number, src: string, y: number): number {
    let x = (this.screenw >> 1) - ((src.length * this.glyphw) >> 1) + (this.glyphw >> 1);
    for (let i=0; i<src.length; i++, x+=this.glyphw) {
      let codepoint = src.charCodeAt(i);
      if (codepoint <= 0x20) continue;
      if (codepoint > 0xff) codepoint = 0x3f; // '?', TODO Really need a Unicode-friendly font.
      dst[p++] = x;
      dst[p++] = x >> 8;
      dst[p++] = y;
      dst[p++] = y >> 8;
      dst[p++] = codepoint;
      dst[p++] = 0;
    }
    return p;
  }
  
  private addLabel(src: string): void {
    const texid = this.font.renderTexture(src);
    if (!texid) return;
    const hdr = egg.texture_get_header(texid);
    let dsty: number;
    if (this.labels.length) dsty = this.rowh * (this.labels.length + 1);
    else dsty = 0;
    this.labels.push({ dstx: 0, dsty, w: hdr.w, h: hdr.h, texid });
  }
  
  private centerLabels(): void {
    if (this.labels.length < 1) return;
    const totalh = this.labels[this.labels.length - 1].dsty + this.labels[this.labels.length - 1].h;
    let y = (this.screenh >> 1) - (totalh >> 1);
    let extra = this.rowh;
    for (const label of this.labels) {
      label.dsty = y;
      y += this.rowh + extra;
      extra = 0;
      label.dstx = (this.screenw >> 1) - (label.w >> 1);
    }
    if (this.labels.length >= 2) {
      this.hilitey0 = this.labels[1].dsty - 1;
    }
  }
  
  getValue(): number {
    if (this.optionp < 0) return 0;
    if (this.optionp >= this.options.length) return 0;
    return this.options[this.optionp][0];
  }
  
  render(): void {
    egg.draw_rect(1, 0, 0, this.screenw, this.screenh, 0x000000c0);
    if ((this.optionp >= 0) && (this.optionp < this.options.length)) {
      egg.draw_rect(1, 0, this.hilitey0 + this.optionp * this.rowh, this.screenw, this.rowh, 0x306090ff);
    }
    if (this.tiles) {
      egg.draw_mode(egg.XferMode.ALPHA, 0xffffffff, 0xff);
      egg.draw_tile(1, this.texid, this.tiles, this.tileCount);
      egg.draw_mode(egg.XferMode.ALPHA, 0, 0xff);
    } else {
      for (const label of this.labels) {
        egg.draw_decal(1, label.texid, label.dstx, label.dsty, 0, 0, label.w, label.h, 0);
      }
    }
  }
  
  userInput(dx: number, dy: number): void {
    if (dx) {
      // Not supporting it yet, but if we want horizontal controls (eg change value of a field), hook in here.
    }
    if (dy && (this.options.length > 0)) {
      this.optionp += dy;
      if (this.optionp < 0) this.optionp = this.options.length - 1;
      else if (this.optionp >= this.options.length) this.optionp = 0;
    }
  }
}
