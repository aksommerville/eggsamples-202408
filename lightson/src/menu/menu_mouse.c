#include <egg/egg.h>
#include "menu.h"
#include "stdlib/egg-stdlib.h"
#include "util/tile_renderer.h"
#include "util/font.h"

#define WHEEL_DISPLAY_TIME 0.250

/* Object definition.
 */
 
struct menu_mouse {
  struct menu hdr;
  int mousex,mousey; // Last sampled position. (unfortunately we can't sample it at startup)
  int mousedx,mousedy; // Wheel motion for display.
  double mousedx_clock,mousedy_clock; // Counts down while wheel should display.
  uint8_t btnstate; // (1<<btnid), and 0x01 is "OTHER"
};

#define MENU ((struct menu_mouse*)menu)

/* Cleanup.
 */
 
static void _mouse_del(struct menu *menu) {
}

/* Event.
 */
 
static void _mouse_event(struct menu *menu,const union egg_event *event) {
  switch (event->type) {
    case EGG_EVENT_MMOTION: {
        MENU->mousex=event->mmotion.x;
        MENU->mousey=event->mmotion.y;
      } break;
    case EGG_EVENT_MBUTTON: {
        uint8_t bit=1;
        if ((event->mbutton.btnid>=1)&&(event->mbutton.btnid<=7)) bit=1<<event->mbutton.btnid;
        if (event->mbutton.value) MENU->btnstate|=bit;
        else MENU->btnstate&=~bit;
      } break;
    case EGG_EVENT_MWHEEL: {
        if (event->mwheel.dx) {
          MENU->mousedx=event->mwheel.dx;
          MENU->mousedx_clock=WHEEL_DISPLAY_TIME;
        }
        if (event->mwheel.dy) {
          MENU->mousedy=event->mwheel.dy;
          MENU->mousedy_clock=WHEEL_DISPLAY_TIME;
        }
      } break;
  }
}

/* Update.
 */
 
static void _mouse_update(struct menu *menu,double elapsed) {
  if (MENU->mousedx_clock>0.0) {
    if ((MENU->mousedx_clock-=elapsed)<=0.0) {
      MENU->mousedx_clock=0.0;
      MENU->mousedx=0;
    }
  }
  if (MENU->mousedy_clock>0.0) {
    if ((MENU->mousedy_clock-=elapsed)<=0.0) {
      MENU->mousedy_clock=0.0;
      MENU->mousedy=0;
    }
  }
}

/* Render.
 */
 
static void _mouse_render(struct menu *menu) {
  if (menu->tile_renderer) {
    tile_renderer_begin(menu->tile_renderer,menu->tilesheet,0xffffffff,0xff);
    
    int y=40;
    char tmp[128];
    int tmpc;
    #define KVINT(k,_v) { \
      tmp[0]=k; \
      tmp[1]=':'; \
      tmp[2]=' '; \
      tmpc=3; \
      int v=(_v); \
      if (v<0) { \
        tmp[tmpc++]='-'; \
        v=-v; \
      } \
      if ((v>=10000)||(v<0)) { \
        tmp[tmpc++]='?'; \
      } else { \
        if (v>=1000) tmp[tmpc++]='0'+(v/1000)%10; \
        if (v>=100 ) tmp[tmpc++]='0'+(v/ 100)%10; \
        if (v>=10  ) tmp[tmpc++]='0'+(v/  10)%10; \
                     tmp[tmpc++]='0'+(v     )%10; \
      } \
      tile_renderer_string(menu->tile_renderer,40,y,tmp,tmpc); \
      y+=8; \
    }
    KVINT('X',MENU->mousex)
    KVINT('Y',MENU->mousey)
    KVINT('x',MENU->mousedx)
    KVINT('y',MENU->mousedy)
    #undef KVINT
    
    if (MENU->btnstate) {
      tmpc=0;
      const char *buttonnames[8]={"OTHER","Left","Right","Middle","4","5","6","7"};
      const char **name=buttonnames;
      int bit=1;
      for (;bit<=0x80;bit<<=1,name++) {
        if (MENU->btnstate&bit) {
          if (tmpc) tmp[tmpc++]=',';
          const char *src=*name;
          for (;*src;src++) tmp[tmpc++]=*src;
        }
      }
      tile_renderer_string(menu->tile_renderer,40,y,tmp,tmpc);
    }
    y+=8;
    
    tile_renderer_end(menu->tile_renderer);
  }
}

/* New.
 */
 
struct menu *menu_new_mouse(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu_mouse),parent);
  if (!menu) return 0;
  menu->del=_mouse_del;
  menu->event=_mouse_event;
  menu->update=_mouse_update;
  menu->render=_mouse_render;
  return menu;
}
