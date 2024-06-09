#include "gravedigger.h"

/* Generate coffins.
 */
 
int generate_coffins(struct coffin *dst,int dsta) {
  if (dsta<1) return 0;
  // For now, we'll just generate one, in a fixed position.
  dst->x=((SCREENW*3)>>2)-(COFFIN_WIDTH>>1);
  dst->y=0;
  const uint32_t *p=g.terrain+dst->x;
  while (dst->y<SCREENH) {
    if (COLOR_IS_DIRT(*p)) break;
    dst->y++;
    p+=SCREENW;
  }
  dst->y-=COFFIN_HEIGHT;
  if (dst->y<0) dst->y=0;
  return 1;
}

/* Test burial.
 * A coffin is buried if every cardinal neighbor of it is either OOB or dirt.
 */
 
static int any_sky(int x,int y,int w,int h) {
  if (x<0) { w+=x; x=0; }
  if (y<0) { h+=y; y=0; }
  if (x>SCREENW-w) w=SCREENW-x;
  if (y>SCREENH-h) h=SCREENH-y;
  if ((w<1)||(h<1)) return 0; // OOB always counts as dirt.
  const uint32_t *row=g.terrain+y*SCREENW+x;
  int yi=h;
  for (;yi-->0;row+=SCREENW) {
    const uint32_t *p=row;
    int xi=w;
    for (;xi-->0;p++) if (!COLOR_IS_DIRT(*p)) return 1;
  }
  return 0;
}
 
int coffin_is_buried(const struct coffin *coffin) {
  return !(
    any_sky(coffin->x-1,coffin->y,1,COFFIN_HEIGHT)||
    any_sky(coffin->x,coffin->y-1,COFFIN_WIDTH,1)||
    any_sky(coffin->x+COFFIN_WIDTH,coffin->y,1,COFFIN_HEIGHT)||
    any_sky(coffin->x,coffin->y+COFFIN_HEIGHT,COFFIN_WIDTH,1)
  );
}

/* Render.
 */
 
void coffins_render(uint32_t *dst,struct coffin *coffin,int coffinc) {
  if (g.dayclock<=0.0) {
    int flop=0; // Animation ought to be timed during update; I'm getting lazy about it here.
    g.vampire_flop_clock++;
    if (g.vampire_flop_clock>=40) {
      g.vampire_flop_clock=0;
    } else if (g.vampire_flop_clock>=20) {
      flop=1;
    }
    for (;coffinc-->0;coffin++) {
      if (coffin->buried) blit_sprite(dst,coffin->x,coffin->y,g.spritebits,0,11,11,4,0);
      else blit_sprite(dst,coffin->x,coffin->y-6,g.spritebits,11,11,11,10,flop);
    }
  } else if (g.dayclock<10.0) {
    for (;coffinc-->0;coffin++) {
      int srcy=11;
      g.vampire_flop_clock++;
      if (g.vampire_flop_clock>=20) {
        g.vampire_flop_clock=0;
      } else if (g.vampire_flop_clock>=10) {
        if (!coffin_is_buried(coffin)) {
          coffin->buried=0;
          srcy+=4;
        } else {
          coffin->buried=1;
        }
      }
      blit_sprite(dst,coffin->x,coffin->y,g.spritebits,0,srcy,11,4,0);
    }
  } else {
    for (;coffinc-->0;coffin++) {
      blit_sprite(dst,coffin->x,coffin->y,g.spritebits,0,11,11,4,0);
    }
  }
}

/* Force valid.
 */

void coffins_force_valid(struct coffin *coffin,int coffinc) {
  for (;coffinc-->0;coffin++) {
  
    // First, should never come up, but force bounds into range.
    if (coffin->x<0) coffin->x=0;
    else if (coffin->x>SCREENW-COFFIN_WIDTH) coffin->x=SCREENW-COFFIN_WIDTH;
    if (coffin->y<0) coffin->y=0;
    else if (coffin->y>SCREENH-COFFIN_HEIGHT) coffin->y=SCREENH-COFFIN_HEIGHT;
    
    // Find the highest dirt in the box below the coffin -- fall down to it if >0.
    int gap=find_highest_dirt(g.terrain,coffin->x,coffin->y+COFFIN_HEIGHT,COFFIN_WIDTH,SCREENH-COFFIN_HEIGHT-coffin->y);
    if (gap>0) {
      coffin->y+=gap;
      if (coffin->y>SCREENH-COFFIN_HEIGHT) coffin->y=SCREENH-COFFIN_HEIGHT;
    }
    
    // If there's dirt here, move it up. Dirt will not move horizontally.
    uint32_t *trow=g.terrain+coffin->x;
    int xi=COFFIN_WIDTH;
    for (;xi-->0;trow++) {
      uint32_t *p=trow+coffin->y*SCREENW;
      int yi=COFFIN_HEIGHT;
      for (;yi-->0;p+=SCREENW) {
        if (COLOR_IS_DIRT(*p)) {
          *p=COLOR_SKY;
          int dsty=coffin->y-1;
          uint32_t *dst=trow+dsty*SCREENW;
          while ((dsty>=0)&&COLOR_IS_DIRT(*dst)) { dsty--; dst-=SCREENW; }
          if (dsty<0) { // oh no! Let it remain in collision, rather than violating conservation.
            *p=COLOR_DIRT;
          } else {
            *dst=COLOR_DIRT;
          }
        }
      }
    }
  }
}

/* Test coffin coverage at a pixel.
 */

int test_coffin_pixel(int x,int y) {
  const struct coffin *coffin=g.coffinv;
  int i=g.coffinc;
  for (;i-->0;coffin++) {
    if (coffin->x>=x) continue;
    if (coffin->y>=y) continue;
    if (coffin->x+COFFIN_WIDTH<=x) continue;
    if (coffin->y+COFFIN_HEIGHT<=y) continue;
    return 1;
  }
  return 0;
}

/* Same idea, but take an explicit list and return the index.
 */
 
int coffin_find(const struct coffin *coffin,int coffinc,int x,int y) {
  int i=0;
  for (;i<coffinc;i++,coffin++) {
    if (coffin->x>=x) continue;
    if (coffin->y>=y) continue;
    if (coffin->x+COFFIN_WIDTH<=x) continue;
    if (coffin->y+COFFIN_HEIGHT<=y) continue;
    return i;
  }
  return -1;
}

/* Add coffin to global list.
 */

struct coffin *coffin_add() {
  if (g.coffinc>=COFFIN_LIMIT) return 0;
  struct coffin *coffin=g.coffinv+g.coffinc++;
  return coffin;
}

/* Remove coffin from global list.
 */
 
void coffin_remove(int p) {
  if ((p<0)||(p>=g.coffinc)) return;
  g.coffinc--;
  // We don't have memmove(). Could implement it if we want to (see eg hardboiled/src/stdlib), but I think not worth it.
  struct coffin *coffin=g.coffinv+p;
  int i=p;
  for (;i<g.coffinc;i++,coffin++) coffin[0]=coffin[1];
}
