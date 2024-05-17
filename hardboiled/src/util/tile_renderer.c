#include "tile_renderer.h"

/* Flush.
 */
 
static void tile_renderer_flush(struct tile_renderer *tr) {
  if (tr->tilec) {
    if (!tr->dsttexid) tr->dsttexid=1;
    egg_draw_tile(tr->dsttexid,tr->texid,tr->tilev,tr->tilec);
    tr->tilec=0;
  }
}

/* Begin.
 */
 
void tile_renderer_begin(struct tile_renderer *tr,int texid,uint32_t tint,uint8_t alpha) {
  tr->texid=texid;
  egg_texture_get_header(&tr->tilesize,0,0,texid);
  tr->tilesize>>=4;
  egg_render_tint(tint);
  egg_render_alpha(alpha);
}

/* End.
 */
 
void tile_renderer_end(struct tile_renderer *tr) {
  tile_renderer_flush(tr);
  egg_render_tint(0);
  egg_render_alpha(0xff);
}

/* Add one tile.
 */

void tile_renderer_tile(struct tile_renderer *tr,int16_t x,int16_t y,uint8_t tileid,uint8_t xform) {
  if (tr->tilec>=TILE_RENDERER_CACHE_SIZE) tile_renderer_flush(tr);
  struct egg_draw_tile *tile=tr->tilev+tr->tilec++;
  tile->x=x;
  tile->y=y;
  tile->tileid=tileid;
  tile->xform=xform;
}

/* Add string.
 */

void tile_renderer_string(struct tile_renderer *tr,int16_t x,int16_t y,const char *src,int srcc) {
  if (!src) return;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  for (;srcc-->0;src++,x+=tr->tilesize) tile_renderer_tile(tr,x,y,*src,0);
}
