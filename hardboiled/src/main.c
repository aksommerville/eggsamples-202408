#include "hardboiled.h"

void egg_client_quit() {
  inkeep_quit();
}

static void cb_text(int codepoint,void *userdata) {
  egg_log("%s U+%x",__func__,codepoint);
}

static void cb_touch(int state,int x,int y,void *userdata) {
  switch (state) {
    case EGG_TOUCH_BEGIN: {
        inkeep_set_cursor(7,32,0,16,16,0,4,1);
      } break;
    case EGG_TOUCH_END: {
        inkeep_set_cursor(7,0,0,16,16,0,4,1);
      } break;
  }
}

// When in TEXT mode, we won't get fake TOUCH events from inkeep.
// Fake it ourselves, from the mouse only.
//TODO Should inkeep send these?
static void cb_mmotion(int x,int y,void *userdata) {
  cb_touch(2,x,y,userdata);
}
static void cb_mbutton(int btnid,int value,int x,int y,void *userdata) {
  if (btnid!=EGG_MBUTTON_LEFT) return;
  cb_touch(value?1:0,x,y,userdata);
}

int egg_client_init() {

  struct font *font=font_new(9);
  font_add_page(font,3,0x21);
  font_add_page(font,4,0xa1);
  font_add_page(font,5,0x400);
  font_add_page(font,6,0x01);

  if (inkeep_init()<0) return -1;
  if (inkeep_set_font(font)<0) return -1;
  if (inkeep_set_keycaps(3,3,12,
     ".qwertyuiop\x08"
     "?asdfghjkl\"\x0a"
    "\3zxcvbnm<> \x01"
          ",QWERTYUIOP\x08"
          "|ASDFGHJKL:\x0a"
         "\3ZXCVBNM{} \x01"
     "`1234567890\x08"
     "~!@#$%^&*()\x0a"
     "+-=[]\\;'/_ \x01"
  )<0) return -1;
  if (inkeep_set_cursor(7,0,0,16,16,0,4,1)<0) return -1;
  inkeep_set_mode(INKEEP_MODE_TEXT);
  inkeep_listen_all(0,0,cb_text,cb_touch,cb_mmotion,cb_mbutton,0,0,0,0);
  
  return 0;
}

void egg_client_update(double elapsed) {
  inkeep_update(elapsed);
}

void egg_client_render() {
  egg_draw_rect(1,0,0,SCREENW,SCREENH,0xa08040ff);
  inkeep_render();
}
