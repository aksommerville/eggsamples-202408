/* verblist.h
 * Manages the list of verbs in the upper right of main screen.
 */

#ifndef VERBLIST_H
#define VERBLIST_H

void verblist_init();
void verblist_render(int x,int y,int w,int h);
void verblist_press(int x,int y);
void verblist_unselect();
void verblist_set(int verb);
int verblist_get();
void verblist_refresh();

#endif
