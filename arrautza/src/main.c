#include "arrautza.h"
#include "inkeep/inkeep.h"
#include <egg/hid_keycode.h>

struct globals g={0};

void egg_client_quit() {
  inkeep_quit();
}

static void cb_joy(int plrid,int btnid,int value,int state,void *userdata) {
  //egg_log("%s %d.0x%04x=%d [0x%04x]",__func__,plrid,btnid,value,state);
  if (!plrid) {
    g.instate=state;
  }
}

static void cb_raw(const union egg_event *event,void *userdata) {
  switch (event->type) {
    case EGG_EVENT_KEY: if (event->key.value) switch (event->key.keycode) {
        case KEY_ESCAPE: egg_request_termination(); break;
      } break;
  }
}

int egg_client_init() {

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
  
  sprgrpv_init();
  srand_auto();
  
  if (!menu_push_hello()) {
    egg_log("Failed to initialize main menu!");
    return -1;
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
    //TODO overlay
  }
  
  // Draw all the selected menus in order.
  for (i=menuc;i-->0;menup++) {
    struct menu *menu=g.menuv[menup];
    if (menu->render) menu->render(menu);
  }
  
  inkeep_render();
}
