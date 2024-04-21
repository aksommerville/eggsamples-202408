export class TileRenderer {

  constructor() {
    this.texid = 0;
    this.vtxc = 0;
    this.VTXLIMIT = 256;
    this.vtxv = new Uint8Array(this.VTXLIMIT * 6);
  }
  
  begin(texid, tint, alpha) {
    if (this.vtxc > 0) this.flush();
    egg.draw_mode(0, tint, alpha);
    this.texid = texid;
  }
  
  end() {
    if (this.vtxc > 0) this.flush();
  }
  
  tile(x, y, tileid, xform) {
    if (this.vtxc >= this.VTXLIMIT) this.flush();
    let p = this.vtxc * 6;
    this.vtxv[p++] = x;
    this.vtxv[p++] = x >> 8;
    this.vtxv[p++] = y;
    this.vtxv[p++] = y >> 8;
    this.vtxv[p++] = tileid;
    this.vtxv[p++] = xform;
    this.vtxc++;
  }
  
  string(x, y, text) {
    for (let i=0; i<text.length; i++, x+=8) {
      this.tile(x, y, text.charCodeAt(i), 0);
    }
  }
  
  flush() {
    egg.draw_tile(1, this.texid, this.vtxv.buffer, this.vtxc);
    this.vtxc = 0;
  }
}
