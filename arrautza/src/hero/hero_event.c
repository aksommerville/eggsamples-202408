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
 
void hero_collision(struct sprite *sprite,struct sprite *other,uint8_t dir,int physics) {
  if (dir==SPRITE->facedir) {
    if (other) { // It's a sprite, so always enter the "push" face.
      SPRITE->pushing=1;
      if (other==SPRITE->pushsprite) {
        SPRITE->pushsprite_again=1;
      } else if (!SPRITE->pushsprite) {
        SPRITE->pushsprite=other;
        SPRITE->pushsprite_time=0;
        SPRITE->pushsprite_again=1;
      }
    } else if (physics&(1<<1)) { // A WALL tile is involved, so do "push".
      SPRITE->pushing=1;
    } else {
      // Pressing something else, eg water. Don't push.
    }
  }
}
