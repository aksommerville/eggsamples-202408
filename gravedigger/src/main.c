#include <egg/hid_keycode.h>
#include "gravedigger.h"

struct globals g={0};

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
    if (egg_image_decode(g.spritebits,sizeof(g.spritebits),0,2)!=sizeof(g.spritebits)) {
      egg_log("Failed to decode image:0:2");
      return -1;
    }
  }
  
  // Reset game state.
  redraw_terrain(g.terrain,SCREENH>>1);
  g.coffinc=generate_coffins(g.coffinv,COFFIN_LIMIT);
  hero_reset(&g.hero);
  g.dayclock=DAY_LENGTH;
  g.vampire_flop_clock=0;
  
  return 0;
}

/* Repair terrain after digging or picking up a coffin.
 *****************************************************************************/

static void sort_terrain_column(int x) {
  uint32_t *v=g.terrain+x;
  
  int lowest_sky=0;
  int y=SCREENH-1;
  uint32_t *p=v+y*SCREENW;
  for (;y>=0;y--,p-=SCREENW) {
    if (!COLOR_IS_DIRT(*p)&&!test_coffin_pixel(x,y)) {
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
      while ((lowest_sky>0)&&COLOR_IS_DIRT(v[lowest_sky*SCREENW])&&!test_coffin_pixel(x,lowest_sky)) lowest_sky--;
      if (!lowest_sky) return;
    }
  }
}

void repair_terrain(int x,int w) {
  if (x<0) { w+=x; x=0; }
  if (x>SCREENW-w) w=SCREENW-x;
  if (w<1) return;
  
  /* Check for floating dirt in the dig zone.
   * As a firm rule, we will never allow sky below dirt. -- Except if there's a coffin; terrain behind a coffin is sky, not dirt.
   * A little heavy-handed about this; we'll examine entire columns of the framebuffer, anywhere it might have been impacted.
   * And actually overshoot a little to be on the safe side. Whatever.
   */
  int chkx=x;
  int xi=w;
  for (;xi-->0;chkx++) {
    sort_terrain_column(chkx);
  }
  
  coffins_force_valid(g.coffinv,g.coffinc);
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
 
void choose_and_move_dirt(int x,int y,int dx) {
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
    if (COLOR_IS_DIRT(g.terrain[dirty*SCREENW+dirtx])) dirtc++;
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
    columnv[columni]=find_highest_dirt(g.terrain,columnx,columny,1,columnh);
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
    if (!COLOR_IS_DIRT(g.terrain[dirty*SCREENW+dirtx])) continue;
    dirtc--;
    g.terrain[dirty*SCREENW+dirtx]=COLOR_SKY;
    int dstcol=0;
    for (columni=columnc;columni-->0;) {
      if (columnv[columni]>=columnv[dstcol]) dstcol=columni;
    }
    if (columnv[dstcol]<1) break; // shouldn't be possible but let's stay safe.
    columnv[dstcol]--;
    int dstx=x-(2+dstcol)*dx;
    g.terrain[(columny+columnv[dstcol])*SCREENW+dstx]=COLOR_DIRT;
  }
  
  repair_terrain(x-8,16);
}

/* Event dispatch.
 *****************************************************************************/
 
static void on_key(int keycode,int value) {
  if (value>1) return;
  // Always available:
  switch (keycode) {
    case KEY_ESCAPE: if (value) egg_request_termination(); break;
  }
  // Only during play:
  if (g.dayclock>0.0) switch (keycode) {
    case KEY_LEFT: if (value) hero_walk_begin(&g.hero,-1); else if (g.hero.dx<0) hero_walk_end(&g.hero); break;
    case KEY_RIGHT: if (value) hero_walk_begin(&g.hero,1); else if (g.hero.dx>0) hero_walk_end(&g.hero); break;
    case KEY_DOWN: if (value) hero_toggle_carry(&g.hero); break;
    case KEY_Z: if (value) hero_jump_begin(&g.hero); else hero_jump_end(&g.hero); break;
    case KEY_X: if (value) hero_dig(&g.hero); break;
  }
}

static void on_joy(int btnid,int value) {
  if (g.dayclock>0.0) switch (btnid) {
    case EGG_JOYBTN_LEFT: if (value) hero_walk_begin(&g.hero,-1); else if (g.hero.dx<0) hero_walk_end(&g.hero); break;
    case EGG_JOYBTN_RIGHT: if (value) hero_walk_begin(&g.hero,1); else if (g.hero.dx>0) hero_walk_end(&g.hero); break;
    case EGG_JOYBTN_DOWN: if (value) hero_toggle_carry(&g.hero); break;
    case EGG_JOYBTN_SOUTH: if (value) hero_jump_begin(&g.hero); else hero_jump_end(&g.hero); break;
    case EGG_JOYBTN_WEST: if (value) hero_dig(&g.hero); break;
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
  if (g.dayclock>0.0) {
    if ((g.dayclock-=elapsed)<=0.0) {
      g.dayclock=0.0;
      hero_jump_end(&g.hero);
      hero_walk_end(&g.hero);
      hero_update_noninteractive(&g.hero,elapsed);
    } else {
      // Normal game update.
      hero_update(&g.hero,elapsed);
    }
  } else {
    hero_update_noninteractive(&g.hero,elapsed);
  }
}

/* Render.
 ************************************************************************/

void egg_client_render() {
  copy_framebuffer(g.fb,g.terrain);
  hero_render(g.fb,&g.hero);
  coffins_render(g.fb,g.coffinv,g.coffinc);
  
  // Sun starts centered in the top left corner, and ends just below the bottom left corner.
  const int sunw=5;
  const int sunh=9;
  const int sunpath_range=SCREENH+((sunh+1)>>1);
  int sunp=(g.dayclock*sunpath_range)/DAY_LENGTH;
  if (sunp<0) sunp=0;
  else if (sunp>sunpath_range) sunp=sunpath_range;
  int suny=SCREENH-sunp;
  blit_sprite(g.fb,0,suny,g.spritebits,22,11,sunw,sunh,0);
  
  if (g.dayclock<=0.0) {
    blit_sprite(g.fb,(SCREENW>>1)-21,10,g.spritebits,27,11,43,10,0);
  }
  
  egg_texture_upload(1,SCREENW,SCREENH,SCREENW<<2,EGG_TEX_FMT_RGBA,g.fb,sizeof(g.fb));
}
