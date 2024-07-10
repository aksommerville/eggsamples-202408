#include "arrautza.h"
#include "inkeep/inkeep.h"
#include <egg/hid_keycode.h>

struct globals g={0};

// Any item that uses the qualifier should be called out here.
struct item_metadata item_metadata[1+ITEM_COUNT]={
  // Accept defaults: RAFT,CLOAK,STOPWATCH,HAMMER,COMPASS
  [ITEM_SWORD]={
    .qual_display=QUAL_DISPLAY_LEVEL,
    .qual_init=1,
    .qual_limit=3,
  },
  [ITEM_SHIELD]={
    .qual_display=QUAL_DISPLAY_LEVEL,
    .qual_init=1,
    .qual_limit=3,
  },
  [ITEM_BOW]={
    .qual_display=QUAL_DISPLAY_COUNT,
    .qual_init=0,
    .qual_limit=99,
  },
  [ITEM_BOMB]={
    .qual_display=QUAL_DISPLAY_COUNT,
    .qual_init=0,
    .qual_limit=99,
  },
  [ITEM_GOLD]={
    .qual_display=QUAL_DISPLAY_COUNT,
    .qual_init=0,
    .qual_limit=99,
  },
};

/* Quit.
 **************************************************************************/

void egg_client_quit() {
  inkeep_quit();
}

/* Input callbacks.
 ***********************************************************************/

/* Pressing AUX1 (Start) summons the PAUSE menu during play, or dismisses it.
 * If any other menu is active, do nothing.
 * Menus and everyone else still sees activity on AUX1 as usual.
 */
static void toggle_pause() {
  if (!g.menuc) {
    menu_push_pause();
  } else if (g.menuv[g.menuc-1]->id==MENU_ID_PAUSE) {
    // Actually don't pop it from here. The menu likes to animate its dismissal.
  }
}

static void cb_joy(int plrid,int btnid,int value,int state,void *userdata) {
  //egg_log("%s %d.0x%04x=%d [0x%04x]",__func__,plrid,btnid,value,state);
  if (!plrid) {
    g.instate=state;
    switch (btnid) {
      case INKEEP_BTNID_AUX1: if (value) toggle_pause(); break;
    }
  }
}

static void cb_raw(const union egg_event *event,void *userdata) {
  switch (event->type) {
    case EGG_EVENT_KEY: if (event->key.value) switch (event->key.keycode) {
        case KEY_ESCAPE: egg_request_termination(); break;
      } break;
  }
}

static void cb_state(int id,int v,void *userdata) {//XXX troubleshooting only
  egg_log("%s:%d:%s: %d=%d",__FILE__,__LINE__,__func__,id,v);
}

static void cb_dialogue(int k,int v,void *userdata) {
  menu_push_dialogue(v);
}

/* Init.
 *************************************************************************/

int egg_client_init() {

  //stobus_listen(&g.stobus,FLD_bombchest1,cb_state,0);//XXX

  // Our hard-coded SCREENW,SCREENH must match the actual main framebuffer.
  int screenw=0,screenh=0;
  egg_texture_get_header(&screenw,&screenh,0,1);
  if ((screenw!=SCREENW)||(screenh!=SCREENH)) {
    egg_log("Expected framebuffer size %dx%d, found %dx%d",SCREENW,SCREENH,screenw,screenh);
    return -1;
  }
  
  if (text_init()<0) return -1;

  if (inkeep_init()<0) return -1;
  inkeep_set_player_count(16);
  inkeep_set_mode(INKEEP_MODE_JOY);
  inkeep_listen_joy(cb_joy,0);
  inkeep_listen_raw(cb_raw,0);
  
  if ((g.texid_tilesheet=egg_texture_new())<0) return -1;
  if (egg_texture_upload(g.texid_transtex=egg_texture_new(),SCREENW,SCREENH,SCREENW<<2,EGG_TEX_FMT_RGBA,0,0)<0) return -1;
  if (egg_texture_load_image(g.texid_spotlight=egg_texture_new(),0,RID_image_spotlight)<0) return -1;
  if (egg_texture_load_image(g.texid_font_tiles=egg_texture_new(),0,RID_image_font_tiles)<0) return -1;
  if (!(g.font=font_new(9))) return -1;
  font_add_page(g.font,RID_image_font_9h_21,0x0021);
  font_add_page(g.font,RID_image_font_9h_a1,0x00a1);
  font_add_page(g.font,RID_image_font_9h_400,0x0400);
  
  sprgrpv_init();
  srand_auto();
  
  /*XXX Start with a bunch of items.
  g.aitem=ITEM_SWORD;
  g.bitem=ITEM_CLOAK;
  g.inventory[0]=ITEM_SHIELD;
  g.inventory[1]=ITEM_BOMB;
  g.inventory[2]=ITEM_BOW;
  g.inventory[3]=ITEM_RAFT;
  g.inventory[4]=ITEM_STOPWATCH;
  g.inventory[5]=ITEM_COMPASS;
  g.inventory[6]=ITEM_HAMMER;
  g.inventory[7]=ITEM_GOLD;
  g.itemqual[ITEM_SWORD]=3;
  g.itemqual[ITEM_SHIELD]=1;
  g.itemqual[ITEM_BOMB]=11;
  g.itemqual[ITEM_GOLD]=22;
  g.itemqual[ITEM_BOW]=0;
  /**/
  
  g.hp=g.hpmax=5;
  g.hp=3;
  if (define_stobus_fields()<0) return -1;
  stobus_listen(&g.stobus,FLD_dialogue,cb_dialogue,0);
  
  if (1) { // XXX During development I prefer to skip the main menu.
    load_map(RID_map_start,-1,-1,TRANSITION_NONE);
    check_map_change();
  } else { // Normal startup: Hello menu
    if (!menu_push_hello()) {
      egg_log("Failed to initialize main menu!");
      return -1;
    }
  }
  
  return 0;
}

