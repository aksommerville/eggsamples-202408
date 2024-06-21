/* MapRes.js
 * Live representation of a map resources.
 * I'd have called it just "Map" but that's already a thing.
 */
 
import { MagicGlobals } from "./MagicGlobals.js";
 
export class MapRes {
  constructor(src) {
    if (!src) {
      this._initDefault();
    } else if (src instanceof MapRes) {
      this._initCopy(src);
    } else {
      if (src instanceof ArrayBuffer) src = new Uint8Array(src);
      if (src instanceof Uint8Array) src = new TextDecoder("utf8").decode(src);
      if (typeof(src) !== "string") throw new Error(`Expected string, Uint8Array, ArrayBuffer, or MapRes.`);
      if (!src) this._initDefault(); // eg could have been an empty Uint8Array
      else this._initText(src);
    }
  }
  
  _initDefault() {
    MagicGlobals.require().then(() => {
      this.w = MagicGlobals.COLC;
      this.h = MagicGlobals.ROWC;
      this.v = new Uint8Array(this.w * this.h);
      this.commands = []; // Strings, one command each.
    }).catch(e => console.error(e));
  }
  
  _initCopy(src) {
    this.w = src.w;
    this.h = src.h;
    this.v = new Uint8Array(this.w * this.h);
    this.v.set(src.v);
    this.commands = src.commands.map(v => v);
  }
  
  _initText(src) {
    this.w = 0;
    this.h = 0;
    this.commands = [];
    const tmpv = [];
    let readingImage = true;
    for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.substring(srcp, nlp).split('#')[0].trim();
      srcp = nlp + 1;
      
      if (readingImage) {
        if (!line) {
          if ((this.w < 1) || (this.h < 1)) throw new Error("Map must start with cells image.");
          readingImage = false;
        } else {
          if (!this.w) {
            if ((line.length < 2) || (line.length & 1)) {
              throw new Error(`Invalid length ${line.length} for first line of map. Must be even and >=2.`);
            }
            this.w = line.length >> 1;
          } else if (line.length !== this.w << 1) {
            throw new Error(`${lineno}: Invalid length ${line.length} in map image, expected ${this.w << 1}`);
          }
          for (let linep=0; linep<line.length; linep+=2) {
            const sub = line.substring(linep, linep + 2);
            const v = parseInt(sub, 16);
            if (isNaN(v)) throw new Error(`${lineno}: Unexpected characters '${sub}' in map image.`);
            tmpv.push(v);
          }
          this.h++;
        }
        continue;
      }
      
      if (!line) continue;
      this.commands.push(line);
    }
    if ((this.w < 1) || (this.h < 1)) throw new Error("Map must start with cells image.");
    this.v = new Uint8Array(tmpv);
  }
}
