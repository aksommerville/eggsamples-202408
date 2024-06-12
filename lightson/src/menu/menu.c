#include <egg/egg.h>
#include "menu.h"
#include "stdlib/egg-stdlib.h"
#include "util/font.h"
#include "util/tile_renderer.h"

/* Generic menu.
 */

void menu_del(struct menu *menu) {
  if (!menu) return;
  if (menu->child) menu_del(menu->child);
  if (menu->del) menu->del(menu);
  if (menu->optionv) {
    while (menu->optionc-->0) free(menu->optionv[menu->optionc]);
    free(menu->optionv);
  }
  if (menu->optiontex) egg_texture_del(menu->optiontex);
  free(menu);
}

struct menu *menu_new(int size,struct menu *parent) {
  if (parent&&parent->child) return 0;
  if (size<(int)sizeof(struct menu)) return 0;
  struct menu *menu=calloc(1,size);
  if (!menu) return 0;
  menu->optionp=-1;
  if (parent) {
    parent->child=menu;
    menu->font=parent->font;
    menu->screenw=parent->screenw;
    menu->screenh=parent->screenh;
    menu->tile_renderer=parent->tile_renderer;
    menu->tilesheet=parent->tilesheet;
  }
  return menu;
}

void menu_event(struct menu *menu,const union egg_event *event) {
  if (!menu) return;
  while (menu->child) menu=menu->child;
  if (!menu->event) return;
  menu->event(menu,event);
}

void menu_update(struct menu *menu,double elapsed) {
  if (!menu) return;
  while (menu->child) menu=menu->child;
  if (!menu->update) return;
  menu->update(menu,elapsed);
}

void menu_render(struct menu *menu) {
  if (!menu) return;
  while (menu->child) menu=menu->child;
  if (!menu->render) return;
  menu->render(menu);
}

/* Options list.
 */
 
int menu_option_add(struct menu *menu,const char *src,int srcc) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (menu->optionc>=menu->optiona) {
    int na=menu->optiona+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(menu->optionv,sizeof(void*)*na);
    if (!nv) return -1;
    menu->optionv=nv;
    menu->optiona=na;
  }
  char *nv=malloc(srcc+1);
  if (!nv) return -1;
  memcpy(nv,src,srcc);
  nv[srcc]=0;
  menu->optionv[menu->optionc++]=nv;
  if (menu->optiontex) { // force rebuild at next render
    egg_texture_del(menu->optiontex);
    menu->optiontex=0;
  }
  return 0;
}

static void menu_option_motion(struct menu *menu,int dx,int dy) {
  egg_audio_play_sound(0,75,0x00010000,0);
  if (dy) {
    if (menu->optionc<1) return;
    menu->optionp+=dy;
    if (menu->optionp<0) menu->optionp=menu->optionc-1;
    else if (menu->optionp>=menu->optionc) menu->optionp=0;
  }
  if (dx) {
    if ((menu->optionp<0)||(menu->optionp>=menu->optionc)) return;
    if (!menu->on_option_adjust) return;
    menu->on_option_adjust(menu,dx);
  }
}

static void menu_option_activate(struct menu *menu) {
  if (!menu->on_option) return;
  if ((menu->optionp<0)||(menu->optionp>=menu->optionc)) return;
  egg_audio_play_sound(0,78,0x00010000,0);
  menu->on_option(menu);
}

static int menu_option_index_for_point(const struct menu *menu,int x,int y) {
  if (x<6) return -1;
  if (menu->optionc<1) return -1;
  int rowh=menu->font?font_get_rowh(menu->font):8;
  if (rowh<1) return -1;
  int p=(y-4)/rowh;
  if (p>=menu->optionc) return -1;
  return p;
}

static void menu_option_highlight_point(struct menu *menu,int x,int y) {
  menu->optionp=menu_option_index_for_point(menu,x,y);
}

static void menu_option_activate_point(struct menu *menu,int x,int y) {
  menu->optionp=menu_option_index_for_point(menu,x,y);
  menu_option_activate(menu);
}

