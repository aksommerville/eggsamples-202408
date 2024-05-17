/* inkeep.h
 * High-level input manager.
 * We take control of the relationship to Egg input.
 * You must not call egg_event_next() or egg_event_enable().
 *
 * We present input in one of several modes at a time, coercing from other devices where possible:
 *  - RAW
 *  - - The default. No extra processing here, events go to you just as they arrived from the platform.
 *  - JOY
 *  - - Creates an abstraction for multiple "players" each with a 16-bit state.
 *  - - Keyboard and touch always map to player one, and so does the first joystick.
 *  - - Analogue sticks and AUX3 are not reported, otherwise what you see is Standard Gamepad Mapping.
 *  - TEXT
 *  - - Text events from the platform pass thru untouched.
 *  - - We also always present a fake on-screen keyboard accessible to mouse, touch, and joysticks.
 *  - TOUCH
 *  - - If a mouse, keyboard, or joystick appears to be present, we show a fake cursor.
 *  - - You'll receive Press, Release, and Move. Beware that Move might only happen while pressed.
 *  - - From real TOUCH events, we will only report one at a time.
 *  - MOUSE
 *  - - Show a fake cursor always, accessible to mouse, joystick, and keyboard.
 *  - - Report motion and multiple buttons.
 *  - - Not accessible to touch-only devices like mobile, so prefer TOUCH if workable.
 *  - POV
 *  - - Report motion+orientation+buttons, from WASD+Mouse or joysticks.
 *  - - TODO Not sure I'll bother with this; Egg is really not meant for 3D graphics, and this style doesn't make sense in 2D.
 *
 * Accelerometer does not participate in these.
 * You can enable or disable it and we'll pass the events along if they arrive, regardless of mode.
 */
 
#ifndef INKEEP_H
#define INKEEP_H

union egg_event;
struct font;

#define INKEEP_MODE_RAW    1
#define INKEEP_MODE_JOY    2
#define INKEEP_MODE_TEXT   3
#define INKEEP_MODE_TOUCH  4
#define INKEEP_MODE_MOUSE  5
#define INKEEP_MODE_POV    6

/* We have a hook for each of the client hooks.
 * Call our 'render' after you've drawn your scene.
 */
void inkeep_quit();
int inkeep_init();
void inkeep_update(double elapsed);
void inkeep_render();

/* Setup.
 * You can change things at any time.
 * Most of these, it's typical to set just once right after init.
 **********************************************************************/

/* Font for the onscreen keyboard.
 * When you set one, we forget the other.
 * We borrow the font object, you must keep it alive.
 */
int inkeep_set_font_tiles(int imageid);
int inkeep_set_font(struct font *font);

/* (caps) must contain (pagec*rowc*colc) codepoints, encoded UTF-8.
 * All pages must have the same geometry.
 * Codepoints U+0..U+7 are reserved for fake keyboard features:
 *   U+0: Unused, focus should skip
 *   U+1: Next Page
 *   U+2: Previous Page
 *   U+3: Quick Shift
 *   (4,5,6,7) might be defined in the future.
 * Quick Shift changes to the next page if even, previous if odd, for only the next key entry.
 * Whether they're in your keycaps or not, we might report artificial U+8 and U+a for backspace and enter.
 */
int inkeep_set_keycaps(int pagec,int rowc,int colc,const char *caps);

/* Image for fake pointer.
 * Zeroes for (w,h) is OK to use the entire image.
 * (hotx,hoty) should be in (0,0)..(w-1,h-1).
 * It's advisable to put all your cursor in the same image; if so, changing between them is dirt cheap.
 */
int inkeep_set_cursor(int imageid,int x,int y,int w,int h,int xform,int hotx,int hoty);

/* How many players should we map to, in JOY mode?
 * Minimum 1 and a maximum you're unlikely to see.
 * Each device maps to one player (or none).
 * There's a special player zero which is the aggregate of all player states.
 * Returns the actual count applied. Maybe be more than the count of available devices.
 */
int inkeep_set_player_count(int playerc);

/* Setting mode returns the mode we actually ended up in.
 * We might reject a change if we know it's not workable (eg MOUSE on a smartphone).
 * You can set an invalid mode, eg zero, to only get current.
 */
int inkeep_set_mode(int mode);

/* Current height of onscreen keyboard in pixels.
 * Zero if we're not displaying one right now.
 * Our onscreen keyboard always occupies the full width, at the bottom.
 */
int inkeep_get_keyboard_height();

/* Event delivery.
 * Register or unregister listeners at any time.
 * Each 'listen' function returns a postive integer on success that you can unlisten with later.
 **************************************************************/
 
void inkeep_unlisten(int id);

/* Events straight from the platform, before we do any processing on them.
 */
int inkeep_listen_raw(void (*cb)(const union egg_event *event,void *userdata),void *userdata);

int inkeep_listen_joy(void (*cb)(int plrid,int btnid,int value,int state,void *userdata),void *userdata);

int inkeep_listen_text(void (*cb)(int codepoint,void *userdata),void *userdata);

/* (state) is (0,1,2)=(Release,Press,Move), same as the platform API.
 * If only real touch events are available, Move can only happen while Pressed.
 * If we're faking it via mouse, keyboard, or joystick, you can get Move at all times.
 */
int inkeep_listen_touch(void (*cb)(int state,int x,int y,void *userdata),void *userdata);

int inkeep_listen_mouse(
  void (*cb_motion)(int x,int y,void *userdata),
  void (*cb_button)(int btnid,int value,int x,int y,void *userdata),
  void (*cb_wheel)(int dx,int dy,int x,int y,void *userdata),
  void *userdata
);

/* (dx,dy) in -1..1.
 * (rx,ry) in -128..127.
 * TODO (buttons)
 */
int inkeep_listen_pov(void (*cb)(int dx,int dy,int rx,int ry,int buttons,void *userdata),void *userdata);

int inkeep_listen_accel(void (*cb)(int x,int y,int z,void *userdata),void *userdata);

/* Register a listener for each non-null argument, all with the same ID.
 */
int inkeep_listen_all(
  void (*cb_raw)(const union egg_event *event,void *userdata),
  void (*cb_joy)(int plrid,int btnid,int value,int state,void *userdata),
  void (*cb_text)(int codepoint,void *userdata),
  void (*cb_touch)(int state,int x,int y,void *userdata),
  void (*cb_mmotion)(int x,int y,void *userdata),
  void (*cb_mbutton)(int btnid,int value,int x,int y,void *userdata),
  void (*cb_mwheel)(int dx,int dy,int x,int y,void *userdata),
  void (*cb_pov)(int dx,int dy,int rx,int ry,int buttons,void *userdata),
  void (*cb_accel)(int x,int y,int z,void *userdata),
  void  *userdata
);

#endif
