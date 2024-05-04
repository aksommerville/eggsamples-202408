/* bus.h
 * General input manager for Egg.
 */
 
#ifndef BUS_H
#define BUS_H

struct bus;
struct font;

//TODO We should probably supply a network abstraction like our TS cousin.

/* We start in RAW mode, where we do nothing special, just forwarding events.
 * JOY: Map joysticks fully, use keyboard as a fake joystick, turn touch events into dpad+2, mouse off.
 * TEXT: Keyboard text from platform, and an onscreen keyboard accessible to joysticks, mouse, and touch.
 * POINT: Present a fake pointer controllable by mouse, touch, keyboard, and joystick.
 * CONFIG: Modal configuration for a joystick. We prompt "Press button X" etc.
 */
#define BUS_MODE_RAW 0
#define BUS_MODE_JOY 1
#define BUS_MODE_TEXT 2
#define BUS_MODE_POINT 3
#define BUS_MODE_CONFIG 4

/* You are required to supply a delegate.
 * Every delegate hook is optional.
 */
struct bus_delegate {

  /* Every event from the platform goes here first, regardless of our mode.
   * In RAW mode, this is the only callback you'll get.
   */
  void (*on_event)(const struct egg_event *event);
  
  /* Events that match their Egg counterpart neatly.
   * You can get natural text and mouse events at any time, if the platform provides them.
   * We only guarantee they're possible when bus is in the corresponding mode.
   */
  void (*on_text)(int codepoint);
  void (*on_mmotion)(int x,int y);
  void (*on_mbutton)(int btnid,int value,int x,int y);
  void (*on_mwheel)(int dx,int dy,int x,int y);
  
  /* Joysticks are always mapped to a set of logical "players".
   * In JOY mode, we map the keyboard and touch events to players too.
   * plrid zero is the aggregate of all players.
   * plrid one always exists.
   */
  void (*on_player)(int plrid,int btnid,int value,int state);
  
  /* Called when we change mode at a time when you didn't ask for it.
   * Currently only happens at the end of a jcfg cycle.
   */
  void (*on_mode_change)(int mode);
};

void bus_del(struct bus *bus);

/* Initialization.
 * It's legal to reinitialize any of these things at any time.
 * Realistically, everything except cursor should be a set-and-forget deal.
 ******************************************************************************/

struct bus *bus_new(const struct bus_delegate *delegate);

/* Replace the cursor image.
 */
void bus_set_cursor(struct bus *bus,
  int texid,int x,int y,int w,int h,int xform,
  int hotx,int hoty
);

/* Set a font object or 256-glyph tilesheet, for fake keyboard and config prompts.
 * I recommend using font rather than tilesheet.
 */
void bus_set_font(struct bus *bus,struct font *font);
void bus_set_tilesheet(struct bus *bus,int texid);

/* Keycaps for the onscreen keyboard.
 * We default to something containing all of ASCII G0 if you don't set it.
 * Length of (codepointv) must be (colc*rowc*pagec).
 */
int bus_set_keycaps(struct bus *bus,int colc,int rowc,int pagec,const int *codepointv);

/* Set prompts for joystick config.
 */
void bus_set_strings(struct bus *bus,
  int stringid_device, // "Device", will be followed by device name.
  int stringid_fault, // eg "Fault! Please press %."
  int stringid_press, // eg "Please press %."
  int stringid_again // eg "Press % again."
);

/* Length of (v) must be (c*2).
 * (v) contains [stringid,stdbtnid,...], indexed by bits in the player state.
 * That is, the name of a button and your hint of where it should locate by default.
 * See jlog.h for more detail.
 */
void bus_define_joystick_mapping(struct bus *bus,const int *v,int c);

/* (playerc) must be at least 1.
 * There is also always a fake player, id zero, which aggregates all the others.
 */
void bus_set_player_count(struct bus *bus,int playerc);

/* Routine operation.
 *******************************************************************************/

int bus_get_mode(const struct bus *bus);
void bus_mode_raw(struct bus *bus);
void bus_mode_joy(struct bus *bus);
void bus_mode_text(struct bus *bus);
void bus_mode_point(struct bus *bus);
void bus_mode_config(struct bus *bus,int devid);

/* Call these in the Egg client hooks.
 * Do not pump Egg's event queue yourself, we do it.
 */
void bus_update(struct bus *bus,double elapsed);
void bus_render(struct bus *bus);

int bus_get_player(const struct bus *bus,int playerid);

/* Less common things.
 ***************************************************************************/

#define JCFG_DEVID_ANY -999

/* The button IDs for Standard Mapping, as Egg reports them.
 */
#define STDBTN_SOUTH 0x80
#define STDBTN_EAST  0x81
#define STDBTN_WEST  0x82
#define STDBTN_NORTH 0x83
#define STDBTN_L1    0x84
#define STDBTN_R1    0x85
#define STDBTN_L2    0x86
#define STDBTN_R2    0x87
#define STDBTN_AUX2  0x88
#define STDBTN_AUX1  0x89
#define STDBTN_LP    0x8a
#define STDBTN_RP    0x8b
#define STDBTN_UP    0x8c
#define STDBTN_DOWN  0x8d
#define STDBTN_LEFT  0x8e
#define STDBTN_RIGHT 0x8f
#define STDBTN_AUX3  0x90
#define STDBTN_LX    0x40
#define STDBTN_LY    0x41
#define STDBTN_RX    0x42
#define STDBTN_RY    0x43

/* If the setup seems a bit much, you can bus_define_joystick_mapping() with this.
 * Put your button names in order, in your string resources, and provide the first one here.
 * I don't recommend using this actually; are you really using all eleven buttons? Better to declare only the ones you use.
 */
#define JLOG_DEFAULT_FULL_GAMEPAD(stringid) \
  (int[]){ \
    stringid+ 0,STDBTN_LEFT , \
    stringid+ 1,STDBTN_RIGHT, \
    stringid+ 2,STDBTN_UP   , \
    stringid+ 3,STDBTN_DOWN , \
    stringid+ 4,STDBTN_SOUTH, \
    stringid+ 5,STDBTN_WEST , \
    stringid+ 6,STDBTN_EAST , \
    stringid+ 7,STDBTN_NORTH, \
    stringid+ 8,STDBTN_L1   , \
    stringid+ 9,STDBTN_R1   , \
    stringid+10,STDBTN_L2   , \
    stringid+11,STDBTN_R2   , \
    stringid+12,STDBTN_AUX1 , \
    stringid+13,STDBTN_AUX2 , \
    stringid+14,STDBTN_AUX3 , \
  },15

/* Access to our supporting cast.
 * Some things need to be configured on these objects directly.
 * All are available at any time during bus's life.
 */
struct fkbd;
struct fkpt;
struct joy2;
struct jlog;
struct jcfg;
struct fkbd *bus_get_fkbd(const struct bus *bus);
struct fkpt *bus_get_fkpt(const struct bus *bus);
struct joy2 *bus_get_joy2(const struct bus *bus);
struct jlog *bus_get_jlog(const struct bus *bus);
struct jcfg *bus_get_jcfg(const struct bus *bus);

/* Hooks to call the delegate, or any other internal processing.
 * The supporting cast is expected to use these, no one else is (tho it's not a problem if you need to for some reason).
 */
void bus_on_text(struct bus *bus,int codepoint);
void bus_on_mmotion(struct bus *bus,int x,int y);
void bus_on_mbutton(struct bus *bus,int btnid,int value,int x,int y);
void bus_on_mwheel(struct bus *bus,int dx,int dy,int x,int y);
void bus_on_player(struct bus *bus,int plrid,int btnid,int value,int state);

#endif
