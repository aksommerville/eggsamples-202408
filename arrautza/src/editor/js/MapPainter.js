/* MapPainter.js
 * Receives events and state from MapBus, applies painting operations to the map.
 */
 
import { MapBus } from "./MapBus.js";
import { Dom } from "./Dom.js"; // For spawning modals only.
import { CommandModal } from "./CommandModal.js";
import { MapStore } from "./MapStore.js";
import { Resmgr } from "./Resmgr.js";
import { DoorModal } from "./DoorModal.js";
import { Bus } from "./Bus.js";
 
export class MapPainter {
  static getDependencies() {
    return [MapBus, Dom, MapStore, Resmgr, Bus];
  }
  constructor(mapBus, dom, mapStore, resmgr, bus) {
    this.mapBus = mapBus;
    this.dom = dom;
    this.mapStore = mapStore;
    this.resmgr = resmgr;
    this.bus = bus;
    
    this.map = null;
    this.toolInProgress = "";
    this.anchorx = 0; // monalisa
    this.anchory = 0;
    this.pvx = 0;
    this.pvy = 0;
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
   
  popcnt8(src) {
    let c = 0;
    for (let m=0x80; m; m>>=1) {
      if (src & m) c++;
    }
    return c;
  }
   
  calculateNeighborMask(x0, y0, family) {
    let mask = 0;
    const cellv = this.mapBus.loc.map.v;
    for (let suby=0, bit=0x80; suby<3; suby++) {
      let y = y0 + suby;
      if (y < 0) y = 0;
      else if (y >= this.mapBus.loc.map.h) y = this.mapBus.loc.map.h - 1;
      for (let subx=0; subx<3; subx++) {
        if ((suby === 1) && (subx === 1)) continue;
        let x = x0 + subx;
        if (x < 0) x = 0;
        else if (x >= this.mapBus.loc.map.w) x = this.mapBus.loc.map.w - 1;
        const tileid = cellv[y * this.mapBus.loc.map.w + x];
        if (this.mapBus.tstables.family[tileid] === family) {
          mask |= bit;
        }
        bit >>= 1;
      }
    }
    return mask;
  }
   
  repairUpdate1(p, x, y) {
    if ((x < 0) || (x >= this.mapBus.loc.map.w)) return false;
    if ((y < 0) || (y >= this.mapBus.loc.map.h)) return false;
    const tileid0 = this.mapBus.loc.map.v[p];
    const family = this.mapBus.tstables.family[tileid0];
    if (!family) return false;
    
    const neiv = this.mapBus.tstables.neighbors;
    if (neiv) {
      const mask = this.calculateNeighborMask(x - 1, y - 1, family);
      const candidates = [];
      let totalWeight = 0;
      let bitCount = 0;
      for (let qtileid=0; qtileid<256; qtileid++) { // TODO Lots of iteration! Is it feasible to pre-index by family?
        if (this.mapBus.tstables.family[qtileid] !== family) continue; // wrong family
        if (this.mapBus.tstables.neighbors[qtileid] & ~mask) continue; // expects a neighbor we don't have
        let weight = this.mapBus.tstables.weight[qtileid] || 0;
        if (weight === 0xff) continue; // appointment only
        if (!weight) weight = 0xff; // must be nonzero, and the default zero should be "likeliest"
        const bc = this.popcnt8(this.mapBus.tstables.neighbors[qtileid]);
        if (bc < bitCount) continue;
        if (bc > bitCount) {
          candidates.splice(0, candidates.length);
          totalWeight = 0;
          bitCount = bc;
        }
        candidates.push(qtileid);
        totalWeight += weight;
      }
      if (!candidates.length) return false;
      if (candidates.length > 2) {
        let choice = Math.floor(Math.random() * totalWeight);
        let tileid = tileid0;
        for (let i=0; i<candidates.length; i++) {
          choice -= this.mapBus.tstables.weight[candidates[i]] || 0xff;
          if (choice < 0) { tileid = candidates[i]; break; }
        }
        candidates[0] = tileid;
      }
      if (candidates[0] === tileid0) return false;
      this.mapBus.loc.map.v[p] = candidates[0];
      return true;
    }
    
    return false;
  }
   
  repairUpdate(wide) {
    const p = this.mousePosition1d();
    if (p < 0) return;
    let changed = false;
    if (wide) {
      for (let dy=-1; dy<=1; dy++) {
        for (let dx=-1; dx<=1; dx++) {
          if (this.repairUpdate1(p + dy * this.mapBus.loc.map.w + dx, this.mapBus.mousecol + dx, this.mapBus.mouserow + dy)) changed = true;
        }
      }
    } else {
      changed = this.repairUpdate1(p, this.mapBus.mousecol, this.mapBus.mouserow);
    }
    if (changed) this.mapBus.dirty();
  }
   
  repairBegin(wide) {
    if (!this.mapBus.tstables.family) {
      // Can't auto-anything without some family sets.
      this.toolInProgress = "";
      return;
    }
    this.repairUpdate(wide);
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
    if (this.poimoveIsEntrance) {
      const door = this.mapBus.entrances[this.poimoveIndex]; // Same object in MapCanvasUi and MapStore.
      door.dstx += dx;
      door.dsty += dy;
      if (door.dstx < 0) door.dstx = 0;
      if (door.dsty < 0) door.dsty = 0;
      const srcentry = this.mapStore.entryByRid(door.srcrid);
      if (srcentry) { // Must exist, i mean, how did we know about the entrance? But let's be safe.
        srcentry.map.updateDoorExit(door.srcx, door.srcy, door.dstx, door.dsty);
        this.resmgr.dirty(srcentry.res.path, () => srcentry.map.encode());
      }
      this.mapBus.broadcast({ type: "render" });
    } else {
      const ocmd = this.map.commands[this.poimoveIndex];
      const ncmd = this.map.moveCommand(ocmd, dx, dy);
      this.map.commands[this.poimoveIndex] = ncmd;
      this.mapBus.dirty();
      this.mapBus.commandsChanged();
    }
  }
   
  poimoveBegin() {
    this.poimoveIsEntrance = false;
    let p = this.findPoi();
    if ((p >= 0) && (p < this.map.commands.length)) {
      this.poimoveIndex = p;
      this.poimovePvx = this.mapBus.mousecol;
      this.poimovePvy = this.mapBus.mouserow;
      return;
    }
    p = this.findEntrance();
    if ((p >= 0) && (p < this.mapBus.entrances.length)) {
      this.poimoveIsEntrance = true;
      this.poimoveIndex = p;
      this.poimovePvx = this.mapBus.mousecol;
      this.poimovePvy = this.mapBus.mouserow;
      return;
    }
    this.toolInProgress = "";
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
  
  /* door: Travel through door commands or entrances, or create a new door.
   ********************************************************/
   
  doorBegin() {
    this.toolInProgress = "";
    // First check entrances. There could be more than one for a cell, and we want to get the correct one.
    const p = this.findEntrance();
    if ((p >= 0) && (p < this.mapBus.entrances.length)) {
      this.mapBus.broadcast({ type: "open", rid: this.mapBus.entrances[p].srcrid });
      return;
    }
    // Next check global doors, both (src) and (dst). We don't care about the sub-position anymore.
    for (const door of this.mapStore.doors) {
      if (
        (door.srcrid === this.mapBus.loc.res.rid) &&
        (door.srcx === this.mapBus.mousecol) &&
        (door.srcy === this.mapBus.mouserow)
      ) {
        this.mapBus.broadcast({ type: "open", rid: door.dstrid });
        return;
      }
      if (
        (door.dstrid === this.mapBus.loc.res.rid) &&
        (door.dstx === this.mapBus.mousecol) &&
        (door.dsty === this.mapBus.mouserow)
      ) {
        this.mapBus.broadcast({ type: "open", rid: door.srcrid });
        return;
      }
    }
    // And finally, let's figure they are asking to create a new door.
    const srcx = this.mapBus.mousecol;
    const srcy = this.mapBus.mouserow;
    if ((srcx < 0) || (srcx >= this.mapBus.loc.map.w)) return;
    if ((srcy < 0) || (srcy >= this.mapBus.loc.map.h)) return;
    const modal = this.dom.spawnModal(DoorModal);
    modal.setOrigin(this.mapBus.loc.res.rid, srcx, srcy);
    modal.result.then(rsp => {
      this.createDoor(rsp, srcx, srcy);
    }).catch(e => { if (e) this.bus.broadcast({ type: "error", e }) });
  }
  
  /* (rsp) comes from DoorModal:
   *   dstrid: string; Could be a name, or empty, or whatever. Might exist.
   *   dstxy: "X,Y"
   *   data1: string
   *   data2: string
   *   remote: boolean
   */
  createDoor(rsp, srcx, srcy) {
    let entry;
    let res = this.mapStore.resByWhatever(rsp.dstrid);
    if (res) {
      entry = this.mapStore.entryByPath(res.path);
    } else {
      entry = this.mapStore.createMap(this.mapStore.planes.length, 0, 0, null, rsp.dstrid);
    }
    if (!entry) return;
    const srcrid = this.mapBus.loc.res.rid;
    const dstrid = entry.res.rid;
    const [dstx, dsty] = rsp.dstxy.split(',').map(v => +v || 0);
    // Generate a new command on the current map, and a door on MapStore:
    this.mapStore.doors.push({ srcrid, srcx, srcy, dstrid, dstx, dsty });
    this.mapBus.loc.map.commands.push(`door @${srcx},${srcy} map:${dstrid} @${dstx},${dsty} ${rsp.data1 || '0'} ${rsp.data2 || '0'}`);
    this.mapBus.dirty();
    this.mapBus.commandsChanged();
    // If requested, generate a command on the remote map too:
    if (rsp.remote) {
      this.mapStore.doors.push({ srcrid: dstrid, srcx: dstx, srcy: dsty, dstrid: srcrid, dstx: srcx, dsty: srcy });
      entry.map.commands.push(`door @${dstx},${dsty} map:${srcrid} @${srcx},${srcy} 0 0`);
      this.resmgr.dirty(entry.res.path, () => entry.map.encode());
      this.mapBus.broadcast({ type: "remoteDoorsChanged" });
    }
  }
  
  /* Find things.
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
  
  // Same idea as findPoi(), but search for 'dst' in mapBus.entrances.
  findEntrance() {
    if (!this.mapBus.visibility.points) return -1;
    if ((this.mapBus.mousecol < 0) || (this.mapBus.mousecol >= this.map.w)) return -1;
    if ((this.mapBus.mouserow < 0) || (this.mapBus.mouserow >= this.map.h)) return -1;
    let subp = ((this.mapBus.mousesubx >= 0.5) ? 1 : 0) + ((this.mapBus.mousesuby >= 0.5) ? 2 : 0);
    let p = 0;
    for (const command of this.map.commands) { // Must walk POIs too, to keep (subp) fresh.
      const c = this.map.parsePointCommand(command);
      if (c && (this.mapBus.mousecol === c.x) && (this.mapBus.mouserow === c.y)) {
        if (!subp--) return -1;
      }
    }
    for (const entrance of this.mapBus.entrances) {
      if ((entrance.dstx === this.mapBus.mousecol) && (entrance.dsty === this.mapBus.mouserow)) {
        if (!subp--) return p;
      }
      p++;
    }
    return -1;
  }
  
  /* General event dispatch.
   *****************************************************/
   
  onBeginPaint() {
    this.pvx = this.mapBus.mousecol;
    this.pvy = this.mapBus.mouserow;
    this.toolInProgress = this.mapBus.effectiveTool;
    switch (this.mapBus.effectiveTool) {
      case "pencil": this.pencilUpdate(); break;
      case "repair": this.repairBegin(false); break;
      case "rainbow": this.pencilUpdate(); this.repairBegin(true); break;
      case "monalisa": this.monalisaBegin(); break;
      case "pickup": this.pickupBegin(); break;
      case "poimove": this.poimoveBegin(); break;
      case "poiedit": this.poieditBegin(); break;
      case "poidelete": this.poideleteBegin(); break;
      case "door": this.doorBegin(); break;
    }
  }
  
  onEndPaint() {
    this.pvx = -1;
    this.pvy = -1;
    switch (this.toolInProgress) {
      // I don't think any of our tools will require an "end" step. But it's here if we need it.
    }
    this.toolInProgress = "";
  }
  
  onMotion() {
    if ((this.mapBus.mousecol === this.pvx) && (this.mapBus.mouserow === this.pvy)) return;
    this.pvx = this.mapBus.mousecol;
    this.pvy = this.mapBus.mouserow;
    switch (this.toolInProgress) {
      case "pencil": this.pencilUpdate(); break;
      case "repair": this.repairUpdate(false); break;
      case "rainbow": this.pencilUpdate(); this.repairUpdate(true); break;
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
