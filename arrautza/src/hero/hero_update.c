#include "hero_internal.h"

/* Motion.
 */
 
static void hero_begin_motion(struct sprite *sprite,int dx,int dy) {
  if (!SPRITE->indx&&!SPRITE->indy) {
    SPRITE->animframe=0;
    SPRITE->animclock=HERO_WALK_ANIM_TIME;
  }
  if (dx) {
    SPRITE->indx=dx;
    if (dx<0) {
      SPRITE->facedir=DIR_W;
    } else {
      SPRITE->facedir=DIR_E;
    }
  } else if (dy) {
    SPRITE->indy=dy;
    if (dy<0) {
      SPRITE->facedir=DIR_N;
    } else {
      SPRITE->facedir=DIR_S;
    }
  }
}

static void hero_end_motion(struct sprite *sprite,int dx,int dy) {
  if (dx) {
    SPRITE->indx=0;
    if (SPRITE->indy<0) {
      SPRITE->facedir=DIR_N;
    } else if (SPRITE->indy>0) {
      SPRITE->facedir=DIR_S;
    }
  } else if (dy) {
    SPRITE->indy=0;
    if (SPRITE->indx<0) {
      SPRITE->facedir=DIR_W;
    } else if (SPRITE->indx>0) {
      SPRITE->facedir=DIR_E;
    }
  }
  SPRITE->animframe=0;
}

/* Update.
 */
 
void hero_update(struct sprite *sprite,double elapsed) {
  if (g.instate!=SPRITE->pvinput) {
    #define PRESS(tag) ((g.instate&INKEEP_BTNID_##tag)&&!(SPRITE->pvinput&INKEEP_BTNID_##tag))
    #define RELEASE(tag) (!(g.instate&INKEEP_BTNID_##tag)&&(SPRITE->pvinput&INKEEP_BTNID_##tag))
    if (PRESS(LEFT)) hero_begin_motion(sprite,-1,0); else if (RELEASE(LEFT)&&(SPRITE->indx<0)) hero_end_motion(sprite,-1,0);
    if (PRESS(RIGHT)) hero_begin_motion(sprite,1,0); else if (RELEASE(RIGHT)&&(SPRITE->indx>0)) hero_end_motion(sprite,1,0);
    if (PRESS(UP)) hero_begin_motion(sprite,0,-1); else if (RELEASE(UP)&&(SPRITE->indy<0)) hero_end_motion(sprite,0,-1);
    if (PRESS(DOWN)) hero_begin_motion(sprite,0,1); else if (RELEASE(DOWN)&&(SPRITE->indy>0)) hero_end_motion(sprite,0,1);
    #undef PRESS
    #undef RELEASE
    SPRITE->pvinput=g.instate;
  }
  if (SPRITE->motion_blackout>0.0) {
    SPRITE->motion_blackout-=elapsed;
  } else if (SPRITE->indx||SPRITE->indy) {
    const double speed=HERO_WALK_SPEED;
    sprite->x+=SPRITE->indx*elapsed*speed;
    sprite->y+=SPRITE->indy*elapsed*speed;
    if ((SPRITE->animclock-=elapsed)<=0.0) {
      SPRITE->animclock+=HERO_WALK_ANIM_TIME;
      if (++(SPRITE->animframe)>=4) {
        SPRITE->animframe=0;
      }
    }
  } else {
    SPRITE->animframe=1;
  }
  SPRITE->pushing=0; // until physics tells us otherwise, each frame.
}
