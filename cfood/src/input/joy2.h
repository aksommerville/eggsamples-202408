/* joy2.h
 * Canonicalizes joysticks and keyboard to a set of 2-state buttons.
 * This service is always running, and is the authority for mappable joystick input.
 * We're the only party examining raw INPUT events.
 */
 
#ifndef JOY2_H
#define JOY2_H

/* All devid >0 are reserved by the platform, and zero reserved as null.
 * We use negative devid for fake devices.
 * KEYBOARD is always on.
 * TOUCH is on if you request it.
 */
#define JOY2_DEVID_KEYBOARD -1
#define JOY2_DEVID_TOUCH -2
 
#define JOY2_MODE_HAT 1
#define JOY2_MODE_AXIS 2

struct joy2;
struct bus;

void joy2_del(struct joy2 *joy2);

struct joy2 *joy2_new(struct bus *bus);

void joy2_event(struct joy2 *joy2,const struct egg_event *event);

/* INPUT, CONNECT, and DISCONNECT events are always processed.
 * You have to tell us whether and when to process KEY and TOUCH events.
 * Changing either of these will report the connection/disconnection of a fake device.
 */
void joy2_enable_keyboard(struct joy2 *joy2,int enable);
void joy2_enable_touch(struct joy2 *joy2,int enable);

/* Examine devices.
 * Our 'get_button' only reports hats and two-way axes.
 * One-way axes and two-state buttons work by default and we don't record the full set.
 */
int joy2_device_exists(const struct joy2 *joy2,int devid);
const char *joy2_device_get_ids(int *vid,int *pid,int *version,int *mapping,const struct joy2 *joy2,int devid);
int joy2_for_each_device(
  const struct joy2 *joy2,
  int (*cb)(int devid,int vid,int pid,int version,int mapping,const char *name,void *userdata),
  void *userdata
);
int joy2_device_get_button(int *btnid,int *mode,const struct joy2 *joy2,int devid,int p);

/* Register a listener for all state changes.
 * There is a fake button (btnid=0,part='c') representing connection of a device.
 * Otherwise, (btnid) is what we got from the platform and (part) is one of:
 *   'b': Plain two-state button. (or unsigned axis)
 *   'l','h': Signed axis, Low and High ranges.
 *   'w','e','n','s': Hat, as four buttons.
 * When we send (0,'c',1) the device is ready for inspection.
 * When we send (0,'c',0) the device is already removed.
 */
int joy2_listen(
  struct joy2 *joy2,
  void (*cb)(int devid,int btnid,char part,int value,void *userdata),
  void *userdata
);
void joy2_unlisten(struct joy2 *joy2,int listenerid);

#endif
