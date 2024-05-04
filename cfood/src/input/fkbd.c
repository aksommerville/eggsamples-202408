#include <egg/egg.h>
#include "fkbd.h"
#include "bus.h"
#include "jlog.h"
#include "../utility/font.h"
#include <stdlib.h>
#include <string.h>

/* Default keycaps.
 * Contains all of G0, plus BS and HT, and roughly follows the US English layout.
 */
 
#define FKBD_DEFAULT_COLC 12
#define FKBD_DEFAULT_ROWC 3
#define FKBD_DEFAULT_PAGEC 3

#define FKBD_REPEAT_TIME_INITIAL 0.333
#define FKBD_REPEAT_TIME_MORE    0.100

static const int FKBD_DEFAULT_CONTENT[
  FKBD_DEFAULT_COLC*FKBD_DEFAULT_ROWC*FKBD_DEFAULT_PAGEC
]={
  'q','w','e','r','t','y','u','i','o','p',' ',0x08,
  'a','s','d','f','g','h','j','k','l',';','\'',0x0a,
  'z','x','c','v','b','n','m',',','.','/',' ',0x01,
  
  'Q','W','E','R','T','Y','U','I','O','P',' ',0x08,
  'A','S','D','F','G','H','J','K','L',':','"',0x0a,
  'Z','X','C','V','B','N','M','<','>','?',' ',0x01,
  
  '1','2','3','4','5','6','7','8','9','0',' ',0x08,
  '!','@','#','$','%','^','&','*','(',')',' ',0x0a,
  '`','~','-','=','_','+','[',']','\\','|',' ',0x01,
};

/* Object definition.
 */
 
struct fkbd {
  struct bus *bus; // WEAK
  int enabled;
  int colc,rowc,pagec;
  int colp,rowp,pagep; // Selection. OOB is legal.
  int *capv; // 3d, minor to major: column, row, page. Values are codepoints.
  struct font *font; // WEAK
  int tilesheet; // WEAK
  int dirty; // Recalculate layout and redraw caps next time we need them.
  int screenw,screenh;
  int blottery; // Fills width, and fills to bottom of screen.
  int boardx,boardy,boardw,boardh; // Extent of active space.
  int colw,rowh; // (colw*colc==boardw) ''h
  struct egg_draw_tile *vtxv;
  int vtxa;
  int overlay; // texid, STRONG
  int pvstate;
  int btnidl,btnidr,btnidu,btnidd,btnidb;
  int dx,dy;
  double repeat_time;
};

/* Delete.
 */
 
void fkbd_del(struct fkbd *fkbd) {
  if (!fkbd) return;
  if (fkbd->capv) free(fkbd->capv);
  if (fkbd->vtxv) free(fkbd->vtxv);
  if (fkbd->overlay) egg_texture_del(fkbd->overlay);
  free(fkbd);
}

/* New.
 */

struct fkbd *fkbd_new(struct bus *bus) {
  if (!bus) return 0;
  struct fkbd *fkbd=calloc(1,sizeof(struct fkbd));
  if (!fkbd) return 0;
  fkbd->bus=bus;
  egg_texture_get_header(&fkbd->screenw,&fkbd->screenh,0,1);
  return fkbd;
}

/* Set keycaps.
 */
 
int fkbd_set_keycaps(struct fkbd *fkbd,int colc,int rowc,int pagec,const int *codepointv) {
  if ((colc<1)||(rowc<1)||(pagec<1)) return -1;
  int *nv=malloc(sizeof(int)*colc*rowc*pagec);
  if (!nv) return -1;
  memcpy(nv,codepointv,sizeof(int)*colc*rowc*pagec);
  if (fkbd->capv) free(fkbd->capv);
  fkbd->capv=nv;
  fkbd->colc=colc;
  fkbd->rowc=rowc;
  fkbd->pagec=pagec;
  fkbd->dirty=1;
  return 0;
}

/* Set font or tilesheet.
 */

void fkbd_set_font(struct fkbd *fkbd,struct font *font) {
  fkbd->font=font;
  fkbd->tilesheet=0;
  fkbd->dirty=1;
}

void fkbd_set_tilesheet(struct fkbd *fkbd,int texid) {
  fkbd->font=0;
  fkbd->tilesheet=texid;
  fkbd->dirty=1;
}

/* Redraw overlay, when using font.
 */
 
