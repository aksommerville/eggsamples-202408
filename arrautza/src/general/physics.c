#include "../arrautza.h"
#include "sprite.h"
#include "map.h"

/* Globals.
 */
 
static struct wall {
  struct aabb aabb;
  int physics; // 1<<tilesheet.physics
} physics_wallv[COLC*ROWC];
static int physics_wallc=0;

#define COLLISION_LIMIT 32
static struct collision {
  struct sprite *a,*b;
  uint8_t dir; // DIR_N,W,E,W, which edge of (a) is colliding
} physics_collisionv[COLLISION_LIMIT];
static int physics_collisionc=0;
 
/* Rebuild statics.
 */
 
void physics_rebuild_map() {
  physics_wallc=0;
  uint8_t tilesheet[256]={0};
  int tilesheetc=egg_res_get(tilesheet,sizeof(tilesheet),EGG_RESTYPE_tilesheet,0,g.imageid_tilesheet);
  if ((tilesheetc<1)||(tilesheetc>sizeof(tilesheet))) {
    egg_log("WARNING: No tilesheet for image:0:%d",g.imageid_tilesheet);
    return;
  }
  const uint8_t *cell=g.map.v;
  int row=0; for (;row<ROWC;row++) {
    int col=0; for (;col<COLC;col++,cell++) {
      int ph=tilesheet[*cell];
      if (!ph) continue;
      ph=1<<ph;
      // Combine with an existing wall if possible.
      // This can dramatically reduce the amount of collision testing we need once running.
      // We're advancing LRTB, so an existing wall can only be up or left of us.
      struct wall *wall=physics_wallv;
      int i=physics_wallc;
      for (;i-->0;wall++) {
        if ((wall->aabb.l==col)&&(wall->aabb.r==col+1.0)&&(wall->aabb.b==row)&&(wall->physics==ph)) {
          wall->aabb.b+=1.0;
          goto _done_adding_wall_;
        }
        if ((wall->aabb.t==row)&&(wall->aabb.b==row+1.0)&&(wall->aabb.r==col)&&(wall->physics==ph)) {
          wall->aabb.r+=1.0;
          goto _done_adding_wall_;
        }
      }
      wall=physics_wallv+physics_wallc++;
      wall->aabb.l=col;
      wall->aabb.t=row;
      wall->aabb.r=col+1.0;
      wall->aabb.b=row+1.0;
      wall->physics=ph;
     _done_adding_wall_:;
    }
  }
  // Any wall touching the screen's edge, extend 100 meters offscreen to be safe.
  const double right=COLC-0.5,bottom=ROWC-0.5;
  struct wall *wall=physics_wallv;
  int i=physics_wallc;
  for (;i-->0;wall++) {
    if (wall->aabb.l<0.5) wall->aabb.l-=100.0;
    if (wall->aabb.t<0.5) wall->aabb.t-=100.0;
    if (wall->aabb.r>right) wall->aabb.r+=100.0;
    if (wall->aabb.b>bottom) wall->aabb.b+=100.0;
  }
}

/* Refresh (sprite->aabb). Must do this whenever we change (x,y), and also at the very start.
 */
 
void physics_refresh_aabb(struct sprite *sprite) {
  sprite->aabb.l=sprite->x-sprite->hbl;
  sprite->aabb.t=sprite->y-sprite->hbu;
  sprite->aabb.r=sprite->x+sprite->hbr;
  sprite->aabb.b=sprite->y+sprite->hbd;
}

/* Record a collision if there's room for it.
 */
 
static void physics_add_collision(struct sprite *a,struct sprite *b,uint8_t dir) {
  if (physics_collisionc>=COLLISION_LIMIT) return;
  struct collision *collision=physics_collisionv+physics_collisionc++;
  collision->a=a;
  collision->b=b;
  collision->dir=dir;
}

/* Resolve any collisions against static bodies and reset all (phconstrain).
 */
 
