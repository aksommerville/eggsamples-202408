import { MapEditor } from "./MapEditor.js";

export function selectCustomEditor(path, serial, type, qual, rid, name, format) {
  //console.log(`selectCustomEditor path=${path} type=${type} qual=${qual} rid=${rid} name=${name} format=${format}`);
  if (type === "map") return MapEditor;
  return null;
}
