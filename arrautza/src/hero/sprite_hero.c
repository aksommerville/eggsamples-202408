#include "hero_internal.h"

/* Cleanup.
 */
 
static void _hero_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {
  sprite->layer=100;
  sprite->imageid=RID_image_hero;
  sprite->tileid=0x00;
  sprite->xform=0;
  SPRITE->facedir=DIR_S;
  return 0;
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {
  if (g.instate!=SPRITE->pvinput) {
    #define PRESS(tag) ((g.instate&INKEEP_BTNID_##tag)&&!(SPRITE->pvinput&INKEEP_BTNID_##tag))
    #define RELEASE(tag) (!(g.instate&INKEEP_BTNID_##tag)&&(SPRITE->pvinput&INKEEP_BTNID_##tag))
    //TODO generalize this a bit, it's only going to get more complex:
    if (PRESS(LEFT)) { SPRITE->facedir=DIR_W; SPRITE->indx=-1; }
    else if (RELEASE(LEFT)&&(SPRITE->indx<0)) SPRITE->indx=0;
    if (PRESS(RIGHT)) { SPRITE->facedir=DIR_E; SPRITE->indx=1; }
    else if (RELEASE(RIGHT)&&(SPRITE->indx>0)) SPRITE->indx=0;
    if (PRESS(UP)) { SPRITE->facedir=DIR_N; SPRITE->indy=-1; }
    else if (RELEASE(UP)&&(SPRITE->indy<0)) SPRITE->indy=0;
    if (PRESS(DOWN)) { SPRITE->facedir=DIR_S; SPRITE->indy=1; }
    else if (RELEASE(DOWN)&&(SPRITE->indy>0)) SPRITE->indy=0;
    #undef PRESS
    #undef RELEASE
    SPRITE->pvinput=g.instate;
  }
  if (SPRITE->indx||SPRITE->indy) {
    //TODO Speed ramp up and down
    const double speed=6.0; // m/s
    sprite->x+=SPRITE->indx*elapsed*speed;
    sprite->y+=SPRITE->indy*elapsed*speed;
    //TODO Physics
  }
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite) {
  int x=sprite->bx+(sprite->bw>>1);
  int y=sprite->by+(sprite->bh>>1);
  int texid=texcache_get(&g.texcache,sprite->imageid);
  if (texid<1) return;
  tile_renderer_begin(&g.tile_renderer,texid,0,0xff);
  switch (SPRITE->facedir) {
  
    case DIR_N: {
        tile_renderer_tile(&g.tile_renderer,x,y-8,0x01,0);
        tile_renderer_tile(&g.tile_renderer,x,y,0x11,0);
      } break;
      
    case DIR_S: {
        tile_renderer_tile(&g.tile_renderer,x,y,0x10,0);
        tile_renderer_tile(&g.tile_renderer,x,y-8,0x00,0);
      } break;
      
    case DIR_W:
    case DIR_E: {
        uint8_t xform=(SPRITE->facedir==DIR_E)?EGG_XFORM_XREV:0;
        tile_renderer_tile(&g.tile_renderer,x,y,0x12,xform);
        tile_renderer_tile(&g.tile_renderer,x,y-8,0x02,xform);
      } break;
      
  }
  tile_renderer_end(&g.tile_renderer);
}

static void _hero_calculate_bounds(struct sprite *sprite,int tilesize,int addx,int addy) {
  int x=(int)(sprite->x*tilesize)+addx;
  int y=(int)(sprite->y*tilesize)+addy;
  sprite->bw=tilesize;
  sprite->bh=tilesize;
  sprite->bx=x-(sprite->bw>>1);
  sprite->by=y-(sprite->bh>>1);
}

/* Footing change.
 */
 
static void _hero_footing(struct sprite *sprite,int8_t pvcol,int8_t pvrow) {
  
  // If we're OOB, request transition to a neighbor map and do nothing more.
  if (sprite->col<0)     { load_neighbor(MAPCMD_neighborw); return; }
  if (sprite->col>=COLC) { load_neighbor(MAPCMD_neighbore); return; }
  if (sprite->row<0)     { load_neighbor(MAPCMD_neighborn); return; }
  if (sprite->row>=ROWC) { load_neighbor(MAPCMD_neighbors); return; }
  
  //TODO doors
  //TODO treadles, etc
}

/* Type definition.
 */
 
const struct sprctl sprctl_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .grpmask=(
    (1<<SPRGRP_UPDATE)|
    (1<<SPRGRP_RENDER)|
    (1<<SPRGRP_HERO)|
    (1<<SPRGRP_FOOTING)|
  0),
  .del=_hero_del,
  .init=_hero_init,
  // No need for (ready), since we are never created from a sprdef.
  .update=_hero_update,
  .render=_hero_render,
  .calculate_bounds=_hero_calculate_bounds,
  .footing=_hero_footing,
};
