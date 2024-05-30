#include "inkeep_internal.h"
#include <egg/hid_keycode.h>

/* Set player button.
 * If plrid nonzero, we cascade into player zero.
 */
 
static void inkeep_set_player_button(int plrid,uint16_t btnid,int value) {
  if ((plrid<0)||(plrid>inkeep.playerc)) return;
  if (value) {
    if (inkeep.playerv[plrid]&btnid) return;
    inkeep.playerv[plrid]|=btnid;
  } else {
    if (!(inkeep.playerv[plrid]&btnid)) return;
    inkeep.playerv[plrid]&=~btnid;
  }
  inkeep_broadcast_joy(plrid,btnid,value,inkeep.playerv[plrid]);
  if (plrid) inkeep_set_player_button(0,btnid,value);
}

/* Search joystick list.
 */
 
static int inkeep_joyv_search(int devid) {
  int lo=0,hi=inkeep.joyc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    int q=inkeep.joyv[ck].devid;
         if (devid<q) hi=ck;
    else if (devid>q) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Joystick connected.
 */
 
static void inkeep_hello_joy(int devid) {
  if (inkeep.joyc>=INKEEP_PLAYER_LIMIT) return; // Too many devices, ignore this one.
  /* Log IDs. We don't actually need them. *
  int vid=0,pid=0,version=0;
  egg_joystick_get_ids(&vid,&pid,&version,devid);
  char name[256];
  int namec=egg_joystick_get_name(name,sizeof(name),devid);
  if ((namec<0)||(namec>sizeof(name))) namec=0;
  egg_log("%s %x:%x:%x '%.*s'",__func__,vid,pid,version,namec,name);
  /**/
  int p=inkeep_joyv_search(devid);
  if (p>=0) return; // Panic! Why do we already have it?
  p=-p-1;
  struct inkeep_joy *joy=inkeep.joyv+p;
  memmove(joy+1,joy,sizeof(struct inkeep_joy)*(inkeep.joyc-p));
  inkeep.joyc++;
  memset(joy,0,sizeof(struct inkeep_joy));
  joy->devid=devid;
  // Leave joy->plrid zero; it gets set when the first real state change arrives.
}

/* Joystick disconnected.
 */
 
static void inkeep_goodbye_joy(int devid) {
  int p=inkeep_joyv_search(devid);
  if (p<0) return;
  struct inkeep_joy *joy=inkeep.joyv+p;
  int plrid=joy->plrid;
  uint16_t state=joy->state;
  inkeep.joyc--;
  memmove(joy,joy+1,sizeof(struct inkeep_joy)*(inkeep.joyc-p));
  if (state) {
    uint16_t bit=0x8000;
    for (;bit;bit>>=1) {
      if (!(state&bit)) continue;
      inkeep_set_player_button(plrid,bit,0);
    }
  }
}

/* Select plrid for joystick if unset, on the first nonzero event.
 */
 
static void inkeep_joy_require_plrid(struct inkeep_joy *joy) {
  if (joy->plrid) return;
  if (inkeep.playerc<1) return;
  // Determine how many devices currently assigned to each player.
  int count_by_plrid[INKEEP_PLAYER_LIMIT]={0};
  const struct inkeep_joy *q=inkeep.joyv;
  int i=inkeep.joyc;
  for (;i-->0;q++) {
    if ((q->plrid<0)||(q->plrid>inkeep.playerc)) continue;
    count_by_plrid[q->plrid]++;
  }
  // Find the least-popular plrid, and ties break low.
  int plrid=1;
  for (i=1;i<=inkeep.playerc;i++) {
    if (count_by_plrid[i]<count_by_plrid[plrid]) plrid=i;
  }
  joy->plrid=plrid;
}

/* Joystick state change.
 */
 
static void inkeep_joy_state_change(int devid,int btnid,int value) {
  switch (btnid) { // RX,RY,AUX3, and anything unknown, we can ignore summarily.
    case EGG_JOYBTN_LX:
    case EGG_JOYBTN_LY: break;
    default: if ((btnid>=0x80)&&(btnid<=0x8f)) break; return;
  }
  int p=inkeep_joyv_search(devid);
  if (p<0) return;
  struct inkeep_joy *joy=inkeep.joyv+p;
  const int threshold=16; // LX,LY have a nominal range of -128..127. Allow a dead zone of +-16.
  switch (btnid) {
    case EGG_JOYBTN_LX: {
        int prev=(joy->lx>=threshold)?1:(joy->lx<=-threshold)?-1:0;
        int next=(value>=threshold)?1:(value<=-threshold)?-1:0;
        joy->lx=value;
        if (prev==next) return;
        if (next) inkeep_joy_require_plrid(joy);
        switch (prev) {
          case -1: if (joy->state&INKEEP_BTNID_LEFT) { joy->state&=~INKEEP_BTNID_LEFT; inkeep_set_player_button(joy->plrid,INKEEP_BTNID_LEFT,0); } break;
          case 1: if (joy->state&INKEEP_BTNID_RIGHT) { joy->state&=~INKEEP_BTNID_RIGHT; inkeep_set_player_button(joy->plrid,INKEEP_BTNID_RIGHT,0); } break;
        }
        switch (next) {
          case -1: if (!(joy->state&INKEEP_BTNID_LEFT)) { joy->state|=INKEEP_BTNID_LEFT; inkeep_set_player_button(joy->plrid,INKEEP_BTNID_LEFT,1); } break;
          case 1: if (!(joy->state&INKEEP_BTNID_RIGHT)) { joy->state|=INKEEP_BTNID_RIGHT; inkeep_set_player_button(joy->plrid,INKEEP_BTNID_RIGHT,1); } break;
        }
      } break;
    case EGG_JOYBTN_LY: {
        int prev=(joy->ly>=threshold)?1:(joy->ly<=-threshold)?-1:0;
        int next=(value>=threshold)?1:(value<=-threshold)?-1:0;
        joy->ly=value;
        if (prev==next) return;
        if (next) inkeep_joy_require_plrid(joy);
        switch (prev) {
          case -1: if (joy->state&INKEEP_BTNID_UP) { joy->state&=~INKEEP_BTNID_UP; inkeep_set_player_button(joy->plrid,INKEEP_BTNID_UP,0); } break;
          case 1: if (joy->state&INKEEP_BTNID_DOWN) { joy->state&=~INKEEP_BTNID_DOWN; inkeep_set_player_button(joy->plrid,INKEEP_BTNID_DOWN,0); } break;
        }
        switch (next) {
          case -1: if (!(joy->state&INKEEP_BTNID_UP)) { joy->state|=INKEEP_BTNID_UP; inkeep_set_player_button(joy->plrid,INKEEP_BTNID_UP,1); } break;
          case 1: if (!(joy->state&INKEEP_BTNID_DOWN)) { joy->state|=INKEEP_BTNID_DOWN; inkeep_set_player_button(joy->plrid,INKEEP_BTNID_DOWN,1); } break;
        }
      } break;
    default: {
        uint16_t bit=1<<(btnid-0x80);
        if (value) {
          inkeep_joy_require_plrid(joy);
          if (joy->state&bit) return;
          joy->state|=bit;
        } else {
          if (!(joy->state&bit)) return;
          joy->state&=~bit;
        }
        inkeep_set_player_button(joy->plrid,bit,value);
      }
  }
}

/* Mapped joystick event.
 */
 
static void inkeep_on_joy(const struct egg_event_joy *event) {
  //egg_log("%s %d.%d=%d",__func__,event->devid,event->btnid,event->value);
  if (!event->btnid) {
    if (event->value) {
      inkeep_hello_joy(event->devid);
    } else {
      inkeep_goodbye_joy(event->devid);
    }
  } else {
    inkeep_joy_state_change(event->devid,event->btnid,event->value);
  }
  if (inkeep.fake_keyboard) fake_keyboard_joy(event->btnid,event->value);
  else if (inkeep.fake_pointer) fake_pointer_joy(event->btnid,event->value);
}

/* Keyboard as joystick.
 * TODO Should this be configurable?
 * Use WASD, arrows, or numeric keypad.
 * Escape and F-keys are deliberately unmapped; those are expected to be used for stateless actions.
 */
 
static struct { int keycode,btnid; } inkeep_keymap[]={
  // This list must be sorted by KEY_*
  {KEY_A,INKEEP_BTNID_LEFT},
  {KEY_C,INKEEP_BTNID_EAST},
  {KEY_D,INKEEP_BTNID_RIGHT},
  {KEY_E,INKEEP_BTNID_R1},
  {KEY_Q,INKEEP_BTNID_L1},
  {KEY_S,INKEEP_BTNID_DOWN},
  {KEY_V,INKEEP_BTNID_NORTH},
  {KEY_W,INKEEP_BTNID_UP},
  {KEY_X,INKEEP_BTNID_WEST},
  {KEY_Z,INKEEP_BTNID_SOUTH},
  {KEY_ENTER,INKEEP_BTNID_AUX1},
  {KEY_BACKSPACE,INKEEP_BTNID_R2},
  {KEY_TAB,INKEEP_BTNID_L1},
  {KEY_SPACE,INKEEP_BTNID_SOUTH},
  {KEY_BACKSLASH,INKEEP_BTNID_R1},
  {KEY_APOSTROPHE,INKEEP_BTNID_AUX2},
  {KEY_GRAVE,INKEEP_BTNID_L2},
  {KEY_COMMA,INKEEP_BTNID_WEST},
  {KEY_DOT,INKEEP_BTNID_EAST},
  {KEY_SLASH,INKEEP_BTNID_NORTH},
  {KEY_RIGHT,INKEEP_BTNID_RIGHT},
  {KEY_LEFT,INKEEP_BTNID_LEFT},
  {KEY_DOWN,INKEEP_BTNID_DOWN},
  {KEY_UP,INKEEP_BTNID_UP},
  {KEY_KPSLASH,INKEEP_BTNID_AUX1},
  {KEY_KPSTAR,INKEEP_BTNID_AUX2},
  {KEY_KPDASH,0}, // Would be AUX3 if we supported that.
  {KEY_KPPLUS,INKEEP_BTNID_EAST},
  {KEY_KPENTER,INKEEP_BTNID_WEST},
  {KEY_KP1,INKEEP_BTNID_L2},
  {KEY_KP2,INKEEP_BTNID_DOWN},
  {KEY_KP3,INKEEP_BTNID_R2},
  {KEY_KP4,INKEEP_BTNID_LEFT},
  {KEY_KP5,INKEEP_BTNID_DOWN},
  {KEY_KP6,INKEEP_BTNID_RIGHT},
  {KEY_KP7,INKEEP_BTNID_L1},
  {KEY_KP8,INKEEP_BTNID_UP},
  {KEY_KP9,INKEEP_BTNID_R1},
  {KEY_KP0,INKEEP_BTNID_SOUTH},
  {KEY_KPDOT,INKEEP_BTNID_NORTH},
};
 
static int inkeep_btnid_from_keycode(int keycode) {
  int lo=0,hi=sizeof(inkeep_keymap)/sizeof(inkeep_keymap[0]);
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    int q=inkeep_keymap[ck].keycode;
         if (keycode<q) hi=ck;
    else if (keycode>q) lo=ck+1;
    else return inkeep_keymap[ck].btnid;
  }
  return 0;
}

