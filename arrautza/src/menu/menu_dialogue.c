/* menu_dialogue.c
 * We present like menu_pause, but only occupy the lower portion of the screen and display static text.
 */

#include "arrautza.h"

#define DIALOGUE_DEPLOY_SPEED 3.0

struct menu_dialogue {
  struct menu hdr;
  double deployment; // 0..1, for the in and out transitions, how close are we to the target position?
  double deployspeed; // +- u/s when transition in progress.
  int pvinput;
  int texid_text;
  int textw,texth;
};

#define MENU ((struct menu_dialogue*)menu)

/* Cleanup.
 */
 
static void _dialogue_del(struct menu *menu) {
  egg_texture_del(MENU->texid_text);
}

/* Dismiss.
 */
 
static void dialogue_dismiss(struct menu *menu) {
  if (MENU->deployspeed>=0.0) {
    MENU->deployspeed=-DIALOGUE_DEPLOY_SPEED;
  }
}

/* Update.
 */
 
static void _dialogue_update(struct menu *menu,double elapsed) {
  if (MENU->deployspeed>0.0) {
    if ((MENU->deployment+=MENU->deployspeed*elapsed)>=1.0) {
      MENU->deployment=1.0;
      MENU->deployspeed=0.0;
    }
  } else if (MENU->deployspeed<0.0) {
    if ((MENU->deployment+=MENU->deployspeed*elapsed)<=0.0) {
      MENU->deployment=0.0;
      MENU->deployspeed=0.0;
      menu_pop(menu);
    }
  } else {
    if (g.instate!=MENU->pvinput) {
      #define PRESS(tag) ((g.instate&INKEEP_BTNID_##tag)&&!(MENU->pvinput&INKEEP_BTNID_##tag))
      if (PRESS(SOUTH)) dialogue_dismiss(menu);
      if (PRESS(WEST)) dialogue_dismiss(menu);
      if (PRESS(AUX1)) dialogue_dismiss(menu);
      #undef PRESS
      MENU->pvinput=g.instate;
    }
  }
}

/* Render.
 */
 
static void dialogue_render_background(struct menu *menu,int x0,int y0,int colc,int rowc,int texid) {
  int x,y,xi,yi;
  tile_renderer_begin(&g.tile_renderer,texid,0,0xff);
  tile_renderer_tile(&g.tile_renderer,x0,y0,0x82,0);
  tile_renderer_tile(&g.tile_renderer,x0+(colc-1)*TILESIZE,y0,0x82,EGG_XFORM_XREV);
  for (xi=colc-2,x=x0+TILESIZE;xi-->0;x+=TILESIZE) {
    uint8_t tileid=0x83;
    if (!(xi%5)) tileid=0x84; else if (!(xi%7)) tileid=0x85;
    tile_renderer_tile(&g.tile_renderer,x,y0,tileid,0);
  }
  for (yi=rowc-1,y=y0+TILESIZE;yi-->0;y+=TILESIZE) {
    uint8_t tileid=0x83;
    if (!(yi%5)) tileid=0x84; else if (!(yi%3)) tileid=0x85;
    tile_renderer_tile(&g.tile_renderer,x0,y,tileid,EGG_XFORM_SWAP);
    tile_renderer_tile(&g.tile_renderer,x0+(colc-1)*TILESIZE,y,tileid,EGG_XFORM_SWAP|EGG_XFORM_YREV);
  }
  for (yi=rowc-1,y=y0+TILESIZE;yi-->0;y+=TILESIZE) {
    for (xi=colc-2,x=x0+TILESIZE;xi-->0;x+=TILESIZE) {
      tile_renderer_tile(&g.tile_renderer,x,y,0x86,0);
    }
  }
  tile_renderer_end(&g.tile_renderer);
}

static void _dialogue_render(struct menu *menu) {

  /* Declare the overall size and position on screen.
   * We slide up from the bottom.
   */
  int colc=COLC-1;
  int rowc=ROWC>>1;
  int x0=TILESIZE;
  int y0=SCREENH-(rowc-1)*TILESIZE;
  if (MENU->deployment<1.0) {
    y0+=(SCREENH-y0)*(1.0-MENU->deployment);
  }
  
  // Most of our graphics come from the hero tilesheet.
  int texid=texcache_get(&g.texcache,RID_image_hero);
  
  dialogue_render_background(menu,x0,y0,colc,rowc,texid);
  egg_draw_decal(1,MENU->texid_text,x0+TILESIZE,y0+TILESIZE,0,0,MENU->textw,MENU->texth,0);
}

/* New.
 */
 
struct menu *menu_push_dialogue(int stringid) {
  struct menu *menu=menu_push(sizeof(struct menu_dialogue));
  if (!menu) return 0;
  
  menu->del=_dialogue_del;
  menu->update=_dialogue_update;
  menu->render=_dialogue_render;
  menu->id=MENU_ID_DIALOGUE;
  menu->opaque=0;
  
  MENU->deployment=0.0;
  MENU->deployspeed=DIALOGUE_DEPLOY_SPEED;
  
  const char *src=0;
  int srcc=text_get_string(&src,stringid);
  if (srcc>0) {
    MENU->texid_text=font_render_new_texture(g.font,src,srcc,(COLC-2)*TILESIZE,0x000000);
    egg_texture_get_header(&MENU->textw,&MENU->texth,0,MENU->texid_text);
  }
  
  return menu;
}
