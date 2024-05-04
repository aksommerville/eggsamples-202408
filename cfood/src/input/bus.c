#include <egg/egg.h>
#include <stdlib.h>
#include "bus.h"
#include "fkbd.h"
#include "fkpt.h"
#include "joy2.h"
#include "jlog.h"
#include "jcfg.h"

/* For so long after any touch event, we'll ignore all natural mouse events.
 * This is to work around web browsers reporting the same event twice, as both touch and mouse.
 */
#define MOUSE_BLACKOUT_TIME 0.125

/* Object definition.
 */
 
struct bus {
  int mode;
  int restore_mode;
  double mouse_blackout;
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
  // TEXT are not necessarily enabled by default, but we want them on always.
  egg_event_enable(EGG_EVENT_TEXT,EGG_EVTSTATE_ENABLED);
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

/* Setup, dispatch to helpers.
 */
 
void bus_set_cursor(struct bus *bus,
  int texid,int x,int y,int w,int h,int xform,
  int hotx,int hoty
) {
  fkpt_set_cursor(bus->fkpt,texid,x,y,w,h,xform,hotx,hoty);
}

void bus_set_font(struct bus *bus,struct font *font) {
  fkbd_set_font(bus->fkbd,font);
  jcfg_set_font(bus->jcfg,font);
}

void bus_set_tilesheet(struct bus *bus,int texid) {
  fkbd_set_tilesheet(bus->fkbd,texid);
  jcfg_set_tilesheet(bus->jcfg,texid);
}

int bus_set_keycaps(struct bus *bus,int colc,int rowc,int pagec,const int *codepointv) {
  return fkbd_set_keycaps(bus->fkbd,colc,rowc,pagec,codepointv);
}

void bus_set_strings(struct bus *bus,int d,int f,int p,int a) {
  jcfg_set_strings(bus->jcfg,d,f,p,a);
}

void bus_define_joystick_mapping(struct bus *bus,const int *v,int c) {
  jlog_define_mapping(bus->jlog,v,c);
}

void bus_set_player_count(struct bus *bus,int playerc) {
  jlog_set_player_count(bus->jlog,playerc);
}

/* Mode.
 */
 
int bus_get_player(const struct bus *bus,int playerid) {
  return jlog_get_player(bus->jlog,playerid);
}
 
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
  if ((devid!=JCFG_DEVID_ANY)&&!joy2_device_exists(bus->joy2,devid)) return;
  if (bus->mode!=BUS_MODE_CONFIG) bus->restore_mode=bus->mode;
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
    case EGG_EVENT_MMOTION: {
        if (bus->mouse_blackout>0.0) return;
        if (bus->delegate.on_mmotion&&(bus->mode!=BUS_MODE_POINT)) bus->delegate.on_mmotion(event->v[0],event->v[1]);
      } break;
    case EGG_EVENT_MBUTTON: {
        if (bus->mouse_blackout>0.0) return;
        if (bus->delegate.on_mbutton&&(bus->mode!=BUS_MODE_POINT)) bus->delegate.on_mbutton(event->v[0],event->v[1],event->v[2],event->v[3]);
      } break;
    case EGG_EVENT_MWHEEL: {
        if (bus->mouse_blackout>0.0) return;
        if (bus->delegate.on_mwheel&&(bus->mode!=BUS_MODE_POINT)) bus->delegate.on_mwheel(event->v[0],event->v[1],event->v[2],event->v[3]);
      } break;
    case EGG_EVENT_TEXT: if (bus->delegate.on_text&&(bus->mode!=BUS_MODE_TEXT)) bus->delegate.on_text(event->v[0]); break;
    case EGG_EVENT_TOUCH: bus->mouse_blackout=MOUSE_BLACKOUT_TIME; break;
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
  if (bus->mouse_blackout>0.0) bus->mouse_blackout-=elapsed;
  struct egg_event eventv[16];
  int eventc;
  while ((eventc=egg_event_next(eventv,16))>0) {
    const struct egg_event *event=eventv;
    for (;eventc-->0;event++) bus_event(bus,event);
  }
  switch (bus->mode) {
    case BUS_MODE_TEXT: fkbd_update(bus->fkbd,elapsed); break;
    case BUS_MODE_POINT: fkpt_update(bus->fkpt,elapsed); break;
    case BUS_MODE_CONFIG: {
        jcfg_update(bus->jcfg,elapsed);
        if (jcfg_is_complete(bus->jcfg)) switch (bus->restore_mode) {
          case BUS_MODE_RAW: bus_mode_raw(bus); if (bus->delegate.on_mode_change) bus->delegate.on_mode_change(bus->mode); break;
          case BUS_MODE_JOY: bus_mode_joy(bus); if (bus->delegate.on_mode_change) bus->delegate.on_mode_change(bus->mode); break;
          case BUS_MODE_TEXT: bus_mode_text(bus); if (bus->delegate.on_mode_change) bus->delegate.on_mode_change(bus->mode); break;
          case BUS_MODE_POINT: bus_mode_point(bus); if (bus->delegate.on_mode_change) bus->delegate.on_mode_change(bus->mode); break;
        }
      } break;
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
  egg_log("%s %d=%d @%d,%d",__func__,btnid,value,x,y);
  if (bus->delegate.on_mbutton) bus->delegate.on_mbutton(btnid,value,x,y);
}

void bus_on_mwheel(struct bus *bus,int dx,int dy,int x,int y) {
  if (bus->delegate.on_mwheel) bus->delegate.on_mwheel(dx,dy,x,y);
}

void bus_on_player(struct bus *bus,int plrid,int btnid,int value,int state) {
  if (bus->delegate.on_player) bus->delegate.on_player(plrid,btnid,value,state);
}
