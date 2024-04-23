import { TileRenderer } from "./TileRenderer";
import {
  InputManager,
  InputManagerPlayerEvent,
  InputManagerButton,
  InputManagerEvent,
} from "./InputManager";

let tileRenderer: TileRenderer;
let inputManager: InputManager;
let texid_font = 0;
let texid_cursor = 0;
let textEntry = "";
let mousex = -1, mousey = -1;
let screenw = 0, screenh = 0;

const buttons: {
  x: number;
  y: number;
  w: number;
  h: number;
  label: string;
  cb: () => void;
}[] = [
  { x:40, y:40, w:100, h:16, label:"Text", cb: onClickText },
  { x:40, y:60, w:100, h:16, label:"Joystick", cb: onClickJoystick },
  { x:40, y:80, w:100, h:16, label:"Config", cb: onClickConfig },
];

function egg_client_init(): number {
  tileRenderer = new TileRenderer();
  
  const fb = egg.texture_get_header(1);
  screenw = fb.w;
  screenh = fb.h;
  
  if (egg.texture_load_image(texid_font = egg.texture_new(), 0, 1) < 0) return -1;
  if (egg.texture_load_image(texid_cursor = egg.texture_new(), 0, 2) < 0) return -1;
  const cursor = egg.texture_get_header(texid_cursor);
  
  inputManager = new InputManager(texid_font, {
    texid: texid_cursor,
    x: 0, y: 0, w: cursor.w, h: cursor.h,
    hotX: 4,
    hotY: 1,
  });
  inputManager.pushMode("pointer");
  const inputListener = inputManager.listen(0xffffffff, e => onEvent(e));
  inputManager.listen("text", e => onText((e as any).codepoint));
  inputManager.listenPlayer(0, e => onPlayerEvent(e));
  
  /* ok
  inputManager.http("GET", "http://localhost:8080/list-games").then(rsp => {
    egg.log(`list-games: ${rsp}`);
  }).catch(error => {
    egg.log(`list-games failed: ${error}`);
  });
  
  inputManager.websocketListen("ws://localhost:8080/ws", msg => {
    if (typeof(msg) === "number") {
      egg.log(`websocket ${msg ? "connected" : "disconnected"}`);
    } else {
      egg.log(`via websocket: ${JSON.stringify(msg)}`);
    }
  });
  /**/
  
  return 0;
}

function onEvent(event: InputManagerEvent): void {
  //egg.log(`index.ts receiving event: ${JSON.stringify(event)}`);
  switch (event.type) {
    case egg.EventType.MMOTION: mousex = event.x; mousey = event.y; break;
    case egg.EventType.MBUTTON: if ((event.btnid === 1) && event.value) {
        for (const button of buttons) {
          if (event.x < button.x) continue;
          if (event.y < button.y) continue;
          if (event.x >= button.x + button.w) continue;
          if (event.y >= button.y + button.h) continue;
          button.cb();
          break;
        }
      } break;
    case egg.EventType.KEY: if (event.value) switch (event.keycode) {
        case 0x00070029: {
            const mode = inputManager.getMode();
            if (mode === "pointer") egg.request_termination();
            else inputManager.popMode(mode);
          } break;
      } break;
  }
}

function onText(codepoint: number): void {
  if (codepoint === 0x08) {
    if (textEntry.length > 0) {
      textEntry = textEntry.substring(0, textEntry.length - 1);
    }
  } else if (codepoint === 0x0a) {
    egg.log(`You entered ${JSON.stringify(textEntry)}.`);
    textEntry = "";
  } else if (textEntry.length < 32) {
    textEntry += String.fromCharCode(codepoint);
  }
}

function onPlayerEvent(event: InputManagerPlayerEvent): void {
  return;
  let msg = `PLAYER ${event.playerid}:`;
  if (event.state & InputManagerButton.LEFT) msg += " LEFT";
  if (event.state & InputManagerButton.RIGHT) msg += " RIGHT";
  if (event.state & InputManagerButton.UP) msg += " UP";
  if (event.state & InputManagerButton.DOWN) msg += " DOWN";
  if (event.state & InputManagerButton.SOUTH) msg += " SOUTH";
  if (event.state & InputManagerButton.WEST) msg += " WEST";
  if (event.state & InputManagerButton.EAST) msg += " EAST";
  if (event.state & InputManagerButton.NORTH) msg += " NORTH";
  if (event.state & InputManagerButton.L1) msg += " L1";
  if (event.state & InputManagerButton.R1) msg += " R1";
  if (event.state & InputManagerButton.L2) msg += " L2";
  if (event.state & InputManagerButton.R2) msg += " R2";
  if (event.state & InputManagerButton.AUX1) msg += " AUX1";
  if (event.state & InputManagerButton.AUX2) msg += " AUX2";
  if (event.state & InputManagerButton.AUX3) msg += " AUX3";
  if (event.state & InputManagerButton.CD) msg += " CD";
  egg.log(msg);
}

