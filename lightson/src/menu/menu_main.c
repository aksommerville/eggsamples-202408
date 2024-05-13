#include <egg/egg.h>
#include "menu.h"
#include "stdlib/egg-stdlib.h"
#include "util/font.h"
#include "util/text.h"

static void _main_option(struct menu *menu) {
  switch (menu->optionp) {
    case 0: menu_new_input(menu); break;
    case 1: menu_new_video(menu); break;
    case 2: menu_new_audio(menu); break;
    case 3: menu_new_store(menu); break;
    case 4: menu_new_misc(menu); break;
  }
}
 
struct menu *menu_new_main() {
  struct menu *menu=menu_new(sizeof(struct menu),0);
  if (!menu) return 0;
  menu->event=menu_option_event;
  menu->update=menu_option_update;
  menu->render=menu_option_render;
  menu->on_option=_main_option;
  
  if (
    (menu_option_add(menu,"Input",5)<0)||
    (menu_option_add(menu,"Video",5)<0)||
    (menu_option_add(menu,"Audio",5)<0)||
    (menu_option_add(menu,"Store",5)<0)||
    (menu_option_add(menu,"Misc",4)<0)||
  0) {
    menu_del(menu);
    return 0;
  }
  
  return menu;
}
