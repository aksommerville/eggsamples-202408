/* MapBus.js
 * Since we have a bunch of separate components, let them share state across this special bus.
 */
 
const MOD_SHIFT = 1;
const MOD_CONTROL = 2;
const MOD_ALT = 4;
 
export class MapBus {
  static getDependencies() {
    return [Window];
  }
  constructor(window) {
    this.window = window;
    
    this.nextId = 1;
    this.listeners = [];
    
    // State.
    this.visibility = {
      grid: false,
      image: true,
      points: true,
      regions: false,
      neighbors: false,
    };
    this.explicitTool = "rainbow";
    this.effectiveTool = "rainbow";
    this.tileid = 0x00;
    this.mousecol = 0;
    this.mouserow = 0;
    this.mousesubx = 0; // Only valid immediately after a mousedown; motion doesn't update it.
    this.mousesuby = 0;
    
    this.mod = 0;
    this.window.addEventListener("keyup", e => this.onKeyUp(e));
    this.window.addEventListener("keydown", e => this.onKeyDown(e));
  }
  
  listen(cb) {
    const id = this.nextId++;
    this.listeners.push({cb, id});
    return id;
  }
  
  unlisten(id) {
    const p = this.listeners.findIndex(l => l.id === id);
    if (p < 0) return;
    this.listeners.splice(p, 1);
  }
  
  broadcast(e) {
    for (const { cb } of this.listeners) cb(e);
  }
  
  onKeyDown(e) {
    switch (e.code) {
      case "ControlLeft": case "ControlRight": this.mod |= MOD_CONTROL; return this.reconsiderEffectiveTool();
      case "ShiftLeft": case "ShiftRight": this.mod |= MOD_SHIFT; return this.reconsiderEffectiveTool();
      case "AltLeft": case "AltRight": this.mod |= MOD_ALT; return this.reconsiderEffectiveTool();
    }
  }
  
  onKeyUp(e) {
    switch (e.code) {
      case "ControlLeft": case "ControlRight": this.mod &= ~MOD_CONTROL; return this.reconsiderEffectiveTool();
      case "ShiftLeft": case "ShiftRight": this.mod &= ~MOD_SHIFT; return this.reconsiderEffectiveTool();
      case "AltLeft": case "AltRight": this.mod &= ~MOD_ALT; return this.reconsiderEffectiveTool();
    }
  }
  
  /* State accessors.
   ***********************************************************************/
   
  setVisibility(k, v) {
    v = !!v;
    if (this.visibility[k] === v) return;
    this.visibility[k] = v;
    this.broadcast({ type: "visibility" });
  }
  
  setExplicitTool(name) {
    if (name === this.explicitTool) return;
    this.explicitTool = name;
    this.reconsiderEffectiveTool();
  }
  
  reconsiderEffectiveTool() {
    let et = this.explicitTool;
    switch (this.explicitTool) {
      case "pencil": {
          if (this.mod & MOD_CONTROL) et = "pickup";
          else if (this.mod & MOD_SHIFT) et = "rainbow";
        } break;
      case "repair": {
        } break;
      case "rainbow": {
          if (this.mod & MOD_CONTROL) et = "pickup";
          else if (this.mod & MOD_SHIFT) et = "pencil";
        } break;
      case "monalisa": {
          if (this.mod & MOD_CONTROL) et = "pickup";
          else if (this.mod & MOD_SHIFT) et = "pencil";
        } break;
      case "pickup": {
        } break;
      case "poimove": {
          if (this.mod & MOD_CONTROL) et = "poiedit";
          else if (this.mod & MOD_SHIFT) et = "poidelete";
        } break;
      case "poiedit": {
          if (this.mod & MOD_CONTROL) et = "poimove";
          else if (this.mod & MOD_SHIFT) et = "poidelete";
        } break;
      case "poidelete": {
          if (this.mod & MOD_CONTROL) et = "poiedit";
          else if (this.mod & MOD_SHIFT) et = "poimove";
        } break;
    }
    if (et === this.effectiveTool) return;
    this.effectiveTool = et;
    this.broadcast({ type: "tool" });
  }
  
  setMouse(col, row, subx, suby) {
    if (!subx) subx = 0;
    if (!suby) suby = 0;
    if ((col === this.mousecol) && (row === this.mouserow) && (subx === this.mousesubx) && (suby === this.mousesuby)) return;
    this.mousecol = col;
    this.mouserow = row;
    this.mousesubx = subx;
    this.mousesuby = suby;
    this.broadcast({ type: "motion" });
  }
  
  beginPaint() {
    this.broadcast({ type: "beginPaint" });
  }
  
  endPaint() {
    this.broadcast({ type: "endPaint" });
  }
  
  dirty() {
    this.broadcast({ type: "dirty" });
  }
  
  commandsChanged() {
    this.broadcast({ type: "commandsChanged" });
  }
  
  setTileid(tileid) {
    if (tileid === this.tileid) return;
    this.tileid = tileid;
    this.broadcast({ type: "tileid" });
  }
}

MapBus.singleton = true;
