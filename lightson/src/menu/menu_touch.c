#include <egg/egg.h>
#include "menu.h"
#include "stdlib/egg-stdlib.h"
#include "util/tile_renderer.h"
#include "util/font.h"

// After letting go, each line fades out over so long.
#define FADE_OUT_TIME 2.0

/* Object definition.
 */
 
struct touchline {
  int touchid; // 0 if dead
  double deadtime; // Counts up once dead.
  struct egg_draw_tile *tilev;
  int tilec,tilea;
};
 
struct menu_touch {
  struct menu hdr;
  int tilesheet;
  struct touchline *touchlinev;
  int touchlinec,touchlinea;
  int disabled;
};

#define MENU ((struct menu_touch*)menu)

/* Cleanup.
 */
 
static void touchline_cleanup(struct touchline *touchline) {
  if (touchline->tilev) free(touchline->tilev);
}
 
static void _touch_del(struct menu *menu) {
  egg_texture_del(MENU->tilesheet);
  if (MENU->touchlinev) {
    while (MENU->touchlinec-->0) touchline_cleanup(MENU->touchlinev+MENU->touchlinec);
    free(MENU->touchlinev);
  }
}

/* tile list in touchline.
 */
 
static struct egg_draw_tile *touchline_add_tile(struct touchline *touchline) {
  if (touchline->tilec>=touchline->tilea) {
    int na=touchline->tilea+16;
    if (na>INT_MAX/sizeof(struct egg_draw_tile)) return 0;
    void *nv=realloc(touchline->tilev,sizeof(struct egg_draw_tile)*na);
    if (!nv) return 0;
    touchline->tilev=nv;
    touchline->tilea=na;
  }
  return touchline->tilev+touchline->tilec++;
}

/* touchline list.
 */
 
