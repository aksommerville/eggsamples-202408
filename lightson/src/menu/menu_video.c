#include <egg/egg.h>
#include "menu.h"
#include "stdlib/egg-stdlib.h"

static void _video_option(struct menu *menu) {
  switch (menu->optionp) {
    case 0: menu_new_primitives(menu); break;
    case 1: menu_new_transforms(menu); break;
    case 2: menu_new_sprites(menu); break;
    case 3: menu_new_framebuffer(menu); break;
  }
}
 
struct menu *menu_new_video(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu),parent);
  if (!menu) return 0;
  menu->event=menu_option_event;
  menu->update=menu_option_update;
  menu->render=menu_option_render;
  menu->on_option=_video_option;
  
  menu_option_add(menu,"Primitives",-1);
  menu_option_add(menu,"Transforms",-1);
  menu_option_add(menu,"Too Many Sprites",-1);
  menu_option_add(menu,"Intermediate Framebuffer",-1);
  
  return menu;
}
