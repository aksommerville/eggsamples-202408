#include <egg/egg.h>
#include "menu.h"
#include "stdlib/egg-stdlib.h"

static void _input_option(struct menu *menu) {
  switch (menu->optionp) {
    case 0: menu_new_joystick(menu); break;
    case 1: menu_new_raw(menu); break;
    case 2: menu_new_keyboard(menu); break;
    case 3: menu_new_mouse(menu); break;
    case 4: menu_new_mouselock(menu); break;
    case 5: menu_new_touch(menu); break;
    case 6: menu_new_accelerometer(menu); break;
  }
}
 
struct menu *menu_new_input(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu),parent);
  if (!menu) return 0;
  menu->event=menu_option_event;
  menu->update=menu_option_update;
  menu->render=menu_option_render;
  menu->on_option=_input_option;
  
  if (
    (menu_option_add(menu,"Joystick",8)<0)||
    (menu_option_add(menu,"Raw Joystick",12)<0)||
    (menu_option_add(menu,"Keyboard",8)<0)||
    (menu_option_add(menu,"Mouse",5)<0)||
    (menu_option_add(menu,"Mouse Lock",10)<0)||
    (menu_option_add(menu,"Touch",5)<0)||
    (menu_option_add(menu,"Accelerometer",13)<0)||
  0) {
    menu_del(menu);
    return 0;
  }
  
  return menu;
}
