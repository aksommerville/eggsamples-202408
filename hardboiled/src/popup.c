#include "hardboiled.h"

#define MARGIN 4
#define WMAX ((SCREENW*3)/4)
#define WMIN 50

/* Globals.
 */
 
static struct {
  int visible;
  int dstx,dsty,dstw,dsth;
  int prompttexid;
  int promptw,prompth;
  int oktexid;
  int okx,oky,okw,okh;
  int canceltexid;
  int cancelx,cancely,cancelw,cancelh;
  void (*on_ok)();
  void (*on_cancel)();
} popup={0};

/* Dismiss.
 */
 
void popup_dismiss() {
  popup.visible=0;
  if (popup.on_cancel) {
    void (*cb)()=popup.on_cancel;
    popup.on_cancel=0;
    cb();
  }
}

/* Present.
 */

void popup_present(
  int prompt,int ok,int cancel,
  void (*on_ok)(),
  void (*on_cancel)()
) {
  popup.on_ok=on_ok;
  popup.on_cancel=on_cancel;
  const char *promptstr=0,*okstr=0,*cancelstr=0;
  int promptc=text_get_string(&promptstr,prompt);
  int okc=text_get_string(&okstr,ok);
  int cancelc=text_get_string(&cancelstr,cancel);
  popup.visible=1;
  
  if (okc) {
    if (!popup.oktexid) {
      popup.oktexid=egg_texture_new();
    }
    font_render_to_texture(popup.oktexid,font,okstr,okc,0,0x0000ff);
    egg_texture_get_header(&popup.okw,&popup.okh,0,popup.oktexid);
  } else {
    popup.okw=0;
    popup.okh=0;
  }
  
  if (cancelc) {
    if (!popup.canceltexid) {
      popup.canceltexid=egg_texture_new();
    }
    font_render_to_texture(popup.canceltexid,font,cancelstr,cancelc,0,0x0000ff);
    egg_texture_get_header(&popup.cancelw,&popup.cancelh,0,popup.canceltexid);
  } else {
    popup.cancelw=0;
    popup.cancelh=0;
  }
  
  if (!popup.prompttexid) {
    popup.prompttexid=egg_texture_new();
  }
  font_render_to_texture(popup.prompttexid,font,promptstr,promptc,WMAX,0x000000);
  egg_texture_get_header(&popup.promptw,&popup.prompth,0,popup.prompttexid);
  
  popup.dstw=popup.okw+popup.cancelw+MARGIN;
  if (popup.dstw<popup.promptw) popup.dstw=popup.promptw;
  if (popup.dstw<WMIN) popup.dstw=WMIN;
  popup.dstw+=MARGIN<<1;
  popup.dsth=popup.prompth;
  int bottomh=(popup.okh>popup.cancelh)?popup.okh:popup.cancelh;
  if (bottomh) bottomh+=MARGIN;
  popup.dsth+=bottomh;
  popup.dsth+=MARGIN<<1;
  popup.dstx=(SCREENW>>1)-(popup.dstw>>1);
  popup.dsty=(SCREENH>>1)-(popup.dsth>>1);
  popup.oky=popup.dsty+popup.dsth-MARGIN-popup.okh+3; // +3: Our font is biased a little high.
  popup.cancely=popup.dsty+popup.dsth-MARGIN-popup.cancelh+3;
  if (popup.okw&&popup.cancelw) {
    int labelsw=popup.okw+popup.cancelw;
    int excess=popup.dstw-labelsw;
    int exeach=excess/3;
    popup.okx=popup.dstx+exeach;
    popup.cancelx=popup.okx+popup.okw+exeach;
  } else if (popup.okw) {
    popup.okx=popup.dstx+(popup.dstw>>1)-(popup.okw>>1);
  } else if (popup.cancelw) {
    popup.cancelx=popup.dstx+(popup.dstw>>1)-(popup.cancelw>>1);
  }
}

/* Trivial accessors.
 */

int popup_is_popped() {
  return popup.visible;
}

/* Trigger completion.
 */
 
static void popup_ok() {
  popup.visible=0;
  if (popup.on_ok) {
    void (*cb)()=popup.on_ok;
    popup.on_ok=0;
    cb();
  }
}

static void popup_cancel() {
  popup.visible=0;
  if (popup.on_cancel) {
    void (*cb)()=popup.on_cancel;
    popup.on_cancel=0;
    cb();
  }
}

/* Touch.
 */
 
void popup_touch(int x,int y) {
  if ((x<popup.dstx)||(y<popup.dsty)||(x>=popup.dstx+popup.dstw)||(y>=popup.dsty+popup.dsth)) {
    popup_cancel();
  } else if (popup.okh||popup.cancelh) {
    int bottomy=popup.dsty+popup.dsth-((popup.okh>popup.cancelh)?popup.okh:popup.cancelh)-(MARGIN<<1);
    if (y>=bottomy) {
      if (!popup.okh) popup_cancel();
      else if (!popup.cancelh) popup_ok();
      else if (x<popup.cancelx-MARGIN) popup_ok();
      else popup_cancel();
    }
  }
}

/* Render.
 */
 
void popup_render() {
  if (!popup.visible) return;
  egg_draw_rect(1,0,0,SCREENW,SCREENH,0x00000080);
  egg_draw_rect(1,popup.dstx+2,popup.dsty+2,popup.dstw,popup.dsth,0x000000c0);
  egg_draw_rect(1,popup.dstx,popup.dsty,popup.dstw,popup.dsth,0xffffffff);
  if (popup.prompttexid) {
    egg_draw_decal(1,popup.prompttexid,popup.dstx+MARGIN,popup.dsty+MARGIN,0,0,popup.promptw,popup.prompth,0);
  }
  if (popup.okh) {
    egg_draw_decal(1,popup.oktexid,popup.okx,popup.oky,0,0,popup.okw,popup.okh,0);
  }
  if (popup.cancelh) {
    egg_draw_decal(1,popup.canceltexid,popup.cancelx,popup.cancely,0,0,popup.cancelw,popup.cancelh,0);
  }
}
