import { MapEditor } from "./MapEditor.js";
import { TilesheetEditor } from "./TilesheetEditor.js";
import { SpriteEditor } from "./SpriteEditor.js";

export function selectCustomEditor(path, serial, type, qual, rid, name, format) {
  if (type === "map") return MapEditor;
  if (type === "tilesheet") return TilesheetEditor;
  if (type === "sprite") return SpriteEditor;
  return null;
}

export function listCustomEditors(path, serial, type, qual, rid, name, format) {
  return [MapEditor, TilesheetEditor, SpriteEditor];
}
