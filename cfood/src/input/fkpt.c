#include <egg/egg.h>
#include "fkpt.h"
#include <stdlib.h>
#include <math.h>
#include "bus.h"
#include "jlog.h"

/* Object definition.
 */
 
struct fkpt {
  struct bus *bus;
  int enabled;
  int screenw,screenh;
  int x,y;
  int button1;
  int dx,dy;
  double speed; // px/s. Clamps low at 1 px/frame regardless of the elapsed time.
  int texid; // WEAK
  int srcx,srcy,srcw,srch;
  int srcxform;
  int hotx,hoty;
  int pvstate;
  int btnidl,btnidr,btnidu,btnidd,btnidb;
};

/* Delete.
 */

void fkpt_del(struct fkpt *fkpt) {
  if (!fkpt) return;
  free(fkpt);
}

/* New.
 */

struct fkpt *fkpt_new(struct bus *bus) {
  struct fkpt *fkpt=calloc(1,sizeof(struct fkpt));
  if (!fkpt) return 0;
  fkpt->bus=bus;
  egg_texture_get_header(&fkpt->screenw,&fkpt->screenh,0,1);
  fkpt->speed=fkpt->screenw/2.0; // Default speed for joysticks is 2 seconds per screen width.
  return fkpt;
}

/* Set cursor.
 */
 
void fkpt_set_cursor(
  struct fkpt *fkpt,
  int texid,int x,int y,int w,int h,int xform,
  int hotx,int hoty
) {
  fkpt->texid=texid;
  fkpt->srcx=x;
  fkpt->srcy=y;
  if (w||h) {
    fkpt->srcw=w;
    fkpt->srch=h;
  } else {
    egg_texture_get_header(&fkpt->srcw,&fkpt->srch,0,texid);
    fkpt->srcw-=x;
    fkpt->srch-=y;
  }
  fkpt->srcxform=xform;
  fkpt->hotx=hotx;
  fkpt->hoty=hoty;
  if (fkpt->enabled) {
    if (fkpt->texid) egg_show_cursor(0);
    else egg_show_cursor(1);
  }
}

/* Set position.
 */
 
static void fkpt_set_position(struct fkpt *fkpt,int x,int y) {
  if (x<0) x=0; else if (x>=fkpt->screenw) x=fkpt->screenw-1;
  if (y<0) y=0; else if (y>=fkpt->screenh) y=fkpt->screenh-1;
  if ((x==fkpt->x)&&(y==fkpt->y)) return;
  fkpt->x=x;
  fkpt->y=y;
  bus_on_mmotion(fkpt->bus,x,y);
}

/* Set button.
 */
 
static void fkpt_set_button(struct fkpt *fkpt,int btnid,int value) {
  // We track button 1, and can change it artificially. All other buttons pass through verbatim.
  if (btnid==1) {
    if (value) {
      if (fkpt->button1) return;
      fkpt->button1=1;
    } else {
      if (!fkpt->button1) return;
      fkpt->button1=0;
    }
  }
  bus_on_mbutton(fkpt->bus,btnid,value,fkpt->x,fkpt->y);
}

/* Touch.
 */
 
static void fkpt_on_touch(struct fkpt *fkpt,int touchid,int state,int x,int y) {
  fkpt_set_position(fkpt,x,y);
  switch (state) {
    case 1: fkpt_set_button(fkpt,1,1); break;
    case 0: fkpt_set_button(fkpt,1,0); break;
  }
}

/* Event.
 */

void fkpt_event(struct fkpt *fkpt,const struct egg_event *event) {
  switch (event->type) {
    case EGG_EVENT_MMOTION: fkpt_set_position(fkpt,event->v[0],event->v[1]); break;
    case EGG_EVENT_MBUTTON: fkpt_set_position(fkpt,event->v[2],event->v[3]); fkpt_set_button(fkpt,event->v[0],event->v[1]); break;
    case EGG_EVENT_MWHEEL: fkpt_set_position(fkpt,event->v[2],event->v[3]); bus_on_mwheel(fkpt->bus,fkpt->x,fkpt->y,event->v[2],event->v[3]); break;
    case EGG_EVENT_TOUCH: fkpt_on_touch(fkpt,event->v[0],event->v[1],event->v[2],event->v[3]); break;
  }
}

/* Update.
 */
 
