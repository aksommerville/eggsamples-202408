/* MapRes.js
 * Live representation of a map resources.
 * I'd have called it just "Map" but that's already a thing.
 */
 
import { MagicGlobals } from "./MagicGlobals.js";
 
export class MapRes {
  constructor(src, h) {
    if (!src) {
      this._initDefault();
    } else if (src instanceof MapRes) {
      this._initCopy(src);
    } else if ((typeof(src) === "number") && (typeof(h) === "number")) {
      this._initDefault(src, h);
    } else {
      if (src instanceof ArrayBuffer) src = new Uint8Array(src);
      if (src instanceof Uint8Array) src = new TextDecoder("utf8").decode(src);
      if (typeof(src) !== "string") throw new Error(`Expected string, Uint8Array, ArrayBuffer, or MapRes.`);
      if (!src) this._initDefault(); // eg could have been an empty Uint8Array
      else this._initText(src);
    }
  }
  
  _initDefault(w, h) {
    this.w = w || MagicGlobals.COLC;
    this.h = h || MagicGlobals.ROWC;
    this.v = new Uint8Array(this.w * this.h);
    this.commands = []; // Strings, one command each.
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
  
  encode() {
    let dst = "";
    for (let y=this.h, vp=0; y-->0; ) {
      for (let x=this.w; x-->0; vp++) {
        const b = this.v[vp];
        dst += "0123456789abcdef"[b >> 4];
        dst += "0123456789abcdef"[b & 15];
      }
      dst += "\n";
    }
    if (this.commands.length) {
      dst += "\n";
      for (const command of this.commands) {
        dst += command + "\n";
      }
    }
    return dst;
  }
  
  getCommandByKeyword(kw, p) {
    if (!p) p = 0;
    for (const cmd of this.commands) {
      if (!cmd.startsWith(kw)) continue;
      if ((cmd.length > kw.length) && (cmd[kw.length] !== " ")) continue;
      if (!p--) return cmd.substring(kw.length).trim();
    }
    return null;
  }
  
  /* Return {kw,x,y,w,h} or {kw,x,y} for commands containing an '@' argument.
   * Null if it doesn't match.
   */
  parseRegionCommand(cmd) {
    const match = cmd.match(/^([0-9a-zA-Z_]+)\s.*@(\d+),(\d+),(\d+),(\d+)(\s|$)/);
    if (!match) return null;
    return {
      kw: match[1],
      x: +match[2],
      y: +match[3],
      w: Math.max(1, +match[4]),
      h: Math.max(1, +match[5]),
    };
  }
  parsePointCommand(cmd) {
    const match = cmd.match(/^([0-9a-zA-Z_]+)\s.*@(\d+),(\d+)(\s|$)/);
    if (!match) return null;
    return {
      kw: match[1],
      x: +match[2],
      y: +match[3],
    };
  }
  
  /* Return a modified command with the "@X,Y" changed accordingly.
   * Returns the input verbatim if we can't.
   * Good for both regions and points.
   */
  moveCommand(cmd, dx, dy) {
    const atp = cmd.indexOf('@');
    if (atp < 0) return cmd;
    const c1p = cmd.indexOf(',', atp);
    let c2p = c1p + 1;
    while ((c2p < cmd.length) && (cmd[c2p] !== ',') && (cmd[c2p] !== ' ')) c2p++;
    let x = +cmd.substring(atp + 1, c1p);
    let y = +cmd.substring(c1p + 1, c2p);
    if (isNaN(x) || isNaN(y)) return cmd;
    x += dx;
    y += dy;
    if (x < 0) x = 0; else if (x > 0xff) x = 0xff;
    if (y < 0) y = 0; else if (y > 0xff) y = 0xff;
    // We don't clamp to the right and bottom edges. Wouldn't be too tricky to do so, but there's no technical need to.
    // Clamping against (0,0) is mandatory.
    return cmd.substring(0, atp + 1) + x + "," + y + cmd.substring(c2p);
  }
  
  replaceSingleCommand(k, v) {
    let replaced = false;
    for (let i=this.commands.length; i-->0; ) {
      if (this.commands[i].startsWith(k) && ((this.commands[i].charCodeAt(k.length) || 0) <= 0x20)) {
        if (!replaced) {
          replaced = true;
          this.commands[i] = `${k} ${v}`;
        } else {
          this.commands.splice(i, 1);
        }
      }
    }
    if (!replaced) {
      this.commands.push(`${k} ${v}`);
    }
  }
  
  removeCommands(k) {
    this.commands = this.commands.filter(cmd => (!cmd.startsWith(k) || (cmd.charCodeAt(k.length) > 0x20)));
  }
}
