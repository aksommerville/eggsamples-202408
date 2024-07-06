#include "arrautza.h"

struct menu_hello {
  struct menu hdr;
  int pvinput;
  int texid;
  int logow,logoh;
  double logo_progress;
  int font_tilesize;
  int cursorx,cursory;
  int optionp;
  struct { int x,y; } xy_newgame,xy_continue,xy_quit;
};

#define MENU ((struct menu_hello*)menu)

// How long for the logo to advance to its resting position, in inverse seconds.
#define LOGO_PROGRESS_SPEED 0.25

/* Cleanup.
 */
 
static void _hello_del(struct menu *menu) {
  egg_texture_del(MENU->texid);
}

/* New game.
 */
 
static void hello_begin_game(struct menu *menu) {
  menu_pop_soon(menu);
  sprgrp_kill(sprgrpv+SPRGRP_HERO);
  g.mapid=0;
  if ((load_map(RID_map_start,-1,-1,TRANSITION_NONE)<0)||(check_map_change()<0)) {
    egg_log("Failed to load initial map.");
    egg_request_termination();
  }
}

/* Continue.
 */
 
static void hello_continue_game(struct menu *menu) {
  //TODO Locate save file.
}

/* Activate.
 */
 
static void hello_activate(struct menu *menu) {
  if (MENU->logo_progress<1.0) {
    MENU->logo_progress=1.0;
  } else switch (MENU->optionp) {
    case 0: hello_begin_game(menu); break;
    case 1: hello_continue_game(menu); break;
    case 2: egg_request_termination(); break;
  }
}

/* Motion.
 */
 
static void hello_motion(struct menu *menu,int dx,int dy) {
  if (MENU->logo_progress<1.0) return;
  if (dy) {
    MENU->optionp+=dy;
    if (MENU->optionp<0) MENU->optionp=2;
    else if (MENU->optionp>=3) MENU->optionp=0;
    //TODO sound effect
    switch (MENU->optionp) {
      case 0: MENU->cursory=MENU->xy_newgame.y-8; break;
      case 1: MENU->cursory=MENU->xy_continue.y-8; break;
      case 2: MENU->cursory=MENU->xy_quit.y-8; break;
    }
  }
}

/* Update.
 */
 
static void _hello_update(struct menu *menu,double elapsed) {
  if (MENU->logo_progress<1.0) {
    MENU->logo_progress+=elapsed*LOGO_PROGRESS_SPEED;
  }
  if (g.instate!=MENU->pvinput) {
    #define PRESS(tag) ((g.instate&INKEEP_BTNID_##tag)&&!(MENU->pvinput&INKEEP_BTNID_##tag))
    if (PRESS(SOUTH)) hello_activate(menu);
    if (PRESS(AUX1)) hello_activate(menu);
    if (PRESS(LEFT)) hello_motion(menu,-1,0);
    if (PRESS(RIGHT)) hello_motion(menu,1,0);
    if (PRESS(UP)) hello_motion(menu,0,-1);
    if (PRESS(DOWN)) hello_motion(menu,0,1);
    #undef PRESS
    MENU->pvinput=g.instate;
  }
}

/* Render.
 */
 
static void _hello_render(struct menu *menu) {
  egg_draw_rect(1,0,0,SCREENW,SCREENH,0x5b1919ff);
  
  int dstx=(SCREENW>>1)-(MENU->logow>>1);
  int dsty=0;
  if (MENU->logo_progress<1.0) {
    dsty=SCREENH+(int)((dsty-SCREENH)*MENU->logo_progress);
  }
  egg_draw_decal(1,MENU->texid,dstx,dsty,0,0,MENU->logow,MENU->logoh,0);
  
  if (MENU->logo_progress>=1.0) {
    const char *src;
    int srcc;
    dstx=SCREENW/3;
    dsty=MENU->logoh+16;
    tile_renderer_begin(&g.tile_renderer,g.texid_font_tiles,0xffffffff,0xff);
    #define ADDSTR(tag) { \
      srcc=text_get_string(&src,RID_string_hello_##tag); \
      tile_renderer_string(&g.tile_renderer,MENU->xy_##tag.x,MENU->xy_##tag.y,src,srcc); \
    }
    ADDSTR(newgame)
    ADDSTR(continue)
    ADDSTR(quit)
    #undef ADDSTR
    tile_renderer_end(&g.tile_renderer);
    
    tile_renderer_begin(&g.tile_renderer,g.texid_font_tiles,0xffffffff,0x80);
    srcc=text_get_string(&src,RID_string_hello_fineprint);
    tile_renderer_string(&g.tile_renderer,MENU->font_tilesize,SCREENH-MENU->font_tilesize,src,srcc);
    tile_renderer_end(&g.tile_renderer);
    
    int texid=texcache_get(&g.texcache,RID_image_hero);
    egg_draw_decal(1,texid,MENU->cursorx,MENU->cursory,0,128,32,16,0);
    egg_draw_decal(1,texid,SCREENW-MENU->cursorx-32,MENU->cursory,0,128,32,16,EGG_XFORM_XREV);
  }
}

/* New.
 */
 
struct menu *menu_push_hello() {
  struct menu *menu=menu_push(sizeof(struct menu_hello));
  if (!menu) return 0;
  
  menu->del=_hello_del;
  menu->update=_hello_update;
  menu->render=_hello_render;
  menu->id=MENU_ID_HELLO;
  menu->opaque=1;
  MENU->pvinput=g.instate;
  
  if (egg_texture_load_image(MENU->texid=egg_texture_new(),0,RID_image_logo)<0) {
    menu_pop(menu);
    return 0;
  }
  egg_texture_get_header(&MENU->logow,&MENU->logoh,0,MENU->texid);
  
  int fontw=0,fonth=0;
  egg_texture_get_header(&fontw,&fonth,0,g.texid_font_tiles);
  if ((MENU->font_tilesize=fontw>>4)<1) MENU->font_tilesize=1;
  
  /* Calculate layout.
   * Logo is centered at the very top.
   * Fineprint is at the bottom, with a half-tile margin above and below.
   * The three option lines should be centered horizontally.
   * They'll center vertically in the available space as a block.
   * Space between options is fixed at one half tile.
   */
  int availh=SCREENH-MENU->logoh-(MENU->font_tilesize<<1);
  int optionsh=MENU->font_tilesize*3+MENU->font_tilesize; // 2 spaces at a half-tile each
  int optionsy=MENU->logoh+(availh>>1)-(optionsh>>1);
  optionsy+=MENU->font_tilesize>>1;
  int ystep=MENU->font_tilesize+(MENU->font_tilesize>>1);
  MENU->xy_newgame.y=optionsy; optionsy+=ystep;
  MENU->xy_continue.y=optionsy; optionsy+=ystep;
  MENU->xy_quit.y=optionsy;
  int longest_option=0;
  #define SETX(tag) { \
    const char *src=0; \
    int srcc=text_get_string(&src,RID_string_hello_##tag); \
    MENU->xy_##tag.x=(SCREENW>>1)-((srcc*MENU->font_tilesize)>>1)+(MENU->font_tilesize>>1); \
    if (srcc>longest_option) longest_option=srcc; \
  }
  SETX(newgame)
  SETX(continue)
  SETX(quit)
  #undef SETX
  MENU->cursorx=(SCREENW>>1)-((longest_option*MENU->font_tilesize)>>1)-6-32;
  MENU->cursory=MENU->xy_newgame.y-8;
  
  egg_audio_play_song(0,RID_song_fanfare,0,1);
  
  return menu;
}
