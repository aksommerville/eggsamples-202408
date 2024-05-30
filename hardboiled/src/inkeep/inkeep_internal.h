#ifndef INKEEP_INTERNAL_H
#define INKEEP_INTERNAL_H

#include <egg/egg.h>
#include "stdlib/egg-stdlib.h"
#include "util/font.h"
#include "util/text.h"
#include "util/tile_renderer.h"
#include "inkeep.h"

#define INKEVTYPE_RAW 1
#define INKEVTYPE_JOY 2
#define INKEVTYPE_TEXT 3
#define INKEVTYPE_TOUCH 4
#define INKEVTYPE_MMOTION 5
#define INKEVTYPE_MBUTTON 6
#define INKEVTYPE_MWHEEL 7
#define INKEVTYPE_POV 8
#define INKEVTYPE_ACCEL 9
 
typedef void (*inkev_RAW)(const union egg_event *event,void *userdata);
typedef void (*inkev_JOY)(int plrid,int btnid,int value,int state,void *userdata);
typedef void (*inkev_TEXT)(int codepoint,void *userdata);
typedef void (*inkev_TOUCH)(int state,int x,int y,void *userdata);
typedef void (*inkev_MMOTION)(int x,int y,void *userdata);
typedef void (*inkev_MBUTTON)(int btnid,int value,int x,int y,void *userdata);
typedef void (*inkev_MWHEEL)(int dx,int dy,int x,int y,void *userdata);
typedef void (*inkev_POV)(int dx,int dy,int rx,int ry,int buttons,void *userdata);
typedef void (*inkev_ACCEL)(int x,int y,int z,void *userdata);

struct inkeep_listener {
  int id;
  int inkevtype;
  void *cb; // inkev_*, matching (inkevtype).
  void *userdata;
};

#define INKEEP_PLAYER_LIMIT 32 /* Limit both for players and for joystick devices. */

struct inkeep_joy {
  int devid;
  int plrid;
  uint16_t state;
  int8_t lx,ly;
};

extern struct inkeep {
  int mode;
  struct font *font;
  int texid_font;
  int id_next;
  struct inkeep_listener *listenerv;
  int listenerc,listenera;
  int screenw,screenh;
  int mousex,mousey;
  int touchid; // If nonzero, we are tracking a platform touch.
  int touchx,touchy;
  int *keycapv;
  int keycap_pagec,keycap_rowc,keycap_colc;
  int keycap_pagep,keycap_rowp,keycap_colp;
  int cursor_texid,cursor_imageid;
  int cursorx,cursory,cursorw,cursorh,cursorxform,cursorhotx,cursorhoty;
  int playerc;
  int fake_keyboard;
  int fake_pointer;
  double cursordx,cursordy;
  int cursorbtn;
  double fakemousex,fakemousey;
  double fakemouse_speed;
  int fakemouse_soft_hide;
  int keycolw,keyrowh;
  int keyoverlay_texid;
  int keyoverlay_dirty;
  int keydx,keydy;
  double keymoveclock;
  int quickshift;
  uint16_t playerv[INKEEP_PLAYER_LIMIT];
  struct inkeep_joy joyv[INKEEP_PLAYER_LIMIT];
  int joyc;
} inkeep;

void inkeep_broadcast_raw(const union egg_event *event);
void inkeep_broadcast_joy(int plrid,int btnid,int value,int state);
void inkeep_broadcast_text(int codepoint);
void inkeep_broadcast_touch(int state,int x,int y);
void inkeep_broadcast_mmotion(int x,int y);
void inkeep_broadcast_mbutton(int btnid,int value,int x,int y);
void inkeep_broadcast_mwheel(int dx,int dy,int x,int y);
void inkeep_broadcast_pov(int dx,int dy,int rx,int ry,int buttons);
void inkeep_broadcast_accel(int x,int y,int z);

void fake_keyboard_render();
void fake_keyboard_update(double elapsed);
void fake_keyboard_activate_at(int x,int y); // in framebuffer pixels
void fake_keyboard_focus_at(int x,int y);
void fake_keyboard_joy(int btnid,int value);

void fake_pointer_render();
void fake_pointer_update(double elapsed);
void fake_pointer_joy(int btnid,int value);
void fake_pointer_key(int keycode,int value);
void fake_pointer_mmotion(int x,int y);
void fake_pointer_mbutton(int btnid,int value);

#endif
