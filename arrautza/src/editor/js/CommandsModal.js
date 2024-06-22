/* CommandsModal.js
 * Allows editing an array of strings, plain text commands for the map.
 */
 
import { Dom } from "./Dom.js";
import { CommandModal } from "./CommandModal.js";

export class CommandsModal {
  static getDependencies() {
    return [HTMLDialogElement, Dom];
  }
  constructor(element, dom) {
    this.element = element;
    this.dom = dom;
    this.commands = [];
    this.buildUi();
  }
  
  setup(commands) {
    this.commands = commands || [];
    this.populateUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    this.dom.spawn(this.element, "TEXTAREA", { "on-keydown": e => this.onKeyDown(e), "on-click": e => this.onClick(e) });
    const buttonsRow = this.dom.spawn(this.element, "DIV", ["buttonsRow"]);
    this.dom.spawn(buttonsRow, "DIV", ["help"], "Ctl-dot or ctl-click for smart editor.");
    this.dom.spawn(buttonsRow, "DIV", ["spacer"]);
    this.dom.spawn(buttonsRow, "INPUT", { type: "button", value: "Validate", "on-click": () => this.onValidate() });
    this.dom.spawn(buttonsRow, "INPUT", { type: "button", value: "Save", "on-click": () => this.onSave() });
  }
  
  populateUi() {
    this.element.querySelector("textarea").value = this.commands.join("\n");
  }
  
  onKeyDown(e) {
    if (e.ctrlKey && (e.code === "Period")) {
      this.help(true);
    }
  }
  
  onClick(e) {
    // textarea.selectionStart should be updated already.
    if (e.ctrlKey) {
      this.help(false);
    }
  }
  
  help(clampToEnd) {
    const textarea = this.element.querySelector("textarea");
    const src = textarea.value || "";
    const cursorp = textarea.selectionStart;
    const end = clampToEnd ? src.length : (src.length - 1);
    if (isNaN(cursorp) || (cursorp < 0) || (cursorp > end)) return;
    let linestart = cursorp;
    while ((linestart > 0) && (src[linestart - 1] !== "\n")) linestart--;
    let lineend = cursorp;
    while ((lineend < src.length) && (src[lineend] !== "\n")) lineend++;
    const line = src.substring(linestart, lineend).trim();
    const modal = this.dom.spawnModal(CommandModal);
    modal.setup(line);
    modal.result.then(cmd => {
      const nsrc = src.substring(0, linestart) + cmd + src.substring(lineend);
      textarea.value = nsrc;
    }).catch(() => {});
  }
  
  onValidate() {
    console.log(`CommandsModal.onValidate TODO`);
    const text = this.element.querySelector("textarea").value;
    //TODO
  }
  
  onSave() {
    const text = this.element.querySelector("textarea").value;
    this.resolve(text.split("\n").map(v => v.trim()).filter(v => v));
  }
}
