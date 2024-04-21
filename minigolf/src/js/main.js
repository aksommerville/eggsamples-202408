import { TileRenderer } from "./TileRenderer.js";

const MAG_MAX = 100; // px
const MAG_MIN = 20; // px
const SHOT_ANIM_TIME = 0.100; // s
const BALL_DECELERATION = 40; // px/s^2
const BALL_VELOCITY_SCALE = 5; // Initial velocity relative to shotMagnitude.
const BALL_RADIUS = 8; // px
const SCREEN_W = 640; // px
const SCREEN_H = 360; // px
const HOLE_RADIUS = 12; // px; distance to consider it In.
const HOLE_RADIUS2 = HOLE_RADIUS * HOLE_RADIUS;
const ROUND_DENOUEMENT_TIME = 2; // s
const ROUND_STARTUP_TIME = 1; // s
const SOUND_PUT = 1;
const SOUND_BOUNCE = 2;
const SOUND_HOLE = 3;

let texid_font, texid_tiles;
let tileRenderer = new TileRenderer();
let shotDx = 0;
let shotDy = -1;
let shotMagnitude = (MAG_MAX * MAG_MIN) / 2; // pixels
let shotAnimClock = 0;
let shotAnimFrame = 0;
let shotCount = 0;
let ballX = 0;
let ballY = 0;
let ballVelocity = 0;
let ballDx = 0;
let ballDy = 0;
let holeX = 0;
let holeY = 0;
let playInProgress = true;
let resetClock = 0;
let startupClock = 0;

function resetGame() {
  ballX = SCREEN_W / 2;
  ballY = SCREEN_H / 2;
  ballVelocity = 0;
  holeX = 10 + Math.random() * (SCREEN_W - 20);
  holeY = 10 + Math.random() * (SCREEN_H - 20);
  playInProgress = true;
  shotCount = 0;
  startupClock = ROUND_STARTUP_TIME;
}

function egg_client_init() {
  if (egg.texture_load_image(texid_font = egg.texture_new(), 0, 1) < 0) return -1;
  if (egg.texture_load_image(texid_tiles = egg.texture_new(), 0, 3) < 0) return -1;
  egg.event_enable(8, 3); // MMOTION
  egg.event_enable(9, 3); // MBUTTON
  resetGame();
  return 0;
}

/* Cutting every corner with the physics, I mean we have just one moving element and it's a circle, easy!
 */
function resolveCollisions() {
  const xlo = ballX - BALL_RADIUS;
  const ylo = ballY - BALL_RADIUS;
  const xhi = ballX + BALL_RADIUS;
  const yhi = ballY + BALL_RADIUS;
  if (xlo < 0) {
    ballX = BALL_RADIUS;
    if (ballDx < 0) {
      egg.audio_play_sound(0, SOUND_BOUNCE, 1.0, 0.0);
      ballDx = -ballDx;
    }
  } else if (xhi > SCREEN_W) {
    ballX = SCREEN_W - BALL_RADIUS;
    if (ballDx > 0) {
      egg.audio_play_sound(0, SOUND_BOUNCE, 1.0, 0.0);
      ballDx = -ballDx;
    }
  }
  if (ylo < 0) {
    ballY = BALL_RADIUS;
    if (ballDy < 0) {
      egg.audio_play_sound(0, SOUND_BOUNCE, 1.0, 0.0);
      ballDy = -ballDy;
    }
  } else if (yhi > SCREEN_H) {
    ballY = SCREEN_H - BALL_RADIUS;
    if (ballDy > 0) {
      egg.audio_play_sound(0, SOUND_BOUNCE, 1.0, 0.0);
      ballDy = -ballDy;
    }
  }
}

function checkHole() {
  const dx = Math.abs(ballX - holeX);
  if (dx >= HOLE_RADIUS) return;
  const dy = Math.abs(ballY - holeY);
  if (dy >= HOLE_RADIUS) return;
  const d2 = dx * dx + dy * dy;
  if (d2 >= HOLE_RADIUS2) return;
  ballX = holeX;
  ballY = holeY;
  ballVelocity = 0;
  playInProgress = false;
  resetClock = ROUND_DENOUEMENT_TIME;
  egg.audio_play_sound(0, SOUND_HOLE, 1.0, 0.0);
}