/* Keyboard event.
 */
 
static void inkeep_on_key(const struct egg_event_key *event) {
  //egg_log("%s 0x%x=%d",__func__,event->keycode,event->value);
  if (inkeep.mode==INKEEP_MODE_JOY) { // In JOY mode, the keyboard is always player one.
   if (event->value!=2) { // Don't process key-repeat events.
      int btnid=inkeep_btnid_from_keycode(event->keycode);
      if (btnid) {
        inkeep_set_player_button(1,btnid,event->value);
      }
    }
  }
  if (inkeep.fake_pointer) fake_pointer_key(event->keycode,event->value);
}

/* Mouse motion.
 */
 
static void inkeep_on_mmotion(const struct egg_event_mmotion *event) {
  if ((event->x==inkeep.mousex)&&(event->y==inkeep.mousey)) return;
  inkeep.mousex=event->x;
  inkeep.mousey=event->y;
  inkeep_broadcast_mmotion(event->x,event->y);
  if (inkeep.fake_keyboard) fake_keyboard_focus_at(event->x,event->y);
  if (inkeep.fake_pointer) fake_pointer_mmotion(event->x,event->y);
}

/* Mouse button.
 */
 
static void inkeep_on_mbutton(const struct egg_event_mbutton *event) {
  if ((event->x!=inkeep.mousex)||(event->y!=inkeep.mousey)) {
    inkeep.mousex=event->x;
    inkeep.mousey=event->y;
    inkeep_broadcast_mmotion(event->x,event->y);
  }
  inkeep_broadcast_mbutton(event->btnid,event->value,event->x,event->y);
  if (inkeep.fake_keyboard&&(event->btnid==EGG_MBUTTON_LEFT)&&event->value) {
    fake_keyboard_activate_at(event->x,event->y);
  }
  if (inkeep.fake_pointer) fake_pointer_mbutton(event->btnid,event->value);
}

