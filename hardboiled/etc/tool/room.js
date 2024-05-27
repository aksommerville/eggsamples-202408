/* room.js
 * Compiles our "room" resources from text to binary.
 */
 
const fs = require("fs");

let dstpath = "";
let srcpath = "";
let hdrpath = "";
for (let i=2; i<process.argv.length; i++) {
  const arg = process.argv[i];
  if (arg.startsWith("-o")) {
    if (dstpath) throw new Error(`Multiple output paths`);
    dstpath = arg.substring(2);
  } else if (arg.startsWith("--header=")) {
    if (hdrpath) throw new Error(`Multiple header paths`);
    hdrpath = arg.substring(9);
  } else if (arg.startsWith("-")) {
    throw new Error(`Unexpected option ${JSON.stringify(arg)}`);
  } else {
    if (srcpath) throw new Error(`Multiple input paths`);
    srcpath = arg;
  }
}
if (!srcpath) throw new Error(`Please specify input path`);
if (!dstpath) throw new Error(`Please specify output path "-oPATH"`);
// hdrpath not required

// { typename: { resname: rid, ... }, ... }
const toc = {};
if (hdrpath) {
  const src = fs.readFileSync(hdrpath);
  for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
    let nextp = src.indexOf(0x0a, srcp);
    if (nextp < 0) nextp = src.length;
    else nextp++;
    const line = src.toString("utf8", srcp, nextp).trim();
    srcp = nextp;
    const match = line.match(/^#define RID_([0-9a-zA-Z]+)_([0-9a-zA-Z_]+) (\d+)$/);
    if (!match) continue;
    const tname = match[1];
    const rname = match[2];
    const rid = +match[3];
    if (!toc[tname]) toc[tname] = {};
    toc[tname][rname] = rid;
  }
}

let dst = Buffer.alloc(1024);
let dstc = 0;

function append8(src) {
  if (dstc >= dst.length) {
    const nv = Buffer.alloc(dst.length + 1024);
    nv.set(dst);
  }
  dst[dstc++] = src;
}

function append16(src) {
  append8(src >> 8);
  append8(src & 0xff);
}

function evalRid(src, tname) {
  let rid;
  if (toc[tname]) rid = toc[tname][src];
  if (!rid) rid = +src;
  if (isNaN(rid) || (rid < 0) || (rid > 0xffff)) throw new Error(`Invalid ${tname} id ${JSON.stringify(src)}`);
  return rid;
}

function singleRid(words, opcode, tname) {
  if (words.length !== 2) throw new Error(`${JSON.stringify(words[0])} takes exactly one argument (${tname} id)`);
  const rid = evalRid(words[1], tname);
  append8(opcode);
  append16(rid);
}

function noun(words) {
  /* INPUT: noun X Y W H [name=STRINGID] [go=ROOMID] [takeable=SRCX,SRCY] [wants=STRINGID prestring=STRINGID poststring=STRINGID]
   * OUTPUT:
   *   door:     0x04 u8:x u8:y u8:w u8:h u16:name u16:go
   *   takeable: 0x05 u8:dstx u8:dsty u8:w u8:h u16:name u16:srcx u16:srcy
   *   witness:  0x06 u8:x u8:y u8:w u8:h u16:name u16:prestring u16:itemstring u16:poststring
   */
  if (words.length < 5) throw new Error(`${JSON.stringify(words[0])} takes at least four arguments: X,Y,W,H`);
  
  const [x, y, w, h] = words.slice(1, 5).map(v => {
    const n = +v;
    if (isNaN(n) || (n < 0) || (n > 96)) {
      throw new Error(`Invalid layout value ${JSON.stringify(v)}, expected integer in 0..96.`);
    }
    return n;
  });
  
  const requireInt = (src, lo, hi) => {
    const dst = +src;
    if (isNaN(dst) || (dst < lo) || (dst > hi)) {
      throw new Error(`Expected integer in ${lo}..${hi}, found ${JSON.stringify(src)}`);
    }
    return dst;
  };
  
  let name = null;
  let go = null;
  let takeablex = null;
  let takeabley = null;
  let wants = null;
  let prestring = null;
  let poststring = null;
  let talk = null;
  for (let i=5; i<words.length; i++) {
    const [k, v] = words[i].split('=');
    switch (k) {
      case "name": name = evalRid(v, "string"); break;
      case "go": go = evalRid(v, "room"); break;
      case "takeable": {
          const [tx, ty] = v.split(',').map(vv => requireInt(vv, 0, 0xffff));
          takeablex = tx;
          takeabley = ty;
        } break;
      case "wants": wants = evalRid(v, "string"); break;
      case "prestring": prestring = evalRid(v, "string"); break;
      case "poststring": poststring = evalRid(v, "string"); break;
      case "talk": talk = v; break;
    }
  }
  
  const prefix = (opcode, includeName) => {
    append8(opcode);
    append8(x);
    append8(y);
    append8(w);
    append8(h);
    if (includeName) append16(name || 0);
  };
  
  if (go) {
    // Should assert no other options (takeable, wants, etc), but meh.
    prefix(0x04, true);
    append16(go);
    return;
  }
  
  if (takeablex !== null) {
    prefix(0x05, true);
    append16(takeablex);
    append16(takeabley);
    return;
  }
  
  if ((wants !== null) || (prestring !== null) || (poststring !== null)) {
    prefix(0x06, true);
    append16(prestring || 0);
    append16(wants || 0);
    append16(poststring || 0);
    return;
  }
  
  if (talk) {
    switch (talk) {
      case "gossip": prefix(0x08, true); append16(1); return;
    }
    throw new Error(`Unknown 'talk' arg ${JSON.stringify(talk)}, expected one of: 'gossip'`);
  }
  
  // "door" with a zero makes an inert noun.
  prefix(0x04, true);
  append16(0);
}

function special(words) {
  if (words.length !== 2) throw new Error(`'special' takes exactly one argument`);
  let v = 0;
  switch (words[1]) {
    case "lineup": v = 1; break;
  }
  if (!v) throw new Error(`Unknown 'special' room: ${JSON.stringify(words[1])}`);
  append8(0x07);
  append8(v);
}

const src = fs.readFileSync(srcpath);
for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
  let nextp = src.indexOf(0x0a, srcp);
  if (nextp < 0) nextp = src.length;
  else nextp++;
  const line = src.toString("utf8", srcp, nextp).split('#')[0];
  srcp = nextp;
  const words = line.split(/\s+/).filter(v => v);
  if (!words.length) continue;
  try {
    switch (words[0]) {
    
      case "image": singleRid(words, 0x01, "image"); break;
      case "song": singleRid(words, 0x02, "song"); break;
      case "exit": singleRid(words, 0x03, "room"); break;
      case "noun": noun(words); break;
      case "special": special(words); break;
    
      default: throw new Error(`Unexpected command ${JSON.stringify(words[0])}`);
    }
  } catch (e) {
    //throw e; // Uncomment to get a full stack trace (eg while tinkering with this tool)
    console.log(`${srcpath}:${lineno}: ${e.message}`);
    process.exit(1);
  }
}

const nv = Buffer.alloc(dstc);
dst.copy(nv, 0, 0, dstc);
dst = nv;

fs.writeFileSync(dstpath, dst);
