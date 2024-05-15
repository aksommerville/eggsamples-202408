#include <egg/egg.h>
#include "menu.h"
#include "stdlib/egg-stdlib.h"

static void _store_option(struct menu *menu) {
  switch (menu->optionp) {
    case 0: menu_new_listres(menu); break;
    case 1: menu_new_strings(menu); break;
    case 2: menu_new_persistence(menu); break;
  }
}
 
struct menu *menu_new_store(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu),parent);
  if (!menu) return 0;
  menu->event=menu_option_event;
  menu->update=menu_option_update;
  menu->render=menu_option_render;
  menu->on_option=_store_option;
  
  menu_option_add(menu,"List Resources",14);
  menu_option_add(menu,"Strings by Language",19);
  menu_option_add(menu,"Persistence",11);
  
  return menu;
}
