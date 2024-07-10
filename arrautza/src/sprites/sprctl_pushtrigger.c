/* sprctl_pushtrigger.c
 * Usually not visible.
 * Trigger some action when the hero collides with us, after a tasteful interval of continued collision.
 */
 
#include "arrautza.h"

// Whoever activates us should enforce their own blackout, but we do as a safety measure too.
#define BLACKOUT_TIME 0.250
 
struct sprite_pushtrigger {
  struct sprite hdr;
  struct sprite *hero; // WEAK
  int gothero;
  double blackout;
  int k,v;
};

#define SPRITE ((struct sprite_pushtrigger*)sprite)

/* Init.
 */
 
static int _pushtrigger_init(struct sprite *sprite) {
  // It's important that we be slightly wider than a cell.
  // We're typically placed on top of a solid cell.
  sprite_set_hitbox(sprite,1.125,1.125,0.0,0.0);
  sprite->mapsolids=0;
  return 0;
}

/* Ready.
 */
 
static int _pushtrigger_ready(struct sprite *sprite,const uint8_t *argv,int argc) {
  if (argc>=1) {
    SPRITE->k=argv[0];
    if (argc>=3) {
      SPRITE->v=(argv[1]<<8)|argv[2];
    }
  }
  return 0;
}

/* Update.
 */
 
static void _pushtrigger_update(struct sprite *sprite,double elapsed) {
  if (SPRITE->hero) {
    if (!SPRITE->gothero) {
      SPRITE->hero=0;
      SPRITE->blackout=0.0;
    } else if (SPRITE->blackout>0.0) {
      SPRITE->blackout-=elapsed;
    }
  }
  SPRITE->gothero=0;
}

/* Collision.
 */
 
static void _pushtrigger_collision(struct sprite *sprite,struct sprite *other,uint8_t dir,int physics) {
  if (other&&(other->sprctl==&sprctl_hero)) {
    SPRITE->gothero=1;
    SPRITE->hero=other;
  }
}

/* Type definition.
 */
 
const struct sprctl sprctl_pushtrigger={
  .name="pushtrigger",
  .objlen=sizeof(struct sprite_pushtrigger),
  .grpmask=(
    (1<<SPRGRP_UPDATE)|
    (1<<SPRGRP_SOLID)|
  0),
  .init=_pushtrigger_init,
  .ready=_pushtrigger_ready,
  .update=_pushtrigger_update,
  .collision=_pushtrigger_collision,
};

/* Activate.
 */
 
int sprite_pushtrigger_activate(struct sprite *sprite,struct sprite *activator,uint8_t dir) {
  if (!sprite||(sprite->sprctl!=&sprctl_pushtrigger)) return 0;
  if (SPRITE->blackout>0.0) {
    return 0;
  }
  SPRITE->blackout=BLACKOUT_TIME;
  if (!SPRITE->k) return 0;
  stobus_set(&g.stobus,SPRITE->k,SPRITE->v);
  return 1;
}
