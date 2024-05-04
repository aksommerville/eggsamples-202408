#include <egg/egg.h>
#include "jcfg.h"
#include "bus.h"
#include "joy2.h"
#include "jlog.h"
#include "../utility/font.h"
#include "../utility/text.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#define JCFG_SHOW_CLOCK_TIME 6.0
#define JCFG_CLOCK_TIME 10.0

#define JCFG_STAGE_FAULT 1
#define JCFG_STAGE_PRESS 2
#define JCFG_STAGE_HOLD 3
#define JCFG_STAGE_AGAIN 4
#define JCFG_STAGE_AGAIN_HOLD 5
#define JCFG_STAGE_COMPLETE 6

/* Object definition.
 */
 
struct jcfg {
  struct bus *bus; // WEAK
  int enabled;
  int devid;
  int btnid;
  int allbtns;
  int stage;
  int srcbtnid;
  char srcpart;
  int listener;
  int screenw,screenh;
  struct font *font; // WEAK
  int tilesheet; // texid, WEAK
  int texid_device; // STRONG
  int texid_prompt; // STRONG
  int texid_clock; // STRONG
  int w_device,h_device,w_prompt,h_prompt,w_clock,h_clock;
  struct egg_draw_tile *tilev;
  int tilec,tilea;
  int tilew,tileh;
  double clock;
  int clockdisplay;
  char *textbuf;
  int textbufc,textbufa;
  int stringid_device;
  int stringid_fault;
  int stringid_press;
  int stringid_again;
  struct jcfg_change {
    int srcbtnid;
    char srcpart;
    int dstbtnid;
  } changev[16];
  int changec;
};

/* Delete.
 */
 
void jcfg_del(struct jcfg *jcfg) {
  if (!jcfg) return;
  egg_texture_del(jcfg->texid_device);
  egg_texture_del(jcfg->texid_prompt);
  egg_texture_del(jcfg->texid_clock);
  if (jcfg->tilev) free(jcfg->tilev);
  if (jcfg->textbuf) free(jcfg->textbuf);
  free(jcfg);
}

/* New.
 */

struct jcfg *jcfg_new(struct bus *bus) {
  struct jcfg *jcfg=calloc(1,sizeof(struct jcfg));
  if (!jcfg) return 0;
  jcfg->bus=bus;
  jcfg->clockdisplay=-1;
  egg_texture_get_header(&jcfg->screenw,&jcfg->screenh,0,1);
  return jcfg;
}

/* Put text in our temporary text buffer.
 */
 
static int jcfg_textbuf_require(struct jcfg *jcfg,int c) {
  if (c<=jcfg->textbufa) return 0;
  int na=(c+32)&~31;
  void *nv=realloc(jcfg->textbuf,na);
  if (!nv) return -1;
  jcfg->textbuf=nv;
  jcfg->textbufa=na;
  return 0;
}
 
static void jcfg_acquire_device_label(struct jcfg *jcfg) {
  jcfg->textbufc=0;
  const char *pfx=0;
  int pfxc=text_get_string(&pfx,jcfg->stringid_device);
  const char *sep=0;
  int sepc=0;
  if (pfxc) { sep=": "; sepc=2; }
  const char *sfx=0;
  if (jcfg->devid==JCFG_DEVID_ANY) sfx="...";
  else sfx=joy2_device_get_ids(0,0,0,0,bus_get_joy2(jcfg->bus),jcfg->devid);
  if (!sfx||!sfx[0]) sfx="?";
  int sfxc=0;
  if (sfx) while (sfx[sfxc]) sfxc++;
  int reqc=pfxc+sepc+sfxc;
  if (jcfg_textbuf_require(jcfg,reqc)<0) return;
  memcpy(jcfg->textbuf,pfx,pfxc);
  memcpy(jcfg->textbuf+pfxc,sep,sepc);
  memcpy(jcfg->textbuf+pfxc+sepc,sfx,sfxc);
  jcfg->textbufc=reqc;
}

