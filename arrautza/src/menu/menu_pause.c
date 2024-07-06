#include "arrautza.h"

#define PAUSE_DEPLOY_SPEED 3.0

struct menu_pause {
  struct menu hdr;
  double deployment; // 0..1, for the in and out transitions, how close are we to the target position?
  double deployspeed; // +- u/s when transition in progress.
  int pvinput;
  int animframe;
  double animclock;
  int selp; // (0..ITEM_COUNT-1)=inventory
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
 
static void pause_select_item(struct menu *menu,int btnid) {
  if (btnid==INKEEP_BTNID_SOUTH) {
    uint8_t tmp=g.aitem;
    g.aitem=g.inventory[MENU->selp];
    g.inventory[MENU->selp]=tmp;
  } else if (btnid==INKEEP_BTNID_WEST) {
    uint8_t tmp=g.bitem;
    g.bitem=g.inventory[MENU->selp];
    g.inventory[MENU->selp]=tmp;
  }
}
 
static void pause_activate(struct menu *menu,int btnid) {
  if (MENU->selp<0) ;
  else if (MENU->selp<ITEM_COUNT) pause_select_item(menu,btnid);
}

/* Motion.
 */
 
static void pause_motion(struct menu *menu,int dx,int dy) {
  if (MENU->selp<0) { // No selection => into inventory, regardless of input.
    MENU->selp=0;
  } else if (MENU->selp<ITEM_COUNT) { // Inventory.
    int col=MENU->selp&3;
    int row=MENU->selp>>2;
    col+=dx; if (col<0) col=3; else if (col>=4) col=0;
    row+=dy; if (row<0) row=3; else if (row>=4) row=0;
    MENU->selp=(row<<2)|col;
  }
  g.menu_pause_selection=MENU->selp;
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
    if (g.instate!=MENU->pvinput) {
      #define PRESS(tag) ((g.instate&INKEEP_BTNID_##tag)&&!(MENU->pvinput&INKEEP_BTNID_##tag))
      if (PRESS(SOUTH)) pause_activate(menu,INKEEP_BTNID_SOUTH);
      if (PRESS(WEST)) pause_activate(menu,INKEEP_BTNID_WEST);
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
 
static void pause_render_background(struct menu *menu,int x0,int y0,int colc,int rowc,int texid) {
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

extern void add_item_tiles(int x,int y,uint8_t itemid);

static void render_selected_item(int x,int y,uint8_t itemid,uint8_t tileid) {
  tile_renderer_tile(&g.tile_renderer,x,y,tileid,0);
  tile_renderer_tile(&g.tile_renderer,x+TILESIZE,y,tileid+1,0);
  tile_renderer_tile(&g.tile_renderer,x,y+TILESIZE,0x8f,EGG_XFORM_YREV);
  tile_renderer_tile(&g.tile_renderer,x+TILESIZE,y+TILESIZE,0x8f,EGG_XFORM_YREV|EGG_XFORM_XREV);
  add_item_tiles(x+(TILESIZE>>1),y+(TILESIZE>>1),itemid);
}

static void pause_render_inventory(struct menu *menu,int x0,int y0,int texid) {
  int x,y,xi,yi;
  tile_renderer_begin(&g.tile_renderer,texid,0,0xff);
  
  render_selected_item(x0+TILESIZE+(TILESIZE>>1),y0+TILESIZE,g.bitem,0x8b);
  render_selected_item(x0+TILESIZE*3+(TILESIZE>>1),y0+TILESIZE,g.aitem,0x8d);
  
  /* Background for inventory well.
   */
  int wellw=5,wellh=5;
  int wellx=x0+TILESIZE,welly=y0+TILESIZE*3;
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
  
  /* Inventory.
   */
  int item_stride=TILESIZE;
  int invix=0;
  for (y=welly+(TILESIZE>>1),yi=4;yi-->0;y+=item_stride) {
    for (x=wellx+(TILESIZE>>1),xi=4;xi-->0;x+=item_stride,invix++) {
      uint8_t itemid=g.inventory[invix];
      add_item_tiles(x,y,itemid);
      if (invix==MENU->selp) {
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

static void _pause_render(struct menu *menu) {

  /* Declare the overall size and position on screen.
   * We slide up from the bottom.
   */
  int colc=COLC-1;
  int rowc=ROWC;
  int x0=TILESIZE;
  int y0=SCREENH-(rowc-1)*TILESIZE;
  if (MENU->deployment<1.0) {
    y0+=(SCREENH-y0)*(1.0-MENU->deployment);
  }
  
  // Most of our graphics come from the hero tilesheet.
  int texid=texcache_get(&g.texcache,RID_image_hero);
  
  pause_render_background(menu,x0,y0,colc,rowc,texid);
  pause_render_inventory(menu,x0,y0,texid);
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
  MENU->selp=g.menu_pause_selection;
  
  return menu;
}
