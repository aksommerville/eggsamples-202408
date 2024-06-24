/* TilesheetEditor.js
 * Top-level resource editor for tilesheets.
 * Compared to maps, this should be a breeze.
 */
 
import { Dom } from "./Dom.js";
import { Resmgr } from "./Resmgr.js";
import { Bus } from "./Bus.js";
import { MapCanvasUi } from "./MapCanvasUi.js"; // We borrow its static color table.
import { TileModal } from "./TileModal.js";

export const DEFAULT_TILESHEET_TABLES = [
  "physics",
  "family",
  "neighbors",
  "weight",
];

const RENDER_DEBOUNCE_TIME = 50; // ms

export class TilesheetEditor {
  static getDependencies() {
    return [HTMLElement, Dom, Resmgr, Bus, "nonce", Window];
  }
  constructor(element, dom ,resmgr, bus, nonce, window) {
    this.element = element;
    this.dom = dom;
    this.resmgr = resmgr;
    this.bus = bus;
    this.nonce = nonce;
    this.window = window;
    
    this.path = "";
    this.serial = null;
    this.rid = 0;
    this.image = null;
    this.tables = {}; // key:string, value:Uint8Array(256)
    this.visibleTables = []; // string
    this.renderTimeout = null;
    this.selp = -1; // control-click to set a selection and shift-click to copy it (visible tables only)
    this.pvmp = -1; // Previous mouse position.
    
    this.resizeObserver = new this.window.ResizeObserver(e => this.renderSoon());
    this.resizeObserver.observe(this.element);
    this.mouseListener = null; // Installed on window if not null.
    this.element.addEventListener("mousedown", e => this.onMouseDown(e));
    
    this.buildUi();
  }
  
  onRemoveFromDom() {
    if (this.renderTimeout) {
      this.window.clearTimeout(this.renderTimeout);
      this.renderTimeout = null;
    }
    if (this.resizeObserver) {
      this.resizeObserver.disconnect();
      this.resizeObserver = null;
    }
    if (this.mouseListener) {
      this.window.removeEventListener("mousemove", this.mouseListener);
      this.window.removeEventListener("mouseup", this.mouseListener);
      this.mouseListener = null;
    }
  }
  