static void jcfg_acquire_prompt(struct jcfg *jcfg) {
  jcfg->textbufc=0;
  int stringid=0;
  switch (jcfg->stage) {
    case JCFG_STAGE_FAULT: stringid=jcfg->stringid_fault; break;
    case JCFG_STAGE_PRESS: stringid=jcfg->stringid_press; break;
    case JCFG_STAGE_AGAIN: stringid=jcfg->stringid_again; break;
  }
  if (!stringid) return;
  const char *pat=0;
  int patc=text_get_string(&pat,stringid);
  int insp=-1;
  int i=patc; while (i-->0) { if (pat[i]=='%') { insp=i; break; } }
  if (insp>=0) {
    stringid=jlog_stringid_for_btnid(bus_get_jlog(jcfg->bus),jcfg->btnid);
    const char *name=0;
    int namec=text_get_string(&name,stringid);
    int reqc=patc-1+namec;
    if (jcfg_textbuf_require(jcfg,reqc)<0) return;
    memcpy(jcfg->textbuf,pat,insp);
    memcpy(jcfg->textbuf+insp,name,namec);
    memcpy(jcfg->textbuf+insp+namec,pat+insp+1,patc-insp-1);
    jcfg->textbufc=reqc;
  } else {
    if (jcfg_textbuf_require(jcfg,patc)<0) return;
    memcpy(jcfg->textbuf,pat,patc);
    jcfg->textbufc=patc;
  }
}

/* Redraw all textures from the font.
 */
 
static void jcfg_redraw_device(struct jcfg *jcfg) {
  if (!jcfg->texid_device) {
    if (!(jcfg->texid_device=egg_texture_new())) return;
  }
  jcfg_acquire_device_label(jcfg);
  font_render_to_texture(jcfg->texid_device,jcfg->font,jcfg->textbuf,jcfg->textbufc,0,0x808080);
  egg_texture_get_header(&jcfg->w_device,&jcfg->h_device,0,jcfg->texid_device);
}

static void jcfg_redraw_prompt(struct jcfg *jcfg) {
  if (!jcfg->texid_prompt) {
    if (!(jcfg->texid_prompt=egg_texture_new())) return;
  }
  jcfg_acquire_prompt(jcfg);
  font_render_to_texture(jcfg->texid_prompt,jcfg->font,jcfg->textbuf,jcfg->textbufc,0,0xffffff);
  egg_texture_get_header(&jcfg->w_prompt,&jcfg->h_prompt,0,jcfg->texid_prompt);
}

static void jcfg_redraw_clock(struct jcfg *jcfg,int digit) {
  if (!jcfg->texid_clock) {
    if (!(jcfg->texid_clock=egg_texture_new())) return;
  }
  char s='0'+digit;
  font_render_to_texture(jcfg->texid_clock,jcfg->font,&s,1,0,0xff8080);
  egg_texture_get_header(&jcfg->w_clock,&jcfg->h_clock,0,jcfg->texid_clock);
  jcfg->clockdisplay=digit;
}
 
static void jcfg_redraw_textures(struct jcfg *jcfg) {
  jcfg_redraw_device(jcfg);
  jcfg_redraw_prompt(jcfg);
  if (jcfg->clock<JCFG_SHOW_CLOCK_TIME) jcfg_redraw_clock(jcfg,jcfg->clockdisplay);
}

/* Rebuild tile list.
 */
 
static struct egg_draw_tile *jcfg_tile_add(struct jcfg *jcfg) {
  if (jcfg->tilec>=jcfg->tilea) {
    int na=jcfg->tilec+64;
    if (na>INT_MAX/sizeof(struct egg_draw_tile)) return 0;
    void *nv=realloc(jcfg->tilev,sizeof(struct egg_draw_tile)*na);
    if (!nv) return 0;
    jcfg->tilev=nv;
    jcfg->tilea=na;
  }
  return jcfg->tilev+jcfg->tilec++;
}

