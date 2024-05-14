#include <egg/egg.h>
#include "menu.h"
#include "stdlib/egg-stdlib.h"
#include "util/tile_renderer.h"
#include "util/font.h"

/* Object definition.
 */
 
struct menu_accelerometer {
  struct menu hdr;
  int disabled;
  int x,y,z;
};

#define MENU ((struct menu_accelerometer*)menu)

/* Cleanup.
 */
 
static void _accelerometer_del(struct menu *menu) {
  egg_event_enable(EGG_EVENT_ACCEL,0);
}

/* Event.
 */
 
static void _accelerometer_event(struct menu *menu,const union egg_event *event) {
  switch (event->type) {
    case EGG_EVENT_ACCEL: {
        MENU->x=event->accel.x;
        MENU->y=event->accel.y;
        MENU->z=event->accel.z;
      } break;
  }
}

/* Update.
 */
 
static void _accelerometer_update(struct menu *menu,double elapsed) {
}

/* Render.
 */
 
static void _accelerometer_render(struct menu *menu) {
  if (!menu->tile_renderer) return;
  if (MENU->disabled) {
    tile_renderer_begin(menu->tile_renderer,menu->tilesheet,0xff0000ff,0xff);
    tile_renderer_string(menu->tile_renderer,16,16,"No accelerometer",-1);
    tile_renderer_end(menu->tile_renderer);
  } else {
    char msg[11]; // eg "x: 00000000"
    #define _(fld,x,y) { \
      msg[0]=(#fld)[0]; \
      msg[1]=':'; \
      msg[2]=' '; \
      msg[3]="0123456789abcdef"[(MENU->fld>>28)&0xf]; \
      msg[4]="0123456789abcdef"[(MENU->fld>>24)&0xf]; \
      msg[5]="0123456789abcdef"[(MENU->fld>>20)&0xf]; \
      msg[6]="0123456789abcdef"[(MENU->fld>>16)&0xf]; \
      msg[7]="0123456789abcdef"[(MENU->fld>>12)&0xf]; \
      msg[8]="0123456789abcdef"[(MENU->fld>> 8)&0xf]; \
      msg[9]="0123456789abcdef"[(MENU->fld>> 4)&0xf]; \
      msg[10]="0123456789abcdef"[(MENU->fld    )&0xf]; \
      tile_renderer_string(menu->tile_renderer,x,y,msg,11); \
    }
    tile_renderer_begin(menu->tile_renderer,menu->tilesheet,0xffffffff,0xff);
    _(x,40,40)
    _(y,40,48)
    _(z,40,56)
    tile_renderer_end(menu->tile_renderer);
    #undef _
  }
}

/* New.
 */
 
struct menu *menu_new_accelerometer(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu_accelerometer),parent);
  if (!menu) return 0;
  menu->del=_accelerometer_del;
  menu->event=_accelerometer_event;
  menu->update=_accelerometer_update;
  menu->render=_accelerometer_render;
  
  if (!egg_event_enable(EGG_EVENT_ACCEL,1)) {
    MENU->disabled=1;
  }
  
  return menu;
}