static void physics_resolve_static(struct sprgrp *sprgrp) {
  physics_collisionc=0;
  struct sprite **p=sprgrp->sprv;
  int i=sprgrp->sprc;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    
    // Reset sprite's physics fields.
    sprite->phconstrain=0;
   _start_over_:;
    physics_refresh_aabb(sprite);
    if (!sprite->mapsolids) continue;
    
    // Identify the walls we're currently colliding with.
    // Sprites should generally be smaller than cells, so there shouldn't be more than 4 walls.
    // But allow more just in case.
    struct aabb wallv[16];
    int wallc=0;
    const struct wall *wall=physics_wallv;
    int wi=physics_wallc;
    for (;wi-->0;wall++) {
      // "Greater or less": Not touching at all, skip this wall.
      if (wall->aabb.l>sprite->aabb.r) continue;
      if (wall->aabb.r<sprite->aabb.l) continue;
      if (wall->aabb.t>sprite->aabb.b) continue;
      if (wall->aabb.b<sprite->aabb.t) continue;
      if (!(sprite->mapsolids&wall->physics)) continue;
      // "Equal": Not touching, but do register a constraint.
      if (wall->aabb.l==sprite->aabb.r) { sprite->phconstrain|=DIR_E; continue; }
      if (wall->aabb.r==sprite->aabb.l) { sprite->phconstrain|=DIR_W; continue; }
      if (wall->aabb.t==sprite->aabb.b) { sprite->phconstrain|=DIR_S; continue; }
      if (wall->aabb.b==sprite->aabb.t) { sprite->phconstrain|=DIR_N; continue; }
      // OK, actually touching.
      wallv[wallc++]=wall->aabb;
      if (wallc>=16) break;
    }
    if (!wallc) continue;
    
    // If there's just one wall, choose the shortest escapement. Easy.
    if (wallc==1) {
      double escl=sprite->aabb.r-wallv[0].l;
      double esct=sprite->aabb.b-wallv[0].t;
      double escr=wallv[0].r-sprite->aabb.l;
      double escb=wallv[0].b-sprite->aabb.t;
      if ((escl<=esct)&&(escl<=escr)&&(escl<=escb)) {
        sprite->x=wallv[0].l-sprite->hbr;
        sprite->phconstrain|=DIR_E;
        physics_add_collision(sprite,0,DIR_E);
      } else if ((esct<=escr)&&(esct<=escb)) {
        sprite->y=wallv[0].t-sprite->hbd;
        sprite->phconstrain|=DIR_S;
        physics_add_collision(sprite,0,DIR_S);
      } else if (escr<=escb) {
        sprite->x=wallv[0].r+sprite->hbl;
        sprite->phconstrain|=DIR_W;
        physics_add_collision(sprite,0,DIR_W);
      } else {
        sprite->y=wallv[0].b+sprite->hbu;
        sprite->phconstrain|=DIR_N;
        physics_add_collision(sprite,0,DIR_N);
      }
      physics_refresh_aabb(sprite);
    } else {
      // Determine the combined escapement, ie how far to move on one axis to escape all collisions.
      double escl=0.0,esct=0.0,escr=0.0,escb=0.0;
      const struct aabb *w=wallv;
      for (wi=wallc;wi-->0;w++) {
        double escl1=sprite->aabb.r-w->l;
        double esct1=sprite->aabb.b-w->t;
        double escr1=w->r-sprite->aabb.l;
        double escb1=w->b-sprite->aabb.t;
        if (escl1>escl) escl=escl1;
        if (esct1>esct) esct=esct1;
        if (escr1>escr) escr=escr1;
        if (escb1>escb) escb=escb1;
      }
      double dx=0.0,dy=0.0,mag=0.0;
      int constrain=0;
      if ((escl<=esct)&&(escl<=escr)&&(escl<=escb)) { dx=-1.0; dy=0.0; mag=escl; constrain=DIR_E; }
      else if ((esct<=escr)&&(esct<=escb)) { dy=-1.0; mag=esct; constrain=DIR_S; }
      else if (escr<=escb) { dx=1.0; mag=escr; constrain=DIR_W; }
      else { dy=1.0; mag=escb; constrain=DIR_N; }
      // If the combined escapement is less than the sprite's smaller axis, use it.
      if ((mag<=sprite->aabb.r-sprite->aabb.l)&&(mag<=sprite->aabb.b-sprite->aabb.t)) {
        sprite->x+=dx*mag;
        sprite->y+=dy*mag;
        sprite->phconstrain|=constrain;
        physics_refresh_aabb(sprite);
        physics_add_collision(sprite,0,constrain);
      } else {
        // This case usually arises when you walk into a concave corner.
        // I don't know how to solve it.
        // Best I can think of is resolve any involved wall, then start again.
        // For the corner case, that's the right thing to do anyway.
        double escl=sprite->aabb.r-wallv[0].l;
        double esct=sprite->aabb.b-wallv[0].t;
        double escr=wallv[0].r-sprite->aabb.l;
        double escb=wallv[0].b-sprite->aabb.t;
        if ((escl<=esct)&&(escl<=escr)&&(escl<=escb)) {
          sprite->x=wallv[0].l-sprite->hbr;
          sprite->phconstrain|=DIR_E;
          physics_add_collision(sprite,0,DIR_E);
        } else if ((esct<=escr)&&(esct<=escb)) {
          sprite->y=wallv[0].t-sprite->hbd;
          sprite->phconstrain|=DIR_S;
          physics_add_collision(sprite,0,DIR_S);
        } else if (escr<=escb) {
          sprite->x=wallv[0].r+sprite->hbl;
          sprite->phconstrain|=DIR_W;
          physics_add_collision(sprite,0,DIR_W);
        } else {
          sprite->y=wallv[0].b+sprite->hbu;
          sprite->phconstrain|=DIR_N;
          physics_add_collision(sprite,0,DIR_N);
        }
        physics_refresh_aabb(sprite);
        goto _start_over_;
      }
    }
  }
}

