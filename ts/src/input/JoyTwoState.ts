import { Bus } from "./Bus";

/**
 * The main thing we do is breaking up signed axes and hats into multiple logical two-state buttons.
 * The combination of `btnid` and `part` is a unique two-state input.
 */
export type Joy2Part = "b" | "lo" | "hi" | "l" | "r" | "u" | "d";

export const enum Joy2Method {
  UNSIGNED, // Single output: ((srclo<=v)&&(v<=srchi))
  SIGNED, // Two outputs: (v<=srclo) or (v>=srchi)
  HAT, // Four outputs, clockwise from Up. Inputs are (srclo)..(srclo+7), or anything else for neutral.
}

export interface Joy2Button {
  btnid: number;
  hidusage: number; // We don't use it, but we happen to capture it.
  method: Joy2Method;
  srclo: number;
  srchi: number;
  values: number[]; // [state] or [lo,hi] or [left,right,up,down]
}

export interface Joy2Device {
  devid: number;
  name: string;
  vid: number;
  pid: number;
  version: number;
  mapping: number;
  buttons: Joy2Button[]; // Anything absent, assume it's a natural two-state: zero vs nonzero.
  natural: Set<number>; // (btnid) for all natural two-states currently held.
}

/**
 * Tracks input devices and massages each into a set of two-state buttons.
 * We track the system keyboard as if it were a joystick.
 * No logical mapping. Just convert signed axes to two buttons, hats to four, etc.
 * We can serve as the canonical registry of devices too.
 */
export class JoyTwoState {
  private nextId = 1;
  private listeners: { id: number; cb: (devid: number, btnid: number, part: Joy2Part, value: 0 | 1) => void }[] = [];
  public devices: Joy2Device[] = [];
  
  constructor(
    private bus: Bus
  ) {
    this.bus.listen(
      (1 << egg.EventType.INPUT) |
      (1 << egg.EventType.CONNECT) |
      (1 << egg.EventType.DISCONNECT) |
      (1 << egg.EventType.KEY) |
    0, e => this.onEvent(e));
    // If KEY events are available, we turn them on and expect them to remain on always.
    switch (egg.event_enable(egg.EventType.KEY, egg.EventState.ENABLED)) {
      case egg.EventState.ENABLED:
      case egg.EventState.REQUIRED: {
          this.devices.push({
            devid: -1,
            name: "System Keyboard",
            vid: 0,
            pid: 0,
            version: 0,
            mapping: 0,
            buttons: [],
            natural: new Set(),
          });
        } break;
    }
  }
  
  listen(cb: (devid: number, btnid: number, part: Joy2Part, value: 0 | 1) => void): number {
    const id = this.nextId++;
    this.listeners.push({ id, cb });
    return id;
  }
  
  unlisten(id: number): void {
    const p = this.listeners.findIndex(l => l.id === id);
    if (p >= 0) {
      this.listeners.splice(p, 1);
    }
  }
  
  private onEvent(event: egg.Event): void {
    switch (event.eventType) {
      case egg.EventType.INPUT: this.setButton(event.v0, event.v1, event.v2); break;
      case egg.EventType.CONNECT: this.onConnect(event.v0, event.v1); break;
      case egg.EventType.DISCONNECT: this.onDisconnect(event.v0); break;
      case egg.EventType.KEY: this.setButton(-1, event.v0, event.v1); break;
    }
  }
  
  private onConnect(devid: number, mapping: number): void {
    if (devid < 1) return;
    if (this.devices.find(d => d.devid === devid)) return;
    const device: Joy2Device = {
      devid: devid,
      name: egg.input_device_get_name(devid),
      ...egg.input_device_get_ids(devid), // vid,pid,version
      mapping,
      buttons: [],
      natural: new Set(),
    };
    for (let p=0; ; p++) {
      const btn = egg.input_device_get_button(devid, p);
      if (!btn || !btn.btnid) break;
      const range = btn.hi - btn.lo + 1;
      if (range < 3) continue; // invalid or natural two-state
      if (range === 8) { // Assume hat.
        device.buttons.push({
          btnid: btn.btnid,
          hidusage: btn.hidusage,
          method: Joy2Method.HAT,
          srclo: btn.lo,
          srchi: btn.hi,
          values: [0, 0, 0, 0],
        });
      } else if (btn.lo === btn.value) { // Assume unsigned.
        device.buttons.push({
          btnid: btn.btnid,
          hidusage: btn.hidusage,
          method: Joy2Method.UNSIGNED,
          srclo: btn.lo + 1,
          srchi: btn.hi,
          values: [0],
        });
      } else { // Assume signed.
        const mid = (btn.lo + btn.hi) >> 1;
        let srclo = (mid + btn.lo) >> 1;
        let srchi = (mid + btn.hi) >> 1;
        if (srclo >= mid) srclo = mid - 1;
        if (srchi <= mid) srchi = mid + 1;
        device.buttons.push({
          btnid: btn.btnid,
          hidusage: btn.hidusage,
          method: Joy2Method.SIGNED,
          srclo,
          srchi,
          values: [0, 0],
        });
      }
    }
    this.devices.push(device);
  }
  
