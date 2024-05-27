#ifndef HARDBOILED_H
#define HARDBOILED_H

#define SCREENW 240
#define SCREENH 135

#define VERB_NONE 0
#define VERB_TALK 1
#define VERB_EXAMINE 2
#define VERB_TAKE 3
#define VERB_GIVE 4
#define VERB_EXIT 5
#define VERB_COUNT 5

#define SUSPECT_COUNT 7

#include <egg/egg.h>
#include "stdlib/egg-stdlib.h"
#include "util/font.h"
#include "util/text.h"
#include "util/tile_renderer.h"
#include "inkeep/inkeep.h"
#include "room.h"
#include "popup.h"
#include "resid.h"

extern struct font *font;
extern struct room *current_room;

int change_room(int rid);
void set_focus_noun(int f);

void log_update(double elapsed);
void log_render();
void log_add_text(const char *src,int srcc); // We implicitly add a newline after.
void log_add_string(int stringid); // Resets verb too.
void log_add_string_keep_verb(int stringid); // ...ok turns out i don't want to always reset the verb
void log_clear();

int inv_get(int stringid); // => (0,1,2) = (untouched,have,given away)
void inv_set(int stringid,int has);
void inventory_press(int x,int y);
extern int inventory_sequence; // Increments whenever the global inventory changes.
void inventory_render();
extern int selected_item; // stringid or zero
void inventory_add_clue(int fieldid,int value); // puzzle_reveal_clue() calls this
void inventory_show_items();
void inv_reset();
int clues_count();
int inv_get_present(int p); // stringid of item in current inventory, (p) from zero
int inv_get_given(int p); // '' items formerly possessed and given away

void puzzle_init();
int puzzle_reveal_clue(char *dst,int dsta); // Picks one randomly, generates log message.
void puzzle_get_suspect(int *hair,int *shirt,int *tie,int suspectp);
int puzzle_get_guilty_index();

#endif
