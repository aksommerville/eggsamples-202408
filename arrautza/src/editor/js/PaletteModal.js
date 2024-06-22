/* PaletteModal.js
 * Show the tilesheet image, with one cell highlighted, and lets you click to pick one.
 */
 
import { Dom } from "./Dom.js";
 
export class PaletteModal {
  static getDependencies() {
    return [HTMLDialogElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    this.element.addEventListener("click", e => this.onClick(e));
  }
  
  // Parent must call this. (image) can be null.
  setup(image, tileid) {
    if (image) {
      const cvs = this.dom.spawn(this.element, "CANVAS", { width: image.naturalWidth, height: image.naturalHeight });
      const ctx = cvs.getContext("2d");
      ctx.drawImage(image, 0, 0);
      const tilesize = image.naturalWidth >> 4;
      const x = (tileid & 0x0f) * tilesize + 0.5;
      const y = (tileid >> 4) * tilesize + 0.5;
      ctx.beginPath();
      ctx.moveTo(x, y);
      ctx.lineTo(x, y + tilesize - 1);
      ctx.lineTo(x + tilesize - 1, y + tilesize - 1);
      ctx.lineTo(x + tilesize - 1, y);
      ctx.lineTo(x, y);
      ctx.strokeStyle = "#f80";
      ctx.stroke();
    } else {
      const table = this.dom.spawn(this.element, "TABLE");
      for (let y=0; y<16; y++) {
        const tr = this.dom.spawn(table, "TR");
        for (let x=0; x<16; x++) {
          const tdtileid = (y << 4) | x;
          const tileidString = tdtileid.toString(16).padStart(2, '0');
          const td = this.dom.spawn(tr, "TD", { "data-tileid": tdtileid }, tileidString);
          if (tdtileid === tileid) td.classList.add("highlight");
        }
      }
    }
  }
  
  onClick(event) {
    if (!event.target) return;
    if (event.target.tagName === "CANVAS") {
      const bounds = event.target.getBoundingClientRect();
      const tilesize = bounds.width >> 4;
      const col = Math.floor((event.x - bounds.x) / tilesize);
      const row = Math.floor((event.y - bounds.y) / tilesize);
      if ((col >= 0) && (row >= 0) && (col < 16) && (row < 16)) {
        this.resolve((row << 4) | col);
      } else {
        this.reject();
      }
    } else if (event.target.tagName === "TD") {
      const tileid = +event.target.getAttribute("data-tileid");
      if (isNaN(tileid)) return;
      this.resolve(tileid);
    }
  }
}
