/* MapStore.js
 * Global singleton that manages all the map resources.
 * Maps are related to each other in complex ways, eg neighbors and doors. We manage all that.
 * The map editor should not interact with Resmgr directly, should all go through MapStore.
 */
 
import { Resmgr } from "./Resmgr.js";
import { Bus } from "./Bus.js";
import { MapBus } from "./MapBus.js";
import { MapRes } from "./MapRes.js";

export class MapStore {
  static getDependencies() {
    return [Resmgr, Bus, MapBus];
  }
  constructor(resmgr, bus, mapBus) {
    this.resmgr = resmgr;
    this.bus = bus;
    this.mapBus = mapBus;
    
    this.loaded = false;
    this.planes = []; // {x,y,w,h,v:({res,map}|null)[]}, for exposing neighbor relationships. Planes will usually have a negative origin.
    this.doors = []; // {srcrid,srcx,srcy,dstrid,dstx,dsty}
    this.resv = []; // Another copy of all the resources, we need it during construction.
  }
  
  require() {
    if (this.loaded) return;
    this.loaded = true;
    this.planes = [];
    this.doors = [];
    this.maps = [];
    for (const res of this.bus.toc) {
      if (res.type !== "map") continue;
      if (!res.rid) throw new Error(`map resources must have an explicit id in 1..65535 (${JSON.stringify(res.path)})`);
      if (this.resv.indexOf(res) >= 0) continue; // Already visited via neighbor link or something.
      this.installMapOnNewPlane(res);
    }
  }
  
  mapByPath(path) {
    return this.entryByPath(path)?.map;
  }
  
  resByPath(path) {
    // We happen to have a flat list of resource objects -- more efficient than searching for live maps.
    return this.resv.find(r => r.path === path);
  }
  
  // => {res,map} or null
  entryByPath(path) {
    for (const plane of this.planes) {
      const entry = plane.v.find(e => e?.res.path === path);
      if (entry) return entry;
    }
    return null;
  }
  
  // => {plane,x,y,res,map}
  findPath(path) {
    let planeix = this.planes.length;
    while (planeix-- > 0) {
      const plane = this.planes[planeix];
      const p = plane.v.findIndex(e => e?.res.path === path);
      if (p < 0) continue;
      const { res, map } = plane.v[p];
      const x = plane.x + p % plane.w;
      const y = plane.y + Math.floor(p / plane.w);
      return { plane: planeix, x, y, res, map };
    }
    return null;
  }
  
  /* Beware that coords are transient -- adding a new map can change them.
   */
  mapByCoords(plane, x, y) {
    return this.entryByCoords(plane, x, y)?.map;
  }
  
  resByCoords(plane, x, y) {
    return this.entryByCoords(plane, x, y)?.res;
  }
  
  entryByCoords(plane, x, y) {
    if (!(plane = this.planes[plane])) return null;
    x -= plane.x;
    y -= plane.y;
    if ((x < 0) || (x >= plane.w)) return null;
    if ((y < 0) || (y >= plane.h)) return null;
    return plane.v[y * plane.w + x];
  }
  
  /* (ref) is a MapRes object to copy from. We'll automatically declare the same image, maybe other things.
   * Neighbor commands get updated for the new map, and also in its new neighbors.
   * (plane) may be one greater than the current highest, to go on a new plane. Otherwise must be in bounds.
   * The live state is fully updated synchronously, and we queue in Resmgr for the remote write.
   */
  createMap(plane, x, y, ref, basename) {
    if (!basename) basename = this.unusedId().toString();
    const path = "map/" + basename;
    if (this.resv.find(r => r.path === path)) throw new Error(`Map path ${JSON.stringify(path)} already in use.`);
    
    // Validate plane, resolve it, create it if needed.
    if ((plane < 0) || (plane > this.planes.length)) throw new Error(`Invalid plane ${plane}`); // sic > not >=
    if (plane === this.planes.length) {
      plane = { x: 0, y: 0, w: 1, h: 1, v: [null] };
      this.planes.push(plane);
    } else {
      plane = this.planes[plane];
    }
    
    // Create the new map object.
    let map;
    if (ref) {
      map = new MapRes(ref.w, ref.h);
      this.copyDefaultCommands(map, ref);
    } else {
      map = new MapRes();
    }
    
    // Tell Resmgr about the new file, and acquire the resource record.
    const res = this.resmgr.createFile(path);
    this.resmgr.dirty(path, () => map.encode());
    
    // Insert in plane and resv.
    const entry = this.addSingleMapToPlane(plane, x, y, res, map);
    this.resv.push(res);
    
    return entry;
  }
  
