#include "hardboiled.h"
#include "verblist.h"
#include <egg/hid_keycode.h>

#define REGION_NONE 0
#define REGION_NOTEPAD 1
#define REGION_ROOM 2
#define REGION_VERBS 3
#define REGION_LOG 4

static int texid_bg=0;
static struct room *room=0;
static int focus_region=REGION_NONE;
static int focus_noun=0;
struct font *font=0;
static int texid_focus_noun=0;
static int texid_focus_noun_w=0,texid_focus_noun_h=0;

void egg_client_quit() {
  inkeep_quit();
}

static void focus_noun_changed() {
  int stringid=0;
  if (focus_noun&&room&&room->name_for_noun) {
    stringid=room->name_for_noun(room,focus_noun);
  }
  const char *src=0;
  int srcc=text_get_string(&src,stringid);
  if (srcc<0) srcc=0;
  font_render_to_texture(texid_focus_noun,font,src,srcc,0,0xffff80);
  egg_texture_get_header(&texid_focus_noun_w,&texid_focus_noun_h,0,texid_focus_noun);
}

// Replaces (focus_region) and (focus_noun), and triggers focus_noun_changed if warranted.
static void refresh_focus_noun(int x,int y) {

  if ((x>=3)&&(y>=3)&&(x<68)&&(y<99)) {
    focus_region=REGION_NOTEPAD;
  } else if ((x>=72)&&(y>=3)&&(x<168)&&(y<99)) {
    focus_region=REGION_ROOM;
  } else if ((x>=172)&&(y>=3)&&(x<237)&&(y<99)) {
    focus_region=REGION_VERBS;
  } else if ((x>=3)&&(y>=103)&&(x<237)&&(y<132)) {
    focus_region=REGION_LOG;
  } else {
    focus_region=REGION_NONE;
  }
  
  if (focus_region==REGION_ROOM) {
    int subx=x-72;
    int suby=y-3;
    if ((subx<0)||(suby<0)||(subx>=96)||(suby>=96)) {
      if (focus_noun) {
        focus_noun=0;
        focus_noun_changed();
      }
    } else {
      int nnoun=0;
      if (room&&room->noun_for_point) {
        nnoun=room->noun_for_point(room,subx,suby);
        if (nnoun<0) nnoun=0;
      }
      if (nnoun!=focus_noun) {
        focus_noun=nnoun;
        focus_noun_changed();
      }
    }
  } else {
    if (focus_noun) {
      focus_noun=0;
      focus_noun_changed();
    }
  }
}

int change_room(struct room *(*ctor)()) {
  struct room *newroom=ctor();
  if (!newroom) return -1;
  if (room) {
    if (room->del) room->del(room);
    free(room);
  }
  room=newroom;
  verblist_unselect();
  focus_noun=0;
  return 0;
}

//TODO I'm implementing cb_raw just to get key events, for the Escape key.
// Can inkeep give us something more structured for stateless signals from the keyboard?
static void cb_raw(const union egg_event *event,void *userdata) {
  switch (event->type) {
    case EGG_EVENT_KEY: if (event->key.value) switch (event->key.keycode) {
        case KEY_ESCAPE: egg_request_termination(); break;
      } break;
  }
}

static void cb_text(int codepoint,void *userdata) {
  egg_log("%s U+%x",__func__,codepoint);
}

static void cb_touch(int state,int x,int y,void *userdata) {
  switch (state) {
    case EGG_TOUCH_BEGIN: {
        refresh_focus_noun(x,y);
        switch (focus_region) {
          case REGION_ROOM: {
              if (room&&room->act) {
                room->act(room,verblist_get(),focus_noun);
              }
            } break;
          case REGION_VERBS: {
              verblist_press(x-170,y-1);
              if (verblist_get()==VERB_EXIT) {
                if (room&&room->exit) {
                  change_room(room->exit);
                }
                verblist_unselect();
                focus_noun=0;
              }
            } break;
        }
      } break;
    case EGG_TOUCH_END: {
        refresh_focus_noun(x,y);
      } break;
    case EGG_TOUCH_MOVE: {
        refresh_focus_noun(x,y);
      } break;
  }
}

// When in TEXT mode, we won't get fake TOUCH events from inkeep.
// Fake it ourselves, from the mouse only.
//TODO Should inkeep send these?
/*...yes it definitely should. Faking it ourselves, we end up doubling events in TOUCH mode.
static void cb_mmotion(int x,int y,void *userdata) {
  cb_touch(2,x,y,userdata);
}
static void cb_mbutton(int btnid,int value,int x,int y,void *userdata) {
  if (btnid!=EGG_MBUTTON_LEFT) return;
  cb_touch(value?1:0,x,y,userdata);
}
/**/

int egg_client_init() {

  if (text_init()<0) return -1;

  egg_texture_load_image(texid_bg=egg_texture_new(),0,RID_image_bg);
  texid_focus_noun=egg_texture_new();

  if (!(font=font_new(9))) return -1;
  font_add_page(font,3,0x21);
  font_add_page(font,4,0xa1);
  font_add_page(font,5,0x400);
  font_add_page(font,6,0x01);

  if (inkeep_init()<0) return -1;
  if (inkeep_set_font(font)<0) return -1;
  if (inkeep_set_keycaps(3,3,12,
     ".qwertyuiop\x08"
     "?asdfghjkl\"\x0a"
    "\3zxcvbnm<> \x01"
          ",QWERTYUIOP\x08"
          "|ASDFGHJKL:\x0a"
         "\3ZXCVBNM{} \x01"
     "`1234567890\x08"
     "~!@#$%^&*()\x0a"
     "+-=[]\\;'/_ \x01"
  )<0) return -1;
  if (inkeep_set_cursor(7,0,0,16,16,0,4,1)<0) return -1;
  inkeep_set_mode(INKEEP_MODE_TOUCH);
  inkeep_listen_all(cb_raw,0,cb_text,cb_touch,0,0,/*cb_mmotion,cb_mbutton,*/0,0,0,0);
  
  verblist_init();
  if (change_room(room_new_title)<0) return -1;
  
  return 0;
}

void egg_client_update(double elapsed) {
  inkeep_update(elapsed);
  if (room&&room->update) room->update(room,elapsed);
}

void egg_client_render() {
  if (room&&room->render) {
    room->render(room,72,3,96,96);
  } else {
    egg_draw_rect(1,72,3,96,96,0x808080ff);
  }
  egg_draw_decal(1,texid_bg,0,0,0,0,SCREENW,SCREENH,0);
  verblist_render(170,1,69,85);
  if (focus_noun) {
    egg_draw_decal(1,texid_focus_noun,174,90,0,0,texid_focus_noun_w,texid_focus_noun_h,0);//TODO really need a default (w,h) for "the whole thing"
  }
  log_render();
  inkeep_render();
}
