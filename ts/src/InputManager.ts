/**
 * InputManager has an overall strategy of how to model input:
 *   `raw`: Deliver events how we receive them.
 *   `natural`: Map joysticks as needed, but don't do anything suprising.
 *   `joystick`: Treat the keyboard as a joystick. Text and raw key events not available.
 *   `text`: Take text from the keyboard, present an artificial keyboard if necessary. You should enter this mode temporarily, only as needed.
 *   `pointer`: Point-and-click events from mouse or touch, or an artificial pointer controlled by keyboard or joystick.
 */
export type InputManagerMode = "raw" | "natural" | "joystick" | "text" | "pointer";

/**
 * In "joystick" mode, we model a set of players each with a 16-bit state.
 */
export const enum InputManagerButton {
  LEFT  = 0x0001, // dpad...
  RIGHT = 0x0002,
  UP    = 0x0004,
  DOWN  = 0x0008,
  SOUTH = 0x0010, // thumb buttons...
  WEST  = 0x0020,
  EAST  = 0x0040,
  NORTH = 0x0080,
  L1    = 0x0100, // triggers...
  R1    = 0x0200,
  L2    = 0x0400,
  R2    = 0x0800,
  AUX1  = 0x1000, // eg Start
  AUX2  = 0x2000, // eg Select
  AUX3  = 0x4000, // eg Heart
  CD    = 0x8000, // Nonzero if connected.
}

export type InputManagerEvent = {
  type: egg.EventType.INPUT;
  devid: number;
  btnid: number;
  value: number;
} | {
  type: egg.EventType.CONNECT;
  joystick: InputManagerJoystick;
} | {
  type: egg.EventType.DISCONNECT;
  devid: number;
} | {
  type: egg.EventType.KEY;
  keycode: number; // USB-HID page 7.
  value: number; // (0,1,2)=(off,on,repeat)
} | {
  type: egg.EventType.TEXT;
  codepoint: number; // Unicode.
} | {
  type: egg.EventType.MMOTION; // Mouse motion, or dragging touches in "pointer" mode.
  x: number;
  y: number;
} | {
  type: egg.EventType.MBUTTON; // Real mouse button, or touches in "pointer" mode.
  btnid: number; // (1,2,3)=(left,right,middle), others are possible
  value: number; // (0,1)=(release,press)
  x: number;
  y: number;
} | {
  type: egg.EventType.MWHEEL;
  dx: number;
  dy: number;
  x: number;
  y: number;
} | {
  type: egg.EventType.TOUCH; // Not reported in "pointer" mode; they turn into a mouse.
  id: number;
  state: number; // (0,1,2)=(release,press,move)
  x: number;
  y: number;
} | {
  type: egg.EventType.ACCELEROMETER;
  x: number; // m/s**2 (all three). In theory, sqrt(x**2+y**2+z**2) should equal 9.8 at rest.
  y: number;
  z: number;
} | {
  // We do report network events raw in all modes, but we also provide a more structured interface.
  type: egg.EventType.HTTP_RSP;
  reqid: number;
  status: number;
  length: number;
  body: ArrayBuffer | null;
} | {
  type: egg.EventType.WS_CONNECT;
  wsid: number;
} | {
  type: egg.EventType.WS_DISCONNECT;
  wsid: number;
} | {
  type: egg.EventType.WS_MESSAGE;
  wsid: number;
  msgid: number;
  length: number;
  message: string;
};

export interface InputManagerPlayerEvent {
  playerid: number;
  btnid: number;
  value: number;
  state: number;
};

export interface InputManagerListener {
  id: number;
  cb: (event: InputManagerEvent) => void;
  events: number; // Multiple (1<<egg.EventType).
}

export interface InputManagerPlayerListener {
  id: number;
  playerid: number; // Zero for all.
  cb: (event: { playerid: number; btnid: InputManagerButton; value: number; state: number; }) => void;
}

export const enum InputManagerJoystickButtonMode {
  RAW,      // Emit source value verbatim.
  SIGNAL,   // Same ranging as TWOSTATE but emit event only when it becomes 1; dstbtnid can be an enum instead of a bit.
  TWOSTATE, // ((v>=lo)&&(v<=hi))?1:0
  THREEWAY, // (v<=lo)?-1:(v>=hi)?1:0, dstbtnid must be (LEFT|RIGHT) or (UP|DOWN)
  HAT,      // (v-lo)=(N,NE,E,SE,S,SW,W,NW), anything OOB is Center.
}

export interface InputManagerJoystickButton {
  srcbtnid: number;
  srcvalue: number;
  mode: InputManagerJoystickButtonMode;
  srclo: number;
  srchi: number;
  dstbtnid: number;
  dstvalue: number;
}

export interface InputManagerJoystick {
  devid: number;
  name: string;
  vid: number;
  pid: number;
  version: number;
  playerid: number;
  buttons: InputManagerJoystickButton[];
}

export interface InputManagerJoystickConfigButton {
  srcbtnid: number;
  dstbtnid: number;
};

export interface InputManagerJoystickConfig {
  name: string; // Empty matches all.
  vid: number; // Zero matches all.
  pid: number; // Zero matches all.
  version: number; // Zero matches all.
  buttons: InputManagerJoystickConfigButton[];
}

export interface InputManagerCursor {
  texid: number;
  x: number;
  y: number;
  w: number;
  h: number;
  hotX: number; // 0..w-1
  hotY: number; // 0..h-1
}

export const enum InputManagerJcfgState {
  NONE = 0,
  FAULT = 1, // Mostly equivalent to WAIT_FIRST, but the last action was invalid.
  WAIT_FIRST = 2,
  HOLD_FIRST = 3,
  WAIT_SECOND = 4,
  HOLD_SECOND = 5,
  DONE = 6,
}

export class InputManager {
  private modeStack: InputManagerMode[] = ["natural"];
  private listeners: InputManagerListener[] = [];
  private playerListeners: InputManagerPlayerListener[] = [];
  private nextListenerId = 1;
  private joysticks: InputManagerJoystick[] = [];
  private joyconfigs: InputManagerJoystickConfig[] = [];
  private joyconfigsDirty = false;
  private keyconfig: [number, number][] = []; // [keycode, btnid]
  private texid_font = 0;
  private cursor?: InputManagerCursor;
  private fakePointer?: { x: number; y: number; dx: number; dy: number; button: boolean; };
  private fakePointerSpeed: number; // px/s
  private fakeKeyboard?: { page: number; row: number; col: number; };
  private fakeKeyboardLayout: string[][] = [[
    "qwertyuiop{\b",
    "asdfghjkl;}\n",
    "zxcvbnm,./ \u0001",
  ], [
    "QWERTYUIOP|\b",
    "ASDFGHJKL: \n",
    "ZXCVBNM<>? \u0001",
  ], [
    "1234567890 \b",
    "!@#$%^&*() \n",
    "`-=[]\\'~_+ \u0001",
  ]];
  private fakeKeyboardColw = 16;
  private fakeKeyboardRowh = 16;
  private fakeKeyboardTiles?: Uint8Array;
  private shiftKey = false;
  private screenw = 0;
  private screenh = 0;
  private playerStates = [0];
  private accelEnable = false;
  private jcfgState: InputManagerJcfgState = InputManagerJcfgState.NONE;
  private jcfgNames: string[] = []; // Indexed by state bit. Skip empties.
  private jcfgIndex = 0;
  private jcfgTiles?: Uint8Array;
  
  private textState: egg.EventState = egg.EventState.QUERY;
  private mouseState: egg.EventState = egg.EventState.QUERY;
  private touchState: egg.EventState = egg.EventState.QUERY;
  
  private httpPending: {
    reqid: number;
    resolve: (arg: { status: number; body: ArrayBuffer | null; }) => void;
    reject: (arg: { status: number; body: ArrayBuffer | null; }) => void;
  }[] = [];
  private wsListeners: {
    id: number;
    url: string;
    wsid: number;
    cb: (message: string | 0 | 1) => void;
  }[] = [];