static int fkbd_refresh_overlay(struct fkbd *fkbd) {
  if (!fkbd->overlay) {
    if ((fkbd->overlay=egg_texture_new())<1) {
      fkbd->overlay=0;
      return -1;
    }
  }
  int stride=fkbd->boardw<<2;
  int pixelslen=stride*fkbd->boardh;
  void *pixels=calloc(stride,fkbd->boardh);
  if (!pixels) return -1;
  int y=fkbd->rowh>>1;
  int yi=fkbd->rowc;
  const int *src=fkbd->capv+fkbd->pagep*(fkbd->colc*fkbd->rowc);
  for (;yi-->0;y+=fkbd->rowh) {
    int x=fkbd->colw>>1;
    int xi=fkbd->colc;
    for (;xi-->0;x+=fkbd->colw,src++) {
      font_render_char_rgba(
        pixels,fkbd->boardw,fkbd->boardh,stride,
        x,y,fkbd->font,*src,0x000000
      );
    }
  }
  int err=egg_texture_upload(fkbd->overlay,fkbd->boardw,fkbd->boardh,stride,EGG_TEX_FMT_RGBA,pixels,pixelslen);
  free(pixels);
  return err;
}

/* Recalculate layout and redraw overlay if we're using font.
 * Call this whenever dirty.
 */
 
static void fkbd_refresh(struct fkbd *fkbd) {
  if (!fkbd->capv) {
    if (fkbd_set_keycaps(fkbd,FKBD_DEFAULT_COLC,FKBD_DEFAULT_ROWC,FKBD_DEFAULT_PAGEC,FKBD_DEFAULT_CONTENT)<0) {
      egg_log("Panic! Failed to set default keycaps. [%s:%d]",__FILE__,__LINE__);
      fkbd->enabled=0;
      return;
    }
  }
  
  // If we have a font, cells are double the font's row height.
  if (fkbd->font) {
    fkbd->rowh=font_get_rowh(fkbd->font)*2;
    fkbd->colw=fkbd->rowh;
  // Likewise, if tilesheet, use double the tile size.
  } else if (fkbd->tilesheet) {
    int tsw=0,tsh=0;
    egg_texture_get_header(&tsw,&tsh,0,fkbd->tilesheet);
    fkbd->rowh=tsh>>3;
    fkbd->colw=fkbd->rowh;
  // No keycap image? This is not a normal way to use us. But arrange for the board to occupy about 1/3 of screen height.
  } else {
    fkbd->rowh=fkbd->screenh/(3*fkbd->rowc);
    fkbd->colw=fkbd->rowh;
  }
  // Finally, enforce a minimum of 8 for no particular reason. Can't be less than 1.
  if ((fkbd->colw<8)||(fkbd->rowh<8)) {
    fkbd->colw=8;
    fkbd->rowh=8;
  }
  
  int margin=fkbd->rowh/3; // No technical need for a vertical margin, but we look cramped without. Exact size not super important.
  fkbd->boardw=fkbd->colc*fkbd->colw;
  fkbd->boardh=fkbd->rowc*fkbd->rowh;
  fkbd->boardx=(fkbd->screenw>>1)-(fkbd->boardw>>1);
  fkbd->boardy=fkbd->screenh-margin-fkbd->boardh;
  fkbd->blottery=fkbd->boardy-margin;
  
  // If we're using font, redraw the overlay.
  if (fkbd->font) {
    if (fkbd_refresh_overlay(fkbd)<0) {
      egg_log("Panic! Failed to allocate keycaps texture. [%s:%d]",__FILE__,__LINE__);
      fkbd->enabled=0;
      return;
    }
  }
    
  // If we're using tilesheet, allocate the vertex buffer.
  if (fkbd->tilesheet) {
    int vtxc=fkbd->colc*fkbd->rowc;
    if (vtxc>fkbd->vtxa) {
      void *nv=realloc(fkbd->vtxv,sizeof(struct egg_draw_tile)*vtxc);
      if (!nv) {
        egg_log("Panic! Failed to allocate vertex buffer. [%s:%d]",__FILE__,__LINE__);
        fkbd->enabled=0;
        return;
      }
      fkbd->vtxv=nv;
      fkbd->vtxa=vtxc;
    }
  }
  
  fkbd->dirty=0;
}

/* Trigger focussed keycap if there is one.
 */
 
