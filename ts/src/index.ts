import { TileRenderer } from "./utility/TileRenderer";
import { TextService } from "./utility/TextService";
import { SingleSelectionMenu } from "./utility/SingleSelectionMenu";
import { Font } from "./utility/Font";
import { Bus } from "./input/Bus";

const HERO_SPEED = 100; // px/s

let tileRenderer: TileRenderer;
let textService: TextService;
let menu: SingleSelectionMenu | null = null;
let font: Font;
let bus: Bus;
let texid_font = 0;
let texid_cursor = 0;
let texid_bg = 0;
let texid_joystick = 0;
let texid_sprites = 0;
let texid_message = 0;
let messagew=0, messageh=0;
let textEntry = "";
let screenw = 0, screenh = 0;
let animclock = 0;
let animframe = 0;
let herodx = 0;
let herody = 0;
let herox = 200;
let heroy = 100;

const bggrid = [
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x10, 0x11, 0x11, 0x11, 0x11, 0x11, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x10, 0x24, 0x21, 0x21, 0x21, 0x21, 0x21, 0x23, 0x12, 0x00, 0x00, 0x10, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x20, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x23, 0x12, 0x00, 0x30, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x20, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x30, 0x31, 0x14, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x22, 0x00, 0x00, 0x00, 0x33, 0x00, 0x34, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x30, 0x14, 0x21, 0x21, 0x21, 0x21, 0x21, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x30, 0x14, 0x21, 0x21, 0x21, 0x13, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x31, 0x31, 0x31, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
];

const bgtiles = new Uint8Array(bggrid.length * 6);

function egg_client_init(): number {
  tileRenderer = new TileRenderer();
  textService = new TextService();
  bus = new Bus();
  
  const fb = egg.texture_get_header(1);
  screenw = fb.w;
  screenh = fb.h;
  
  if (egg.texture_load_image(texid_font = egg.texture_new(), 0, 1) < 0) return -1;
  if (egg.texture_load_image(texid_cursor = egg.texture_new(), 0, 2) < 0) return -1;
  if (egg.texture_load_image(texid_bg = egg.texture_new(), 0, 5) < 0) return -1;
  if (egg.texture_load_image(texid_joystick = egg.texture_new(), 0, 4) < 0) return -1;
  if (egg.texture_load_image(texid_sprites = egg.texture_new(), 0, 6) < 0) return -1;
  const cursor = egg.texture_get_header(texid_cursor);
  
  bus.setFontTilesheet(texid_font);
  bus.setCursor(texid_cursor, 16, 0, 16, 16, 8, 8);
  
  refreshBusVerbiage();
  
  bus.listen("all", event => onEvent(event));
  bus.joyLogical.listen((p, b, v, s) => onPlayerEvent(p, b, v, s));
  bus.requireJoysticks();
  
  font = new Font(9);
  font.addPage(0x0001, 10);
  font.addPage(0x0021, 7);
  font.addPage(0x00a1, 8);
  font.addPage(0x0410, 9);
  bus.setFont(font);
  texid_message = font.renderTexture("This is a long sample of text which will certainly require some line breaking.\n Also an explicit line break and indent.", 200);
  const hdr = egg.texture_get_header(texid_message);
  messagew = hdr.w;
  messageh = hdr.h;
  
  return 0;
}

function refreshBusVerbiage(): void {
  const localButtonNames = ["", "", "", "", "", "", "", ""];
  const standardButtonNames = ["", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""];
  localButtonNames[0] = standardButtonNames[14] = textService.getString(12); // Left
  localButtonNames[1] = standardButtonNames[15] = textService.getString(13); // Right
  localButtonNames[2] = standardButtonNames[12] = textService.getString(14); // Up
  localButtonNames[3] = standardButtonNames[13] = textService.getString(15); // Down
  localButtonNames[4] = standardButtonNames[ 0] = textService.getString(16); // Jump
  localButtonNames[5] = standardButtonNames[ 2] = textService.getString(17); // Attack
  localButtonNames[6] = standardButtonNames[ 1] = textService.getString(18); // Dash
  localButtonNames[7] = standardButtonNames[ 9] = textService.getString(19); // Pause
  bus.setButtonNames(localButtonNames, standardButtonNames);
  bus.joyQuery.setPrompts([
    textService.getString(20),
    textService.getString(21),
    textService.getString(22),
  ]);
}