  /**
   * If you provide (texid_font) it must be a tilesheet containing the first 256 codepoints.
   * If zero or undefined, we won't produce an onscreen keyboard and won't do interactive joystick mapping.
   * (cursor) is similar, a cursor image for fake pointer on devices that don't provide mouse or touch events.
   */
  constructor(
    texid_font?: number,
    cursor?: InputManagerCursor
  ) {
    if (texid_font) this.texid_font = texid_font;
    this.cursor = cursor;
    const fb = egg.texture_get_header(1);
    this.screenw = fb.w;
    this.screenh = fb.h;
    // Default fakePointer speed: Cross the longer fb axis in 2 seconds.
    this.fakePointerSpeed = Math.max(this.screenw, this.screenh) / 2;
    this.loadJoyconfigs();
    this.enterMode(this.modeStack[0]);
  }
  
  /**
   * You can change the cursor at any time, if you want to animate it.
   */
  setCursor(cursor?: InputManagerCursor): void {
    this.cursor = cursor;
  }
  
  /**
   * In "joystick" mode, we map each device to a >0 "playerid".
   * By default, there's just one player.
   */
  setPlayerCount(count: number): void {
    if (count < 1) throw new Error(`Invalid player count ${JSON.stringify(count)}`);
    if (count < this.playerStates.length) {
      this.playerStates.splice(count, this.playerStates.length - count);
    } else {
      while (count > this.playerStates.length) this.playerStates.push(0);
    }
    this.refreshJoystickAssignments();
  }
  
  /**
   * The 16-bit state of one player as an integer.
   * Use playerid zero to aggregate all players.
   */
  getPlayer(playerid: number): number {
    if (!playerid) return this.playerStates.reduce((a, v) => a | v, 0);
    return this.playerStates[playerid - 1] || 0;
  }
  
  /**
   * Returns true if we are modal, ie you should pause normal processing.
   */
  update(elapsed: number): boolean {
    if (this.joyconfigsDirty) {
      this.joyconfigsDirty = false;
      this.saveJoyconfigs();
    }
    for (const event of egg.event_next()) switch (event.eventType) {
      case egg.EventType.INPUT: this.onInput(event.v0, event.v1, event.v2); break;
      case egg.EventType.CONNECT: this.onConnect(event.v0); break;
      case egg.EventType.DISCONNECT: this.onDisconnect(event.v0); break;
      case egg.EventType.HTTP_RSP: this.onHttpRsp(event.v0, event.v1, event.v2); break;
      case egg.EventType.WS_CONNECT: this.onWsConnect(event.v0); break;
      case egg.EventType.WS_DISCONNECT: this.onWsDisconnect(event.v0); break;
      case egg.EventType.WS_MESSAGE: this.onWsMessage(event.v0, event.v1, event.v2); break;
      case egg.EventType.MMOTION: this.onMmotion(event.v0, event.v1); break;
      case egg.EventType.MBUTTON: this.onMbutton(event.v0, event.v1, event.v2, event.v3); break;
      case egg.EventType.MWHEEL: this.onMwheel(event.v0, event.v1, event.v2, event.v3); break;
      case egg.EventType.KEY: this.onKey(event.v0, event.v1); break;
      case egg.EventType.TEXT: this.onText(event.v0); break;
      case egg.EventType.TOUCH: this.onTouch(event.v0, event.v1, event.v2, event.v3); break;
      case egg.EventType.ACCELEROMETER: this.onAccelerometer(event.v0, event.v1, event.v2); break;
      default: this.onUnknownEvent(event);
    }
    if (this.fakePointer) {
      const pvx = ~~this.fakePointer.x;
      const pvy = ~~this.fakePointer.y;
      this.fakePointer.x = Math.min(this.screenw - 1, Math.max(0, this.fakePointer.x + this.fakePointer.dx * elapsed * this.fakePointerSpeed));
      this.fakePointer.y = Math.min(this.screenh - 1, Math.max(0, this.fakePointer.y + this.fakePointer.dy * elapsed * this.fakePointerSpeed));
      const nx = ~~this.fakePointer.x;
      const ny = ~~this.fakePointer.y;
      if ((nx !== pvx) || (ny !== pvy)) {
        this.onMmotion(nx, ny);
      }
    }
    if (this.jcfgState) {
      return true;
    }
    return false;
  }
  
  /**
   * You must call at the end of the main render, for onscreen keyboard or fake pointer to appear.
   */
  render(): void {
    if (this.jcfgState) {
      egg.draw_rect(1, 0, 0, this.screenw, this.screenh, 0x102040c0);
      const buttonName = this.jcfgNames[this.jcfgIndex];
      let msg = "";
      switch (this.jcfgState) {
        // TODO Allow override of verbiage, the player doesn't necessarily speak English!
        case InputManagerJcfgState.FAULT: msg = `Error! Please press ${buttonName}`; break;
        case InputManagerJcfgState.WAIT_FIRST: msg = `Please press ${buttonName}`; break;
        case InputManagerJcfgState.WAIT_SECOND: msg = `OK, press ${buttonName} again`; break;
      }
      if (msg) {
        if (!this.jcfgTiles || (msg.length * 6 > this.jcfgTiles.length)) {
          this.jcfgTiles = new Uint8Array(msg.length * 6);
        }
        const fmt = egg.texture_get_header(this.texid_font);
        const glyphw = fmt.w >> 4;
        let x = (this.screenw >> 1) - ((msg.length * glyphw) >> 1) + (glyphw >> 1);
        let y = this.screenh >> 1;
        let vtxp = 0;
        for (let i=0; i<msg.length; i++, x+=glyphw) {
          this.jcfgTiles[vtxp++] = x;
          this.jcfgTiles[vtxp++] = x >> 8;
          this.jcfgTiles[vtxp++] = y;
          this.jcfgTiles[vtxp++] = y >> 8;
          this.jcfgTiles[vtxp++] = msg.charCodeAt(i);
          this.jcfgTiles[vtxp++] = 0;
        }
        egg.draw_mode(egg.XferMode.ALPHA, 0x000000ff, 0xff);
        egg.draw_tile(1, this.texid_font, this.jcfgTiles.buffer, msg.length);
        for (let i=0, vtxp=0; i<msg.length; i++, vtxp+=6) {
          if (this.jcfgTiles[vtxp+0]) this.jcfgTiles[vtxp+0]--;
          else { this.jcfgTiles[vtxp+0] = 0xff; this.jcfgTiles[vtxp+1]--; }
          if (this.jcfgTiles[vtxp+2]) this.jcfgTiles[vtxp+2]--;
          else { this.jcfgTiles[vtxp+2] = 0xff; this.jcfgTiles[vtxp+3]--; }
        }
        egg.draw_mode(egg.XferMode.ALPHA, 0xffffffff, 0xff);
        egg.draw_tile(1, this.texid_font, this.jcfgTiles.buffer, msg.length);
        egg.draw_mode(egg.XferMode.ALPHA, 0, 0xff);
      }
      return;
    }
    if (this.fakeKeyboard) {
      this.renderFakeKeyboard();
    }
    if (this.fakePointer) {
      egg.draw_decal(
        1, this.cursor.texid,
        this.fakePointer.x - this.cursor.hotX, this.fakePointer.y - this.cursor.hotY,
        this.cursor.x, this.cursor.y,
        this.cursor.w, this.cursor.h,
        egg.Xform.NONE
      );
    }
  }
  
  getMode(): InputManagerMode {
    return this.modeStack[this.modeStack.length - 1];
  }
  
  pushMode(mode: InputManagerMode): void {
    this.modeStack.push(mode);
    this.enterMode(mode);
  }
  
  popMode(mode: InputManagerMode): InputManagerMode {
    if ((this.modeStack.length <= 1) || (this.modeStack[this.modeStack.length - 1] !== mode)) {
      throw new Error(`Invalid request to pop input mode ${JSON.stringify(mode)}. Current stack: ${JSON.stringify(this.modeStack)}`);
    }
    this.modeStack.splice(this.modeStack.length - 1, 1);
    const newMode = this.modeStack[this.modeStack.length - 1];
    this.enterMode(newMode);
    return newMode;
  }
  