/* Detect and resolve sprite-on-sprite collisions.
 * We only resolve individual collisions. It is possible for the overall set to remain in a conflicted state.
 * But we do take pains to avoid static collisions, that's what (sprite->phconstrain) is for.
 */
 
static void physics_resolve_sprites(struct sprgrp *sprgrp) {
  int ai=sprgrp->sprc; while (ai-->1) {
    struct sprite *a=sprgrp->sprv[ai];
    int bi=ai; while (bi-->0) {
      struct sprite *b=sprgrp->sprv[bi];
      
      // If both sprites have infinite mass, we can't move either, so no sense even checking.
      if (!a->invmass&&!b->invmass) continue;
      
      /* Detect collision and measure escapement (labelled by the direction (a) would move).
       */
      double escl=a->aabb.r-b->aabb.l; if (escl<=0.0) continue;
      double escr=b->aabb.r-a->aabb.l; if (escr<=0.0) continue;
      double esct=a->aabb.b-b->aabb.t; if (esct<=0.0) continue;
      double escb=b->aabb.b-a->aabb.t; if (escb<=0.0) continue;
      
      /* If both sprites are constrained in opposite directions, poison the escapements for that axis.
       * This is an unusual case, think two fat guys passing in a narrow corridor.
       * One would expect it to pick the opposite axis anyway, just being extra careful.
       */
      if ((a->phconstrain&DIR_W)&&(b->phconstrain&DIR_E)) escl=escr=999.0;
      if ((a->phconstrain&DIR_E)&&(b->phconstrain&DIR_W)) escl=escr=999.0;
      if ((a->phconstrain&DIR_N)&&(b->phconstrain&DIR_S)) esct=escb=999.0;
      if ((a->phconstrain&DIR_S)&&(b->phconstrain&DIR_N)) esct=escb=999.0;
      
      /* Select the shortest escapement, and initially allocate all of it to (a).
       */
      int abit,bbit;
      double adx=0.0,ady=0.0;
      if ((escl<=escr)&&(escl<=esct)&&(escl<=escb)) { adx=-escl; abit=DIR_W; bbit=DIR_E; }
      else if ((escr<=esct)&&(escr<=escb)) { adx=escr; abit=DIR_E; bbit=DIR_W; }
      else if (esct<=escb) { ady=-esct; abit=DIR_N; bbit=DIR_S; }
      else { ady=escb; abit=DIR_S; bbit=DIR_N; }
      
      /* Record the collision for reporting later.
       * Note that (abit,bbit) are the direction of travel, and we're recording (a)'s direction of impact -- use (bbit).
       */
      physics_add_collision(a,b,bbit);
      
      /* If either sprite is constrained, have the other do the full escape.
       * Otherwise, allocate proportionately to inverse mass.
       */
      double bdx=0.0,bdy=0.0;
      if (a->phconstrain&abit) {
        if (b->phconstrain&bbit) continue; // oh no! We selected a constrained axis despite the poisoning. Don't touch this mess.
        bdx=-adx; adx=0.0;
        bdy=-ady; ady=0.0;
      } else if (b->phconstrain&bbit) {
        // (b) constrained. We've already allocated the full escape to (a), great.
      } else if (!a->invmass) {
        bdx=-adx; adx=0.0;
        bdy=-ady; ady=0.0;
      } else if (!b->invmass) {
        // Keep all in (a).
      } else {
        double total=a->invmass+b->invmass;
        double aprop=a->invmass/total;
        double bprop=-(b->invmass/total);
        bdx=bprop*adx; adx*=aprop;
        bdy=bprop*ady; ady*=aprop;
      }
      a->x+=adx;
      a->y+=ady;
      b->x+=bdx;
      b->y+=bdy;
      physics_refresh_aabb(a);
      physics_refresh_aabb(b);
    }
  }
}

/* Wrap up: Set previous position to current, trigger collision callbacks.
 */
 
static void physics_finalize(struct sprgrp *sprgrp) {
  struct sprite **p=sprgrp->sprv;
  int i=sprgrp->sprc;
  for (;i-->0;p++) {
    struct sprite *sprite=*p;
    sprite->pvx=sprite->x;
    sprite->pvy=sprite->y;
  }
  struct collision *coll=physics_collisionv;
  for (i=physics_collisionc;i-->0;coll++) {
    if (coll->a->sprctl&&coll->a->sprctl->collision) {
      coll->a->sprctl->collision(coll->a,coll->b,coll->dir);
    }
    if (coll->b&&coll->b->sprctl&&coll->b->sprctl->collision) {
      coll->b->sprctl->collision(coll->b,coll->a,dir_reverse(coll->dir));
    }
  }
}

/* Update sprite physics, main entry point.
 */
 
void physics_update(struct sprgrp *sprgrp,double elapsed) {
  physics_resolve_static(sprgrp);
  physics_resolve_sprites(sprgrp);
  physics_finalize(sprgrp);
}
