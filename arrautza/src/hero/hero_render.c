#include "hero_internal.h"

/* Render.
 */
 
void hero_render(int dsttexid,struct sprite *sprite) {
  int x=sprite->bx+(sprite->bw>>1);
  int y=sprite->by+(sprite->bh>>1);
  int texid=texcache_get(&g.texcache,sprite->imageid);
  if (texid<1) return;
  tile_renderer_begin(&g.tile_renderer,texid,0,0xff);
  switch (SPRITE->facedir) {
  
    case DIR_N: {
        uint8_t bodytileid=0x04;
        if (SPRITE->indx||SPRITE->indy) switch (SPRITE->animframe) {
          case 0: bodytileid=0x14; break;
          case 2: bodytileid=0x24; break;
        }
        if (SPRITE->pushing) bodytileid+=0x03;
        int heady=y;
        if (!SPRITE->pushing&&(SPRITE->animframe>=2)) heady-=7;
        else heady-=8;
        tile_renderer_tile(&g.tile_renderer,x,heady,0x01,0);
        tile_renderer_tile(&g.tile_renderer,x,y,bodytileid,0);
      } break;
      
    case DIR_S: {
        uint8_t bodytileid=0x03;
        if (SPRITE->indx||SPRITE->indy) switch (SPRITE->animframe) {
          case 0: bodytileid=0x13; break;
          case 2: bodytileid=0x23; break;
        }
        if (SPRITE->pushing) bodytileid+=0x03;
        int heady=y;
        if (!SPRITE->pushing&&(SPRITE->animframe>=2)) heady-=7;
        else heady-=8;
        tile_renderer_tile(&g.tile_renderer,x,y,bodytileid,0);
        tile_renderer_tile(&g.tile_renderer,x,heady,0x00,0);
      } break;
      
    case DIR_W:
    case DIR_E: {
        uint8_t bodytileid=0x05;
        if (SPRITE->indx||SPRITE->indy) switch (SPRITE->animframe) {
          case 0: bodytileid=0x15; break;
          case 2: bodytileid=0x25; break;
        }
        if (SPRITE->pushing) bodytileid+=0x03;
        int heady=y;
        if (!SPRITE->pushing&&(SPRITE->animframe>=2)) heady-=7;
        else heady-=8;
        uint8_t xform=(SPRITE->facedir==DIR_E)?EGG_XFORM_XREV:0;
        tile_renderer_tile(&g.tile_renderer,x,y,bodytileid,xform);
        tile_renderer_tile(&g.tile_renderer,x,heady,0x02,xform);
      } break;
      
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