  listen(events: string[] | string | number, cb: (event: InputManagerEvent) => void): number {
    const id = this.nextListenerId++;
    if (typeof(events) === "string") {
      events = 1 << this.evalEvent(events);
    } else if (events instanceof Array) {
      events = events.map(v => 1 << this.evalEvent(v)).reduce((a, v) => a | v, 0);
    } else if (typeof(events) !== "number") {
      throw new Error(`InputManager.listen events must be number, string, or array of string`);
    }
    this.listeners.push({ id, cb, events });
    return id;
  }
  
  listenPlayer(playerid: number, cb: (event: { playerid: number; btnid: InputManagerButton; value: number; state: number; }) => void): number {
    const id = this.nextListenerId++;
    this.playerListeners.push({ id, cb, playerid });
    return id;
  }
  
  unlisten(id: number): void {
    let p;
    if ((p = this.listeners.findIndex(l => l.id === id)) >= 0) {
      this.listeners.splice(p, 1);
    } else if ((p = this.playerListeners.findIndex(l => l.id === id)) >= 0) {
      this.playerListeners.splice(p, 1);
    }
  }
  
  /**
   * Enter a special mode where we prompt the user to press each button on her joystick.
   * (names) are the button names to display, indexed by state bit, and we'll skip empties.
   * If you don't provide it, we'll ask for all 15 buttons, and call them:
   *   Left Right Up Down South West East North L1 R1 L2 R2 Start Select Menu
   * Returns true if we entered config mode or false if we failed to (eg font unset).
   */
  configureJoystick(names?: string[]): boolean {
    return this.jcfgBegin(names || [
      "Left", "Right", "Up", "Down",
      "South", "West", "East", "North",
      "L1", "R1", "L2", "R2",
      "Start", "Select", "Menu",
    ]);
  }
  
  cancelJoystickConfig(): void {
    this.jcfgEnd();
  }
  
  evalEvent(name: string): egg.EventType {
    switch (name.toUpperCase()) {
      case "INPUT": return egg.EventType.INPUT;
      case "CONNECT": return egg.EventType.CONNECT;
      case "DISCONNECT": return egg.EventType.DISCONNECT;
      case "HTTP_RSP": return egg.EventType.HTTP_RSP;
      case "WS_CONNECT": return egg.EventType.WS_CONNECT;
      case "WS_DISCONNECT": return egg.EventType.WS_DISCONNECT;
      case "MMOTION": return egg.EventType.MMOTION;
      case "MBUTTON": return egg.EventType.MBUTTON;
      case "MWHEEL": return egg.EventType.MWHEEL;
      case "KEY": return egg.EventType.KEY;
      case "TEXT": return egg.EventType.TEXT;
      case "TOUCH": return egg.EventType.TOUCH;
      case "ACCELEROMETER": return egg.EventType.ACCELEROMETER;
    }
    throw new Error(`Unknown event type ${JSON.stringify(name)}`);
  }
  
  /**
   * You must make HTTP calls thru us, since responses will be delivered in the event queue.
   */
  http(method: string, url: string, body?: string | ArrayBuffer): Promise<{ status: number; body: ArrayBuffer | null }> {
    const reqid = egg.http_request(method, url, body);
    if (reqid < 1) return Promise.reject({
      status: 0,
      body: null,
    });
    return new Promise((resolve, reject) => {
      this.httpPending.push({ reqid, resolve, reject });
    });
  }
  
  /**
   * Makes a new connection, or borrows one if another listener already exists.
   */
  websocketListen(url: string, cb: (message: string | 0 | 1) => void): number {
    let wsid = 0;
    for (const listener of this.wsListeners) {
      if (!listener.wsid) continue;
      if (listener.url !== url) continue;
      wsid = listener.wsid;
      break;
    }
    if (wsid) {
      cb(1);
    } else {
      if ((wsid = egg.ws_connect(url)) < 1) return -1;
      for (const listener of this.wsListeners) {
        if (listener.url !== url) continue;
        if (listener.wsid) continue;
        listener.wsid = wsid;
      }
    }
    const id = this.nextListenerId++;
    this.wsListeners.push({ id, wsid, url, cb });
    return id;
  }
  
  websocketUnlisten(id: number): void {
    const p = this.wsListeners.findIndex(l => l.id === id);
    if (p >= 0) {
      const { url, wsid } = this.wsListeners[p];
      this.wsListeners.splice(p, 1);
      if (this.wsListeners.find(l => l.wsid === wsid)) {
        // Somebody else is listening on this socket, so let it live.
      } else {
        egg.ws_disconnect(wsid);
      }
    }
  }
  
  /**
   * Accelerometers don't participate in our main mode or virtualization.
   * Enable or disable at will, and events will flow to our listeners.
   */
  enableAccelerometer(enable: boolean): void {
    if (enable === this.accelEnable) return;
    this.accelEnable = enable;
    egg.event_enable(egg.EventType.ACCELEROMETER, enable ? egg.EventState.ENABLED : egg.EventState.DISABLED);
  }
   
/* ----- Everything below this point is private. ----- */

  private enterMode(mode: InputManagerMode): void {
    this.fakeKeyboard = null;
    this.fakePointer = null;
    this.shiftKey = false;
    switch (mode) {
    
      case "raw": {
          this.textState = egg.event_enable(egg.EventType.TEXT, egg.EventState.DISABLED);
          this.mouseState = egg.event_enable(egg.EventType.MMOTION, egg.EventState.DISABLED);
          egg.event_enable(egg.EventType.MBUTTON, egg.EventState.DISABLED);
          egg.event_enable(egg.EventType.MWHEEL, egg.EventState.DISABLED);
          this.touchState = egg.event_enable(egg.EventType.TOUCH, egg.EventState.DISABLED);
        } break;
        
      case "natural": {
          this.textState = egg.event_enable(egg.EventType.TEXT, egg.EventState.ENABLED);
          this.mouseState = egg.event_enable(egg.EventType.MMOTION, egg.EventState.DISABLED);
          egg.event_enable(egg.EventType.MBUTTON, egg.EventState.DISABLED);
          egg.event_enable(egg.EventType.MWHEEL, egg.EventState.DISABLED);
          this.touchState = egg.event_enable(egg.EventType.TOUCH, egg.EventState.DISABLED);
        } break;
        
      case "joystick": {
          this.textState = egg.event_enable(egg.EventType.TEXT, egg.EventState.DISABLED);
          this.mouseState = egg.event_enable(egg.EventType.MMOTION, egg.EventState.DISABLED);
          egg.event_enable(egg.EventType.MBUTTON, egg.EventState.DISABLED);
          egg.event_enable(egg.EventType.MWHEEL, egg.EventState.DISABLED);
          this.touchState = egg.event_enable(egg.EventType.TOUCH, egg.EventState.DISABLED);
        } break;
        
      case "text": {
          this.textState = egg.event_enable(egg.EventType.TEXT, egg.EventState.ENABLED);
          //this.textState = egg.event_enable(egg.EventType.TEXT, egg.EventState.DISABLED);// XXX disabled for testing fakeKeyboard
          if ((this.textState === egg.EventState.ENABLED) || (this.textState === egg.EventState.REQUIRED)) {
            // We'll get real text events, perfect.
            this.mouseState = egg.event_enable(egg.EventType.MMOTION, egg.EventState.DISABLED);
            egg.event_enable(egg.EventType.MBUTTON, egg.EventState.DISABLED);
            egg.event_enable(egg.EventType.MWHEEL, egg.EventState.DISABLED);
            this.touchState = egg.event_enable(egg.EventType.TOUCH, egg.EventState.DISABLED);
          } else if (this.texid_font) {
            // Use a fake keyboard accessible to joysticks, mouse, or touch.
            // Note that we will also try to map KEY events, if any come in.
            this.mouseState = egg.event_enable(egg.EventType.MMOTION, egg.EventState.ENABLED);
            egg.event_enable(egg.EventType.MBUTTON, egg.EventState.ENABLED);
            egg.event_enable(egg.EventType.MWHEEL, egg.EventState.ENABLED);
            this.touchState = egg.event_enable(egg.EventType.TOUCH, egg.EventState.ENABLED);
            this.beginFakeKeyboard();
          }
        } break;
        
      case "pointer": {
          this.textState = egg.event_enable(egg.EventType.TEXT, egg.EventState.DISABLED);
          this.mouseState = egg.event_enable(egg.EventType.MMOTION, egg.EventState.ENABLED);
          //this.mouseState = egg.event_enable(egg.EventType.MMOTION, egg.EventState.DISABLED); // XXX disabled for testing fakePointer
          egg.event_enable(egg.EventType.MBUTTON, egg.EventState.ENABLED);
          egg.event_enable(egg.EventType.MWHEEL, egg.EventState.ENABLED);
          this.touchState = egg.event_enable(egg.EventType.TOUCH, egg.EventState.ENABLED);
          if ((this.mouseState === egg.EventState.ENABLED) || (this.mouseState === egg.EventState.REQUIRED)) {
            // We have a real mouse, perfect.
          } else if ((this.touchState === egg.EventState.ENABLED) || (this.touchState === egg.EventState.REQUIRED)) {
            // We get touch events, that's just as good.
          } else if (this.cursor) {
            // No mouse or touch, but we can make a fake cursor to be controlled by keyboard or joystick. TODO
            this.fakePointer = { x: this.screenw >> 1, y: this.screenh >> 1, dx: 0, dy: 0, button: false };
          }
        } break;
    }
  }
  