static void fkbd_activate(struct fkbd *fkbd) {
  if ((fkbd->colp<0)||(fkbd->colp>=fkbd->colc)) return;
  if ((fkbd->rowp<0)||(fkbd->rowp>=fkbd->rowc)) return;
  if ((fkbd->pagep<0)||(fkbd->pagep>=fkbd->pagec)) return;
  int codepoint=fkbd->capv[
    fkbd->pagep*(fkbd->colc*fkbd->rowc)+
    fkbd->rowp*fkbd->colc+
    fkbd->colp
  ];
  if (codepoint==1) { // Next page.
    fkbd->pagep++;
    if (fkbd->pagep>=fkbd->pagec) fkbd->pagep=0;
    fkbd->dirty=1;
  } else if (codepoint==2) { // Previous page.
    fkbd->pagep--;
    if (fkbd->pagep<0) fkbd->pagep=fkbd->pagec-1;
    fkbd->dirty=1;
  } else { // Normal text.
    bus_on_text(fkbd->bus,codepoint);
  }
}

/* Mouse and touch.
 */
 
static void fkbd_on_mmotion(struct fkbd *fkbd,int x,int y) {
  if (fkbd->dirty) fkbd_refresh(fkbd);
  if (!fkbd->enabled) return; // could happen, if refresh failed
  if (x<fkbd->boardx) fkbd->colp=-1; else fkbd->colp=(x-fkbd->boardx)/fkbd->colw;
  if (y<fkbd->boardy) fkbd->rowp=-1; else fkbd->rowp=(y-fkbd->boardy)/fkbd->rowh;
}

static void fkbd_on_mbutton(struct fkbd *fkbd,int btnid,int value,int x,int y) {
  if (btnid!=1) return;
  if (!value) return;
  fkbd_on_mmotion(fkbd,x,y);
  fkbd_activate(fkbd);
}

/* Event.
 */

void fkbd_event(struct fkbd *fkbd,const struct egg_event *event) {
  if (!fkbd->enabled) return;
  switch (event->type) {
    case EGG_EVENT_TEXT: bus_on_text(fkbd->bus,event->v[0]); break;
    case EGG_EVENT_MMOTION: fkbd_on_mmotion(fkbd,event->v[0],event->v[1]); break;
    case EGG_EVENT_MBUTTON: fkbd_on_mbutton(fkbd,event->v[0],event->v[1],event->v[2],event->v[3]); break;
    case EGG_EVENT_TOUCH: if (event->v[1]==1) fkbd_on_mbutton(fkbd,1,1,event->v[2],event->v[3]); break;
  }
}

/* Move pointer according to (dx,dy).
 */
 
static void fkbd_move_rel(struct fkbd *fkbd) {
  if (fkbd->dx&&fkbd->colc) {
    fkbd->colp+=fkbd->dx;
    if (fkbd->colp<0) fkbd->colp=fkbd->colc-1;
    else if (fkbd->colp>=fkbd->colc) fkbd->colp=0;
  }
  if (fkbd->dy&&fkbd->rowc) {
    fkbd->rowp+=fkbd->dy;
    if (fkbd->rowp<0) fkbd->rowp=fkbd->rowc-1;
    else if (fkbd->rowp>=fkbd->rowc) fkbd->rowp=0;
  }
}

/* Update.
 */
 
void fkbd_update(struct fkbd *fkbd,double elapsed) {
  if (!fkbd->enabled) return;
  
  // Receive joystick events.
  int state=jlog_get_player(bus_get_jlog(fkbd->bus),0);
  if (state!=fkbd->pvstate) {
    #define BTN(btnid,on,off) \
      if ((state&fkbd->btnid)&&!(fkbd->pvstate&fkbd->btnid)) { on; fkbd_move_rel(fkbd); fkbd->repeat_time=FKBD_REPEAT_TIME_INITIAL; } \
      else if (!(state&fkbd->btnid)&&(fkbd->pvstate&fkbd->btnid)) off;
    BTN(btnidl,fkbd->dx=-1,if (fkbd->dx<0) fkbd->dx=0)
    BTN(btnidr,fkbd->dx= 1,if (fkbd->dx>0) fkbd->dx=0)
    BTN(btnidu,fkbd->dy=-1,if (fkbd->dy<0) fkbd->dy=0)
    BTN(btnidd,fkbd->dy= 1,if (fkbd->dy>0) fkbd->dy=0)
    #undef BTN
    if ((state&fkbd->btnidb)&&!(fkbd->pvstate&fkbd->btnidb)) fkbd_activate(fkbd);
    fkbd->pvstate=state;
  }
  
  // Auto-repeat relative motion.
  if (fkbd->dx||fkbd->dy) {
    if ((fkbd->repeat_time-=elapsed)<=0.0) {
      fkbd_move_rel(fkbd);
      fkbd->repeat_time+=FKBD_REPEAT_TIME_MORE;
    }
  }
}

