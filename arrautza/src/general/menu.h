/* menu.h
 * Generic modal, rendered on top of the scene.
 * For out-of-game menus, and also dialogue or cutscenes.
 * Multiple menus may exist in a stack, but only one should be active at a time.
 */
 
#ifndef MENU_H
#define MENU_H

// Size of global stack.
#define MENU_LIMIT 8

// Menus have a fixed size, so we provide some loose storage space in each.
#define MENU_IV_SIZE 8
#define MENU_FV_SIZE 8

#define MENU_ID_HELLO 1
#define MENU_ID_PAUSE 2
#define MENU_ID_DIALOGUE 3

struct menu {

  int id; // MENU_ID_*
  int opaque; // If nonzero, main won't bother rendering anything below this.
  int defunct;
  
  /* Populate these hooks to get callbacks from main.c.
   * During update, the global (g.instate) has the latest input state.
   */
  void (*del)(struct menu *menu);
  void (*update_bg)(struct menu *menu,double elapsed);
  void (*update)(struct menu *menu,double elapsed);
  void (*render)(struct menu *menu);
};

// Clients should never need this; all live menus are owned by (g.menuv).
void menu_del(struct menu *menu);

/* Validate stack and put a new empty menu there.
 * This should only be called from the typed pushers, see below.
 * The returned reference is WEAK; (g.menuv) owns it.
 */
struct menu *menu_push(int objlen);

/* If (menu) is in the stack, delete and remove it, and return >=0.
 * Pass null to pop whatever's on top.
 * Prefer the "_soon" version -- it only flags the menu as defunct, and they'll be reaped after the stack drains.
 */
int menu_pop(struct menu *menu);
int menu_pop_soon(struct menu *menu);

void reap_defunct_menus();

/* Typed pushers, the normal way to enter a menu.
 */
struct menu *menu_push_hello();
struct menu *menu_push_pause();
struct menu *menu_push_dialogue(int stringid);

#endif