  unusedId() {
    // No doubt there are better ways to do this but we'll be naive:
    // Build a Set of all rid, then iterate from 1 until we find something not in the Set.
    const all = new Set();
    for (const res of this.resv) all.add(res.rid);
    for (let rid=1; ; rid++) if (!all.has(rid)) return rid;
  }
  
  /* Everything below is private.
   ************************************************************************************/
   
  copyDefaultCommands(dst, src) {
    for (const command of src.commands) {
      if (command.startsWith("image ")) {
        dst.commands.push(command);
      }
    }
  }
  
  /* Create a new plane, put this map on it, and enter its neighbors recursively.
   */
  installMapOnNewPlane(res) {
    const plane = { x: 0, y: 0, w: 0, h: 0, v: [] };
    this.planes.push(plane);
    this.addMapToPlane(plane, 0, 0, res);
  }
  
  // Grow the plane if needed, put this map there, and proceed into neighbors and doors.
  addMapToPlane(plane, x, y, res) {
    if (this.resv.indexOf(res) >= 0) return;
    if ((x >= plane.x) && (y >= plane.y) && (x < plane.x + plane.w) && (y < plane.y + plane.h)) {
      const existing = plane.v[(y - plane.y) * plane.w + x - plane.x];
      if (existing) throw new Error(`Maps ${existing.res.path} and ${res.path} ended up in the same position.`);
    }
    const map = new MapRes(res.serial);
    
    const growInterval = 4;
    if (x < plane.x) this.growPlaneLeft(plane, plane.x - x + growInterval);
    if (y < plane.y) this.growPlaneUp(plane, plane.y - y + growInterval);
    if (x >= plane.x + plane.w) this.growPlaneRight(plane, x - plane.w - plane.x + growInterval);
    if (y >= plane.x + plane.h) this.growPlaneDown(plane, y - plane.h - plane.y + growInterval);
    plane.v[(y - plane.y) * plane.w + x - plane.x] = { res, map };
    this.resv.push(res);
    
    for (const command of map.commands) {
      const words = command.split(/\s+/g).filter(v => v);
      
      if (words[0] === "door") {
        if (words.length < 4) continue;
        const [srcx, srcy] = this.parsePosition(words[1]);
        const dstrid = this.resmgr.resByString(words[2], "map")?.rid || 0;
        const [dstx, dsty] = this.parsePosition(words[3]);
        this.doors.push({ srcrid: res.rid, srcx, srcy, dstrid, dstx, dsty });
        // Just record the door. Don't enter the remote map.
        
      } else if (words[0].startsWith("neighbor")) {
        const dir = words[0][8];
        const dstres = this.resmgr.resByString(words[1], "map");
        if (!dstres) continue;
        switch (dir) {
          case "n": this.addMapToPlane(plane, x, y - 1, dstres); break;
          case "s": this.addMapToPlane(plane, x, y + 1, dstres); break;
          case "w": this.addMapToPlane(plane, x - 1, y, dstres); break;
          case "e": this.addMapToPlane(plane, x + 1, y, dstres); break;
        }
      }
    }
  }
  
