#include <egg/egg.h>
#include "menu.h"
#include "util/font.h"
#include "util/tile_renderer.h"
#include "stdlib/egg-stdlib.h"

extern int texid_misc;

/* Object definition.
 */
 
struct menu_framebuffer {
  struct menu hdr;
  int texid;
  double monkeyclock;
};

#define MENU ((struct menu_framebuffer*)menu)

/* Cleanup.
 */
 
static void _framebuffer_del(struct menu *menu) {
  egg_texture_del(MENU->texid);
}

/* Update.
 */
 
static void _framebuffer_update(struct menu *menu,double elapsed) {
  if ((MENU->monkeyclock+=elapsed)>=20.0) MENU->monkeyclock-=20.0;
}

/* Draw the scene. We'll do this both into an intermediate framebuffer, and onto the main.
 * Should be 100x50 pixels.
 */
 
static void framebuffer_draw_scene(struct menu *menu,int dsttexid,int x0,int y0) {
  {
    struct egg_draw_line vtxv[]={
      {x0+0,y0+0,0x80,0x00,0x00,0xff},
      {x0+0,y0+50,0x00,0x80,0x00,0xff},
      {x0+100,y0+0,0x00,0x00,0xff,0xff},
      {x0+100,y0+50,0x00,0x00,0x00,0xff},
    };
    egg_draw_trig(dsttexid,vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  }
  
  int pvdst=menu->tile_renderer->dsttexid;
  menu->tile_renderer->dsttexid=dsttexid;
  tile_renderer_begin(menu->tile_renderer,menu->tilesheet,0xffffffff,0x40);
  tile_renderer_string(menu->tile_renderer,x0+8,y0+8,"Identical?",-1);
  tile_renderer_end(menu->tile_renderer);
  menu->tile_renderer->dsttexid=pvdst;
  
  {
    struct egg_draw_line vtxv[]={
      {x0+ 5,y0+45,0xff,0xff,0xff,0x80},
      {x0+25,y0+40,0xff,0xff,0xff,0x80},
      {x0+45,y0+45,0xff,0xff,0xff,0x80},
      {x0+65,y0+40,0xff,0xff,0xff,0x80},
      {x0+85,y0+45,0xff,0xff,0xff,0x80},
    };
    egg_draw_line(dsttexid,vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  }
  
  egg_draw_decal(dsttexid,texid_misc,x0+42,y0+17,64,16,16,16,0);
}

/* Render.
 */
 
static void _framebuffer_render(struct menu *menu) {

  // Just to mess with their heads, every 20 seconds, spin the framebuffer'd one around.
  if (MENU->monkeyclock>=19.0) {
    double t=(MENU->monkeyclock-19.0)*M_PI*2.0;
    int ts1616=(int)(t*65536.0);
    egg_draw_decal_mode7(1,MENU->texid,100,90,0,0,100,50,ts1616,0x00010000,0x00010000);
  } else {
    egg_draw_decal(1,MENU->texid,50,65,0,0,100,50,0);
  }
  
  framebuffer_draw_scene(menu,1,170,65);
}

/* New.
 */
 
struct menu *menu_new_framebuffer(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu_framebuffer),parent);
  if (!menu) return 0;
  menu->del=_framebuffer_del;
  menu->update=_framebuffer_update;
  menu->render=_framebuffer_render;
  
  MENU->texid=egg_texture_new();
  if (egg_texture_upload(MENU->texid,150,50,0,EGG_TEX_FMT_RGBA,0,0)<0) {
    egg_log("Initialize texture failed!");
  }
  framebuffer_draw_scene(menu,MENU->texid,0,0);
  
  return menu;
}
