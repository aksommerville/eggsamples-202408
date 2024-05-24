/* genhdr.js
 * Receives `eggdev toc` on stdin and generates our header of resource IDs.
 */
 
const fs = require("fs");
 
let dstpath = "";
let typespath = "";
for (let i=2; i<process.argv.length; i++) {
  const arg = process.argv[i];
  if (arg.startsWith("-o")) {
    if (dstpath) throw new Error("Multiple output paths.");
    dstpath = arg.substring(2);
  } else if (arg.startsWith("--types=")) {
    if (typespath) throw new Error("Multiple types paths.");
    typespath = arg.substring(8);
  } else {
    throw new Error(`Unexpected argument ${JSON.stringify(arg)}`);
  }
}
if (!dstpath) throw new Error(`Must specify output path as '-oPATH'`);

const types = []; // {name,tid}
if (typespath) {
  const src = fs.readFileSync(typespath);
  for (let srcp=0, lineno=1; srcp<src.length; lineno++) {
    let nextp = src.indexOf(0x0a, srcp);
    if (nextp < 0) nextp = src.length;
    else nextp++;
    const line = src.toString("utf8", srcp, nextp).split('#')[0];
    srcp = nextp;
    const words = line.split(/\s+/).filter(v => v);
    if (!words.length) continue;
    const tid = +words[0];
    const name = words[1];
    types.push({tid, name});
  }
}

function composeFile(src) {
  let dst = "";
  // First define all the custom types, in the same fashion as standard egg resource types.
  for (const { name, tid } of types) {
    dst += `#define EGG_RESTYPE_${name} ${tid}\n`;
  }
  // Then RID_{type}_{name}={id} for each named resource, as reported via stdin. eg "image 00 2 font_tiles"
  for (let srcp=0; srcp<src.length; ) {
    let nextp = src.indexOf("\n", srcp);
    if (nextp < 0) nextp = src.length;
    else nextp++;
    const line = src.substring(srcp, nextp);
    srcp = nextp;
    const words = line.split(/\s+/).filter(v => v);
    if (words.length === 4) {
      const tname = words[0];
      const rid = words[2];
      const rname = words[3];
      dst += `#define RID_${tname}_${rname} ${rid}\n`;
    }
  }
  return dst;
}

let src = "";
process.stdin.on("readable", () => {
  const intake = process.stdin.read(1024);
  if (!intake) return;
  src += intake.toString("utf8");
});
process.stdin.on("end", () => {
  const dst = composeFile(src);
  fs.writeFileSync(dstpath, dst);
});