/* Update.
 ******************************************************************************/

void egg_client_update(double elapsed) {
  inkeep_update(elapsed);
  
  // When a menu is open, the one on top is basically the only thing active.
  if (g.menuc) {
    struct menu *menu=g.menuv[g.menuc-1];
    if (menu->update) menu->update(menu,elapsed);
    int i=g.menuc-1;
    while (i-->0) {
      menu=g.menuv[i];
      if (menu->update_bg) menu->update_bg(menu,elapsed);
    }
    reap_defunct_menus();
    sprgrp_update(sprgrpv+SPRGRP_UPDATE,elapsed,1);
  
  // No menu, normal game update.
  } else {
    if (g.transclock>0.0) {
      if ((g.transclock-=elapsed)<=0.0) {
        // Transition completed. Probably nothing we need to do.
      }
    }
    sprgrp_update(sprgrpv+SPRGRP_UPDATE,elapsed,0);
    physics_update(sprgrpv+SPRGRP_SOLID,elapsed);
    check_sprites_footing(sprgrpv+SPRGRP_FOOTING);
    check_sprites_heronotify(sprgrpv+SPRGRP_HERONOTIFY,sprgrpv+SPRGRP_HERO);
    // Any non-sprite update stuff goes here.
    sprgrp_kill(sprgrpv+SPRGRP_DEATHROW);
    check_map_change();
  }
}

/* Render.
 ********************************************************************************/

static void render_game_untransitioned() {
  render_map(1);
  sprgrp_render(1,sprgrpv+SPRGRP_RENDER);
}

// The old scene goes at (dstx,dsty), and the new one at (rx,ry) relative to the old.
static void render_pan(int dstx,int dsty,int rx,int ry) {
  egg_draw_decal(1,g.texid_transtex,dstx,dsty,0,0,SCREENW,SCREENH,0);
  g.renderx=dstx+rx;
  g.rendery=dsty+ry;
  render_game_untransitioned();
}

static void render_dissolve(double p) {
  render_game_untransitioned();
  int alpha=(1.0-p)*0xff;
  if (alpha<=0) return;
  if (alpha>0xff) alpha=0xff;
  egg_render_alpha(alpha);
  egg_draw_decal(1,g.texid_transtex,0,0,0,0,SCREENW,SCREENH,0);
  egg_render_alpha(0xff);
}

// (which) is (0,1)=(old,new), (black) in 0..1
static void render_fade(int which,double black) {
  if (which) {
    render_game_untransitioned();
  } else {
    egg_draw_decal(1,g.texid_transtex,0,0,0,0,SCREENW,SCREENH,0);
  }
  int alpha=black*0xff;
  if (alpha<=0) return;
  if (alpha>0xff) alpha=0xff;
  egg_draw_rect(1,0,0,SCREENW,SCREENH,(g.transrgba&0xffffff00)|alpha);
}

// (which) is (0,1)=(old,new), (size) is the normalized width of the spotlight.
static void render_spotlight(int which,double size) {
  // Draw the lit image, and choose a focus point.
  int fx,fy;
  if (which) {
    render_game_untransitioned();
    if (sprgrpv[SPRGRP_HERO].sprc) {
      struct sprite *hero=sprgrpv[SPRGRP_HERO].sprv[0];
      fx=(int)(hero->x*TILESIZE);
      fy=(int)(hero->y*TILESIZE);
    }
  } else {
    egg_draw_decal(1,g.texid_transtex,0,0,0,0,SCREENW,SCREENH,0);
    fx=g.transfx;
    fy=g.transfy;
  }
  // How far do each of the four edges travel?
  //TODO We'll want to cheat these out somewhat, once we do the circular cutout.
  int addx=SCREENW>>2;
  int addy=SCREENH>>2;
  int ld=fx+addx;
  int rd=SCREENW-fx+addx;
  int td=fy+addy;
  int bd=SCREENH-fy+addy;
  if (ld<1) ld=1;
  if (rd<1) rd=1;
  if (td<1) td=1;
  if (bd<1) bd=1;
  // With the total travel computed, now it's easy to find the spotlight's box.
  int l=fx-(int)(ld*size);
  int t=fy-(int)(td*size);
  int r=fx+(int)(rd*size);
  int b=fy+(int)(bd*size);
  // Draw four black rectangles for the bulk of the blackout region.
  egg_draw_rect(1,0,0,l,SCREENH,g.transrgba);
  egg_draw_rect(1,0,0,SCREENW,t,g.transrgba);
  egg_draw_rect(1,r,0,SCREENW,SCREENH,g.transrgba);
  egg_draw_rect(1,0,b,SCREENW,SCREENH,g.transrgba);
  //TODO circular cutout
  int srcw=0,srch=0;
  egg_texture_get_header(&srcw,&srch,0,g.texid_spotlight);
  if ((srcw>0)&&(srch>0)) {
    egg_render_tint(g.transrgba|0xff);
    double xscale=(double)(r+1-l)/(double)srcw;
    double yscale=(double)(b+1-t)/(double)srch;
    egg_draw_decal_mode7(1,g.texid_spotlight,(l+r)>>1,(t+b)>>1,0,0,srcw,srch,0,(int)(xscale*65536.0),(int)(yscale*65536.0));
    egg_render_tint(0);
  }
}

