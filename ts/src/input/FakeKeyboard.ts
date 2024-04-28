import { Bus } from "./Bus";

const JOY_REPEAT_INITIAL = 0.333;
const JOY_REPEAT_NEXT    = 0.100;

export class FakeKeyboard {
  public enabled = false;
  private screenw = 0;
  private screenh = 0;
  private texid = 0;
  private colw = 0;
  private rowh = 0;
  private busListener = 0;
  private btnidLeft = 0;
  private btnidRight = 0;
  private btnidUp = 0;
  private btnidDown = 0;
  private btnidActivate = 0;
  private btnidBackspace = 0;
  private btnidSubmit = 0;
  private joyDx = 0;
  private joyDy = 0;
  private joyRepeatClock = 0;
  private focusPage = 0;
  private focusX = 0;
  private focusY = 0;
  private pages = [[
    "qwertyuiop \u0008",
    "asdfghjkl;'\u000a",
    "zxcvbnm,./ \u0001",
  ], [
    "QWERTYUIOP \u0008",
    "ASDFGHJKL:\"\u000a",
    "ZXCVBNM<>? \u0001",
  ], [
    "1234567890 \u0008",
    "!@#$%^&*() \u000a",
    "`~-=_+[]\\| \u0001",
  ]];
  private blotterY = 0;
  private blotterH = 0;
  private boardX = 0;
  private boardY = 0;
  private boardW = 0;
  private boardH = 0;
  private tiles: Uint8Array | null = null;
  
  constructor(private bus: Bus) {
    const fb = egg.texture_get_header(1);
    this.screenw = fb.w;
    this.screenh = fb.h;
  }
  
  /**
   * See default above, it's basically US-English.
   * Provide an array of array of strings with the codepoints to display.
   * We can only use codepoints below U+100.
   * Each string must be the same length (width) and each page must be the same length (height).
   *
   * There are two special codepoints:
   *  - U+01 Next page.
   *  - U+02 Previous page.
   */
  setKeyCaps(pages: string[][]): void {
    if (this.enabled) throw new Error(`Do not reconfigure while enabled.`);
    if (pages.length < 1) throw new Error(`Invalid keycaps.`);
    const rowc = pages[0].length;
    if (rowc < 1) throw new Error(`Invalid keycaps.`);
    const colc = pages[0][0].length;
    if (colc < 1) throw new Error(`Invalid keycaps.`);
    for (const page of pages) {
      if (page.length !== rowc) throw new Error(`Invalid keycaps.`);
      for (const row of page) {
        if (row.length !== colc) throw new Error(`Invalid keycaps.`);
      }
    }
    this.pages = pages;
  }
  
  setFont(texid: number): void {
    if (this.enabled) throw new Error(`Do not reconfigure while enabled.`);
    if (this.texid = texid) {
      const hdr = egg.texture_get_header(this.texid);
      this.colw = (hdr.w >> 4) * 2;
      this.rowh = (hdr.h >> 4) * 2;
    }
  }
  
  setButtonNames(
    names: string[],
    standardNames?: string[]
  ): void {
    const btn = (p) => {
      if (!standardNames) return 0;
      const name = standardNames[p];
      if (!name) return 0;
      const ix = names.indexOf(name);
      if (ix < 0) return 0;
      return 1 << ix;
    };
    this.btnidLeft = btn(14);
    this.btnidRight = btn(15);
    this.btnidUp = btn(12);
    this.btnidDown = btn(13);
    this.btnidActivate = btn(0);
    this.btnidBackspace = btn(2);
    this.btnidSubmit = btn(9);
  }
  
  canOperate(): boolean {
    if (!this.texid) return false;
    return true;
  }
  
  enable(enable: boolean): void {
    if (enable) {
      if (this.enabled) return;
      this.enabled = true;
      egg.show_cursor(true);
      egg.event_enable(egg.EventType.TEXT, egg.EventState.ENABLED);
      egg.event_enable(egg.EventType.MMOTION, egg.EventState.ENABLED);
      egg.event_enable(egg.EventType.MBUTTON, egg.EventState.ENABLED);
      egg.event_enable(egg.EventType.TOUCH, egg.EventState.ENABLED);
      if (!this.busListener) {
        this.busListener = this.bus.listen(
          (1 << egg.EventType.MMOTION) |
          (1 << egg.EventType.MBUTTON) |
          (1 << egg.EventType.TOUCH) |
        0, e => this.onBusEvent(e));
      }
      this.bus.joyLogical.enablePrivate(false, (b, v) => this.onJoyEvent(b, v));
      this.refreshMeasurements();
    } else {
      if (!this.enabled) return;
      this.enabled = false;
      egg.event_enable(egg.EventType.TEXT, egg.EventState.DISABLED);
      egg.event_enable(egg.EventType.MMOTION, egg.EventState.DISABLED);
      egg.event_enable(egg.EventType.MBUTTON, egg.EventState.DISABLED);
      egg.event_enable(egg.EventType.TOUCH, egg.EventState.DISABLED);
      this.bus.unlisten(this.busListener);
      this.busListener = 0;
      this.bus.joyLogical.enable(false);
    }
  }
  
  update(elapsed: number): void {
    if (this.joyDx || this.joyDy) {
      if ((this.joyRepeatClock -= elapsed) <= 0) {
        this.joyRepeatClock = JOY_REPEAT_NEXT;
        this.moveFocus(this.joyDx, this.joyDy);
      }
    }
  }
  
