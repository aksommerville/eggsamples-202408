#include "inkeep_internal.h"

struct inkeep inkeep={0};

/* Quit.
 */
 
void inkeep_quit() {
  if (inkeep.texid_font) egg_texture_del(inkeep.texid_font);
  if (inkeep.listenerv) free(inkeep.listenerv);
  if (inkeep.keycapv) free(inkeep.keycapv);
  if (inkeep.cursor_texid) egg_texture_del(inkeep.cursor_texid);
  if (inkeep.keyoverlay_texid) egg_texture_del(inkeep.keyoverlay_texid);
  memset(&inkeep,0,sizeof(inkeep));
}

/* Init.
 */
 
int inkeep_init() {
  inkeep.mode=INKEEP_MODE_RAW;
  inkeep.id_next=1;
  egg_texture_get_header(&inkeep.screenw,&inkeep.screenh,0,1);
  inkeep.mousex=inkeep.touchx=inkeep.screenw>>1;
  inkeep.mousey=inkeep.touchy=inkeep.screenh>>1;
  inkeep.playerc=1;
  inkeep.fakemouse_speed=(double)((inkeep.screenw>inkeep.screenh)?inkeep.screenw:inkeep.screenh)/2.0; // 2s to cross the screen's longer axis
  inkeep.keyoverlay_dirty=1;
  return 0;
}

/* Render.
 */
 
void inkeep_render() {
  if (inkeep.fake_keyboard) fake_keyboard_render();
  if (inkeep.fake_pointer) fake_pointer_render();
}

/* Set font tilesheet.
 */

int inkeep_set_font_tiles(int imageid) {
  if (!inkeep.texid_font) {
    if ((inkeep.texid_font=egg_texture_new())<1) {
      inkeep.texid_font=0;
      return -1;
    }
  }
  if (egg_texture_load_image(inkeep.texid_font,0,imageid)<0) {
    egg_texture_del(inkeep.texid_font);
    inkeep.texid_font=0;
    return -1;
  }
  inkeep.font=0;
  inkeep.keycolw=0;
  inkeep.keyrowh=0;
  inkeep.keyoverlay_dirty=1;
  return inkeep.texid_font;
}

/* Set font object.
 */
 
int inkeep_set_font(struct font *font) {
  inkeep.font=font;
  if (inkeep.texid_font) {
    egg_texture_del(inkeep.texid_font);
    inkeep.texid_font=0;
  }
  inkeep.keycolw=0;
  inkeep.keyrowh=0;
  inkeep.keyoverlay_dirty=1;
  return 0;
}

/* Set keycaps.
 */

int inkeep_set_keycaps(int pagec,int rowc,int colc,const char *caps) {
  if (!caps) return -1;
  
  // All must be at least one, and also enforce an arbitrary sanity limit.
  if ((pagec<1)||(pagec>16)) return -1;
  if ((rowc<1)||(rowc>8)) return -1;
  if ((colc<1)||(colc>32)) return -1;

  int capc=pagec*rowc*colc;
  int *nv=malloc(sizeof(int)*capc);
  if (!nv) return -1;
  int np=0,capsp=0;
  for (;np<capc;np++) {
    int codepoint,seqlen;
    if ((seqlen=text_utf8_decode(&codepoint,caps+capsp,4))<1) {
      free(nv);
      return -1;
    }
    capsp+=seqlen;
    nv[np]=codepoint;
  }
  
  if (inkeep.keycapv) free(inkeep.keycapv);
  inkeep.keycapv=nv;
  inkeep.keycap_pagec=pagec;
  inkeep.keycap_rowc=rowc;
  inkeep.keycap_colc=colc;
  
  if (inkeep.keycap_pagep>=inkeep.keycap_pagec) inkeep.keycap_pagep=0;
  if (inkeep.keycap_rowp>=inkeep.keycap_rowc) inkeep.keycap_rowp=0;
  if (inkeep.keycap_colp>=inkeep.keycap_colc) inkeep.keycap_colp=0;
  
  inkeep.keycolw=0;
  inkeep.keyrowh=0;
  inkeep.keyoverlay_dirty=1;
  
  return 0;
}

/* Set cursor.
 */

int inkeep_set_cursor(int imageid,int x,int y,int w,int h,int xform,int hotx,int hoty) {
  if (imageid!=inkeep.cursor_imageid) {
    if (!inkeep.cursor_texid) {
      if ((inkeep.cursor_texid=egg_texture_new())<1) {
        inkeep.cursor_texid=0;
        return -1;
      }
    }
    if (egg_texture_load_image(inkeep.cursor_texid,0,imageid)<0) {
      egg_texture_del(inkeep.cursor_texid);
      inkeep.cursor_texid=0;
      return -1;
    }
    inkeep.cursor_imageid=imageid;
  }
  inkeep.cursorx=x;
  inkeep.cursory=y;
  inkeep.cursorw=w;
  inkeep.cursorh=h;
  inkeep.cursorxform=xform;
  inkeep.cursorhotx=hotx;
  inkeep.cursorhoty=hoty;
  return inkeep.cursor_texid;
}

/* Set player count.
 */

int inkeep_set_player_count(int playerc) {
  if (playerc<1) playerc=1;
  else if (playerc>=INKEEP_PLAYER_LIMIT) playerc=INKEEP_PLAYER_LIMIT-1;
  inkeep.playerc=playerc;
  struct inkeep_joy *joy=inkeep.joyv;
  int i=inkeep.joyc;
  for (;i-->0;joy++) {
    if (joy->plrid>playerc) joy->plrid=0;
  }
  return playerc;
}