  setup(serial, path) {
    this.serial = serial;
    this.path = path;
    const id = this.resmgr.parsePath(this.path);
    this.rid = id.rid;
    this.image = this.resmgr.resById("image", this.rid)?.image;
    this.tables = TilesheetEditor.decode(this.serial);
    this.reset();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "DIV", ["toolbar"], { "on-input": () => this.onToolbarInput() });
    this.dom.spawn(this.element, "CANVAS");
  }
  
  reset() {
    const toolbar = this.element.querySelector(".toolbar");
    toolbar.innerHTML = "";
    this.visibleTables = [];
    // Toolbar has a visibility toggle for each of the default tables, whether they exist or not, in a predetermined order.
    for (const name of DEFAULT_TILESHEET_TABLES) {
      this.spawnVisibilityToggle(toolbar, name);
      this.visibleTables.push(name);
    }
    // After that, include anything non-default in the tilesheet.
    for (const name of Object.keys(this.tables)) {
      if (DEFAULT_TILESHEET_TABLES.includes(name)) continue;
      this.spawnVisibilityToggle(toolbar, name);
      this.visibleTables.push(name);
    }
    this.dom.spawn(toolbar, "DIV", ["help"], "Ctl-click to select, shift-click to copy, visible tables only.");
    this.renderSoon();
  }
  
  spawnVisibilityToggle(toolbar, name) {
    const id = `TilesheetEditor-${this.nonce}-${name}-visibility`;
    const checkbox = this.dom.spawn(toolbar, "INPUT", ["toggle", "visibility"], { type: "checkbox", name, id, checked: "true" });
    const label = this.dom.spawn(toolbar, "LABEL", { for: id }, this.dom.spawn(null, "SPAN", name));
  }
  
  onToolbarInput() {
    this.visibleTables = Array.from(this.element.querySelectorAll(".toolbar .visibility:checked")).map(e => e.name);
    this.renderSoon();
  }
  
  renderSoon() {
    if (this.renderTimeout) return;
    this.renderTimeout = this.window.setTimeout(() => {
      this.renderTimeout = null;
      this.renderNow();
    }, RENDER_DEBOUNCE_TIME);
  }
  
  renderNow() {
    const canvas = this.element.querySelector("canvas");
    const bounds = canvas.getBoundingClientRect();
    canvas.width = bounds.width;
    canvas.height = bounds.height;
    const ctx = canvas.getContext("2d");
    ctx.imageSmoothingEnabled = false;
    ctx.clearRect(0, 0, bounds.width, bounds.height);
    const dstw = Math.min(bounds.width, bounds.height);
    const dsth = dstw;
    const dstx = (bounds.width >> 1) - (dstw >> 1);
    const dsty = (bounds.height >> 1) - (dsth >> 1);
    if (this.image) {
      ctx.drawImage(this.image, 0, 0, this.image.naturalWidth, this.image.naturalHeight, dstx, dsty, dstw, dsth);
    } else {
      ctx.fillStyle = "#fff";
      ctx.fillRect(dstx, dsty, dstw, dsth);
    }
    for (const name of this.visibleTables) {
      switch (name) {
        case "physics": this.drawPhysics(ctx, dstx, dsty, dstw, dsth); break;
        case "family": this.drawFamily(ctx, dstx, dsty, dstw, dsth); break;
        case "neighbors": this.drawNeighbors(ctx, dstx, dsty, dstw, dsth); break;
        case "weight": this.drawWeight(ctx, dstx, dsty, dstw, dsth); break;
      }
    }
    this.drawGridLines(ctx, dstx, dsty, dstw, dsth);
    if (this.selp >= 0) {
      const x = dstx + (dstw * (this.selp & 15)) / 16;
      const y = dsty + (dsth * (this.selp >> 4)) / 16;
      const w = dstx + (dstw * ((this.selp & 15) + 1)) / 16 - x;
      const h = dsty + (dsth * ((this.selp >> 4) + 1)) / 16 - y;
      ctx.globalAlpha = 0.5;
      ctx.fillStyle = "#0ff";
      ctx.fillRect(x, y, w, h);
      ctx.globalAlpha = 1;
    }
  }
  
  drawGridLines(ctx, dstx, dsty, dstw, dsth) {
    ctx.beginPath();
    for (let col=0; col<=16; col++) { // sic <= not < -- draw a line on each edge
      const x = dstx + (dstw * col) / 16;
      ctx.moveTo(x, dsty);
      ctx.lineTo(x, dsty + dsth);
    }
    for (let row=0; row<=16; row++) {
      const y = dsty + (dsth * row) / 16;
      ctx.moveTo(dstx, y);
      ctx.lineTo(dstx + dstw, y);
    }
    ctx.strokeStyle = "#0f0";
    ctx.stroke();
  }
  
  // physics: Colored box in the upper-left corner of the tile.
  drawPhysics(ctx, dstx, dsty, dstw, dsth) {
    const src = this.tables.physics;
    if (!src) return;
    const boxw = Math.floor(dstw / (16 * 3));
    const boxh = Math.floor(dsth / (16 * 4));
    for (let srcp=0, row=0; row<16; row++) {
      const y = dsty + (dsth * row) / 16;
      for (let col=0; col<16; col++, srcp++) {
        const v = src[srcp];
        if (!v) continue;
        const x = dstx + (dstw * col) / 16;
        ctx.fillStyle = MapCanvasUi.ctab[v];
        ctx.fillRect(x, y, boxw, boxh);
      }
    }
  }
  
  // family: Colored box in the upper middle of the tile, exactly the same as physics but shifted.
  drawFamily(ctx, dstx, dsty, dstw, dsth) {
    const src = this.tables.family;
    if (!src) return;
    const boxw = Math.floor(dstw / (16 * 3));
    const boxh = Math.floor(dsth / (16 * 4));
    for (let srcp=0, row=0; row<16; row++) {
      const y = dsty + (dsth * row) / 16;
      for (let col=0; col<16; col++, srcp++) {
        const v = src[srcp];
        if (!v) continue;
        const x = dstx + (dstw * col) / 16 + boxw;
        ctx.fillStyle = MapCanvasUi.ctab[v];
        ctx.fillRect(x, y, boxw, boxh);
      }
    }
  }
  
  // weight: Colored box in the upper-right corner of the tile. 0 and 255 are special; 1..254 are a gradient.
  drawWeight(ctx, dstx, dsty, dstw, dsth) {
    const src = this.tables.weight;
    if (!src) return;
    const boxw = Math.floor(dstw / (16 * 3));
    const boxh = Math.floor(dsth / (16 * 4));
    for (let srcp=0, row=0; row<16; row++) {
      const y = dsty + (dsth * row) / 16;
      for (let col=0; col<16; col++, srcp++) {
        const v = src[srcp] || 0;
        const x = dstx + (dstw * col) / 16 + (boxw << 1);
             if (v === 0x00) ctx.fillStyle = "#00f";
        else if (v === 0xff) ctx.fillStyle = "#f80";
        else {
          const luma = Math.floor((v * 100) / 256);
          ctx.fillStyle = `hsl(0 0 ${luma})`;
        }
        ctx.fillRect(x, y, boxw, boxh);
      }
    }
  }
  
  // neighbors: Icon occupying the SE quarter of the tile, with 0..8 little spokes for the neighbors.
  drawNeighbors(ctx, dstx, dsty, dstw, dsth) {
    const src = this.tables.neighbors;
    if (!src) return;
    const boxw = Math.floor(dstw / (16 * 2));
    const boxh = Math.floor(dsth / (16 * 2));
    for (let srcp=0, row=0; row<16; row++) {
      const y = dsty + (dsth * row) / 16 + boxh;
      for (let col=0; col<16; col++, srcp++) {
        const v = src[srcp] || 0;
        const x = dstx + (dstw * col) / 16 + boxw;
        ctx.globalAlpha = 0.5;
        ctx.fillStyle = "#fff";
        ctx.fillRect(x, y, boxw, boxh);
        ctx.globalAlpha = 1;
        if (v) {
          const midx = x + (boxw >> 1);
          const midy = y + (boxh >> 1);
          ctx.beginPath();
          for (let dy=-1, mask=0x80; dy<=1; dy++) {
            for (let dx=-1; dx<=1; dx++) {
              if (!dx && !dy) continue; // Important that we not shift mask at center.
              if (v & mask) {
                ctx.moveTo(midx, midy);
                ctx.lineTo(midx + dx * (boxw >> 1), midy + dy * (boxh >> 1));
              }
              mask >>= 1;
            }
          }
          ctx.strokeStyle = "#000";
          ctx.stroke();
        }
      }
    }
  }
  
  tileidFromCoords(x, y) {
    const canvas = this.element.querySelector("canvas");
    const bounds = canvas.getBoundingClientRect();
    const dstw = Math.min(bounds.width, bounds.height);
    const dsth = dstw;
    const dstx = bounds.x + (bounds.width >> 1) - (dstw >> 1);
    const dsty = bounds.y + (bounds.height >> 1) - (dsth >> 1);
    x -= dstx;
    y -= dsty;
    const row = Math.floor((y * 16) / dsth);
    const col = Math.floor((x * 16) / dstw);
    if ((col < 0) || (col >= 16) || (row < 0) || (row >= 16)) return -1;
    return (row << 4) | col;
  }
  
  onMouseUpOrMove(event) {
    if (event.type === "mouseup") {
      this.window.removeEventListener("mousemove", this.mouseListener);
      this.window.removeEventListener("mouseup", this.mouseListener);
      this.mouseListener = null;
      return;
    }
    const p = this.tileidFromCoords(event.x, event.y);
    if (p < 0) return;
    if (p === this.pvmp) return;
    this.pvmp = p;
    if (p === this.selp) return;
    let changed = false;
    for (const name of this.visibleTables) {
      const table = this.tables[name];
      if (table[p] === table[this.selp]) continue;
      table[p] = table[this.selp];
      changed = true;
    }
    if (changed) {
      this.resmgr.dirty(this.path, () => this.encode(this.tables));
      this.renderSoon();
    }
  }
  
  onMouseDown(event) {
    if (this.mouseListener) return; // Already tracking a mouse gesture, get out.
    const p = this.tileidFromCoords(event.clientX, event.clientY);
    if (p < 0) return;
    
    // Control to move selection.
    if (event.ctrlKey) {
      if (p === this.selp) this.selp = -1;
      else this.selp = p;
      this.renderSoon();
      return;
    }
    
    // Shift-drag to copy selection.
    if (event.shiftKey) {
      this.pvmp = -1;
      this.mouseListener = e => this.onMouseUpOrMove(e);
      this.window.addEventListener("mousemove", this.mouseListener);
      this.window.addEventListener("mouseup", this.mouseListener);
      this.onMouseUpOrMove(event);
      return;
    }
    
    // Any other click opens the modal. Modal takes care of saving etc.
    const modal = this.dom.spawnModal(TileModal);
    modal.encode = () => this.encode(this.tables);
    modal.setup(this.path, this.tables, this.image, p);
    modal.result.catch(() => {}).then(() => this.renderSoon());
  }
  
  static decode(src) {
    const dst = {};
    let table = null; // null or Uint8Array(256)
    let tablep = 0;
    src = new TextDecoder("utf8").decode(src);
    for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.substring(srcp, nlp).trim();
      srcp = nlp + 1;
      
      if (table) {
        if (line.length !== 32) throw new Error(`${lineno}: Invalid line length ${line.length}, expected 32`);
        for (let i=16, linep=0; i-->0; tablep++) {
          const hi = parseInt(line[linep++], 16);
          const lo = parseInt(line[linep++], 16);
          if (isNaN(hi) || isNaN(lo)) throw new Error(`${lineno}: Invalid hex byte '${line[linep-2]}${line[linep-1]}'`);
          table[tablep] = (hi << 4) | lo;
        }
        if (tablep >= 0x100) {
          table = null;
          tablep = 0;
        }
      } else {
        if (!line.length) continue; // Blank lines permitted between tables.
        if (!line.match(/^[0-9a-zA-Z_]{1,31}$/)) {
          throw new Error(`${lineno}: Invalid table name. Expected a C identifier under 32 bytes.`);
        }
        table = new Uint8Array(256);
        dst[line] = table;
        tablep = 0;
      }
    }
    if (tablep) throw new Error(`Unexpected EOF during table`);
    return dst;
  }
  
  encode(tables) {
    // It's easy to know the total length in advance: Key lengths + 512 hex digits per table + 18 newlines per table.
    const names = Object.keys(tables);
    const dsta = names.reduce((a, v) => a + v.length + 18 + 512, 0);
    const dst = new Uint8Array(dsta);
    let dstp = 0;
    for (const name of names) {
      for (let i=0; i<name.length; i++) dst[dstp++] = name.charCodeAt(i);
      dst[dstp++] = 0x0a;
      const table = tables[name];
      for (let yi=16, tablep=0; yi-->0; ) {
        for (let xi=16; xi-->0; tablep++) {
          dst[dstp++] = "0123456789abcdef".charCodeAt(table[tablep] >> 4);
          dst[dstp++] = "0123456789abcdef".charCodeAt(table[tablep] & 15);
        }
        dst[dstp++] = 0x0a;
      }
      dst[dstp++] = 0x0a;
    }
    if (dstp !== dsta) throw new Error(`Expected to produce ${dsta} bytes for tilesheet, actual was ${dstp}`);
    return dst;
  }
}