static void render_game_transition(int transition,double p) {
  switch (transition) {
    case TRANSITION_PAN_LEFT: render_pan(SCREENW*p,0,-SCREENW,0); break;
    case TRANSITION_PAN_RIGHT: render_pan(-SCREENW*p,0,SCREENW,0); break;
    case TRANSITION_PAN_UP: render_pan(0,SCREENH*p,0,-SCREENH); break;
    case TRANSITION_PAN_DOWN: render_pan(0,-SCREENH*p,0,SCREENH); break;
    case TRANSITION_DISSOLVE: render_dissolve(p); break;
    case TRANSITION_FADE_BLACK: if (p<0.5) render_fade(0,p*2.0); else render_fade(1,(1-p)*2.0); break;
    case TRANSITION_SPOTLIGHT: if (p<0.5) render_spotlight(0,(0.5-p)*2.0); else render_spotlight(1,(p-0.5)*2.0); break;
    default: render_game_untransitioned();
  }
}

// This is externally linked. menu/menu_pause.c borrows it.
void add_item_tiles(int x,int y,uint8_t itemid) {
  if ((itemid<1)||(itemid>ITEM_COUNT)) return;
  const struct item_metadata *metadata=item_metadata+itemid;
  uint8_t q=g.itemqual[itemid];
  uint8_t tileid=0x90+itemid-1;
  if ((q>1)&&(metadata->qual_display==QUAL_DISPLAY_LEVEL)) { // Change tile per qualifier.
    switch (itemid) {
      case ITEM_SWORD: switch (q) {
          case 2: tileid=0xb0; break;
          case 3: tileid=0xb1; break;
        } break;
      case ITEM_SHIELD: switch (q) {
          case 2: tileid=0xb2; break;
          case 3: tileid=0xb3; break;
        } break;
    }
  }
  tile_renderer_tile(&g.tile_renderer,x,y,tileid,0);
  if (metadata->qual_display==QUAL_DISPLAY_COUNT) {
    uint8_t bgtileid;
    if (q>=metadata->qual_limit) bgtileid=0xad; // green at capacity
    else if (q) bgtileid=0xac; // black if some present
    else bgtileid=0xab; // red if empty
    tile_renderer_tile(&g.tile_renderer,x+3,y+6,bgtileid,0);
    tile_renderer_tile(&g.tile_renderer,x+1,y+6,0xa0+((q/10)%10),0);
    tile_renderer_tile(&g.tile_renderer,x+5,y+6,0xa0+(q%10),0);
  }
}

static void render_overlay() {
  tile_renderer_begin(&g.tile_renderer,texcache_get(&g.texcache,RID_image_hero),0,0xff);

  add_item_tiles(8,8,g.bitem);
  add_item_tiles(24,8,g.aitem);
  
  int i=0,x=40,y=6;
  for (;i<g.hpmax;i++,x+=8) {
    if (i<g.hp) tile_renderer_tile(&g.tile_renderer,x,y,0xaf,0);
    else tile_renderer_tile(&g.tile_renderer,x,y,0xae,0);
  }
    
  tile_renderer_end(&g.tile_renderer);
}

void egg_client_render() {

  // Search for an opaque menu, determine how many layers we actually need to draw.
  int menup=0;
  int menuc=g.menuc;
  int i=menuc;
  for (;i-->0;) {
    struct menu *menu=g.menuv[i];
    if (menu->opaque) {
      menup=i;
      menuc=g.menuc-i;
      break;
    }
  }

  // No menus, or the first one is not opaque, we need to draw the scene.
  if (!menuc||!g.menuv[menup]->opaque) {
    g.renderx=g.rendery=0;
    if (g.transclock>0.0) {
      double p=1.0-g.transclock/g.transtotal;
      render_game_transition(g.transition,p);
    } else {
      render_game_untransitioned();
    }
    render_overlay();
  }
  
  // Draw all the selected menus in order.
  for (i=menuc;i-->0;menup++) {
    struct menu *menu=g.menuv[menup];
    if (menu->render) menu->render(menu);
  }
  
  inkeep_render();
}