static void jcfg_append_device_tiles(struct jcfg *jcfg) {
  int y=(jcfg->screenh>>1)-jcfg->tileh;
  jcfg_acquire_device_label(jcfg);
  const char *text=jcfg->textbuf;
  int textc=jcfg->textbufc;
  int x=(jcfg->screenw>>1)-((jcfg->tilew*textc)>>1)+(jcfg->tilew>>1);
  for (;textc-->0;text++,x+=jcfg->tilew) {
    struct egg_draw_tile *tile=jcfg_tile_add(jcfg);
    if (!tile) return;
    tile->x=x;
    tile->y=y;
    tile->tileid=*text;
    tile->xform=0;
  }
}

static void jcfg_append_prompt_tiles(struct jcfg *jcfg) {
  int y=(jcfg->screenh>>1)+jcfg->tileh;
  jcfg_acquire_prompt(jcfg);
  const char *text=jcfg->textbuf;
  int textc=jcfg->textbufc;
  int x=(jcfg->screenw>>1)-((jcfg->tilew*textc)>>1)+(jcfg->tilew>>1);
  for (;textc-->0;text++,x+=jcfg->tilew) {
    struct egg_draw_tile *tile=jcfg_tile_add(jcfg);
    if (!tile) return;
    tile->x=x;
    tile->y=y;
    tile->tileid=*text;
    tile->xform=0;
  }
}
 
static void jcfg_update_clock_tile(struct jcfg *jcfg,int display) {
  if (display>=0) {
    if ((jcfg->clockdisplay<0)||(jcfg->tilec<1)) {
      if (!jcfg_tile_add(jcfg)) return;
    }
    struct egg_draw_tile *tile=jcfg->tilev+jcfg->tilec-1;
    tile->x=jcfg->screenw>>1;
    tile->y=(jcfg->screenh>>1)+jcfg->tileh*3;
    tile->tileid='0'+display;
    tile->xform=0;
  }
  jcfg->clockdisplay=display;
}

static void jcfg_redraw_tiles(struct jcfg *jcfg) {
  jcfg->tilec=0;
  jcfg_append_device_tiles(jcfg);
  jcfg_append_prompt_tiles(jcfg);
  int display=jcfg->clockdisplay;
  jcfg->clockdisplay=-1;
  jcfg_update_clock_tile(jcfg,display);
}

/* Trivial accessors.
 */
 
void jcfg_set_font(struct jcfg *jcfg,struct font *font) {
  jcfg->font=font;
  jcfg->tilesheet=0;
  if (jcfg->enabled) jcfg_redraw_textures(jcfg);
}

void jcfg_set_tilesheet(struct jcfg *jcfg,int texid) {
  jcfg->tilesheet=texid;
  jcfg->font=0;
  egg_texture_get_header(&jcfg->tilew,&jcfg->tileh,0,texid);
  jcfg->tilew>>=4;
  jcfg->tileh>>=4;
  if (jcfg->enabled) jcfg_redraw_tiles(jcfg);
}

void jcfg_set_strings(struct jcfg *jcfg,
  int stringid_device, // "Device", will be followed by device name.
  int stringid_fault, // eg "Fault! Please press %."
  int stringid_press, // eg "Please press %."
  int stringid_again // eg "Press % again."
) {
  jcfg->stringid_device=stringid_device;
  jcfg->stringid_fault=stringid_fault;
  jcfg->stringid_press=stringid_press;
  jcfg->stringid_again=stringid_again;
  if (jcfg->enabled) {
    if (jcfg->font) jcfg_redraw_textures(jcfg);
    else if (jcfg->tilesheet) jcfg_redraw_tiles(jcfg);
  }
}

int jcfg_is_complete(const struct jcfg *jcfg) {
  return (jcfg->stage==JCFG_STAGE_COMPLETE);
}

/* Push changes out to jlog.
 */
 
static void jcfg_save(struct jcfg *jcfg) {
  int vid=0,pid=0,version=0,mapping=0;
  const char *name=joy2_device_get_ids(&vid,&pid,&version,&mapping,bus_get_joy2(jcfg->bus),jcfg->devid);
  struct jlog *jlog=bus_get_jlog(jcfg->bus);
  void *tm=jlog_clear_template(jlog,vid,pid,version,name,-1);
  if (!tm) return;
  const struct jcfg_change *change=jcfg->changev;
  int i=jcfg->changec;
  for (;i-->0;change++) {
    jlog_append_template(jlog,tm,change->srcbtnid,change->srcpart,change->dstbtnid);
  }
  jlog_store_templates(jlog);
}

