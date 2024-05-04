#include <egg/egg.h>
#include "joy2.h"
#include "bus.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* Object definition.
 */
 
// Corresponds to one input button with multiple outputs. (hat or signed axis).
struct joy2_button {
  int srcbtnid;
  int srcvalue;
  int mode;
  int srclo,srchi;
  int state[4]; // HAT:(w,e,n,s), AXIS:(lo,hi)
};
 
struct joy2_device {
  int devid;
  char *name;
  int vid,pid,version;
  int mapping;
  struct joy2_button *buttonv; // Input buttons that produce multiple outputs.
  int buttonc,buttona;
  int *holdv; // 2-state btnid with a nonzero value right now.
  int holdc,holda;
};

struct joy2_touch {
  int touchid;
  int x,y;
  uint8_t state; // Multiple (1<<(btnid-1)). btnid (1,2,3,4) can combine.
};

struct joy2_listener {
  int listenerid;
  void (*cb)(int devid,int btnid,char part,int value,void *userdata);
  void *userdata;
};
 
struct joy2 {
  struct bus *bus;
  int touch_enabled; // Has our owner requested touch?
  int has_touch; // Did the platform acknowledge touch?
  int key_enabled;
  int screenw,screenh;
  struct joy2_device *devicev;
  int devicec,devicea;
  struct joy2_touch *touchv;
  int touchc,toucha;
  struct joy2_listener *listenerv;
  int listenerc,listenera;
  int listenerid_next;
};

/* Delete.
 */
 
static void joy2_device_cleanup(struct joy2_device *device) {
  if (device->name) free(device->name);
  if (device->buttonv) free(device->buttonv);
  if (device->holdv) free(device->holdv);
}
 
void joy2_del(struct joy2 *joy2) {
  if (!joy2) return;
  if (joy2->devicev) {
    while (joy2->devicec-->0) joy2_device_cleanup(joy2->devicev+joy2->devicec);
    free(joy2->devicev);
  }
  if (joy2->touchv) free(joy2->touchv);
  if (joy2->listenerv) free(joy2->listenerv);
  free(joy2);
}

/* New.
 */

struct joy2 *joy2_new(struct bus *bus) {
  struct joy2 *joy2=calloc(1,sizeof(struct joy2));
  if (!joy2) return 0;
  joy2->bus=bus;
  joy2->listenerid_next=1;
  egg_texture_get_header(&joy2->screenw,&joy2->screenh,0,1);
  return joy2;
}

/* Listeners.
 */
 
int joy2_listen(
  struct joy2 *joy2,
  void (*cb)(int devid,int btnid,char part,int value,void *userdata),
  void *userdata
) {
  if (joy2->listenerid_next<1) return -1;
  // When someone unlistens, we just zero their record and leave it.
  // Fill in any gaps first.
  struct joy2_listener *listener=joy2->listenerv;
  int i=joy2->listenerc;
  for (;i-->0;listener++) {
    if (listener->listenerid) continue;
    listener->listenerid=joy2->listenerid_next++;
    listener->cb=cb;
    listener->userdata=userdata;
    return listener->listenerid;
  }
  if (joy2->listenerc>=joy2->listenera) {
    int na=joy2->listenera+8;
    if (na>INT_MAX/sizeof(struct joy2_listener)) return -1;
    void *nv=realloc(joy2->listenerv,sizeof(struct joy2_listener)*na);
    if (!nv) return -1;
    joy2->listenerv=nv;
    joy2->listenera=na;
  }
  listener=joy2->listenerv+joy2->listenerc++;
  listener->listenerid=joy2->listenerid_next++;
  listener->cb=cb;
  listener->userdata=userdata;
  return listener->listenerid;
}

static void joy2_cb_inert(int devid,int btnid,char part,int value,void *userdata) {}

