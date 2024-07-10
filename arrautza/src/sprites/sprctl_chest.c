#include "arrautza.h"

struct sprite_chest {
  struct sprite hdr;
  int full;
  int itemid;
  int count;
  int flag;
};

#define SPRITE ((struct sprite_chest*)sprite)

/* Ready.
 */
 
static int _chest_ready(struct sprite *sprite,const uint8_t *argv,int argc) {
  if (argc>=1) SPRITE->itemid=argv[0];
  if (argc>=2) SPRITE->count=argv[1];
  if (argc>=3) SPRITE->flag=argv[2];
  
  // If flag is declared, that alone drives whether we're full.
  if (SPRITE->flag) {
    if (stobus_get(&g.stobus,SPRITE->flag)) SPRITE->full=0;
    else SPRITE->full=1;
    
  // If itemid is invalid, make us empty.
  } else if ((SPRITE->itemid<1)||(SPRITE->itemid>ITEM_COUNT)) {
    SPRITE->full=0;
    
  // No flag, but a nonzero count? We're full always. This is probably a bad idea.
  // Player can leave and come back and max out her inventory.
  } else if (SPRITE->count) {
    SPRITE->full=1;
  
  // No flag or count, our fullness depends on whether the item is acquired.
  } else if (item_possessed(SPRITE->itemid)) {
    SPRITE->full=0;
  } else {
    SPRITE->full=1;
  }
  
  // The empty tile must be immediately right of the natural (full) tile.
  if (!SPRITE->full) sprite->tileid++;
  
  sprite_set_hitbox(sprite,1.0,1.0,0,0);
  
  return 0;
}

/* Hero overlaps.
 */
 
static void _chest_heronotify(struct sprite *sprite,struct sprite *hero) {
  if (!SPRITE->full) return;
  SPRITE->full=0;
  sprite->tileid++;
  if (SPRITE->flag) {
    stobus_set(&g.stobus,SPRITE->flag,1);
  }
  if ((SPRITE->itemid>0)&&(SPRITE->itemid<=ITEM_COUNT)) {
    acquire_item(SPRITE->itemid,SPRITE->count);
    //TODO Sound effect.
    uint8_t tileid;
    if (SPRITE->itemid==ITEM_COMPASS) tileid=0xb5; // Compass's natural tile has the hand removed. Use a special one.
    else tileid=0x90+SPRITE->itemid-1;
    struct sprite *blinktoast=sprite_spawn_resless(&sprctl_blinktoast,RID_image_hero,tileid,sprite->x,sprite->y-0.75,0,0);
    if (blinktoast) {
      blinktoast->layer=120;
    }
  }
}

/* Type definition.
 */
 
const struct sprctl sprctl_chest={
  .name="chest",
  .objlen=sizeof(struct sprite_chest),
  .grpmask=(
    (1<<SPRGRP_RENDER)|
    (1<<SPRGRP_UPDATE)|
  0),
  .ready=_chest_ready,
  .heronotify=_chest_heronotify,
};
