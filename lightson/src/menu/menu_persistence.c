#include <egg/egg.h>
#include "menu.h"
#include "util/tile_renderer.h"
#include "util/font.h"
#include "stdlib/egg-stdlib.h"

static void _persistence_option(struct menu *menu) {
  switch (menu->optionp) {

    case 0: { // Read
        egg_log("Reading 'myStoredData' from storage...");
        char v[32];
        int vc=egg_store_get(v,sizeof(v),"myStoredData",12);
        if (vc<1) egg_log("...empty");
        else if (vc>sizeof(v)) egg_log("...too big! %d>%d",vc,(int)sizeof(v));
        else egg_log("...'%.*s'",vc,v);
      } break;

    case 1: { // Write
        int date[7]={0};
        egg_time_local(date,7);
        char v[]={
          '0'+(date[0]/1000)%10,
          '0'+(date[0]/ 100)%10,
          '0'+(date[0]/  10)%10,
          '0'+(date[0]     )%10,
          '-',
          '0'+(date[1]/  10)%10,
          '0'+(date[1]     )%10,
          '-',
          '0'+(date[2]/  10)%10,
          '0'+(date[2]     )%10,
          'T',
          '0'+(date[3]/  10)%10,
          '0'+(date[3]     )%10,
          ':',
          '0'+(date[4]/  10)%10,
          '0'+(date[4]     )%10,
          ':',
          '0'+(date[5]/  10)%10,
          '0'+(date[5]     )%10,
          '.',
          '0'+(date[6]/ 100)%10,
          '0'+(date[6]/  10)%10,
          '0'+(date[6]     )%10,
        };
        egg_log("Writing storage 'myStoredData' = '%.*s'",(int)sizeof(v),v);
        int err=egg_store_set("myStoredData",12,v,sizeof(v));
        if (err>=0) egg_log("...success!");
        else egg_log("...FAILURE!");
      } break;

    case 2: { // List
        egg_log("Listing storage keys...");
        char k[256];
        int kc,p=0;
        for (;;p++) {
          if ((kc=egg_store_key_by_index(k,sizeof(k),p))<1) break;
          egg_log("  '%.*s'",kc,k);
        }
        egg_log("...end of list");
      } break;
  }
}

static void _persistence_render(struct menu *menu) {
  menu_option_render(menu);
  tile_renderer_begin(menu->tile_renderer,menu->tilesheet,0x808080ff,0xff);
  tile_renderer_string(menu->tile_renderer,5,menu->screenh-5,"Watch log for results.",-1);
  tile_renderer_end(menu->tile_renderer);
}
 
struct menu *menu_new_persistence(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu),parent);
  if (!menu) return 0;
  menu->event=menu_option_event;
  menu->update=menu_option_update;
  menu->render=_persistence_render;
  menu->on_option=_persistence_option;
  
  menu_option_add(menu,"Read",4);
  menu_option_add(menu,"Write",5);
  menu_option_add(menu,"List",4);
  
  return menu;
}
