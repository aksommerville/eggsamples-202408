#include "hero_internal.h"

/* Footing change.
 */
 
void hero_footing(struct sprite *sprite,int8_t pvcol,int8_t pvrow) {
  
  // If we're OOB, request transition to a neighbor map and do nothing more.
  if (sprite->col<0)     { load_neighbor(MAPCMD_neighborw); return; }
  if (sprite->col>=COLC) { load_neighbor(MAPCMD_neighbore); return; }
  if (sprite->row<0)     { load_neighbor(MAPCMD_neighborn); return; }
  if (sprite->row>=ROWC) { load_neighbor(MAPCMD_neighbors); return; }
  
  //TODO doors
  //TODO treadles, etc
}

/* Collision.
 */
 
void hero_collision(struct sprite *sprite,struct sprite *other,uint8_t dir) {
  //egg_log("%s %p 0x%x %s",__func__,other,dir,other?"nonzero":"zero");
  if (dir==SPRITE->facedir) {
    //TODO Pushing against static walls is good, but do check map physics and don't enter (pushing) if it's HOLE or WATER.
    SPRITE->pushing=1;
  }
}
