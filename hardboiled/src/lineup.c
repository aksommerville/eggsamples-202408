#include "hardboiled.h"

/* Globals.
 * By rights, these ought to live in struct room, but whatever, we know there's just one room at a time.
 */
 
#define LINEUP_WIDTH (SUSPECT_COUNT*48)
#define SCROLL_LIMIT (LINEUP_WIDTH-96)
 
static int eyes1=0,eyes2=0;
static int eyesclock=0;
static int scroll=0; // in framebuffer pixels, 0..SCROLL_LIMIT
static int dscroll=0; // -1,0,1

/* Act.
 */
 
int lineup_act(struct room *room,int verb,int nounid) {
  switch (nounid) {
    case 1: if (scroll>=48) dscroll=-1; return 1;
    case 2: if (scroll<=SCROLL_LIMIT-48) dscroll=1; return 1;
  }
  if (!verb&&!nounid) { // Check whether we've clicked on a suspect.
    // I didn't plan well for this case, where we click on something that can scroll behind the scenes.
    // It's not really appropriate for a room to talk to inkeep, or to know its absolute framebuffer position.
    int x=0,y=0;
    inkeep_get_mouse(&x,&y);
    x-=72;
    y-=3;
    if ((x>0)&&(x<96)&&(y>0)&&(y<70)) {
      int suspectp=(x+scroll)/48;
      if ((suspectp>=0)&&(suspectp<SUSPECT_COUNT)) {
        egg_log("TODO Arrest suspect %d",suspectp);// will require confirmation
      }
    }
  }
  return 0;
}

/* Draw one suspect.
 * (hair,shirt,tie) all in (0..2).
 */
 
static void draw_suspect(struct room *room,int x,int y,int hair,int shirt,int tie,int eyes) {
  
  egg_draw_decal(1,room->texid,x,y+15,96,0,40,58,0);
  if (eyes) {
    egg_draw_decal(1,room->texid,x+14,y+28,110,13,14,3,EGG_XFORM_XREV);
  }
  
  switch (shirt) {
    case 0: egg_render_tint(0x0000ffc0); break;
    case 1: egg_render_tint(0x008000c0); break;
    case 2: egg_render_tint(0xff0000c0); break;
  }
  egg_draw_decal(1,room->texid,x,y+15,136,0,40,58,0);
  egg_render_tint(0);
  
  switch (tie) {
    case 0: break; 
    case 1: egg_draw_decal(1,room->texid,x+17,y+49,176,34,10,23,0); break;
    case 2: egg_draw_decal(1,room->texid,x+17,y+49,176,25,10,7,0); break;
  }
  
  switch (hair) {
    case 0: egg_draw_decal(1,room->texid,x,y+13,96,58,40,17,0); break;
    case 1: egg_draw_decal(1,room->texid,x,y+13,96,75,40,17,0); break;
    case 2: break;
  }
}

/* Render.
 */

void lineup_render(struct room *room,int x,int y,int w,int h) {
  // Far wall is already rendered. And nouns, if we're using those.
  
  if (dscroll) {
    scroll+=dscroll;
    if (!(scroll%48)) dscroll=0;
  }
  
  // Every so often, make one suspect's eyes flip, to make them look sneaky. Purely ornamental.
  if (--eyesclock<=0) {
    if (rand()&1) {
      eyes1^=1;
    } else {
      eyes2^=1;
    }
    eyesclock=80+rand()%100;
  }
  
  int dstx=-scroll%48;
  int suspectp=scroll/48;
  for (;dstx<96;dstx+=48,suspectp++) {
    int hair=0,shirt=0,tie=0;
    puzzle_get_suspect(&hair,&shirt,&tie,suspectp);
    draw_suspect(room,x+dstx,y,hair,shirt,tie,(suspectp&1)?eyes1:eyes2);
  }
  
  egg_draw_decal(1,room->texid,x,y,0,96,96,96,0);
  
  egg_draw_decal(1,room->texid,x+10,y+77,96+((scroll<48)?15:0),92,15,15,0);
  egg_draw_decal(1,room->texid,x+71,y+77,96+((scroll>SCROLL_LIMIT-48)?15:0),92,15,15,EGG_XFORM_XREV);
}
