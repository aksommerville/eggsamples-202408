#include <egg/egg.h>
#include "jlog.h"
#include "bus.h"
#include "joy2.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>

/* Object definition.
 */
 
struct jlog_button {
  int srcbtnid;
  char srcpart;
  int dstbtnid;
};
 
struct jlog_tm {
  int vid,pid,version; // Zero matches anything.
  char *name; // Empty matches anything.
  int namec;
  struct jlog_button *buttonv;
  int buttonc,buttona;
};
 
struct jlog_device {
  int devid;
  int playerid;
  int state;
  struct jlog_button *buttonv;
  int buttonc,buttona;
};
 
struct jlog {
  struct bus *bus;
  int enabled;
  int joy2listener;
  struct jlog_device *devicev;
  int devicec,devicea;
  int *statev; // Indexed by playerid. [0] is the aggregate.
  int statec,statea; // (statec) is always at least 2.
  struct jlog_tm **tmv;
  int tmc,tma;
  int stringid_by_btnix[16];
  int stdbtnid_by_btnix[16];
};

/* Delete.
 */
 
static void jlog_tm_del(struct jlog_tm *tm) {
  if (tm->buttonv) free(tm->buttonv);
}
 
static void jlog_device_cleanup(struct jlog_device *device) {
  if (device->buttonv) free(device->buttonv);
}
 
void jlog_del(struct jlog *jlog) {
  if (!jlog) return;
  // Don't unlisten (joy2listener). joy2 should delete at the same time we do, and that might already have happened.
  if (jlog->devicev) {
    while (jlog->devicec-->0) jlog_device_cleanup(jlog->devicev+jlog->devicec);
    free(jlog->devicev);
  }
  if (jlog->statev) free(jlog->statev);
  if (jlog->tmv) {
    while (jlog->tmc-->0) jlog_tm_del(jlog->tmv[jlog->tmc]);
    free(jlog->tmv);
  }
  free(jlog);
}

/* New.
 */

struct jlog *jlog_new(struct bus *bus) {
  struct jlog *jlog=calloc(1,sizeof(struct jlog));
  if (!jlog) return 0;
  jlog->bus=bus;
  
  // Must start with at least 2 states, for the aggregate and player one.
  if (!(jlog->statev=calloc(sizeof(int),8))) {
    jlog_del(jlog);
    return 0;
  }
  jlog->statea=8;
  jlog->statec=2;
  
  jlog_load_templates(jlog);
  
  return jlog;
}

/* Define player layout.
 */
 
int jlog_define_mapping_(struct jlog *jlog,...) {
  memset(jlog->stringid_by_btnix,0,sizeof(jlog->stringid_by_btnix));
  memset(jlog->stdbtnid_by_btnix,0,sizeof(jlog->stdbtnid_by_btnix));
  va_list vargs;
  va_start(vargs,jlog);
  int btnix=0;
  for (;;btnix++) {
    int stringid=va_arg(vargs,int);
    if (!stringid) break;
    int stdbtnid=va_arg(vargs,int);
    if (btnix>=16) return -1;
    jlog->stringid_by_btnix[btnix]=stringid;
    jlog->stdbtnid_by_btnix[btnix]=stdbtnid;
  }
  return 0;
}

/* Set or get player count (public).
 */
 
int jlog_get_player_count(const struct jlog *jlog) {
  return jlog->statec-1;
}

// After artifically changing nonzero player states, call this to rebuild the aggregate and report any differences.
static void jlog_poke_player_zero(struct jlog *jlog) {
  int nstate=0;
  const int *v=jlog->statev+1;
  int i=jlog->statec-1;
  for (;i-->0;v++) nstate|=*v;
  if (nstate==jlog->statev[0]) return;
  int relevant=nstate|jlog->statev[0];
  int mask=1;
  for (;mask<=relevant;mask<<=1) {
    if (mask&nstate) {
      if (mask&jlog->statev[0]) continue;
      jlog->statev[0]|=mask;
      bus_on_player(jlog->bus,0,mask,1,jlog->statev[0]);
    } else {
      if (!(mask&jlog->statev[0])) continue;
      jlog->statev[0]&=~mask;
      bus_on_player(jlog->bus,0,mask,0,jlog->statev[0]);
    }
  }
}

