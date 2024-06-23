/* DoorModal.js
 * Prompt for details before creating a new door.
 */
 
import { Dom } from "./Dom.js";
import { MapStore } from "./MapStore.js";

export class DoorModal {
  static getDependencies() {
    return [HTMLDialogElement, Dom, MapStore];
  }
  constructor(element, dom, mapStore) {
    this.element = element;
    this.dom = dom;
    this.mapStore = mapStore;
    
    this.buildUi();
  }
  
  setOrigin(srcrid, srcx, srcy) {
    this.element.querySelector("td[data-key='src']").innerText = `${srcrid}:${srcx},${srcy}`;
    this.element.querySelector("td[data-key='dstrid'] input").value = this.mapStore.unusedId().toString();
    this.element.querySelector("td[data-key='dstxy'] input").value = `${srcx},${srcy}`; // better than nothing
  }
  
  buildUi() {
    this.element.innerHTML = "";
    const form = this.dom.spawn(this.element, "FORM", { "on-submit": e => e.preventDefault() });
    const table = this.dom.spawn(form, "TABLE");
    this.spawnConstant(table, "From", "src");
    const dstridInput = this.spawnMutable(table, "To Map", "dstrid");
    dstridInput.classList.add("rid");
    dstridInput.addEventListener("input", () => {
      if (this.mapExists(dstridInput.value)) {
        dstridInput.classList.add("existing");
      } else {
        dstridInput.classList.remove("existing");
      }
    });
    this.spawnMutable(table, "To Position", "dstxy");
    this.spawnMutable(table, "Data 1", "data1");
    this.spawnMutable(table, "Data 2", "data2");
    this.spawnBoolean(table, "Create Remote", "remote");
    this.dom.spawn(form, "INPUT", { type: "submit", value: "OK", "on-click": e => {
      e.preventDefault();
      e.stopPropagation();
      this.onOk();
    }});
  }
  
  spawnConstant(table, label, key) {
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", ["key"], label);
    this.dom.spawn(tr, "TD", { "data-key": key });
  }
  
  spawnMutable(table, label, key) {
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", ["key"], label);
    const tdk = this.dom.spawn(tr, "TD", { "data-key": key });
    return this.dom.spawn(tdk, "INPUT", { type: "text", name: key });
  }
  
  spawnBoolean(table, label, key) {
    const tr = this.dom.spawn(table, "TR");
    this.dom.spawn(tr, "TD", ["key"], label);
    const tdk = this.dom.spawn(tr, "TD", { "data-key": key });
    return this.dom.spawn(tdk, "INPUT", { type: "checkbox", checked: "true", name: key });
  }
  
  mapExists(id) {
    return !!this.mapStore.resByWhatever(id);
  }
  
  onOk() {
    const result = {};
    for (const input of this.element.querySelectorAll("input")) {
      if (input.type === "submit") {
      } else if (input.type === "checkbox") {
        result[input.name] = input.checked;
      } else {
        result[input.name] = input.value;
      }
    }
    this.resolve(result);
  }
}