/* Next button. Does not commit current.
 */
 
static void jcfg_advance(struct jcfg *jcfg) {
  if (jcfg->stage==JCFG_STAGE_COMPLETE) return;
  jcfg->clock=JCFG_CLOCK_TIME;
  jcfg->clockdisplay=-1;
  jcfg->btnid<<=1;
  if (jcfg->btnid>jcfg->allbtns) {
    jcfg_save(jcfg);
    jcfg->stage=JCFG_STAGE_COMPLETE;
  } else {
    jcfg->stage=JCFG_STAGE_PRESS;
    if (jcfg->font) jcfg_redraw_prompt(jcfg);
    else if (jcfg->tilesheet) jcfg_redraw_tiles(jcfg);
  }
}

/* Commit the current input.
 */
 
static void jcfg_commit(struct jcfg *jcfg) {
  int a=sizeof(jcfg->changev)/sizeof(jcfg->changev[0]);
  if (jcfg->changec>=a) return;
  struct jcfg_change *change=jcfg->changev+jcfg->changec++;
  change->srcbtnid=jcfg->srcbtnid;
  change->srcpart=jcfg->srcpart;
  change->dstbtnid=jcfg->btnid;
}

/* Update.
 */

void jcfg_update(struct jcfg *jcfg,double elapsed) {
  if (!jcfg->enabled) return;
  if (jcfg->stage==JCFG_STAGE_COMPLETE) return;
  if ((jcfg->clock-=elapsed)<=0.0) {
    jcfg_advance(jcfg);
  } else {
    if (jcfg->clock<JCFG_SHOW_CLOCK_TIME) {
      int display=(int)jcfg->clock;
      if (display!=jcfg->clockdisplay) {
        if (jcfg->font) {
          jcfg_redraw_clock(jcfg,display);
        } else if (jcfg->tilesheet) {
          jcfg_update_clock_tile(jcfg,display);
        }
      }
    }
  }
}

/* joy2 event.
 */
 
void jcfg_on_joy2(int devid,int btnid,char part,int value,void *userdata) {
  struct jcfg *jcfg=userdata;
  
  // If we don't have a device yet, any press event sets it, and release events are ignored.
  if (jcfg->devid==JCFG_DEVID_ANY) {
    if (!value) return;
    jcfg->devid=devid;
    if (jcfg->font) jcfg_redraw_device(jcfg);
    else if (jcfg->tilesheet) jcfg_redraw_tiles(jcfg);
    // ...and also proceed to process the event normally.
  }
  
  switch (jcfg->stage) {
  
    case JCFG_STAGE_FAULT:
    case JCFG_STAGE_PRESS: {
        if (!value) return;
        jcfg->srcbtnid=btnid;
        jcfg->srcpart=part;
        jcfg->stage=JCFG_STAGE_HOLD;
        if (jcfg->font) jcfg_redraw_prompt(jcfg);
        else if (jcfg->tilesheet) jcfg_redraw_tiles(jcfg);
        jcfg->clock=JCFG_CLOCK_TIME;
      } break;
      
    case JCFG_STAGE_HOLD: {
        if (value||(btnid!=jcfg->srcbtnid)||(part!=jcfg->srcpart)) {
          jcfg->stage=JCFG_STAGE_FAULT;
          if (jcfg->font) jcfg_redraw_prompt(jcfg);
          else if (jcfg->tilesheet) jcfg_redraw_tiles(jcfg);
        } else {
          jcfg->stage=JCFG_STAGE_AGAIN;
          if (jcfg->font) jcfg_redraw_prompt(jcfg);
          else if (jcfg->tilesheet) jcfg_redraw_tiles(jcfg);
        }
        jcfg->clock=JCFG_CLOCK_TIME;
      } break;
      
    case JCFG_STAGE_AGAIN: {
        if (!value) return;
        if ((btnid!=jcfg->srcbtnid)||(part!=jcfg->srcpart)) {
          jcfg->stage=JCFG_STAGE_FAULT;
          if (jcfg->font) jcfg_redraw_prompt(jcfg);
          else if (jcfg->tilesheet) jcfg_redraw_tiles(jcfg);
        } else {
          jcfg->stage=JCFG_STAGE_AGAIN_HOLD;
          if (jcfg->font) jcfg_redraw_prompt(jcfg);
          else if (jcfg->tilesheet) jcfg_redraw_tiles(jcfg);
        }
        jcfg->clock=JCFG_CLOCK_TIME;
      } break;
      
    case JCFG_STAGE_AGAIN_HOLD: {
        if (value||(btnid!=jcfg->srcbtnid)||(part!=jcfg->srcpart)) {
          jcfg->stage=JCFG_STAGE_FAULT;
          if (jcfg->font) jcfg_redraw_prompt(jcfg);
          else if (jcfg->tilesheet) jcfg_redraw_tiles(jcfg);
        } else {
          jcfg_commit(jcfg);
          jcfg_advance(jcfg);
        }
      } break;
      
  }
}

