/* jlog.h
 * Logical joystick mapping.
 * This is the big kahuna of our input system.
 * Takes input exclusively from joy2, ie normalized two-state buttons.
 * Produces a 16-bit state for an arbitrary set of "players".
 */
 
#ifndef JLOG_H
#define JLOG_H

struct jlog;
struct cbus;

//TODO
//TODO How to communicate with joy2?

static inline void jlog_del(struct jlog *jlog) {}

static inline struct jlog *jlog_new(struct bus *bus) { return (struct jlog*)""; }

static inline void jlog_event(struct jlog *jlog,const struct egg_event *event) {}

static inline void jlog_enable(struct jlog *jlog,int enable) {}

#endif
