#include <egg/egg.h>
#include "menu.h"
#include "util/font.h"
#include "util/tile_renderer.h"
#include "stdlib/egg-stdlib.h"

extern int texid_misc;

/* Object definition.
 */
 
struct menu_primitives {
  struct menu hdr;
  int cursorp;
  int tint;
  int alpha;
  double rotation;
};

#define MENU ((struct menu_primitives*)menu)

/* Motion.
 */
 
#define next_in_list(prev,d,...) ({ \
  const int vv[]={__VA_ARGS__}; \
  int c=sizeof(vv)/sizeof(int); \
  int p=-1,i=c; \
  while (i-->0) { \
    if (vv[i]==prev) { p=i; break; } \
  } \
  p+=d; \
  if (p<0) p=c-1; \
  else if (p>=c) p=0; \
  vv[p]; \
})
 
static void primitives_motion(struct menu *menu,int dx,int dy) {
  if (dy) {
    MENU->cursorp+=dy;
    if (MENU->cursorp<0) MENU->cursorp=1;
    else if (MENU->cursorp>=2) MENU->cursorp=0;
  }
  if (dx) switch (MENU->cursorp) {
    case 0: MENU->tint=next_in_list(MENU->tint,dx,0x00000000,0x00000080,0x000000ff,0xff000080,0xff0000ff,0x0000ff80,0x0000ffff,0xffffff80,0xffffffff); break;
    case 1: MENU->alpha=next_in_list(MENU->alpha,dx,0x00,0x40,0x80,0xc0,0xff); break;
  }
}

/* Event.
 */
 
static void _primitives_event(struct menu *menu,const union egg_event *event) {
  switch (event->type) {
    case EGG_EVENT_JOY: if (event->joy.value) switch (event->joy.btnid) {
        case EGG_JOYBTN_LEFT: primitives_motion(menu,-1,0); break;
        case EGG_JOYBTN_RIGHT: primitives_motion(menu,1,0); break;
        case EGG_JOYBTN_UP: primitives_motion(menu,0,-1); break;
        case EGG_JOYBTN_DOWN: primitives_motion(menu,0,1); break;
      } break;
    case EGG_EVENT_KEY: if (event->key.value) switch (event->key.keycode) {
        case 0x0007004f: primitives_motion(menu,1,0); break;
        case 0x00070050: primitives_motion(menu,-1,0); break;
        case 0x00070051: primitives_motion(menu,0,1); break;
        case 0x00070052: primitives_motion(menu,0,-1); break;
        default: egg_log("%x",event->key.keycode);
      } break;
  }
}

/* Update.
 */
 
static void _primitives_update(struct menu *menu,double elapsed) {
  MENU->rotation+=elapsed*2.0; // radians/sec
  if (MENU->rotation>=M_PI) MENU->rotation-=M_PI*2.0;
}

/* Render.
 */
 
static void _primitives_render(struct menu *menu) {
  egg_render_tint(MENU->tint);
  egg_render_alpha(MENU->alpha);

  { // rect
    egg_draw_rect(1,10,10,20,10,0xff0000ff);
    egg_draw_rect(1,20,15,20,20,0x00ff0080);
  }
    
  { // line
    struct egg_draw_line vtxv[]={
      {50,10,0xff,0xff,0xff,0xff},
      {90,10,0xff,0xff,0xff,0xff},
      {70,20,0x00,0x00,0x00,0xff},
      {90,30,0xff,0x00,0x00,0xff},
      {70,40,0xff,0x00,0x00,0x00},
    };
    egg_draw_line(1,vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  }
  
  { // trig
    struct egg_draw_line vtxv[]={
      {100,10,0xff,0xff,0xff,0xff},
      {200,10,0xff,0x00,0x00,0xff},
      {120,40,0x00,0xff,0x00,0xff},
      {200,40,0x00,0x00,0xff,0x00},
    };
    egg_draw_trig(1,vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  }
  
  { // decal
    egg_draw_decal(1,texid_misc,10,50,64,16,16,16,0);
  }
  
  { // mode7 with identity params, must match decal.
    egg_draw_decal_mode7(1,texid_misc,38,58,64,16,16,16,0,0x10000,0x10000);
  }
    
  { // mode7
    egg_draw_decal_mode7(1,texid_misc,68,68,64,16,16,16,MENU->rotation*65536.0,0x00020000,0x00020000);
  }
  
  { // tile
    struct egg_draw_tile vtx={100,60,0x14,0};
    egg_draw_tile(1,texid_misc,&vtx,1);
  }
  
  egg_render_tint(0);
  egg_render_alpha(0xff);

  // Interactive portion: Two lines of text at the lower left.
  char tmp[8]={
    "0123456789abcdef"[(MENU->tint>>28)&0xf],
    "0123456789abcdef"[(MENU->tint>>24)&0xf],
    "0123456789abcdef"[(MENU->tint>>20)&0xf],
    "0123456789abcdef"[(MENU->tint>>16)&0xf],
    "0123456789abcdef"[(MENU->tint>>12)&0xf],
    "0123456789abcdef"[(MENU->tint>> 8)&0xf],
    "0123456789abcdef"[(MENU->tint>> 4)&0xf],
    "0123456789abcdef"[(MENU->tint    )&0xf],
  };
  tile_renderer_begin(menu->tile_renderer,menu->tilesheet,(MENU->cursorp==0)?0xffff00ff:0xc0c0c0ff,0xff);
  tile_renderer_string(menu->tile_renderer,8,menu->screenh-16,"Tint:",5);
  tile_renderer_string(menu->tile_renderer,8+6*8,menu->screenh-16,tmp,8);
  tile_renderer_end(menu->tile_renderer);
  tmp[0]="0123456789abcdef"[(MENU->alpha>>4)&0xf];
  tmp[1]="0123456789abcdef"[MENU->alpha&0xf],
  tile_renderer_begin(menu->tile_renderer,menu->tilesheet,(MENU->cursorp==1)?0xffff00ff:0xc0c0c0ff,0xff);
  tile_renderer_string(menu->tile_renderer,8,menu->screenh-8,"Alpha:",6);
  tile_renderer_string(menu->tile_renderer,8+7*8,menu->screenh-8,tmp,2);
  tile_renderer_end(menu->tile_renderer);
}

/* New.
 */
 
struct menu *menu_new_primitives(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu_primitives),parent);
  if (!menu) return 0;
  menu->event=_primitives_event;
  menu->update=_primitives_update;
  menu->render=_primitives_render;
  MENU->tint=0x00000000;
  MENU->alpha=0xff;
  return menu;
}