  private broadcast(event: InputManagerEvent): void {
    const bit = 1 << event.type;
    for (const listener of this.listeners) {
      if (listener.events & bit) listener.cb(event);
    }
  }
  
  /* jcfg
   *********************************************************************/
   
  private jcfgBegin(names: string[]): boolean {
    if (!names.find(v => v)) return false;
    if (!this.texid_font) return false;
    this.jcfgNames = names;
    this.jcfgIndex = 0;
    this.jcfgState = InputManagerJcfgState.WAIT_FIRST;
    return true;
  }
  
  private jcfgEnd(): void {
    this.jcfgState = InputManagerJcfgState.NONE;
  }
  
  private jcfgInput(devid: number, btnid: number, value: number): void {
    egg.log(`TODO jcfgInput(${devid},${btnid},${value})`);
  }
  
  private jcfgKey(keycode: number, value: number): void {
    egg.log(`TODO jcfgKey(${keycode},${value})`);
  }
  
  /* Unknown event -- shouldn't happen, we're handling all defined events.
   ***********************************************************************/
   
  private onUnknownEvent(event: egg.Event): void {
    egg.log(`UNKNOWN ${JSON.stringify(event)}`);
  }
  
  /* Joysticks (or generic input).
   **************************************************************************/
  
  private onInput(devid: number, btnid: number, value: number): void {
    if (this.jcfgState) this.jcfgInput(devid, btnid, value);
    const mode = this.getMode();
    switch (mode) {
      case "raw": this.broadcast({ type: egg.EventType.INPUT, devid, btnid, value }); break;
      default: {
          const joystick = this.joysticks.find(j => j.devid === devid);
          if (!joystick) return;
          const button = joystick.buttons.find(b => b.srcbtnid === btnid);
          if (!button) return;
          if (value === button.srcvalue) return;
          button.srcvalue = value;
          switch (button.mode) {
            case InputManagerJoystickButtonMode.RAW: {
                if (value === button.dstvalue) return;
                for (const listener of this.playerListeners) {
                  if (!listener.playerid || (listener.playerid === joystick.playerid)) {
                    listener.cb({ playerid: joystick.playerid, btnid: button.dstbtnid, value, state: this.playerStates[joystick.playerid]});
                  }
                }
              } break;
            case InputManagerJoystickButtonMode.SIGNAL: {
                const nv = ((value >= button.srclo) && (value <= button.srchi)) ? 1 : 0;
                if (nv === button.dstvalue) return;
                button.dstvalue = nv;
                if (nv) {
                  for (const listener of this.playerListeners) {
                    if (!listener.playerid || (listener.playerid === joystick.playerid)) {
                      listener.cb({ playerid: joystick.playerid, btnid: button.dstbtnid, value: 1, state: this.playerStates[joystick.playerid]});
                    }
                  }
                }
              } break;
            case InputManagerJoystickButtonMode.TWOSTATE: {
                const nv = ((value >= button.srclo) && (value <= button.srchi)) ? 1 : 0;
                if (nv === button.dstvalue) return;
                button.dstvalue = nv;
                this.setPlayerButton(joystick.playerid, button.dstbtnid, nv);
              } break;
            case InputManagerJoystickButtonMode.THREEWAY: {
                const nv = (value <= button.srclo) ? -1 : (value >= button.srchi) ? 1 : 0;
                if (nv === button.dstvalue) return;
                if (button.dstvalue < 0) {
                  this.setPlayerButton(joystick.playerid, button.dstbtnid & (InputManagerButton.LEFT | InputManagerButton.UP), 0);
                } else if (button.dstvalue > 0) {
                  this.setPlayerButton(joystick.playerid, button.dstbtnid & (InputManagerButton.RIGHT | InputManagerButton.DOWN), 0);
                }
                button.dstvalue = nv;
                if (button.dstvalue < 0) {
                  this.setPlayerButton(joystick.playerid, button.dstbtnid & (InputManagerButton.LEFT | InputManagerButton.UP), 1);
                } else if (button.dstvalue > 0) {
                  this.setPlayerButton(joystick.playerid, button.dstbtnid & (InputManagerButton.RIGHT | InputManagerButton.DOWN), 1);
                }
              } break;
            case InputManagerJoystickButtonMode.HAT: {
                let pvx=0, pvy=0, nx=0, ny=0;
                switch (button.dstvalue) {
                  case 7: case 0: case 1: pvy = -1; break;
                  case 5: case 4: case 3: pvy = 1; break;
                }
                switch (button.dstvalue) {
                  case 7: case 6: case 5: pvx = -1; break;
                  case 1: case 2: case 3: pvx = 1; break;
                }
                button.dstvalue = value - button.srclo;
                switch (button.dstvalue) {
                  case 7: case 0: case 1: ny = -1; break;
                  case 5: case 4: case 3: ny = 1; break;
                }
                switch (button.dstvalue) {
                  case 7: case 6: case 5: nx = -1; break;
                  case 1: case 2: case 3: nx = 1; break;
                }
                if (nx !== pvx) {
                       if (pvx < 0) this.setPlayerButton(joystick.playerid, InputManagerButton.LEFT, 0);
                  else if (pvx > 0) this.setPlayerButton(joystick.playerid, InputManagerButton.RIGHT, 0);
                       if (pvy < 0) this.setPlayerButton(joystick.playerid, InputManagerButton.UP, 0);
                  else if (pvy > 0) this.setPlayerButton(joystick.playerid, InputManagerButton.DOWN, 0);
                       if (nx < 0) this.setPlayerButton(joystick.playerid, InputManagerButton.LEFT, 1);
                  else if (nx > 0) this.setPlayerButton(joystick.playerid, InputManagerButton.RIGHT, 1);
                       if (ny < 0) this.setPlayerButton(joystick.playerid, InputManagerButton.UP, 1);
                  else if (ny > 0) this.setPlayerButton(joystick.playerid, InputManagerButton.DOWN, 1);
                }
              } break;
          }
        }
    }
  }
  
  private onConnect(devid: number): void {
    // Map joysticks no matter what, since mode can change.
    const joystick: InputManagerJoystick = {
      devid,
      name: egg.input_device_get_name(devid),
      vid: 0,
      pid: 0,
      version: 0,
      ...egg.input_device_get_ids(devid),
      buttons: [],
      playerid: 0,
    };
    let config = this.findJoyconfig(joystick);
    if (!config) config = this.synthesizeJoyconfig(joystick);
    if (config) this.applyJoyconfig(joystick, config);
    this.joysticks.push(joystick);
    this.refreshJoystickAssignments();
    this.broadcast({ type: egg.EventType.CONNECT, joystick });
  }

