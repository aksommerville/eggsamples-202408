#include "hero_internal.h"

/* Typical two-tile render.
 */
 
static void hero_render_twotile(
  struct sprite *sprite,
  uint8_t bodytileid,uint8_t bodyxform,
  uint8_t headtileid,uint8_t headxform,
  int headfirst
) {
  int x=sprite->bx+(sprite->bw>>1);
  int y=sprite->by+(sprite->bh>>1);
  // Animate body if the dpad is active.
  if (SPRITE->indx||SPRITE->indy) switch (SPRITE->animframe) {
    case 0: bodytileid+=0x10; break;
    case 2: bodytileid+=0x20; break;
  }
  // If pushing, body tile is 3 columns to the right.
  if (SPRITE->pushing) bodytileid+=0x03;
  // Head bobbles up and down while walking.
  int heady=y;
  if (!SPRITE->pushing&&(SPRITE->animframe>=2)) heady-=7;
  else heady-=8;
  // Head first when facing north, otherwise body first.
  if (headfirst) {
    tile_renderer_tile(&g.tile_renderer,x,heady,headtileid,headxform);
    tile_renderer_tile(&g.tile_renderer,x,y,bodytileid,bodyxform);
  } else {
    tile_renderer_tile(&g.tile_renderer,x,y,bodytileid,bodyxform);
    tile_renderer_tile(&g.tile_renderer,x,heady,headtileid,headxform);
  }
}

/* Swinging sword.
 */
 
static void hero_render_sword(struct sprite *sprite) {
  int x=sprite->bx+(sprite->bw>>1);
  int y=sprite->by+(sprite->bh>>1);
  int heady=y-8;
  // Pick head and body tiles. No animation.
  uint8_t headtileid,bodytileid,xform=0,swordxform;
  int swordx=x,swordy=y;
  switch (SPRITE->facedir) {
    case DIR_N: swordy-=16; headtileid=0x11; bodytileid=0x21; swordxform=EGG_XFORM_YREV|EGG_XFORM_XREV; break;
    case DIR_S: swordy+=10; headtileid=0x10; bodytileid=0x20; swordxform=0; break;
    case DIR_W: swordx-=12; swordy-=2; headtileid=0x12; bodytileid=0x22; swordxform=EGG_XFORM_SWAP|EGG_XFORM_YREV; break;
    case DIR_E: swordx+=12; swordy-=2; headtileid=0x12; bodytileid=0x22; xform=EGG_XFORM_XREV; swordxform=EGG_XFORM_SWAP|EGG_XFORM_XREV; break;
  }
  tile_renderer_tile(&g.tile_renderer,swordx,swordy,0x30,swordxform);
  if (SPRITE->facedir==DIR_N) {
    tile_renderer_tile(&g.tile_renderer,x,heady,headtileid,xform);
    tile_renderer_tile(&g.tile_renderer,x,y,bodytileid,xform);
  } else {
    tile_renderer_tile(&g.tile_renderer,x,y,bodytileid,xform);
    tile_renderer_tile(&g.tile_renderer,x,heady,headtileid,xform);
  }
  // 4-frame swash to create an illusion of having been swung from the right.
  int swashframe=(int)((SPRITE->swordtime*4.0)/0.120);
  if (swashframe<4) {
    int swashx=swordx,swashy=swordy;
    switch (SPRITE->facedir) {
      case DIR_N: swashx+=10; swashy+=4; break;
      case DIR_S: swashx-=10; swashy-=4; break;
      case DIR_W: swashy-=10; swashx+=4; break;
      case DIR_E: swashy+=10; swashx-=4; break;
    }
    tile_renderer_tile(&g.tile_renderer,swashx,swashy,0x31+swashframe,swordxform);
  }
}

/* Swashing buckler.
 */
 
static void hero_render_shield(struct sprite *sprite) {
  int x=sprite->bx+(sprite->bw>>1);
  int y=sprite->by+(sprite->bh>>1);
  int heady=y-8;
  // Pick head and body tiles. No animation.
  uint8_t headtileid,bodytileid,xform=0;
  switch (SPRITE->facedir) {
    case DIR_N: headtileid=0x01; bodytileid=0x41; y-=3; break;
    case DIR_S: headtileid=0x00; bodytileid=0x40; break;
    case DIR_W: headtileid=0x02; bodytileid=0x42; break;
    case DIR_E: headtileid=0x02; bodytileid=0x42; xform=EGG_XFORM_XREV; break;
  }
  tile_renderer_tile(&g.tile_renderer,x,y,bodytileid,xform);
  tile_renderer_tile(&g.tile_renderer,x,heady,headtileid,xform);
}

/* Render.
 */
 
void hero_render(int dsttexid,struct sprite *sprite) {
  int texid=texcache_get(&g.texcache,sprite->imageid);
  if (texid<1) return;
  
  uint8_t alpha=0xff;
  uint32_t tint=0;
  if (hero_item_in_use(sprite,ITEM_CLOAK)) alpha=0x40;
  if (SPRITE->hurtclock>0.0) {
    int frame=(int)(SPRITE->hurtclock*12.000);
    if (frame&1) tint=0xff000080;
    else tint=0xffffff80;
  }
  tile_renderer_begin(&g.tile_renderer,texid,tint,alpha);
  
  // Sword is highly significant so it gets priority.
  if (hero_item_in_use(sprite,ITEM_SWORD)) {
    hero_render_sword(sprite);
    
  // Shield is like the default, but different body tiles.
  } else if (hero_item_in_use(sprite,ITEM_SHIELD)) {
    hero_render_shield(sprite);
    
  // With cloak in use, skip 2/3 frames. (otherwise normal render). Nope, that's going to kill somebody with a seizure.
  //} else if (hero_item_in_use(sprite,ITEM_CLOAK)&&(g.renderseq%5)) {
  
  // Normal render: Just head and body, with walking and pushing animation.
  } else {
    switch (SPRITE->facedir) {
      case DIR_N: hero_render_twotile(sprite,0x04,0,0x01,0,1); break;
      case DIR_S: hero_render_twotile(sprite,0x03,0,0x00,0,0); break;
      case DIR_W: hero_render_twotile(sprite,0x05,0,0x02,0,0); break;
      case DIR_E: hero_render_twotile(sprite,0x05,EGG_XFORM_XREV,0x02,EGG_XFORM_XREV,0); break;
    }
  }
  tile_renderer_end(&g.tile_renderer);
}

/* Calculate visible bounds.
 */

void hero_calculate_bounds(struct sprite *sprite,int tilesize,int addx,int addy) {
  int x=(int)(sprite->x*tilesize)+addx;
  int y=(int)(sprite->y*tilesize)+addy;
  sprite->bw=tilesize;
  sprite->bh=tilesize;
  sprite->bx=x-(sprite->bw>>1);
  sprite->by=y-(sprite->bh>>1);
}