int jlog_set_player_count(struct jlog *jlog,int playerc) {

  // Ensure the new count is kosher, and allocate room.
  if ((playerc<1)||(playerc>16)) return jlog->statec-1;
  int nc=1+playerc;
  if (nc>jlog->statea) {
    void *nv=realloc(jlog->statev,sizeof(int)*nc);
    if (!nv) return jlog->statec-1;
    jlog->statev=nv;
    jlog->statea=nc;
  }
  
  // If decreasing, we must send OFF events for every bit set on a playerid being removed.
  // Also must zero any device=>player assignments that have become invalid.
  if (jlog->statec>nc) {
    while (jlog->statec>nc) {
      jlog->statec--;
      int state=jlog->statev[jlog->statec];
      if (state) {
        int mask=1;
        for (;mask<=state;mask<<=1) {
          if (state&mask) {
            state&=~mask;
            bus_on_player(jlog->bus,jlog->statec,mask,0,state);
          }
        }
      }
    }
    jlog_poke_player_zero(jlog);
    struct jlog_device *device=jlog->devicev;
    int i=jlog->devicec;
    for (;i-->0;device++) {
      if (device->playerid>=jlog->statec) device->playerid=0;
    }
    
  // Increasing is easy by comparison: Just append zero states until we reach the target.
  } else {
    while (jlog->statec<nc) jlog->statev[jlog->statec++]=0;
  }
  return jlog->statec-1;
}

int jlog_is_player_mapped(const struct jlog *jlog,int playerid) {
  const struct jlog_device *device=jlog->devicev;
  int i=jlog->devicec;
  for (;i-->0;device++) if (device->playerid==playerid) return 1;
  return 0;
}

int jlog_get_player(const struct jlog *jlog,int playerid) {
  if ((playerid<0)||(playerid>jlog->statec)) return 0;
  return jlog->statev[playerid];
}

int jlog_btnid_from_standard(const struct jlog *jlog,int stdbtnid) {
  int p=0; for (;p<16;p++) {
    if (jlog->stdbtnid_by_btnix[p]==stdbtnid) return 1<<p;
  }
  return 0;
}

/* Player id with the fewest devices assigned.
 * It's important that this break ties low: The first playerid we choose must be 1, then 2, and so on.
 * It only gets interesting when you have an improbably high count of devices.
 */
 
static int jlog_loneliest_playerid(const struct jlog *jlog) {
  if (jlog->statec<=2) return 1; // Invalid state, or just one playerid: The answer is one.
  int q=1,best=1,bestc=999;
  for (;q<jlog->statec;q++) {
    int c=0;
    int devicei=jlog->devicec;
    const struct jlog_device *device=jlog->devicev;
    for (;devicei-->0;device++) {
      if (device->devid<1) continue; // Special devices like Keyboard and Touch don't count.
      if (device->playerid==q) c++;
    }
    if (!c) return q; // First zero we find, that's the winner immediately.
    if (c<bestc) {
      best=q;
      bestc=c;
    }
  }
  return best;
}

/* Device list.
 */
 
