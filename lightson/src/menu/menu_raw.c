#include <egg/egg.h>
#include <stdarg.h>
#include "menu.h"
#include "stdlib/egg-stdlib.h"
#include "util/tile_renderer.h"
#include "util/font.h"

#define ABORT_IDLE_TIME 10.0
#define ABORT_IDLE_WARNING_TIME 3.0

/* Object definition.
 */
 
struct menu_raw {
  struct menu hdr;
  char *log;
  int logc,loga;
  int rowh;
  int rowc;
  int logtexid; // Only if font in play.
  int logtexdirty; // ''
  void *logbits; // '', we only need it during redraw but no sense reallocating every time. RGBA.
  int logbitsstride;
  double abortclock; // Counts up.
};

#define MENU ((struct menu_raw*)menu)

/* Cleanup.
 */
 
static void _raw_del(struct menu *menu) {
  if (MENU->log) free(MENU->log);
  if (MENU->logtexid) egg_texture_del(MENU->logtexid);
  if (MENU->logbits) free(MENU->logbits);
  lightson_default_event_mask(0);
}

/* Add one line of raw text to the log.
 * Replaces any LF in the input.
 * Drops a line from the front if full.
 */
 
static void raw_append_line(struct menu *menu,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  while (srcc&&((unsigned char)src[srcc-1]<=0x20)) srcc--;
  
  // If we're at capacity, drop the first line.
  int linec=0,i=MENU->logc;
  const char *q=MENU->log;
  for (;i-->0;q++) if (*q==0x0a) linec++;
  if (linec>=MENU->rowc) {
    int nlp=-1;
    for (i=0;i<MENU->logc;i++) if (MENU->log[i]==0x0a) { nlp=i; break; }
    nlp++;
    MENU->logc-=nlp;
    memmove(MENU->log,MENU->log+nlp,MENU->logc);
  }
  
  // Grow buffer if needed. And add one for the LF.
  int nc=MENU->logc+srcc+1;
  if (nc>MENU->loga) {
    if (nc>32768) return; // safety, in case they try to write a megabytes-long line
    int na=(nc+1024)&~1023;
    void *nv=realloc(MENU->log,na);
    if (!nv) return;
    MENU->log=nv;
    MENU->loga=na;
  }
  
  // Append.
  memcpy(MENU->log+MENU->logc,src,srcc);
  MENU->logc+=srcc;
  MENU->log[MENU->logc++]=0x0a;
  
  MENU->logtexdirty=1;
}

/* Add formatted text to the log.
 * This is not printf! Each format unit is exactly one char:
 *  - x: 4-digit hexadecimal
 *  - X: 8-digit hexadecimal
 *  - d: Signed decimal
 *  - s: char*, nul terminated
 */
 
static void raw_append(struct menu *menu,const char *fmt,...) {
  va_list vargs;
  va_start(vargs,fmt);
  char tmp[256];
  int tmpc=0;
  while (*fmt) {
    if (*fmt=='%') {
      fmt++;
      switch (*fmt++) {
        case 'x': {
            int v=va_arg(vargs,int);
            if (tmpc<sizeof(tmp)) tmp[tmpc++]="0123456789abcdef"[(v>>12)&0xf];
            if (tmpc<sizeof(tmp)) tmp[tmpc++]="0123456789abcdef"[(v>> 8)&0xf];
            if (tmpc<sizeof(tmp)) tmp[tmpc++]="0123456789abcdef"[(v>> 4)&0xf];
            if (tmpc<sizeof(tmp)) tmp[tmpc++]="0123456789abcdef"[(v    )&0xf];
          } break;
        case 'X': {
            int v=va_arg(vargs,int);
            if (tmpc<sizeof(tmp)) tmp[tmpc++]="0123456789abcdef"[(v>>28)&0xf];
            if (tmpc<sizeof(tmp)) tmp[tmpc++]="0123456789abcdef"[(v>>24)&0xf];
            if (tmpc<sizeof(tmp)) tmp[tmpc++]="0123456789abcdef"[(v>>20)&0xf];
            if (tmpc<sizeof(tmp)) tmp[tmpc++]="0123456789abcdef"[(v>>16)&0xf];
            if (tmpc<sizeof(tmp)) tmp[tmpc++]="0123456789abcdef"[(v>>12)&0xf];
            if (tmpc<sizeof(tmp)) tmp[tmpc++]="0123456789abcdef"[(v>> 8)&0xf];
            if (tmpc<sizeof(tmp)) tmp[tmpc++]="0123456789abcdef"[(v>> 4)&0xf];
            if (tmpc<sizeof(tmp)) tmp[tmpc++]="0123456789abcdef"[(v    )&0xf];
          } break;
        case 'd': {
            int v=va_arg(vargs,int);
            if (v<0) {
              if (tmpc<sizeof(tmp)) tmp[tmpc++]='-';
              v=-v;
              if (v<0) v=0x7fffffff; // possible for INT_MIN
            }
            int digitc=1,limit=10;
            while (v>=limit) { digitc++; if (limit>INT_MAX/10) break; limit*=10; }
            if (tmpc<=sizeof(tmp)-digitc) {
              int i=digitc; while (i-->0) {
                tmp[tmpc+i]='0'+v%10;
                v/=10;
              }
            }
            tmpc+=digitc;
          } break;
        case 's': {
            const char *v=va_arg(vargs,const char*);
            if (v) {
              for (;*v;v++) {
                if (tmpc<sizeof(tmp)) tmp[tmpc++]=*v;
              }
            }
          } break;
        default: if (tmpc<sizeof(tmp)) tmp[tmpc++]=*fmt;
      }
    } else {
      if (tmpc<sizeof(tmp)) tmp[tmpc++]=*fmt;
      fmt++;
    }
  }
  raw_append_line(menu,tmp,tmpc);
}

