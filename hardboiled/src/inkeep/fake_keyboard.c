#include "inkeep_internal.h"

#define MARGIN 5
#define MOTION_TIME_INITIAL 0.250
#define MOTION_TIME_REPEAT  0.100

/* Get keyboard height.
 */

int inkeep_get_keyboard_height() {
  if (!inkeep.fake_keyboard) return 0;
  if (!inkeep.keycolw||!inkeep.keyrowh) {
  
    if (inkeep.font) {
      // Keys will be square. Double the width of an uppercase W, or the line height, whichever is larger.
      int rowh=font_get_rowh(inkeep.font);
      int colw=font_measure_glyph(inkeep.font,'W');
      inkeep.keycolw=(rowh>colw)?rowh:colw;
      inkeep.keycolw<<=1;
      inkeep.keyrowh=inkeep.keycolw;
  
    } else if (inkeep.texid_font) {
      // Keys will be square, double the tile width.
      int tilesize=0;
      egg_texture_get_header(&tilesize,0,0,inkeep.texid_font);
      inkeep.keycolw=tilesize>>4;
      inkeep.keycolw<<=1;
      inkeep.keyrowh=inkeep.keycolw;
    }
  }
  if (inkeep.keyrowh<1) return 0;
  // Spacing between keys is built-in to the column and row sizes. Just add the top and bottom margin.
  return inkeep.keyrowh*inkeep.keycap_rowc+(MARGIN<<1);
}

/* Draw a little decoration on the Shift key when quickshift engaged.
 */
 
static void fake_keyboard_fill_rect(uint8_t *pixels,int stride,int x,int y,int w,int h,int rgba) {
  pixels+=y*stride+(x<<2);
  uint8_t r=rgba>>24,g=rgba>>16,b=rgba>>8,a=rgba;
  for (;h-->0;pixels+=stride) {
    uint8_t *dstp=pixels;
    int xi=w;
    for (;xi-->0;dstp+=4) {
      dstp[0]=r;
      dstp[1]=g;
      dstp[2]=b;
      dstp[3]=a;
    }
  }
}
 
static void fake_keyboard_render_shift(uint8_t *pixels,int stride,int x,int y) {
  fake_keyboard_fill_rect(pixels,stride,x+1,y+1,inkeep.keycolw-3,1,0x000040ff);
  fake_keyboard_fill_rect(pixels,stride,x+1,y+1,1,inkeep.keyrowh-3,0x000040ff);
  fake_keyboard_fill_rect(pixels,stride,x+inkeep.keycolw-3,y+1,1,inkeep.keyrowh-3,0x000040ff);
  fake_keyboard_fill_rect(pixels,stride,x+1,y+inkeep.keyrowh-3,inkeep.keycolw-3,1,0x000040ff);
}

/* Redraw overlay if dirty.
 */
 
static void fake_keyboard_require_overlay(int w,int h) {
  if (!inkeep.keyoverlay_dirty) return;
  if (!inkeep.keyoverlay_texid) {
    if ((inkeep.keyoverlay_texid=egg_texture_new())<1) {
      inkeep.keyoverlay_texid=0;
      return;
    }
  }
  int stride=w<<2;
  void *pixels=calloc(stride,h);
  if (!pixels) return;
  
  const int *src=inkeep.keycapv+inkeep.keycap_pagep*inkeep.keycap_rowc*inkeep.keycap_colc;
  int y=inkeep.keyrowh>>1;
  int yi=inkeep.keycap_rowc;
  for (;yi-->0;y+=inkeep.keyrowh) {
    int x=inkeep.keycolw>>1;
    int xi=inkeep.keycap_colc;
    for (;xi-->0;x+=inkeep.keycolw,src++) {
      if (inkeep.quickshift&&(*src==3)) fake_keyboard_render_shift(pixels,stride,x-(inkeep.keycolw>>1),y-(inkeep.keyrowh>>1));
      font_render_char_rgba(pixels,w,h,stride,x,y,inkeep.font,*src,0x000000);
    }
  }
  
  egg_texture_upload(inkeep.keyoverlay_texid,w,h,w<<2,EGG_TEX_FMT_RGBA,pixels,stride*h);
  free(pixels);
  inkeep.keyoverlay_dirty=0;
}

/* Render.
 */
 
void fake_keyboard_render() {
  int h=inkeep_get_keyboard_height();
  if (h<1) return;
  egg_draw_rect(1,0,inkeep.screenh-h,inkeep.screenw,h,0x00000080);
  int boardw=inkeep.keycap_colc*inkeep.keycolw;
  int boardh=inkeep.keycap_rowc*inkeep.keyrowh;
  int boardx=(inkeep.screenw>>1)-(boardw>>1);
  int boardy=inkeep.screenh-MARGIN-boardh;
  
  // Draw a rect for each key.
  int y=boardy,yi=inkeep.keycap_rowc,row=0;
  for (;yi-->0;y+=inkeep.keyrowh,row++) {
    int x=boardx,xi=inkeep.keycap_colc,col=0;
    for (;xi-->0;x+=inkeep.keycolw,col++) {
      if ((row==inkeep.keycap_rowp)&&(col==inkeep.keycap_colp)) {
        egg_draw_rect(1,x-1,y-1,inkeep.keycolw+1,inkeep.keyrowh+1,0xffff00ff);
      }
      egg_draw_rect(1,x,y,inkeep.keycolw-1,inkeep.keyrowh-1,0xc0c0c0e0);
    }
  }
  
  if (inkeep.font) {
    fake_keyboard_require_overlay(boardw,boardh);
    egg_draw_decal(1,inkeep.keyoverlay_texid,boardx,boardy,0,0,boardw,boardh,0);
  } else {
    //TODO Do we really want to support tiles in addition to font? It's kind of a pain to do both.
  }
}