static int jlog_device_search(const struct jlog *jlog,int devid) {
  int lo=0,hi=jlog->devicec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct jlog_device *q=jlog->devicev+ck;
         if (devid<q->devid) hi=ck;
    else if (devid>q->devid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static struct jlog_device *jlog_device_insert(struct jlog *jlog,int p,int devid) {
  if ((p<0)||(p>jlog->devicec)) return 0;
  if (jlog->devicec>=jlog->devicea) {
    int na=jlog->devicea+8;
    if (na>INT_MAX/sizeof(struct jlog_device)) return 0;
    void *nv=realloc(jlog->devicev,sizeof(struct jlog_device)*na);
    if (!nv) return 0;
    jlog->devicev=nv;
    jlog->devicea=na;
  }
  struct jlog_device *device=jlog->devicev+p;
  memmove(device+1,device,sizeof(struct jlog_device)*(jlog->devicec-p));
  jlog->devicec++;
  memset(device,0,sizeof(struct jlog_device));
  device->devid=devid;
  return device;
}

static struct jlog_device *jlog_device_get(const struct jlog *jlog,int devid) {
  int p=jlog_device_search(jlog,devid);
  if (p<0) return 0;
  return jlog->devicev+p;
}

// Caller should clear state first.
static void jlog_device_remove(struct jlog *jlog,int p) {
  if ((p<0)||(p>=jlog->devicec)) return;
  struct jlog_device *device=jlog->devicev+p;
  jlog_device_cleanup(device);
  jlog->devicec--;
  memmove(device,device+1,sizeof(struct jlog_device)*(jlog->devicec-p));
}

/* Change a player state and trigger callbacks as needed.
 * For (playerid>0), this will also update the aggregate.
 * Aggregate in general reports the most recent change, not the union of all states.
 * Caller is responsible for device states.
 */
 
static void jlog_set_player_button(struct jlog *jlog,int plrid,int btnid,int value) {
  if ((plrid<0)||(plrid>jlog->statec)) return;
  if (value) {
    if (jlog->statev[plrid]&btnid) return;
    jlog->statev[plrid]|=btnid;
    bus_on_player(jlog->bus,plrid,btnid,1,jlog->statev[plrid]);
  } else {
    if (!(jlog->statev[plrid]&btnid)) return;
    jlog->statev[plrid]&=~btnid;
    bus_on_player(jlog->bus,plrid,btnid,0,jlog->statev[plrid]);
  }
  if (plrid) jlog_set_player_button(jlog,0,btnid,value);
}

/* Clear device state. Fires events as needed.
 */
 
static void jlog_device_clear_state(struct jlog *jlog,struct jlog_device *device) {
  if (!device->state) return;
  int mask=1;
  for (;mask<=device->state;mask<<=1) {
    if (!(device->state&mask)) continue;
    device->state&=~mask;
    jlog_set_player_button(jlog,device->playerid,mask,0);
  }
}

/* Device button list.
 */
 
static int jlog_device_buttonv_search(const struct jlog_device *device,int btnid,char part) {
  int lo=0,hi=device->buttonc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct jlog_button *q=device->buttonv+ck;
         if (btnid<q->srcbtnid) hi=ck;
    else if (btnid>q->srcbtnid) lo=ck+1;
    else if (part<q->srcpart) hi=ck;
    else if (part>q->srcpart) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static struct jlog_button *jlog_device_buttonv_insert(struct jlog_device *device,int p,int srcbtnid,char srcpart) {
  if ((p<0)||(p>device->buttonc)) return 0;
  if (device->buttonc>=device->buttona) {
    int na=device->buttona+16;
    if (na>INT_MAX/sizeof(struct jlog_button)) return 0;
    void *nv=realloc(device->buttonv,sizeof(struct jlog_button)*na);
    if (!nv) return 0;
    device->buttonv=nv;
    device->buttona=na;
  }
  struct jlog_button *button=device->buttonv+device->buttonc++;
  button->srcbtnid=srcbtnid;
  button->srcpart=srcpart;
  button->dstbtnid=0;
  return button;
}

/* Button changed.
 */
 
static void jlog_on_button(struct jlog *jlog,struct jlog_device *device,int btnid,char part,int value) {
  
  // Get a player id if we don't have one yet. This is intentionally deferred to the first real event.
  // (devid<1) is for special things like Keyboard and Touch. Those map to player 1 no matter what.
  if (!device->playerid) {
    if (device->devid<1) device->playerid=1;
    else device->playerid=jlog_loneliest_playerid(jlog);
  }
  
  int p=jlog_device_buttonv_search(device,btnid,part);
  if (p<0) return;
  int dstbtnid=device->buttonv[p].dstbtnid;
  if (!dstbtnid) return;
  if (value) {
    if (device->state&dstbtnid) return;
    device->state|=dstbtnid;
  } else {
    if (!(device->state&dstbtnid)) return;
    device->state&=~dstbtnid;
  }
  jlog_set_player_button(jlog,device->playerid,dstbtnid,value);
}

/* Add template.
 */
 
static struct jlog_tm *jlog_tmv_add(struct jlog *jlog,int vid,int pid,int version,const char *name,int namec) {
  if (jlog->tmc>=jlog->tma) {
    int na=jlog->tma+16;
    if (na>INT_MAX/sizeof(void*)) return 0;
    void *nv=realloc(jlog->tmv,sizeof(void*)*na);
    if (!nv) return 0;
    jlog->tmv=nv;
    jlog->tma=na;
  }
  struct jlog_tm *tm=calloc(1,sizeof(struct jlog_tm));
  if (!tm) return 0;
  tm->vid=vid;
  tm->pid=pid;
  tm->version=version;
  if (!name) namec=0; else if (namec<0) { namec=0; while (name[namec]) namec++; }
  if (namec) {
    if (!(tm->name=malloc(namec+1))) {
      free(tm);
      return 0;
    }
    memcpy(tm->name,name,namec);
    tm->name[namec]=0;
    tm->namec=namec;
  }
  jlog->tmv[jlog->tmc++]=tm;
  return tm;
}

/* Template button list.
 */
 
static int jlog_tm_buttonv_search(const struct jlog_tm *tm,int srcbtnid,char srcpart) {
  // When decoding templates, we build up the list by searching.
  // It's overwhelmingly redundant; the input should already be sorted.
  // But that would yield worst-case performance here.
  // So a special check: Is it OOB at the high end?
  if (tm->buttonc<1) return -1;
  const struct jlog_button *last=tm->buttonv+tm->buttonc-1;
  if (srcbtnid>last->srcbtnid) return -tm->buttonc-1;
  if ((srcbtnid==last->srcbtnid)&&(srcpart>last->srcpart)) return -tm->buttonc-1;
  // ...ok we now resume your regularly-scheduled binary search...
  int lo=0,hi=tm->buttonc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct jlog_button *button=tm->buttonv+ck;
         if (srcbtnid<button->srcbtnid) hi=ck;
    else if (srcbtnid>button->srcbtnid) lo=ck+1;
    else if (srcpart<button->srcpart) hi=ck;
    else if (srcpart>button->srcpart) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static int jlog_tm_add_button(struct jlog_tm *tm,int srcbtnid,char srcpart,int dstbtnid) {
  int p=jlog_tm_buttonv_search(tm,srcbtnid,srcpart);
  if (p>=0) return -1;
  p=-p-1;
  if (tm->buttonc>=tm->buttona) {
    int na=tm->buttona+16;
    if (na>INT_MAX/sizeof(struct jlog_button)) return -1;
    void *nv=realloc(tm->buttonv,sizeof(struct jlog_button)*na);
    if (!nv) return -1;
    tm->buttonv=nv;
    tm->buttona=na;
  }
  struct jlog_button *button=tm->buttonv+p;
  memmove(button+1,button,sizeof(struct jlog_button)*(tm->buttonc-p));
  tm->buttonc++;
  button->srcbtnid=srcbtnid;
  button->srcpart=srcpart;
  button->dstbtnid=dstbtnid;
  return 0;
}

/* Unsigned hexadecimal integer tokens with no prefix.
 * For convenience, eval will also consume leading and trailing space; you don't need to measure.
 */
 
static int jlog_hexuint_repr(char *dst,int dsta,int src) {
  unsigned int mask=0xf;
  int dstc=1;
  while (src&~mask) { dstc++; mask|=mask<<4; }
  if (dstc>dsta) return dstc;
  int i=dstc; for (;i-->0;src>>=4) dst[i]="0123456789abcdef"[src&0xf];
  return dstc;
}

static int jlog_hexuint_eval(int *dst,const char *src,int srcc) {
  int srcp=0;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if (srcp>=srcc) return -1;
  *dst=0;
  int digitc=0;
  while (srcp<srcc) {
    int digit;
    if ((src[srcp]>='0')&&(src[srcp]<='9')) digit=src[srcp++]-'0';
    else if ((src[srcp]>='a')&&(src[srcp]<='f')) digit=src[srcp++]-'a'+10;
    else break;
    (*dst)<<=4;
    (*dst)|=digit;
    digitc++;
  }
  if (!digitc) return -1;
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  return srcp;
}

/* Load templates.
 */
 
static int jlog_load_template_text(struct jlog *jlog,const char *src,int srcc) {
  int srcp=0,err;
  
  // First line: VID PID VERSION NAME
  int vid,pid,version;
  if ((err=jlog_hexuint_eval(&vid,src+srcp,srcc-srcp))<1) return -1; srcp+=err;
  if ((err=jlog_hexuint_eval(&pid,src+srcp,srcc-srcp))<1) return -1; srcp+=err;
  if ((err=jlog_hexuint_eval(&version,src+srcp,srcc-srcp))<1) return -1; srcp+=err;
  const char *name=src+srcp;
  int namec=0;
  while ((srcp<srcc)&&(src[srcp++]!=0x0a)) namec++;
  
  struct jlog_tm *tm=jlog_tmv_add(jlog,vid,pid,version,name,namec);
  if (!tm) return -1;
  
  // Buttons block: [(SRCBTNID SRCPART DSTBTNID)...]
  while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
  if ((srcp>=srcc)||(src[srcp++]!='[')) return -1;
  while (1) {
    if (srcp>=srcc) return -1;
    if (src[srcp]==']') return srcp+1;
    
    if (src[srcp++]!='(') return -1;
    int srcbtnid,srcpart,dstbtnid;
    if ((err=jlog_hexuint_eval(&srcbtnid,src+srcp,srcc-srcp))<1) return -1; srcp+=err;
    if ((srcp>=srcc)||(src[srcp]<'a')||(src[srcp]>'z')) return -1;
    srcpart=src[srcp++];
    if ((err=jlog_hexuint_eval(&dstbtnid,src+srcp,srcc-srcp))<1) return -1; srcp+=err;
    if ((srcp>=srcc)||(src[srcp++]!=')')) return -1;
    if (jlog_tm_add_button(tm,srcbtnid,srcpart,dstbtnid)<0) return -1;
  }
}
 
static int jlog_load_templates_text(struct jlog *jlog,const char *src,int srcc) {
  int srcp=0;
  while (srcp<srcc) {
    if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
    int err=jlog_load_template_text(jlog,src+srcp,srcc-srcp);
    if (err<=0) return -1;
    srcp+=err;
  }
  return 0;
}
 
int jlog_load_templates(struct jlog *jlog) {
  while (jlog->tmc>0) { jlog->tmc--; jlog_tm_del(jlog->tmv[jlog->tmc]); }
  int srca=1024,srcc=0;
  char *src=malloc(srca);
  if (!src) return -1;
  while (1) {
    if ((srcc=egg_store_get(src,srca,"joymap",6))<=srca) break;
    void *nv=realloc(src,srca=srcc);
    if (!nv) {
      free(src);
      return -1;
    }
    src=nv;
  }
  int err=-1;
  if (srcc>=0) err=jlog_load_templates_text(jlog,src,srcc);
  free(src);
  return err;
}

/* Store templates.
 */
 
static int jlog_tm_encode(char *dst,int dsta,const struct jlog_tm *tm) {
  int dstc=0;
  
  // First line: VID PID VERSION NAME
  dstc+=jlog_hexuint_repr(dst+dstc,dsta-dstc,tm->vid);
  if (dstc<dsta) dst[dstc]=' '; dstc++;
  dstc+=jlog_hexuint_repr(dst+dstc,dsta-dstc,tm->pid);
  if (dstc<dsta) dst[dstc]=' '; dstc++;
  dstc+=jlog_hexuint_repr(dst+dstc,dsta-dstc,tm->version);
  if (dstc<dsta) dst[dstc]=' '; dstc++;
  if (dstc<=dsta-tm->namec) memcpy(dst+dstc,tm->name,tm->namec);
  dstc+=tm->namec;
  if (dstc<dsta) dst[dstc]=0x0a; dstc++;
  
  // Second line: [(SRCBTNID SRCPART DSTBTNID)...]
  if (dstc<dsta) dst[dstc]='['; dstc++;
  const struct jlog_button *button=tm->buttonv;
  int i=tm->buttonc;
  for (;i-->0;button++) {
    if (dstc<dsta) dst[dstc]='('; dstc++;
    dstc+=jlog_hexuint_repr(dst+dstc,dsta-dstc,button->srcbtnid);
    if (dstc<dsta) dst[dstc]=' '; dstc++;
    if (dstc<dsta) dst[dstc]=button->srcpart; dstc++;
    if (dstc<dsta) dst[dstc]=' '; dstc++;
    dstc+=jlog_hexuint_repr(dst+dstc,dsta-dstc,button->dstbtnid);
    if (dstc<dsta) dst[dstc]=')'; dstc++;
  }
  if (dstc<dsta) dst[dstc]=']'; dstc++;
  if (dstc<dsta) dst[dstc]=0x0a; dstc++;
  
  return dstc;
}
 
static int jlog_encode_templates(void *dstpp,const struct jlog *jlog) {
  int dstc=0,dsta=1024;
  char *dst=malloc(dsta);
  if (!dst) return -1;
  int i=0; for (;i<jlog->tmc;i++) {
    struct jlog_tm *tm=jlog->tmv[i];
    while (1) {
      int err=jlog_tm_encode(dst+dstc,dsta-dstc,tm);
      if (err<0) {
        free(dst);
        return -1;
      }
      if (dstc<=dsta-err) {
        dstc+=err;
        break;
      }
      dsta=(dstc+1024)&~1023;
      void *nv=realloc(dst,dsta);
      if (!nv) {
        free(dst);
        return -1;
      }
      dst=nv;
    }
  }
  *(void**)dstpp=dst;
  return dstc;
}
 
int jlog_store_templates(struct jlog *jlog) {
  char *src=0;
  int srcc=jlog_encode_templates(&src,jlog);
  if (srcc<0) return -1;
  int err=egg_store_set("joymap",6,src,srcc);
  free(src);
  return err;
}

/* Apply template to new device.
 */
 
static int jlog_device_apply_tm(struct jlog *jlog,struct jlog_device *device,const struct jlog_tm *tm) {
  if (tm->buttonc<1) return 0;
  struct jlog_button *nv=malloc(sizeof(struct jlog_button)*tm->buttonc);
  if (!nv) return -1;
  memcpy(nv,tm->buttonv,sizeof(struct jlog_button)*tm->buttonc);
  if (device->buttonv) free(device->buttonv);
  device->buttonv=nv;
  device->buttonc=tm->buttonc;
  device->buttona=tm->buttonc;
  return 0;
}

/* Populate a template by guessing.
 */
 
static const struct jlog_keymap {
  uint8_t usage;
  uint8_t stdbtnid;
} jlog_keymapv[]={
  {0x04,14}, // a => left
  {0x06, 1}, // c => east
  {0x07,15}, // d => right
  {0x16,13}, // s => down
  {0x19, 3}, // v => north
  {0x1a,12}, // w => up
  {0x1b, 2}, // x => west
  {0x1d, 0}, // z => south
  {0x28, 9}, // enter => aux1
  {0x2a, 7}, // backspace => r2
  {0x2b, 4}, // tab => l1
  {0x2c, 0}, // space => south
  {0x2f,16}, // open bracket => aux3
  {0x30, 8}, // close bracket => aux2
  {0x31, 5}, // backslash => r1
  {0x35, 6}, // grave => l2
  {0x36, 2}, // comma => west
  {0x37, 1}, // dot => east
  {0x38, 3}, // slash => north
  {0x4f,15}, // right => right
  {0x50,14}, // left => left
  {0x51,13}, // down => down
  {0x52,12}, // up => up
  {0x54, 9}, // kp slash => aux1
  {0x55, 8}, // kp star => aux2
  {0x56,16}, // kp dash => aux3
  {0x57, 1}, // kp plus => east
  {0x58, 2}, // kp enter => west
  {0x59, 6}, // kp 1 => l2
  {0x5a,13}, // kp 2 => down
  {0x5b, 7}, // kp 3 => r2
  {0x5c,14}, // kp 4 => left
  {0x5d,13}, // kp 5 => down
  {0x5e,15}, // kp 6 => right
  {0x5f, 4}, // kp 7 => l1
  {0x60,12}, // kp 8 => up
  {0x61, 5}, // kp 9 => r1
  {0x62, 0}, // kp 0 => south
  {0x63, 3}, // kp dot => north
};
 
static int jlog_tm_generate(struct jlog *jlog,struct jlog_tm *tm,int devid,int mapping) {

  /* If it uses Standard Mapping, we already know enough to map it.
   */
  if (mapping==1) {
    int btnix=0; for (;btnix<16;btnix++) {
      int srcbtnid=jlog->stdbtnid_by_btnix[btnix];
      if (!srcbtnid) continue;
      if (jlog_tm_add_button(tm,srcbtnid,'b',1<<btnix)<0) return -1;
      switch (srcbtnid) { // If the dpad is mapped, alias left stick to it.
        case 0x8c: if (jlog_tm_add_button(tm,0x41,'l',1<<btnix)<0) return -1; break;
        case 0x8d: if (jlog_tm_add_button(tm,0x41,'h',1<<btnix)<0) return -1; break;
        case 0x8e: if (jlog_tm_add_button(tm,0x40,'l',1<<btnix)<0) return -1; break;
        case 0x8f: if (jlog_tm_add_button(tm,0x40,'h',1<<btnix)<0) return -1; break;
      }
    }
    return 0;
  }
  
  /* If it's the System Keyboard, compare the static defaults above, to stdbtnid_by_btnix.
   */
  if (devid==JOY2_DEVID_KEYBOARD) {
    const struct jlog_keymap *keymap=jlog_keymapv;
    int i=sizeof(jlog_keymapv)/sizeof(jlog_keymapv[0]);
    for (;i-->0;keymap++) {
      int dstbtnid=0;
      int btnix=0; for (;btnix<16;btnix++) {
        if (jlog->stdbtnid_by_btnix[btnix]==(0x80|keymap->stdbtnid)) {
          dstbtnid=1<<btnix;
          break;
        }
      }
      if (dstbtnid) {
        if (jlog_tm_add_button(tm,0x00070000|keymap->usage,'b',dstbtnid)<0) return -1;
      }
    }
    return 0;
  }
  
  /* Joysticks or whatever.
   * joy2 has already taken a crack at it, and has identified axes and hats.
   * We'll use that first.
   * joy2 did not record the two-state buttons, so we'll ask Egg for those.
   */
  int btnid_left=0,btnid_right=0,btnid_up=0,btnid_down=0;
  int i=0; for (;i<16;i++) switch (jlog->stdbtnid_by_btnix[i]) {
    case 0x8c: btnid_up=1<<i; break;
    case 0x8d: btnid_down=1<<i; break;
    case 0x8e: btnid_left=1<<i; break;
    case 0x8f: btnid_right=1<<i; break;
  }
  int xc=0,yc=0;
  if (btnid_left&&btnid_right&&btnid_up&&btnid_down) {
    struct joy2 *joy2=bus_get_joy2(jlog->bus);
    int p=0; for (;;p++) {
      int btnid=0,mode=0;
      if (joy2_device_get_button(&btnid,&mode,joy2,devid,p)<0) break;
      if (!btnid) break;
      switch (mode) {
        case JOY2_MODE_HAT: {
            jlog_tm_add_button(tm,btnid,'w',btnid_left);
            jlog_tm_add_button(tm,btnid,'e',btnid_right);
            jlog_tm_add_button(tm,btnid,'n',btnid_up);
            jlog_tm_add_button(tm,btnid,'s',btnid_down);
            xc++;
            yc++;
          } break;
        case JOY2_MODE_AXIS: {
            if (xc<=yc) {
              jlog_tm_add_button(tm,btnid,'l',btnid_left);
              jlog_tm_add_button(tm,btnid,'h',btnid_right);
              xc++;
            } else {
              jlog_tm_add_button(tm,btnid,'l',btnid_up);
              jlog_tm_add_button(tm,btnid,'h',btnid_down);
              yc++;
            }
          } break;
      }
    }
  }
  int count_by_btnix[16];
  for (i=0;i<16;i++) {
    if (jlog->stringid_by_btnix[i]) {
      count_by_btnix[i]=0;
      if ((1<<i==btnid_left)||(1<<i==btnid_right)) count_by_btnix[i]=xc;
      else if ((1<<i==btnid_up)||(1<<i==btnid_down)) count_by_btnix[i]=yc;
    } else {
      count_by_btnix[i]=INT_MAX;
    }
  }
  int p=0; for (;;p++) {
    int btnid=0,hidusage=0,lo=0,hi=0,value=0;
    egg_input_device_get_button(&btnid,&hidusage,&lo,&hi,&value,devid,p);
    if (!btnid) break;
    if (lo||value) continue; // Only interested if low and default are both zero.
    int range=hi-lo+1;
    if (range==8) continue; // Not interested in hats.
    
    int dstbtnid=1,btnix=0;
    int bestc=INT_MAX;
    for (i=0;i<16;i++) {
      if (count_by_btnix[i]<bestc) {
        btnix=i;
        dstbtnid=1<<i;
        bestc=count_by_btnix[i];
      }
    }
    count_by_btnix[btnix]++;
    
    jlog_tm_add_button(tm,btnid,'b',dstbtnid);
  }

  return 0;
}

/* Find or generate a template.
 * Either way, if we return nonzero, it's WEAK and owned by jlog.
 */
 
static struct jlog_tm *jlog_find_or_generate_template(struct jlog *jlog,int devid,int vid,int pid,int version,int mapping,const char *name) {
  
  // Find a match among existing templates.
  int namec=0; if (name) while (name[namec]) namec++;
  int i=0; for (;i<jlog->tmc;i++) {
    struct jlog_tm *tm=jlog->tmv[i];
    if (tm->vid&&(tm->vid!=vid)) continue;
    if (tm->pid&&(tm->pid!=pid)) continue;
    if (tm->version&&(tm->version!=version)) continue;
    if (tm->namec&&((tm->namec!=namec)||memcmp(tm->name,name,namec))) continue;
    return tm;
  }
  
  // Prepare a blank template and call out to populate it.
  struct jlog_tm *tm=jlog_tmv_add(jlog,vid,pid,version,name,namec);
  if (!tm) {
    egg_log("Failed to generate mapping for device '%s'",name);
    return 0;
  }
  jlog_tm_generate(jlog,tm,devid,mapping);
  jlog_store_templates(jlog);
  return tm;
}

/* Device connected.
 */
 
static void jlog_on_connect(struct jlog *jlog,int devid) {
  
  // Find the insertion point but don't create the record yet.
  int p=jlog_device_search(jlog,devid);
  if (p>=0) return; // oops?
  p=-p-1;
  int vid=0,pid=0,version=0,mapping=0;
  const char *name=joy2_device_get_ids(&vid,&pid,&version,&mapping,bus_get_joy2(jlog->bus),devid);
  
  // Find a template or generate one.
  // If we fail to -- perfectly likely -- then don't bother recording the device.
  struct jlog_tm *tm=jlog_find_or_generate_template(jlog,devid,vid,pid,version,mapping,name);
  if (!tm) return;
  
  // Create a new device record and apply this template.
  struct jlog_device *device=jlog_device_insert(jlog,p,devid);
  if (!device) return;
  jlog_device_apply_tm(jlog,device,tm);
  
  // Do not assign (device->playerid) at this time, leave it zero.
  // We'll assign those the first time an actual state change comes in.
  // That way, if you're like me and have multiple joysticks connected, you ask "which is player one?" and you'll guess right every time!
}

/* Device disconnected.
 */
 
static void jlog_on_disconnect(struct jlog *jlog,int devid) {
  int p=jlog_device_search(jlog,devid);
  if (p<0) return;
  jlog_device_clear_state(jlog,jlog->devicev+p);
  jlog_device_remove(jlog,p);
}

/* joy2 event.
 */
 
static void jlog_on_event(int devid,int btnid,char part,int value,void *userdata) {
  struct jlog *jlog=userdata;
  if (!btnid&&(part=='c')) {
    if (value) {
      jlog_on_connect(jlog,devid);
    } else {
      jlog_on_disconnect(jlog,devid);
    }
  } else {
    struct jlog_device *device=jlog_device_get(jlog,devid);
    if (!device) return;
    jlog_on_button(jlog,device,btnid,part,value);
  }
}

/* At enable, connect all the devices already present in joy2.
 * Nothing special here, we just pretend we got "0:c=1" for each device.
 */
 
static int jlog_cb_connect_existing(int devid,int vid,int pid,int version,int mapping,const char *name,void *userdata) {
  jlog_on_connect(userdata,devid);
  return 0;
}
 
static void jlog_connect_existing_devices(struct jlog *jlog) {
  joy2_for_each_device(bus_get_joy2(jlog->bus),jlog_cb_connect_existing,jlog);
}

/* Enable.
 */

void jlog_enable(struct jlog *jlog,int enable) {
  if (enable) {
    if (jlog->enabled) return;
    jlog->enabled=1;
    jlog_connect_existing_devices(jlog);
    if (jlog->joy2listener<1) {
      jlog->joy2listener=joy2_listen(bus_get_joy2(jlog->bus),jlog_on_event,jlog);
    }
  } else {
    if (!jlog->enabled) return;
    jlog->enabled=0;
    joy2_unlisten(bus_get_joy2(jlog->bus),jlog->joy2listener);
    jlog->joy2listener=0;
    // Simulate disconnection of all devices.
    while (jlog->devicec>0) jlog_on_disconnect(jlog,jlog->devicev[jlog->devicec-1].devid);
  }
}