void fkpt_update(struct fkpt *fkpt,double elapsed) {

  int state=jlog_get_player(bus_get_jlog(fkpt->bus),0);
  if (state!=fkpt->pvstate) {
    #define BTN(btnid,on,off) \
      if ((state&fkpt->btnid)&&!(fkpt->pvstate&fkpt->btnid)) on; \
      else if (!(state&fkpt->btnid)&&(fkpt->pvstate&fkpt->btnid)) off;
    BTN(btnidl,fkpt->dx=-1,if (fkpt->dx<0) fkpt->dx=0)
    BTN(btnidr,fkpt->dx= 1,if (fkpt->dx>0) fkpt->dx=0)
    BTN(btnidu,fkpt->dy=-1,if (fkpt->dy<0) fkpt->dy=0)
    BTN(btnidd,fkpt->dy= 1,if (fkpt->dy>0) fkpt->dy=0)
    BTN(btnidb,fkpt_set_button(fkpt,1,1),fkpt_set_button(fkpt,1,0))
    #undef BTN
    fkpt->pvstate=state;
  }
  
  if (fkpt->dx||fkpt->dy) {
    int dx=0,dy=0;
    if (fkpt->dx&&(!(dx=fkpt->dx*lround(elapsed*fkpt->speed)))) dx=fkpt->dx;
    if (fkpt->dy&&(!(dy=fkpt->dy*lround(elapsed*fkpt->speed)))) dy=fkpt->dy;
    fkpt_set_position(fkpt,fkpt->x+dx,fkpt->y+dy);
  }
}

/* Render.
 */
 
void fkpt_render(struct fkpt *fkpt) {
  if (!fkpt->enabled) return;
  if (fkpt->texid) {
    int dstx=fkpt->x-fkpt->hotx;
    int dsty=fkpt->y-fkpt->hoty;
    egg_draw_decal(1,fkpt->texid,dstx,dsty,fkpt->srcx,fkpt->srcy,fkpt->srcw,fkpt->srch,fkpt->srcxform);
  }
}

/* Enable.
 */

void fkpt_enable(struct fkpt *fkpt,int enable) {
  if (enable) {
    if (fkpt->enabled) return;
    fkpt->enabled=1;
    if (fkpt->texid) egg_show_cursor(0);
    else egg_show_cursor(1);
    egg_event_enable(EGG_EVENT_MMOTION,EGG_EVTSTATE_ENABLED);
    egg_event_enable(EGG_EVENT_MBUTTON,EGG_EVTSTATE_ENABLED);
    egg_event_enable(EGG_EVENT_MWHEEL,EGG_EVTSTATE_ENABLED);
    egg_event_enable(EGG_EVENT_TOUCH,EGG_EVTSTATE_ENABLED);
    struct jlog *jlog=bus_get_jlog(fkpt->bus);
    fkpt->btnidl=jlog_btnid_from_standard(jlog,STDBTN_LEFT);
    fkpt->btnidr=jlog_btnid_from_standard(jlog,STDBTN_RIGHT);
    fkpt->btnidu=jlog_btnid_from_standard(jlog,STDBTN_UP);
    fkpt->btnidd=jlog_btnid_from_standard(jlog,STDBTN_DOWN);
    int btnidv[]={STDBTN_SOUTH,STDBTN_EAST,STDBTN_R1,STDBTN_L1,STDBTN_LP,STDBTN_WEST,STDBTN_NORTH,STDBTN_R2,STDBTN_L2,STDBTN_RP,STDBTN_AUX1,STDBTN_AUX2,STDBTN_AUX3,0};
    const int *btnid=btnidv;
    for (;*btnid;btnid++) {
      if (fkpt->btnidb=jlog_btnid_from_standard(jlog,*btnid)) break;
    }
  } else {
    if (!fkpt->enabled) return;
    fkpt->enabled=0;
    egg_event_enable(EGG_EVENT_MMOTION,EGG_EVTSTATE_DISABLED);
    egg_event_enable(EGG_EVENT_MBUTTON,EGG_EVTSTATE_DISABLED);
    egg_event_enable(EGG_EVENT_MWHEEL,EGG_EVTSTATE_DISABLED);
    egg_event_enable(EGG_EVENT_TOUCH,EGG_EVTSTATE_DISABLED);
  }
}
