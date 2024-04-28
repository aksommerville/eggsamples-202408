import { TileRenderer } from "./TileRenderer";
import { Bus } from "./input/Bus";

let tileRenderer: TileRenderer;
let bus: Bus;
let texid_font = 0;
let texid_cursor = 0;
let texid_bg = 0;
let texid_joystick = 0;
let textEntry = "";
let joyState = 0;
let screenw = 0, screenh = 0;
let joyp = 0;

const joystickButtonLocations = [
  [6, 30, 15, 14],
  [33, 30, 15, 14],
  [20, 16, 14, 15],
  [20, 43, 14, 15],
  [94, 44, 14, 14],
  [80, 30, 14, 14],
  [108, 30, 14, 14],
  [94, 16, 14, 14],
  [19, 6, 27, 5],
  [82, 6, 27, 5],
  [10, 5, 9, 6],
  [109, 5, 9, 6],
  [53, 51, 22, 7],
  [53, 41, 22, 7],
  [53, 31, 22, 7],
  [58, 17, 12, 9],
];

function egg_client_init(): number {
  tileRenderer = new TileRenderer();
  bus = new Bus();
  
  const fb = egg.texture_get_header(1);
  screenw = fb.w;
  screenh = fb.h;
  
  if (egg.texture_load_image(texid_font = egg.texture_new(), 0, 1) < 0) return -1;
  if (egg.texture_load_image(texid_cursor = egg.texture_new(), 0, 2) < 0) return -1;
  if (egg.texture_load_image(texid_bg = egg.texture_new(), 0, 3) < 0) return -1;
  if (egg.texture_load_image(texid_joystick = egg.texture_new(), 0, 4) < 0) return -1;
  const cursor = egg.texture_get_header(texid_cursor);
  
  bus.setFont(texid_font);
  bus.setCursor(texid_cursor, 16, 0, 16, 16, 8, 8);
  bus.setButtonNames(
    //["Left", "Right", "Up", "Down", "South", "West", "East", "North", "L1", "R1", "L2", "R2", "Aux1", "Aux2", "Aux3"],
    //["South", "East", "West", "North", "L1", "R1", "L2", "R2", "Aux2", "Aux1", "LP", "RP", "Up", "Down", "Left", "Right", "Aux3"]
    // A more realistic game would assign meaningful labels. (hint: Use string resources so they can translate).
    ["Left", "Right", "Up", "Down", "Jump", "Attack", "Dash", "Pause"],
    ["Jump", "Dash", "Attack", "", "", "", "", "", "", "Pause", "", "", "Up", "Down", "Left", "Right", ""]
  );
  //bus.joyQuery.setPrompts(["The hell? I said '%'.", "przzzz % if you dare", "again, %s"]);
  
  bus.listen("all", event => onEvent(event));
  bus.joyLogical.listen((p, b, v, s) => onPlayerEvent(p, b, v, s));
  
  // Select an input mode:
  //if (bus.requireJoysticks()) egg.log("Listening for mapped joystick events."); else egg.log("Bus.requireJoysticks failed!");
  //if (bus.requireText()) egg.log("Listening for text."); else egg.log("Bus.requireText failed!");
  if (bus.requirePointer()) egg.log("Listening for pointer."); else egg.log("Bus.requirePointer failed!");
  
  return 0;
}

function onEvent(event: egg.Event): void {
  //egg.log(`index.ts:onEvent: ${JSON.stringify(event)}`);
  switch (event.eventType) {
  
    case egg.EventType.CONNECT: {
        //inputProxy.joyShaper.configureDevice(event.v0);
      } break;
  
    case egg.EventType.TEXT: {
        if (event.v0 === 0x0a) {
          egg.log(`Text entry complete: ${JSON.stringify(textEntry)}`);
          textEntry = "";
        } else if (event.v0 === 0x08) {
          if (textEntry.length > 0) textEntry = textEntry.substring(0, textEntry.length - 1);
        } else if (event.v0 < 0x20) {
          // Ignore C0, eg ESC.
        } else {
          textEntry += String.fromCharCode(event.v0);
        }
      } break;
      
    case egg.EventType.MBUTTON: {
        if (event.v0 === 1) {
          if (event.v1) bus.setCursor(texid_cursor, 32, 0, 16, 16, 8, 8);
          else bus.setCursor(texid_cursor, 16, 0, 16, 16, 8, 8);
        }
      } break;
  }
}

function onPlayerEvent(playerid: number, btnid: number, value: number, state: number): void {
  //egg.log(`index.ts:onPlayerEvent: ${playerid}.${btnid}=${value} [${state}]`);
  if (!playerid) {
    joyState = state;
    if (value) switch (btnid) {
      case 0x0004: if (--joyp < 0) joyp = bus.joyTwoState.devices.length - 1; break;
      case 0x0008: if (++joyp >= bus.joyTwoState.devices.length) joyp = 0; break;
      case 0x0010: bus.beginJoyQuery(bus.joyTwoState.devices[joyp]?.devid || 0); break;
    }
  }
}

function egg_client_update(elapsed: number): void {
  bus.update(elapsed);
}

function egg_client_render(): void {
  //egg.draw_rect(1, 0, 0, screenw, screenh, 0x104060ff);
  egg.draw_decal(1, texid_bg, 0, 0, 0, 0, screenw, screenh, 0);
  
  egg.draw_rect(1, 0, 15 + joyp * 8, screenw, 9, 0x403000ff);
  tileRenderer.begin(texid_font, 0x80ff80ff, 0xff);
  let y = 20;
  for (const device of bus.joyTwoState.devices) {
    tileRenderer.string(8, y, device.name);
    y += 8;
  }
  tileRenderer.end();
  
  egg.draw_decal(1, texid_joystick, screenw - 128, 0, 0, 0, 128, 64, 0);
  if (joyState) {
    for (let i=0; i<16; i++) {
      if (!(joyState & (1 << i))) continue;
      const [subx, suby, subw, subh] = joystickButtonLocations[i];
      egg.draw_decal(1, texid_joystick, screenw - 128 + subx, suby, subx, 64 + suby, subw, subh, 0);
    }
  }
  
  if (textEntry) {
    tileRenderer.begin(texid_font, 0xffffffff, 0xff);
    tileRenderer.string(8, 8, textEntry);
    tileRenderer.end();
  }
  
  bus.render();
}

exportModule({
  egg_client_init,
  egg_client_update,
  egg_client_render,
});
