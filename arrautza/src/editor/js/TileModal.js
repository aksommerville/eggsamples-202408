/* TileModal.js
 * Detailed view openable from TilesheetEditor, for one tile.
 */
 
import { Dom } from "./Dom.js";
import { Resmgr } from "./Resmgr.js";
import { DEFAULT_TILESHEET_TABLES } from "./TilesheetEditor.js";

const PRESETS = [
  {
    name: "fat5x3",
    w: 5,
    h: 3,
    neighbors: [
      0x0b,0x1f,0x16,0xfe,0xfb,
      0x6b,0xff,0xd6,0xdf,0x7f,
      0x68,0xf8,0xd0,0x00,0x00,
    ],
  },
  {
    name: "skinny4x4",
    w: 4,
    h: 4,
    neighbors: [
      0x00,0x08,0x18,0x10,
      0x02,0x0a,0x1a,0x12,
      0x42,0x4a,0x5a,0x52,
      0x40,0x48,0x58,0x50,
    ],
  },
  {
    name: "square3x3",
    w: 3,
    h: 3,
    neighbors: [
      0x0b,0x1f,0x16,
      0x6b,0xff,0xd6,
      0x68,0xf8,0xd0,
    ],
  },
];

export class TileModal {
  static getDependencies() {
    return [HTMLDialogElement, Dom, Resmgr, Window];
  }
  constructor(element, dom, resmgr, window) {
    this.element = element;
    this.dom = dom;
    this.resmgr = resmgr;
    this.window = window;
    
    this.path = "";
    this.tables = {};
    this.image = null;
    this.tileid = -1;
    this.tilesize = 16;
    this.mouseListener = null;
    this.nmaskPaintState = false; // Are we setting or clearing bits? Depends on initial mousedown.
    this.encode = () => {}; // Caller must set
    
    this.buildUi();
  }
  
  onRemoveFromDom() {
    if (this.mouseListener) {
      this.window.removeEventListener("mouseup", this.mouseListener);
      this.window.removeEventListener("mousemove", this.mouseListener);
      this.mouseListener = null;
    }
  }
  
  setup(path, tables, image, tileid) {
    this.path = path;
    this.tables = tables;
    this.image = image;
    this.tileid = tileid;
    if (this.image) {
      this.tilesize = this.image.naturalWidth >> 4;
      const nmask = this.element.querySelector(".nmask");
      nmask.width = this.tilesize * 2;
      nmask.height = this.tilesize * 2;
    }
    this.populateUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    const toprow = this.dom.spawn(this.element, "DIV", ["row"]);
    const nmask = this.dom.spawn(toprow, "CANVAS", ["nmask"], {
      width: this.tilesize * 2, 
      height: this.tilesize * 2,
      "on-mousedown": e => this.onNmaskMouseDown(e),
    });
    const nav = this.dom.spawn(toprow, "TABLE", ["nav"]);
    this.buildNav(nav);
    
    this.dom.spawn(this.element, "TABLE", ["fields"]);
    
    this.dom.spawn(this.element, "DIV", ["instructions"], "Set physics, family, and neighbors:");
    const presets = this.dom.spawn(this.element, "DIV", ["presets"]);
    for (const preset of PRESETS) {
      const btn = this.dom.spawn(presets, "BUTTON", { "on-click": () => this.applyPreset(preset) });
      this.dom.spawn(btn, "CANVAS", { width: 1, height: 1 });
      this.dom.spawn(btn, "DIV", ["name"], preset.name);
    }
  }
  
  buildNav(table) {
    const tr0 = this.dom.spawn(table, "TR");
    this.dom.spawn(tr0, "TD");
    this.dom.spawn(tr0, "TD", this.dom.spawn(null, "INPUT", { type: "button", value: "^", "on-click": () => this.nav(0, -1) }));
    this.dom.spawn(tr0, "TD");
    const tr1 = this.dom.spawn(table, "TR");
    this.dom.spawn(tr1, "TD", this.dom.spawn(null, "INPUT", { type: "button", value: "<", "on-click": () => this.nav(-1, 0) }));
    this.dom.spawn(tr1, "TD", ["tileid"], this.tileid.toString(16));
    this.dom.spawn(tr1, "TD", this.dom.spawn(null, "INPUT", { type: "button", value: ">", "on-click": () => this.nav(1, 0) }));
    const tr2 = this.dom.spawn(table, "TR");
    this.dom.spawn(tr2, "TD");
    this.dom.spawn(tr2, "TD", this.dom.spawn(null, "INPUT", { type: "button", value: "v", "on-click": () => this.nav(0, 1) }));
    this.dom.spawn(tr2, "TD");
  }
  