/* Render.
 */
 
void jcfg_render(struct jcfg *jcfg) {
  if (!jcfg->enabled) return;
  egg_draw_rect(1,0,0,jcfg->screenw,jcfg->screenw,0x000000c0);
  if (jcfg->stage==JCFG_STAGE_COMPLETE) return;
  
  if (jcfg->font) {
    int spacing=jcfg->h_device>>1;
    int mainh=jcfg->h_device+jcfg->h_prompt;
    int y=(jcfg->screenh>>1)-(mainh>>1); // Centering does not include clock; that gets tacked on below.
    egg_draw_decal(1,jcfg->texid_device,(jcfg->screenw>>1)-(jcfg->w_device>>1),y,0,0,jcfg->w_device,jcfg->h_device,0);
    y+=jcfg->h_device+spacing;
    egg_draw_decal(1,jcfg->texid_prompt,(jcfg->screenw>>1)-(jcfg->w_prompt>>1),y,0,0,jcfg->w_prompt,jcfg->h_prompt,0);
    if (jcfg->clock<JCFG_SHOW_CLOCK_TIME) {
      y+=jcfg->h_prompt+spacing;
      egg_draw_decal(1,jcfg->texid_clock,(jcfg->screenw>>1)-(jcfg->w_clock>>1),y,0,0,jcfg->w_clock,jcfg->h_clock,0);
    }
  
  } else if (jcfg->tilesheet&&(jcfg->tilec>0)) {
    egg_draw_mode(0,0xffffffff,0xff);
    egg_draw_tile(1,jcfg->tilesheet,jcfg->tilev,jcfg->tilec);
    egg_draw_mode(0,0,0xff);
  }
}

/* Enable.
 */

void jcfg_enable(struct jcfg *jcfg,int devid) {
  if (devid) {
    if (!jcfg->enabled) {
      jcfg->enabled=1;
    }
    jcfg->devid=devid;
    jcfg->clock=JCFG_CLOCK_TIME;
    jcfg->allbtns=jlog_get_state_mask(bus_get_jlog(jcfg->bus));
    if (!jcfg->allbtns) {
      jcfg->enabled=0;
      return;
    }
    jcfg->btnid=1;
    while (!(jcfg->btnid&jcfg->allbtns)) jcfg->btnid<<=1;
    jcfg->stage=JCFG_STAGE_PRESS;
    jcfg->listener=joy2_listen(bus_get_joy2(jcfg->bus),jcfg_on_joy2,jcfg);
    jcfg->changec=0;
    if (jcfg->font) jcfg_redraw_textures(jcfg);
    else if (jcfg->tilesheet) jcfg_redraw_tiles(jcfg);
  } else {
    if (!jcfg->enabled) return;
    joy2_unlisten(bus_get_joy2(jcfg->bus),jcfg->listener);
    jcfg->listener=0;
    jcfg->enabled=0;
  }
}