  private onDisconnect(devid: number): void {
    const p = this.joysticks.findIndex(j => j.devid === devid);
    if (p >= 0) {
    
      // Zero all buttons:
      const joystick = this.joysticks[p];
      for (const button of joystick.buttons) {
        switch (button.mode) {
          // No zeroing: RAW, SIGNAL
          case InputManagerJoystickButtonMode.TWOSTATE: {
              if (button.dstvalue) this.setPlayerButton(joystick.playerid, button.dstbtnid, 0); break;
            } break;
          case InputManagerJoystickButtonMode.THREEWAY: {
              if (button.dstvalue < 0) {
                this.setPlayerButton(joystick.playerid, button.dstbtnid & (InputManagerButton.LEFT | InputManagerButton.UP), 0);
              } else if (button.dstvalue > 0) {
                this.setPlayerButton(joystick.playerid, button.dstbtnid & (InputManagerButton.RIGHT | InputManagerButton.DOWN), 0);
              }
            } break;
          case InputManagerJoystickButtonMode.HAT: {
              switch (button.dstvalue) {
                case 0: case 1: case 7: this.setPlayerButton(joystick.playerid, InputManagerButton.UP, 0); break;
                case 3: case 4: case 5: this.setPlayerButton(joystick.playerid, InputManagerButton.DOWN, 0); break;
              }
              switch (button.dstvalue) {
                case 1: case 2: case 3: this.setPlayerButton(joystick.playerid, InputManagerButton.RIGHT, 0); break;
                case 5: case 6: case 7: this.setPlayerButton(joystick.playerid, InputManagerButton.LEFT, 0); break;
              }
            } break;
        }
      }
      
      this.joysticks.splice(p, 1);
    }
    this.refreshJoystickAssignments();
    this.broadcast({ type: egg.EventType.DISCONNECT, devid });
  }
  
  private findJoyconfig(joystick: InputManagerJoystick): InputManagerJoystickConfig | null {
    for (const config of this.joyconfigs) {
      if (config.name && (config.name !== joystick.name)) continue;
      if (config.vid && (config.vid !== joystick.vid)) continue;
      if (config.pid && (config.pid !== joystick.pid)) continue;
      if (config.version && (config.version !== joystick.version)) continue;
      return config;
    }
    return null;
  }
  
  private synthesizeJoyconfig(joystick: InputManagerJoystick): InputManagerJoystickConfig | null {
    const config: InputManagerJoystickConfig = {
      name: joystick.name,
      vid: joystick.vid,
      pid: joystick.pid,
      version: joystick.version,
      buttons: [],
    };
    let have = 0;
    let keyboardCount = 0;
    const HORZ = InputManagerButton.LEFT | InputManagerButton.RIGHT;
    const VERT = InputManagerButton.UP | InputManagerButton.DOWN;
    const DPAD = HORZ | VERT;
    const countByIndex = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    // LEFT RIGHT UP DOWN ; SOUTH WEST EAST NORTH ; L1 R1 L2 R2 ; AUX1 AUX2 AUX3 CD
    const loneliestThumb = () => {
      let lop = 4;
      for (let i=5; i<8; i++) {
        if (countByIndex[i] < countByIndex[lop]) lop = i;
      }
      return 1 << lop;
    };
    const loneliestTrigger = () => {
      let lop = 8;
      for (let i=9; i<12; i++) {
        if (countByIndex[i] < countByIndex[lop]) lop = i;
      }
      return 1 << lop;
    };
    const loneliestTwoState = () => {
      let lop = 4;
      for (let i=5; i<15; i++) {
        if (countByIndex[i] < countByIndex[lop]) lop = i;
      }
      return 1 << lop;
    };
    const loneliestAxis = () => {
      const xc = Math.min(countByIndex[0], countByIndex[1]);
      const yc = Math.min(countByIndex[2], countByIndex[3]);
      if (xc <= yc) return HORZ;
      return VERT;
    };
    for (let p=0; ; p++) {
      const btn = egg.input_device_get_button(joystick.devid, p);
      if (!btn) break;
      let dstbtnid = 0, min = 0;
      
      // If it's in the keyboard page, ignore it. Platforms should report these via KEY instead of INPUT.
      if ((btn.hidusage >= 0x00070000) && (btn.hidusage < 0x00080000)) {
        keyboardCount++;
        if (keyboardCount >= 20) return null; // At least 20 of them? It's a real keyboard, not just a reporting fluke. Ignore it.
        continue;
      }
      
      // Check some known usages.
      switch (btn.hidusage) {
        case 0x00010030: min = 3; dstbtnid = HORZ; break; // x
        case 0x00010031: min = 3; dstbtnid = VERT; break; // y
        case 0x00010033: min = 3; dstbtnid = HORZ; break; // rx
        case 0x00010034: min = 3; dstbtnid = VERT; break; // ry
        case 0x00010039: min = 7; dstbtnid = DPAD; break; // hat
        case 0x00010090: min = 2; dstbtnid = InputManagerButton.UP; break; // dpad
        case 0x00010091: min = 2; dstbtnid = InputManagerButton.DOWN; break; // dpad
        case 0x00010092: min = 2; dstbtnid = InputManagerButton.RIGHT; break; // dpad
        case 0x00010093: min = 2; dstbtnid = InputManagerButton.LEFT; break; // dpad
        case 0x00050037: min = 2; dstbtnid = loneliestThumb(); break; // Gamepad Fire/Jump
        case 0x00050039: min = 2; dstbtnid = loneliestTrigger(); break; // Gamepad Trigger
      }
      
      // If it wasn't explicitly known by Usage, pick the most sensible thing according to its range.
      const range = btn.hi - btn.lo + 1;
      if (range < 2) continue;
      if (!min) {
        if (range === 2) {
          min = 2;
          dstbtnid = loneliestTwoState();
        } else if (range === 7) {
          min = 7;
          dstbtnid = DPAD;
        } else {
          min = 3;
          dstbtnid = loneliestAxis();
        }
      }
      
      if (!min) continue;
      if (range < min) continue;
      have |= dstbtnid;
      config.buttons.push({
        srcbtnid: btn.btnid,
        dstbtnid,
      });
      for (let p=0; dstbtnid; p++, dstbtnid>>=1) {
        if (dstbtnid & 1) countByIndex[p]++;
      }
    }
    // Confirm we got a DPAD and at least one thumb button.
    if ((have & DPAD) !== DPAD) return null;
    if (!(have & (InputManagerButton.SOUTH | InputManagerButton.WEST | InputManagerButton.EAST | InputManagerButton.NORTH))) return null;
    this.joyconfigs.push(config);
    this.joyconfigsDirty = true;
    return config;
  }
  