/* Render.
 */
 
void fkbd_render(struct fkbd *fkbd) {
  if (fkbd->dirty) fkbd_refresh(fkbd);
  if (!fkbd->enabled) return;
  egg_draw_rect(1,0,fkbd->blottery,fkbd->screenw,fkbd->screenh,0x000000c0);
  if ((fkbd->colp>=0)&&(fkbd->colp<fkbd->colc)&&(fkbd->rowp>=0)&&(fkbd->rowp<fkbd->rowc)) {
    egg_draw_rect(1,fkbd->boardx+fkbd->colp*fkbd->colw-1,fkbd->boardy+fkbd->rowp*fkbd->rowh-1,fkbd->colw+1,fkbd->rowh+1,0xffff00ff);
  }
  int y=fkbd->boardy,yi=fkbd->rowc;
  for (;yi-->0;y+=fkbd->rowh) {
    int x=fkbd->boardx,xi=fkbd->colc;
    for (;xi-->0;x+=fkbd->colw) {
      egg_draw_rect(1,x,y,fkbd->colw-1,fkbd->rowh-1,0xc0c0c0e0);
    }
  }
  if (fkbd->font) {
    egg_draw_decal(1,fkbd->overlay,fkbd->boardx,fkbd->boardy,0,0,fkbd->boardw,fkbd->boardh,0);
  } else if (fkbd->tilesheet) {
    int vtxc=fkbd->colc*fkbd->rowc;
    struct egg_draw_tile *vtx=fkbd->vtxv;
    const int *src=fkbd->capv+fkbd->pagep*(fkbd->colc*fkbd->rowc);
    for (y=fkbd->boardy+(fkbd->rowh>>1),yi=fkbd->rowc;yi-->0;y+=fkbd->rowh) {
      int x=fkbd->boardx+(fkbd->colw>>1);
      int xi=fkbd->colc;
      for (;xi-->0;x+=fkbd->colw,vtx++,src++) {
        vtx->x=x;
        vtx->y=y;
        vtx->tileid=*src;
        vtx->xform=0;
      }
    }
    egg_draw_mode(0,0x000000ff,0xff);
    egg_draw_tile(1,fkbd->tilesheet,fkbd->vtxv,vtxc);
    egg_draw_mode(0,0,0xff);
  }
}

/* Enable.
 */

void fkbd_enable(struct fkbd *fkbd,int enable) {
  if (enable) {
    if (fkbd->enabled) return;
    fkbd->enabled=1;
    egg_show_cursor(1);
    egg_event_enable(EGG_EVENT_TEXT,EGG_EVTSTATE_ENABLED);
    egg_event_enable(EGG_EVENT_MMOTION,EGG_EVTSTATE_ENABLED);
    egg_event_enable(EGG_EVENT_MBUTTON,EGG_EVTSTATE_ENABLED);
    egg_event_enable(EGG_EVENT_TOUCH,EGG_EVTSTATE_ENABLED);
    struct jlog *jlog=bus_get_jlog(fkbd->bus);
    fkbd->btnidl=jlog_btnid_from_standard(jlog,STDBTN_LEFT);
    fkbd->btnidr=jlog_btnid_from_standard(jlog,STDBTN_RIGHT);
    fkbd->btnidu=jlog_btnid_from_standard(jlog,STDBTN_UP);
    fkbd->btnidd=jlog_btnid_from_standard(jlog,STDBTN_DOWN);
    int btnidv[]={STDBTN_SOUTH,STDBTN_EAST,STDBTN_R1,STDBTN_L1,STDBTN_LP,STDBTN_WEST,STDBTN_NORTH,STDBTN_R2,STDBTN_L2,STDBTN_RP,STDBTN_AUX1,STDBTN_AUX2,STDBTN_AUX3,0};
    const int *btnid=btnidv;
    for (;*btnid;btnid++) {
      if (fkbd->btnidb=jlog_btnid_from_standard(jlog,*btnid)) break;
    }
    fkbd->dirty=1;
  } else {
    if (!fkbd->enabled) return;
    fkbd->enabled=0;
    egg_event_enable(EGG_EVENT_MMOTION,EGG_EVTSTATE_DISABLED);
    egg_event_enable(EGG_EVENT_MBUTTON,EGG_EVTSTATE_DISABLED);
    egg_event_enable(EGG_EVENT_TOUCH,EGG_EVTSTATE_DISABLED);
  }
}
