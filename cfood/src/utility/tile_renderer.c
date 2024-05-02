#include <egg/egg.h>
#include <stdlib.h>

// Completely arbitrary. Increase to trade memory for efficiency.
#define VTXLIMIT 256

struct tile_renderer {
  int dsttexid;
  int srctexid;
  int tilew; // zero if we haven't checked yet; only 'string' needs it
  struct egg_draw_tile vtxv[VTXLIMIT];
  int vtxc;
};

/* Object lifecycle.
 */
 
void tile_renderer_del(struct tile_renderer *tr) {
  if (!tr) return;
  free(tr);
}

struct tile_renderer *tile_renderer_new() {
  struct tile_renderer *tr=calloc(1,sizeof(struct tile_renderer));
  if (!tr) return 0;
  tr->dsttexid=1;
  return tr;
}

/* Trivial accessors.
 */

void tile_renderer_set_output(struct tile_renderer *tr,int texid) {
  tr->dsttexid=texid;
}

/* Flush cache.
 */
 
static void tile_renderer_flush(struct tile_renderer *tr) {
  egg_draw_tile(tr->dsttexid,tr->srctexid,tr->vtxv,tr->vtxc);
  tr->vtxc=0;
}

/* Begin batch.
 */

void tile_renderer_begin(struct tile_renderer *tr,int texid,int tint,int alpha) {
  if (tr->vtxc) tile_renderer_flush(tr);
  if (texid!=tr->srctexid) {
    tr->tilew=0;
  }
  tr->srctexid=texid;
  egg_draw_mode(EGG_XFERMODE_ALPHA,tint,alpha);
}

/* Finish batch.
 */

void tile_renderer_end(struct tile_renderer *tr) {
  if (tr->vtxc) tile_renderer_flush(tr);
  egg_draw_mode(0,0,0xff);
}

void tile_renderer_cancel(struct tile_renderer *tr) {
  tr->vtxc=0;
  egg_draw_mode(0,0,0xff);
}

/* Add tile.
 */

void tile_renderer_tile(struct tile_renderer *tr,int x,int y,int tileid,int xform) {
  if (tr->vtxc>=VTXLIMIT) tile_renderer_flush(tr);
  struct egg_draw_tile *tile=tr->vtxv+tr->vtxc++;
  tile->x=x;
  tile->y=y;
  tile->tileid=tileid;
  tile->xform=xform;
}

/* Add string.
 */
 
void tile_renderer_string(struct tile_renderer *tr,int x,int y,const char *src,int srcc) {
  if (!src) return;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if ((srcc>1)&&!tr->tilew) {
    int w=0,h=0,fmt=0;
    egg_texture_get_header(&w,&h,&fmt,tr->srctexid);
    tr->tilew=w>>4;
  }
  for (;srcc-->0;src++,x+=tr->tilew) tile_renderer_tile(tr,x,y,*src,0);
}