  private applyJoyconfig(joystick: InputManagerJoystick, config: InputManagerJoystickConfig): void {
    const HORZ = InputManagerButton.LEFT | InputManagerButton.RIGHT;
    const VERT = InputManagerButton.UP | InputManagerButton.DOWN;
    const HAT = HORZ | VERT;
    for (let p=0; ; p++) {
      const btn = egg.input_device_get_button(joystick.devid, p);
      if (!btn) break;
      const bcfg = config.buttons.find(q => q.srcbtnid === btn.btnid);
      if (!bcfg) continue;
      const jbtn: InputManagerJoystickButton = {
        srcbtnid: btn.btnid,
        srcvalue: 0,
        mode: InputManagerJoystickButtonMode.RAW,
        srclo: 0,
        srchi: 0,
        dstbtnid: bcfg.dstbtnid,
        dstvalue: 0,
      };
      switch (bcfg.dstbtnid) {
        case HAT: if (btn.lo + 7 === btn.hi) {
            jbtn.srcvalue = btn.lo - 1;
            jbtn.mode = InputManagerJoystickButtonMode.HAT;
            jbtn.srclo = btn.lo;
            jbtn.srchi = btn.hi;
            jbtn.dstbtnid = HAT;
            jbtn.dstvalue = -1;
          } break;
        case HORZ:
        case VERT: if (btn.lo + 2 <= btn.hi) {
            let mid = (btn.lo + btn.hi) >> 1;
            let midlo = (btn.lo + mid) >> 1;
            let midhi = (btn.hi + mid) >> 1;
            if (midlo >= mid) midlo = mid - 1;
            if (midhi <= mid) midhi = mid + 1;
            jbtn.srcvalue = mid;
            jbtn.mode = InputManagerJoystickButtonMode.THREEWAY;
            jbtn.srclo = midlo;
            jbtn.srchi = midhi;
            jbtn.dstbtnid = bcfg.dstbtnid;
          } break;
        case InputManagerButton.LEFT:
        case InputManagerButton.RIGHT:
        case InputManagerButton.UP:
        case InputManagerButton.DOWN:
        case InputManagerButton.SOUTH:
        case InputManagerButton.WEST:
        case InputManagerButton.EAST:
        case InputManagerButton.NORTH:
        case InputManagerButton.L1:
        case InputManagerButton.R1:
        case InputManagerButton.L2:
        case InputManagerButton.R2:
        case InputManagerButton.AUX1:
        case InputManagerButton.AUX2:
        case InputManagerButton.AUX3: if (btn.lo < btn.hi) {
            jbtn.srcvalue = btn.lo;
            jbtn.mode = InputManagerJoystickButtonMode.TWOSTATE;
            jbtn.srclo = btn.lo + 1;
            jbtn.srchi = btn.hi;
            jbtn.dstbtnid = bcfg.dstbtnid;
          } break;
        case 0: break;
        case InputManagerButton.CD: break;
        default: {
            jbtn.mode = InputManagerJoystickButtonMode.SIGNAL;
            jbtn.dstbtnid = bcfg.dstbtnid;
          }
      }
      if (jbtn.mode === InputManagerJoystickButtonMode.RAW) continue;
      joystick.buttons.push(jbtn);
    }
  }
  
  private refreshJoystickAssignments(): void {
    // Count joysticks per playerid:
    const countByPlayerId = this.playerStates.map(v => 0);
    countByPlayerId.push(0);
    for (const joystick of this.joysticks) {
      if (countByPlayerId[joystick.playerid]) countByPlayerId[joystick.playerid]++;
      else countByPlayerId[joystick.playerid] = 1;
    }
    // Any joysticks assigned to invalid playerid, force them valid: 
    for (const joystick of this.joysticks) {
      if ((joystick.playerid < 1) || (joystick.playerid > this.playerStates.length)) {
        let nplayerid = 1;
        for (let q=this.playerStates.length+1; q-->1; ) {
          if (countByPlayerId[q] < countByPlayerId[nplayerid]) {
            nplayerid = q;
          }
        }
        joystick.playerid = nplayerid;
        countByPlayerId[nplayerid]++;
      }
    }
    // If a player has no joysticks, and another has more than one, yoink one:
    for (;;) {
      let zerop = -1, multip = -1;
      for (let i=this.playerStates.length+1; i-->1; ) {
        if (!countByPlayerId[i]) {
          zerop = i;
        } else if (countByPlayerId[i] > 1) {
          multip = i;
        }
      }
      if ((zerop < 0) || (multip < 0)) break;
      for (const joystick of this.joysticks) {
        if (joystick.playerid !== multip) continue;
        joystick.playerid = zerop;
        countByPlayerId[zerop] = 1;
        countByPlayerId[multip]--;
        break;
      }
    }
    // Finally, update CD bits:
    const lost = [...this.playerStates];
    for (const joystick of this.joysticks) {
      if (this.playerStates[joystick.playerid - 1] & InputManagerButton.CD) continue;
      this.setPlayerButton(joystick.playerid, InputManagerButton.CD, 1);
      lost[joystick.playerid - 1] = 0;
    }
    for (let i=lost.length; i-->0; ) {
      if (!lost[i]) continue;
      for (let btnid=1; btnid<0x10000; btnid<<=1) {
        if (!(lost[i] & btnid)) continue;
        lost[i] &= ~btnid;
        this.setPlayerButton(i + 1, InputManagerButton.CD, 0);
      }
    }
  }
  
  private loadJoyconfigs(): void {
    let src: InputManagerJoystickConfig[];
    try {
      src = JSON.parse(egg.store_get("joyconfig"));
    } catch (e) {}
    if (src instanceof Array) {
      this.joyconfigs = src.map(sercfg => ({
        name: sercfg.name || "",
        vid: sercfg.vid || 0,
        pid: sercfg.pid || 0,
        version: sercfg.version || 0,
        buttons: ((sercfg.buttons || []) as any).map(([srcbtnid, dstbtnid]) => ({srcbtnid, dstbtnid})),
      }));
    }
    this.keyconfig = [];
    try {
      const keyconfig = JSON.parse(egg.store_get("keyconfig"));
      if (keyconfig instanceof Array) {
        this.keyconfig = keyconfig;
      } else {
        this.keyconfig = [];
      }
    } catch (e) {}
    if (!this.keyconfig.length) this.keyconfig = this.defaultKeyconfig();
  }
  
  private saveJoyconfigs(): void {
    try {
      const serial = JSON.stringify(this.joyconfigs.map(cfg => ({
        name: cfg.name,
        vid: cfg.vid,
        pid: cfg.pid,
        version: cfg.version,
        buttons: cfg.buttons.map(({srcbtnid, dstbtnid}) => [srcbtnid, dstbtnid]),
      })));
      egg.store_set("joyconfig", serial);
    } catch (e) {}
    try {
      const serial = JSON.stringify(this.keyconfig);
      egg.store_set("keyconfig", serial);
    } catch (e) {}
  }
  
  private setPlayerButton(playerid: number, btnid: number, value: number): void {
    if ((playerid < 1) || (playerid > this.playerStates.length)) return;
    let state = this.playerStates[playerid - 1];
    if (value) {
      if (state & btnid) return;
      state |= btnid;
    } else {
      if (!(state & btnid)) return;
      state &= ~btnid;
    }
    this.playerStates[playerid - 1] = state;
    for (const listener of this.playerListeners) {
      if (!listener.playerid || (listener.playerid === playerid)) {
        listener.cb({ playerid, btnid, value, state });
      }
    }
    if (this.fakePointer) {
      switch (btnid) {
        case InputManagerButton.LEFT: this.onFakePointerMotion(-1, 0, value); break;
        case InputManagerButton.RIGHT: this.onFakePointerMotion(1, 0, value); break;
        case InputManagerButton.UP: this.onFakePointerMotion(0, -1, value); break;
        case InputManagerButton.DOWN: this.onFakePointerMotion(0, 1, value); break;
        case InputManagerButton.SOUTH: this.onFakePointerButton(value); break;
      }
    } else if (this.fakeKeyboard) {
      if (value) switch (btnid) {
        case InputManagerButton.LEFT: this.fakeKeyboardStep(-1, 0); break;
        case InputManagerButton.RIGHT: this.fakeKeyboardStep(1, 0); break;
        case InputManagerButton.UP: this.fakeKeyboardStep(0, -1); break;
        case InputManagerButton.DOWN: this.fakeKeyboardStep(0, 1); break;
        case InputManagerButton.SOUTH: this.fakeKeyboardMbutton(NaN, NaN); break;
      }
    }
  }
  
  /* Keyboard.
   *************************************************************************/
   
  private onKey(keycode: number, value: number): void {
    if (this.jcfgState) this.jcfgKey(keycode, value);
    // Send the verbatim event no matter what.
    this.broadcast({ type: egg.EventType.KEY, keycode, value });
    switch (this.getMode()) {
      case "raw":
      case "natural": break;
      case "pointer": {
          if (this.fakePointer) {
            switch (keycode) {
              case 0x0007002c: this.onFakePointerButton(value); break; // space
              case 0x0007004f: this.onFakePointerMotion( 1, 0, value); break;
              case 0x00070050: this.onFakePointerMotion(-1, 0, value); break;
              case 0x00070051: this.onFakePointerMotion( 0, 1, value); break;
              case 0x00070052: this.onFakePointerMotion( 0,-1, value); break;
            }
          }
        } break;
      case "text": {
          if ((this.textState === egg.EventState.ENABLED) || (this.textState === egg.EventState.REQUIRED)) {
            // We're getting TEXT events from the platform, so no need to fake it.
          } else if ((keycode === 0x000700e1) || (keycode === 0x000700e5)) { // shift
              this.shiftKey = !!value;
          } else if (value) {
            const codepoint = this.codepointFromKeycode(keycode, this.shiftKey);
            if (codepoint) {
              this.onText(codepoint);
            }
          }
        } break;
      case "joystick": if ((value === 0) || (value === 1)) {
          const cfg = this.keyconfig.find(kc => kc[0] === keycode);
          if (cfg) {
            this.setPlayerButton(1, cfg[1], value);
          }
        } break;
    }
  }
  
