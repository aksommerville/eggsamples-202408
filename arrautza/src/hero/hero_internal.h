#ifndef HERO_INTERNAL_H
#define HERO_INTERNAL_H

#include "arrautza.h"

#define HERO_WALK_ANIM_TIME 0.150 /* s/frame */
#define HERO_WALK_SPEED 5.0 /* m/s */

struct sprite_hero {
  struct sprite hdr;
  int pvinput;
  int facedir; // DIR_N,DIR_W,DIR_E,DIR_S
  int indx,indy; // Input state, digested to (-1,0,1).
  double animclock;
  int animframe;
  int pushing; // Clear on update, then it gets set during physics resolution.
  double motion_blackout; // Motion is suppressed until this counts down.
};

#define SPRITE ((struct sprite_hero*)sprite)

// hero_render.c
void hero_render(int dsttexid,struct sprite *sprite);
void hero_calculate_bounds(struct sprite *sprite,int tilesize,int addx,int addy);

// hero_update.c
void hero_update(struct sprite *sprite,double elapsed);

// hero_event.c
void hero_footing(struct sprite *sprite,int8_t pvcol,int8_t pvrow);
void hero_collision(struct sprite *sprite,struct sprite *other,uint8_t dir,int physics);

#endif
