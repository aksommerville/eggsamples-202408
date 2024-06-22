/* CommandModal.js
 * Modal for a single map command.
 */
 
import { Dom } from "./Dom.js";
import { MapRes } from "./MapRes.js";
import { MagicGlobals } from "./MagicGlobals.js";
import { Resmgr } from "./Resmgr.js";

export class CommandModal {
  static getDependencies() {
    return [HTMLDialogElement, Dom, Resmgr, "nonce"];
  }
  constructor(element, dom, resmgr, nonce) {
    this.element = element;
    this.dom = dom;
    this.resmgr = resmgr;
    this.nonce = nonce;
    
    this.cmd = "";
    
    this.buildUi();
  }
  
  setup(cmd) {
    this.cmd = cmd;
    this.populateUi();
  }
  
  buildUi() {
    this.element.innerText = "";
    this.dom.spawn(this.element, "PRE", ["verbatim"]);
    this.dom.spawn(this.element, "PRE", ["preview"]);
    const form = this.dom.spawn(this.element, "FORM", { "on-input": e => this.onInput(e), action: "#", "on-submit": e => e.preventDefault() });
    this.dom.spawn(form, "TABLE");
    this.dom.spawn(form, "INPUT", { type: "submit", value: "Save", "on-click": e => {
      e.preventDefault();
      e.stopPropagation();
      this.resolve(this.recomposeCommand());
    } });
  }
  
  populateUi() {
    this.element.querySelector(".verbatim").innerText = this.cmd;
    this.element.querySelector(".preview").innerText = this.generatePreview(this.cmd);
    const table = this.element.querySelector("table");
    table.innerHTML = "";
    const words = this.cmd.split(/\s+/g).filter(v => v);
    this.spawnOpcode(table, words[0] || "");
    const argNames = this.argumentNamesForOpcode(words[0]);
    for (let i=1; i<words.length; i++) {
      this.spawnArgument(table, words[i], words[0], argNames[i] || "!!!");
    }
    for (let i=words.length; i<argNames.length; i++) {
      this.spawnArgument(table, "", words[0], argNames[i]);
    }
  }
  
  spawnOpcode(table, opcode) {
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", ["key"], "Opcode");
    const tdv = this.dom.spawn(tr, "TD", ["value"]);
    const dlid = `CommandModal-${this.nonce}-opcodes`;
    const datalist = this.dom.spawn(tdv, "DATALIST", { id: dlid });
    for (const [opcode, name] of MagicGlobals.mapcmd) {
      this.dom.spawn(datalist, "OPTION", { value: name }, name);
    }
    const input = this.dom.spawn(tdv, "INPUT", { type: "text", value: opcode, list: dlid });
    input.focus();
  }
  
  spawnArgument(table, src, opcode, name) {
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", ["key"], name);
    const tdv = this.dom.spawn(tr, "TD", ["value"]);
    //TODO Different input methods for '@' and resid
    const input = this.dom.spawn(tdv, "INPUT", { type: "text", value: src });
  }
  
  generatePreview(cmd) {
    const words = cmd.split(/\s+/g).filter(v => v);
    if (!words.length) return "";
    let dst = "";
    const opcode = this.evalOpcode(words[0]);
    dst += "0123456789abcdef"[opcode >> 4];
    dst += "0123456789abcdef"[opcode & 15];
    for (let i=1; i<words.length; i++) {
      for (const b of this.evalArgument(words[i])) {
        dst += " ";
        dst += "0123456789abcdef"[b >> 4];
        dst += "0123456789abcdef"[b & 15];
      }
    }
    return dst;
  }
  
  evalOpcode(src) {
    let v = +src;
    if (!isNaN(v)) return v;
    for (const [opcode, name] of MagicGlobals.mapcmd) {
      if (name === src) return opcode;
    }
    return 0;
  }
  
  evalArgument(src) {
    
    // Plain integers.
    let v;
    if (!isNaN(v = +src)) return [v];
    if (src.endsWith("u16")) {
      v = +src.substring(0, src.length - 3) || 0;
      return [v >> 8, v & 0xff];
    }
    if (src.endsWith("u24")) {
      v = +src.substring(0, src.length - 3) || 0;
      return [v >> 16, (v >> 8) & 0xff, v & 0xff];
    }
    if (src.endsWith("u32")) {
      v = +src.substring(0, src.length - 3) || 0;
      return [v >> 24, (v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff];
    }
    
    // "@X,Y" or "@X,Y,W,H" -- 1 byte each in both cases.
    if (src[0] === "@") {
      return src.substring(1).split(',').map(v => +v || 0);
    }
    
    // Quoted strings bytewise. Double-quote only, and UTF-8.
    // Our string format is close enough to JSON that hopefully we can get away with JSON here.
    if ((src.length >= 2) && src.startsWith('"') && src.endsWith('"')) {
      try {
        const e = JSON.parse(src);
        return new TextEncoder("utf8").encode(e);
      } catch (error) { return []; }
    }
    
    // "TYPE:NAME" for 2-byte rid.
    const colonp = src.indexOf(":");
    if (colonp >= 0) {
      const res = this.resmgr.resByString(src);
      if (!res) return [0, 0];
      return [res.rid >> 8, res.rid & 0xff];
    }
    
    // Unknown.
    return [];
  }
  
  /* Returns array of strings, with a blank in the beginning.
   * (so indices in this array match your split command).
   */
  argumentNamesForOpcode(opcode) {
    const numeric = +opcode;
    if (!isNaN(numeric)) {
      for (const [b, name, args] of MagicGlobals.mapcmd) {
        if (b !== numeric) continue;
        return ["", ...args.map(v => v.split(":")[1] || v)];
      }
      // Not defined, assume each argument is one byte.
      const args = [""];
      if (opcode >= 0xe0) return []; // Complex or custom length, we can't work with it.
      if (opcode >= 0x20) { args.push("[0]"); args.push("[1]"); }
      if (opcode >= 0x40) { args.push("[2]"); args.push("[3]"); }
      if (opcode >= 0x60) { args.push("[4]"); args.push("[5]"); }
      if (opcode >= 0x80) { args.push("[6]"); args.push("[7]"); }
      if (opcode >= 0xa0) { args.push("[8]"); args.push("[9]"); args.push("[10]"); args.push("[11]"); }
      if (opcode >= 0xc0) { args.push("[12]"); args.push("[13]"); args.push("[14]"); args.push("[15]"); }
      return args;
    } else {
      for (const [b, name, args] of MagicGlobals.mapcmd) {
        if (name !== opcode) continue;
        return ["", ...args.map(v => v.split(":")[1] || v)];
      }
    }
    // Finally, for named or invalid commands, we can't guess it, just assume none.
    return [];
  }
  
  recomposeCommand() {
    return Array.from(this.element.querySelectorAll("table input")).map(e => e.value).join(' ');
  }
  
  onInput(event) {
    const cmd = this.recomposeCommand();
    this.element.querySelector(".verbatim").innerText = cmd;
    this.element.querySelector(".preview").innerText = this.generatePreview(cmd);
  }
}
