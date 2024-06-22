/* MapPainter.js
 * Receives events and state from MapBus, applies painting operations to the map.
 */
 
import { MapBus } from "./MapBus.js";
import { Dom } from "./Dom.js"; // For spawning modals only.
import { CommandModal } from "./CommandModal.js";
 
export class MapPainter {
  static getDependencies() {
    return [MapBus, Dom];
  }
  constructor(mapBus, dom) {
    this.mapBus = mapBus;
    this.dom = dom;
    this.map = null;
    this.toolInProgress = "";
    this.anchorx = 0; // monalisa
    this.anchory = 0;
    this.busListener = this.mapBus.listen(e => this.onBusEvent(e));
  }
  
  reset(map) {
    this.map = map;
  }
  
  clear() {
    this.map = null;
  }
  
  /* pencil: Write selected tile to the map verbatim, no bells or whistles.
   ********************************************************/
   
  pencilUpdate() {
    const p = this.mousePosition1d();
    if (p < 0) return;
    if (this.map.v[p] === this.mapBus.tileid) return;
    this.map.v[p] = this.mapBus.tileid;
    this.mapBus.dirty();
  }
  
  /* repair: Randomize and join neighbors for the focussed cell.
   * There is no "rainbowUpdate": It's just pencil + repair.
   ********************************************************/
   
  repairUpdate() {
    const p = this.mousePosition1d();
    if (p < 0) return;
    //TODO repair tool
  }
  
  /* monalisa: Drop an anchor and paint a large verbatim region from the tilesheet.
   *******************************************************/
   
  monalisaUpdate() {
    const p = this.mousePosition1d();
    if (p < 0) return;
    const dx = this.mapBus.mousecol - this.anchorx;
    const dy = this.mapBus.mouserow - this.anchory;
    const tilex = (this.mapBus.tileid & 0x0f) + dx;
    const tiley = (this.mapBus.tileid >> 4) + dy;
    if ((tilex < 0) || (tilex >= 0x10) || (tiley < 0) || (tiley >= 0x11)) return;
    const tileid = (tiley << 4) | tilex;
    if (this.map.v[p] === tileid) return;
    this.map.v[p] = tileid;
    this.mapBus.dirty();
  }
   
  monalisaBegin() {
    const p = this.mousePosition1d();
    if (p < 0) {
      this.toolInProgress = "";
      return;
    }
    this.anchorx = this.mapBus.mousecol;
    this.anchory = this.mapBus.mouserow;
    this.monalisaUpdate();
  }
  
  /* pickup: Replace the selected tile with one from the map.
   ********************************************************/
   
  pickupBegin() {
    this.toolInProgress = "";
    const p = this.mousePosition1d();
    if (p < 0) return;
    this.mapBus.setTileid(this.map.v[p]);
  }
  
  /* poimove: Drag point commands or corners of region commands.
   ***********************************************************/
   
  poimoveUpdate() {
    const dx = this.mapBus.mousecol - this.poimovePvx;
    const dy = this.mapBus.mouserow - this.poimovePvy;
    if (!dx && !dy) return;
    this.poimovePvx = this.mapBus.mousecol;
    this.poimovePvy = this.mapBus.mouserow;
    const ocmd = this.map.commands[this.poimoveIndex];
    const ncmd = this.map.moveCommand(ocmd, dx, dy);
    this.map.commands[this.poimoveIndex] = ncmd;
    this.mapBus.dirty();
    this.mapBus.commandsChanged();
  }
   
  poimoveBegin() {
    const cmdp = this.findPoi();
    if ((cmdp < 0) || (cmdp >= this.map.commands.length)) {
      this.toolInProgress = "";
      return;
    }
    this.poimoveIndex = cmdp;
    this.poimovePvx = this.mapBus.mousecol;
    this.poimovePvy = this.mapBus.mouserow;
  }
  
  /* poiedit: Open a modal for an existing point or region command.
   ********************************************************/
   