static int touch_touchlinev_search(const struct menu *menu,int touchid) {
  int lo=0,hi=MENU->touchlinec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    int q=MENU->touchlinev[ck].touchid;
         if (touchid<q) hi=ck;
    else if (touchid>q) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static struct touchline *touch_get_line(const struct menu *menu,int touchid) {
  int p=touch_touchlinev_search(menu,touchid);
  if (p<0) return 0;
  return MENU->touchlinev+p;
}

static struct touchline *touch_add_line(struct menu *menu,int touchid) {
  int p=touch_touchlinev_search(menu,touchid);
  if (p>=0) {
    egg_log("!!! touchid %d already exists",touchid);
    return 0;
  }
  p=-p-1;
  if (MENU->touchlinec>=MENU->touchlinea) {
    int na=MENU->touchlinea+8;
    if (na>INT_MAX/sizeof(struct touchline)) return 0;
    void *nv=realloc(MENU->touchlinev,sizeof(struct touchline)*na);
    if (!nv) return 0;
    MENU->touchlinev=nv;
    MENU->touchlinea=na;
  }
  struct touchline *touchline=MENU->touchlinev+p;
  memmove(touchline+1,touchline,sizeof(struct touchline)*(MENU->touchlinec-p));
  MENU->touchlinec++;
  memset(touchline,0,sizeof(struct touchline));
  touchline->touchid=touchid;
  return touchline;
}

/* Finish one stroke.
 */
 
static void touch_finish(struct menu *menu,int touchid,int x,int y) {
  struct touchline *touchline=touch_get_line(menu,touchid);
  if (!touchline) {
    egg_log("!!! Unexpected touch finish event for touchid %d",touchid);
    return;
  }
  touchline->touchid=0;
  struct egg_draw_tile *tile=touchline_add_tile(touchline);
  if (tile) {
    tile->x=x;
    tile->y=y;
    tile->tileid=0x00;
    tile->xform=0;
  }
}

static void touch_add(struct menu *menu,int touchid,int x,int y) {
  if (touchid<1) {
    egg_log("!!! Invalid touch start event for touchid %d",touchid);
    return;
  }
  struct touchline *touchline=touch_add_line(menu,touchid);
  if (!touchline) {
    egg_log("!!! Failed to add touch %d. Expect further errors.",touchid);
    return;
  }
  struct egg_draw_tile *tile=touchline_add_tile(touchline);
  if (tile) {
    tile->x=x;
    tile->y=y;
    tile->tileid=0x01;
    tile->xform=0;
  }
}

static void touch_move(struct menu *menu,int touchid,int x,int y) {
  struct touchline *touchline=touch_get_line(menu,touchid);
  if (!touchline) {
    egg_log("!!! Unexpected touch move event for touchid %d",touchid);
    return;
  }
  struct egg_draw_tile *tile=touchline_add_tile(touchline);
  if (tile) {
    tile->x=x;
    tile->y=y;
    tile->tileid=0x02;
    tile->xform=0;
  }
}

/* Event.
 */
 
static void _touch_event(struct menu *menu,const union egg_event *event) {
  switch (event->type) {
    case EGG_EVENT_TOUCH: {
        switch (event->touch.state) {
          case 0: touch_finish(menu,event->touch.touchid,event->touch.x,event->touch.y); break;
          case 1: touch_add(menu,event->touch.touchid,event->touch.x,event->touch.y); break;
          case 2: touch_move(menu,event->touch.touchid,event->touch.x,event->touch.y); break;
          default: egg_log("!!! Unexpected state %d for event on touchid %d",event->touch.state,event->touch.touchid);
        }
      } break;
      
    #if 0 //XXX Fake a touch with the mouse, so i can validate. Leaving this here because we'll probably need again at some point.
    case EGG_EVENT_MMOTION: if (XXX_faketouchid) touch_move(menu,XXX_faketouchid,event->mmotion.x,event->mmotion.y); break;
    case EGG_EVENT_MBUTTON: if (event->mbutton.value) {
        if (!XXX_faketouchid) {
          XXX_faketouchid=1;
          touch_add(menu,XXX_faketouchid,event->mbutton.x,event->mbutton.y);
        }
      } else {
        if (XXX_faketouchid) {
          touch_finish(menu,XXX_faketouchid,event->mbutton.x,event->mbutton.y);
          XXX_faketouchid=0;
        }
      } break;
    #endif
  }
}

/* Update.
 */
 
static void _touch_update(struct menu *menu,double elapsed) {
  int i=MENU->touchlinec;
  struct touchline *touchline=MENU->touchlinev+i-1;
  for (;i-->0;touchline--) {
    if (touchline->touchid) continue;
    if ((touchline->deadtime+=elapsed)<FADE_OUT_TIME) continue;
    MENU->touchlinec--;
    memmove(touchline,touchline+1,sizeof(struct touchline)*(MENU->touchlinec-i));
  }
}

/* Render.
 */
 
static void _touch_render(struct menu *menu) {
  if (MENU->disabled) {
    tile_renderer_begin(menu->tile_renderer,MENU->tilesheet,0xff8080ff,0xff);
    tile_renderer_string(menu->tile_renderer,20,menu->screenh>>1,"EGG_EVENT_TOUCH failed to enable",-1);
    tile_renderer_end(menu->tile_renderer);
  }
  struct touchline *touchline=MENU->touchlinev;
  int i=MENU->touchlinec;
  for (;i-->0;touchline++) {
    if (!touchline->touchid) egg_render_alpha(0xff-(int)(touchline->deadtime*255.0)/FADE_OUT_TIME);
    egg_draw_tile(1,MENU->tilesheet,touchline->tilev,touchline->tilec);
    if (!touchline->touchid) egg_render_alpha(0xff);
  }
}

/* New.
 */
 
struct menu *menu_new_touch(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu_touch),parent);
  if (!menu) return 0;
  menu->del=_touch_del;
  menu->event=_touch_event;
  menu->update=_touch_update;
  menu->render=_touch_render;
  
  if (egg_texture_load_image(MENU->tilesheet=egg_texture_new(),0,8)<0) {
    egg_log("!!! Failed to load image:0:8");
  }
  
  // TOUCH is enabled by default. We're doing it again in order to check the response.
  if (!egg_event_enable(EGG_EVENT_TOUCH,1)) {
    MENU->disabled=1;
  }
  
  return menu;
}
