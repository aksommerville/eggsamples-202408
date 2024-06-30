#include "arrautza.h"

struct sprite_animate {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_animate*)sprite)

/* Update.
 */
 
static void _animate_update(struct sprite *sprite,double elapsed) {
  //TODO
}

/* Type definition.
 */
 
const struct sprctl sprctl_animate={
  .name="animate",
  .objlen=sizeof(struct sprite_animate),
  .grpmask=(
    (1<<SPRGRP_UPDATE)|
    (1<<SPRGRP_RENDER)|
  0),
  .update=_animate_update,
};
