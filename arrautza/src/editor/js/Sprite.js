/* Sprite.js
 * Model of one "sprite" resource.
 */
 
import { MagicGlobals } from "./MagicGlobals.js";
 
export class Sprite {
  constructor(src) {
    if (!src) this._initDefault();
    else if (src instanceof Sprite) this._initCopy(src);
    else if (typeof(src) === "string") this._initText(src);
    else if (src instanceof Uint8Array) this._initText(new TextDecoder("utf8").decode(src));
    else if (src instanceof ArrayBuffer) this._initText(new TextDecoder("utf8").decode(src));
    else throw new Error(`Invalid input for Sprite`);
  }
  
  _initDefault() {
    this.commands = []; // string[], split on whitespace
  }
  
  _initCopy(src) {
    this.commands = src.commands.map(v => v);
  }
  
  _initText(src) {
    this.commands = [];
    for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.substring(srcp, nlp).split("#")[0].trim();
      srcp = nlp + 1;
      if (!line) continue;
      this.commands.push(line.split(/\s+/g));
    }
  }
  
  encode() {
    let dst = "";
    for (const command of this.commands) {
      dst += command.join(' ') + "\n";
    }
    return dst;
  }
}
