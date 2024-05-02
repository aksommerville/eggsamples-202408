#include <egg/egg.h>
#include "utility/tile_renderer.h"
#include "utility/font.h"
#include "input/bus.h"
#include "input/fkbd.h"

static int screenw=0,screenh=0;
static int texid_fonttiles=0;
static int texid_longtext=0;
static int longtextw=0,longtexth=0;
static struct tile_renderer *tile_renderer=0;
static struct font *font=0;
static struct bus *bus=0;

/* Bus callbacks.
 */
 
static void on_event(const struct egg_event *event) {
  egg_log("%s %d [%d,%d,%d,%d]",__func__,event->type,event->v[0],event->v[1],event->v[2],event->v[3]);
}
 
static void on_text(int codepoint) {
  egg_log("%s U+%x",__func__,codepoint);
}

static void on_mmotion(int x,int y) {
  egg_log("%s %d,%d",__func__,x,y);
}

static void on_mbutton(int btnid,int value,int x,int y) {
  egg_log("%s %d=%d @%d,%d",__func__,btnid,value,x,y);
}

static void on_mwheel(int dx,int dy,int x,int y) {
  egg_log("%s %d,%d @%d,%d",__func__,dx,dy,x,y);
}

static void on_player(int plrid,int btnid,int value,int state) {
  egg_log("%s %d.%04x=%d [%04x]",__func__,plrid,btnid,value,state);
}

/* Init.
 */

int egg_client_init() {
  
  egg_texture_get_header(&screenw,&screenh,0,1);
  
  if (egg_texture_load_image(texid_fonttiles=egg_texture_new(),0,1)<0) return -1;
  
  if (!(tile_renderer=tile_renderer_new())) return -1;
  if (!(font=font_new(9))) return -1;
  if (font_add_page(font,2,0x0021)<0) return -1; // ASCII
  if (font_add_page(font,3,0x00a1)<0) return -1; // Latin-1
  if (font_add_page(font,4,0x0400)<0) return -1; // Cyrillic
  if (font_add_page(font,5,0x0001)<0) return -1; // Control icons for fake keyboard.
  egg_log("Decoded font. rowh=%d, %d glyphs in %d pages.",font_get_rowh(font),font_count_glyphs(font),font_count_pages(font));
  
  if ((texid_longtext=font_render_new_texture(font,
    "This is some long text that will surely need to break and show us how automatic line measurement and breaking works.\n <- Should be some indent right here due to the explicit newline.",-1,
    200,0xffffff
  ))<1) return -1;
  egg_texture_get_header(&longtextw,&longtexth,0,texid_longtext);
  
  struct bus_delegate delegate={
    .on_text=on_text,
  };
  if (!(bus=bus_new(&delegate))) return -1;
  //fkbd_set_tilesheet(bus_get_fkbd(bus),texid_fonttiles);
  fkbd_set_font(bus_get_fkbd(bus),font);
  bus_mode_text(bus);
  
  return 0;
}

/* Update.
 */

void egg_client_update(double elapsed) {
  bus_update(bus,elapsed);
}

/* Render.
 */

void egg_client_render() {
  egg_draw_rect(1,0,0,screenw,screenh,0x002040ff);
  tile_renderer_begin(tile_renderer,texid_fonttiles,0xffff00ff,0xff);
  tile_renderer_string(tile_renderer,10,10,"The quick brown fox",-1);
  tile_renderer_string(tile_renderer,10,18,"jumps over the lazy dog.",-1);
  tile_renderer_end(tile_renderer);
  egg_draw_decal(1,texid_longtext,10,30,0,0,longtextw,longtexth,0);
  bus_render(bus);
}