void menu_option_event(struct menu *menu,const union egg_event *event) {
  switch (event->type) {
    case EGG_EVENT_JOY: if (event->joy.value) switch (event->joy.btnid) {
        case EGG_JOYBTN_UP: menu_option_motion(menu,0,-1); return;
        case EGG_JOYBTN_DOWN: menu_option_motion(menu,0,1); return;
        case EGG_JOYBTN_LEFT: menu_option_motion(menu,-1,0); return;
        case EGG_JOYBTN_RIGHT: menu_option_motion(menu,1,0); return;
        case EGG_JOYBTN_SOUTH: menu_option_activate(menu); return;
      } break;
    case EGG_EVENT_KEY: if (event->key.value) switch (event->key.keycode) {
        case 0x0007002c: menu_option_activate(menu); return; // Space
        case 0x00070028: menu_option_activate(menu); return; // Enter
        case 0x0007004f: menu_option_motion(menu,1,0); return; // Arrows...
        case 0x00070050: menu_option_motion(menu,-1,0); return;
        case 0x00070051: menu_option_motion(menu,0,1); return;
        case 0x00070052: menu_option_motion(menu,0,-1); return;
      } break;
    case EGG_EVENT_MMOTION: menu_option_highlight_point(menu,event->mmotion.x,event->mmotion.y); return;
    case EGG_EVENT_MBUTTON: if (event->mbutton.value) switch (event->mbutton.btnid) {
        case EGG_MBUTTON_LEFT: menu_option_activate_point(menu,event->mbutton.x,event->mbutton.y); return;
      } break;
    case EGG_EVENT_TOUCH: if (event->touch.state==1) {
        menu_option_activate_point(menu,event->touch.x,event->touch.y);
      } break;
  }
}

void menu_option_update(struct menu *menu,double elapsed) {
}

static int menu_option_render_font(struct menu *menu) {
  int rowh=font_get_rowh(menu->font);
  int imgw=menu->screenw-8;
  int imgh=rowh*menu->optionc;
  int stride=imgw<<2;
  uint8_t *pixels=calloc(stride,imgh);
  if (!pixels) return 0;
  char **text=menu->optionv;
  int i=menu->optionc;
  int y=0;
  for (;i-->0;text++,y+=rowh) {
    font_render_string_rgba(pixels,imgw,imgh,stride,0,y,menu->font,*text,-1,0xffffff);
  }
  int texid=egg_texture_new();
  if (texid<0) {
    free(pixels);
    return 0;
  }
  int err=egg_texture_upload(texid,imgw,imgh,stride,EGG_TEX_FMT_RGBA,pixels,stride*imgh);
  free(pixels);
  if (err<0) {
    egg_texture_del(texid);
    return 0;
  }
  return texid;
}

void menu_option_render(struct menu *menu) {
  if (menu->optionc<1) return;
  
  if (menu->font) {
    if (!menu->optiontex) {
      menu->optiontex=menu_option_render_font(menu);
    }
    int w=0,h=0;
    egg_texture_get_header(&w,&h,0,menu->optiontex);
    if ((menu->optionp>=0)&&(menu->optionp<menu->optionc)) {
      int rowh=font_get_rowh(menu->font);
      egg_draw_rect(1,2,3+rowh*menu->optionp,menu->screenw-4,rowh+1,0x000000ff);
    }
    egg_draw_decal(1,menu->optiontex,8,4,0,0,w,h,0);
    
  } else if (menu->tile_renderer) {
    if ((menu->optionp>=0)&&(menu->optionp<menu->optionc)) {
      egg_draw_rect(1,2,3+8*menu->optionp,menu->screenw-4,9,0x000000ff);
    }
    tile_renderer_begin(menu->tile_renderer,menu->tilesheet,0xffffffff,0xff);
    char **text=menu->optionv;
    int i=menu->optionc;
    int x=12;
    int y=8;
    for (;i-->0;text++,y+=8) {
      tile_renderer_string(menu->tile_renderer,x,y,*text,-1);
    }
    tile_renderer_end(menu->tile_renderer);
  }
}

static void menu_option_dirty(struct menu *menu) {
  if (menu->optiontex) {
    egg_texture_del(menu->optiontex);
    menu->optiontex=0;
  }
}

/* Tail of menu option, conveniences.
 */
 
int menu_option_read_tail_string(char *dst,int dsta,const struct menu *menu,int p) {
  int dstc=0;
  if ((p>=0)&&(p<menu->optionc)) {
    const char *src=menu->optionv[p];
    int srcc=0; while (src[srcc]) srcc++;
    while (srcc&&((unsigned char)src[srcc-1]<=0x20)) srcc--;
    const char *token=src+srcc;
    while (srcc&&((unsigned char)src[srcc-1]>0x20)) { srcc--; token--; dstc++; }
    if (dstc<=dsta) memcpy(dst,token,dstc);
  }
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}

int menu_option_read_tail_int(const struct menu *menu,int p) {
  char tmp[16];
  int tmpc=menu_option_read_tail_string(tmp,sizeof(tmp),menu,p);
  if ((tmpc<1)||(tmpc>sizeof(tmp))) return 0;
  if ((tmpc==4)&&!memcmp(tmp,"true",4)) return 1;
  if ((tmpc==5)&&!memcmp(tmp,"false",5)) return 0;
  int v=0,tmpp=0;
  if ((tmpp<tmpc)&&(tmp[tmpp]=='-')) {
    tmpp++;
    while (tmpp<tmpc) {
      int digit=tmp[tmpp++]-'0';
      if ((digit<0)||(digit>9)) return 0;
      v*=10;
      v-=digit;
    }
  } else {
    while (tmpp<tmpc) {
      int digit=tmp[tmpp++]-'0';
      if ((digit<0)||(digit>9)) return 0;
      v*=10;
      v+=digit;
    }
  }
  return v;
}

