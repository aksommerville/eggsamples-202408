#include <egg/egg.h>
#include "menu.h"
#include "stdlib/egg-stdlib.h"
#include "util/tile_renderer.h"
#include "util/font.h"
#include "util/text.h"

// USB HID Boot Protocol uses a set of 6.
// Individual keyboards of course have their own limits too.
// I think 16 is safe, unlikely that any keyboard can hold that many keys down at once.
#define HELD_SIZE 16

// How long to "blip" when a repeat event arrives. Keep it short, repeats tend to be fast.
#define HIGHLIGHT_TIME 0.0625

#define TEXT_ENTRY_SIZE 32

/* Object definition.
 */
 
struct menu_keyboard {
  struct menu hdr;
  int held[HELD_SIZE];
  double highlight[HELD_SIZE]; // counts down, for highlighting repeat events
  char text_entry[TEXT_ENTRY_SIZE];
  int textdirty;
  int texttexid;
};

#define MENU ((struct menu_keyboard*)menu)

/* Cleanup.
 */
 
static void _keyboard_del(struct menu *menu) {
  if (MENU->texttexid) egg_texture_del(MENU->texttexid);
}

/* Event.
 */
 
static void _keyboard_event(struct menu *menu,const union egg_event *event) {
  switch (event->type) {
    case EGG_EVENT_KEY: if (event->key.value) {
        int heldp=-1,zerop=-1,i=HELD_SIZE;
        while (i-->0) {
          if (MENU->held[i]==event->key.keycode) {
            heldp=i;
            break;
          } else if (!MENU->held[i]) {
            zerop=i;
          }
        }
        if (heldp>=0) {
          MENU->highlight[heldp]=HIGHLIGHT_TIME;
        } else if (zerop>=0) {
          MENU->highlight[zerop]=HIGHLIGHT_TIME;
          MENU->held[zerop]=event->key.keycode;
        } else {
          egg_log("!!! Unable to record keyboard event %x=%d, buffer full",event->key.keycode,event->key.value);
        }
      } else {
        int i=HELD_SIZE;
        while (i-->0) {
          if (MENU->held[i]==event->key.keycode) {
            MENU->held[i]=0;
          }
        }
      } break;
    case EGG_EVENT_TEXT: {
        char encoded[4];
        int encodedc=text_utf8_encode(encoded,sizeof(encoded),event->text.codepoint);
        if ((encodedc<1)||(encodedc>sizeof(encoded))) {
          encoded[0]='?';
          encodedc=1;
        }
        memmove(MENU->text_entry,MENU->text_entry+encodedc,TEXT_ENTRY_SIZE-encodedc);
        memcpy(MENU->text_entry+TEXT_ENTRY_SIZE-encodedc,encoded,encodedc);
        MENU->textdirty=1;
      } break;
  }
}

/* Update.
 */
 
static void _keyboard_update(struct menu *menu,double elapsed) {
  int i=HELD_SIZE;
  while (i-->0) {
    if ((MENU->highlight[i]-=elapsed)<=0.0) MENU->highlight[i]=0.0;
  }
}

/* Refresh texttexid.
 */
 
static void keyboard_render_text(struct menu *menu) {
  MENU->textdirty=0;
  const char *src=MENU->text_entry;
  int srcc=TEXT_ENTRY_SIZE;
  while (srcc&&!src) { src++; srcc--; }
  if (!srcc) return;
  if (!MENU->texttexid) {
    if ((MENU->texttexid=egg_texture_new())<1) {
      MENU->texttexid=0;
      return;
    }
  }
  font_render_to_texture(MENU->texttexid,menu->font,src,srcc,0,0xffffff);
}

/* Screen bounds for a held key.
 * Must have enough room for 8 digits of 8x8 pixels, so 64x8 bare minimum.
 */
 
static void keyboard_get_held_bounds(int *x,int *y,int *w,int *h,const struct menu *menu,int p) {
  int col=p&1;
  int row=p>>1;
  *w=menu->screenw>>1;
  *h=menu->screenw>>3;
  *x=(*w)*col;
  *y=(*h)*row;
}

/* Render.
 */
 
static void _keyboard_render(struct menu *menu) {
  int i;
  // Text entry uses font if available.
  if (menu->font) {
    // Text entry.
    if (MENU->textdirty) keyboard_render_text(menu);
    if (MENU->texttexid) {
      int w=0,h=0;
      egg_texture_get_header(&w,&h,0,MENU->texttexid);
      egg_draw_decal(1,MENU->texttexid,menu->screenw-w-4,menu->screenh-h-4,0,0,w,h,0);
    }
  } else if (menu->tile_renderer) {
    //TODO
    tile_renderer_begin(menu->tile_renderer,menu->tilesheet,0xff0000ff,0xff);
    tile_renderer_string(menu->tile_renderer,8,8,"TODO: menu_keyboard",-1);
    tile_renderer_end(menu->tile_renderer);
  }
  // Held keys only uses tilesheet.
  // Highlight backgrounds, for fresh events.
  int p=0;
  for (i=0;i<HELD_SIZE;i++) {
    if (!MENU->held[i]) continue;
    if (MENU->highlight[i]>0.0) {
      int x,y,w,h,rgba;
      keyboard_get_held_bounds(&x,&y,&w,&h,menu,p);
      rgba=0x00408000|(int)((MENU->highlight[i]*255.0)/HIGHLIGHT_TIME);
      egg_draw_rect(1,x,y,w,h,rgba);
    }
    p++;
  }
  // Held key labels.
  tile_renderer_begin(menu->tile_renderer,menu->tilesheet,0xffffffff,0xff);
  for (i=0,p=0;i<HELD_SIZE;i++) {
    if (!MENU->held[i]) continue;
    int x,y,w,h,rgba;
    keyboard_get_held_bounds(&x,&y,&w,&h,menu,p++);
    x=x+(w>>1)-36;
    y=y+(h>>1);
    int shift=28;
    for (;shift>=0;shift-=4,x+=8) tile_renderer_tile(menu->tile_renderer,x,y,"0123456789abcdef"[(MENU->held[i]>>shift)&0xf],0);
  }
  tile_renderer_end(menu->tile_renderer);
}

/* New.
 */
 
struct menu *menu_new_keyboard(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu_keyboard),parent);
  if (!menu) return 0;
  menu->del=_keyboard_del;
  menu->event=_keyboard_event;
  menu->update=_keyboard_update;
  menu->render=_keyboard_render;
  return menu;
}
