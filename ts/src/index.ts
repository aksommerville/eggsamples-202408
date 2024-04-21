import { TileRenderer } from "./TileRenderer";
import { InputManager } from "./InputManager";

let tileRenderer: TileRenderer;
let inputManager: InputManager;
let texid_font = 0;

function egg_client_init(): number {
  tileRenderer = new TileRenderer();
  inputManager = new InputManager();
  
  texid_font = egg.texture_new();
  if (egg.texture_load_image(texid_font, 0, 1) < 0) return -1;
  
  inputManager.pushMode("raw");
  const inputListener = inputManager.listen(0xffffffff, e => egg.log(`index.ts receiving event: ${JSON.stringify(e)}`));
  
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

function egg_client_update(elapsed: number): void {
  inputManager.update();
}

function egg_client_render(): void {
  egg.draw_rect(1, 0, 0, 320, 180, 0x804000ff);
  tileRenderer.begin(texid_font, 0xffffffff, 0xff);
  tileRenderer.tile(40, 40, 0x41, egg.Xform.NONE);
  tileRenderer.string(40, 48, "The quick brown fox");
  tileRenderer.string(40, 56, "jumps over the lazy dog.");
  tileRenderer.end();
}

exportModule({
  egg_client_init,
  egg_client_update,
  egg_client_render,
});
