import { Bus } from "./Bus";

export class FakePointer {
  public enabled = false;
  private texid = 0;
  private srcx = 0;
  private srcy = 0;
  private srcw = 0;
  private srch = 0;
  private hotx = 0;
  private hoty = 0;
  private x = 0;
  private y = 0;
  private button = false;
  private busListener = 0;
  private btnidLeft = 0;
  private btnidRight = 0;
  private btnidUp = 0;
  private btnidDown = 0;
  private btnidButton = 0;
  private joyDx = 0;
  private joyDy = 0;
  private joySpeed = 0;
  private screenw = 0;
  private screenh = 0;
  
  constructor(private bus: Bus) {
    const fb = egg.texture_get_header(1);
    this.screenw = fb.w;
    this.screenh = fb.h;
    this.joySpeed = this.screenw / 2;
  }
  
  setCursor(
    texid: number,
    x: number, y: number, w: number, h: number,
    hotX: number, hotY: number
  ): void {
    if (this.texid = texid) {
      this.srcx = x;
      this.srcy = y
      this.srcw = w;
      this.srch = h;
      this.hotx = hotX;
      this.hoty = hotY;
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
    this.btnidButton = btn(0);
  }
  
  canOperate(): boolean {
    if (this.texid) return true;
    if (egg.event_enable(egg.EventType.MMOTION, egg.EventState.QUERY) !== egg.EventState.IMPOSSIBLE) return true;
    if (egg.event_enable(egg.EventType.TOUCH, egg.EventState.QUERY) !== egg.EventState.IMPOSSIBLE) return true;
    return false;
  }
  
  enable(enable: boolean): void {
    if (enable) {
      if (this.enabled) return;
      this.enabled = true;
      egg.show_cursor(!this.texid);
      egg.event_enable(egg.EventType.MMOTION, egg.EventState.ENABLED);
      egg.event_enable(egg.EventType.MBUTTON, egg.EventState.ENABLED);
      egg.event_enable(egg.EventType.MWHEEL, egg.EventState.ENABLED);
      egg.event_enable(egg.EventType.TOUCH, egg.EventState.ENABLED);
      this.busListener = this.bus.listen(
        (1 << egg.EventType.MMOTION) |
        (1 << egg.EventType.MBUTTON) |
        (1 << egg.EventType.MWHEEL) |
        (1 << egg.EventType.TOUCH) |
      0, e => this.onBusEvent(e));
      this.bus.joyLogical.enablePrivate(true, (b, v) => this.onJoyEvent(b, v));
      this.joyDx = 0;
      this.joyDy = 0;
    } else {
      if (!this.enabled) return;
      this.enabled = true;
      egg.event_enable(egg.EventType.MMOTION, egg.EventState.DISABLED);
      egg.event_enable(egg.EventType.MBUTTON, egg.EventState.DISABLED);
      egg.event_enable(egg.EventType.MWHEEL, egg.EventState.DISABLED);
      egg.event_enable(egg.EventType.TOUCH, egg.EventState.DISABLED);
      this.bus.unlisten(this.busListener);
      this.busListener = 0;
      this.bus.joyLogical.enable(false);
    }
  }
  
  update(elapsed: number): void {
    if (this.joyDx || this.joyDy) {
      this.setPosition(this.x + this.joyDx * this.joySpeed * elapsed, this.y + this.joyDy * this.joySpeed * elapsed, false);
    }
  }
  
  render(): void {
    if (this.enabled && this.texid) {
      const dstx = Math.round(this.x - this.hotx);
      const dsty = Math.round(this.y - this.hoty);
      egg.draw_decal(1, this.texid, dstx, dsty, this.srcx, this.srcy, this.srcw, this.srch, 0);
    }
  }
  
  private onBusEvent(event: egg.Event): void {
    switch (event.eventType) {
      case egg.EventType.MMOTION: this.setPosition(event.v0, event.v1, true); break;
      case egg.EventType.MBUTTON: this.setPosition(event.v2, event.v3, true); if (event.v0 === 1) this.setButton(event.v1, true); break;
      case egg.EventType.TOUCH: {
          this.setPosition(event.v2, event.v3, false);
          if (event.v1 === 1) this.setButton(1, false);
          else if (event.v1 === 0) this.setButton(0, false);
        } break;
    }
  }
  
  private onJoyEvent(btnid: number, value: number): void {
         if (btnid === this.btnidLeft ) { if (value) this.joyDx = -1; else if (this.joyDx < 0) this.joyDx = 0; }
    else if (btnid === this.btnidRight) { if (value) this.joyDx =  1; else if (this.joyDx > 0) this.joyDx = 0; }
    else if (btnid === this.btnidUp   ) { if (value) this.joyDy = -1; else if (this.joyDy < 0) this.joyDy = 0; }
    else if (btnid === this.btnidDown ) { if (value) this.joyDy =  1; else if (this.joyDy > 0) this.joyDy = 0; }
    else if (btnid === this.btnidButton) this.setButton(value, false);
  }
  
  private setPosition(x: number, y: number, fromMouseEvent: boolean): void {
    if (x < 0) x = 0; else if (x >= this.screenw) x = this.screenw - 1;
    if (y < 0) y = 0; else if (y >= this.screenh) y = this.screenh - 1;
    if ((x === this.x) && (y === this.y)) return;
    this.x = x;
    this.y = y;
    if (!fromMouseEvent) this.bus.onEvent({ eventType: egg.EventType.MMOTION, v0: Math.round(this.x), v1: Math.round(this.y), v2: 0, v3: 0 });
  }
  
  private setButton(v: number, fromMouseEvent: boolean): void {
    if (v) {
      if (this.button) return;
      this.button = true;
    } else {
      if (!this.button) return;
      this.button = false;
    }
    if (!fromMouseEvent) this.bus.onEvent({ eventType: egg.EventType.MBUTTON, v0: 1, v1: this.button ? 1 : 0, v2: Math.round(this.x), v3: Math.round(this.y) });
  }
}