function onClickText(): void {
  inputManager.pushMode("text");
}

function onClickJoystick(): void {
  inputManager.pushMode("joystick");
}

function onClickConfig(): void {
  inputManager.configureJoystick([
    "Left", "Right", "Up", "Down",
    "South", "West", "East", "North",
    "L1", "R1", "L2", "R2",
    "Aux1", "Aux2", "Aux3",
  ]);
}

function egg_client_update(elapsed: number): void {
  if (inputManager.update(elapsed)) return;
}

function egg_client_render(): void {

  egg.draw_rect(1, 0, 0, screenw, screenh, 0x804000ff);
  
  switch (inputManager.getMode()) {

    case "text": {
        tileRenderer.begin(texid_font, 0xffffffff, 0xff);
        tileRenderer.string(24, 20, textEntry);
        tileRenderer.end();
      } break;
  
    case "pointer": {
        for (const button of buttons) {
          egg.draw_rect(1, button.x, button.y, button.w, button.h, 0xc0c0c0ff);
        }
        tileRenderer.begin(texid_font, 0x000000ff, 0xff);
        for (const button of buttons) {
          tileRenderer.string(button.x + 8, button.y + (button.h >> 1), button.label);
        }
        tileRenderer.end();
      } break;
      
    case "joystick": {
        const state = inputManager.getPlayer(0);
        const colw = Math.floor(screenw / 9);
        const rowh = Math.floor(screenh / 7);
        const on = 0xffff00ff;
        const off = 0x80808080;
        egg.draw_rect(1, colw * 1, rowh * 1, colw, rowh, (state & InputManagerButton.L2) ? on : off);
        egg.draw_rect(1, colw * 4, rowh * 1, colw, rowh, (state & InputManagerButton.CD) ? on : off);
        egg.draw_rect(1, colw * 7, rowh * 1, colw, rowh, (state & InputManagerButton.R2) ? on : off);
        egg.draw_rect(1, colw * 1, rowh * 2, colw, rowh, (state & InputManagerButton.L1) ? on : off);
        egg.draw_rect(1, colw * 7, rowh * 2, colw, rowh, (state & InputManagerButton.R1) ? on : off);
        egg.draw_rect(1, colw * 2, rowh * 3, colw, rowh, (state & InputManagerButton.UP) ? on : off);
        egg.draw_rect(1, colw * 4, rowh * 3, colw, rowh, (state & InputManagerButton.AUX1) ? on : off);
        egg.draw_rect(1, colw * 6, rowh * 3, colw, rowh, (state & InputManagerButton.NORTH) ? on : off);
        egg.draw_rect(1, colw * 1, rowh * 4, colw, rowh, (state & InputManagerButton.LEFT) ? on : off);
        egg.draw_rect(1, colw * 3, rowh * 4, colw, rowh, (state & InputManagerButton.RIGHT) ? on : off);
        egg.draw_rect(1, colw * 4, rowh * 4, colw, rowh, (state & InputManagerButton.AUX2) ? on : off);
        egg.draw_rect(1, colw * 5, rowh * 4, colw, rowh, (state & InputManagerButton.WEST) ? on : off);
        egg.draw_rect(1, colw * 7, rowh * 4, colw, rowh, (state & InputManagerButton.EAST) ? on : off);
        egg.draw_rect(1, colw * 2, rowh * 5, colw, rowh, (state & InputManagerButton.DOWN) ? on : off);
        egg.draw_rect(1, colw * 4, rowh * 5, colw, rowh, (state & InputManagerButton.AUX3) ? on : off);
        egg.draw_rect(1, colw * 6, rowh * 5, colw, rowh, (state & InputManagerButton.SOUTH) ? on : off);
      } break;

  }
  inputManager.render();
}

exportModule({
  egg_client_init,
  egg_client_update,
  egg_client_render,
});
