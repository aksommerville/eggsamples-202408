import { MapEditor } from "./MapEditor.js";
import { TilesheetEditor } from "./TilesheetEditor.js";

export function selectCustomEditor(path, serial, type, qual, rid, name, format) {
  if (type === "map") return MapEditor;
  if (type === "tilesheet") return TilesheetEditor;
  return null;
}

export function listCustomEditors(path, serial, type, qual, rid, name, format) {
  return [MapEditor, TilesheetEditor];
}
