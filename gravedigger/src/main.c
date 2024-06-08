#include <egg/egg.h>
#include <egg/hid_keycode.h>
#include "gravedigger.h"

static uint32_t fb[SCREENW*SCREENH]; // Redrawn each frame.
static uint32_t terrain[SCREENW*SCREENH]; // Background image, changes only on user actions.
static uint32_t spritebits[SPRITESW*SPRITESH];

static double herox=SCREENW/2.0; // center
static double heroy=SCREENH/2.0; // bottom
static int herodx=0;
static int herofacex=1;
static int heroframe=0;
static double heroclock=0.0;
static int herocarrying=0;
static int heroseated=1;
static int heroeffort=0; // Nonzero if we ascended on the last frame; slow down the next motion.
static int herojump=0;
static double herojumppower=0.0;
static double herogravity=0.0;
static double herocoyote=0.0; // Counts down after unseating, a brief interval when jumping is still allowed.
static double herodigclock=0.0; // Counts down during dig animation; no other activity permitted until it reaches zero.

void egg_client_quit() {
}

/* Init.
 **************************************************************************/

int egg_client_init() {

  { // Load spritesheet.
    int w=0,h=0,stride=0,fmt=0;
    egg_image_get_header(&w,&h,&stride,&fmt,0,2);
    if ((w!=SPRITESW)||(h!=SPRITESH)||(stride!=SPRITESW<<2)||(fmt!=EGG_TEX_FMT_RGBA)) {
      egg_log("image:0:2 must be %dx%d RGBA, found %dx%d fmt=%d",SPRITESW,SPRITESH,w,h,fmt);
      return -1;
    }
    if (egg_image_decode(spritebits,sizeof(spritebits),0,2)!=sizeof(spritebits)) {
      egg_log("Failed to decode image:0:2");
      return -1;
    }
  }
  
  // Reset game state.
  redraw_terrain(terrain,SCREENH>>1);
  herox=SCREENW/2.0;
  heroy=SCREENH/2.0;
  herofacex=1;
  herodx=0;
  heroframe=0;
  heroclock=0.0;
  herocarrying=0;
  heroseated=1;
  heroeffort=0;
  herojump=0;
  herojumppower=0.0;
  herogravity=0.0;
  herodigclock=0.0;
  
  return 0;
}

/* Shovel: Find some dirt we can move and commit the change immediately in (terrain).
 ************************************************************************/

/* Names all of the candidate positions to dig dirt from, relative to the hero.
 * These are phrased for facing right, ie (dx>0). Multiply by (dx).
 */
static const struct digzoned {
  int8_t dx;
  int8_t dy;
} digzone[]={
                  { 1,-3},{ 2,-3},
                  { 1,-2},{ 2,-2},{ 3,-2},
  {-1,-1},{ 0,-1},{ 1,-1},{ 2,-1},{ 3,-1},{ 4,-1},
  {-1, 0},{ 0, 0},{ 1, 0},{ 2, 0},{ 3, 0},{ 4, 0},
  {-1, 1},{ 0, 1},{ 1, 1},{ 2, 1},{ 3, 1},{ 4, 1},
  {-1, 2},{ 0, 2},{ 1, 2},{ 2, 2},{ 3, 2},{ 4, 2},
  {-1, 3},{ 0, 3},{ 1, 3},{ 2, 3},{ 3, 3},
          { 0, 4},{ 1, 4},{ 2, 4},
};

static void sort_terrain_column(int x) {
  uint32_t *v=terrain+x;
  
  int lowest_sky=0;
  int y=SCREENH-1;
  uint32_t *p=v+y*SCREENW;
  for (;y>=0;y--,p-=SCREENW) {
    if (!COLOR_IS_DIRT(*p)) {
      lowest_sky=y;
      break;
    }
  }
  if (!lowest_sky) return;

  for (y=0,p=v;y<lowest_sky;y++,p+=SCREENW) {
    if (COLOR_IS_DIRT(*p)) {
      *p=COLOR_SKY;
      v[lowest_sky*SCREENW]=COLOR_DIRT;
      lowest_sky--;
      while ((lowest_sky>0)&&COLOR_IS_DIRT(v[lowest_sky*SCREENW])) lowest_sky--;
      if (!lowest_sky) return;
    }
  }
}
 
