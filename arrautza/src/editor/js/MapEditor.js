/* MapEditor.js
 * Top level of our custom map editor.
 */
 
import { Dom } from "./Dom.js";
import { Resmgr } from "./Resmgr.js";
import { MapRes } from "./MapRes.js";
import { MagicGlobals } from "./MagicGlobals.js";

export class MapEditor {
  static getDependencies() {
    return [HTMLElement, Dom, Resmgr];
  }
  constructor(element, dom, resmgr) {
    this.element = element;
    this.dom = dom;
    this.resmgr = resmgr;
    
    MagicGlobals.require().catch(e => console.error(e));
    
    this.map = null;
    this.rid = 0;
    
    this.buildUi();
  }
  
  setup(serial, rid) {
    this.map = new MapRes(serial);
    this.rid = rid;
    console.log(`MapEditor.setup`, { serial, rid, map: this.map });
  }
  
  buildUi() {
    this.element.innerHTML = "";
  }
}