  /* Grow if necessary, insert map, update neighbors (neighbor commands in map, and in its new neighbors).
   * Slot must not be occupied.
   * No recursion.
   * We do not touch (resv) or (doors).
   * We dirty neighbors if changed, but do not dirty (res) itself. Assuming you are already dirtying it.
   */
  addSingleMapToPlane(plane, x, y, res, map) {
  
    if (
      (x >= plane.x) && (x < plane.x + plane.w) &&
      (y >= plane.y) && (y < plane.y + plane.h) &&
      plane.v[(y - plane.y) * plane.w + x - plane.x]
    ) throw new Error(`New map position is already occupied.`);
  
    const growInterval = 4;
    if (x < plane.x) this.growPlaneLeft(plane, plane.x - x + growInterval);
    if (y < plane.y) this.growPlaneUp(plane, plane.y - y + growInterval);
    if (x >= plane.x + plane.w) this.growPlaneRight(plane, x - plane.w - plane.x + growInterval);
    if (y >= plane.y + plane.h) this.growPlaneDown(plane, y - plane.h - plane.y + growInterval);
    const entry = { res, map };
    plane.v[(y - plane.y) * plane.w + x - plane.x] = entry;
    
    this.updateNeighborCommands(plane, x, y);
    
    return entry;
  }
  
  updateNeighborCommands(plane, x, y) {
    const p = (y - plane.y) * plane.w + x - plane.x;
    const home = plane.v[p];
    const id = (e) => e.res.name ? `map:${e.res.name}` : `map:${e.res.rid}`;
    const chk = (dp, homek, neik) => {
      if (plane.v[p + dp]) {
        const nei = plane.v[p + dp];
        home.map.replaceSingleCommand(homek, id(nei));
        nei.map.replaceSingleCommand(neik, id(home));
        this.resmgr.dirty(nei.res.path, () => nei.map.encode());
      } else {
        home.map.removeCommands(homek);
      }
    };
    if (x > 0) chk(-1, "neighborw", "neighbore");
    if (y > 0) chk(-plane.w, "neighborn", "neighbors");
    if (x < plane.w - 1) chk(1, "neighbore", "neighborw");
    if (y < plane.h - 1) chk(plane.w, "neighbors", "neighborn");
  }
  
  /* Grow a plane in place.
   */
  growPlaneLeft(plane, addc) {
    const nw = plane.w + addc;
    const nv = [];
    for (let i=nw*plane.h; i-->0; ) nv.push(null);
    this.copyPlane(nv, nw, plane.h, plane.v, plane.w, plane.h, addc, 0);
    plane.x -= addc;
    plane.w = nw;
    plane.v = nv;
  }
  growPlaneUp(plane, addc) {
    const nh = plane.h + addc;
    const nv = [];
    for (let i=nh*plane.w; i-->0; ) nv.push(null);
    this.copyPlane(nv, plane.w, nh, plane.v, plane.w, plane.h, 0, addc);
    plane.y -= addc;
    plane.h = nh;
    plane.v = nv;
  }
  growPlaneRight(plane, addc) {
    const nw = plane.w + addc;
    const nv = [];
    for (let i=nw*plane.h; i-->0; ) nv.push(null);
    this.copyPlane(nv, nw, plane.h, plane.v, plane.w, plane.h, 0, 0);
    plane.w = nw;
    plane.v = nv;
  }
  growPlaneDown(plane, addc) {
    const nh = plane.h + addc;
    for (let yi=addc; yi-->0; ) {
      for (let xi=plane.w; xi-->0; ) {
        plane.v.push(null);
      }
    }
    plane.h = nh;
  }
  
  /* Copy from one plane.v to another.
   * (dst) must be at least as big as (src), all of (src) is copied.
   */
  copyPlane(dst, dstw, dsth, src, srcw, srch, dstx, dsty) {
    for (let dstrowp=dsty*dstw+dstx, srcrowp=0, yi=srch; yi-->0; dstrowp+=dstw, srcrowp+=srcw) {
      for (let dstp=dstrowp, srcp=srcrowp, xi=srcw; xi-->0; dstp++, srcp++) {
        dst[dstp] = src[srcp];
      }
    }
  }
}

MapStore.singleton = true;
