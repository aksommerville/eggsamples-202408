#include "inkeep_internal.h"

/* Mapped joystick event.
 */
 
static void inkeep_on_joy(const struct egg_event_joy *event) {
  //egg_log("%s %d.%d=%d",__func__,event->devid,event->btnid,event->value);
  //TODO Map to joystick if applicable.
  if (inkeep.fake_keyboard) fake_keyboard_joy(event->btnid,event->value);
  else if (inkeep.fake_pointer) fake_pointer_joy(event->btnid,event->value);
}

/* Keyboard event.
 */
 
static void inkeep_on_key(const struct egg_event_key *event) {
  //egg_log("%s 0x%x=%d",__func__,event->keycode,event->value);
  //TODO Map to joystick if applicable.
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