/* Greet new device.
 */
 
static int raw_cb_count_buttons(int btnid,int hidusage,int lo,int hi,int rest,void *userdata) {
  (*(int*)userdata)++;
  return 0;
}
 
static void raw_hello_device(struct menu *menu,int devid) {
  int vid=0,pid=0,version=0,namec,btnc=0;
  char name[256];
  egg_joystick_get_ids(&vid,&pid,&version,devid);
  egg_joystick_for_each_button(devid,raw_cb_count_buttons,&btnc);
  namec=egg_joystick_get_name(name,sizeof(name),devid);
  if ((namec<0)||(namec>=sizeof(name))) namec=0;
  name[namec]=0;
  raw_append(menu,"  New device %d: %x:%x:%x",devid,vid,pid,version);
  raw_append(menu,"  \"%s\"",name);
  raw_append(menu,"  %d buttons",btnc);
}

/* Event.
 */
 
static void _raw_event(struct menu *menu,const union egg_event *event) {
  switch (event->type) {
    case EGG_EVENT_RAW: {
        MENU->abortclock=0;
        raw_append(menu,"devid=%d btnid=%X value=%d",event->raw.devid,event->raw.btnid,event->raw.value);
        if (!event->raw.btnid&&event->raw.value) {
          raw_hello_device(menu,event->raw.devid);
        }
      } break;
  }
}

/* Update.
 */
 
static void _raw_update(struct menu *menu,double elapsed) {
  // We've suppressed JOY events, so it's important that we provide some means of exit.
  // This matters if the host only has joystick devices, eg my bespoke game consoles.
  if ((MENU->abortclock+=elapsed)>=ABORT_IDLE_TIME) {
    egg_log("menu_raw closing due to %f s idle.",ABORT_IDLE_TIME);
    pop_menu();
    return;
  }
}

/* Redraw log texture with font.
 */
 
static void raw_redraw_logtex(struct menu *menu) {
  if (!MENU->logtexid) {
    if ((MENU->logtexid=egg_texture_new())<1) {
      MENU->logtexid=0;
      return;
    }
  }
  if (!MENU->logbits) {
    MENU->logbitsstride=menu->screenw<<2;
    if (!(MENU->logbits=malloc(MENU->logbitsstride*menu->screenh))) return;
  }
  memset(MENU->logbits,0,MENU->logbitsstride*menu->screenh);
  // Don't break lines with font. We break only on LFs, and lines may breach the right edge.
  int logp=0,y=0;
  while (logp<MENU->logc) {
    const char *line=MENU->log+logp;
    int linec=0;
    while ((logp<MENU->logc)&&(MENU->log[logp++]!=0x0a)) linec++;
    font_render_string_rgba(
      MENU->logbits,menu->screenw,menu->screenh,MENU->logbitsstride,
      0,y,menu->font,line,linec,0xffffff
    );
    y+=MENU->rowh;
  }
  egg_texture_upload(MENU->logtexid,menu->screenw,menu->screenh,MENU->logbitsstride,EGG_TEX_FMT_RGBA,MENU->logbits,MENU->logbitsstride*menu->screenh);
}

/* Render.
 */
 
static void _raw_render(struct menu *menu) {

  // If we're close to timing out, flash the background a few times.
  double remaining=ABORT_IDLE_TIME-MENU->abortclock;
  if (remaining<ABORT_IDLE_WARNING_TIME) {
    int phase=((int)(remaining*5.0))&1;
    if (phase) egg_draw_rect(1,0,0,menu->screenw,menu->screenh,0x401020ff);
  }

  if (menu->font) {
    if (MENU->logtexdirty) raw_redraw_logtex(menu);
    egg_draw_decal(1,MENU->logtexid,0,0,0,0,menu->screenw,menu->screenh,0);
  } else if (menu->tile_renderer) {
    tile_renderer_begin(menu->tile_renderer,menu->tilesheet,0xffffffff,0xff);
    int y=MENU->rowh>>1,x=MENU->rowh>>1;
    int logp=0;
    for (;logp<MENU->logc;logp++) {
      uint8_t ch=MENU->log[logp];
      if (ch==0x0a) {
        y+=MENU->rowh;
        x=MENU->rowh>>1;
        continue;
      }
      if (ch>0x20) {
        tile_renderer_tile(menu->tile_renderer,x,y,ch,0);
      }
      x+=MENU->rowh;
    }
    tile_renderer_end(menu->tile_renderer);
  }
}

/* New.
 */
 
struct menu *menu_new_raw(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu_raw),parent);
  if (!menu) return 0;
  menu->del=_raw_del;
  menu->event=_raw_event;
  menu->update=_raw_update;
  menu->render=_raw_render;
  
  if (menu->font) {
    MENU->rowh=font_get_rowh(menu->font);
  } else if (menu->tile_renderer) {
    int w=0,h=0;
    egg_texture_get_header(&w,&h,0,menu->tilesheet);
    MENU->rowh=h>>4;
  }
  if (MENU->rowh<1) {
    egg_log("!!! menu_raw requires a font or tilesheet !!!");
    MENU->rowh=1;
  }
  if ((MENU->rowc=menu->screenh/MENU->rowh)<1) MENU->rowc=1;
  
  egg_event_enable(EGG_EVENT_JOY,0);
  egg_event_enable(EGG_EVENT_RAW,1);
  
  int p=0; for (;;p++) {
    int devid=egg_joystick_devid_by_index(p);
    if (devid<1) break;
    raw_hello_device(menu,devid);
  }
  
  raw_append(menu,"Esc, mouse, or 10 s idle to return.");
  
  return menu;
}
