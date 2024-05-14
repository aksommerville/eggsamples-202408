#include <egg/egg.h>
#include "menu.h"
#include "stdlib/egg-stdlib.h"
#include "util/tile_renderer.h"
#include "util/font.h"

/* Object definition.
 */
 
struct menu_mouselock {
  struct menu hdr;
  int scrollx,scrolly;
};

#define MENU ((struct menu_mouselock*)menu)

/* Cleanup.
 */
 
static void _mouselock_del(struct menu *menu) {
  egg_lock_cursor(0);
  //egg_show_cursor(1);
}

/* Event.
 */
 
static void _mouselock_event(struct menu *menu,const union egg_event *event) {
  switch (event->type) {
    case EGG_EVENT_MMOTION: {
        MENU->scrollx-=event->mmotion.x;
        MENU->scrolly-=event->mmotion.y;
      } break;
    case EGG_EVENT_MBUTTON: {
        if ((event->mbutton.btnid==EGG_MBUTTON_LEFT)&&event->mbutton.value) {
          pop_menu();
        }
      } break;
  }
}

/* Update.
 */
 
static void _mouselock_update(struct menu *menu,double elapsed) {
}

/* Render.
 */
 
static void _mouselock_render(struct menu *menu) {
  const int tilesize=80;
  const int halftile=tilesize>>1;
  int x0=MENU->scrollx%tilesize;
  int y0=MENU->scrolly%tilesize;
  if (x0>0) x0-=tilesize;
  if (y0>0) y0-=tilesize;
  int y=y0; for (;y<menu->screenh;y+=tilesize) {
    int x=x0; for (;x<menu->screenw;x+=tilesize) {
      struct egg_draw_line vtxv[]={
        {x         ,y         ,0x00,0x00,0x00,0x00},
        {x         ,y+tilesize,0x00,0x00,0x00,0x00},
        {x+halftile,y+halftile,0xff,0xff,0xff,0xc0},
        {x+tilesize,y+tilesize,0x00,0x00,0x00,0x00},
        {x+tilesize,y         ,0x00,0x00,0x00,0x00},
        {x+halftile,y+halftile,0xff,0xff,0xff,0xc0},
        {x         ,y         ,0x00,0x00,0x00,0x00},
      };
      egg_draw_trig(1,vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
    }
  }
}

/* Prepare a warning label, for if egg_lock_cursor() failed.
 */
 
static void mouselock_show_failed(struct menu *menu) {
}

/* New.
 */
 
struct menu *menu_new_mouselock(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu_mouselock),parent);
  if (!menu) return 0;
  menu->del=_mouselock_del;
  menu->event=_mouselock_event;
  menu->update=_mouselock_update;
  menu->render=_mouselock_render;
  
  if (egg_lock_cursor(1)>0) {
    egg_log("locked cursor");
  } else {
    egg_log("egg_lock_cursor failed!");
    mouselock_show_failed(menu);
  }
  
  return menu;
}
