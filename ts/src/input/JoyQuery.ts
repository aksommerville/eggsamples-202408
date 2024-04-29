import { Bus } from "./Bus";
import { Joy2Device, Joy2Part } from "./JoyTwoState";

const enum JoyQueryState {
  NONE,
  INITIAL_ZERO, // Wait for all buttons to go zero before doing anything.
  ERROR_WAIT,
  FIRST_WAIT,
  FIRST_HOLD,
  AGAIN_WAIT,
  AGAIN_HOLD,
}

const TIMEOUT = 10; // seconds
const TIMEOUT_VISIBLE = 6; // How many seconds will we show the clock? <10

export class JoyQuery {
  public enabled = false;
  private twoStateListener = 0;
  private buttonNames: string[] = [];
  private standardNames: string[] = [];
  private prompts = [
    "Fault! Please press %.",
    "Please press %.",
    "Press % again.",
  ];
  private labelTexid = 0;
  private labelw = 0;
  private labelh = 0;
  private labelMessage = "";
  private texid = 0;
  private glyphw = 0;
  private glyphh = 0;
  private state: JoyQueryState = JoyQueryState.NONE;
  private buttonp = 0;
  private device: Joy2Device | null = null;
  private screenw = 0;
  private screenh = 0;
  private timeout = 0;
  private tiles: Uint8Array | null = null;
  private srcbtnid = 0;
  private srcpart: Joy2Part = 'b';
  private changes: { srcbtnid: number; srcpart: Joy2Part; dstbtnid: number; }[] = [];
  
  constructor(private bus: Bus) {
    const fb = egg.texture_get_header(1);
    this.screenw = fb.w;
    this.screenh = fb.h;
  }
  
  /**
   * Provide the verbiage for our prompts.
   * Must be an array of three strings, where '%' is replaced by the button name.
   * We supply defaults in English.
   *  - [0] Initial prompt, after receiving invalid input. "Fault! Please press %."
   *  - [1] Initial prompt, normal case. "Please press %."
   *  - [2] Secondary prompt, asking user to press it again. "Press % again."
   */
  setPrompts(prompts: string[]): void {
    this.prompts = prompts;
  }
  
  setFontTilesheet(texid: number): void {
    if (this.texid = texid) {
      const hdr = egg.texture_get_header(this.texid);
      this.glyphw = hdr.w >> 4;
      this.glyphh = hdr.h >> 4;
    }
  }
  
  setButtonNames(
    names: string[],
    standardNames?: string[]
  ): void {
    this.buttonNames = names;
    this.standardNames = standardNames || [];
  }
  
  begin(devid: number): boolean {
    this.enabled = false;
    if (!devid) return false;
    if (!this.texid) return false;
    if (!this.buttonNames.length) return false;
    this.device = this.bus.joyTwoState.devices.find(d => d.devid === devid) || null;
    if (!this.device) return false;
    for (this.buttonp=0; this.buttonp<this.buttonNames.length; this.buttonp++) {
      if (this.buttonNames[this.buttonp]) break;
    }
    if (this.buttonp >= this.buttonNames.length) return false; // They gave us names, but they're all empty.
    this.state = JoyQueryState.FIRST_WAIT;
    if ((this.device.natural.size > 0) || this.device.buttons.find(b => b.values.find(v => v))) {
      this.state = JoyQueryState.INITIAL_ZERO;
    }
    this.enabled = true;
    this.timeout = TIMEOUT;
    this.changes = [];
    if (!this.twoStateListener) {
      this.twoStateListener = this.bus.joyTwoState.listen((d, b, p, v) => this.onTwoState(d, b, p, v));
    }
    this.bus.joyLogical.suspend();
    return true;
  }
  
  cancel(): void {
    this.enabled = false;
    this.bus.joyTwoState.unlisten(this.twoStateListener);
    this.twoStateListener = 0;
    this.bus.joyLogical.resume();
  }
  
  private commit(): void {
    this.bus.joyLogical.replaceMap(this.device, this.changes);
  }
  
  update(elapsed: number): void {
    if (!this.enabled) return;
    if ((this.timeout -= elapsed) <= 0) {
      this.advance(false);
    }
  }
  
  render(): void {
    if (!this.enabled) return;
    egg.draw_rect(1, 0, 0, this.screenw, this.screenh, 0x000000c0);
    if (this.bus.font) {
      this.renderWithFont();
    } else {
      this.renderWithTiles();
    }
  }
  
  private renderWithFont(): void {
    const message = this.getMessage();
    if (message !== this.labelMessage) {
      egg.texture_del(this.labelTexid);
      this.labelTexid = this.bus.font.renderTexture(message);
      const hdr = egg.texture_get_header(this.labelTexid);
      this.labelw = hdr.w;
      this.labelh = hdr.h;
    }
    const dstx = (this.screenw >> 1) - (this.labelw >> 1);
    const dsty = (this.screenh >> 1) - (this.labelh >> 1);
    egg.draw_decal(1, this.labelTexid, dstx, dsty, 0, 0, this.labelw, this.labelh, 0);
    if ((this.timeout < TIMEOUT_VISIBLE) && (this.glyphw > 0)) {
      const x = (this.screenw >> 1) - (this.glyphw >> 1);
      const y = (this.screenh >> 1) + this.glyphh;
      const tileid = 0x30 + Math.floor(this.timeout);
      const srcx = (tileid & 0x0f) * this.glyphw;
      const srcy = (tileid >> 4) * this.glyphh;
      egg.draw_mode(egg.XferMode.ALPHA, 0xffffffff, 0xff);
      egg.draw_decal(1, this.texid, x, y, srcx, srcy, this.glyphw, this.glyphh, 0);
      egg.draw_mode(egg.XferMode.ALPHA, 0, 0xff);
    }
  }
  
