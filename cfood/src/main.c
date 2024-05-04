#include <egg/egg.h>
#include "utility/tile_renderer.h"
#include "utility/font.h"
#include "utility/text.h"
#include "input/bus.h"
#include <string.h>

static int screenw=0,screenh=0;
static int texid_fonttiles=0;
static int texid_longtext=0;
static int texid_cursor=0;
static int longtextw=0,longtexth=0;
static char textentry[1024];
static int textentryc=0;
static int lang=0x656e; // en
static int joyselectp=0;
static struct tile_renderer *tile_renderer=0;
static struct font *font=0;
static struct bus *bus=0;

static struct button {
  int x,y,w,h;
  int label_texid;
  int lblw,lblh;
  int focus;
} buttonv[5];

static void unfocus_buttons() {
  struct button *button=buttonv;
  int i=sizeof(buttonv)/sizeof(buttonv[0]);
  for (;i-->0;button++) button->focus=0;
}

static void activate_button(int p) {
  int c=sizeof(buttonv)/sizeof(buttonv[0]);
  if ((p<0)||(p>=c)) return;
  unfocus_buttons();
  struct button *button=buttonv+p;
  button->focus=1;
  joyselectp=p;
  switch (p) {
    case 0: bus_mode_raw(bus); break;
    case 1: bus_mode_joy(bus); break;
    case 2: bus_mode_text(bus); break;
    case 3: bus_mode_point(bus); break;
    case 4: bus_mode_config(bus,JCFG_DEVID_ANY); break;
  }
}

/* Bus callbacks.
 */
 
static void on_event(const struct egg_event *event) {
  //egg_log("%s %d [%d,%d,%d,%d]",__func__,event->type,event->v[0],event->v[1],event->v[2],event->v[3]);
  switch (event->type) {
    case EGG_EVENT_KEY: if (event->v[1]) switch (event->v[0]) {
        case 0x00070029: egg_request_termination(); break; // esc
        case 0x0007003a: activate_button(0); break; // f1
        case 0x0007003b: activate_button(1); break; // f2
        case 0x0007003c: activate_button(2); break; // f3
        case 0x0007003d: activate_button(3); break; // f4
        case 0x0007003e: activate_button(4); break; // f5
        //default: egg_log("key: 0x%08x",event->v[0]);
      } break;
  }
}

static void on_text_command(const char *src,int srcc) {
  if ((srcc==3)&&!memcmp(src,"raw",3)) { activate_button(0); }
  else if ((srcc==3)&&!memcmp(src,"joy",3)) { activate_button(1); }
  else if ((srcc==4)&&!memcmp(src,"text",4)) { activate_button(2); }
  else if ((srcc==5)&&!memcmp(src,"point",5)) { activate_button(3); }
  else if ((srcc==6)&&!memcmp(src,"config",6)) { activate_button(4); }
  else {
    egg_log("Unknown command: '%.*s'",srcc,src);
  }
}
 
static void on_text(int codepoint) {
  if (codepoint==0x08) {
    if (textentryc) {
      textentryc--;
      while (textentryc&&((textentry[textentryc]&0xc0)==0x80)) textentryc--;
    }
  } else if (codepoint==0x0a) {
    on_text_command(textentry,textentryc);
    textentryc=0;
  } else {
    int err=text_utf8_encode(textentry+textentryc,sizeof(textentry)-textentryc,codepoint);
    if (err<=0) return;
    if (textentryc+err<=sizeof(textentry)) textentryc+=err;
  }
  font_render_to_texture(texid_longtext,font,textentry,textentryc,200,0xffffff);
  egg_texture_get_header(&longtextw,&longtexth,0,texid_longtext);
}

static void on_mmotion(int x,int y) {
}

static void on_mbutton(int btnid,int value,int x,int y) {
  //egg_log("%s %d=%d",__func__,btnid,value);
  if (btnid==1) {
    // Hand open/close:
    //if (value) bus_set_cursor(bus,texid_cursor,32,0,16,16,0,8,8);
    //else bus_set_cursor(bus,texid_cursor,16,0,16,16,0,8,8);
    if (value) {
      struct button *button=buttonv;
      int i=0;
      int c=sizeof(buttonv)/sizeof(buttonv[0]);
      for (;i<c;i++,button++) {
        if (x<button->x) continue;
        if (y<button->y) continue;
        if (x>=button->x+button->w) continue;
        if (y>=button->y+button->h) continue;
        activate_button(i);
        break;
      }
    }
  }
}

static void on_mwheel(int dx,int dy,int x,int y) {
  //egg_log("%s %d,%d @%d,%d",__func__,dx,dy,x,y);
}

static void on_player(int plrid,int btnid,int value,int state) {
  //egg_log("%s %d.%04x=%d [%04x]",__func__,plrid,btnid,value,state);
  switch (bus_get_mode(bus)) {
    case BUS_MODE_TEXT: return;
    case BUS_MODE_POINT: return;
  }
  if (!plrid&&value) switch (btnid) {
    case 0x0001: if (--joyselectp<0) joyselectp=4; break;
    case 0x0002: if (++joyselectp>=5) joyselectp=0; break;
    case 0x0010: activate_button(joyselectp); break;
  }
}

static void on_mode_change(int mode) {
  unfocus_buttons();
  switch (mode) {
    case BUS_MODE_RAW: buttonv[0].focus=1; break;
    case BUS_MODE_JOY: buttonv[1].focus=1; break;
    case BUS_MODE_TEXT: buttonv[2].focus=1; break;
    case BUS_MODE_POINT: buttonv[3].focus=1; break;
    case BUS_MODE_CONFIG: buttonv[4].focus=1; break;
  }
}

