#include <egg/egg.h>
#include "menu.h"
#include "stdlib/egg-stdlib.h"

static void _audio_option(struct menu *menu) {
  switch (menu->optionp) {
  }
}
 
struct menu *menu_new_audio(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu),parent);
  if (!menu) return 0;
  menu->event=menu_option_event;
  menu->update=menu_option_update;
  menu->render=menu_option_render;
  menu->on_option=_audio_option;
  
  if (
    (menu_option_add(menu,"La la la",-1)<0)||
    (menu_option_add(menu,"La la la la la",-1)<0)||
  0) {
    menu_del(menu);
    return 0;
  }
  
  return menu;
}
