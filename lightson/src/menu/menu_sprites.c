/* menu_sprites.c
 * Bounce a bunch of sprites around the screen, with a single egg_draw_tile() to render.
 * This test shows starkly the performance cost of WebAssembly:
 * With 32k sprites, native builds still run dirt cheap, 3-4% CPU, but wasm eats like 20% CPU.
 * At 1k, they both deliver stellar performance, so nothing to really worry about i think.
 */

#include <egg/egg.h>
#include "menu.h"
#include "util/font.h"
#include "util/tile_renderer.h"
#include "stdlib/egg-stdlib.h"

extern int texid_misc;

/* Object definition.
 */
 
struct sprite {
  double x,y; // pixels
  double dx,dy; // px/s
  uint8_t tileid;
  uint8_t xform;
};

struct menu_sprites {
  struct menu hdr;
  struct sprite *spritev;
  int spritec,spritea;
  struct egg_draw_tile *tilev;
  int tilea;
};

#define MENU ((struct menu_sprites*)menu)

/* Cleanup.
 */
 
static void _sprites_del(struct menu *menu) {
  if (MENU->spritev) free(MENU->spritev);
  if (MENU->tilev) free(MENU->tilev);
}

/* Add or remove sprites.
 */
 
static void sprites_set_count(struct menu *menu,int nc) {
  if ((nc<1)||(nc>32768)) return;
  if (MENU->spritec>=nc) {
    MENU->spritec=nc;
    return;
  }
  if (nc>MENU->spritea) {
    void *nv=realloc(MENU->spritev,sizeof(struct sprite)*nc);
    if (!nv) return;
    MENU->spritev=nv;
    MENU->spritea=nc;
  }
  while (MENU->spritec<nc) {
    struct sprite *sprite=MENU->spritev+MENU->spritec++;
    sprite->x=(double)(rand()%menu->screenw);
    sprite->y=(double)(rand()%menu->screenh);
    sprite->dx=((double)((rand()&0x7fff)-16374))/100.0;
    sprite->dy=((double)((rand()&0x7fff)-16384))/100.0;
    switch (rand()&3) {
      case 0: sprite->tileid=0x00; break;
      case 1: sprite->tileid=0x01; break;
      case 2: sprite->tileid=0x14; break;
      case 3: sprite->tileid=0x16; break;
    }
    sprite->xform=rand()&7;
  }
}

/* Event.
 */
 
static void _sprites_event(struct menu *menu,const union egg_event *event) {
  switch (event->type) {
    case EGG_EVENT_JOY: if (event->joy.value) switch (event->joy.btnid) {
        case EGG_JOYBTN_LEFT: sprites_set_count(menu,MENU->spritec>>1); break;
        case EGG_JOYBTN_RIGHT: sprites_set_count(menu,MENU->spritec<<1); break;
      } break;
    case EGG_EVENT_KEY: if (event->key.value) switch (event->key.keycode) {
        case 0x0007004f: sprites_set_count(menu,MENU->spritec<<1); break;
        case 0x00070050: sprites_set_count(menu,MENU->spritec>>1); break;
      } break;
  }
}

/* Update.
 */
 
static void _sprites_update(struct menu *menu,double elapsed) {
  struct sprite *sprite=MENU->spritev;
  int i=MENU->spritec;
  for (;i-->0;sprite++) {
    sprite->x+=sprite->dx*elapsed;
    sprite->y+=sprite->dy*elapsed;
    if (((sprite->x<0.0)&&(sprite->dx<0.0))||((sprite->x>320.0)&&(sprite->dx>0.0))) sprite->dx=-sprite->dx;
    if (((sprite->y<0.0)&&(sprite->dy<0.0))||((sprite->y>180.0)&&(sprite->dy>0.0))) sprite->dy=-sprite->dy;
  }
}

/* Render.
 */
 
static void _sprites_render(struct menu *menu) {
  if (MENU->spritec>MENU->tilea) {
    void *nv=realloc(MENU->tilev,sizeof(struct egg_draw_tile)*MENU->spritec);
    if (!nv) return;
    MENU->tilev=nv;
    MENU->tilea=MENU->spritec;
  }
  struct sprite *sprite=MENU->spritev;
  struct egg_draw_tile *tile=MENU->tilev;
  int i=MENU->spritec;
  for (;i-->0;sprite++,tile++) {
    tile->x=(int)sprite->x;
    tile->y=(int)sprite->y;
    tile->tileid=sprite->tileid;
    tile->xform=sprite->xform;
  }
  egg_draw_tile(1,texid_misc,MENU->tilev,MENU->spritec);
  
  char tmp[5];
  if (MENU->spritec<10000) tmp[0]=' '; else tmp[0]='0'+(MENU->spritec/10000)%10;
  if (MENU->spritec<1000) tmp[1]=' '; else tmp[1]='0'+(MENU->spritec/1000)%10;
  if (MENU->spritec<100) tmp[2]=' '; else tmp[2]='0'+(MENU->spritec/100)%10;
  if (MENU->spritec<10) tmp[3]=' '; else tmp[3]='0'+(MENU->spritec/10)%10;
  tmp[4]='0'+MENU->spritec%10;
  tile_renderer_begin(menu->tile_renderer,menu->tilesheet,0xffffffff,0xc0);
  tile_renderer_string(menu->tile_renderer,8,menu->screenh-8,tmp,5);
  tile_renderer_end(menu->tile_renderer);
}

/* New.
 */
 
struct menu *menu_new_sprites(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu_sprites),parent);
  if (!menu) return 0;
  menu->del=_sprites_del;
  menu->event=_sprites_event;
  menu->update=_sprites_update;
  menu->render=_sprites_render;
  srand_auto();
  sprites_set_count(menu,32);
  return menu;
}