  private onText(codepoint: number): void {
    if (this.fakeKeyboard) switch (codepoint) {
      case 0x01: this.fakeKeyboardNextPage(); return;
    }
    this.broadcast({ type: egg.EventType.TEXT, codepoint });
  }
  
  private codepointFromKeycode(keycode: number, shift: boolean): number {
    if ((keycode >= 0x00070004) && (keycode <= 0x0007001d)) { // letters
      return keycode - 0x00070004 + (shift ? 0x41 : 0x61);
    }
    if (!shift && (keycode >= 0x0007001e) && (keycode <= 0x00070026)) { // digits 1..9
      return keycode - 0x0007001e + 0x31;
    }
    if ((keycode >= 0x00070059) && (keycode <= 0x00070061)) { // digits 1..9 (keypad)
      return keycode - 0x00070059 + 0x31;
    }
    switch (keycode) {
      // Keyboard (US layout):
      case 0x0007001e: return 0x21; // 1 !
      case 0x0007001f: return 0x40; // 2 @
      case 0x00070020: return 0x23; // 3 #
      case 0x00070021: return 0x24; // 4 $
      case 0x00070022: return 0x25; // 5 %
      case 0x00070023: return 0x5e; // 6 ^
      case 0x00070024: return 0x26; // 7 &
      case 0x00070025: return 0x2a; // 8 *
      case 0x00070026: return 0x28; // 9 (
      case 0x00070027: return shift ? 0x29 : 0x30; // 0
      case 0x00070028: return 0x0a; // enter
      case 0x00070029: return 0x1b; // escape
      case 0x0007002a: return 0x08; // backspace
      case 0x0007002b: return 0x09; // tab
      case 0x0007002c: return 0x20; // space
      case 0x0007002d: return shift ? 0x5f : 0x2d; // -
      case 0x0007002e: return shift ? 0x3d : 0x2b; // =
      case 0x0007002f: return shift ? 0x5b : 0x7b; // [
      case 0x00070030: return shift ? 0x5d : 0x7d; // ]
      case 0x00070031: return shift ? 0x5c : 0x7c; // \ 
      case 0x00070033: return shift ? 0x3b : 0x3a; // ;
      case 0x00070034: return shift ? 0x27 : 0x22; // '
      case 0x00070035: return shift ? 0x60 : 0x7e; // `
      case 0x00070036: return shift ? 0x2c : 0x3c; // ,
      case 0x00070037: return shift ? 0x2e : 0x3e; // .
      case 0x00070038: return shift ? 0x2f : 0x3f; // /
      // Keypad:
      case 0x00070054: return 0x2f; // /
      case 0x00070055: return 0x2a; // *
      case 0x00070056: return 0x2d; // -
      case 0x00070057: return 0x2b; // +
      case 0x00070058: return 0x0a; // enter
      case 0x00070062: return 0x30; // 0
      case 0x00070063: return 0x2e; // .
    }
    return 0;
  }
  
  private defaultKeyconfig(): [number, number][] {
    return [
      [0x00070004,InputManagerButton.LEFT], // a
      [0x00070006,InputManagerButton.EAST], // c
      [0x00070007,InputManagerButton.RIGHT], // d
      [0x00070016,InputManagerButton.DOWN], // s
      [0x00070019,InputManagerButton.NORTH], // v
      [0x0007001a,InputManagerButton.UP], // w
      [0x0007001b,InputManagerButton.WEST], // x
      [0x0007001d,InputManagerButton.SOUTH], // z
      [0x00070028,InputManagerButton.AUX1], // enter
      [0x0007002a,InputManagerButton.R2], // backspace
      [0x0007002b,InputManagerButton.L1], // tab
      [0x0007002c,InputManagerButton.SOUTH], // space
      [0x0007002f,InputManagerButton.AUX3], // open bracket
      [0x00070030,InputManagerButton.AUX2], // close bracket
      [0x00070031,InputManagerButton.R1], // backslash
      [0x00070035,InputManagerButton.L2], // grave
      [0x00070036,InputManagerButton.WEST], // comma
      [0x00070037,InputManagerButton.EAST], // dot
      [0x00070038,InputManagerButton.NORTH], // slash
      [0x0007004f,InputManagerButton.RIGHT], // right
      [0x00070050,InputManagerButton.LEFT], // left
      [0x00070051,InputManagerButton.DOWN], // down
      [0x00070052,InputManagerButton.UP], // up
      [0x00070054,InputManagerButton.AUX1], // kp slash
      [0x00070055,InputManagerButton.AUX2], // kp star
      [0x00070056,InputManagerButton.AUX3], // kp dash
      [0x00070057,InputManagerButton.EAST], // kp plus
      [0x00070058,InputManagerButton.WEST], // kp enter
      [0x00070059,InputManagerButton.L2], // kp 1
      [0x0007005a,InputManagerButton.DOWN], // kp 2
      [0x0007005b,InputManagerButton.R2], // kp 3
      [0x0007005c,InputManagerButton.LEFT], // kp 4
      [0x0007005d,InputManagerButton.DOWN], // kp 5
      [0x0007005e,InputManagerButton.RIGHT], // kp 6
      [0x0007005f,InputManagerButton.L1], // kp 7
      [0x00070060,InputManagerButton.UP], // kp 8
      [0x00070061,InputManagerButton.R1], // kp 9
      [0x00070062,InputManagerButton.SOUTH], // kp 0
      [0x00070063,InputManagerButton.NORTH], // kp dot
    ];
  }
  
  private fakeKeyboardNextPage(): void {
    this.fakeKeyboard.page++;
    if (this.fakeKeyboard.page >= this.fakeKeyboardLayout.length) {
      this.fakeKeyboard.page = 0;
    }
  }
  
  private beginFakeKeyboard(): void {
    const tex = egg.texture_get_header(this.texid_font);
    const glyphw = tex.w >> 4;
    const glyphh = tex.h >> 4;
    this.fakeKeyboardColw = glyphw * 2;
    this.fakeKeyboardRowh = glyphh * 2;
    this.fakeKeyboard = { page: 0, row: 0, col: 0 };
    let tileCount = 0;
    for (const page of this.fakeKeyboardLayout) {
      const rowc = page.length;
      const colc = Math.max(...page.map(r => r.length));
      tileCount = Math.max(tileCount, rowc * colc);
    }
    if (!this.fakeKeyboardTiles || (tileCount * 6 > this.fakeKeyboardTiles.length)) {
      this.fakeKeyboardTiles = new Uint8Array(tileCount * 6);
    }
  }
  
  private fakeKeyboardStep(dx: number, dy: number): void {
    const page = this.fakeKeyboardLayout[this.fakeKeyboard.page];
    if (!page) return;
    if (dy) {
      this.fakeKeyboard.row += dy;
      if (this.fakeKeyboard.row < 0) this.fakeKeyboard.row = page.length - 1;
      else if (this.fakeKeyboard.row >= page.length) this.fakeKeyboard.row = 0;
      return;
    }
    const trow = page[this.fakeKeyboard.row];
    if (!trow) return;
    this.fakeKeyboard.col += dx;
    if (this.fakeKeyboard.col < 0) this.fakeKeyboard.col = trow.length - 1;
    else if (this.fakeKeyboard.col >= trow.length) this.fakeKeyboard.col = 0;
  }
  