  poieditBegin() {
    this.toolInProgress = "";
    const cmdp = this.findPoi();
    if (cmdp < 0) return;
    if (cmdp >= this.map.commands.length) return;
    const modal = this.dom.spawnModal(CommandModal);
    modal.setup(this.map.commands[cmdp]);
    modal.result.then(cmd => {
      this.map.commands[cmdp] = cmd;
      this.mapBus.dirty();
      this.mapBus.commandsChanged();
    }).catch(() => {});
  }
  
  /* poidelete: One-click delete for point and region commands.
   ********************************************************/
   
  poideleteBegin() {
    this.toolInProgress = "";
    const cmdp = this.findPoi();
    if (cmdp < 0) return;
    this.map.commands.splice(cmdp, 1);
    this.mapBus.dirty();
    this.mapBus.commandsChanged();
  }
  
  /* General event dispatch.
   *****************************************************/
   
  // Index in (this.map.v) for the current hover cell, or <0 if OOB.
  mousePosition1d() {
    if ((this.mapBus.mousecol < 0) || (this.mapBus.mousecol >= this.map.w)) return -1;
    if ((this.mapBus.mouserow < 0) || (this.mapBus.mouserow >= this.map.h)) return -1;
    return this.mapBus.mouserow * this.map.w + this.mapBus.mousecol;
  }
  
  // Index in (this.map.commands) or <0. Point or region, according to visibility.
  findPoi() {
    if ((this.mapBus.mousecol < 0) || (this.mapBus.mousecol >= this.map.w)) return -1;
    if ((this.mapBus.mouserow < 0) || (this.mapBus.mouserow >= this.map.h)) return -1;
    let subp = ((this.mapBus.mousesubx >= 0.5) ? 1 : 0) + ((this.mapBus.mousesuby >= 0.5) ? 2 : 0);
    // Important to check all points before checking any regions -- points always win a tie.
    if (this.mapBus.visibility.points) {
      let cmdp = 0;
      for (const command of this.map.commands) {
        const c = this.map.parsePointCommand(command);
        if (c && (this.mapBus.mousecol === c.x) && (this.mapBus.mouserow === c.y)) {
          if (!subp--) {
            return cmdp;
          }
        }
        cmdp++;
      }
    }
    if (this.mapBus.visibility.regions) {
      let cmdp = 0;
      for (const command of this.map.commands) {
        const c = this.map.parseRegionCommand(command);
        if (c &&
          (this.mapBus.mousecol >= c.x) &&
          (this.mapBus.mouserow >= c.y) &&
          (this.mapBus.mousecol < c.x + c.w) &&
          (this.mapBus.mouserow < c.y + c.h)
        ) {
          return cmdp;
        }
        cmdp++;
      }
    }
    return -1;
  }
   
  onBeginPaint() {
    this.toolInProgress = this.mapBus.effectiveTool;
    switch (this.mapBus.effectiveTool) {
      case "pencil": this.pencilUpdate(); break;
      case "repair": this.repairUpdate(); break;
      case "rainbow": this.pencilUpdate(); this.repairUpdate(); break;
      case "monalisa": this.monalisaBegin(); break;
      case "pickup": this.pickupBegin(); break;
      case "poimove": this.poimoveBegin(); break;
      case "poiedit": this.poieditBegin(); break;
      case "poidelete": this.poideleteBegin(); break;
    }
  }
  
  onEndPaint() {
    switch (this.toolInProgress) {
      // I don't think any of our tools will require an "end" step. But it's here if we need it.
    }
    this.toolInProgress = "";
  }
  
  onMotion() {
    switch (this.toolInProgress) {
      case "pencil": this.pencilUpdate(); break;
      case "repair": this.repairUpdate(); break;
      case "rainbow": this.pencilUpdate(); this.repairUpdate(); break;
      case "monalisa": this.monalisaUpdate(); break;
      case "poimove": this.poimoveUpdate(); break;
    }
  }
  
  onBusEvent(event) {
    if (!this.map) return;
    switch (event.type) {
      case "beginPaint": this.onBeginPaint(); break;
      case "endPaint": this.onEndPaint(); break;
      case "motion": this.onMotion(); break;
    }
  }
}

MapPainter.singleton = true;
