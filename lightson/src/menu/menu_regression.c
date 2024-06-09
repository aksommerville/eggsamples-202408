#include <egg/egg.h>
#include "menu.h"
#include "stdlib/egg-stdlib.h"
#include "util/font.h"
#include "util/text.h"

// Tests must return (-1,0,1)=(fail,indeterminate,pass).
// ("indeterminate" might mean user needs to review log or something).

/* 2024-06-09: Confirm that '%p' pops the right size off the stack.
 * In Wasm code, this must always be 4 bytes.
 **************************************************************/
 
static int regression_20240609_log_pct_p() {
  egg_log("This must print '123 ((some address)) 456':");
  egg_log("%d %p %d",123,__func__,456);
  egg_log("This must print '123 456 ((some address)) 789 111':");
  egg_log("%d %d %p %d %d",123,456,__func__,789,111);
  return 0;
}

/* TOC.
 **************************************************************/
 
#define FOR_EACH_TEST \
  _(20240609_log_pct_p)
 
#define HIGHLIGHT_TIME 1.0

struct menu_regression {
  struct menu hdr;
  double highlight_time;
  uint32_t highlight_color;
};

#define MENU ((struct menu_regression*)menu)

static void _regression_update(struct menu *menu,double elapsed) {
  menu_option_update(menu,elapsed);
  if ((MENU->highlight_time-=elapsed)<=0.0) {
    MENU->highlight_time=0.0;
    MENU->highlight_color=0;
  }
}

static void _regression_render(struct menu *menu) {
  if (MENU->highlight_time>0.0) {
    int a=(int)((MENU->highlight_time*0xff)/HIGHLIGHT_TIME);
    if (a<0) a=0; else if (a>0xff) a=0xff;
    MENU->highlight_color&=0xffffff00;
    MENU->highlight_color|=a;
    egg_draw_rect(1,0,0,1000,1000,MENU->highlight_color);
  }
  menu_option_render(menu);
}

static void _regression_option(struct menu *menu) {
  int (*fn)()=0;
  if (menu->optionp<0) return;
  int i=menu->optionp;
  #define _(tag) if (!i--) fn=regression_##tag;
  FOR_EACH_TEST
  #undef _
  if (!fn) return;
  int result=fn();
  if (result<0) MENU->highlight_color=0xff000000;
  else if (result>0) MENU->highlight_color=0x00800000;
  else MENU->highlight_color=0x80808000;
  MENU->highlight_time=HIGHLIGHT_TIME;
}
 
struct menu *menu_new_regression(struct menu *parent) {
  struct menu *menu=menu_new(sizeof(struct menu_regression),parent);
  if (!menu) return 0;
  menu->event=menu_option_event;
  menu->update=_regression_update;
  menu->render=_regression_render;
  menu->on_option=_regression_option;
  
  if (
    #define _(tag) (menu_option_add(menu,#tag,sizeof(#tag)-1)<0)||
    FOR_EACH_TEST
    #undef _
  0) {
    menu_del(menu);
    return 0;
  }
  
  return menu;
}