  private onDisconnect(devid: number): void {
    const p = this.devices.findIndex(d => d.devid === devid);
    if (p < 0) return;
    const device = this.devices[p];
    this.devices.splice(p, 1);
    for (const button of device.buttons) {
      switch (button.method) {
        case Joy2Method.UNSIGNED: {
            if (button.values[0]) this.buttonChanged(devid, button.btnid, "b", 0);
          } break;
        case Joy2Method.SIGNED: {
            if (button.values[0]) this.buttonChanged(devid, button.btnid, "lo", 0);
            if (button.values[1]) this.buttonChanged(devid, button.btnid, "hi", 0);
          } break;
        case Joy2Method.HAT: {
            if (button.values[0]) this.buttonChanged(devid, button.btnid, "l", 0);
            if (button.values[1]) this.buttonChanged(devid, button.btnid, "r", 0);
            if (button.values[2]) this.buttonChanged(devid, button.btnid, "u", 0);
            if (button.values[3]) this.buttonChanged(devid, button.btnid, "d", 0);
          } break;
      }
    }
    for (const btnid of Array.from(device.natural)) {
      this.buttonChanged(devid, btnid, "b", 0);
    }
  }
  
  private setButton(devid: number, btnid: number, value: number): void {
    const device = this.devices.find(d => d.devid === devid);
    if (!device) return;
    const button = device.buttons.find(b => b.btnid === btnid);
    if (button) {
      switch (button.method) {
      
        case Joy2Method.UNSIGNED: {
            if ((value >= button.srclo) && (value <= button.srchi)) {
              if (button.values[0]) return;
              button.values[0] = 1;
              this.buttonChanged(devid, btnid, "b", 1);
            } else if (button.values[1]) {
              button.values[1] = 0;
              this.buttonChanged(devid, btnid, "b", 0);
            }
          } break;
          
        case Joy2Method.SIGNED: {
            if (value <= button.srclo) {
              if (!button.values[0]) {
                button.values[0] = 1;
                this.buttonChanged(devid, btnid, "lo", 1);
              }
            } else if (button.values[0]) {
              button.values[0] = 0;
              this.buttonChanged(devid, btnid, "lo", 0);
            }
            if (value >= button.srchi) {
              if (!button.values[1]) {
                button.values[1] = 1;
                this.buttonChanged(devid, btnid, "hi", 1);
              }
            } else if (button.values[1]) {
              button.values[1] = 0;
              this.buttonChanged(devid, btnid, "hi", 0);
            }
          } break;
          
        case Joy2Method.HAT: {
            value -= button.srclo;
            let b0:0|1=0, b1:0|1=0, b2:0|1=0, b3:0|1=0;
            switch (value) {
              case 7: case 6: case 5: b0 = 1; break;
              case 1: case 2: case 3: b1 = 1; break;
            }
            switch (value) {
              case 7: case 0: case 1: b2 = 1; break;
              case 5: case 4: case 3: b3 = 1; break;
            }
            if (button.values[0] !== b0) { button.values[0] = b0; this.buttonChanged(devid, btnid, "l", b0); }
            if (button.values[1] !== b1) { button.values[1] = b1; this.buttonChanged(devid, btnid, "r", b1); }
            if (button.values[2] !== b2) { button.values[2] = b2; this.buttonChanged(devid, btnid, "u", b2); }
            if (button.values[3] !== b3) { button.values[3] = b3; this.buttonChanged(devid, btnid, "d", b3); }
          } break;
      }
    } else if (value) { // Natural two-state ON.
      if (!device.natural.has(btnid)) {
        device.natural.add(btnid);
        this.buttonChanged(devid, btnid, "b", 1);
      }
    } else { // Natural two-state OFF.
      if (device.natural.delete(btnid)) {
        this.buttonChanged(devid, btnid, "b", 0);
      }
    }
  }
  
  private buttonChanged(devid: number, btnid: number, part: Joy2Part, value: 0 | 1): void {
    for (const { cb } of this.listeners) cb(devid, btnid, part, value);
  }
}
