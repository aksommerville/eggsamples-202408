/* MapToolbarUi.js
 * Top of the editor area.
 */
 
import { Dom } from "./Dom.js";
import { MapBus } from "./MapBus.js";
import { Resmgr } from "./Resmgr.js";
import { PaletteModal } from "./PaletteModal.js";
import { CommandsModal } from "./CommandsModal.js";

export class MapToolbarUi {
  static getDependencies() {
    return [HTMLElement, Dom, MapBus, "nonce", Resmgr];
  }
  constructor(element, dom, mapBus, nonce, resmgr) {
    this.element = element;
    this.dom = dom;
    this.mapBus = mapBus;
    this.nonce = nonce;
    this.resmgr = resmgr;
    
    this.map = null;
    this.mapBusListener = this.mapBus.listen(e => this.onBusEvent(e));
    
    this.buildUi();
  }
  
  onRemoveFromDom() {
    this.mapBus.unlisten(this.mapBusListener);
  }
  
  setMap(map) {
    this.map = map;
    this.drawPalette();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    
    /* Visibility toggles.
     */
    this.spawnVisibilityToggle("grid", 0);
    this.spawnVisibilityToggle("image", 1);
    this.spawnVisibilityToggle("points", 2);
    this.spawnVisibilityToggle("regions", 3);
    this.spawnVisibilityToggle("neighbors", 4);
    
    /* Palette thumbnail.
     */
    this.dom.spawn(this.element, "CANVAS", ["palette"], { width: "32", height: "32", "on-click": () => this.onOpenPalette() });
    
    /* Tools.
     */
    this.spawnTool("pencil", 0);
    this.spawnTool("rainbow", 1);
    this.spawnTool("monalisa", 2);
    this.spawnTool("pickup", 3);
    this.spawnTool("poimove", 4);
    this.spawnTool("poiedit", 5);
    this.spawnTool("poidelete", 6);
    this.spawnTool("repair", 7);
    
    /* Mouse tattle.
     */
    this.dom.spawn(this.element, "DIV", ["tattle"]);
    
    /* Commands button.
     */
    this.dom.spawn(this.element, "INPUT", { type: "button", value: "Commands", "on-click": () => this.onOpenCommands() });
  }
  
  spawnVisibilityToggle(k, iconix) {
    const id = `MapToolbarUi-${this.nonce}-${k}`;
    const input = this.dom.spawn(this.element, "INPUT", ["toggle"], { type: "checkbox", id });
    const label = this.dom.spawn(this.element, "LABEL", { for: id });
    const icon = this.dom.spawn(label, "SPAN");
    icon.style.backgroundPositionX = (iconix * -16) + "px";
    if (this.mapBus.visibility[k]) input.checked = true;
    input.addEventListener("change", () => {
      this.mapBus.setVisibility(k, input.checked);
    });
  }
  
  spawnTool(name, iconix) {
    const btn = this.dom.spawn(this.element, "DIV", ["tool"], { name, "on-click": () => this.mapBus.setExplicitTool(name) });
    const icon = this.dom.spawn(btn, "DIV", ["icon"]);
    icon.style.backgroundPositionX = (iconix * -16) + "px";
    if (name === this.mapBus.effectiveTool) btn.classList.add("selected");
  }
  
  drawPalette() {
    if (!this.map) return;
    const imageName = this.map.getCommandByKeyword("image") || "";
    const image = this.resmgr.getImage(imageName);
    if (!image?.complete) {
      if (image) image.addEventListener("load", () => this.drawPalette());
      return;
    }
    const tilesize = image.naturalWidth >> 4;
    
    const cvs = this.element.querySelector(".palette");
    const bounds = cvs.getBoundingClientRect();
    cvs.width = tilesize;
    cvs.height = tilesize;
    const ctx = cvs.getContext("2d");
    ctx.clearRect(0, 0, bounds.width, bounds.height);
    const srcx = (this.mapBus.tileid & 15) * tilesize;
    const srcy = (this.mapBus.tileid >> 4) * tilesize;
    ctx.drawImage(image, srcx, srcy, tilesize, tilesize, 0, 0, tilesize, tilesize);
  }
  
  onOpenPalette() {
    const modal = this.dom.spawnModal(PaletteModal);
    const imageName = this.map.getCommandByKeyword("image") || "";
    const image = this.resmgr.getImage(imageName);
    modal.setup(image, this.mapBus.tileid);
    modal.result.then(tileid => this.mapBus.setTileid(tileid)).catch(() => {});
  }
  
  onOpenCommands() {
    if (!this.map) return;
    const modal = this.dom.spawnModal(CommandsModal);
    modal.setup(this.map.commands);
    modal.result.then(commands => {
      this.map.commands = commands;
      this.mapBus.dirty();
      this.mapBus.commandsChanged();
    }).catch(() => {});
  }
  
  highlightTool(name) {
    for (const element of this.element.querySelectorAll(".tool")) {
      element.classList.remove("selected");
      element.classList.remove("transient");
    }
    const btn = this.element.querySelector(`.tool[name='${name}']`);
    if (btn) {
      btn.classList.add("selected");
      if (name !== this.mapBus.explicitTool) {
        btn.classList.add("transient");
      }
    }
  }
  
  setTattle(x, y) {
    const tattle = this.element.querySelector(".tattle");
    tattle.innerText = x.toString().padStart(2, ' ') + "," + y.toString().padStart(2, ' ');
    if (this.map && (x >= 0) && (y >= 0) && (x < this.map.w) && (y < this.map.h)) {
      tattle.classList.remove("oob");
    } else {
      tattle.classList.add("oob");
    }
  }
  
  onBusEvent(e) {
    switch (e.type) {
      case "tool": this.highlightTool(this.mapBus.effectiveTool); break;
      case "motion": this.setTattle(this.mapBus.mousecol, this.mapBus.mouserow); break;
      case "tileid": this.drawPalette(); break;
    }
  }
}