/* Set mode.
 */

int inkeep_set_mode(int mode) {
  if (mode!=inkeep.mode) {
    switch (inkeep.mode) {
      case INKEEP_MODE_RAW: break;
      case INKEEP_MODE_JOY: break;
      case INKEEP_MODE_TEXT: break;
      case INKEEP_MODE_TOUCH: break;
      case INKEEP_MODE_MOUSE: break;
      case INKEEP_MODE_POV: break;
    }
    inkeep.mode=mode;
    switch (mode) {
      case INKEEP_MODE_RAW: {
          // Event mask stays put. Caller owns it now.
          inkeep.fake_keyboard=0;
          inkeep.fake_pointer=0;
        } break;
      case INKEEP_MODE_JOY: {
          egg_event_enable(EGG_EVENT_JOY,1);
          egg_event_enable(EGG_EVENT_KEY,1);
          egg_event_enable(EGG_EVENT_TEXT,0);
          egg_event_enable(EGG_EVENT_MMOTION,0);
          egg_event_enable(EGG_EVENT_MBUTTON,0);
          egg_event_enable(EGG_EVENT_MWHEEL,0);
          egg_event_enable(EGG_EVENT_TOUCH,1);
          inkeep.fake_keyboard=0;
          inkeep.fake_pointer=0;
          egg_lock_cursor(0);
          egg_show_cursor(0);
        } break;
      case INKEEP_MODE_TEXT: {
          egg_event_enable(EGG_EVENT_JOY,1);
          egg_event_enable(EGG_EVENT_KEY,0);
          egg_event_enable(EGG_EVENT_TEXT,1);
          int have_mouse=egg_event_enable(EGG_EVENT_MMOTION,1);
          egg_event_enable(EGG_EVENT_MBUTTON,1);
          egg_event_enable(EGG_EVENT_MWHEEL,1);
          egg_event_enable(EGG_EVENT_TOUCH,1);
          inkeep.fake_keyboard=(inkeep.font||inkeep.texid_font)?1:0;
          inkeep.fake_pointer=0;
          egg_lock_cursor(0);
          if (have_mouse&&inkeep.fake_keyboard) {
            if (inkeep.cursor_texid) {
              inkeep.fake_pointer=1;
              egg_show_cursor(0);
              inkeep.fakemousex=inkeep.mousex;
              inkeep.fakemousey=inkeep.mousey;
            } else {
              egg_show_cursor(1);
            }
          } else {
            egg_show_cursor(0);
          }
        } break;
      case INKEEP_MODE_TOUCH:  {
          int have_joy=egg_event_enable(EGG_EVENT_JOY,1);
          int have_key=egg_event_enable(EGG_EVENT_KEY,1);
          egg_event_enable(EGG_EVENT_TEXT,0);
          int have_mouse=egg_event_enable(EGG_EVENT_MMOTION,1);
          egg_event_enable(EGG_EVENT_MBUTTON,1);
          egg_event_enable(EGG_EVENT_MWHEEL,1);
          egg_event_enable(EGG_EVENT_TOUCH,1);
          inkeep.fake_keyboard=0;
          inkeep.fake_pointer=0;
          if (have_joy||have_key||have_mouse) {
            if (inkeep.cursor_texid) {
              inkeep.fake_pointer=1;
              inkeep.fakemousex=inkeep.mousex;
              inkeep.fakemousey=inkeep.mousey;
              inkeep.fakemouse_soft_hide=1;
            }
          }
          egg_lock_cursor(0);
          egg_show_cursor(inkeep.fake_pointer?0:1);
        } break;
      case INKEEP_MODE_MOUSE:  {
          int have_joy=egg_event_enable(EGG_EVENT_JOY,1);
          int have_key=egg_event_enable(EGG_EVENT_KEY,1);
          egg_event_enable(EGG_EVENT_TEXT,0);
          int have_mouse=egg_event_enable(EGG_EVENT_MMOTION,1);
          egg_event_enable(EGG_EVENT_MBUTTON,1);
          egg_event_enable(EGG_EVENT_MWHEEL,1);
          egg_event_enable(EGG_EVENT_TOUCH,1);
          inkeep.fake_keyboard=0;
          inkeep.fake_pointer=0;
          if (have_joy||have_key||have_mouse) {
            if (inkeep.cursor_texid) {
              inkeep.fake_pointer=1;
              inkeep.fakemousex=inkeep.mousex;
              inkeep.fakemousey=inkeep.mousey;
              inkeep.fakemouse_soft_hide=1;
            }
          }
          egg_lock_cursor(0);
          egg_show_cursor(inkeep.fake_pointer?0:1);
        } break;
      case INKEEP_MODE_POV:  {
          egg_event_enable(EGG_EVENT_JOY,1);
          egg_event_enable(EGG_EVENT_KEY,1);
          egg_event_enable(EGG_EVENT_TEXT,0);
          egg_event_enable(EGG_EVENT_MMOTION,1);
          egg_event_enable(EGG_EVENT_MBUTTON,1);
          egg_event_enable(EGG_EVENT_MWHEEL,1);
          egg_event_enable(EGG_EVENT_TOUCH,0);
          inkeep.fake_keyboard=0;
          inkeep.fake_pointer=0;
          egg_lock_cursor(1);
        } break;
    }
  }
  return inkeep.mode;
}

/* Trivial accessors.
 */
 
void inkeep_get_mouse(int *x,int *y) {
  if (x) *x=inkeep.mousex;
  if (y) *y=inkeep.mousey;
}
