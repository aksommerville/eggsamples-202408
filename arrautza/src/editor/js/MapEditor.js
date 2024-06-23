/* MapEditor.js
 * Top level of our custom map editor.
 */
 
import { Dom } from "./Dom.js";
import { Resmgr } from "./Resmgr.js";
import { MapRes } from "./MapRes.js";
import { MagicGlobals } from "./MagicGlobals.js";
import { MapToolbarUi } from "./MapToolbarUi.js";
import { MapCanvasUi } from "./MapCanvasUi.js";
import { MapBus } from "./MapBus.js";
import { MapPainter } from "./MapPainter.js";
import { MapStore } from "./MapStore.js";
import { Bus } from "./Bus.js";

export class MapEditor {
  static getDependencies() {
    return [HTMLElement, Dom, Resmgr, MapBus, MapPainter, MapStore, Bus];
  }
  constructor(element, dom, resmgr, mapBus, mapPainter, mapStore, bus) {
    this.element = element;
    this.dom = dom;
    this.resmgr = resmgr;
    this.mapBus = mapBus;
    this.mapPainter = mapPainter;
    this.mapStore = mapStore;
    this.bus = bus;
    
    this.loc = null; // From MapStore: {plane,x,y,res,map}
    this.map = null;
    this.path = "";
    this.toolbar = null;
    this.canvas = null;
    
    this.mapBusListener = this.mapBus.listen(e => this.onBusEvent(e));
    
    this.globalsReady = false;
    MagicGlobals.require().then(() => {
      this.globalsReady = true;
      this.mapStore.require();
      this.maybeReady();
    }).catch(e => console.error(e));
  }
  
  onRemoveFromDom() {
    this.mapBus.unlisten(this.mapBusListener);
    this.mapPainter.clear();
  }
  
  setup(serial, path) {
    // Beware that MagicGlobals and MapStore might not be ready yet -- initial load, it's basically guaranteed they are not.
    this.path = path;
    this.maybeReady();
  }
  
  maybeReady() {
    if (!this.globalsReady) return;
    if (!this.path) return;
    this.loc = this.mapStore.findPath(this.path);
    if (!this.loc) throw new Error(`Map ${this.path} not found in MapStore`);
    this.map = this.loc.map;
    this.mapBus.setLoc(this.loc);
    this.mapPainter.reset(this.map);
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.toolbar = this.dom.spawnController(this.element, MapToolbarUi);
    this.canvas = this.dom.spawnController(this.element, MapCanvasUi);
    this.toolbar.setMap(this.map);
  }
  
  onOpen(rid) {
    if (!rid) return;
    const loc = this.mapStore.entryByRid(rid);
    if (!loc) return;
    this.bus.broadcast({ type: "open", path: loc.res.path });
  }
  
  onBusEvent(e) {
    switch (e.type) {
      case "dirty": if (this.map) this.resmgr.dirty(this.path, () => this.map.encode()); break;
      case "open": this.onOpen(e.rid); break;
    }
  }
}
