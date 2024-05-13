#include <egg/egg.h>
#include "menu.h"
#include "stdlib/egg-stdlib.h"

static void _input_option(struct menu *menu) {
  switch (menu->optionp) {
    case 0: break; // Joystick
    case 1: break; // Raw Joystick
    case 2: break; // Keyboard
    case 3: break; // Mouse
    case 4: break; // Touch
    case 5: break; // Accelerometer
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
    (menu_option_add(menu,"Touch",5)<0)||
    (menu_option_add(menu,"Accelerometer",13)<0)||
  0) {
    menu_del(menu);
    return 0;
  }
  
  return menu;
}
