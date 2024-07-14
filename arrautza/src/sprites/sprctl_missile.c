/* sprctl_missile.c
 * Generic missile. Moves in a fixed direction until something stops us, or we go offscreen.
 * argv: [dir,speed(m/s),preadvance(pixels)]
 * Natural direction is South.
 */
 
#include "arrautza.h"
 
struct sprite_missile {
  struct sprite hdr;
  double dx,dy;
};

#define SPRITE ((struct sprite_missile*)sprite)

/* Init.
 */
 
static int _missile_init(struct sprite *sprite) {
  return 0;
}

static int _missile_ready(struct sprite *sprite,const uint8_t *argv,int argc) {
  if (argc<1) {
    // Ensure a nonzero speed, so we don't stick. But I guess it's an error either way.
    SPRITE->dy=1.0;
  } else {
    switch (argv[0]) {
      case DIR_S: SPRITE->dy= 1.0; sprite->xform=0; break;
      case DIR_N: SPRITE->dy=-1.0; sprite->xform=EGG_XFORM_XREV|EGG_XFORM_YREV; break;
      case DIR_W: SPRITE->dx=-1.0; sprite->xform=EGG_XFORM_SWAP|EGG_XFORM_YREV; break;
      case DIR_E: SPRITE->dx= 1.0; sprite->xform=EGG_XFORM_SWAP|EGG_XFORM_XREV; break;
      default: return -1;
    }
    if (argc>=2) {
      SPRITE->dx*=(double)argv[1];
      SPRITE->dy*=(double)argv[1];
    }
  }
  if ((argc>=3)&&argv[2]) {
    // Caller may request to advance a bit from the start position. eg for the hero's bow and arrow.
    double offset=(double)argv[2]/(double)TILESIZE;
         if (SPRITE->dx<0.0) sprite->x-=offset;
    else if (SPRITE->dx>0.0) sprite->x+=offset;
    else if (SPRITE->dy<0.0) sprite->y-=offset;
    else if (SPRITE->dy>0.0) sprite->y+=offset;
  }
  return 0;
}

/* Update.
 */
 
static void _missile_update(struct sprite *sprite,double elapsed) {
  sprite->x+=SPRITE->dx*elapsed;
  sprite->y+=SPRITE->dy*elapsed;
  if ((sprite->x<-1.0)||(sprite->y<-1.0)||(sprite->x>COLC+1.0)||(sprite->y>ROWC+1.0)) {
    sprite_kill_soon(sprite);
    return;
  }
  //TODO Other obstructions and damage.
}

/* Type definition.
 */
 
const struct sprctl sprctl_missile={
  .name="missile",
  .objlen=sizeof(struct sprite_missile),
  .grpmask=(
    (1<<SPRGRP_RENDER)|
    (1<<SPRGRP_UPDATE)|
  0),
  .init=_missile_init,
  .ready=_missile_ready,
  .update=_missile_update,
};
