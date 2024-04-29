import { JoyTwoState } from "./JoyTwoState";
import { JoyLogical } from "./JoyLogical";
import { JoyQuery } from "./JoyQuery";
import { FakeKeyboard } from "./FakeKeyboard";
import { FakePointer } from "./FakePointer";
import { Font } from "../utility/Font"; // Optional, safe to remove if you're not using it.

/**
 * Global input manager and event bus.
 * Since we take control of the event loop, we also supply handling for network calls and timeouts.
 */
export class Bus {
  private nextId = 1;
  private clock = 0;
  private listeners: { id: number; cb: (event: egg.Event) => void; mask: number; }[] = [];
  private timeouts: { id: number; cb: () => void; when: number; }[] = [];
  private httpListeners: {
    reqid: number;
    resolve: (rsp: { status: number; body: ArrayBuffer | null; }) => void;
    reject: (rsp: { status: number; body: ArrayBuffer | null; }) => void;
  }[] = [];
  private wsListeners: {
    id: number;
    wsid: number;
    url: string;
    connected: boolean;
    cb: (message: string | 0 | 1) => void;
  }[] = [];
  
  public font: Font | null = null;
  public joyTwoState = new JoyTwoState(this);
  public joyLogical = new JoyLogical(this);
  public joyQuery = new JoyQuery(this);
  public fakeKeyboard = new FakeKeyboard(this);
  public fakePointer = new FakePointer(this);
  
  /*----- Setup. -----*/
  
  constructor() {
  }
  
  /**
   * If you supply a Font, JoyQuery and FakeKeyboard will both use it in preference to fontTilesheet.
   */
  setFont(font: Font | null): void {
    this.font = font;
  }
  
  /**
   * Must be an image containing 16 rows and 16 columns, squares, corresponding to codepoints 0..0xff.
   * The fake keyboard and joystick configurer depend on it.
   */
  setFontTilesheet(texid: number): void {
    this.joyQuery.setFontTilesheet(texid);
    this.fakeKeyboard.setFontTilesheet(texid);
  }
  
  /**
   * Set an image, or a portion of one, to use as the fake cursor.
   * If you supply (0,0) as the size, we'll take the entire image.
   * (hotX,hotY) should be in (0,0)..(w-1,h-1).
   */
  setCursor(
    texid: number,
    x: number, y: number, w: number, h: number,
    hotX: number, hotY: number
  ): void {
    if (texid && (!w || !h)) {
      const hdr = egg.texture_get_header(texid);
      if (!w) w = hdr.w - x;
      if (!h) h = hdr.h - y;
    }
    this.fakePointer.setCursor(texid, x, y, w, h, hotX, hotY);
  }
  
  /**
   * To enable high-level mapping of joystick events, you must tell us how many buttons and what to call them.
   *
   * Index in `names` corresponds little-endianly to bits in the player state.
   * You get to make these up.
   *
   * Index in `standardNames` corresponds to button indices in the Standard Mapping:
   *   [
   *     South, East, West, North,
   *     L1, R1, L2, R2,
   *     AuxLeft, AuxRight,
   *     LeftPlunger, RightPlunger,
   *     Up, Down, Left, Right,
   *     AuxCenter
   *   ]
   *
   * Use the same strings in both arrays, then we can map obvious things like the dpad without any help.
   */
  setButtonNames(
    names: string[],
    standardNames?: string[]
  ): void {
    this.joyLogical.setButtonNames(names, standardNames);
    this.joyQuery.setButtonNames(names, standardNames);
    this.fakeKeyboard.setButtonNames(names, standardNames);
    this.fakePointer.setButtonNames(names, standardNames);
  }
  
  /**
   * Turn off all mapping features.
   */
  rawInputOnly(): void {
    this.joyLogical.enable(false);
    this.fakeKeyboard.enable(false);
    this.fakePointer.enable(false);
  }
  
  /**
   * Begin mapping everything to logical player inputs.
   * I feel this is the appropriate mode for most games.
   * Text and pointer input will not be available.
   */
  requireJoysticks(): boolean {
    if (!this.joyLogical.canOperate()) return false;
    this.fakeKeyboard.enable(false);
    this.fakePointer.enable(false);
    this.joyLogical.enable(true);
    return true;
  }
  