/* Mouse wheel.
 */
 
static void inkeep_on_mwheel(const struct egg_event_mwheel *event) {
  if ((event->x!=inkeep.mousex)||(event->y!=inkeep.mousey)) {
    inkeep.mousex=event->x;
    inkeep.mousey=event->y;
    inkeep_broadcast_mmotion(event->x,event->y);
  }
  inkeep_broadcast_mwheel(event->dx,event->dy,event->x,event->y);
}

/* Touch.
 */
 
static void inkeep_on_touch(const struct egg_event_touch *event) {
  //TODO Fake joystick.
  switch (event->state) {
  
    case EGG_TOUCH_END: {
        if (event->touchid==inkeep.touchid) {
          inkeep.touchx=event->x;
          inkeep.touchy=event->y;
          inkeep_broadcast_touch(EGG_TOUCH_END,inkeep.touchx,inkeep.touchy);
        }
      } break;
      
    case EGG_TOUCH_BEGIN: {
        // If we were tracking another touch, pretend it released.
        // Ingress supports multiple touches but egress is strictly one at a time.
        if (inkeep.touchid) {
          inkeep_broadcast_touch(EGG_TOUCH_END,inkeep.touchx,inkeep.touchy);
        }
        inkeep.touchid=event->touchid;
        inkeep.touchx=event->x;
        inkeep.touchy=event->y;
        inkeep_broadcast_touch(EGG_TOUCH_BEGIN,inkeep.touchx,inkeep.touchy);
        if (inkeep.fake_keyboard) fake_keyboard_activate_at(inkeep.touchx,inkeep.touchy);
      } break;
    
    case EGG_TOUCH_MOVE: {
        if (event->touchid==inkeep.touchid) {
          inkeep.touchx=event->x;
          inkeep.touchy=event->y;
          inkeep_broadcast_touch(EGG_TOUCH_MOVE,inkeep.touchx,inkeep.touchy);
        }
      } break;
  }
}

