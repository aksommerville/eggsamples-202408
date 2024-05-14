/* lightson.c
 * An Egg ROM that touches the entire public API.
 */
 
#include <egg/egg.h>
#include "resid.h" // Gets built by our Makefile, with eggdev's help.
#include "util/tile_renderer.h"
#include "util/text.h"
#include "util/font.h"
#include "menu/menu.h"
#include "stdlib/egg-stdlib.h"

// We of course know this already, it's in our metadata file.
// I feel it's safer to read back from the API during init.
static int screenw,screenh;

static int texid_font_tiles=0;
static struct tile_renderer tile_renderer={0};
static struct font *font=0;
static struct menu *menu=0;
int texid_misc=0;
 
void egg_client_quit() {
}

void lightson_default_event_mask(int log) {
  #define ENEV(tag) if (!egg_event_enable(EGG_EVENT_##tag,1)) { if (log) egg_log("Failed to enable %s events.",#tag); }
  #define DISEV(tag) egg_event_enable(EGG_EVENT_##tag,0);
  ENEV(JOY) // JOY, KEY, and TEXT are enabled by default anyway.
  ENEV(KEY)
  ENEV(TEXT)
  ENEV(MMOTION) // Enabling any mouse event also makes the system cursor visible.
  ENEV(MBUTTON)
  ENEV(MWHEEL)
  ENEV(TOUCH) // Enabled by default.
  DISEV(ACCEL) // Bad idea to enable, it causes a lot of noise if it actually exists.
  DISEV(RAW) // Prefer JOY.
  #undef ENEV
  #undef DISEV
}

int egg_client_init() {

  egg_texture_get_header(&screenw,&screenh,0,1);
  
  if (egg_texture_load_image(texid_font_tiles=egg_texture_new(),0,RID_image_font_tiles)<0) return -1;
  if (egg_texture_load_image(texid_misc=egg_texture_new(),0,RID_image_misctiles)<0) return -1;
  
  if (!(font=font_new(9))) return -1;
  if (font_add_page(font,RID_image_font_9h_21,0x21)<0) return -1;
  if (font_add_page(font,RID_image_font_9h_a1,0xa1)<0) return -1;
  if (font_add_page(font,RID_image_font_9h_400,0x400)<0) return -1;
  if (font_add_page(font,RID_image_font_9h_01,0x01)<0) return -1;
  
  lightson_default_event_mask(1);
  
  if (!(menu=menu_new_main())) return -1;
  menu->font=font;
  menu->screenw=screenw;
  menu->screenh=screenh;
  menu->tile_renderer=&tile_renderer;
  menu->tilesheet=texid_font_tiles;

  return 0;
}
 
void pop_menu() {
  if (!menu) {
    egg_request_termination();
    return;
  }
  struct menu *front=menu,*parent=0;
  while (front->child) {
    parent=front;
    front=front->child;
  }
  if (parent) parent->child=0;
  menu_del(front);
  if (front==menu) {
    menu=0;
    egg_request_termination();
  }
}

void egg_client_update(double elapsed) {

  /* It's wise to poll multiple events at once, more efficient.
   * But you could egg_event_get(&event,1) for legibility too, not a big deal.
   */
  union egg_event eventv[16];
  int eventc;
  while ((eventc=egg_event_get(eventv,16))>0) {
    const union egg_event *event=eventv;
    int i=eventc; for (;i-->0;event++) {
    
      // We control dismissing of menus, and we have a way of doing it for every reasonable event class.
      switch (event->type) {
        case EGG_EVENT_KEY: if (event->key.value) switch (event->key.keycode) {
            case 0x00070029: pop_menu(); continue; // Escape
          } break;
        case EGG_EVENT_JOY: if (event->joy.value) switch (event->joy.btnid) {
            case EGG_JOYBTN_WEST: pop_menu(); continue; 
          } break;
        case EGG_EVENT_MBUTTON: if (event->mbutton.value) switch (event->mbutton.btnid) {
            case EGG_MBUTTON_LEFT: if ((event->mbutton.x>=0)&&(event->mbutton.y>=0)&&(event->mbutton.x<6)&&(event->mbutton.y<6)) {
                pop_menu();
                continue;
              } break;
          } break;
        case EGG_EVENT_TOUCH: if ((event->touch.state==1)&&(event->touch.x<6)&&(event->touch.y<6)) {
            pop_menu();
            continue;
          } break;
      }
      
      menu_event(menu,event);
    }
  }
  
  menu_update(menu,elapsed);
}

void egg_client_render() {
  egg_draw_rect(1,0,0,screenw,screenh,0x203040ff);
  egg_draw_rect(1,0,0,6,6,0xff0000ff);
  menu_render(menu);
}
