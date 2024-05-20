#ifndef HARDBOILED_H
#define HARDBOILED_H

#define SCREENW 240
#define SCREENH 135

#define VERB_NONE 0
#define VERB_GO 1
#define VERB_TALK 2
#define VERB_EXAMINE 3
#define VERB_OPEN 4
#define VERB_EXIT 5
#define VERB_COUNT 5

#include <egg/egg.h>
#include "stdlib/egg-stdlib.h"
#include "util/font.h"
#include "util/text.h"
#include "util/tile_renderer.h"
#include "inkeep/inkeep.h"
#include "resid.h"

extern struct font *font;

struct room {
  void (*del)(struct room *room);
  void (*render)(struct room *room,int x,int y,int w,int h); // (w,h) will always be (96,96)
  void (*update)(struct room *room,double elapsed);
  int (*noun_for_point)(struct room *room,int x,int y); // (x,y) in (0,0)..(95,95), return noun or 0
  int (*name_for_noun)(struct room *room,int noun); // string id
  void (*act)(struct room *room,int verb,int noun); // (verb) can be zero
  struct room *(*exit)();
};

int change_room(struct room *(*ctor)());
struct room *room_new_title();
struct room *room_new_hq();
struct room *room_new_streets();
struct room *room_new_cityhall();
struct room *room_new_shoeshine();
struct room *room_new_newsstand();
struct room *room_new_warehouse();
struct room *room_new_nightclub();
struct room *room_new_park();
struct room *room_new_docks();
struct room *room_new_tenement();
struct room *room_new_lineup();

void log_render();
void log_add_text(const char *src,int srcc); // We implicitly add a newline after.
void log_add_string(int stringid); // Resets verb too.

#endif
