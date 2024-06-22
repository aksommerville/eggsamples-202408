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

export class MapEditor {
  static getDependencies() {
    return [HTMLElement, Dom, Resmgr, MapBus, MapPainter];
  }
  constructor(element, dom, resmgr, mapBus, mapPainter) {
    this.element = element;
    this.dom = dom;
    this.resmgr = resmgr;
    this.mapBus = mapBus;
    this.mapPainter = mapPainter;
    
    this.map = null;
    this.path = "";
    this.toolbar = null;
    this.canvas = null;
    
    this.mapBusListener = this.mapBus.listen(e => this.onBusEvent(e));
    
    this.globalsReady = false;
    MagicGlobals.require().then(() => {
      this.globalsReady = true;
      this.maybeReady();
    }).catch(e => console.error(e));
  }
  
  onRemoveFromDom() {
    this.mapBus.unlisten(this.mapBusListener);
    this.mapPainter.clear();
  }
  
  setup(serial, path) {
    this.map = new MapRes(serial);
    this.path = path;
    this.maybeReady();
  }
  
  maybeReady() {
    if (!this.globalsReady) return;
    if (!this.map) return;
    this.mapPainter.reset(this.map);
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.toolbar = this.dom.spawnController(this.element, MapToolbarUi);
    this.canvas = this.dom.spawnController(this.element, MapCanvasUi);
    this.toolbar.setMap(this.map);
    this.canvas.setMap(this.map);
  }
  
  onBusEvent(e) {
    switch (e.type) {
      case "dirty": if (this.map) this.resmgr.dirty(this.path, () => this.map.encode()); break;
    }
  }
}
