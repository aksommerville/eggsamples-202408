/**
 * InputManager has an overall strategy of how to model input:
 *   `raw`: Deliver events how we receive them.
 *   `natural`: Map joysticks as needed, but don't do anything suprising.
 *   `joystick`: Treat the keyboard as a joystick. Text and raw key events not available.
 *   `text`: Take text from the keyboard, present an artificial keyboard if necessary. You should enter this mode temporarily, only as needed.
 *   `pointer`: Point-and-click events from mouse or touch. TODO Can we simulate with joystick and keyboard?
 */
export type InputManagerMode = "raw" | "natural" | "joystick" | "text" | "pointer";

export type InputManagerEvent = {
  type: egg.EventType;
} | {
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

export interface InputManagerListener {
  id: number;
  cb: (event: InputManagerEvent) => void,
  events: number; // Multiple (1<<egg.EventType).
}

export interface InputManagerJoystick {
  devid: number;
  name: string;
  vid: number;
  pid: number;
  version: number;
  //TODO Detail for mapping.
}

export class InputManager {
  private modeStack: InputManagerMode[] = ["natural"];
  private listeners: InputManagerListener[] = [];
  private nextListenerId = 1;
  private joysticks: InputManagerJoystick[] = [];
  private textState: egg.EventState = egg.EventState.QUERY;
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

  constructor() {
    this.enterMode(this.modeStack[0]);
  }
  
  update(): void {
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
  
  http(method: string, url: string, body?: string | ArrayBuffer): Promise<any>{//Promise<{ status: number; body: ArrayBuffer | null }> {
    const reqid = egg.http_request(method, url, body);
    if (reqid < 1) return Promise.reject({
      status: 0,
      body: null,
    });
    return new Promise((resolve, reject) => {
      this.httpPending.push({ reqid, resolve, reject });
    });
  }
  
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
   
/* ----- Everything below this point is private. ----- */

  private enterMode(mode: InputManagerMode): void {
    egg.log(`InputManager.enterMode ${JSON.stringify(mode)}`);
    switch (mode) {
      case "raw": {
          this.textState = egg.event_enable(egg.EventType.TEXT, egg.EventState.DISABLED);
        } break;
      case "natural": {
          this.textState = egg.event_enable(egg.EventType.TEXT, egg.EventState.ENABLED);
        } break;
      case "joystick": {
          this.textState = egg.event_enable(egg.EventType.TEXT, egg.EventState.DISABLED);
        } break;
      case "text": {
          this.textState = egg.event_enable(egg.EventType.TEXT, egg.EventState.ENABLED);
        } break;
      case "pointer": {
          this.textState = egg.event_enable(egg.EventType.TEXT, egg.EventState.DISABLED);
        } break;
    }
  }
  
  private broadcast(event: InputManagerEvent): void {
    const bit = 1 << event.type;
    for (const listener of this.listeners) {
      if (listener.events & bit) listener.cb(event);
    }
  }
  
  /* Unknown event -- shouldn't happen, we're handling all defined events.
   ***********************************************************************/
   
  private onUnknownEvent(event: egg.Event): void {
    egg.log(`UNKNOWN ${JSON.stringify(event)}`);
  }
  
  /* Joysticks (or generic input).
   **************************************************************************/
  
  private onInput(devid: number, btnid: number, value: number): void {
    const mode = this.getMode();
    switch (mode) {
      case "raw": this.broadcast({ type: egg.EventType.INPUT, devid, btnid, value }); break;
      default: {
          //TODO map joystick event
        }
    }
  }
  
  private onConnect(devid: number): void {
    const joystick: InputManagerJoystick = {
      devid,
      name: egg.input_device_get_name(devid),
      vid: 0,
      pid: 0,
      version: 0,
      ...egg.input_device_get_ids(devid),
    };
    //TODO Prepare mapping.
    this.joysticks.push(joystick);
    this.broadcast({ type: egg.EventType.CONNECT, joystick });
  }
  
  private onDisconnect(devid: number): void {
    const p = this.joysticks.findIndex(j => j.devid === devid);
    if (p >= 0) {
      //TODO Drop any nonzero state
      this.joysticks.splice(p, 1);
    }
    this.broadcast({ type: egg.EventType.DISCONNECT, devid });
  }
  
  /* Keyboard.
   *************************************************************************/
   
  private onKey(keycode: number, value: number): void {
    switch (this.getMode()) {
      case "raw":
      case "pointer":
      case "natural": this.broadcast({ type: egg.EventType.KEY, keycode, value }); break;
      case "text": {
          // TODO Look for obvious non-text keys and report them as usual, eg ESC and F1.
          if ((this.textState === egg.EventState.ENABLED) || (this.textState === egg.EventState.REQUIRED)) {
            // We're getting TEXT events from the platform, so drop KEY events.
          } else {
            // TODO Fake keyboard mapping to text.
          }
        } break;
      case "joystick": break; //TODO map keyboard like a joystick
    }
  }
  
  private onText(codepoint: number): void {
    this.broadcast({ type: egg.EventType.TEXT, codepoint });
  }
  
  /* Mouse.
   ***********************************************************************/
   
  private onMmotion(x: number, y: number): void {
    this.broadcast({ type: egg.EventType.MMOTION, x, y });
  }
  
  private onMbutton(btnid: number, value: number, x: number, y: number): void {
    this.broadcast({ type: egg.EventType.MBUTTON, btnid, value, x, y });
    //TODO onscreen keyboard, in "text" mode?
  }
  
  private onMwheel(dx: number, dy: number, x: number, y: number): void {
    this.broadcast({ type: egg.EventType.MWHEEL, dx, dy, x, y });
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
    this.broadcast({ type: egg.EventType.WS_MESSAGE, wsid, message });
    for (const listener of this.wsListeners) {
      if (listener.wsid !== wsid) continue;
      listener.cb(message);
    }
  }
}
