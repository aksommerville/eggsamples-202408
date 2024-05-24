#include "hardboiled.h"
#include "verblist.h"

#define NPX 3
#define NPY 3
#define NPW 67
#define NPH 98

#define INV_LIMIT 16
static int invv[INV_LIMIT]={0}; // stringid of item's name
static int invc=0;
static int inv_donev[INV_LIMIT]={0}; // stringid of item's name; they go here after giving away
static int inv_donec=0;
int inventory_sequence=0;
static int texid_tabs=0;
static int selected_tab=0; // 0,1 = Items,Clues
static int texid_overlay=0;
static int overlay_sequence=-1;
static void *overlay_tmp=0;
int selected_item=0;

/* Public accessors.
 */
 
int inv_get(int stringid) {
  const int *v=invv;
  int i=invc;
  for (;i-->0;v++) if (*v==stringid) return 1;
  for (v=inv_donev,i=inv_donec;i-->0;v++) if (*v==stringid) return 2;
  return 0;
}

static int inv_remove(int *v,int c,int stringid) {
  int i=c;
  while (i-->0) {
    if (v[i]==stringid) {
      c--;
      memmove(v+i,v+i+1,sizeof(int)*(c-i));
      inventory_sequence++;
      if (stringid==selected_item) selected_item=0;
      return c;
    }
  }
  return c;
}

static int inv_append(int *v,int c,int stringid) {
  int i=c;
  int *p=v;
  for (;i-->0;p++) {
    if (*p==stringid) return c;
  }
  if (c>=INV_LIMIT) return c;
  v[c++]=stringid;
  inventory_sequence++;
  return c;
}

void inv_set(int stringid,int has) {
  switch (has) {
    case 0: invc=inv_remove(invv,invc,stringid); inv_donec=inv_remove(inv_donev,inv_donec,stringid); return;
    case 1: invc=inv_append(invv,invc,stringid); inv_donec=inv_remove(inv_donev,inv_donec,stringid); return;
    case 2: invc=inv_remove(invv,invc,stringid); inv_donec=inv_append(inv_donev,inv_donec,stringid); return;
  }
}

/* Mouse event.
 */
 
static void inv_set_tab(int tabid) {
  if (tabid==selected_tab) return;
  selected_tab=tabid;
  inventory_sequence++;
}
 
void inventory_press(int x,int y) {
  if (y>=88) { // Click in tabs.
    selected_item=0;
    if (x<38) {
      inv_set_tab(0);
    } else {
      inv_set_tab(1);
    }
    return;
  }
  if (selected_tab==0) { // Click in items.
    selected_item=0;
    int itemp=(y-7)/10;
    if ((itemp<0)||(itemp>=invc)) return;
    if (verblist_get()==VERB_GIVE) {
      selected_item=invv[itemp];
    }
    return;
  }
}

/* Redraw overlay texture.
 */
 
static void inv_redraw_overlay() {
  if (!overlay_tmp) {
    if (!(overlay_tmp=malloc((NPW<<2)*NPH))) return;
  }
  memset(overlay_tmp,0,(NPW<<2)*NPH);
  
  if (selected_tab==0) {
    int i=0; for (;i<invc;i++) {
      const char *src=0;
      int srcc=text_get_string(&src,invv[i]);
      if (srcc<1) continue;
      font_render_string_rgba(
        overlay_tmp,NPW,NPH,NPW<<2,
        3,6+i*10,font,src,srcc,0x0000ff
      );
    }
    
  } else if (selected_tab==1) {
    //TODO Text for clues
    font_render_string_rgba(
      overlay_tmp,NPW,NPH,NPW<<2,
      3,6,font,"Text here.",10,0x0000ff
    );
  }
  
  font_render_string_rgba(
    overlay_tmp,NPW,NPH,NPW<<2,
    3,87,font,"Items",5,(selected_tab==0)?0x000000:0x444444
  );
  font_render_string_rgba(
    overlay_tmp,NPW,NPH,NPW<<2,
    35,87,font,"Clues",5,(selected_tab==1)?0x000000:0x444444
  );
  
  egg_texture_upload(texid_overlay,NPW,NPH,NPW<<2,EGG_TEX_FMT_RGBA,overlay_tmp,(NPW<<2)*NPH);
}

/* Render.
 * Our content goes in the top-left corner of the screen.
 * The background image of a notepad is already there.
 */
 
void inventory_render() {

  // Load tabs sources if we haven't.
  if (!texid_tabs) {
    if (egg_texture_load_image(texid_tabs=egg_texture_new(),0,RID_image_notepad)<0) {
      texid_tabs=0;
      return;
    }
  }

  // Draw tab backgrounds.
  egg_draw_decal(1,texid_tabs,2,87,0,(selected_tab==0)?13:0,34,13,0);
  egg_draw_decal(1,texid_tabs,35,87,0,(selected_tab==1)?13:0,34,13,EGG_XFORM_XREV);
  
  // Highlight selected item if we're on the Items tab and something is selected.
  if ((selected_tab==0)&&selected_item) {
    int i=0; for (;i<invc;i++) {
      if (invv[i]!=selected_item) continue;
      egg_draw_rect(1,NPX,NPY+5+i*10,NPW-2,9,0xe0a090ff);
      break;
    }
  }
  
  // Redraw overlay if needed.
  if (!texid_overlay) {
    if ((texid_overlay=egg_texture_new())<1) {
      texid_overlay=0;
      return;
    }
  }
  if (overlay_sequence!=inventory_sequence) {
    inv_redraw_overlay();
    overlay_sequence=inventory_sequence;
  }
  
  // Transfer overlay.
  egg_draw_decal(1,texid_overlay,NPX,NPY,0,0,NPW,NPH,0);
}
