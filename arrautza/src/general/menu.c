#include "arrautza.h"

/* Cleanup.
 */
 
void menu_del(struct menu *menu) {
  if (!menu) return;
  if (menu->del) menu->del(menu);
  free(menu);
}

/* Push empty menu.
 */
 
struct menu *menu_push(int objlen) {
  if (g.menuc>=MENU_LIMIT) return 0;
  if (objlen<(int)sizeof(struct menu)) return 0;
  struct menu *menu=calloc(1,objlen);
  if (!menu) return 0;
  g.menuv[g.menuc++]=menu;
  return menu;
}

/* Pop menu.
 */

int menu_pop(struct menu *menu) {
  if (g.menuc<1) return -1;
  int p;
  if (!menu) {
    p=g.menuc-1;
    menu=g.menuv[p];
  } else {
    p=-1;
    int i=g.menuc; while (i-->0) {
      if (g.menuv[i]==menu) {
        p=i;
        break;
      }
    }
    if (p<0) return -1;
  }
  g.menuc--;
  memmove(g.menuv+p,g.menuv+p+1,sizeof(void*)*(g.menuc-p));
  menu_del(menu);
  return 0;
}

/* Mark menu defunct.
 */
 
int menu_pop_soon(struct menu *menu) {
  // Confirm it's in the stack, and resolve null to the top of the stack.
  int p=-1;
  if (!menu) {
    if (g.menuc<1) return -1;
    p=g.menuc-1;
    menu=g.menuv[p];
  } else {
    int i=g.menuc; while (i-->0) {
      if (g.menuv[i]==menu) {
        p=i;
        break;
      }
    }
  }
  if (p<0) return -1;
  menu->defunct=1;
  return 0;
}

/* Drop any menus marked defunct.
 */
 
void reap_defunct_menus() {
  int i=g.menuc;
  while (i-->0) {
    struct menu *menu=g.menuv[i];
    if (!menu->defunct) continue;
    menu_pop(menu);
  }
}

/* If a menu of the given ID is open, return it.
 */
 
struct menu *get_open_menu_by_id(int id) {
  int i=g.menuc;
  while (i-->0) {
    struct menu *menu=g.menuv[i];
    if (menu->id==id) return menu;
  }
  return 0;
}
