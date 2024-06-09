#include "gravedigger.h"

/* Reset.
 */
 
void hero_reset(struct hero *hero) {
  hero->x=SCREENW/2.0; // center
  hero->y=SCREENH/2.0; // bottom
  hero->dx=0;
  hero->facex=1;
  hero->frame=0;
  hero->clock=0.0;
  hero->carrying=0;
  hero->seated=1;
  hero->effort=0; // Nonzero if we ascended on the last frame; slow down the next motion.
  hero->jump=0;
  hero->jumppower=0.0;
  hero->gravity=0.0;
  hero->coyote=0.0; // Counts down after unseating, a brief interval when jumping is still allowed.
  hero->digclock=0.0; // Counts down during dig animation; no other activity permitted until it reaches zero.
}

/* Render.
 */
 
void hero_render(uint32_t *dst,const struct hero *hero) {
  int dstx=(int)hero->x-3;
  int dsty=(int)hero->y-11;
  int srcx=hero->frame*7;
  if (hero->carrying) srcx+=28;
  else if (hero->digclock>=0.200) srcx=56;
  else if (hero->digclock>0.0) srcx=63;
  blit_sprite(dst,dstx,dsty,g.spritebits,srcx,0,7,11,hero->facex<0);
  if (hero->carrying) {
    blit_sprite(dst,dstx-2,dsty-4,g.spritebits,0,11,11,4,0);
  }
}

/* Set or clear (seated).
 */

void hero_check_seated(struct hero *hero,int bubble_up) {
  int x=(int)hero->x;
  int y=(int)hero->y;
  if (x<0) x=0; else if (x>=SCREENW) x=SCREENW-1;
  if (y<0) y=0; else if (y>=SCREENH) y=SCREENH;
  if ((y>=SCREENH)||COLOR_IS_DIRT(g.terrain[y*SCREENW+x])) {
    if (bubble_up) {
      while ((y>0)&&COLOR_IS_DIRT(g.terrain[(y-1)*SCREENW+x])) {
        y--;
        hero->y-=1.0;
      }
    }
    if (!hero->seated) {
      while ((y>0)&&COLOR_IS_DIRT(g.terrain[(y-1)*SCREENW+x])) y--;
      hero->seated=1;
      hero->y=y;
      hero->gravity=0.0;
    }
  } else if (hero->seated) {
    hero->seated=0;
    hero->coyote=0.080;
  }
}

/* If we've walked into dirt, try stepping up. If that fails, return zero to request backing out the move.
 */
 
static int hero_adjust_after_walk(struct hero *hero) {
  int x=(int)hero->x;
  int y=(int)hero->y-11;
  if (x<0) x=0; else if (x>=SCREENW) x=SCREENW-1;
  if (y<0) y=0; else if (y>=SCREENH) y=SCREENH-1;
  int depth=find_highest_dirt(g.terrain,x,y,1,11);
  if (depth>=11) return 1; // Free and clear.
  if (depth>=8) { // Allow walking up gentle slopes.
    hero->y=y+depth;
    hero->effort=1;
    return 1;
  }
  // Too much dirt here. Reject the move.
  return 0;
}

/* Update.
 */
 
void hero_update(struct hero *hero,double elapsed) {

  if ((hero->coyote-=elapsed)<=0.0) hero->coyote=0.0;
  if ((hero->digclock-=elapsed)<=0.0) hero->digclock=0.0;

  if (hero->dx&&(hero->digclock<=0.0)) {
    if ((hero->clock-=elapsed)<=0) {
      hero->clock+=0.125;
      if (++hero->frame>=4) hero->frame=0;
    }
    double walkspeed=60.0; // px/s
    if (hero->effort) {
      walkspeed*=0.5;
      hero->effort=0;
    }
    double herox0=hero->x;
    hero->x+=walkspeed*elapsed*hero->dx;
    if (hero->x<0.0) hero->x=0.0;
    else if (hero->x>SCREENW) hero->x=SCREENW;
    if (!hero_adjust_after_walk(hero)) hero->x=herox0;
    hero_check_seated(hero,0);
  }

  if (hero->jump) {
    hero->jumppower-=400.0*elapsed;
    if (hero->jumppower<10.0) {
      hero->jump=0;
    } else {
      hero->y-=hero->jumppower*elapsed;
    }
    hero_check_seated(hero,0);

  } else if (!hero->seated) {
    hero->gravity+=300.0*elapsed;
    if (hero->gravity>200.0) hero->gravity=200.0;
    hero->y+=hero->gravity*elapsed;
    if (hero->y>=SCREENH) hero->y=SCREENH;
    hero_check_seated(hero,0);
  }
}

void hero_update_noninteractive(struct hero *hero,double elapsed) {
  if (!hero->seated) {
    hero->gravity+=300.0*elapsed;
    if (hero->gravity>200.0) hero->gravity=200.0;
    hero->y+=hero->gravity*elapsed;
    if (hero->y>=SCREENH) hero->y=SCREENH;
    hero_check_seated(hero,0);
  }
}

/* Impulse events.
 */

void hero_walk_begin(struct hero *hero,int dx) {
  hero->dx=dx;
  hero->facex=dx;
}

void hero_walk_end(struct hero *hero) {
  hero->dx=0;
  hero->frame=0;
  hero->clock=0.0;
}

void hero_jump_begin(struct hero *hero) {
  if (!hero->seated&&(hero->coyote<=0.0)) return;
  if (hero->digclock>0.0) return;
  hero->jump=1;
  hero->jumppower=150.0;
  hero->gravity=0.0;
  hero->coyote=0.0;
}

void hero_jump_end(struct hero *hero) {
  if (!hero->jump) return;
  hero->jump=0;
  hero_check_seated(hero,0);
}

void hero_toggle_carry(struct hero *hero) {
  if (hero->carrying) {
    struct coffin *coffin=coffin_add();
    if (!coffin) {
      // oh no... what to do?
    } else {
      hero->carrying=0;
      coffin->x=((int)hero->x)-(COFFIN_WIDTH>>1);
      coffin->y=(int)hero->y-COFFIN_HEIGHT-11;
      coffins_force_valid(g.coffinv,g.coffinc);
    }
  } else {
    int p=coffin_find(g.coffinv,g.coffinc,(int)hero->x,(int)hero->y-1);
    if (p<0) {
      // Nothing to pick up here.
    } else {
      hero->carrying=1;
      int cx=g.coffinv[p].x;
      coffin_remove(p);
      repair_terrain(cx,COFFIN_WIDTH);
    }
  }
}

void hero_dig(struct hero *hero) {
  if (hero->carrying) return;
  if (!hero->seated) return;
  if (hero->jump) return;
  hero->digclock=0.400;
  int x=(int)hero->x;
  int y=(int)hero->y;
  choose_and_move_dirt(x,y,hero->facex);
  hero_check_seated(hero,1);
}