function onEvent(event: egg.Event): void {
  //egg.log(`EVENT: ${JSON.stringify(event)}`);
  switch (event.eventType) {
    case egg.EventType.KEY: if (event.v1) switch (event.v0) {
        case 0x00070029: egg.request_termination(); break; // esc
        case 0x0007003a: bus.beginJoyQuery(-1); break; // F1: Configure keyboard.
        case 0x0007003b: beginLanguageSelection(); break; // F2: Change language.
        case 0x0007003c: bus.requireText(); break; // F3: Bring up fake keyboard.
        case 0x0007003d: bus.requirePointer(); break; // F4: Bring up fake pointer.
        case 0x0007003e: { // F5: Configure first joystick if present.
            const device = bus.joyTwoState.devices.find(d => d.devid > 0);
            if (device) bus.beginJoyQuery(device.devid);
          } break;
        //default: egg.log("KEY %08x", event.v0);
      } break;
    case egg.EventType.TEXT: {
        egg.log("Received text: U+%x (%c)", event.v0, event.v0);
        if (event.v0 === 0x0a) bus.requireJoysticks();
      } break;
    case egg.EventType.MBUTTON: if (event.v0 === 1) switch (event.v1) {
        case 0: bus.setCursor(texid_cursor, 16, 0, 16, 16, 8, 8); break;
        case 1: bus.setCursor(texid_cursor, 32, 0, 16, 16, 8, 8); break;
      } break;
  }
}

function onPlayerEvent(playerid: number, btnid: number, value: number, state: number): void {
  //egg.log(`PLAYER ${playerid} ${btnid} ${value} ${state}`);
  if (!playerid) {
    if (menu) {
      if (value) switch (btnid) {
        case 0x0001: menu.userInput(-1, 0); break;
        case 0x0002: menu.userInput(1, 0); break;
        case 0x0004: menu.userInput(0, -1); break;
        case 0x0008: menu.userInput(0, 1); break;
        case 0x0010: {
            if (textService.setLanguage(menu.getValue())) {
              refreshBusVerbiage();
            }
            menu = null;
          } break;
      }
    } else {
      switch (btnid) {
        case 0x0001: if (value) herodx = -1; else if (herodx < 0) herodx = 0; break;
        case 0x0002: if (value) herodx = 1; else if (herodx > 0) herodx = 0; break;
        case 0x0004: if (value) herody = -1; else if (herody < 0) herody = 0; break;
        case 0x0008: if (value) herody = 1; else if (herody > 0) herody = 0; break;
      }
    }
  }
}

function beginLanguageSelection(): void {
  menu = new SingleSelectionMenu(
    font, //texid_font,
    textService.getString(23),
    textService.getLanguagesWithDescription(),
    textService.getLanguage()
  );
}

function egg_client_update(elapsed: number): void {
  bus.update(elapsed);
  if ((animclock -= elapsed) <= 0) {
    animclock += 0.200;
    if (++animframe >= 4) {
      animframe = 0;
    }
  }
  herox += herodx * HERO_SPEED * elapsed;
  heroy += herody * HERO_SPEED * elapsed;
}

function egg_client_render(): void {
  //egg.draw_decal(1, texid_bg, 0, 0, 0, 0, screenw, screenh, 0);
  
  for (let yi=11, y=8, tilep=0, bggridp=0; yi-->0; y+=16) {
    for (let xi=20, x=8; xi-->0; x+=16, bggridp++) {
      let tileid = bggrid[bggridp];
      if ((tileid >= 0x10) && (tileid < 0x40)) switch (animframe) {
        case 0: break;
        case 1: tileid += 5; break;
        case 2: tileid += 10; break;
        case 3: tileid += 5; break;
      }
      bgtiles[tilep++] = x;
      bgtiles[tilep++] = x >> 8;
      bgtiles[tilep++] = y;
      bgtiles[tilep++] = y >> 8;
      bgtiles[tilep++] = tileid;
      bgtiles[tilep++] = 0;
    }
  }
  egg.draw_tile(1, texid_bg, bgtiles.buffer, 20 * 11);
  
  tileRenderer.begin(texid_sprites, 0, 0xff);
  const hx = Math.round(herox);
  const hy = Math.round(heroy);
  tileRenderer.tile(hx, hy+3, 0x00, 0);
  tileRenderer.tile(hx, hy, 0x21, 0);
  tileRenderer.tile(hx+1, hy-13, 0x11, 0);
  tileRenderer.end();
  
  egg.draw_decal(1, texid_message, 10, 10, 0, 0, messagew, messageh, 0);
  
  if (menu) menu.render();
  
  bus.render();
}

exportModule({
  egg_client_init,
  egg_client_update,
  egg_client_render,
});
