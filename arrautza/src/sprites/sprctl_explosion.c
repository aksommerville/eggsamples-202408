/* sprctl_explosion.c
 * Animates once then disappears.
 * argv: [sound(boolean),damage(boolean)]
 * Sound and damage are deferred to the first update.
 */
 
#include "arrautza.h"

#define EXPLOSION_TTL 0.250

struct sprite_explosion {
  struct sprite hdr;
  double ttl;
  uint8_t do_sound;
  uint8_t do_damage;
};

#define SPRITE ((struct sprite_explosion*)sprite)

/* Init.
 */
 
static int _explosion_init(struct sprite *sprite) {
  sprite->layer=120;
  SPRITE->ttl=EXPLOSION_TTL;
  return 0;
}

static int _explosion_ready(struct sprite *sprite,const uint8_t *argv,int argc) {
  if (argc>=1) {
    SPRITE->do_sound=argv[0];
    if (argc>=2) {
      SPRITE->do_damage=argv[1];
    }
  }
  return 0;
}

/* Deal damage.
 */
 
static void explosion_deal_damage(struct sprite *sprite) {
  const double radius=2.0;
  const double radius2=radius*radius;
  struct sprgrp *sprgrp;
  int i;
  
  // Damage fragile sprites in range.
  // This will trigger other bombs.
  sprgrp=sprgrpv+SPRGRP_FRAGILE;
  for (i=sprgrp->sprc;i-->0;) {
    struct sprite *victim=sprgrp->sprv[i];
    if (!victim->sprctl||!victim->sprctl->damage) continue;
    double adx=victim->x-sprite->x; if (adx<0.0) adx=-adx;
    if (adx>=radius) continue;
    double ady=victim->y-sprite->y; if (ady<0.0) ady=-ady;
    if (ady>=radius) continue;
    double d2=adx*adx+ady*ady;
    if (d2>radius2) continue;
    victim->sprctl->damage(victim,1,sprite);
  }

  //TODO Eliminate specially-marked map cells.
}

/* Update.
 */
 
static void _explosion_update(struct sprite *sprite,double elapsed) {
  if (SPRITE->do_sound) {
    SPRITE->do_sound=0;
    //TODO sound effect
  }
  if (SPRITE->do_damage) {
    SPRITE->do_damage=0;
    explosion_deal_damage(sprite);
  }
  if ((SPRITE->ttl-=elapsed)<=0.0) {
    sprite_kill_soon(sprite);
    return;
  }
}

/* Render.
 * We don't use the default renderer because we need four tiles.
 */
 
static void _explosion_render(int dsttexid,struct sprite *sprite) {
  // (dstx,dsty): Position of top-left tile.
  int dstx=(int)(sprite->x*TILESIZE)-(TILESIZE>>1);
  int dsty=(int)(sprite->y*TILESIZE)-(TILESIZE>>1);
  int frame=(int)(((EXPLOSION_TTL-SPRITE->ttl)*4.0)/EXPLOSION_TTL);
  if (frame<0) frame=0;
  else if (frame>3) frame=3;
  tile_renderer_begin(&g.tile_renderer,texcache_get(&g.texcache,sprite->imageid),0,0xff);
  tile_renderer_tile(&g.tile_renderer,dstx,dsty,sprite->tileid+frame,0);
  tile_renderer_tile(&g.tile_renderer,dstx+TILESIZE,dsty,sprite->tileid+frame,EGG_XFORM_SWAP|EGG_XFORM_YREV);
  tile_renderer_tile(&g.tile_renderer,dstx,dsty+TILESIZE,sprite->tileid+frame,EGG_XFORM_SWAP|EGG_XFORM_XREV);
  tile_renderer_tile(&g.tile_renderer,dstx+TILESIZE,dsty+TILESIZE,sprite->tileid+frame,EGG_XFORM_XREV|EGG_XFORM_YREV);
  tile_renderer_end(&g.tile_renderer);
}

/* Type definition.
 */
 
const struct sprctl sprctl_explosion={
  .name="explosion",
  .objlen=sizeof(struct sprite_explosion),
  .grpmask=(
    (1<<SPRGRP_RENDER)|
    (1<<SPRGRP_UPDATE)|
  0),
  .init=_explosion_init,
  .ready=_explosion_ready,
  .update=_explosion_update,
  .render=_explosion_render,
};