/* Activate.
 */
 
static void fake_keyboard_activate() {
  if (inkeep.keycap_colp<0) return;
  if (inkeep.keycap_rowp<0) return;
  if (inkeep.keycap_colp>=inkeep.keycap_colc) return;
  if (inkeep.keycap_rowp>=inkeep.keycap_rowc) return;
  int codepoint=inkeep.keycapv[
    inkeep.keycap_pagep*inkeep.keycap_rowc*inkeep.keycap_colc+
    inkeep.keycap_rowp*inkeep.keycap_colc+
    inkeep.keycap_colp
  ];
  if (codepoint>=8) {
    inkeep_broadcast_text(codepoint);
    if (inkeep.quickshift) {
      inkeep.quickshift=0;
      inkeep.keycap_pagep^=1;
      if (inkeep.keycap_pagep>=inkeep.keycap_pagec) inkeep.keycap_pagep=inkeep.keycap_pagec-1;
      inkeep.keyoverlay_dirty=1;
    }
  } else switch (codepoint) {
    case 1: if (++(inkeep.keycap_pagep)>=inkeep.keycap_pagec) inkeep.keycap_pagep=0; inkeep.keyoverlay_dirty=1; break;
    case 2: if (--(inkeep.keycap_pagep)<0) inkeep.keycap_pagep=inkeep.keycap_pagec-1; inkeep.keyoverlay_dirty=1; break;
    case 3: {
        if (inkeep.quickshift) {
          inkeep.quickshift=0;
          inkeep.keycap_pagep^=1;
          if (inkeep.keycap_pagep>=inkeep.keycap_pagec) inkeep.keycap_pagep=inkeep.keycap_pagec-1;
        } else {
          inkeep.quickshift=1;
          inkeep.keycap_pagep^=1;
          if (inkeep.keycap_pagep>=inkeep.keycap_pagec) inkeep.keycap_pagep=inkeep.keycap_pagec-1;
        }
        inkeep.keyoverlay_dirty=1;
      } break;
  }
}

/* Apply motion from joysticks.
 */
 
static void fake_keyboard_apply_motion() {
  inkeep.keycap_colp+=inkeep.keydx;
  if (inkeep.keycap_colp<0) inkeep.keycap_colp=inkeep.keycap_colc-1;
  else if (inkeep.keycap_colp>=inkeep.keycap_colc) inkeep.keycap_colp=0;
  inkeep.keycap_rowp+=inkeep.keydy;
  if (inkeep.keycap_rowp<0) inkeep.keycap_rowp=inkeep.keycap_rowc-1;
  else if (inkeep.keycap_rowp>=inkeep.keycap_rowc) inkeep.keycap_rowp=0;
  inkeep.keymoveclock+=MOTION_TIME_REPEAT;
}

/* Update.
 */
 
void fake_keyboard_update(double elapsed) {
  if (inkeep.keydx||inkeep.keydy) {
    if ((inkeep.keymoveclock-=elapsed)<=0.0) {
      fake_keyboard_apply_motion();
    }
  }
}

/* Adjust focus.
 */
 
void fake_keyboard_focus_at(int x,int y) {
  int h=inkeep_get_keyboard_height();
  if (h<1) return;
  int boardw=inkeep.keycap_colc*inkeep.keycolw;
  int boardh=inkeep.keycap_rowc*inkeep.keyrowh;
  int boardx=(inkeep.screenw>>1)-(boardw>>1);
  int boardy=inkeep.screenh-MARGIN-boardh;
  if (y<boardy) inkeep.keycap_rowp=-1;
  else if ((inkeep.keycap_rowp=(y-boardy)/inkeep.keyrowh)>=inkeep.keycap_rowc) inkeep.keycap_rowp=-1;
  if (x<boardx) inkeep.keycap_colp=-1;
  else if ((inkeep.keycap_colp=(x-boardx)/inkeep.keycolw)>=inkeep.keycap_colc) inkeep.keycap_colp=-1;
}

/* Press a key.
 */
 
void fake_keyboard_activate_at(int x,int y) {
  fake_keyboard_focus_at(x,y);
  fake_keyboard_activate();
}

/* Begin motion.
 */
 
static void fake_keyboard_begin_motion(int dx,int dy) {
  inkeep.keydx=dx;
  inkeep.keydy=dy;
  fake_keyboard_apply_motion();
  inkeep.keymoveclock=MOTION_TIME_INITIAL;
}

/* Joystick event.
 */
 
void fake_keyboard_joy(int btnid,int value) {
  switch (btnid) {
    case EGG_JOYBTN_LEFT: if (value) fake_keyboard_begin_motion(-1,0); else if (inkeep.keydx<0) inkeep.keydx=0; break;
    case EGG_JOYBTN_RIGHT: if (value) fake_keyboard_begin_motion(1,0); else if (inkeep.keydx>0) inkeep.keydx=0; break;
    case EGG_JOYBTN_UP: if (value) fake_keyboard_begin_motion(0,-1); else if (inkeep.keydy<0) inkeep.keydy=0; break;
    case EGG_JOYBTN_DOWN: if (value) fake_keyboard_begin_motion(0,1); else if (inkeep.keydy>0) inkeep.keydy=0; break;
    case EGG_JOYBTN_SOUTH: if (value) fake_keyboard_activate(); break;
    case EGG_JOYBTN_WEST: if (value) inkeep_broadcast_text(0x08); break;
    case EGG_JOYBTN_AUX1: if (value) inkeep_broadcast_text(0x0a); break;
  }
}
