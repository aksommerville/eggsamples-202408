/* MapBus.js
 * Since we have a bunch of separate components, let them share state across this special bus.
 */
 
import { TilesheetEditor } from "./TilesheetEditor.js"; // For its static decode() function.
import { Resmgr } from "./Resmgr.js";
 
const MOD_SHIFT = 1;
const MOD_CONTROL = 2;
const MOD_ALT = 4;
 
export class MapBus {
  static getDependencies() {
    return [Window, Resmgr];
  }
  constructor(window, resmgr) {
    this.window = window;
    this.resmgr = resmgr;
    
    this.nextId = 1;
    this.listeners = [];
    
    // State.
    this.visibility = {
      grid: false,
      image: true,
      points: true,
      regions: false,
      neighbors: true,
    };
    this.explicitTool = "rainbow";
    this.effectiveTool = "rainbow";
    this.tileid = 0x00;
    this.mousecol = 0;
    this.mouserow = 0;
    this.mousesubx = 0; // Only valid immediately after a mousedown; motion doesn't update it.
    this.mousesuby = 0;
    this.loc = null; // From MapStore: {plane,x,y,res,map}. Will always be populated before MapEditor builds its children.
    this.entrances = []; // From MapCanvasUi. MapStore.doors: {srcrid,srcx,srcy,dstrid,dstx,dsty}, we are "dst".
    this.tstables = {}; // [name]:Uint8Array(256), from tilesheet resource. We update at setLoc()
    
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
  
  setLoc(loc) {
    this.loc = loc;
    this.tstables = {};
    const imageName = this.loc.map.getCommandByKeyword("image");
    const imageRes = this.resmgr.resByString(imageName, "image");
    const rid = imageRes?.rid;
    if (rid) {
      const tsres = this.resmgr.resById("tilesheet", rid);
      if (tsres) {
        this.tstables = TilesheetEditor.decode(tsres.serial);
      }
    }
    this.broadcast({ type: "loc" });
  }
  
  setEntrances(entrances) {
    this.entrances = entrances;
    this.broadcast({ type: "entrances" });
  }
}

MapBus.singleton = true;
