/* sprctl_bomb.c
 */
 
#include "arrautza.h"

#define BOMB_TTL_MAX 3.0

struct sprite_bomb {
  struct sprite hdr;
  double animclock;
  int animframe;
  double ttl;
  int triggered; // Prevent self-triggering and maybe other faults.
};

#define SPRITE ((struct sprite_bomb*)sprite)

/* Init.
 */
 
static int _bomb_init(struct sprite *sprite) {
  SPRITE->ttl=BOMB_TTL_MAX;
  sprite->invmass=128; // as heavy as a man, let's say?
  sprite_set_hitbox(sprite,0.6,0.6,0.0,0.0);
  return 0;
}

static int _bomb_ready(struct sprite *sprite,const uint8_t *argv,int argc) {
  return 0;
}

/* Update.
 */
 
static void _bomb_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.250;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
  }
  if ((SPRITE->ttl-=elapsed)<=0.0) {
    sprite->sprctl->damage(sprite,0,0);
    return;
  }
  
  // Add ourselves to the SOLID group if the hero is not colliding, then stay in it.
  if (!sprgrp_has(sprgrpv+SPRGRP_SOLID,sprite)) {
    if (!sprite_collides_with_group(sprite,sprgrpv+SPRGRP_HERO)) {
      sprgrp_add(sprgrpv+SPRGRP_SOLID,sprite);
    }
  }
}

/* Damage.
 */
 
static void _bomb_damage(struct sprite *sprite,int qual,struct sprite *assailant) {
  if (SPRITE->triggered) return;
  SPRITE->triggered=1;
  uint8_t argv[]={1,1}; // sound,damage
  struct sprite *explosion=sprite_spawn_resless(&sprctl_explosion,sprite->imageid,sprite->tileid+0x10,sprite->x,sprite->y,argv,sizeof(argv));
  sprite_kill_soon(sprite);
}

/* Render.
 * We don't use the default renderer because we need two tiles: bomb and fuse.
 */
 
static void _bomb_render(int dsttexid,struct sprite *sprite) {
  int dstx=(int)(sprite->x*TILESIZE);
  int dsty=(int)(sprite->y*TILESIZE);
  uint8_t tileid=sprite->tileid;
  switch (SPRITE->animframe) {
    case 1: tileid+=0x01; break;
    case 3: tileid+=0x02; break;
  }
  int fuseframe=6-(int)((SPRITE->ttl*7.0)/BOMB_TTL_MAX);
  if (fuseframe<0) fuseframe=0;
  else if (fuseframe>6) fuseframe=6;
  uint8_t fusetileid=sprite->tileid+3+fuseframe;
  tile_renderer_begin(&g.tile_renderer,texcache_get(&g.texcache,sprite->imageid),0,0xff);
  tile_renderer_tile(&g.tile_renderer,dstx,dsty,tileid,0);
  tile_renderer_tile(&g.tile_renderer,dstx,dsty,fusetileid,0);
  tile_renderer_end(&g.tile_renderer);
}

/* Type definition.
 */
 
const struct sprctl sprctl_bomb={
  .name="bomb",
  .objlen=sizeof(struct sprite_bomb),
  .grpmask=(
    (1<<SPRGRP_RENDER)|
    (1<<SPRGRP_UPDATE)|
    (1<<SPRGRP_FRAGILE)|
  0),
  .init=_bomb_init,
  .ready=_bomb_ready,
  .update=_bomb_update,
  .render=_bomb_render,
  .damage=_bomb_damage,
};