  render(): void {
    if (!this.enabled) return;
    egg.draw_rect(1, 0, this.blotterY, this.screenw, this.blotterH, 0x000000c0);
    if ((this.focusX >= 0) && (this.focusY >= 0)) {
      egg.draw_rect(1, this.boardX + this.focusX * this.colw - 1, this.boardY + this.focusY * this.rowh - 1, this.colw + 1, this.rowh + 1, 0xffff00ff);
    }
    const halfcolw = this.colw >> 1;
    const halfrowh = this.rowh >> 1;
    const colc = this.pages[0][0].length;
    const rowc = this.pages[0].length;
    const tileCount = colc * rowc;
    if (!this.tiles || (tileCount * 6 > this.tiles.length)) {
      this.tiles = new Uint8Array(tileCount * 6);
    }
    let tilep = 0;
    for (let y=this.boardY, row=0, ty=this.boardY+halfrowh; row<rowc; y+=this.rowh, row++, ty+=this.rowh) {
      const textRow = this.pages[this.focusPage][row];
      for (let x=this.boardX, col=0, tx=this.boardX+halfcolw; col<colc; x+=this.colw, col++, tx+=this.colw) {
        egg.draw_rect(1, x, y, this.colw - 1, this.rowh - 1, 0xe0e0e0c0);
        this.tiles[tilep++] = tx;
        this.tiles[tilep++] = tx >> 8;
        this.tiles[tilep++] = ty;
        this.tiles[tilep++] = ty >> 8;
        this.tiles[tilep++] = textRow.charCodeAt(col);
        this.tiles[tilep++] = 0;
      }
    }
    egg.draw_mode(egg.XferMode.ALPHA, 0x000000ff, 0xff);
    egg.draw_tile(1, this.texid, this.tiles.buffer, tileCount);
    egg.draw_mode(egg.XferMode.ALPHA, 0, 0xff);
  }
  
  private refreshMeasurements(): void {
    const rowc = this.pages[0].length;
    const colc = this.pages[0][0].length;
    this.boardW = this.colw * colc;
    this.boardH = this.rowh * rowc;
    this.boardX = (this.screenw >> 1) - (this.boardW >> 1);
    this.boardY = this.screenh - 5 - this.boardH;
    this.blotterH = this.boardH + 10;
    this.blotterY = this.screenh - this.blotterH;
  }
  
  private onBusEvent(event: egg.Event): void {
    switch (event.eventType) {
      case egg.EventType.MMOTION: this.focusAbsolute(event.v0, event.v1); break;
      case egg.EventType.MBUTTON: if ((event.v0 === 1) && event.v1) { this.focusAbsolute(event.v2, event.v3); this.activate(); } break;
      case egg.EventType.TOUCH: if (event.v1 === 1) { this.focusAbsolute(event.v2, event.v3); this.activate(); } break;
    }
  }
  
  private onJoyEvent(btnid: number, value: number): void {
         if (btnid === this.btnidLeft ) { this.joyRepeatClock = JOY_REPEAT_INITIAL; if (value) { this.moveFocus(-1, 0); this.joyDx = -1; } else if (this.joyDx < 0) this.joyDx = 0; }
    else if (btnid === this.btnidRight) { this.joyRepeatClock = JOY_REPEAT_INITIAL; if (value) { this.moveFocus( 1, 0); this.joyDx =  1; } else if (this.joyDx > 0) this.joyDx = 0; }
    else if (btnid === this.btnidUp   ) { this.joyRepeatClock = JOY_REPEAT_INITIAL; if (value) { this.moveFocus( 0,-1); this.joyDy = -1; } else if (this.joyDy < 0) this.joyDy = 0; }
    else if (btnid === this.btnidDown ) { this.joyRepeatClock = JOY_REPEAT_INITIAL; if (value) { this.moveFocus( 0, 1); this.joyDy =  1; } else if (this.joyDy > 0) this.joyDy = 0; }
    else if (btnid === this.btnidActivate) { if (value) this.activate(); }
    else if (btnid === this.btnidBackspace) { if (value) this.sendCodepoint(0x08); }
    else if (btnid === this.btnidSubmit) { if (value) this.sendCodepoint(0x0a); }
  }
  
  private focusAbsolute(x: number, y: number): void {
    this.focusX = Math.floor((x - this.boardX) / this.colw);
    if ((this.focusX < 0) || (this.focusX >= this.pages[0][0].length)) this.focusX = -1;
    this.focusY = Math.floor((y - this.boardY) / this.rowh);
    if ((this.focusY < 0) || (this.focusY >= this.pages[0].length)) this.focusY = -1;
  }
  
  private moveFocus(dx: number, dy: number): void {
    if (dx) {
      const colc = this.pages[0][0].length;
      this.focusX += dx;
      if (this.focusX >= colc) this.focusX = 0;
      else if (this.focusX < 0) this.focusX = colc - 1;
    }
    if (dy) {
      const rowc = this.pages[0].length;
      this.focusY += dy;
      if (this.focusY >= rowc) this.focusY = 0;
      else if (this.focusY < 0) this.focusY = rowc - 1;
    }
  }
  
  private activate(): void {
    if ((this.focusX >= 0) && (this.focusY >= 0)) {
      this.sendCodepoint(this.pages[this.focusPage][this.focusY].charCodeAt(this.focusX));
    }
  }
  
  private sendCodepoint(codepoint: number): void {
    if (codepoint === 0x01) {
      this.focusPage++;
      if (this.focusPage >= this.pages.length) this.focusPage = 0;
    } else if (codepoint === 0x02) {
      this.focusPage--;
      if (this.focusPage < 0) this.focusPage = this.pages.length - 1;
    } else {
      this.bus.onEvent({ eventType: egg.EventType.TEXT, v0: codepoint, v1: 0, v2: 0, v3: 0});
    }
  }
}
