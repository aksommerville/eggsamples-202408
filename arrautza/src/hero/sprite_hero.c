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
  .update=hero_update,
  .render=hero_render,
  .calculate_bounds=hero_calculate_bounds,
  .footing=hero_footing,
  .collision=hero_collision,
};
