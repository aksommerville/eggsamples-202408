/* sprctl_blinktoast.c
 * Blinks one frame for a second or two, then destroys self.
 */

#include "arrautza.h"

struct sprite_blinktoast {
  struct sprite hdr;
  double ttl;
  double clock;
  double halfperiod;
  int phase; // 0,1
};

#define SPRITE ((struct sprite_blinktoast*)sprite)

/* Init.
 */
 
static int _blinktoast_init(struct sprite *sprite) {
  SPRITE->ttl=2.0;
  SPRITE->halfperiod=0.125;
  SPRITE->clock=SPRITE->halfperiod;
  return 0;
}

/* Update.
 */
 
static void _blinktoast_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->clock-=elapsed)<=0.0) {
    SPRITE->clock+=SPRITE->halfperiod;
    SPRITE->phase^=1;
  }
  if ((SPRITE->ttl-=elapsed)<=0.0) {
    sprite_kill_soon(sprite);
  }
}

/* Render.
 */
 
static void _blinktoast_render(int dsttexid,struct sprite *sprite) {
  int texid=texcache_get(&g.texcache,sprite->imageid);
  if (texid<1) return;
  if (SPRITE->phase) egg_render_alpha(0x80);
  struct egg_draw_tile vtx={
    .x=sprite->bx+(sprite->bw>>1),
    .y=sprite->by+(sprite->bh>>1),
    .tileid=sprite->tileid,
    .xform=sprite->xform,
  };
  egg_draw_tile(dsttexid,texid,&vtx,1);
  if (SPRITE->phase) egg_render_alpha(0xff);
}

/* Type definition.
 */
 
const struct sprctl sprctl_blinktoast={
  .name="blinktoast",
  .objlen=sizeof(struct sprite_blinktoast),
  .grpmask=(
    (1<<SPRGRP_RENDER)|
    (1<<SPRGRP_UPDATE)|
  0),
  .init=_blinktoast_init,
  .update=_blinktoast_update,
  .render=_blinktoast_render,
};