  /**
   * Present a fake keyboard if possible, and accept text from that and the system keyboard.
   * Recommend using this for short text collections eg "what is your name?".
   */
  requireText(): boolean {
    if (!this.fakeKeyboard.canOperate()) return false;
    this.fakePointer.enable(false);
    this.joyLogical.enable(false);
    this.fakeKeyboard.enable(true);
    return true;
  }
  
  /**
   * Present a fake cursor and report MMOTION, MBUTTON, and MWHEEL events.
   * Pointer can be controlled via mouse, keyboard, joysticks, or touch, to some extent.
   * It's sensible to leave this mode on permanently, for point-and-click games like Maniac Mansion eg.
   */
  requirePointer(): boolean {
    if (!this.fakePointer.canOperate()) return false;
    this.joyLogical.enable(false);
    this.fakeKeyboard.enable(false);
    this.fakePointer.enable(true);
    return true;
  }
  
  /**
   * Begin modal interactive configuration for one input device.
   * `devid` is -1 for the keyboard, or a positive connected devid.
   */
  beginJoyQuery(devid: number): boolean {
    if (!this.joyQuery.begin(devid)) return false;
    this.fakeKeyboard.enable(false);
    this.fakePointer.enable(false);
    return true;
  }
  
  /**
   * Request a callback whenever the given events happen.
   * You may supply an event mask (`1 << egg.EventType`), or symbolic names.
   * Returns a positive ID that you can use to unlisten later.
   */
  listen(events: number | string | string[], cb: (event: egg.Event) => void): number {
    const mask = this.eventMaskFromAnything(events);
    if (!mask) return 0;
    const id = this.nextId++;
    this.listeners.push({ id, mask, cb });
    return id;
  }
  
  /**
   * Remove an event listener.
   */
  unlisten(id: number): void {
    const listener = this.listeners.find(l => l.id === id);
    if (!listener) return;
    listener.id = 0;
    listener.cb = (event) => {};
  }
  
  eventMaskFromAnything(input: number | string | string[]): number {
    switch (typeof(input)) {
      case "number": return input;
      case "string": {
          const t = this.evalEventType(input);
          if (!t) return 0;
          if (t === 0x7fffffff) return t;
          return 1 << t;
        }
      case "object": {
          if (!input) return 0;
          if (input instanceof Array) return input.reduce((a, v) => {
            const t = this.evalEventType(v);
            if (t) return a | (1 << t);
            return a;
          }, 0);
        } break;
    }
    return 0;
  }
  
  evalEventType(input: string): number {
    switch (input.toUpperCase()) {
      case "ALL": return 0x7fffffff;
      case "INPUT": return egg.EventType.INPUT;
      case "CONNECT": return egg.EventType.CONNECT;
      case "DISCONNECT": return egg.EventType.DISCONNECT;
      case "HTTP_RSP": return egg.EventType.HTTP_RSP;
      case "WS_CONNECT": return egg.EventType.WS_CONNECT;
      case "WS_DISCONNECT": return egg.EventType.WS_DISCONNECT;
      case "WS_MESSAGE": return egg.EventType.WS_MESSAGE;
      case "MMOTION": return egg.EventType.MMOTION;
      case "MBUTTON": return egg.EventType.MBUTTON;
      case "MWHEEL": return egg.EventType.MWHEEL;
      case "KEY": return egg.EventType.KEY;
      case "TEXT": return egg.EventType.TEXT;
      case "TOUCH": return egg.EventType.TOUCH;
      case "ACCELEROMETER": return egg.EventType.ACCELEROMETER;
    }
    return 0;
  }
  
  /*----- Event loop. -----*/
  
  /**
   * Call on each client update.
   */
  update(elapsed: number): void {
    this.clock += elapsed;
    this.checkTimeouts();
    this.dropDefunctListeners();
    this.updatePlugins(elapsed);
    for (const event of egg.event_next()) this.onEvent(event);
  }
  
  /**
   * Call at the end of each client render.
   * We might have plugins like a fake keyboard or pointer that expect to render above your content.
   */
  render(): void {
    this.renderPlugins();
  }
  
