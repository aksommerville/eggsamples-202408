#include <egg/egg.h>
#include "utility/tile_renderer.h"
#include "utility/font.h"
#include "utility/text.h"
#include "input/bus.h"
#include "input/fkbd.h"
#include "input/fkpt.h"
#include "input/joy2.h"
#include "input/jlog.h"

static int screenw=0,screenh=0;
static int texid_fonttiles=0;
static int texid_longtext=0;
static int texid_cursor=0;
static int longtextw=0,longtexth=0;
static char textentry[1024];
static int textentryc=0;
static struct tile_renderer *tile_renderer=0;
static struct font *font=0;
static struct bus *bus=0;

/* Bus callbacks.
 */
 
static void on_event(const struct egg_event *event) {
  //egg_log("%s %d [%d,%d,%d,%d]",__func__,event->type,event->v[0],event->v[1],event->v[2],event->v[3]);
}
 
static void on_text(int codepoint) {
  if (codepoint==0x08) {
    if (textentryc) {
      textentryc--;
      while (textentryc&&((textentry[textentryc]&0xc0)==0x80)) textentryc--;
    }
  } else {
    int err=text_utf8_encode(textentry+textentryc,sizeof(textentry)-textentryc,codepoint);
    if (err<=0) return;
    if (textentryc+err<=sizeof(textentry)) textentryc+=err;
  }
  font_render_to_texture(texid_longtext,font,textentry,textentryc,200,0xffffff);
  egg_texture_get_header(&longtextw,&longtexth,0,texid_longtext);
}

static void on_mmotion(int x,int y) {
  //egg_log("%s %d,%d",__func__,x,y);
}

static void on_mbutton(int btnid,int value,int x,int y) {
  //egg_log("%s %d=%d @%d,%d",__func__,btnid,value,x,y);
  if (btnid==1) {
    if (value) fkpt_set_cursor(bus_get_fkpt(bus),texid_cursor,32,0,16,16,0,8,8);
    else fkpt_set_cursor(bus_get_fkpt(bus),texid_cursor,16,0,16,16,0,8,8);
  }
}

static void on_mwheel(int dx,int dy,int x,int y) {
  //egg_log("%s %d,%d @%d,%d",__func__,dx,dy,x,y);
}

static void on_player(int plrid,int btnid,int value,int state) {
  //egg_log("%s %d.%04x=%d [%04x]",__func__,plrid,btnid,value,state);
}

/* Init.
 */

int egg_client_init() {
  
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
  };
  if (!(bus=bus_new(&delegate))) return -1;
  //fkbd_set_tilesheet(bus_get_fkbd(bus),texid_fonttiles);
  fkbd_set_font(bus_get_fkbd(bus),font);
  fkpt_set_cursor(bus_get_fkpt(bus),texid_cursor,16,0,16,16,0,8,8);
  jlog_define_mapping(bus_get_jlog(bus),JLOG_DEFAULT_FULL_GAMEPAD(2));
  jlog_set_player_count(bus_get_jlog(bus),4);
  bus_mode_text(bus); // _joy _text _point
  
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
  tile_renderer_begin(tile_renderer,texid_fonttiles,0xffff00ff,0xff);
  tile_renderer_string(tile_renderer,10,10,"The quick brown fox",-1);
  tile_renderer_string(tile_renderer,10,18,"jumps over the lazy dog.",-1);
  tile_renderer_end(tile_renderer);
  egg_draw_decal(1,texid_longtext,10,30,0,0,longtextw,longtexth,0);
  
  struct jlog *jlog=bus_get_jlog(bus);
  tile_renderer_begin(tile_renderer,texid_fonttiles,0xc0c0c0ff,0xff);
  int y=100;
  int playerid=0;
  for (;playerid<=4;playerid++,y+=8) {
    char desc[256];
    int descc=describe_player(desc,sizeof(desc),playerid,jlog_get_player(jlog,playerid));
    if ((descc>0)&&(descc<=sizeof(desc))) {
      tile_renderer_string(tile_renderer,16,y,desc,descc);
    }
  }
  tile_renderer_end(tile_renderer);
  
  bus_render(bus);
}
