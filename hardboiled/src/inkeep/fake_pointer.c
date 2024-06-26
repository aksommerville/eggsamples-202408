#include "inkeep_internal.h"
#include <egg/hid_keycode.h>

/* Render.
 */
 
void fake_pointer_render() {
  if (inkeep.fakemouse_soft_hide) return;
  int dstx,dsty;
  if (inkeep.mode==INKEEP_MODE_TOUCH) {
    dstx=inkeep.touchx;
    dsty=inkeep.touchy;
  } else {
    dstx=inkeep.mousex;
    dsty=inkeep.mousey;
  }
  if ((dstx<0)||(dsty<0)) return;
  int fbw=0,fbh=0;
  egg_texture_get_header(&fbw,&fbh,0,1);
  if ((dstx>=fbw)||(dsty>=fbh)) return;
  dstx-=inkeep.cursorhotx;
  dsty-=inkeep.cursorhoty;
  egg_draw_decal(1,inkeep.cursor_texid,dstx,dsty,inkeep.cursorx,inkeep.cursory,inkeep.cursorw,inkeep.cursorh,inkeep.cursorxform);
}

/* Update.
 */

void fake_pointer_update(double elapsed) {
  inkeep.fakemousex+=elapsed*inkeep.fakemouse_speed*inkeep.cursordx;
  inkeep.fakemousey+=elapsed*inkeep.fakemouse_speed*inkeep.cursordy;
  int ix=(int)inkeep.fakemousex;
  int iy=(int)inkeep.fakemousey;
  if (inkeep.mode==INKEEP_MODE_TOUCH) {
    if ((ix!=inkeep.touchx)||(iy!=inkeep.touchy)) {
      inkeep.fakemouse_soft_hide=0;
      inkeep.touchx=inkeep.mousex=ix;
      inkeep.touchy=inkeep.mousey=iy;
      inkeep_broadcast_touch(EGG_TOUCH_MOVE,ix,iy);
    }
  } else {
    if ((ix!=inkeep.mousex)||(iy!=inkeep.mousey)) {
      inkeep.fakemouse_soft_hide=0;
      inkeep.mousex=inkeep.touchx=ix;
      inkeep.mousey=inkeep.touchy=iy;
      inkeep_broadcast_mmotion(ix,iy);
    }
  }
}

/* Joystick and key events.
 */
 
static void fake_pointer_button(int value) {
  if (value) {
    if (inkeep.cursorbtn) return;
    inkeep.cursorbtn=1;
  } else {
    if (!inkeep.cursorbtn) return;
    inkeep.cursorbtn=0;
  }
  switch (inkeep.mode) {
    case INKEEP_MODE_TOUCH: inkeep_broadcast_touch(inkeep.cursorbtn?EGG_TOUCH_BEGIN:EGG_TOUCH_END,inkeep.mousex,inkeep.mousey); break;
    case INKEEP_MODE_MOUSE: inkeep_broadcast_mbutton(EGG_MBUTTON_LEFT,inkeep.cursorbtn,inkeep.mousex,inkeep.mousey); break;
  }
}
 
void fake_pointer_joy(int btnid,int value) {
  switch (btnid) {
    case EGG_JOYBTN_LEFT: if (value) inkeep.cursordx=-1.0; else if (inkeep.cursordx<0.0) inkeep.cursordx=0.0; break;
    case EGG_JOYBTN_RIGHT: if (value) inkeep.cursordx=1.0; else if (inkeep.cursordx>0.0) inkeep.cursordx=0.0; break;
    case EGG_JOYBTN_UP: if (value) inkeep.cursordy=-1.0; else if (inkeep.cursordy<0.0) inkeep.cursordy=0.0; break;
    case EGG_JOYBTN_DOWN: if (value) inkeep.cursordy=1.0; else if (inkeep.cursordy>0.0) inkeep.cursordy=0.0; break;
    case EGG_JOYBTN_SOUTH: fake_pointer_button(value); break;
    //TODO LX,LY should control it analoguely.
  }
}

void fake_pointer_key(int keycode,int value) {
  switch (keycode) {
    case KEY_KP4: case KEY_A: case KEY_LEFT: if (value) inkeep.cursordx=-1.0; else if (inkeep.cursordx<0.0) inkeep.cursordx=0.0; break;
    case KEY_KP6: case KEY_D: case KEY_RIGHT: if (value) inkeep.cursordx=1.0; else if (inkeep.cursordx>0.0) inkeep.cursordx=0.0; break;
    case KEY_KP8: case KEY_W: case KEY_UP: if (value) inkeep.cursordy=-1.0; else if (inkeep.cursordy<0.0) inkeep.cursordy=0.0; break;
    case KEY_KP5: case KEY_KP2: case KEY_S: case KEY_DOWN: if (value) inkeep.cursordy=1.0; else if (inkeep.cursordy>0.0) inkeep.cursordy=0.0; break;
    case KEY_KP0: case KEY_SPACE: fake_pointer_button(value); break;
  }
}

void fake_pointer_mmotion(int x,int y) {
  inkeep.fakemouse_soft_hide=0;
  inkeep.fakemousex=inkeep.mousex=inkeep.touchx=x;
  inkeep.fakemousey=inkeep.mousey=inkeep.touchy=y;
  if (inkeep.mode==INKEEP_MODE_TOUCH) {
    inkeep_broadcast_touch(EGG_TOUCH_MOVE,x,y);
  }
}

void fake_pointer_mbutton(int btnid,int value) {
  if (btnid!=EGG_MBUTTON_LEFT) return;
  fake_pointer_button(value);
}
