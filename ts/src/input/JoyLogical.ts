import { Bus } from "./Bus";
import { Joy2Part, Joy2Device } from "./JoyTwoState";

interface LogicalDevice {
  devid: number;
  mapping: number;
  state: number;
  playerid: number;
  buttons: [number, Joy2Part, number][]; // [srcbtnid,part,dstbtnid]
}

interface LogicalMap {
  name: string;
  vid: number;
  pid: number;
  version: number;
  buttons: [number, Joy2Part, number][]; // [srcbtnid,part,dstbtnid]
}

/**
 * Maps events from JoyTwoState into a bitmap for each player, whose shape is defined by the client.
 */
export class JoyLogical {
  private nextId = 1;
  private listeners: { id: number; cb: (playerid: number, btnbit: number, value: number, state: number) => void; }[] = [];
  private privateCb: ((btnid: number, value: number) => void) | null = null;
  private privateIncludesKeyboard = false;
  private twoStateListener = 0;
  private busListener = 0;
  private enabled = false;
  private buttonNames: string[] = [];
  private standardNames: string[] = [];
  private players: number[] = [0, 0]; // Indexed by playerid; zero is the aggregate.
  private devices: LogicalDevice[] = [];
  private maps: LogicalMap[] = [];
  private suspended = false;
  
  constructor(private bus: Bus) {
    this.busListener = this.bus.listen(
      (1 << egg.EventType.CONNECT) |
      (1 << egg.EventType.DISCONNECT),
      e => this.onBusEvent(e)
    );
    this.loadMaps();
  }
  
  setPlayerCount(playerCount: number): void {
    if (playerCount < 1) return;
    const newLength = 1 + playerCount;
    while (this.players.length < newLength) this.players.push(0);
    for (let playerid=this.players.length; playerid-->newLength; ) {
      const state = this.players[playerid];
      if (state) {
        for (let i=0, bit=1; i<this.buttonNames.length; i++, bit<<=1) {
          if (state & bit) {
            this.setPlayerButton(playerid, bit, 0);
          }
        }
      }
    }
    this.players.splice(newLength, this.players.length - newLength);
  }
  
  setButtonNames(
    names: string[],
    standardNames?: string[]
  ): void {
    this.buttonNames = names;
    this.standardNames = standardNames || [];
  }
  
  listen(cb: (playerid: number, btnid: number, value: number, state: number) => void): number {
    const id = this.nextId++;
    this.listeners.push({ id, cb });
    return id;
  }
  
  unlisten(id: number): void {
    const p = this.listeners.findIndex(l => l.id === id);
    if (p < 0) return;
    this.listeners.splice(p, 1);
  }
  
  canOperate(): boolean {
    // False in the very unlikely case that (INPUT,KEY,TOUCH) are all unsupported.
    if (egg.event_enable(egg.EventType.INPUT, egg.EventState.QUERY) !== egg.EventState.IMPOSSIBLE) return true;
    if (egg.event_enable(egg.EventType.KEY, egg.EventState.QUERY) !== egg.EventState.IMPOSSIBLE) return true;
    if (egg.event_enable(egg.EventType.TOUCH, egg.EventState.QUERY) !== egg.EventState.IMPOSSIBLE) return true;
    return false;
  }
  
  enable(enable: boolean): void {
    this.privateCb = null;
    if (enable) {
      if (this.enabled) return;
      this.enabled = true;
      this.twoStateListener = this.bus.joyTwoState.listen((d, b, p, v) => this.onTwoState(d, b, p, v));
      egg.event_enable(egg.EventType.INPUT, egg.EventState.ENABLED);
      egg.event_enable(egg.EventType.KEY, egg.EventState.ENABLED);
      egg.event_enable(egg.EventType.TOUCH, egg.EventState.ENABLED);
    } else {
      if (!this.enabled) return;
      this.enabled = false;
      this.bus.joyTwoState.unlisten(this.twoStateListener);
      this.twoStateListener = 0;
      // Don't disable INPUT or KEY. It should be on always anyway, but we enable kind of as commentary.
      egg.event_enable(egg.EventType.TOUCH, egg.EventState.DISABLED);
    }
  }
  
  enablePrivate(includeKeyboard: boolean, cb: (btnid: number, value: number) => void): void {
    this.enabled = true;
    this.privateCb = cb;
    this.privateIncludesKeyboard = includeKeyboard;
    if (!this.twoStateListener) {
      this.twoStateListener = this.bus.joyTwoState.listen((d, b, p, v) => this.onTwoState(d, b, p, v));
    }
    egg.event_enable(egg.EventType.INPUT, egg.EventState.ENABLED);
  }
  
