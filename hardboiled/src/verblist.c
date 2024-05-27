#include "hardboiled.h"
#include "verblist.h"

/* Globals.
 */
 
#define STRINGID_FIRST 3 /* for verb 1, "Go" */
 
static int texid_buttons=0;
static int texid_labels[VERB_COUNT]={0};
int verb_selected=VERB_TALK;
static int enabled[VERB_COUNT]={0};

/* Init label.
 */
 
static int verblist_init_label(int p) {
  const char *src=0;
  int srcc=text_get_string(&src,STRINGID_FIRST+p);
  if (srcc<1) return -1;
  if ((texid_labels[p]=font_render_new_texture(font,src,srcc,0,0x000000))<1) return -1;
  return 0;
}
 
/* Init.
 */
 
void verblist_init() {
  egg_texture_load_image(texid_buttons=egg_texture_new(),0,RID_image_button);
  int i=0; for (;i<VERB_COUNT;i++) {
    verblist_init_label(i);
    enabled[i]=1;
  }
}

/* Render.
 */
 
void verblist_render(int x,int y,int w,int h) {
  int i=0,dsty=y+4;
  for (;i<VERB_COUNT;i++,dsty+=16) {
    int selected=(i+1==verb_selected);
    
    int srcy=0;
    if (selected) srcy=15;
    else if (!enabled[i]) srcy=30;
    egg_draw_decal(1,texid_buttons,x,dsty,0,srcy,69,15,0);
    
    int lblw=0,lblh=0;
    egg_texture_get_header(&lblw,&lblh,0,texid_labels[i]);
    int lblx=x+(w>>1)-(lblw>>1);
    int lbly=dsty+8-(lblh>>1);
    if (selected) egg_render_tint(0xffffffff);
    else if (!enabled[i]) egg_render_tint(0x404040ff);
    egg_draw_decal(1,texid_labels[i],lblx,lbly,0,0,lblw,lblh,0);
    if (selected||!enabled[i]) egg_render_tint(0);
  }
}

/* Press.
 */
 
void verblist_press(int x,int y) {
  selected_item=0;
  int verb=(y-4)/16+1;
  if ((verb<1)||(verb>VERB_COUNT)) return;
  if (verb==verb_selected) {
    verb_selected=0;
  } else {
    verb_selected=verb;
    if (verb==VERB_GIVE) inventory_show_items();
  }
}

void verblist_unselect() {
  verb_selected=0;
  selected_item=0;
}

void verblist_set(int verb) {
  verb_selected=verb;
}

int verblist_get() {
  return verb_selected;
}

/* Reassess enablement after room change.
 */
 
void verblist_refresh() {
  int i=VERB_COUNT;
  while (i-->0) {
    enabled[i]=1;
  }
  if (!room_get_exit(current_room)) enabled[VERB_EXIT-1]=0;
}
