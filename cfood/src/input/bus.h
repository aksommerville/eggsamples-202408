/* bus.h
 * General input manager for Egg.
 */
 
#ifndef BUS_H
#define BUS_H

struct bus;

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
   * These might be the real thing, or we might be faking it, you don't need to care.
   * You will only get on_text when in TEXT mode, and on_m* in POINT mode.
   */
  void (*on_text)(int codepoint);
  void (*on_mmotion)(int x,int y);
  void (*on_mbutton)(int btnid,int value,int x,int y);
  void (*on_mwheel)(int dx,int dy,int x,int y);
  
  /* In JOY mode, all player state changes go here.
   * plrid zero is the aggregate of all players.
   * plrid one always exists.
   */
  void (*on_player)(int plrid,int btnid,int value,int state);
};

void bus_del(struct bus *bus);

struct bus *bus_new(const struct bus_delegate *delegate);

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