function egg_client_update(elapsed) {
  for (const event of egg.event_next()) switch (event.eventType) {
  
    case 8: { // MMOTION
        const dx = event.v0 - ballX;
        const dy = event.v1 - ballY;
        shotMagnitude = Math.sqrt(dx * dx + dy * dy);
        if (dx || dy) {
          shotDx = -dx / shotMagnitude;
          shotDy = -dy / shotMagnitude;
        }
        shotMagnitude = Math.max(MAG_MIN, Math.min(MAG_MAX, shotMagnitude));
      } break;
      
    case 9: { // MBUTTON
        if ((event.v0 === 1) && (event.v1 === 1) && !ballVelocity && playInProgress) {
          egg.audio_play_sound(0, SOUND_PUT, 1.0, 0.0);
          ballDx = shotDx;
          ballDy = shotDy;
          ballVelocity = shotMagnitude * BALL_VELOCITY_SCALE;
          shotCount++;
        }
      } break;
  
  }
  
  if (resetClock > 0) {
    if ((resetClock -= elapsed) <= 0) {
      resetClock = 0;
      resetGame();
    }
  }
  
  if (startupClock > 0) {
    if ((startupClock -= elapsed) <= 0) {
      startupClock = 0;
    }
  }
  
  if ((shotAnimClock -= elapsed) <= 0) {
    shotAnimClock += SHOT_ANIM_TIME;
    if (++shotAnimFrame >= 4) {
      shotAnimFrame = 0;
    }
  }
  
  if (ballVelocity > 0) {
    ballX += ballVelocity * ballDx * elapsed;
    ballY += ballVelocity * ballDy * elapsed;
    ballVelocity *= 0.700 ** elapsed;
    if (ballVelocity < 10) {
      ballVelocity = 0;
    }
    resolveCollisions();
    checkHole();
  }
}

function egg_client_render() {
  egg.draw_rect(1, 0, 0, SCREEN_W, SCREEN_H, 0x108030ff);
  tileRenderer.begin(texid_tiles, 0, 0xff);
  tileRenderer.tile(holeX - 8, holeY - 8, 0x10, 0);
  tileRenderer.tile(holeX + 8, holeY - 8, 0x11, 0);
  tileRenderer.tile(holeX - 8, holeY + 8, 0x20, 0);
  tileRenderer.tile(holeX + 8, holeY + 8, 0x21, 0);
  tileRenderer.tile(ballX, ballY, 0x00, 0);
  if (playInProgress && !ballVelocity) {
    const rangex = shotDx * shotMagnitude;
    const rangey = shotDy * shotMagnitude;
    const shotTile = (d) => {
      switch ((shotAnimFrame + d) & 3) {
        case 0: return 0x03;
        case 1: return 0x02;
        case 2: return 0x01;
        default: return 0x02;
      }
    };
    tileRenderer.tile(ballX + rangex * 0.250, ballY + rangey * 0.250, shotTile(0), 0);
    tileRenderer.tile(ballX + rangex * 0.500, ballY + rangey * 0.500, shotTile(1), 0);
    tileRenderer.tile(ballX + rangex * 0.750, ballY + rangey * 0.750, shotTile(2), 0);
    tileRenderer.tile(ballX + rangex * 1.000, ballY + rangey * 1.000, shotTile(3), 0);
  }
  tileRenderer.end();
  if (resetClock > 0) {
    const alpha = 0xff - Math.min(0xff, Math.max(0, Math.floor((resetClock * 0xff) / ROUND_DENOUEMENT_TIME)));
    egg.draw_rect(1, 0, 0, SCREEN_W, SCREEN_H, 0x00000000 | alpha);
    tileRenderer.begin(texid_font, 0xffffffff, 0xff);
    tileRenderer.string((SCREEN_W >> 1) - 60, (SCREEN_H >> 1), `Hole in ${shotCount} shot${(shotCount === 1) ? '' : 's'}`);
    tileRenderer.end();
  } else if (startupClock > 0) {
    const alpha = Math.min(0xff, Math.max(0, Math.floor((startupClock * 0xff) / ROUND_STARTUP_TIME)));
    egg.draw_rect(1, 0, 0, SCREEN_W, SCREEN_H, 0x00000000 | alpha);
  }
}

exportModule({
  egg_client_init,
  egg_client_update,
  egg_client_render,
});