  suspend(): void {
    if (this.suspended) return;
    for (let playerid=this.players.length; playerid-->0; ) {
      const state = this.players[playerid];
      if (state) {
        for (let i=0, bit=1; i<this.buttonNames.length; i++, bit<<=1) {
          if (state & bit) {
            this.setPlayerButton(playerid, bit, 0);
          }
        }
      }
    }
    this.suspended = true;
  }
  
  resume(): void {
    this.suspended = false;
  }
  
  private onBusEvent(event: egg.Event): void {
    switch (event.eventType) {
      case egg.EventType.CONNECT: this.requireDevice(event.v0, event.v1); break;
      case egg.EventType.DISCONNECT: this.dropDevice(event.v0); break;
    }
  }
  
  private onTwoState(devid: number, btnid: number, part: Joy2Part, value: 0 | 1): void {
    if ((devid < 0) && this.privateCb) { // In private mode, keyboard can be ignored.
      if (!this.privateIncludesKeyboard) return;
    }
    const device = this.requireDevice(devid, 0);
    const button = device.buttons.find(b => ((b[0] === btnid) && (b[1] === part)));
    if (!button) return;
    if (!device.playerid) {
      if (!value) return;
      device.playerid = this.selectLoneliestPlayer();
    }
    if (value) {
      if (device.state & button[2]) return;
      device.state |= button[2];
    } else {
      if (!(device.state & button[2])) return;
      device.state &= ~button[2];
    }
    this.setPlayerButton(device.playerid, button[2], value);
  }
  
  private selectLoneliestPlayer(): number {
    const countByPlayerid = this.players.map(v => 0);
    for (const device of this.devices) {
      countByPlayerid[device.playerid]++;
    }
    let loneliest = 1;
    for (let playerid=2; playerid<countByPlayerid.length; playerid++) {
      if (countByPlayerid[playerid] < countByPlayerid[loneliest]) {
        loneliest = playerid;
      }
    }
    return loneliest;
  }
  
  private requireDevice(devid: number, mapping: number): LogicalDevice {
    let device = this.devices.find(d => d.devid === devid);
    if (device) return device;
    device = {
      devid,
      mapping,
      state: 0,
      playerid: 0, // Devices don't pick up their playerid until the first real event.
      buttons: [],
    };
    this.mapDevice(device);
    this.devices.push(device);
    return device;
  }
  
  private dropDevice(devid: number): void {
    const p = this.devices.findIndex(d => d.devid === devid);
    if (p < 0) return;
    const device = this.devices[p];
    this.devices.splice(p, 1);
    for (let bit=1; bit<=device.state; bit<<=1) {
      if (device.state & bit) {
        this.setPlayerButton(device.playerid, bit, 0);
      }
    }
  }
  
  private setPlayerButton(playerid: number, bit: number, value: number): void {
    if ((playerid < 0) || (playerid >= this.players.length)) return;
    value = value ? 1 : 0;
    const prev = this.players[playerid];
    if (value) {
      if (prev & bit) return;
      this.players[playerid] |= bit;
    } else {
      if (!(prev & bit)) return;
      this.players[playerid] &= ~bit;
    }
    if (this.privateCb) {
      if (!playerid) this.privateCb(bit, value);
    } else if (!this.suspended) {
      for (const { cb } of this.listeners) cb(playerid, bit, value, this.players[playerid]);
    }
    if (playerid) this.setPlayerButton(0, bit, value);
  }
  
  /*-------- Mapping for new devices. --------*/
  
  private mapDevice(device: LogicalDevice): void {
    const dev2 = this.bus.joyTwoState.devices.find(d => d.devid === device.devid);
    if (!dev2) return;
    let map = this.findMap(dev2);
    if (map) {
      this.applyMap(device, dev2, map);
    } else {
      map = this.synthesizeMap(device, dev2);
      if (map) {
        this.applyMap(device, dev2, map);
        this.maps.push(map);
        this.storeMaps();
      }
    }
  }
  
  private findMap(device: Joy2Device): LogicalMap | null {
    return this.maps.find(map => (
      (map.name === device.name) &&
      (map.vid === device.vid) &&
      (map.pid === device.pid) &&
      (map.version === device.version)
    )) || null;
  }
  
  private applyMap(device: LogicalDevice, dev2: Joy2Device, map: LogicalMap): void {
    device.buttons = map.buttons.map(btn => [...btn]);
  }
  
