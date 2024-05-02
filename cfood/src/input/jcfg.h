/* jcfg.h
 * Interactive joystick configuration.
 * We take input from joy2 and present a modal UI "Press button A", etc.
 */
 
#ifndef JCFG_H
#define JCFG_H

struct jcfg;
struct bus;

//TODO

static inline void jcfg_del(struct jcfg *jcfg) {}

static inline struct jcfg *jcfg_new(struct bus *bus) { return (struct jcfg*)""; }

static inline void jcfg_update(struct jcfg *jcfg,double elapsed) {}
static inline void jcfg_render(struct jcfg *jcfg) {}

// devid zero to disable
static inline void jcfg_enable(struct jcfg *jcfg,int devid) {}

#endif