  populateUi() {
    this.element.querySelector(".tileid").innerText = "0x" + this.tileid.toString(16).padStart(2, '0');
    this.nmaskRender();
    
    const fields = this.element.querySelector(".fields");
    fields.innerHTML = "";
    const names = Array.from(new Set([...DEFAULT_TILESHEET_TABLES, ...Object.keys(this.tables)]));
    for (const name of names) {
      if (name === "neighbors") continue; // we're providing better UI for this up above
      const tr = this.dom.spawn(fields, "TR");
      this.dom.spawn(tr, "TD", ["key"], name);
      const tdk = this.dom.spawn(tr, "TD", ["value"]);
      const input = this.dom.spawn(tdk, "INPUT", {
        name,
        type: "number",
        min: 0,
        max: 255,
        value: this.tables[name]?.[this.tileid] || 0,
        "on-input": () => {
          if (!this.tables[name]) this.tables[name] = new Uint8Array(256);
          this.tables[name][this.tileid] = +input.value;
          this.resmgr.dirty(this.path, this.encode);
        },
      });
      if (name === "family") {
        this.dom.spawn(tdk, "INPUT", { type: "button", value: "New", "on-click": () => this.newFamily() });
      }
    }
    
    const col = this.tileid & 15;
    const row = this.tileid >> 4;
    const srcx = col * this.tilesize;
    const srcy = row * this.tilesize;
    let psp = 0;
    for (const button of this.element.querySelectorAll(".presets > button")) {
      const preset = PRESETS[psp++];
      if (!preset) break;
      const canvas = button.querySelector("canvas");
      canvas.width = this.tilesize * preset.w;
      canvas.height = this.tilesize * preset.h;
      const ctx = canvas.getContext("2d");
      if (this.image) {
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        ctx.drawImage(this.image, srcx, srcy, canvas.width, canvas.height, 0, 0, canvas.width, canvas.height);
      }
      button.disabled = ((col + preset.w > 16) || (row + preset.h > 16));
    }
  }
  
  nmaskRender() {
    const canvas = this.element.querySelector(".nmask");
    const ctx = canvas.getContext("2d");
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    if (this.image) {
      const srcx = (this.tileid & 15) * this.tilesize;
      const srcy = (this.tileid >> 4) * this.tilesize;
      const dstx = (canvas.width >> 1) - (this.tilesize >> 1);
      const dsty = (canvas.height >> 1) - (this.tilesize >> 1);
      ctx.drawImage(this.image, srcx, srcy, this.tilesize, this.tilesize, dstx, dsty, this.tilesize, this.tilesize);
    }
    const x0 = (this.tilesize >> 1) + 0.5;
    const x1 = x0 + this.tilesize;
    const y0 = (this.tilesize >> 1) + 0.5;
    const y1 = y0 + this.tilesize;
    ctx.fillStyle = "#0f0";
    const v = this.tables.neighbors?.[this.tileid] || 0;
    if (v & 0x80) ctx.fillRect(0, 0, x0, y0);
    if (v & 0x40) ctx.fillRect(x0, 0, x1 - x0, y0);
    if (v & 0x20) ctx.fillRect(x1, 0, canvas.width - x1, y0);
    if (v & 0x10) ctx.fillRect(0, y0, x0, y1 - y0);
    if (v & 0x08) ctx.fillRect(x1, y0, canvas.width - x1, y1 - y0);
    if (v & 0x04) ctx.fillRect(0, y1, x0, canvas.height - y1);
    if (v & 0x02) ctx.fillRect(x0, y1, x1 - x0, canvas.height - y1);
    if (v & 0x01) ctx.fillRect(x1, y1, canvas.width - x1, canvas.height - y1);
    ctx.beginPath();
    ctx.moveTo(0.5, 0.5);
    ctx.lineTo(0.5, canvas.height - 0.5);
    ctx.lineTo(canvas.width - 0.5, canvas.height - 0.5);
    ctx.lineTo(canvas.width - 0.5, 0);
    ctx.lineTo(0.5, 0.5);
    ctx.moveTo(x0, 0);
    ctx.lineTo(x0, canvas.height);
    ctx.moveTo(x1, 0);
    ctx.lineTo(x1, canvas.height);
    ctx.moveTo(0, y0);
    ctx.lineTo(canvas.width, y0);
    ctx.moveTo(0, y1);
    ctx.lineTo(canvas.width, y1);
    ctx.strokeStyle = "#000";
    ctx.stroke();
  }
  