  private synthesizeMap(device: LogicalDevice, dev2: Joy2Device): LogicalMap | null {
    if (this.buttonNames.length < 1) return null;
    
    const map: LogicalMap = {
      name: dev2.name,
      vid: dev2.vid,
      pid: dev2.pid,
      version: dev2.version,
      buttons: [],
    };
  
    // System keyboard? Hard-coded standard config.
    if (device.devid < 0) {
      this.addStandardMapButton(map, 0x00070004, 14); // a => left
      this.addStandardMapButton(map, 0x00070006,  1); // c => east
      this.addStandardMapButton(map, 0x00070007, 15); // d => right
      this.addStandardMapButton(map, 0x00070016, 13); // s => down
      this.addStandardMapButton(map, 0x00070019,  3); // v => north
      this.addStandardMapButton(map, 0x0007001a, 12); // w => up
      this.addStandardMapButton(map, 0x0007001b,  2); // x => west
      this.addStandardMapButton(map, 0x0007001d,  0); // z => south
      this.addStandardMapButton(map, 0x00070028,  9); // enter => aux1
      this.addStandardMapButton(map, 0x0007002a,  7); // backspace => r2
      this.addStandardMapButton(map, 0x0007002b,  4); // tab => l1
      this.addStandardMapButton(map, 0x0007002c,  0); // space => south
      this.addStandardMapButton(map, 0x0007002f, 16); // open bracket => aux3
      this.addStandardMapButton(map, 0x00070030,  8); // close bracket => aux2
      this.addStandardMapButton(map, 0x00070031,  5); // backslash => r1
      this.addStandardMapButton(map, 0x00070035,  6); // grave => l2
      this.addStandardMapButton(map, 0x00070036,  2); // comma => west
      this.addStandardMapButton(map, 0x00070037,  1); // dot => east
      this.addStandardMapButton(map, 0x00070038,  3); // slash => north
      this.addStandardMapButton(map, 0x0007004f, 15); // right => right
      this.addStandardMapButton(map, 0x00070050, 14); // left => left
      this.addStandardMapButton(map, 0x00070051, 13); // down => down
      this.addStandardMapButton(map, 0x00070052, 12); // up => up
      this.addStandardMapButton(map, 0x00070054,  9); // kp slash => aux1
      this.addStandardMapButton(map, 0x00070055,  8); // kp star => aux2
      this.addStandardMapButton(map, 0x00070056, 16); // kp dash => aux3
      this.addStandardMapButton(map, 0x00070057,  1); // kp plus => east
      this.addStandardMapButton(map, 0x00070058,  2); // kp enter => west
      this.addStandardMapButton(map, 0x00070059,  6); // kp 1 => l2
      this.addStandardMapButton(map, 0x0007005a, 13); // kp 2 => down
      this.addStandardMapButton(map, 0x0007005b,  7); // kp 3 => r2
      this.addStandardMapButton(map, 0x0007005c, 14); // kp 4 => left
      this.addStandardMapButton(map, 0x0007005d, 13); // kp 5 => down
      this.addStandardMapButton(map, 0x0007005e, 15); // kp 6 => right
      this.addStandardMapButton(map, 0x0007005f,  4); // kp 7 => l1
      this.addStandardMapButton(map, 0x00070060, 12); // kp 8 => up
      this.addStandardMapButton(map, 0x00070061,  5); // kp 9 => r1
      this.addStandardMapButton(map, 0x00070062,  0); // kp 0 => south
      this.addStandardMapButton(map, 0x00070063,  3); // kp dot => north
      return map;
    }
    
    // Identify the dpad buttons. Abort if we can't find one.
    let btnidl = this.buttonNames.indexOf(this.standardNames[14]); if (btnidl < 0) return null; btnidl = 1 << btnidl;
    let btnidr = this.buttonNames.indexOf(this.standardNames[15]); if (btnidr < 0) return null; btnidr = 1 << btnidr;
    let btnidu = this.buttonNames.indexOf(this.standardNames[12]); if (btnidu < 0) return null; btnidu = 1 << btnidu;
    let btnidd = this.buttonNames.indexOf(this.standardNames[13]); if (btnidd < 0) return null; btnidd = 1 << btnidd;
    
    // Standard Mapping? We can finish it up based largely on faith.
    if (dev2.mapping === 1) {
      for (let i=0; i<17; i++) this.addStandardMapButton(map, 0x80 + i, i);
      map.buttons.push([0x40, 'lo', btnidl]);
      map.buttons.push([0x40, 'hi', btnidr]);
      map.buttons.push([0x41, 'lo', btnidu]);
      map.buttons.push([0x41, 'hi', btnidd]);
      return map;
    }
    
    // Generic devices, we'll read all the capabilities and make stuff up.
    const unassigned: number[] = []; // btnid, 'b' only
    let horzc=0, vertc=0, hatc=0;
    for (let p=0; ; p++) {
      const btn = egg.input_device_get_button(device.devid, p);
      if (!btn || !btn.btnid) break;
      const btn2 = dev2.buttons.find(b => b.btnid == btn.btnid);
      if (btn2) {
        switch (btn2.values.length) {
        
          // Two-state: Save for the next pass.
          case 1: unassigned.push(btn.btnid); break;
          
          // Three-state: Horz or vert.
          case 2: {
              let horz: boolean;
              switch (btn.hidusage) {
                case 0x00010030: horz = true; break;
                case 0x00010031: horz = false; break;
                case 0x00010033: horz = true; break;
                case 0x00010034: horz = false; break;
                default: horz = (horzc <= vertc);
              }
              if (horz) {
                horzc++;
                map.buttons.push([btn.btnid, 'lo', btnidl]);
                map.buttons.push([btn.btnid, 'hi', btnidr]);
              } else {
                vertc++;
                map.buttons.push([btn.btnid, 'lo', btnidu]);
                map.buttons.push([btn.btnid, 'hi', btnidd]);
              }
            } break;
            
          // Hat.
          case 4: {
              hatc++;
              map.buttons.push([btn.btnid, 'l', btnidl]);
              map.buttons.push([btn.btnid, 'r', btnidr]);
              map.buttons.push([btn.btnid, 'u', btnidu]);
              map.buttons.push([btn.btnid, 'd', btnidd]);
            } break;
        }
      } else {
        unassigned.push(btn.btnid);
      }
    }
    
    if (hatc || (horzc && vertc)) {
      // We got both axes mapped; don't assign any other buttons to them.
    } else if (unassigned.length >= 4) {
      // No axes or hats. Assign four buttons as the dpad. Odds we'll get it right are painfully slim.
      map.buttons.push([unassigned[0], 'l', btnidl]);
      map.buttons.push([unassigned[1], 'r', btnidr]);
      map.buttons.push([unassigned[2], 'u', btnidu]);
      map.buttons.push([unassigned[3], 'd', btnidd]);
      unassigned.splice(0, 4);
    }
    // Everything else goes willy-nilly to the non-dpad buttons.
    const available = [];
    for (let i=0; i<this.buttonNames.length; i++) {
      const btnid = 1 << i;
      if (btnid === btnidl) continue;
      if (btnid === btnidr) continue;
      if (btnid === btnidu) continue;
      if (btnid === btnidd) continue;
      available.push(btnid);
    }
    if (available.length > 0) {
      let availp = 0;
      for (const btnid of unassigned) {
        map.buttons.push([btnid, 'b', available[availp]]);
        if (++availp >= available.length) availp = 0;
      }
    }
      
    return map;
  }
  