double menu_option_read_tail_double(const struct menu *menu,int p) {
  char tmp[16];
  int tmpc=menu_option_read_tail_string(tmp,sizeof(tmp),menu,p);
  if ((tmpc<1)||(tmpc>sizeof(tmp))) return 0.0;
  double v=0.0;
  int tmpp=0,positive=1;
  if ((tmpp<tmpc)&&(tmp[tmpp]=='-')) { positive=0; tmpp++; }
  while ((tmpp<tmpc)&&(tmp[tmpp]>='0')&&(tmp[tmpp]<='9')) {
    int digit=tmp[tmpp++]-'0';
    v*=10.0;
    if (positive) v+=digit; else v-=digit;
  }
  if ((tmpp<tmpc)&&(tmp[tmpp]=='.')) {
    tmpp++;
    double coef=1.0;
    while ((tmpp<tmpc)&&(tmp[tmpp]>='0')&&(tmp[tmpp]<='9')) {
      int digit=tmp[tmpp++]-'0';
      coef*=0.1;
      if (positive) v+=digit*coef; else v-=digit*coef;
    }
  }
  return v;
}

int menu_option_rewrite_tail_string(struct menu *menu,int p,const char *src,int srcc) {
  if ((p<0)||(p>=menu->optionc)) return -1;
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  char *prev=menu->optionv[p];
  int prevc=0; while (prev[prevc]) prevc++;
  while (prevc&&((unsigned char)prev[prevc-1]<=0x20)) prevc--;
  while (prevc&&((unsigned char)prev[prevc-1]>0x20)) prevc--;
  while (prevc&&((unsigned char)prev[prevc-1]<=0x20)) prevc--;
  int nc=prevc+1+srcc;
  char *nv=malloc(nc+1);
  if (!nv) return -1;
  memcpy(nv,prev,prevc);
  nv[prevc]=' ';
  memcpy(nv+prevc+1,src,srcc);
  nv[nc]=0;
  free(prev);
  menu->optionv[p]=nv;
  menu_option_dirty(menu);
  return 0;
}

int menu_option_rewrite_tail_int(struct menu *menu,int p,int v) {
  char tmp[16];
  int tmpc=0;
  if (v<0) {
    tmp[tmpc++]='-';
    v=-v;
  }
  if (v>=1000000000) tmp[tmpc++]='0'+v/1000000000;
  if (v>= 100000000) tmp[tmpc++]='0'+(v/100000000)%10;
  if (v>=  10000000) tmp[tmpc++]='0'+(v/10000000)%10;
  if (v>=   1000000) tmp[tmpc++]='0'+(v/1000000)%10;
  if (v>=    100000) tmp[tmpc++]='0'+(v/100000)%10;
  if (v>=     10000) tmp[tmpc++]='0'+(v/10000)%10;
  if (v>=      1000) tmp[tmpc++]='0'+(v/1000)%10;
  if (v>=       100) tmp[tmpc++]='0'+(v/100)%10;
  if (v>=        10) tmp[tmpc++]='0'+(v/10)%10;
  tmp[tmpc++]='0'+v%10;
  return menu_option_rewrite_tail_string(menu,p,tmp,tmpc);
}

int menu_option_rewrite_tail_double(struct menu *menu,int p,double v) {
  char tmp[8];
  int tmpc=0;
  if (v<0.0) {
    tmp[tmpc++]='-';
    v=-v;
  }
  if (v>=100.0) {
    int n=((int)v/100)%10;
    if (n<0) n=0; else if (n>9) n=9;
    tmp[tmpc++]='0'+n;
  }
  if (v>=10.0) {
    int n=((int)v/10)%10;
    if (n<0) n=0; else if (n>9) n=9;
    tmp[tmpc++]='0'+n;
  }
  int n=(int)v%10;
  if (n<0) n=0; else if (n>9) n=9;
  tmp[tmpc++]='0'+n;
  tmp[tmpc++]='.';
  tmp[tmpc++]='0'+((int)(v*10.0)%10);
  tmp[tmpc++]='0'+((int)(v*100.0)%10);
  tmp[tmpc++]='0'+((int)(v*1000.0)%10);
  return menu_option_rewrite_tail_string(menu,p,tmp,tmpc);
}