  private fakeKeyboardMmotion(x: number, y: number): void {
    const page = this.fakeKeyboardLayout[this.fakeKeyboard.page];
    const fullw = this.fakeKeyboardColw * page[0].length;
    const fullh = this.fakeKeyboardRowh * page.length;
    const fullx = (this.screenw >> 1) - (fullw >> 1);
    const fully = this.screenh - fullh - 10;
    const col = Math.floor((x - fullx) / this.fakeKeyboardColw);
    const row = Math.floor((y - fully) / this.fakeKeyboardRowh);
    if ((col < 0) || (row < 0)) return;
    if (!page || (row >= page.length)) return;
    const trow = page[this.fakeKeyboard.row];
    if (!trow || (col >= trow.length)) return;
    this.fakeKeyboard.col = col;
    this.fakeKeyboard.row = row;
  }
  
  private fakeKeyboardMbutton(x: number, y: number): void {
    const page = this.fakeKeyboardLayout[this.fakeKeyboard.page];
    let col: number;
    let row: number;
    if (isNaN(x) && isNaN(y)) {
      col = this.fakeKeyboard.col;
      row = this.fakeKeyboard.row;
    } else {
      const fullw = this.fakeKeyboardColw * page[0].length;
      const fullh = this.fakeKeyboardRowh * page.length;
      const fullx = (this.screenw >> 1) - (fullw >> 1);
      const fully = this.screenh - fullh - 10;
      col = Math.floor((x - fullx) / this.fakeKeyboardColw);
      row = Math.floor((y - fully) / this.fakeKeyboardRowh);
      if ((col < 0) || (row < 0)) return;
    }
    if (!page || (row >= page.length)) return;
    const trow = page[this.fakeKeyboard.row];
    if (!trow || (col >= trow.length)) return;
    const codepoint = trow.charCodeAt(col);
    this.onText(codepoint);
  }
  
  private renderFakeKeyboard(): void {
    const page = this.fakeKeyboardLayout[this.fakeKeyboard.page];
    if (!page) return;
    const row = page[this.fakeKeyboard.row];
    if (!row) return;
    const fullw = this.fakeKeyboardColw * row.length;
    const fullh = this.fakeKeyboardRowh * page.length;
    const fullx = (this.screenw >> 1) - (fullw >> 1);
    const fully = this.screenh - fullh - 10; //TODO Option to toggle top/bottom. Should that be managed by us? I think so.
    egg.draw_rect(1, fullx - 1, fully - 1, fullw + 2, fullh + 2, 0xffffffff);
    egg.draw_rect(1, fullx, fully, fullw, fullh, 0x808080ff);
    if (true) { // TODO If we only have touch -- no mouse or fakePointer -- then don't draw the cell highlight.
      egg.draw_rect(1,
        fullx + this.fakeKeyboard.col * this.fakeKeyboardColw,
        fully + this.fakeKeyboard.row * this.fakeKeyboardRowh,
        this.fakeKeyboardColw,
        this.fakeKeyboardRowh,
        0x90d0ffff,
      );
    }
    let tileCount = 0;
    const tiles = this.fakeKeyboardTiles;
    for (let yi=0, tilesp=0; yi<page.length; yi++) {
      const rrow = page[yi];
      const y = fully + yi * this.fakeKeyboardRowh + (this.fakeKeyboardRowh >> 1);
      for (let xi=0, x=fullx + (this.fakeKeyboardColw >> 1); xi<rrow.length; xi++, x+=this.fakeKeyboardColw) {
        tiles[tilesp++] = x;
        tiles[tilesp++] = x >> 8;
        tiles[tilesp++] = y;
        tiles[tilesp++] = y >> 8;
        tiles[tilesp++] = rrow.charCodeAt(xi);
        tiles[tilesp++] = 0;
        tileCount++;
      }
    }
    egg.draw_mode(egg.XferMode.ALPHA, 0x000000ff, 0xff);
    egg.draw_tile(1, this.texid_font, this.fakeKeyboardTiles.buffer, tileCount);
    egg.draw_mode(egg.XferMode.ALPHA, 0, 0xff);
  }
  
  /* Mouse.
   ***********************************************************************/
   
  private onMmotion(x: number, y: number): void {
    if (this.fakeKeyboard) {
      this.fakeKeyboardMmotion(x, y);
    } else {
      this.broadcast({ type: egg.EventType.MMOTION, x, y });
    }
  }
  
  private onMbutton(btnid: number, value: number, x: number, y: number): void {
    if (this.fakeKeyboard && (btnid === 1)) {
      if (value === 1) this.fakeKeyboardMbutton(x, y);
    } else {
      this.broadcast({ type: egg.EventType.MBUTTON, btnid, value, x, y });
    }
  }
  
  private onMwheel(dx: number, dy: number, x: number, y: number): void {
    this.broadcast({ type: egg.EventType.MWHEEL, dx, dy, x, y });
  }
  
  private onFakePointerMotion(dx: number, dy: number, value: number): void {
    if (value) {
      if (dx) this.fakePointer.dx = dx;
      else this.fakePointer.dy = dy;
    } else {
      if (this.fakePointer.dx === dx) this.fakePointer.dx = 0;
      if (this.fakePointer.dy === dy) this.fakePointer.dy = 0;
    }
  }
  
  private onFakePointerButton(value: number): void {
    if (value) {
      if (this.fakePointer.button) return;
      this.fakePointer.button = true;
    } else {
      if (!this.fakePointer.button) return;
      this.fakePointer.button = false;
    }
    this.onMbutton(1, value ? 1 : 0, ~~this.fakePointer.x, ~~this.fakePointer.y);
  }
  
  /* Touch.
   *****************************************************************/
   
  private onTouch(id: number, state: number, x: number, y: number): void {
    switch (this.getMode()) {
      case "raw":
      case "natural":
      case "joystick": this.broadcast({ type: egg.EventType.TOUCH, id, state, x, y }); break;
      case "pointer": {
          switch (state) {
            case 0: this.broadcast({ type: egg.EventType.MBUTTON, btnid: 1, value: 0, x, y }); break;
            case 1: this.broadcast({ type: egg.EventType.MBUTTON, btnid: 1, value: 1, x, y }); break;
            case 2: this.broadcast({ type: egg.EventType.MMOTION, x, y }); break;
          }
        } break;
      case "text": {
          //TODO onscreen keyboard
        } break;
    }
  }
  
  /* Accelerometer.
   ****************************************************************/
   
  private onAccelerometer(x16: number, y16: number, z16: number): void {
    this.broadcast({ type: egg.EventType.ACCELEROMETER, x: x16 / 65536, y: y16 / 65536, z: z16 / 65536 });
  }
  
  /* Network.
   ***************************************************************/
   
  private onHttpRsp(reqid: number, status: number, length: number): void {
    const body = egg.http_get_body(reqid);
    this.broadcast({ type: egg.EventType.HTTP_RSP, reqid, status, length, body });
    for (let i=this.httpPending.length; i-->0; ) {
      const pending = this.httpPending[i];
      if (pending.reqid !== reqid) continue;
      this.httpPending.splice(i, 1);
      if ((status >= 200) && (status < 300)) {
        pending.resolve({ status, body });
      } else {
        pending.reject({ status, body });
      }
      return;
    }
  }
  
  private onWsConnect(wsid: number): void {
    this.broadcast({ type: egg.EventType.WS_CONNECT, wsid });
    for (const listener of this.wsListeners) {
      if (listener.wsid !== wsid) continue;
      listener.cb(1);
    }
  }
  
  private onWsDisconnect(wsid: number): void {
    this.broadcast({ type: egg.EventType.WS_DISCONNECT, wsid });
    for (const listener of [...this.wsListeners]) {
      if (listener.wsid !== wsid) continue;
      listener.cb(0);
    }
  }
  
  private onWsMessage(wsid: number, msgid: number, length: number): void {
    const message = egg.ws_get_message(wsid, msgid);
    this.broadcast({ type: egg.EventType.WS_MESSAGE, wsid, msgid, length, message });
    for (const listener of this.wsListeners) {
      if (listener.wsid !== wsid) continue;
      listener.cb(message);
    }
  }
}