  private addStandardMapButton(map: LogicalMap, keycode: number, index: number): void {
    const name = this.standardNames[index];
    if (!name) return;
    const btnix = this.buttonNames.indexOf(name);
    if (btnix < 0) return;
    map.buttons.push([keycode, 'b', 1 << btnix]);
  }
  
  /*--------- Storage. -----------------*/
  
  private loadMaps(): void {
    try {
      const serial = egg.store_get("joymaps");
      if (serial) this.maps = JSON.parse(serial);
      else this.maps = [];
    } catch (e) {
      egg.log(`Failed to load input maps: ${e}`);
    }
  }
  
  private storeMaps(): void {
    try {
      const serial = JSON.stringify(this.maps);
      egg.store_set("joymaps", serial);
    } catch (e) {
      egg.log(`Failed to store input maps: ${e}`);
    }
  }
  
  replaceMap(dev2: Joy2Device, buttons: { srcbtnid: number, srcpart: Joy2Part, dstbtnid: number}[]): void {
    const p = this.maps.findIndex(m => (
      (m.name === dev2.name) &&
      (m.vid === dev2.vid) &&
      (m.pid === dev2.pid) &&
      (m.version === dev2.version)
    ));
    if (p >= 0) this.maps.splice(p, 1);
    const map: LogicalMap = {
      name: dev2.name,
      vid: dev2.vid,
      pid: dev2.pid,
      version: dev2.version,
      buttons: buttons.map(b => [b.srcbtnid, b.srcpart, b.dstbtnid]),
    };
    this.maps.push(map);
    this.storeMaps();
    const device = this.devices.find(d => d.devid === dev2.devid);
    if (device) {
      this.applyMap(device, dev2, map);
    }
  }
}