  /**
   * Normally only called from within Bus, but you can insert fake events too.
   */
  onEvent(event: egg.Event): void {
    switch (event.eventType) {
      //case egg.EventType.INPUT: devid,btnid,value
      //case egg.EventType.CONNECT: devid,mapping
      //case egg.EventType.DISCONNECT: devid
      case egg.EventType.HTTP_RSP: this.onHttpResponse(event.v0, event.v1, event.v2); break;
      case egg.EventType.WS_CONNECT: this.onWsConnect(event.v0); break;
      case egg.EventType.WS_DISCONNECT: this.onWsDisconnect(event.v0); break;
      case egg.EventType.WS_MESSAGE: this.onWsMessage(event.v0, event.v1, event.v2); break;
      //case egg.EventType.MMOTION: x,y
      //case egg.EventType.MBUTTON: btnid,value,x,y
      //case egg.EventType.MWHEEL: dx,dy,x,y
      //case egg.EventType.KEY: keycode,value
      //case egg.EventType.TEXT: codepoint
      //case egg.EventType.TOUCH: id,state,x,y
      //case egg.EventType.ACCELEROMETER: x,y,z
    }
    const bit = 1 << event.eventType;
    for (const { cb, mask } of this.listeners) {
      if (!(mask & bit)) continue;
      cb(event);
    }
  }
  
  /*----- Timeouts. -----*/
  
  /**
   * Behaves basically like `Window.setTimeout`, which is not available to Egg games.
   * Two gotchas:
   *  - Callback will trigger during some future `Bus.update()`.
   *  - Time is in game time, which is not necessarily real time. But it's usually close.
   */
  setTimeout(cb: () => void, ms: number): number {
    const id = this.nextId++;
    this.timeouts.push({ id, cb, when: this.clock + ms / 1000 });
    return id;
  }
  
  /**
   * Prevent a timeout from firing, if it hasn't yet.
   */
  clearTimeout(id: number): void {
    const timeout = this.timeouts.find(t => t.id === id);
    if (timeout) {
      timeout.id = 0;
      timeout.cb = () => {};
    }
  }
  
  private checkTimeouts(): void {
    for (let i=this.timeouts.length; i-->0; ) {
      const timeout = this.timeouts[i];
      if (timeout.when > this.clock) continue;
      this.timeouts.splice(i, 1);
      timeout.cb();
    }
  }
  
  /*----- Network. -----*/
  
  /**
   * Begin an HTTP request.
   * The returned Promise rejects on status codes outside 200..299.
   * Rejections get exactly the same arguments as resolutions.
   * It's reasonable to `httpRequest(...).catch(r => r).then(r => ...)`.
   */
  httpRequest(
    method: string,
    url: string,
    body?: string | ArrayBuffer
  ): Promise<{ status: number; body: ArrayBuffer | null; }> {
    const reqid = egg.http_request(method, url, body);
    if (reqid < 1) return Promise.reject({ status: 0, body: null });
    return new Promise((resolve, reject) => {
      this.httpListeners.push({ reqid, resolve, reject });
    });
  }
  
  /**
   * Subscribe to a WebSocket, and open it if not open yet.
   * Multiple subscribers to the *exact* same URL will share a connection.
   * Your callback will receive `1` on connect, `0` on disconnect, and strings in between.
   * Note that the ID we return is private to Bus; it is not the Egg "wsid".
   * Subscriptions remain open until you close them, regardless of the underlying connection.
   * If connection is synchronously rejected by the platform, you'll get zero here.
   */
  websocketOpen(
    url: string,
    cb: (message: 0 | 1 | string) => void
  ): number {
    const wsid = this.wsidForUrl(url);
    if (!wsid) return 0;
    const id = this.nextId++;
    let connected = this.wsidIsConnected(wsid);
    this.wsListeners.push({ id, wsid, url, connected, cb });
    return id;
  }
  
  /**
   * Unsubscribe from a WebSocket, and close the connection if this is its last subscriber.
   */
  websocketClose(id: number): void {
    const listener = this.wsListeners.find(l => l.id === id);
    if (!listener) return;
    listener.id = 0;
    listener.cb = (message: 0 | 1 | string) => {};
    if (this.wsListeners.find(l => l.wsid === listener.wsid)) {
      // Other listeners are present; leave the socket open.
    } else {
      egg.ws_disconnect(listener.wsid);
    }
  }
  
