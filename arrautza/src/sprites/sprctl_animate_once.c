#include "arrautza.h"

struct sprite_animate_once {
  struct sprite hdr;
};

#define SPRITE ((struct sprite_animate_once*)sprite)

/* Update.
 */
 
static void _animate_once_update(struct sprite *sprite,double elapsed) {
  //TODO
}

/* Type definition.
 */
 
const struct sprctl sprctl_animate_once={
  .name="animate_once",
  .objlen=sizeof(struct sprite_animate_once),
  .grpmask=(
    (1<<SPRGRP_UPDATE)|
    (1<<SPRGRP_RENDER)|
  0),
  .update=_animate_once_update,
};