/* My GUI.
 */
 
static void button_init(struct button *button,int p,int c,int lblstringid) {
  button->y=5;
  button->h=20;
  button->x=(p*screenw)/c;
  button->w=screenw/c;
  button->x+=5;
  button->w-=10;
  char text[256];
  int textc=egg_res_get(text,sizeof(text),EGG_TID_string,lang,lblstringid);
  if ((textc<0)||(textc>sizeof(text))) textc=0;
  button->label_texid=font_render_new_texture(font,text,textc,0,0x000000);
  egg_texture_get_header(&button->lblw,&button->lblh,0,button->label_texid);
}

/* Init.
 */

int egg_client_init() {

  lang=text_init();
  
  egg_texture_get_header(&screenw,&screenh,0,1);
  
  if (egg_texture_load_image(texid_fonttiles=egg_texture_new(),0,1)<0) return -1;
  if (egg_texture_load_image(texid_cursor=egg_texture_new(),0,7)<0) return -1;
  
  if (!(tile_renderer=tile_renderer_new())) return -1;
  if (!(font=font_new(9))) return -1;
  if (font_add_page(font,2,0x0021)<0) return -1; // ASCII
  if (font_add_page(font,3,0x00a1)<0) return -1; // Latin-1
  if (font_add_page(font,4,0x0400)<0) return -1; // Cyrillic
  if (font_add_page(font,5,0x0001)<0) return -1; // Control icons for fake keyboard.
  egg_log("Decoded font. rowh=%d, %d glyphs in %d pages.",font_get_rowh(font),font_count_glyphs(font),font_count_pages(font));
  
  if ((texid_longtext=font_render_new_texture(font,"",0,200,0xffffff))<1) return -1;
  egg_texture_get_header(&longtextw,&longtexth,0,texid_longtext);
  
  struct bus_delegate delegate={
    .on_event=on_event,
    .on_text=on_text,
    .on_mmotion=on_mmotion,
    .on_mbutton=on_mbutton,
    .on_mwheel=on_mwheel,
    .on_player=on_player,
    .on_mode_change=on_mode_change,
  };
  if (!(bus=bus_new(&delegate))) return -1;
  bus_set_cursor(bus,texid_cursor,0,0,16,16,0,1,4);
  //bus_set_tilesheet(bus,texid_fonttiles);
  bus_set_font(bus,font);
  bus_set_strings(bus,22,23,24,25);
  bus_define_joystick_mapping(bus,JLOG_DEFAULT_FULL_GAMEPAD(2));
  bus_set_player_count(bus,4);
  
  button_init(buttonv+0,0,5,17);
  button_init(buttonv+1,1,5,18);
  button_init(buttonv+2,2,5,19);
  button_init(buttonv+3,3,5,20);
  button_init(buttonv+4,4,5,21);
  
  bus_mode_text(bus); // _raw _joy _text _point
  buttonv[2].focus=1;
  
  return 0;
}

/* Update.
 */

void egg_client_update(double elapsed) {
  bus_update(bus,elapsed);
}

/* Render.
 */
 
static int describe_player(char *dst,int dsta,int playerid,int state) {
  if (dsta<18) return 0;
  int dstc=0;
  dst[dstc++]='0'+playerid;
  dst[dstc++]=':';
  dst[dstc++]=' ';
  const char *btnnames="LRUDABCDLRlr123";
  int mask=1,i=0;
  for (;mask<0x8000;mask<<=1,i++) dst[dstc++]=(state&mask)?btnnames[i]:'.';
  return dstc;
}

void egg_client_render() {
  egg_draw_rect(1,0,0,screenw,screenh,0x002040ff);
  
  egg_draw_decal(1,texid_longtext,10,30,0,0,longtextw,longtexth,0);
  
  tile_renderer_begin(tile_renderer,texid_fonttiles,0xc0c0c0ff,0xff);
  int y=100;
  int playerid=0;
  for (;playerid<=4;playerid++,y+=8) {
    char desc[256];
    int descc=describe_player(desc,sizeof(desc),playerid,bus_get_player(bus,playerid));
    if ((descc>0)&&(descc<=sizeof(desc))) {
      tile_renderer_string(tile_renderer,16,y,desc,descc);
    }
  }
  tile_renderer_end(tile_renderer);
  
  struct button *button=buttonv;
  int i=0;
  int c=sizeof(buttonv)/sizeof(buttonv[0]);
  int mode=bus_get_mode(bus);
  for (;i<c;i++,button++) {
    egg_draw_rect(1,button->x,button->y,button->w,button->h,button->focus?0xffff00ff:0xc0c0c0ff);
    int dstx=button->x+(button->w>>1)-(button->lblw>>1);
    int dsty=button->y+(button->h>>1)-(button->lblh>>1);
    egg_draw_decal(1,button->label_texid,dstx,dsty,0,0,button->lblw,button->lblh,0);
    if ((i==joyselectp)&&(mode!=BUS_MODE_TEXT)&&(mode!=BUS_MODE_POINT)) {
      egg_draw_rect(1,button->x+2,button->y+2,button->w-4,1,0x404040ff);
      egg_draw_rect(1,button->x+2,button->y+button->h-3,button->w-4,1,0x404040ff);
      egg_draw_rect(1,button->x+2,button->y+2,1,button->h-4,0x404040ff);
      egg_draw_rect(1,button->x+button->w-3,button->y+2,1,button->h-4,0x404040ff);
    }
  }
  
  bus_render(bus);
}
