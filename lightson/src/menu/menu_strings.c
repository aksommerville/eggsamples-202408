#include <egg/egg.h>
#include "menu.h"
#include "util/tile_renderer.h"
#include "util/font.h"
#include "stdlib/egg-stdlib.h"

/* Object definition.
 */
 
struct menu_strings {
  struct menu hdr;
  int *langv;
  int langc,langa;
  int langp;
  int dirty; // refresh textures before next render
  int texid;
};

#define MENU ((struct menu_strings*)menu)

/* Cleanup.
 */
 
static void _strings_del(struct menu *menu) {
  if (MENU->langv) free(MENU->langv);
  egg_texture_del(MENU->texid);
}

/* Motion.
 */
 
static void strings_motion(struct menu *menu,int d) {
  if (MENU->langc<1) return;
  MENU->langp+=d;
  if (MENU->langp<0) MENU->langp=MENU->langc-1;
  else if (MENU->langp>=MENU->langc) MENU->langp=0;
  MENU->dirty=1;
}

/* Event.
 */
 
static void _strings_event(struct menu *menu,const union egg_event *event) {
  switch (event->type) {
    case EGG_EVENT_JOY: if (event->joy.value) switch (event->joy.btnid) {
        case EGG_JOYBTN_LEFT: strings_motion(menu,-1); break;
        case EGG_JOYBTN_RIGHT: strings_motion(menu,1); break;
      } break;
    case EGG_EVENT_KEY: if (event->key.value) switch (event->key.keycode) {
        case 0x0007004f: strings_motion(menu,1); break;
        case 0x00070050: strings_motion(menu,-1); break;
      } break;
  }
}

/* Update.
 */
 
static void _strings_update(struct menu *menu,double elapsed) {
}

/* Redraw textures.
 */
 
struct strings_cb_draw_context {
  struct menu *menu;
  void *pixels;
  int y;
};

static int strings_cb_draw(int tid,int qual,int rid,int len,void *userdata) {
  struct strings_cb_draw_context *ctx=userdata;
  struct menu *menu=ctx->menu;
  if (tid<EGG_RESTYPE_string) return 0;
  if (tid>EGG_RESTYPE_string) return 1;
  if (qual<MENU->langv[MENU->langp]) return 0;
  if (qual>MENU->langv[MENU->langp]) return 1;
  
  char serial[256];
  if (len>sizeof(serial)) return 0;
  if (egg_res_get(serial,len,tid,qual,rid)!=len) return 0;
  font_render_string_rgba(ctx->pixels,menu->screenw,menu->screenh,menu->screenw<<2,4,ctx->y,menu->font,serial,len,0xffffff);
  ctx->y+=font_get_rowh(menu->font);
  
  return 0;
}
 
static void strings_redraw(struct menu *menu) {
  MENU->dirty=0;
  if ((MENU->langp<0)||(MENU->langp>=MENU->langc)) return;
  void *pixels=calloc(menu->screenw<<2,menu->screenh);
  if (!pixels) return;
  char desc[2]={
    "012345abcdefghijklmnopqrstuvwxyz"[(MENU->langv[MENU->langp]>>5)&0x1f],
    "012345abcdefghijklmnopqrstuvwxyz"[(MENU->langv[MENU->langp]   )&0x1f],
  };
  font_render_string_rgba(pixels,menu->screenw,menu->screenh,menu->screenw<<2,10,1,menu->font,desc,2,0xffff00);
  font_render_string_rgba(pixels,menu->screenw,menu->screenh,menu->screenw<<2,30,1,menu->font,"L/R to change.",-1,0xc0c0c0);
  struct strings_cb_draw_context ctx={
    .menu=menu,
    .pixels=pixels,
    .y=font_get_rowh(menu->font)+1,
  };
  egg_res_for_each(strings_cb_draw,&ctx);
  if (!MENU->texid) MENU->texid=egg_texture_new();
  egg_texture_upload(MENU->texid,menu->screenw,menu->screenh,menu->screenw<<2,EGG_TEX_FMT_RGBA,pixels,menu->screenw*menu->screenh*4);
  free(pixels);
}

/* Render.
 */
 
static void _strings_render(struct menu *menu) {
  if (MENU->dirty) strings_redraw(menu);
  egg_draw_decal(1,MENU->texid,0,0,0,0,menu->screenw,menu->screenh,0);
}

/* Acquire list of languages.
 */
 
static int strings_cb_res(int tid,int qual,int rid,int len,void *userdata) {
  struct menu *menu=userdata;
  if (tid<EGG_RESTYPE_string) return 0;
  if (tid>EGG_RESTYPE_string) return 1;
  if (!qual) return 0;
  // Resources are stored sorted and we are allowed to depend on that.
  // So we only need to check the last thing stored.
  if (!MENU->langc||(qual!=MENU->langv[MENU->langc-1])) {
    if (MENU->langc>=MENU->langa) {
      int na=MENU->langa+8;
      if (na>INT_MAX/sizeof(int)) return -1;
      void *nv=realloc(MENU->langv,sizeof(int)*na);
      if (!nv) return -1;
      MENU->langv=nv;
      MENU->langa=na;
    }
    MENU->langv[MENU->langc++]=qual;
  }
  return 0;
}

/* New.
 */
 
struct menu *menu_new_strings(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu_strings),parent);
  if (!menu) return 0;
  menu->del=_strings_del;
  menu->event=_strings_event;
  menu->update=_strings_update;
  menu->render=_strings_render;
  MENU->dirty=1;
  egg_res_for_each(strings_cb_res,menu);
  return menu;
}
