#include "hardboiled.h"
#include "verblist.h"

/* Globals.
 * By rights, these ought to live in struct room, but whatever, we know there's just one room at a time.
 */
 
#define LINEUP_WIDTH (SUSPECT_COUNT*48)
#define SCROLL_LIMIT (LINEUP_WIDTH-48)
 
static int eyes1=0,eyes2=0;
static int eyesclock=0;
static int scroll=0; // in framebuffer pixels, 0..SCROLL_LIMIT
static int dscroll=0; // -1,0,1
static int lineup_query_suspect=-1; // 0..SUSPECT_COUNT-1, if the confirmation modal is up

/* Confirmation modal responses.
 */
 void log_rand_seed();
 
static void lineup_play_again() {
  scroll=0;
  inv_reset();
  log_clear();
  verblist_init();
  puzzle_init();
  change_room(1);
}
 
static void lineup_confirm_arrest() {
  if ((lineup_query_suspect>=0)&&(lineup_query_suspect<SUSPECT_COUNT)) {
    int guiltyp=puzzle_get_guilty_index();
    if (lineup_query_suspect==guiltyp) {
      popup_present(RID_string_prompt_success,RID_string_opt_again,0,lineup_play_again,0);
    } else {
      popup_present(RID_string_prompt_failure,RID_string_opt_again,0,lineup_play_again,0);
    }
  }
  lineup_query_suspect=-1;
}

static void lineup_cancel_arrest() {
  lineup_query_suspect=-1;
}

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
      int suspectp=scroll/48;
      if ((suspectp>=0)&&(suspectp<SUSPECT_COUNT)) {
        lineup_query_suspect=suspectp;
        popup_present(RID_string_prompt_arrest,RID_string_opt_yes,RID_string_opt_no,lineup_confirm_arrest,lineup_cancel_arrest);
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
    case 0: egg_draw_decal(1,room->texid,x,y+13,96,75,40,17,0); break;
    case 1: egg_draw_decal(1,room->texid,x,y+13,96,58,40,17,0); break;
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
  
  int dstx=-(scroll-24)%48;
  int suspectp=(scroll-24)/48;
  for (;dstx<96;dstx+=48,suspectp++) {
    if (suspectp>=SUSPECT_COUNT) break;
    int hair=0,shirt=0,tie=0;
    puzzle_get_suspect(&hair,&shirt,&tie,suspectp);
    draw_suspect(room,x+dstx,y,hair,shirt,tie,(suspectp&1)?eyes1:eyes2);
  }
  
  egg_draw_decal(1,room->texid,x,y,0,96,96,96,0);
  
  egg_draw_decal(1,room->texid,x+10,y+77,96+((scroll<48)?15:0),92,15,15,0);
  egg_draw_decal(1,room->texid,x+71,y+77,96+((scroll>SCROLL_LIMIT-48)?15:0),92,15,15,EGG_XFORM_XREV);
}
