#include "hero_internal.h"

/* Footing change.
 */
 
static int hero_footing_cb(const uint8_t *cmd,int cmdc,void *userdata) {
  struct sprite *sprite=userdata;
  if (cmdc<3) return 0;
  if (cmd[1]!=sprite->col) return 0;
  if (cmd[2]!=sprite->row) return 0;
  switch (cmd[0]) {
  
    case MAPCMD_door: {
        int mapid=(cmd[3]<<8)|cmd[4];
        int dstx=cmd[5];
        int dsty=cmd[6];
        // [7,8] exist and might mean something in the future.
        load_map(mapid,dstx,dsty,TRANSITION_SPOTLIGHT);
        SPRITE->motion_blackout=0.500; // Come to a full stop during the transition. This is only for doors, not neighbor transitions.
      } return 1; // stop iteration. important!
      
    //TODO treadles. Should those be mapcmd or sprite?
  }
  return 0;
}
 
void hero_footing(struct sprite *sprite,int8_t pvcol,int8_t pvrow) {
  
  // If we're OOB, request transition to a neighbor map and do nothing more.
  if (sprite->col<0)     { load_neighbor(MAPCMD_neighborw); return; }
  if (sprite->col>=COLC) { load_neighbor(MAPCMD_neighbore); return; }
  if (sprite->row<0)     { load_neighbor(MAPCMD_neighborn); return; }
  if (sprite->row>=ROWC) { load_neighbor(MAPCMD_neighbors); return; }
  
  // Anything else we do at footing, the cell must be marked in poibits, and it's driven by a map command.
  int p=sprite->row*COLC+sprite->col;
  if (!(g.poibits[p>>3]&(0x80>>(p&7)))) return;
  map_for_each_command(&g.map,hero_footing_cb,sprite);
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