  private renderWithTiles(): void {
    const message = this.getMessage();
    const tileCount = message.length + ((this.timeout < TIMEOUT_VISIBLE) ? 1 : 0);
    if (!this.tiles || (tileCount * 6 > this.tiles.length)) {
      this.tiles = new Uint8Array(tileCount * 6);
    }
    let tilep = 0;
    let x = (this.screenw >> 1) - ((message.length * this.glyphw) >> 1);
    let y = this.screenh >> 1;
    for (let i=0; i<message.length; i++, x+=this.glyphw) {
      this.tiles[tilep++] = x;
      this.tiles[tilep++] = x >> 8;
      this.tiles[tilep++] = y;
      this.tiles[tilep++] = y >> 8;
      this.tiles[tilep++] = message.charCodeAt(i);
      this.tiles[tilep++] = 0;
    }
    if (this.timeout < TIMEOUT_VISIBLE) {
      x = this.screenw >> 1;
      y = (this.screenh >> 1) + (this.glyphh << 1);
      this.tiles[tilep++] = x;
      this.tiles[tilep++] = x >> 8;
      this.tiles[tilep++] = y;
      this.tiles[tilep++] = y >> 8;
      this.tiles[tilep++] = 0x30 + Math.floor(this.timeout);
      this.tiles[tilep++] = 0;
    }
    egg.draw_mode(egg.XferMode.ALPHA, 0xffffffff, 0xff);
    egg.draw_tile(1, this.texid, this.tiles.buffer, tileCount);
    egg.draw_mode(egg.XferMode.ALPHA, 0, 0xff);
  }
  
  private getMessage(): string {
    switch (this.state) {
      case JoyQueryState.ERROR_WAIT: return this.prompts[0].replace('%', this.buttonNames[this.buttonp]); break;
      case JoyQueryState.FIRST_WAIT: return this.prompts[1].replace('%', this.buttonNames[this.buttonp]); break;
      case JoyQueryState.AGAIN_WAIT: return this.prompts[2].replace('%', this.buttonNames[this.buttonp]); break;
    }
    return "";
  }
  
  private onTwoState(devid: number, btnid: number, part: Joy2Part, value: number): void {
    if (!this.enabled) return;
    if (devid !== this.device.devid) return;
    switch (this.state) {
    
      case JoyQueryState.INITIAL_ZERO: {
          if ((this.device.natural.size > 0) || this.device.buttons.find(b => b.values.find(v => v))) {
            // still waiting...
          } else if (!value) {
            this.state = JoyQueryState.FIRST_WAIT;
            this.timeout = TIMEOUT;
          }
        } break;
    
      case JoyQueryState.ERROR_WAIT:
      case JoyQueryState.FIRST_WAIT: {
          if (!value) return;
          this.srcbtnid = btnid;
          this.srcpart = part;
          this.timeout = TIMEOUT;
          this.state = JoyQueryState.FIRST_HOLD;
        } break;
        
      case JoyQueryState.FIRST_HOLD: {
          if ((btnid === this.srcbtnid) && (part === this.srcpart) && !value) {
            this.state = JoyQueryState.AGAIN_WAIT;
            this.timeout = TIMEOUT;
          } else if (value) {
            this.state = JoyQueryState.ERROR_WAIT;
            this.timeout = TIMEOUT;
          }
        } break;
        
      case JoyQueryState.AGAIN_WAIT: {
          if (!value) return;
          if ((btnid !== this.srcbtnid) || (part !== this.srcpart)) {
            this.state = JoyQueryState.ERROR_WAIT;
            this.timeout = TIMEOUT;
          } else {
            this.state = JoyQueryState.AGAIN_HOLD;
            this.timeout = TIMEOUT;
          }
        } break;
        
      case JoyQueryState.AGAIN_HOLD: {
          if ((btnid === this.srcbtnid) && (part === this.srcpart) && !value) {
            this.advance(true);
          } else if (value) {
            this.state = JoyQueryState.ERROR_WAIT;
            this.timeout = TIMEOUT;
          }
        } break;
        
      default: this.cancel();
    }
  }
  
  private advance(record: boolean): void {
    if (record) {
      this.changes.push({ srcbtnid: this.srcbtnid, srcpart: this.srcpart, dstbtnid: 1 << this.buttonp });
    }
    for (;;) {
      this.buttonp++;
      if (this.buttonp >= this.buttonNames.length) {
        this.commit();
        this.cancel();
        return;
      }
      if (this.buttonNames[this.buttonp]) break;
    }
    this.state = JoyQueryState.FIRST_WAIT;
    this.timeout = TIMEOUT;
  }
}
