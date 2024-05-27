/* popup.h
 * Our UI system is not as generic as it looks.
 * There can be one popup at a time, with text and two buttons.
 */
 
#ifndef POPUP_H
#define POPUP_H

void popup_dismiss();

/* If a popup is already presented, it quietly cancels.
 * (prompt,ok,cancel) are string IDs. Zero to not display a button.
 */
void popup_present(
  int prompt,int ok,int cancel,
  void (*on_ok)(),
  void (*on_cancel)()
);

/* While popped, main.c should suspend all other interactivity.
 * It should render as usual.
 * Render can be called whether popped or not, we quickly noop if not.
 */
int popup_is_popped();
void popup_touch(int x,int y);
void popup_render();

#endif
