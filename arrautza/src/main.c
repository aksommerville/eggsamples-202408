#include "arrautza.h"
#include "inkeep/inkeep.h"
#include <egg/hid_keycode.h>

struct globals g={0};

void egg_client_quit() {
  inkeep_quit();
}

static void cb_joy(int plrid,int btnid,int value,int state,void *userdata) {
  egg_log("%s %d.0x%04x=%d [0x%04x]",__func__,plrid,btnid,value,state);
}

static void cb_raw(const union egg_event *event,void *userdata) {
  switch (event->type) {
    case EGG_EVENT_KEY: if (event->key.value) switch (event->key.keycode) {
        case KEY_ESCAPE: egg_request_termination(); break;
      } break;
  }
}

static int cb_log_mapcmd(const uint8_t *cmd,int cmdc,void *userdata) {
  char tmp[256];
  int tmpc=0;
  for (;cmdc-->0;cmd++) {
    tmp[tmpc++]=' ';
    tmp[tmpc++]="0123456789abcdef"[(*cmd)>>4];
    tmp[tmpc++]="0123456789abcdef"[(*cmd)&15];
  }
  egg_log("  %.*s",tmpc,tmp);
  return 0;
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
  
  //XXX Testing map builder and loader.
  struct map map={0};
  if (map_from_res(&map,0,RID_map_start)>=0) {
    egg_log("Loaded map. [0,0]=%02x [19,10]=%02x cmd[0]=%02x",map.v[0],map.v[10*COLC+19],map.commands[0]);
    egg_log("Commands:");
    map_for_each_command(&map,cb_log_mapcmd,0);
  } else {
    egg_log("!!! Failed to load map 'start' !!!");
  }
  
  return 0;
}

void egg_client_update(double elapsed) {
  inkeep_update(elapsed);
}

void egg_client_render() {
  inkeep_render();
}
