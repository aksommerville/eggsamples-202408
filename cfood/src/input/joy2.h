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

struct joy2;
struct bus;

//TODO

static inline void joy2_del(struct joy2 *joy2) {}

static inline struct joy2 *joy2_new(struct bus *bus) { return (struct joy2*)""; }

static inline void joy2_event(struct joy2 *joy2,const struct egg_event *event) {}

static inline void joy2_enable_touch(struct joy2 *joy2,int enable) {}

static inline int joy2_device_exists(const struct joy2 *joy2,int devid) { return 0; }

#endif
