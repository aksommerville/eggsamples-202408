/* stars.c
 * Draw a cool background of moving stars.
 */

#include "shmup.h"

#define STAR_COUNT 50

static struct star {
  int x,y;
  int dy;
  int rgba;
} starv[STAR_COUNT]={0};

void stars_init() {
  struct star *star=starv;
  int i=STAR_COUNT;
  for (;i-->0;star++) {
    star->x=rand()&0x7f;
    star->y=rand()&0x7f;
    switch (star->dy=1+rand()%3) {
      case 1: star->rgba=0x101010ff; break;
      case 2: star->rgba=0x202020ff; break;
      case 3: star->rgba=0x303030ff; break;
    }
  }
}

void stars_render() {
  int r=(int)(alarm_clock*500);
  if (r>0xff) r=0xff;
  else if (r<0) r=0;
  egg_draw_rect(1,0,0,128,128,(r<<24)|0xff);
  struct star *star=starv;
  int i=STAR_COUNT;
  for (;i-->0;star++) {
    star->y+=star->dy;
    if (star->y>=128) {
      star->x=rand()&0x7f;
      star->y=0;
      switch (star->dy=1+rand()%3) {
        case 1: star->rgba=0x101010ff; break;
        case 2: star->rgba=0x202020ff; break;
        case 3: star->rgba=0x303030ff; break;
      }
    }
    egg_draw_rect(1,star->x,star->y,1,star->dy+1,star->rgba);
  }
}