  nav(dx, dy) {
    let x = this.tileid & 15;
    let y = this.tileid >> 4;
    x += dx;
    y += dy;
    // No wrapping. And no sense clamping, since we will only move by ones.
    if ((x < 0) || (y < 0) || (x >= 16) || (y >= 16)) return;
    this.tileid = (y << 4) | x;
    this.populateUi();
  }
  
  nmaskForPoint(x, y) {
    const canvas = this.element.querySelector(".nmask");
    const bounds = canvas.getBoundingClientRect();
    // Canvas is divided into quarters, with the middle 4 the thumbnail.
    const col = Math.floor(((x - bounds.x) * 4) / bounds.width);
    const row = Math.floor(((y - bounds.y) * 4) / bounds.height);
    if (row === 0) {
      if (col === 0) return 0x80;
      if (col === 1) return 0x40;
      if (col === 2) return 0x40;
      if (col === 3) return 0x20;
    } else if ((row === 1) || (row === 2)) {
      if (col === 0) return 0x10;
      if (col === 3) return 0x08;
    } else if (row === 3) {
      if (col === 0) return 0x04;
      if (col === 1) return 0x02;
      if (col === 2) return 0x02;
      if (col === 3) return 0x01;
    }
    return 0;
  }
  
  onNmaskMoveOrUp(event) {
    if (event.type === "mouseup") {
      this.window.removeEventListener("mouseup", this.mouseListener);
      this.window.removeEventListener("mousemove", this.mouseListener);
      this.mouseListener = null;
      return;
    }
    const mask = this.nmaskForPoint(event.x, event.y);
    if (!mask) return;
    if (this.nmaskPaintState) {
      if (this.tables.neighbors[this.tileid] & mask) return;
      this.tables.neighbors[this.tileid] |= mask;
    } else {
      if (!(this.tables.neighbors[this.tileid] & mask)) return;
      this.tables.neighbors[this.tileid] &= ~mask;
    }
    this.nmaskRender();
    this.resmgr.dirty(this.path, this.encode);
  }
  
  onNmaskMouseDown(event) {
    if (this.mouseListener) return;
    const mask = this.nmaskForPoint(event.x, event.y);
    if (!mask) return;
    if (!this.tables.neighbors) this.tables.neighbors = new Uint8Array(256);
    this.nmaskPaintState = !(this.tables.neighbors[this.tileid] & mask);
    this.tables.neighbors[this.tileid] ^= mask;
    this.nmaskRender();
    this.resmgr.dirty(this.path, this.encode);
    this.mouseListener = e => this.onNmaskMoveOrUp(e);
    this.window.addEventListener("mousemove", this.mouseListener);
    this.window.addEventListener("mouseup", this.mouseListener);
  }
  
  applyPreset(preset) {
    const col = this.tileid & 0x0f;
    const row = this.tileid >> 4;
    if (col + preset.w > 16) return;
    if (row + preset.h > 16) return;
    if (!this.tables.neighbors) this.tables.neighbors = new Uint8Array(256);
    if (!this.tables.physics) this.tables.physics = new Uint8Array(256);
    if (!this.tables.family) this.tables.family = new Uint8Array(256);
    const p0 = row * 16 + col;
    let np = 0;
    for (let suby=0; suby<preset.h; suby++) {
      for (let subx=0; subx<preset.w; subx++, np++) {
        const p = (row + suby) * 16 + col + subx;
        this.tables.neighbors[p] = preset.neighbors[np];
        this.tables.physics[p] = this.tables.physics[p0];
        this.tables.family[p] = this.tables.family[p0];
      }
    }
    this.resmgr.dirty(this.path, this.encode);
    this.populateUi();
  }
  
  newFamily() {
    if (!this.tables.family) this.tables.family = new Uint8Array(256);
    const usage = [];
    for (let i=256; i-->0; ) usage[this.tables.family[i]] = true;
    for (let i=1; i<256; i++) if (!usage[i]) {
      this.tables.family[this.tileid] = i;
      this.element.querySelector("input[name='family']").value = i;
      this.resmgr.dirty(this.path, this.encode);
      return;
    }
  }
}