static void choose_and_move_dirt(int x,int y,int dx) {
  if (x<0) x=0; else if (x>=SCREENW) x=SCREENW-1;
  if (y<0) y=0; else if (y>=SCREENH) y=SCREENH-1;
  
  /* Determine how many pixels we are able to dig up.
   */
  int dirtc=0;
  int digzonec=sizeof(digzone)/sizeof(digzone[0]);
  const struct digzoned *d=digzone;
  int i=digzonec;
  for (;i-->0;d++) {
    int dirtx=x+d->dx*dx;
    if ((dirtx<0)||(dirtx>=SCREENW)) continue;
    int dirty=y+d->dy;
    if ((dirty<0)||(dirty>=SCREENH)) continue;
    if (COLOR_IS_DIRT(terrain[dirty*SCREENW+dirtx])) dirtc++;
  }
  if (!dirtc) return; // No dirt here (must be standing on the bottom of screen?)
  
  /* Dirt is deposited in a fixed count of columns, two columns backward from (x), starting two rows upward of (y).
   * Measure the available space in each of those four columns.
   * If there's a vacancy at the hero's feet, but the upper end of the column is occupied, we do NOT fill it. (TODO will that be a problem?)
   * You can't dig with your back to the screen edge.
   */
  int columnv[8]={0};
  int columnc=sizeof(columnv)/sizeof(columnv[0]);
  int columny=y-8; if (columny<0) columny=0;
  int columnh=SCREENH-columny;
  int columnx=x+dx*-2;
  int columni=0;
  int vacantc=0;
  for (;columni<columnc;columni++,columnx-=dx) {
    if ((columnx<0)||(columnx>=SCREENW)) continue;
    columnv[columni]=find_highest_dirt(terrain,columnx,columny,1,columnh);
    vacantc+=columnv[columni];
  }
  if (!vacantc) return; // Nowhere to put the dirt.
  if (vacantc<dirtc) dirtc=vacantc; // Dig up only what we have a new home for.
  
  /* Run over the candidates again, until (dirtc) hits zero.
   * Each dirt pixel we find, replace it with sky, then add a dirt at the bottom of the deepest column.
   */
  for (i=digzonec,d=digzone;(i-->0)&&dirtc;d++) {
    int dirtx=x+d->dx*dx;
    if ((dirtx<0)||(dirtx>=SCREENW)) continue;
    int dirty=y+d->dy;
    if ((dirty<0)||(dirty>=SCREENH)) continue;
    if (!COLOR_IS_DIRT(terrain[dirty*SCREENW+dirtx])) continue;
    dirtc--;
    terrain[dirty*SCREENW+dirtx]=COLOR_SKY;
    int dstcol=0;
    for (columni=columnc;columni-->0;) {
      if (columnv[columni]>=columnv[dstcol]) dstcol=columni;
    }
    if (columnv[dstcol]<1) break; // shouldn't be possible but let's stay safe.
    columnv[dstcol]--;
    int dstx=x-(2+dstcol)*dx;
    terrain[(columny+columnv[dstcol])*SCREENW+dstx]=COLOR_DIRT;
  }
  
  /* Check for floating dirt in the dig zone.
   * As a firm rule, we will never allow sky below dirt.
   * A little heavy-handed about this; we'll examine entire columns of the framebuffer, anywhere it might have been impacted.
   * And actually overshoot a little to be on the safe side. Whatever.
   */
  int chkxlo=x-8,chkxhi=x+8;
  if (chkxlo<0) chkxlo=0;
  if (chkxhi>=SCREENW) chkxhi=SCREENW-1;
  if (chkxlo<=chkxhi) {
    int chkx=chkxlo;
    for (;chkx<=chkxhi;chkx++) {
      sort_terrain_column(chkx);
    }
  }
}

/* Hero.
 ***********************************************************************/
 
static void hero_render() {
  int dstx=(int)herox-3;
  int dsty=(int)heroy-11;
  int srcx=heroframe*7;
  if (herocarrying) srcx+=28;
  else if (herodigclock>=0.200) srcx=56;
  else if (herodigclock>0.0) srcx=63;
  blit_sprite(fb,dstx,dsty,spritebits,srcx,0,7,11,herofacex<0);
  if (herocarrying) {
    blit_sprite(fb,dstx-2,dsty-4,spritebits,0,11,11,4,0);
  }
}

// Set or clear (heroseated) according to current position.
// With (bubble_up), allow that he might be buried, and force (heroy) higher to escape it.
static void hero_check_seated(int bubble_up) {
  int x=(int)herox;
  int y=(int)heroy;
  if (x<0) x=0; else if (x>=SCREENW) x=SCREENW-1;
  if (y<0) y=0; else if (y>=SCREENH) y=SCREENH;
  if ((y>=SCREENH)||COLOR_IS_DIRT(terrain[y*SCREENW+x])) {
    if (bubble_up) {
      while ((y>0)&&COLOR_IS_DIRT(terrain[(y-1)*SCREENW+x])) {
        y--;
        heroy-=1.0;
      }
    }
    if (!heroseated) {
      while ((y>0)&&COLOR_IS_DIRT(terrain[(y-1)*SCREENW+x])) y--;
      heroseated=1;
      heroy=y;
      herogravity=0.0;
    }
  } else if (heroseated) {
    heroseated=0;
    herocoyote=0.080;
  }
}

