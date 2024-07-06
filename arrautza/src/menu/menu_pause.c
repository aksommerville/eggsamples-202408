#include "arrautza.h"

#define PAUSE_DEPLOY_SPEED 3.0

struct menu_pause {
  struct menu hdr;
  double deployment; // 0..1, for the in and out transitions, how close are we to the target position?
  double deployspeed; // +- u/s when transition in progress.
  int pvinput;
  int animframe;
  double animclock;
  int selp;
};

#define MENU ((struct menu_pause*)menu)

/* Cleanup.
 */
 
static void _pause_del(struct menu *menu) {
}

/* Dismiss.
 */
 
static void pause_dismiss(struct menu *menu) {
  if (MENU->deployspeed>=0.0) {
    MENU->deployspeed=-PAUSE_DEPLOY_SPEED;
  }
}

/* Activate.
 */
 
static void pause_activate(struct menu *menu) {
}

/* Motion.
 */
 
static void pause_motion(struct menu *menu,int dx,int dy) {
  if (MENU->selp<0) { // No selection => into items, regardless of input.
    MENU->selp=0;
  } else if (MENU->selp<16) { // Items.
    int col=MENU->selp&3;
    int row=MENU->selp>>2;
    col+=dx; if (col<0) col=3; else if (col>=4) col=0;
    row+=dy; if (row<0) row=3; else if (row>=4) row=0;
    MENU->selp=(row<<2)|col;
  }
}

/* Update.
 */
 
static void _pause_update(struct menu *menu,double elapsed) {
  if ((MENU->animclock-=elapsed)<=0.0) {
    MENU->animclock+=0.125;
    if (++(MENU->animframe)>=4) MENU->animframe=0;
  }
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
    //TODO interactive
    if (g.instate!=MENU->pvinput) {
      #define PRESS(tag) ((g.instate&INKEEP_BTNID_##tag)&&!(MENU->pvinput&INKEEP_BTNID_##tag))
      if (PRESS(SOUTH)) pause_activate(menu);
      if (PRESS(AUX1)) pause_dismiss(menu);
      if (PRESS(LEFT)) pause_motion(menu,-1,0);
      if (PRESS(RIGHT)) pause_motion(menu,1,0);
      if (PRESS(UP)) pause_motion(menu,0,-1);
      if (PRESS(DOWN)) pause_motion(menu,0,1);
      #undef PRESS
      MENU->pvinput=g.instate;
    }
  }
}

/* Render.
 */
 
static void _pause_render(struct menu *menu) {

  /* Background is a sheet of paper that slides up from the bottom.
   */
  int colc=COLC-1;
  int rowc=ROWC;
  int x0=TILESIZE;
  int y0=SCREENH-(rowc-1)*TILESIZE;
  if (MENU->deployment<1.0) {
    y0+=(SCREENH-y0)*(1.0-MENU->deployment);
  }
  int x,y,xi,yi;
  int texid=texcache_get(&g.texcache,RID_image_hero);
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
  
  /* Background for inventory well.
   */
  int wellw=5,wellh=5;
  int wellx=x0+TILESIZE,welly=y0+TILESIZE;
  tile_renderer_begin(&g.tile_renderer,texid,0,0xff);
  tile_renderer_tile(&g.tile_renderer,wellx,welly,0x87,0);
  tile_renderer_tile(&g.tile_renderer,wellx+(wellw-1)*TILESIZE,welly,0x87,EGG_XFORM_XREV);
  tile_renderer_tile(&g.tile_renderer,wellx,welly+(wellh-1)*TILESIZE,0x87,EGG_XFORM_YREV);
  tile_renderer_tile(&g.tile_renderer,wellx+(wellw-1)*TILESIZE,welly+(wellh-1)*TILESIZE,0x87,EGG_XFORM_XREV|EGG_XFORM_YREV);
  for (x=wellx+TILESIZE,xi=wellw-2;xi-->0;x+=TILESIZE) {
    tile_renderer_tile(&g.tile_renderer,x,welly,0x88,0);
    tile_renderer_tile(&g.tile_renderer,x,welly+(wellh-1)*TILESIZE,0x88,EGG_XFORM_YREV);
  }
  for (y=welly+TILESIZE,yi=wellh-2;yi-->0;y+=TILESIZE) {
    tile_renderer_tile(&g.tile_renderer,wellx,y,0x88,EGG_XFORM_SWAP);
    tile_renderer_tile(&g.tile_renderer,wellx+(wellw-1)*TILESIZE,y,0x88,EGG_XFORM_SWAP|EGG_XFORM_YREV);
  }
  for (y=welly+TILESIZE,yi=wellh-2;yi-->0;y+=TILESIZE) {
    for (x=wellx+TILESIZE,xi=wellw-2;xi-->0;x+=TILESIZE) {
      tile_renderer_tile(&g.tile_renderer,x,y,0x89,0);
    }
  }
  tile_renderer_end(&g.tile_renderer);
  
  /* 4x4 inventory well in the upper left.
   */
  int item_stride=TILESIZE;
  int itemid=0;
  tile_renderer_begin(&g.tile_renderer,texid,0,0xff);
  for (y=welly+(TILESIZE>>1),yi=4;yi-->0;y+=item_stride) {
    for (x=wellx+(TILESIZE>>1),xi=4;xi-->0;x+=item_stride,itemid++) {
      tile_renderer_tile(&g.tile_renderer,x,y,0x90+itemid,0);
      if (itemid==MENU->selp) {
        uint8_t xform;
        switch (MENU->animframe) {
          case 0: xform=0; break;
          case 1: xform=EGG_XFORM_YREV; break;
          case 2: xform=EGG_XFORM_XREV|EGG_XFORM_YREV; break;
          case 3: xform=EGG_XFORM_XREV; break;
        }
        tile_renderer_tile(&g.tile_renderer,x,y,0x8a,xform);
      }
    }
  }
  tile_renderer_end(&g.tile_renderer);
}

/* New.
 */
 
struct menu *menu_push_pause() {
  struct menu *menu=menu_push(sizeof(struct menu_pause));
  if (!menu) return 0;
  
  menu->del=_pause_del;
  menu->update=_pause_update;
  menu->render=_pause_render;
  menu->id=MENU_ID_PAUSE;
  menu->opaque=0;
  
  MENU->deployment=0.0;
  MENU->deployspeed=PAUSE_DEPLOY_SPEED;
  
  return menu;
}
