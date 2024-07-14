#ifndef HERO_INTERNAL_H
#define HERO_INTERNAL_H

#include "arrautza.h"

#define HERO_WALK_ANIM_TIME 0.150 /* s/frame */
#define HERO_WALK_SPEED 5.0 /* m/s */
#define HERO_PUSH_ACTIVATION_TIME 0.200
#define HERO_HURT_TIME 0.500

struct sprite_hero {
  struct sprite hdr;
  int pvinput;
  int facedir; // DIR_N,DIR_W,DIR_E,DIR_S
  int indx,indy; // Input state, digested to (-1,0,1).
  double animclock;
  int animframe;
  int pushing; // Clear on update, then it gets set during physics resolution.
  double motion_blackout; // Motion is suppressed until this counts down.
  struct sprite *pushsprite; // WEAK, for identification only. DO NOT DEREFERENCE.
  double pushsprite_time; // How long has (pushsprite) been colliding?
  int pushsprite_again; // Set by hero_collision() if (pushsprite) is still in a state of collision.
  uint8_t aitem,bitem; // Nonzero only if an action is in progress.
  double swordtime;
  int sword_drop_pending;
  double shieldtime;
  int shield_drop_pending;
  double hurtclock; // Counts down; invincible if nonzero.
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
void hero_damage(struct sprite *sprite,int qual,struct sprite *assailant);

// hero_item.c
void hero_item_begin(struct sprite *sprite,uint8_t itemid,int slot); // slot is (0,1)=(a,b)
void hero_item_end(struct sprite *sprite,uint8_t itemid);
void hero_item_update(struct sprite *sprite,uint8_t itemid,double elapsed);
int hero_item_in_use(struct sprite *sprite,uint8_t itemid);
int hero_walk_inhibited_by_item(struct sprite *sprite);

#endif
