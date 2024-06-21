/* MagicGlobals.js
 * Dynamically loads some symbols from src/arrautza.h.
 */
 
export class MagicGlobals {
  static require() {
    if (MagicGlobals.loaded > 0) return Promise.resolve();
    if (MagicGlobals.loaded < 0) return Promise.reject();
    if (MagicGlobals.pending) return MagicGlobals.pending;
    MagicGlobals.loaded = true;
    return MagicGlobals.pending = fetch("../arrautza.h").then(rsp => {
      if (!rsp.ok) throw rsp;
      return rsp.text();
    }).then(src => {
      MagicGlobals.pending = null;
      MagicGlobals.decode(src);
      MagicGlobals.loaded = 1;
    }).catch(e => {
      MagicGlobals.pending = null;
      MagicGlobals.loaded = -1;
      throw e;
    });
  }
  
  static decode(src) {
    MagicGlobals.mapcmd = [];
    for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.substring(srcp, nlp).trim();
      srcp = nlp + 1;
      if (!line) continue;
      
      //console.log(`${lineno}: ${line}`);
      
      let match;
      if (match = line.match(/^#define\s+MAPCMD_([a-zA-Z0-9_]+)\s+([0-9a-zA-Z_]+).*$/)) {
        const name = match[1];
        const opcode = +match[2];
        if (!isNaN(opcode)) {
          MagicGlobals.mapcmd.push([opcode, name]);
        }
        continue;
      }
      if (match = line.match(/^#define\s+([a-zA-Z0-9_]+)\s+([a-zA-Z0-9_]+).*$/)) {
        const k = match[1];
        const v = +match[2];
        if (!isNaN(v)) {
          MagicGlobals[k] = v;
        }
        continue;
      }
    }
    if (!MagicGlobals.COLC || !MagicGlobals.ROWC) {
      throw new Error(`Expected at least COLC and ROWC to be defined in arrautza.h`);
    }
  }
}

MagicGlobals.loaded = 0;
