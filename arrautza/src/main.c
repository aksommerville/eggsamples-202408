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

  if (inkeep_init()<0) return -1;
  inkeep_set_player_count(16);
  inkeep_set_mode(INKEEP_MODE_JOY);
  inkeep_listen_joy(cb_joy,0);
  inkeep_listen_raw(cb_raw,0);
  
  if ((g.texid_tilesheet=egg_texture_new())<0) return -1;
  
  sprgrpv_init();
  srand_auto();
  
  if (load_map(RID_map_start)<0) {
    egg_log("Failed to load initial map.");
    return -1;
  }
  
  return 0;
}

void egg_client_update(double elapsed) {
  inkeep_update(elapsed);
  sprgrp_update(sprgrpv+SPRGRP_UPDATE,elapsed);
  physics_update(sprgrpv+SPRGRP_SOLID,elapsed);
  check_sprites_footing(sprgrpv+SPRGRP_FOOTING);
  // Any non-sprite update stuff goes here.
  sprgrp_kill(sprgrpv+SPRGRP_DEATHROW);
}

void egg_client_render() {
  render_map();
  sprgrp_render(sprgrpv+SPRGRP_RENDER);
  inkeep_render();
}
