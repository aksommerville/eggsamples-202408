/**
 * Coordinates calls to `egg.draw_tile()`.
 * We gather requested tiles, which share a texture, tint, and alpha, flushing the cache as needed.
 * A convenience for text is provided, only ASCII or 8859-1 will work properly.
 */
export class TileRenderer {
  private inputTexid = 0;
  private outputTexid = 1;
  private vtxc = 0;
  private readonly VTXLIMIT = 256;
  private vtxv = new Uint8Array(this.VTXLIMIT * 6);

  constructor() {
  }
  
  setOutput(texid: number): void {
    this.outputTexid = texid;
  }
  
  cancel(): void {
    this.vtxc = 0;
    egg.draw_mode(egg.XferMode.ALPHA, 0, 0xff);
  }
  
  begin(texid: number, tint: number, alpha: number): void {
    if (this.vtxc > 0) this.flush();
    egg.draw_mode(egg.XferMode.ALPHA, tint, alpha);
    this.inputTexid = texid;
  }
  
  end(): void {
    if (this.vtxc > 0) this.flush();
    egg.draw_mode(egg.XferMode.ALPHA, 0, 0xff);
  }
  
  tile(x: number, y: number, tileid: number, xform: egg.Xform): void {
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
  
  string(x: number, y: number, text: string): void {
    for (let i=0; i<text.length; i++, x+=8) {
      this.tile(x, y, text.charCodeAt(i), 0);
    }
  }
  
  private flush(): void {
    egg.draw_tile(this.outputTexid, this.inputTexid, this.vtxv.buffer, this.vtxc);
    this.vtxc = 0;
  }
}