void joy2_unlisten(struct joy2 *joy2,int listenerid) {
  int i=joy2->listenerc;
  struct joy2_listener *listener=joy2->listenerv+i-1;
  for (;i-->0;listener--) {
    if (listener->listenerid!=listenerid) continue;
    // Don't actually remove it; there might be an iteration in progress. Just mark for later removal.
    listener->listenerid=0;
    listener->cb=joy2_cb_inert;
    return;
  }
}

/* Device list.
 */
 
static int joy2_devicev_search(const struct joy2 *joy2,int devid) {
  int lo=0,hi=joy2->devicec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct joy2_device *device=joy2->devicev+ck;
         if (devid<device->devid) hi=ck;
    else if (devid>device->devid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static struct joy2_device *joy2_devicev_insert(struct joy2 *joy2,int p,int devid) {
  if ((p<0)||(p>joy2->devicec)) return 0;
  if (joy2->devicec>=joy2->devicea) {
    int na=joy2->devicea+8;
    if (na>INT_MAX/sizeof(struct joy2_device)) return 0;
    void *nv=realloc(joy2->devicev,sizeof(struct joy2_device)*na);
    if (!nv) return 0;
    joy2->devicev=nv;
    joy2->devicea=na;
  }
  struct joy2_device *device=joy2->devicev+p;
  memmove(device+1,device,sizeof(struct joy2_device)*(joy2->devicec-p));
  joy2->devicec++;
  memset(device,0,sizeof(struct joy2_device));
  device->devid=devid;
  return device;
}
 
static struct joy2_device *joy2_devicev_add(struct joy2 *joy2,int devid) {
  int p=joy2_devicev_search(joy2,devid);
  if (p>=0) return 0;
  struct joy2_device *device=joy2_devicev_insert(joy2,-p-1,devid);
  if (!device) return 0;
  return device;
}

static struct joy2_device *joy2_devicev_get(const struct joy2 *joy2,int devid) {
  int p=joy2_devicev_search(joy2,devid);
  if (p<0) return 0;
  return joy2->devicev+p;
}

/* Device name.
 */
 
static int joy2_device_set_name(struct joy2_device *device,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  char *nv=0;
  if (srcc) {
    if (!(nv=malloc(srcc+1))) return -1;
    memcpy(nv,src,srcc);
    nv[srcc]=0;
  }
  if (device->name) free(device->name);
  device->name=nv;
  return 0;
}

static int joy2_device_set_name_from_platform(struct joy2_device *device) {
  if (device->name) {
    free(device->name);
    device->name=0;
  }
  int a=32;
  char *v=malloc(a);
  if (!v) return -1;
  for (;;) {
    int c=egg_input_device_get_name(v,a,device->devid);
    if (c<=0) {
      free(v);
      return 0;
    }
    if (c<a) {
      v[c]=0;
      device->name=v;
      return 0;
    }
    char *nv=realloc(v,a=c+1);
    if (!nv) {
      free(v);
      return -1;
    }
    v=nv;
  }
}

/* Add mappable features to device.
 */
 
static int joy2_device_buttonv_search(const struct joy2_device *device,int btnid) {
  int lo=0,hi=device->buttonc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct joy2_button *button=device->buttonv+ck;
         if (btnid<button->srcbtnid) hi=ck;
    else if (btnid>button->srcbtnid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static struct joy2_button *joy2_device_add_button(struct joy2_device *device,int btnid) {
  int p=joy2_device_buttonv_search(device,btnid);
  if (p>=0) return 0;
  p=-p-1;
  if (device->buttonc>=device->buttona) {
    int na=device->buttona+16;
    if (na>INT_MAX/sizeof(struct joy2_button)) return 0;
    void *nv=realloc(device->buttonv,sizeof(struct joy2_button)*na);
    if (!nv) return 0;
    device->buttonv=nv;
    device->buttona=na;
  }
  struct joy2_button *button=device->buttonv+p;
  memmove(button+1,button,sizeof(struct joy2_button)*(device->buttonc-p));
  device->buttonc++;
  memset(button,0,sizeof(struct joy2_button));
  button->srcbtnid=btnid;
  return button;
}
 
static int joy2_device_add_hat(struct joy2_device *device,int btnid,int lo) {
  struct joy2_button *button=joy2_device_add_button(device,btnid);
  if (!button) return -1;
  button->mode=JOY2_MODE_HAT;
  button->srclo=lo;
  return 0;
}

static int joy2_device_add_axis(struct joy2_device *device,int btnid,int hidusage,int lo,int hi) {
  int mid=(lo+hi)>>1;
  int midlo=(lo+mid)>>1;
  int midhi=(hi+mid)>>1;
  if (midlo>=mid) midlo=mid-1;
  if (midhi<=mid) midhi=mid+1;
  struct joy2_button *button=joy2_device_add_button(device,btnid);
  if (!button) return -1;
  button->mode=JOY2_MODE_AXIS;
  button->srcvalue=mid;
  button->srclo=midlo;
  button->srchi=midhi;
  return 0;
}

/* Report some output state change.
 * Does not change our state.
 */
 
static void joy2_report_button(struct joy2 *joy2,int devid,int btnid,char part,int value) {
  const struct joy2_listener *listener=joy2->listenerv;
  int i=joy2->listenerc;
  for (;i-->0;listener++) {
    listener->cb(devid,btnid,part,value,listener->userdata);
  }
}

/* Device connected.
 */
 
static void joy2_on_connect(struct joy2 *joy2,int devid,int mapping) {
  if (devid<1) return;
  struct joy2_device *device=joy2_devicev_add(joy2,devid);
  if (!device) return;
  device->mapping=mapping;
  egg_input_device_get_ids(&device->vid,&device->pid,&device->version,devid);
  joy2_device_set_name_from_platform(device);
  int p=0; for (;;p++) {
    int btnid=0,hidusage=0,lo=0,hi=0,value=0;
    egg_input_device_get_button(&btnid,&hidusage,&lo,&hi,&value,devid,p);
    if (!btnid) break;
    int range=hi-lo+1;
    if (range<3) continue; // Invalid or two-state-only range, skip it.
    
    // If the range is 8, it must be a hat.
    if (range==8) {
      joy2_device_add_hat(device,btnid,lo);
      continue;
    }
    
    // Low value and resting value both zero, assume it's one-way, and we can treat like a regular two-state button.
    if (!lo&&!value) continue;
    
    // Resting value falls between (not on) the end points, assume it's two-way. Mapping is required.
    if ((lo<value)&&(value<hi)) {
      joy2_device_add_axis(device,btnid,hidusage,lo,hi);
      continue;
    }
    
    // Anything else is a mystery, don't touch.
    //egg_log("  btnid=%d hidusage=%08x range=%d..%d value=%d",btnid,hidusage,lo,hi,value);
  }
  joy2_report_button(joy2,devid,0,'c',1);
}

/* Drop all state for a device.
 */
 
static void joy2_clear_device_state(struct joy2 *joy2,struct joy2_device *device) {
  struct joy2_button *button=device->buttonv;
  int i=device->buttonc;
  for (;i-->0;button++) {
    switch (button->mode) {
      case JOY2_MODE_HAT: {
          if (button->state[0]) { button->state[0]=0; joy2_report_button(joy2,device->devid,button->srcbtnid,'w',0); }
          if (button->state[1]) { button->state[1]=0; joy2_report_button(joy2,device->devid,button->srcbtnid,'e',0); }
          if (button->state[2]) { button->state[2]=0; joy2_report_button(joy2,device->devid,button->srcbtnid,'n',0); }
          if (button->state[3]) { button->state[3]=0; joy2_report_button(joy2,device->devid,button->srcbtnid,'s',0); }
        } break;
      case JOY2_MODE_AXIS: {
          if (button->state[0]) { button->state[0]=0; joy2_report_button(joy2,device->devid,button->srcbtnid,'l',0); }
          if (button->state[1]) { button->state[1]=0; joy2_report_button(joy2,device->devid,button->srcbtnid,'h',0); }
        } break;
    }
  }
  while (device->holdc>0) {
    device->holdc--;
    joy2_report_button(joy2,device->devid,device->holdv[device->holdc],'b',0);
  }
}

/* Add or remove a hold. Fires event if changed.
 */
 
static void joy2_device_add_hold(struct joy2 *joy2,struct joy2_device *device,int btnid) {
  int i=device->holdc;
  while (i-->0) {
    if (device->holdv[i]==btnid) return;
  }
  if (device->holdc>=device->holda) {
    int na=device->holda+8;
    if (na>INT_MAX/sizeof(int)) return;
    void *nv=realloc(device->holdv,sizeof(int)*na);
    if (!nv) return;
    device->holdv=nv;
    device->holda=na;
  }
  device->holdv[device->holdc++]=btnid;
  joy2_report_button(joy2,device->devid,btnid,'b',1);
}

static void joy2_device_remove_hold(struct joy2 *joy2,struct joy2_device *device,int btnid) {
  int i=device->holdc;
  while (i-->0) {
    if (device->holdv[i]!=btnid) continue;
    device->holdc--;
    memmove(device->holdv+i,device->holdv+i+1,sizeof(int)*(device->holdc-i));
    joy2_report_button(joy2,device->devid,btnid,'b',0);
    return;
  }
}

/* Device disconnected.
 */
 
static void joy2_on_disconnect(struct joy2 *joy2,int devid) {
  if (devid<1) return;
  int p=joy2_devicev_search(joy2,devid);
  if (p<0) return;
  struct joy2_device *device=joy2->devicev+p;
  joy2_clear_device_state(joy2,device);
  joy2_device_cleanup(device);
  joy2->devicec--;
  memmove(device,device+1,sizeof(struct joy2_device)*(joy2->devicec-p));
  joy2_report_button(joy2,devid,0,'c',0);
}

/* Generic input.
 */
 
static void joy2_on_input(struct joy2 *joy2,int devid,int btnid,int value) {
  int p=joy2_devicev_search(joy2,devid);
  if (p<0) return;
  struct joy2_device *device=joy2->devicev+p;
  p=joy2_device_buttonv_search(device,btnid);
  if (p>=0) {
    struct joy2_button *button=device->buttonv+p;
    if (value==button->srcvalue) return;
    int pv=button->srcvalue;
    button->srcvalue=value;
    switch (button->mode) {
    
      case JOY2_MODE_HAT: {
          int px=0,py=0,nx=0,ny=0;
          switch (pv-button->srclo) {
            case 7: case 6: case 5: px=-1; break;
            case 1: case 2: case 3: px=1; break;
          }
          switch (pv-button->srclo) {
            case 7: case 0: case 1: py=-1; break;
            case 5: case 4: case 3: py=1; break;
          }
          switch (value-button->srclo) {
            case 7: case 6: case 5: nx=-1; break;
            case 1: case 2: case 3: nx=1; break;
          }
          switch (value-button->srclo) {
            case 7: case 0: case 1: ny=-1; break;
            case 5: case 4: case 3: ny=1; break;
          }
          if (px!=nx) {
                 if (px<0) { button->state[0]=0; joy2_report_button(joy2,devid,btnid,'w',0); }
            else if (px>0) { button->state[1]=0; joy2_report_button(joy2,devid,btnid,'e',0); }
                 if (nx<0) { button->state[0]=1; joy2_report_button(joy2,devid,btnid,'w',1); }
            else if (nx>0) { button->state[1]=1; joy2_report_button(joy2,devid,btnid,'e',1); }
          }
          if (py!=ny) {
                 if (py<0) { button->state[2]=0; joy2_report_button(joy2,devid,btnid,'n',0); }
            else if (py>0) { button->state[3]=0; joy2_report_button(joy2,devid,btnid,'s',0); }
                 if (ny<0) { button->state[2]=1; joy2_report_button(joy2,devid,btnid,'n',1); }
            else if (ny>0) { button->state[3]=1; joy2_report_button(joy2,devid,btnid,'s',1); }
          }
        } break;
        
      case JOY2_MODE_AXIS: {
          if (value<=button->srclo) {
            if (button->state[1]) { button->state[1]=0; joy2_report_button(joy2,devid,btnid,'h',0); }
            if (!button->state[0]) { button->state[0]=1; joy2_report_button(joy2,devid,btnid,'l',1); }
          } else if (value>=button->srchi) {
            if (button->state[0]) { button->state[0]=0; joy2_report_button(joy2,devid,btnid,'l',0); }
            if (!button->state[1]) { button->state[1]=1; joy2_report_button(joy2,devid,btnid,'h',1); }
          } else {
            if (button->state[0]) { button->state[0]=0; joy2_report_button(joy2,devid,btnid,'l',0); }
            if (button->state[1]) { button->state[1]=0; joy2_report_button(joy2,devid,btnid,'h',0); }
          }
        } break;
        
    }
  } else {
    if (value) joy2_device_add_hold(joy2,device,btnid);
    else joy2_device_remove_hold(joy2,device,btnid);
  }
}

/* Button state for screen position, for touch events.
 * The screen is split horizontally, dpad on the left and two buttons on the right.
 *   +----+----+----+-------+------+
 *   | 1,3|  3 | 2,3|       |      |
 *   +----+----+----+   8   |   6  |
 *   |  1 |  0 |  2 +-------+------|
 *   |----+----+----+       |      |
 *   | 1,4|  4 | 2,4|   7   |   5  |
 *   +----+----+----+-------+------+
 * The returned state may contain multiple set bits, if a diagonal is pressed.
 * Those 8 bits map to btnid in our version of Standard Gamepad Mapping.
 */
 
static int JOY2_TOUCH_BTNID[8]={
  0x8e, // Left
  0x8f, // Right
  0x8c, // Up
  0x8d, // Down
  0x80, // "South" (southeast)
  0x82, // "West" (northeast; I consider standard West the second-most-reachable, but in a touchscreen layout, Northeast is second)
  0x81, // "East" (southwest)
  0x83, // "North" (northwest)
};
 
static uint8_t joy2_state_for_touch(const struct joy2 *joy2,int x,int y) {
  if ((x<0)||(x>=joy2->screenw)) return 0;
  if ((y<0)||(y>=joy2->screenh)) return 0;
  int midx=joy2->screenw>>1;
  if (x<midx) {
    int colw=midx/3;
    int rowh=joy2->screenh/3;
    int col=x/colw;
    int row=y/rowh;
    switch (row*3+col) {
      case 0: return 0x05; // NW
      case 1: return 0x04; // N
      case 2: return 0x06; // NE
      case 3: return 0x01; // W
      case 4: return 0x00; // center
      case 5: return 0x02; // E
      case 6: return 0x09; // SW
      case 7: return 0x08; // S
      case 8: return 0x0a; // SE
    }
  } else {
    int colw=midx>>1;
    int rowh=joy2->screenh>>1;
    int col=(x-midx)/colw;
    int row=y/rowh;
    if (col==0) {
      if (row==0) return 0x80;
      if (row==1) return 0x40;
    } else if (col==1) {
      if (row==0) return 0x20;
      if (row==1) return 0x10;
    }
  }
  return 0;
}

/* Touch.
 */
 
static void joy2_on_touch(struct joy2 *joy2,int touchid,int state,int x,int y) {
  if (!joy2->has_touch) return; // Possible for events to enable, but we couldn't install the device for some reason. Suppress events in that case.
  switch (state) {
  
    case 0: { // End of touch.
        int i=joy2->touchc;
        struct joy2_touch *touch=joy2->touchv+i-1;
        for (;i-->0;touch--) {
          if (touch->touchid!=touchid) continue;
          if (touch->state) {
            struct joy2_device *device=joy2_devicev_get(joy2,JOY2_DEVID_TOUCH);
            if (device) {
              uint8_t bit=1,btnix=0;
              for (;bit;bit<<=1,btnix++) {
                if (touch->state&bit) joy2_device_remove_hold(joy2,device,JOY2_TOUCH_BTNID[btnix]);
              }
            }
          }
          joy2->touchc--;
          memmove(touch,touch+1,sizeof(struct joy2_touch)*(joy2->touchc-i));
          return;
        }
      } break;
      
    case 1: { // New touch.
        // Check that we don't already have this touchid.
        // Something is horribly broken if we do, but proceeding in that case would break it even worse.
        int i=joy2->touchc;
        struct joy2_touch *touch=joy2->touchv;
        for (;i-->0;touch++) {
          if (touch->touchid==touchid) return;
        }
        if (joy2->touchc>=joy2->toucha) {
          int na=joy2->toucha+4;
          if (na>INT_MAX/sizeof(struct joy2_touch)) return;
          void *nv=realloc(joy2->touchv,sizeof(struct joy2_touch)*na);
          if (!nv) return;
          joy2->touchv=nv;
          joy2->toucha=na;
        }
        touch=joy2->touchv+joy2->touchc++;
        memset(touch,0,sizeof(struct joy2_touch));
        touch->touchid=touchid;
      } // pass: Now process it like a moved touch (moved from 0,0 OOB).
      
    case 2: { // Existing touch moved.
        int i=joy2->touchc;
        struct joy2_touch *touch=joy2->touchv;
        for (;i-->0;touch++) {
          if (touch->touchid!=touchid) continue;
          uint8_t nstate=joy2_state_for_touch(joy2,x,y);
          if (nstate==touch->state) return;
          struct joy2_device *device=joy2_devicev_get(joy2,JOY2_DEVID_TOUCH);
          if (device) {
            uint8_t bit=1,btnix=0;
            for (;bit;bit<<=1,btnix++) {
              if (touch->state&bit) {
                if (!(nstate&bit)) joy2_device_remove_hold(joy2,device,JOY2_TOUCH_BTNID[btnix]);
              } else {
                if (nstate&bit) joy2_device_add_hold(joy2,device,JOY2_TOUCH_BTNID[btnix]);
              }
            }
          }
          touch->state=nstate;
          return;
        }
      } break;
      
  }
}

/* Key.
 */
 
static void joy2_on_key(struct joy2 *joy2,int keycode,int value) {
  int p=joy2_devicev_search(joy2,JOY2_DEVID_KEYBOARD);
  if (p<0) return;
  struct joy2_device *device=joy2->devicev+p;
  if (value) joy2_device_add_hold(joy2,device,keycode);
  else joy2_device_remove_hold(joy2,device,keycode);
}

/* Event.
 */

void joy2_event(struct joy2 *joy2,const struct egg_event *event) {
  switch (event->type) {
    case EGG_EVENT_CONNECT: joy2_on_connect(joy2,event->v[0],event->v[1]); break;
    case EGG_EVENT_DISCONNECT: joy2_on_disconnect(joy2,event->v[0]); break;
    case EGG_EVENT_INPUT: joy2_on_input(joy2,event->v[0],event->v[1],event->v[2]); break;
    case EGG_EVENT_TOUCH: joy2_on_touch(joy2,event->v[0],event->v[1],event->v[2],event->v[3]); break;
    case EGG_EVENT_KEY: joy2_on_key(joy2,event->v[0],event->v[1]); break;
  }
}

/* Enable keyboard events.
 * KEY events are always on, even when we've disabled processing of them.
 */
 
void joy2_enable_keyboard(struct joy2 *joy2,int enable) {
  if (enable) {
    if (joy2->key_enabled) return;
    joy2->key_enabled=1;
    // KEY events should already be enabled, but we can double-check, and also we don't want to create the device if KEY events will not be sent.
    switch (egg_event_enable(EGG_EVENT_KEY,EGG_EVTSTATE_ENABLED)) {
      case EGG_EVTSTATE_ENABLED:
      case EGG_EVTSTATE_REQUIRED: {
          struct joy2_device *device=joy2_devicev_add(joy2,JOY2_DEVID_KEYBOARD);
          if (device) {
            joy2_device_set_name(device,"System Keyboard",-1);
            joy2_report_button(joy2,JOY2_DEVID_KEYBOARD,0,'c',1);
          }
        } break;
    }
  } else {
    if (!joy2->key_enabled) return;
    joy2->key_enabled=0;
    // Don't disable KEY events. They're supposed to stay on at all times.
    int p=joy2_devicev_search(joy2,JOY2_DEVID_KEYBOARD);
    if (p>=0) {
      struct joy2_device *device=joy2->devicev+p;
      joy2_clear_device_state(joy2,device);
      joy2_device_cleanup(device);
      joy2->devicec--;
      memmove(device,device+1,sizeof(struct joy2_device)*(joy2->devicec-p));
      joy2_report_button(joy2,JOY2_DEVID_KEYBOARD,0,'c',0);
    }
  }
}

/* Enable touch events.
 */

void joy2_enable_touch(struct joy2 *joy2,int enable) {
  if (enable) {
    if (joy2->touch_enabled) return;
    joy2->touch_enabled=1;
    switch (egg_event_enable(EGG_EVENT_TOUCH,EGG_EVTSTATE_ENABLED)) {
      case EGG_EVTSTATE_ENABLED:
      case EGG_EVTSTATE_REQUIRED: {
          struct joy2_device *device=joy2_devicev_add(joy2,JOY2_DEVID_TOUCH);
          if (device) {
            joy2->has_touch=1;
            joy2_device_set_name(device,"Touch",-1);
            device->mapping=1; // Calling touch "standard mapping" is the easiest way for our counterparts to process it.
            joy2_report_button(joy2,JOY2_DEVID_TOUCH,0,'c',1);
          } else {
            joy2->has_touch=0;
          }
        } break;
      default: {
          joy2->has_touch=0;
        }
    }
  } else {
    if (!joy2->touch_enabled) return;
    joy2->touch_enabled=0;
    egg_event_enable(EGG_EVENT_TOUCH,EGG_EVTSTATE_DISABLED);
    int p=joy2_devicev_search(joy2,JOY2_DEVID_TOUCH);
    if (p>=0) {
      struct joy2_device *device=joy2->devicev+p;
      joy2_clear_device_state(joy2,device);
      joy2_device_cleanup(device);
      joy2->devicec--;
      memmove(device,device+1,sizeof(struct joy2_device)*(joy2->devicec-p));
      joy2_report_button(joy2,JOY2_DEVID_TOUCH,0,'c',0);
    }
    joy2->touchc=0;
  }
}

/* Test device.
 */

int joy2_device_exists(const struct joy2 *joy2,int devid) {
  return joy2_devicev_get(joy2,devid)?1:0;
}

const char *joy2_device_get_ids(int *vid,int *pid,int *version,int *mapping,const struct joy2 *joy2,int devid) {
  const struct joy2_device *device=joy2_devicev_get(joy2,devid);
  if (!device) return 0;
  if (vid) *vid=device->vid;
  if (pid) *pid=device->pid;
  if (version) *version=device->version;
  if (mapping) *mapping=device->mapping;
  return device->name;
}

int joy2_for_each_device(
  const struct joy2 *joy2,
  int (*cb)(int devid,int vid,int pid,int version,int mapping,const char *name,void *userdata),
  void *userdata
) {
  struct joy2_device *device=joy2->devicev;
  int i=joy2->devicec,err;
  for (;i-->0;device++) {
    if (err=cb(device->devid,device->vid,device->pid,device->version,device->mapping,device->name,userdata)) return err;
  }
  return 0;
}

int joy2_device_get_button(int *btnid,int *mode,const struct joy2 *joy2,int devid,int p) {
  if (p<0) return -1;
  const struct joy2_device *device=joy2_devicev_get(joy2,devid);
  if (!device) return -1;
  if (p>=device->buttonc) return -1;
  const struct joy2_button *button=device->buttonv+p;
  if (btnid) *btnid=button->srcbtnid;
  if (mode) *mode=button->mode;
  return 0;
}
