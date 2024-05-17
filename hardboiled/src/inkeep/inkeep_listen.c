#include "inkeep_internal.h"

/* Broadcast to event listeners.
 * Do not retain a pointer in (inkeep.listenerv) across callbacks.
 * Retaining an index is OK; content won't move.
 */
 
#define BROADCAST(tag,...) { \
  int i=inkeep.listenerc; \
  while (i-->0) { \
    struct inkeep_listener *listener=inkeep.listenerv+i; \
    if (listener->inkevtype!=INKEVTYPE_##tag) continue; \
    ((inkev_##tag)listener->cb)(__VA_ARGS__,listener->userdata); \
  } \
}
void inkeep_broadcast_raw(const union egg_event *event) BROADCAST(RAW,event)
void inkeep_broadcast_joy(int plrid,int btnid,int value,int state) BROADCAST(JOY,plrid,btnid,value,state)
void inkeep_broadcast_text(int codepoint) BROADCAST(TEXT,codepoint)
void inkeep_broadcast_touch(int state,int x,int y) BROADCAST(TOUCH,state,x,y)
void inkeep_broadcast_mmotion(int x,int y) BROADCAST(MMOTION,x,y)
void inkeep_broadcast_mbutton(int btnid,int value,int x,int y) BROADCAST(MBUTTON,btnid,value,x,y)
void inkeep_broadcast_mwheel(int dx,int dy,int x,int y) BROADCAST(MWHEEL,dx,dy,x,y)
void inkeep_broadcast_pov(int dx,int dy,int rx,int ry,int buttons) BROADCAST(POV,dx,dy,rx,ry,buttons)
void inkeep_broadcast_accel(int x,int y,int z) BROADCAST(ACCEL,x,y,z)
#undef BROADCAST

/* Remove event listeners.
 * There can be more than one per id.
 * Also, we are not allowed to move content here, just neutralize matching listeners.
 */

void inkeep_unlisten(int id) {
  int i=inkeep.listenerc;
  struct inkeep_listener *listener=inkeep.listenerv;
  for (;i-->0;listener++) {
    if (listener->id!=id) continue;
    memset(listener,0,sizeof(struct inkeep_listener));
  }
}

/* Add listener, private.
 * Caller must set inkevtype.
 */
 
static struct inkeep_listener *inkeep_listener_add(void *cb,void *userdata) {
  if (!cb) return 0;
  if (inkeep.id_next<1) return 0;
  if (inkeep.listenerc>=inkeep.listenera) {
    int na=inkeep.listenera+16;
    if (na>INT_MAX/sizeof(struct inkeep_listener)) return 0;
    void *nv=realloc(inkeep.listenerv,sizeof(struct inkeep_listener)*na);
    if (!nv) return 0;
    inkeep.listenerv=nv;
    inkeep.listenera=na;
  }
  struct inkeep_listener *listener=inkeep.listenerv+inkeep.listenerc++;
  listener->id=inkeep.id_next++;
  listener->cb=cb;
  listener->userdata=userdata;
  listener->inkevtype=0;
  return listener;
}

/* Add event listeners, public.
 */

int inkeep_listen_raw(void (*cb)(const union egg_event *event,void *userdata),void *userdata) {
  struct inkeep_listener *listener=inkeep_listener_add(cb,userdata);
  if (!listener) return -1;
  listener->inkevtype=INKEVTYPE_RAW;
  return listener->id;
}

int inkeep_listen_joy(void (*cb)(int plrid,int btnid,int value,int state,void *userdata),void *userdata) {
  struct inkeep_listener *listener=inkeep_listener_add(cb,userdata);
  if (!listener) return -1;
  listener->inkevtype=INKEVTYPE_JOY;
  return listener->id;
}

int inkeep_listen_text(void (*cb)(int codepoint,void *userdata),void *userdata) {
  struct inkeep_listener *listener=inkeep_listener_add(cb,userdata);
  if (!listener) return -1;
  listener->inkevtype=INKEVTYPE_TEXT;
  return listener->id;
}

int inkeep_listen_touch(void (*cb)(int state,int x,int y,void *userdata),void *userdata) {
  struct inkeep_listener *listener=inkeep_listener_add(cb,userdata);
  if (!listener) return -1;
  listener->inkevtype=INKEVTYPE_TOUCH;
  return listener->id;
}

int inkeep_listen_mouse(
  void (*cb_motion)(int x,int y,void *userdata),
  void (*cb_button)(int btnid,int value,int x,int y,void *userdata),
  void (*cb_wheel)(int dx,int dy,int x,int y,void *userdata),
  void *userdata
) {
  struct inkeep_listener *listener=0;
  int id=0;
  if (cb_motion) {
    if (!(listener=inkeep_listener_add(cb_motion,userdata))) return -1;
    id=listener->id;
    listener->inkevtype=INKEVTYPE_MMOTION;
  }
  if (cb_button) {
    if (!(listener=inkeep_listener_add(cb_button,userdata))) { inkeep_unlisten(id); return -1; }
    if (id) listener->id=id;
    else id=listener->id;
    listener->inkevtype=INKEVTYPE_MBUTTON;
  }
  if (cb_wheel) {
    if (!(listener=inkeep_listener_add(cb_wheel,userdata))) { inkeep_unlisten(id); return -1; }
    if (id) listener->id=id;
    else id=listener->id;
    listener->inkevtype=INKEVTYPE_MWHEEL;
  }
  if (!id) return -1;
  return id;
}

int inkeep_listen_pov(void (*cb)(int dx,int dy,int rx,int ry,int buttons,void *userdata),void *userdata) {
  struct inkeep_listener *listener=inkeep_listener_add(cb,userdata);
  if (!listener) return -1;
  listener->inkevtype=INKEVTYPE_POV;
  return listener->id;
}

int inkeep_listen_accel(void (*cb)(int x,int y,int z,void *userdata),void *userdata) {
  struct inkeep_listener *listener=inkeep_listener_add(cb,userdata);
  if (!listener) return -1;
  listener->inkevtype=INKEVTYPE_ACCEL;
  return listener->id;
}

int inkeep_listen_all(
  void (*cb_raw)(const union egg_event *event,void *userdata),
  void (*cb_joy)(int plrid,int btnid,int value,int state,void *userdata),
  void (*cb_text)(int codepoint,void *userdata),
  void (*cb_touch)(int state,int x,int y,void *userdata),
  void (*cb_mmotion)(int x,int y,void *userdata),
  void (*cb_mbutton)(int btnid,int value,int x,int y,void *userdata),
  void (*cb_mwheel)(int dx,int dy,int x,int y,void *userdata),
  void (*cb_pov)(int dx,int dy,int rx,int ry,int buttons,void *userdata),
  void (*cb_accel)(int x,int y,int z,void *userdata),
  void  *userdata
) {
  struct inkeep_listener *listener=0;
  int id=0;
  if (cb_raw) {
    if (!(listener=inkeep_listener_add(cb_raw,userdata))) return -1;
    listener->id=id;
    listener->inkevtype=INKEVTYPE_RAW;
  }
  if (cb_joy) {
    if (!(listener=inkeep_listener_add(cb_joy,userdata))) { inkeep_unlisten(id); return -1; }
    if (id) listener->id=id;
    else id=listener->id;
    listener->inkevtype=INKEVTYPE_JOY;
  }
  if (cb_text) {
    if (!(listener=inkeep_listener_add(cb_text,userdata))) { inkeep_unlisten(id); return -1; }
    if (id) listener->id=id;
    else id=listener->id;
    listener->inkevtype=INKEVTYPE_TEXT;
  }
  if (cb_touch) {
    if (!(listener=inkeep_listener_add(cb_touch,userdata))) { inkeep_unlisten(id); return -1; }
    if (id) listener->id=id;
    else id=listener->id;
    listener->inkevtype=INKEVTYPE_TOUCH;
  }
  if (cb_mmotion) {
    if (!(listener=inkeep_listener_add(cb_mmotion,userdata))) { inkeep_unlisten(id); return -1; }
    if (id) listener->id=id;
    else id=listener->id;
    listener->inkevtype=INKEVTYPE_MMOTION;
  }
  if (cb_mbutton) {
    if (!(listener=inkeep_listener_add(cb_mbutton,userdata))) { inkeep_unlisten(id); return -1; }
    if (id) listener->id=id;
    else id=listener->id;
    listener->inkevtype=INKEVTYPE_MBUTTON;
  }
  if (cb_mwheel) {
    if (!(listener=inkeep_listener_add(cb_mwheel,userdata))) { inkeep_unlisten(id); return -1; }
    if (id) listener->id=id;
    else id=listener->id;
    listener->inkevtype=INKEVTYPE_MWHEEL;
  }
  if (cb_pov) {
    if (!(listener=inkeep_listener_add(cb_pov,userdata))) { inkeep_unlisten(id); return -1; }
    if (id) listener->id=id;
    else id=listener->id;
    listener->inkevtype=INKEVTYPE_POV;
  }
  if (cb_accel) {
    if (!(listener=inkeep_listener_add(cb_accel,userdata))) { inkeep_unlisten(id); return -1; }
    if (id) listener->id=id;
    else id=listener->id;
    listener->inkevtype=INKEVTYPE_ACCEL;
  }
  if (!id) return -1;
  return id;
}
