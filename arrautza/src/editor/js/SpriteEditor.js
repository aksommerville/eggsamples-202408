/* SpriteEditor.js
 * When we say "sprite" in the context of the editor, we mean the resources,
 * which correspond to struct sprdef in src/general/sprite.h.
 */
 
import { Dom } from "./Dom.js";
import { Resmgr } from "./Resmgr.js";
import { Sprite } from "./Sprite.js";
import { Bus } from "./Bus.js";
import { MagicGlobals } from "./MagicGlobals.js";

export class SpriteEditor {
  static getDependencies() {
    return [HTMLElement, Dom, Resmgr, Bus];
  }
  constructor(element, dom, resmgr, bus) {
    this.element = element;
    this.dom = dom;
    this.resmgr = resmgr;
    this.bus = bus;
    
    this.serial = null;
    this.path = "";
    this.sprite = null;
    
    this.buildUi();
  }
  
  setup(serial, path) {
    this.path = path;
    this.serial = serial;
    this.sprite = new Sprite(this.serial);
    console.log(`SpriteEditor.setup`, { path, serial, sprite: this.sprite });
    MagicGlobals.require().then(() => {
      this.populateUi();
    }).catch(error => this.bus.broadcast({ type: "error", error }));
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const table = this.dom.spawn(this.element, "TABLE", { "on-input": () => this.dirty() });
    this.dom.spawn(this.element, "INPUT", { type: "button", value: "+", "on-click": () => this.onAddCommand() });
  }
  
  populateUi() {
    const table = this.element.querySelector("table");
    table.innerHTML = "";
    if (!this.sprite) return;
    for (let i=0; i<this.sprite.commands.length; i++) {
      const command = this.sprite.commands[i];
      const tr = this.dom.spawn(table, "TR", { "data-key": command[0] });
      this.dom.spawn(tr, "TD", ["controls"],
        this.dom.spawn(null, "INPUT", { type: "button", value: "X", "on-click": () => this.onDeleteCommand(i) })
      );
      this.dom.spawn(tr, "TD", ["key"], command[0]);
      const tdv = this.dom.spawn(tr, "TD", ["value"]);
      this.spawnUiForValue(command[0], tdv, command.slice(1));
    }
  }
  
  spawnControlsGeneric(td, value) {
    this.dom.spawn(td, "INPUT", { type: "text", value: value.join(" ") });
  }
  
  spawnControlsImage(td, value) {
    return this.spawnControlsGeneric(td, value);//TODO
  }
  
  spawnControlsTileid(td, value) {
    return this.spawnControlsGeneric(td, value);//TODO
  }
  
  spawnControlsXform(td, value) {
    return this.spawnControlsGeneric(td, value);//TODO
  }
  
  spawnControlsSprctl(td, value) {
    return this.spawnControlsGeneric(td, value);//TODO
  }
  
  spawnControlsGroups(td, value) {
    return this.spawnControlsGeneric(td, value);//TODO
  }
  
  spawnUiForValue(key, td, value) {
    switch (key) {
        
      case "image": this.spawnControlsImage(td, value); break;
      case "tileid": this.spawnControlsTileid(td, value); break;
      case "xform": this.spawnControlsXform(td, value); break;
      case "sprctl": this.spawnControlsSprctl(td, value); break;
      case "groups": this.spawnControlsGroups(td, value); break;
        
      default: this.spawnControlsGeneric(td, value);
    }
  }
  
  readValueFromUi(key, td) {
    switch (key) {
    
      //TODO UI for specific commands
    
    }
    const input = td.querySelector("input");
    if (input) return input.value.split(/\s+/g).filter(v => v);
    return [];
  }
  
  readSpriteFromUi() {
    const sprite = new Sprite(null);
    for (const tr of this.element.querySelectorAll("tr[data-key]")) {
      const key = tr.getAttribute("data-key");
      const tdv = tr.querySelector("td.value");
      const value = this.readValueFromUi(key, tdv);
      sprite.commands.push([key, ...value]);
    }
    return sprite;
  }
  
  dirty() {
    this.resmgr.dirty(this.path, () => {
      this.sprite = this.readSpriteFromUi();
      return this.sprite.encode();
    });
  }
  
  onDeleteCommand(index) {
    console.log(`onDeleteCommand ${index}/${this.sprite.commands.length}`);
    if (!this.sprite || (index < 0) || (index >= this.sprite.commands.length)) return;
    this.sprite.commands.splice(index, 1);
    this.populateUi();
    this.dirty();
  }
  
  onAddCommand() {
    if (!this.sprite) return;
    const keywords = MagicGlobals.spritecmd.map(v => v[1]).filter(kw => !this.element.querySelector(`tr[data-key='${kw}']`));
    this.dom.modalPickOne(keywords, "Keyword").then(rsp => {
      if (!rsp) return;
      this.sprite.commands.push([rsp]);
      this.populateUi();
      this.dirty();
    }).catch(() => {});
  }
}
