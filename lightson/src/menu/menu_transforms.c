#include <egg/egg.h>
#include "menu.h"
#include "util/font.h"
#include "util/tile_renderer.h"
#include "stdlib/egg-stdlib.h"

extern int texid_misc;

/* Render.
 */
 
static void _transforms_render(struct menu *menu) {

  { // First a reference row, using tiles pre-transformed.
    struct egg_draw_tile vtxv[]={
      { 20,45,0x16,0},
      { 40,45,0x17,0},
      { 60,45,0x18,0},
      { 80,45,0x19,0},
      {100,45,0x1a,0},
      {120,45,0x1b,0},
      {140,45,0x1c,0},
      {160,45,0x1d,0},
    };
    egg_draw_tile(1,texid_misc,vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  }

  { // Exactly the same thing, but using only one tileid.
    struct egg_draw_tile vtxv[]={
      { 20,75,0x16,0},
      { 40,75,0x16,EGG_XFORM_XREV},
      { 60,75,0x16,EGG_XFORM_YREV},
      { 80,75,0x16,EGG_XFORM_XREV|EGG_XFORM_YREV},
      {100,75,0x16,EGG_XFORM_SWAP},
      {120,75,0x16,EGG_XFORM_SWAP|EGG_XFORM_XREV},
      {140,75,0x16,EGG_XFORM_SWAP|EGG_XFORM_YREV},
      {160,75,0x16,EGG_XFORM_SWAP|EGG_XFORM_XREV|EGG_XFORM_YREV},
    };
    egg_draw_tile(1,texid_misc,vtxv,sizeof(vtxv)/sizeof(vtxv[0]));
  }
  
  { // Same thing but use the decal shader.
    egg_draw_decal(1,texid_misc,12,97,96,16,16,16,0);
    egg_draw_decal(1,texid_misc,32,97,96,16,16,16,EGG_XFORM_XREV);
    egg_draw_decal(1,texid_misc,52,97,96,16,16,16,EGG_XFORM_YREV);
    egg_draw_decal(1,texid_misc,72,97,96,16,16,16,EGG_XFORM_XREV|EGG_XFORM_YREV);
    egg_draw_decal(1,texid_misc,92,97,96,16,16,16,EGG_XFORM_SWAP);
    egg_draw_decal(1,texid_misc,112,97,96,16,16,16,EGG_XFORM_SWAP|EGG_XFORM_XREV);
    egg_draw_decal(1,texid_misc,132,97,96,16,16,16,EGG_XFORM_SWAP|EGG_XFORM_YREV);
    egg_draw_decal(1,texid_misc,152,97,96,16,16,16,EGG_XFORM_SWAP|EGG_XFORM_XREV|EGG_XFORM_YREV);
  }
  
  { // Simulate the same transforms, using mode7 decals.
    egg_draw_decal_mode7(1,texid_misc,20,135,96,16,16,16,0x00000000,0x00010000,0x00010000);
    egg_draw_decal_mode7(1,texid_misc,40,135,96,16,16,16,0x00000000,0xffff0000,0x00010000);
    egg_draw_decal_mode7(1,texid_misc,60,135,96,16,16,16,0x00000000,0x00010000,0xffff0000);
    egg_draw_decal_mode7(1,texid_misc,80,135,96,16,16,16,0x00000000,0xffff0000,0xffff0000);
    egg_draw_decal_mode7(1,texid_misc,100,135,96,16,16,16,0x00019220,0x00010000,0xffff0000); // 0x0001.9220 = PI/2
    egg_draw_decal_mode7(1,texid_misc,120,135,96,16,16,16,0x00019220,0xffff0000,0xffff0000);
    egg_draw_decal_mode7(1,texid_misc,140,135,96,16,16,16,0x00019220,0x00010000,0x00010000);
    egg_draw_decal_mode7(1,texid_misc,160,135,96,16,16,16,0x00019220,0xffff0000,0x00010000);
  }

  // Label for each row.
  tile_renderer_begin(menu->tile_renderer,menu->tilesheet,0xffffffff,0xff);
  tile_renderer_string(menu->tile_renderer,200,45,"Reference",-1);
  tile_renderer_string(menu->tile_renderer,200,75,"Tile",-1);
  tile_renderer_string(menu->tile_renderer,200,105,"Decal",-1);
  tile_renderer_string(menu->tile_renderer,200,135,"Mode7",-1);
  tile_renderer_end(menu->tile_renderer);
}

/* New.
 */
 
struct menu *menu_new_transforms(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu),parent);
  if (!menu) return 0;
  menu->render=_transforms_render;
  return menu;
}
