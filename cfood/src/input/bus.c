#include <egg/egg.h>
#include <stdlib.h>
#include "bus.h"
#include "fkbd.h"
#include "fkpt.h"
#include "joy2.h"
#include "jlog.h"
#include "jcfg.h"

/* Object definition.
 */
 
struct bus {
  int mode;
  struct bus_delegate delegate;
  struct fkbd *fkbd;
  struct fkpt *fkpt;
  struct joy2 *joy2;
  struct jlog *jlog;
  struct jcfg *jcfg;
};

/* Delete.
 */

void bus_del(struct bus *bus) {
  if (!bus) return;
  bus_mode_raw(bus); // Important to turn everything off before we delete anything.
  fkbd_del(bus->fkbd);
  fkpt_del(bus->fkpt);
  joy2_del(bus->joy2);
  jlog_del(bus->jlog);
  jcfg_del(bus->jcfg);
  free(bus);
}

/* New.
 */

struct bus *bus_new(const struct bus_delegate *delegate) {
  struct bus *bus=calloc(1,sizeof(struct bus));
  if (!bus) return 0;
  bus->mode=BUS_MODE_RAW;
  bus->delegate=*delegate;
  if (
    !(bus->fkbd=fkbd_new(bus))||
    !(bus->fkpt=fkpt_new(bus))||
    !(bus->joy2=joy2_new(bus))||
    !(bus->jlog=jlog_new(bus))||
    !(bus->jcfg=jcfg_new(bus))||
  0) {
    bus_del(bus);
    return 0;
  }
  return bus;
}

/* Trivial accessors.
 */
 
struct fkbd *bus_get_fkbd(const struct bus *bus) {
  return bus->fkbd;
}

struct fkpt *bus_get_fkpt(const struct bus *bus) {
  return bus->fkpt;
}

struct joy2 *bus_get_joy2(const struct bus *bus) {
  return bus->joy2;
}

struct jlog *bus_get_jlog(const struct bus *bus) {
  return bus->jlog;
}

struct jcfg *bus_get_jcfg(const struct bus *bus) {
  return bus->jcfg;
}

/* Mode.
 */
 
int bus_get_mode(const struct bus *bus) {
  return bus->mode;
}

void bus_mode_raw(struct bus *bus) {
  if (bus->mode==BUS_MODE_RAW) return;
  fkbd_enable(bus->fkbd,0);
  fkpt_enable(bus->fkpt,0);
  joy2_enable_keyboard(bus->joy2,0);
  joy2_enable_touch(bus->joy2,0);
  jlog_enable(bus->jlog,0);
  jcfg_enable(bus->jcfg,0);
  bus->mode=BUS_MODE_RAW;
}

void bus_mode_joy(struct bus *bus) {
  if (bus->mode==BUS_MODE_JOY) return;
  fkbd_enable(bus->fkbd,0);
  fkpt_enable(bus->fkpt,0);
  jcfg_enable(bus->jcfg,0);
  joy2_enable_keyboard(bus->joy2,1);
  joy2_enable_touch(bus->joy2,1);
  jlog_enable(bus->jlog,1);
  bus->mode=BUS_MODE_JOY;
}

void bus_mode_text(struct bus *bus) {
  if (bus->mode==BUS_MODE_TEXT) return;
  fkpt_enable(bus->fkpt,0);
  joy2_enable_keyboard(bus->joy2,0);
  joy2_enable_touch(bus->joy2,0);
  jlog_enable(bus->jlog,1); // jlog stays on, just we don't feed it key or touch anymore.
  jcfg_enable(bus->jcfg,0);
  fkbd_enable(bus->fkbd,1);
  bus->mode=BUS_MODE_TEXT;
}

void bus_mode_point(struct bus *bus) {
  if (bus->mode==BUS_MODE_POINT) return;
  fkbd_enable(bus->fkbd,0);
  joy2_enable_keyboard(bus->joy2,1);
  joy2_enable_touch(bus->joy2,0);
  jlog_enable(bus->jlog,1); // jlog stays on, and we feed it key but not touch.
  jcfg_enable(bus->jcfg,0);
  fkpt_enable(bus->fkpt,1);
  bus->mode=BUS_MODE_POINT;
}

void bus_mode_config(struct bus *bus,int devid) {
  if (!joy2_device_exists(bus->joy2,devid)) return;
  fkbd_enable(bus->fkbd,0);
  fkpt_enable(bus->fkpt,0);
  jlog_enable(bus->jlog,0);
  joy2_enable_keyboard(bus->joy2,1);
  joy2_enable_touch(bus->joy2,1);
  jcfg_enable(bus->jcfg,devid);
  bus->mode=BUS_MODE_CONFIG;
}

/* Receive event.
 */
 
static void bus_event(struct bus *bus,const struct egg_event *event) {
  if (bus->delegate.on_event) bus->delegate.on_event(event);
  switch (event->type) {
  }
  joy2_event(bus->joy2,event);
  switch (bus->mode) {
    case BUS_MODE_TEXT: fkbd_event(bus->fkbd,event); break;
    case BUS_MODE_POINT: fkpt_event(bus->fkpt,event); break;
  }
}

/* Update.
 */

void bus_update(struct bus *bus,double elapsed) {
  struct egg_event eventv[16];
  int eventc;
  while ((eventc=egg_event_next(eventv,16))>0) {
    const struct egg_event *event=eventv;
    for (;eventc-->0;event++) bus_event(bus,event);
  }
  switch (bus->mode) {
    case BUS_MODE_TEXT: fkbd_update(bus->fkbd,elapsed); break;
    case BUS_MODE_POINT: fkpt_update(bus->fkpt,elapsed); break;
    case BUS_MODE_CONFIG: jcfg_update(bus->jcfg,elapsed); break;
  }
}

/* Render.
 */
 
void bus_render(struct bus *bus) {
  switch (bus->mode) {
    case BUS_MODE_TEXT: fkbd_render(bus->fkbd); break;
    case BUS_MODE_POINT: fkpt_render(bus->fkpt); break;
    case BUS_MODE_CONFIG: jcfg_render(bus->jcfg); break;
  }
}

/* Public access to delegate callbacks.
 */
 
void bus_on_text(struct bus *bus,int codepoint) {
  if (bus->delegate.on_text) bus->delegate.on_text(codepoint);
}

void bus_on_mmotion(struct bus *bus,int x,int y) {
  if (bus->delegate.on_mmotion) bus->delegate.on_mmotion(x,y);
}

void bus_on_mbutton(struct bus *bus,int btnid,int value,int x,int y) {
  if (bus->delegate.on_mbutton) bus->delegate.on_mbutton(btnid,value,x,y);
}

void bus_on_mwheel(struct bus *bus,int dx,int dy,int x,int y) {
  if (bus->delegate.on_mwheel) bus->delegate.on_mwheel(dx,dy,x,y);
}

void bus_on_player(struct bus *bus,int plrid,int btnid,int value,int state) {
  if (bus->delegate.on_player) bus->delegate.on_player(plrid,btnid,value,state);
}
