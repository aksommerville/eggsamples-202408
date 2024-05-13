/* menu.h
 * Represents one screenful of modal content.
 */
 
#ifndef MENU_H
#define MENU_H

struct font;
union egg_event;
struct tile_renderer;

struct menu {

// Parent should set:
  struct font *font;
  int screenw,screenh;
  struct tile_renderer *tile_renderer;
  int tilesheet;
  
  struct menu *child;
  void (*del)(struct menu *menu);
  void (*event)(struct menu *menu,const union egg_event *event);
  void (*update)(struct menu *menu,double elapsed);
  void (*render)(struct menu *menu);
  
  // Options list, to present as rows of text.
  // This is entirely opt-in; menu controllers don't have to use it.
  char **optionv;
  int optionc,optiona;
  int optionp;
  int optiontex;
  void (*on_option)(struct menu *menu); // check menu->optionp
  void (*on_option_adjust)(struct menu *menu,int dx);
};

void menu_del(struct menu *menu);

/* You probably want one of the typed constructors below, not menu_new.
 */
struct menu *menu_new(int size,struct menu *parent);

/* These public hooks walk downstack automatically, so only the frontmost menu updates at any time.
 * App is expected to clear the framebuffer before calling out for render.
 */
void menu_event(struct menu *menu,const union egg_event *event);
void menu_update(struct menu *menu,double elapsed);
void menu_render(struct menu *menu);

struct menu *menu_new_main();
struct menu *menu_new_input(struct menu *parent);
struct menu *menu_new_video(struct menu *parent);
struct menu *menu_new_audio(struct menu *parent);
struct menu *menu_new_store(struct menu *parent);
struct menu *menu_new_misc(struct menu *parent);

int menu_option_add(struct menu *menu,const char *src,int srcc);
void menu_option_event(struct menu *menu,const union egg_event *event);
void menu_option_update(struct menu *menu,double elapsed);
void menu_option_render(struct menu *menu);

#endif