// If we've walked into dirt, try stepping up. If that fails, return zero to request backing out the move.
static int hero_adjust_after_walk() {
  int x=(int)herox;
  int y=(int)heroy-11;
  if (x<0) x=0; else if (x>=SCREENW) x=SCREENW-1;
  if (y<0) y=0; else if (y>=SCREENH) y=SCREENH-1;
  int depth=find_highest_dirt(terrain,x,y,1,11);
  if (depth>=11) return 1; // Free and clear.
  if (depth>=8) { // Allow walking up gentle slopes.
    heroy=y+depth;
    heroeffort=1;
    return 1;
  }
  // Too much dirt here. Reject the move.
  return 0;
}
 
static void hero_update(double elapsed) {

  if ((herocoyote-=elapsed)<=0.0) herocoyote=0.0;
  if ((herodigclock-=elapsed)<=0.0) herodigclock=0.0;

  if (herodx&&(herodigclock<=0.0)) {
    if ((heroclock-=elapsed)<=0) {
      heroclock+=0.125;
      if (++heroframe>=4) heroframe=0;
    }
    double walkspeed=60.0; // px/s
    if (heroeffort) {
      walkspeed*=0.5;
      heroeffort=0;
    }
    double herox0=herox;
    herox+=walkspeed*elapsed*herodx;
    if (herox<0.0) herox=0.0;
    else if (herox>SCREENW) herox=SCREENW;
    if (!hero_adjust_after_walk()) herox=herox0;
    hero_check_seated(0);
  }

  if (herojump) {
    herojumppower-=400.0*elapsed;
    if (herojumppower<10.0) {
      herojump=0;
    } else {
      heroy-=herojumppower*elapsed;
    }
    hero_check_seated(0);

  } else if (!heroseated) {
    herogravity+=300.0*elapsed;
    if (herogravity>200.0) herogravity=200.0;
    heroy+=herogravity*elapsed;
    if (heroy>=SCREENH) heroy=SCREENH;
    hero_check_seated(0);
  }
}

static void hero_walk_begin(int dx) {
  herodx=dx;
  herofacex=dx;
}

static void hero_walk_end() {
  herodx=0;
  heroframe=0;
  heroclock=0.0;
}

static void hero_jump_begin() {
  if (!heroseated&&(herocoyote<=0.0)) return;
  if (herodigclock>0.0) return;
  herojump=1;
  herojumppower=150.0;
  herogravity=0.0;
  herocoyote=0.0;
}

static void hero_jump_end() {
  if (!herojump) return;
  herojump=0;
  hero_check_seated(0);
}

static void hero_toggle_carry() {
  herocarrying^=1;//TODO
}

static void hero_dig() {
  if (herocarrying) return;
  if (!heroseated) return;
  if (herojump) return;
  herodigclock=0.400;
  int x=(int)herox;
  int y=(int)heroy;
  choose_and_move_dirt(x,y,herofacex);
  hero_check_seated(1);
}

/* Event dispatch.
 *****************************************************************************/
 
static void on_key(int keycode,int value) {
  if (value>1) return;
  switch (keycode) {
    case KEY_ESCAPE: if (value) egg_request_termination(); break;
    case KEY_LEFT: if (value) hero_walk_begin(-1); else if (herodx<0) hero_walk_end(); break;
    case KEY_RIGHT: if (value) hero_walk_begin(1); else if (herodx>0) hero_walk_end(); break;
    case KEY_DOWN: if (value) hero_toggle_carry(); break;
    case KEY_Z: if (value) hero_jump_begin(); else hero_jump_end(); break;
    case KEY_X: if (value) hero_dig(); break;
  }
}

static void on_joy(int btnid,int value) {
  switch (btnid) {
    case EGG_JOYBTN_LEFT: if (value) hero_walk_begin(-1); else if (herodx<0) hero_walk_end(); break;
    case EGG_JOYBTN_RIGHT: if (value) hero_walk_begin(1); else if (herodx>0) hero_walk_end(); break;
    case EGG_JOYBTN_DOWN: if (value) hero_toggle_carry(); break;
    case EGG_JOYBTN_SOUTH: if (value) hero_jump_begin(); else hero_jump_end(); break;
    case EGG_JOYBTN_WEST: if (value) hero_dig(); break;
  }
}

void egg_client_update(double elapsed) {
  union egg_event eventv[16];
  int eventc;
  while ((eventc=egg_event_get(eventv,16))>0) {
    const union egg_event *event=eventv;
    int i=eventc;
    for (;i-->0;event++) switch (event->type) {
      case EGG_EVENT_KEY: on_key(event->key.keycode,event->key.value); break;
      case EGG_EVENT_JOY: on_joy(event->joy.btnid,event->joy.value); break;
    }
    if (eventc<16) break;
  }
  hero_update(elapsed);
}

/* Render.
 ************************************************************************/

void egg_client_render() {
  copy_framebuffer(fb,terrain);
  hero_render();
  //TODO sprites
  //TODO overlay
  // Commit to main output.
  egg_texture_upload(1,SCREENW,SCREENH,SCREENW<<2,EGG_TEX_FMT_RGBA,fb,sizeof(fb));
}
