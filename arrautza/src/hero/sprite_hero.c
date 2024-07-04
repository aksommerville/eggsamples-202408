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
  sprite_set_hitbox(sprite,0.500,0.625,0.0,0.0);
  sprite->invmass=128;
  sprite->mapsolids=(
    (1<<MAP_PHYSICS_SOLID)|
    (1<<MAP_PHYSICS_WATER)|
    (1<<MAP_PHYSICS_HOLE)|
  0);
  return 0;
}

/* Motion.
 */
 
static void hero_begin_motion(struct sprite *sprite,int dx,int dy) {
  if (!SPRITE->indx&&!SPRITE->indy) {
    SPRITE->animframe=0;
    SPRITE->animclock=0.200;
  }
  if (dx) {
    SPRITE->indx=dx;
    if (dx<0) {
      SPRITE->facedir=DIR_W;
    } else {
      SPRITE->facedir=DIR_E;
    }
  } else if (dy) {
    SPRITE->indy=dy;
    if (dy<0) {
      SPRITE->facedir=DIR_N;
    } else {
      SPRITE->facedir=DIR_S;
    }
  }
}

static void hero_end_motion(struct sprite *sprite,int dx,int dy) {
  if (dx) {
    SPRITE->indx=0;
    if (SPRITE->indy<0) {
      SPRITE->facedir=DIR_N;
    } else if (SPRITE->indy>0) {
      SPRITE->facedir=DIR_S;
    }
  } else if (dy) {
    SPRITE->indy=0;
    if (SPRITE->indx<0) {
      SPRITE->facedir=DIR_W;
    } else if (SPRITE->indx>0) {
      SPRITE->facedir=DIR_E;
    }
  }
  SPRITE->animframe=0;
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {
  if (g.instate!=SPRITE->pvinput) {
    #define PRESS(tag) ((g.instate&INKEEP_BTNID_##tag)&&!(SPRITE->pvinput&INKEEP_BTNID_##tag))
    #define RELEASE(tag) (!(g.instate&INKEEP_BTNID_##tag)&&(SPRITE->pvinput&INKEEP_BTNID_##tag))
    if (PRESS(LEFT)) hero_begin_motion(sprite,-1,0); else if (RELEASE(LEFT)&&(SPRITE->indx<0)) hero_end_motion(sprite,-1,0);
    if (PRESS(RIGHT)) hero_begin_motion(sprite,1,0); else if (RELEASE(RIGHT)&&(SPRITE->indx>0)) hero_end_motion(sprite,1,0);
    if (PRESS(UP)) hero_begin_motion(sprite,0,-1); else if (RELEASE(UP)&&(SPRITE->indy<0)) hero_end_motion(sprite,0,-1);
    if (PRESS(DOWN)) hero_begin_motion(sprite,0,1); else if (RELEASE(DOWN)&&(SPRITE->indy>0)) hero_end_motion(sprite,0,1);
    #undef PRESS
    #undef RELEASE
    SPRITE->pvinput=g.instate;
  }
  if (SPRITE->indx||SPRITE->indy) {
    const double speed=5.0; // m/s
    sprite->x+=SPRITE->indx*elapsed*speed;
    sprite->y+=SPRITE->indy*elapsed*speed;
    if ((SPRITE->animclock-=elapsed)<=0.0) {
      SPRITE->animclock+=0.150;
      if (++(SPRITE->animframe)>=4) {
        SPRITE->animframe=0;
      }
    }
  } else {
    SPRITE->animframe=1;
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
        uint8_t bodytileid=0x11;
        if (SPRITE->indx||SPRITE->indy) switch (SPRITE->animframe) {
          case 0: bodytileid=0x21; break;
          case 2: bodytileid=0x31; break;
        }
        tile_renderer_tile(&g.tile_renderer,x,y-((SPRITE->animframe>=2)?7:8),0x01,0);
        tile_renderer_tile(&g.tile_renderer,x,y,bodytileid,0);
      } break;
      
    case DIR_S: {
        uint8_t bodytileid=0x10;
        if (SPRITE->indx||SPRITE->indy) switch (SPRITE->animframe) {
          case 0: bodytileid=0x20; break;
          case 2: bodytileid=0x30; break;
        }
        tile_renderer_tile(&g.tile_renderer,x,y,bodytileid,0);
        tile_renderer_tile(&g.tile_renderer,x,y-((SPRITE->animframe>=2)?7:8),0x00,0);
      } break;
      
    case DIR_W:
    case DIR_E: {
        uint8_t bodytileid=0x12;
        if (SPRITE->indx||SPRITE->indy) switch (SPRITE->animframe) {
          case 0: bodytileid=0x22; break;
          case 2: bodytileid=0x32; break;
        }
        uint8_t xform=(SPRITE->facedir==DIR_E)?EGG_XFORM_XREV:0;
        tile_renderer_tile(&g.tile_renderer,x,y,bodytileid,xform);
        tile_renderer_tile(&g.tile_renderer,x,y-((SPRITE->animframe>=2)?7:8),0x02,xform);
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

/* Collision.
 */
 
static void _hero_collision(struct sprite *sprite,struct sprite *other,uint8_t dir) {
  //egg_log("%s %p 0x%x %s",__func__,other,dir,other?"nonzero":"zero");
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
    (1<<SPRGRP_SOLID)|
  0),
  .del=_hero_del,
  .init=_hero_init,
  // No need for (ready), since we are never created from a sprdef.
  .update=_hero_update,
  .render=_hero_render,
  .calculate_bounds=_hero_calculate_bounds,
  .footing=_hero_footing,
  .collision=_hero_collision,
};