  /**
   * If anyone is subscribing to this url but the connection was lost,
   * try to reestablish it.
   */
  websocketPoke(url?: string): void {
    for (const listener of this.wsListeners) {
      if (listener.wsid) continue;
      if (url && (listener.url !== url)) continue;
      if ((listener.wsid = egg.ws_connect(url)) > 0) {
        this.reconnectOrphanWebsockets(listener.url, listener.wsid);
      } else {
        listener.wsid = 0;
      }
    }
  }
  
  /**
   * Send a text packet.
   * Returns true if this WebSocket is connected.
   */
  websocketSend(id: number, message: string): boolean {
    const listener = this.wsListeners.find(l => l.id === id);
    if (!listener) return false;
    if (!listener.connected) return false;
    egg.ws_send(listener.wsid, 1, message);
    return true;
  }
  
  private onHttpResponse(reqid: number, status: number, length: number): void {
    const p = this.httpListeners.findIndex(l => l.reqid === reqid);
    if (p >= 0) {
      const listener = this.httpListeners[p];
      this.httpListeners.splice(p, 1);
      const body = egg.http_get_body(reqid);
      if ((status >= 200) && (status < 300)) {
        listener.resolve({ status, body });
      } else {
        listener.reject({ status, body });
      }
    }
  }
  
  private onWsConnect(wsid: number): void {
    for (const listener of this.wsListeners) {
      if (listener.wsid !== wsid) continue;
      listener.connected = true;
      listener.cb(1);
    }
  }
  
  private onWsDisconnect(wsid: number): void {
    for (const listener of this.wsListeners) {
      if (listener.wsid !== wsid) continue;
      listener.connected = false;
      listener.wsid = 0;
      listener.cb(0);
    }
  }
  
  private onWsMessage(wsid: number, msgid: number, length: number): void {
    const message = egg.ws_get_message(wsid, msgid);
    for (const listener of this.wsListeners) {
      if (listener.wsid !== wsid) continue;
      if (!listener.connected) {
        listener.connected = true;
        listener.cb(1);
      }
      listener.cb(message);
    }
  }
  
  private wsidForUrl(url: string): number {
    let others = false;
    for (const listener of this.wsListeners) {
      if (listener.url !== url) continue;
      if (listener.wsid) return listener.wsid;
      others = true;
    }
    const wsid = egg.ws_connect(url);
    if (wsid <= 0) return 0;
    if (others) {
      for (const listener of this.wsListeners) {
        if (listener.url !== url) continue;
        listener.wsid = wsid;
      }
    }
    return wsid;
  }
  
  private wsidIsConnected(wsid: number): boolean {
    return !!this.wsListeners.find(l => ((l.wsid === wsid) && l.connected));
  }
  
  private reconnectOrphanWebsockets(url: string, wsid: number): void {
    for (const listener of this.wsListeners) {
      if (listener.wsid) continue;
      if (listener.url !== url) continue;
      listener.wsid = wsid;
    }
  }
  
  /*----- Internals. Nothing public below this point. -----*/
  
  // To nullify any listener, set its ID zero and replace its callback.
  // In general, we'll reap the zero IDs here, rather than modifying arrays on the fly.
  private dropDefunctListeners(): void {
    for (let i=this.listeners.length; i-->0; ) {
      if (this.listeners[i].id) continue;
      this.listeners.splice(i, 1);
    }
    for (let i=this.timeouts.length; i-->0; ) {
      if (this.timeouts[i].id) continue;
      this.timeouts.splice(i, 1);
    }
    // httpListeners doesn't need a defunct check.
    for (let i=this.wsListeners.length; i-->0; ) {
      if (this.wsListeners[i].id) continue;
      this.wsListeners.splice(i, 1);
    }
  }
  
  private updatePlugins(elapsed: number): void {
    if (this.joyQuery.enabled) this.joyQuery.update(elapsed);
    else if (this.fakeKeyboard.enabled) this.fakeKeyboard.update(elapsed);
    else if (this.fakePointer.enabled) this.fakePointer.update(elapsed);
  }
  
  private renderPlugins(): void {
    if (this.joyQuery.enabled) this.joyQuery.render();
    else if (this.fakeKeyboard.enabled) this.fakeKeyboard.render();
    else if (this.fakePointer.enabled) this.fakePointer.render();
  }
}