/* Update.
 */
 
void inkeep_update(double elapsed) {
  union egg_event eventv[16];
  int eventc;
  while ((eventc=egg_event_get(eventv,16))>0) {
    const union egg_event *event=eventv;
    int i=eventc;
    for (;i-->0;event++) {
      inkeep_broadcast_raw(event);
      switch (event->type) {
        case EGG_EVENT_JOY: inkeep_on_joy(&event->joy); break;
        case EGG_EVENT_KEY: inkeep_on_key(&event->key); break;
        case EGG_EVENT_TEXT: inkeep_broadcast_text(event->text.codepoint); break;
        case EGG_EVENT_MMOTION: inkeep_on_mmotion(&event->mmotion); break;
        case EGG_EVENT_MBUTTON: inkeep_on_mbutton(&event->mbutton); break;
        case EGG_EVENT_MWHEEL: inkeep_on_mwheel(&event->mwheel); break;
        case EGG_EVENT_TOUCH: inkeep_on_touch(&event->touch);
        case EGG_EVENT_ACCEL: inkeep_broadcast_accel(event->accel.x,event->accel.y,event->accel.z); break;
        case EGG_EVENT_RAW: break; // I think we never use.
      }
    }
    if (eventc<16) break;
  }
  if (inkeep.fake_keyboard) fake_keyboard_update(elapsed);
  if (inkeep.fake_pointer) fake_pointer_update(elapsed);
}
