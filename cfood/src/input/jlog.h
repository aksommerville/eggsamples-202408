/* jlog.h
 * Logical joystick mapping.
 * This is the big kahuna of our input system.
 * Takes input exclusively from joy2, ie normalized two-state buttons.
 * Produces a 16-bit state for an arbitrary set of "players".
 */
 
#ifndef JLOG_H
#define JLOG_H

struct jlog;
struct bus;

/* The button IDs for Standard Mapping, as Egg reports them.
 */
#define STDBTN_SOUTH 0x80
#define STDBTN_EAST  0x81
#define STDBTN_WEST  0x82
#define STDBTN_NORTH 0x83
#define STDBTN_L1    0x84
#define STDBTN_R1    0x85
#define STDBTN_L2    0x86
#define STDBTN_R2    0x87
#define STDBTN_AUX2  0x88
#define STDBTN_AUX1  0x89
#define STDBTN_LP    0x8a
#define STDBTN_RP    0x8b
#define STDBTN_UP    0x8c
#define STDBTN_DOWN  0x8d
#define STDBTN_LEFT  0x8e
#define STDBTN_RIGHT 0x8f
#define STDBTN_AUX3  0x90
#define STDBTN_LX    0x40
#define STDBTN_LY    0x41
#define STDBTN_RX    0x42
#define STDBTN_RY    0x43

/* If the setup seems a bit much, you can jlog_define_mapping() with this.
 * Put your button names in order, in your string resources, and provide the first one here.
 * I don't recommend using this actually; are you really using all eleven buttons? Better to declare only the ones you use.
 */
#define JLOG_DEFAULT_FULL_GAMEPAD(stringid) \
  stringid+ 0,STDBTN_LEFT , \
  stringid+ 1,STDBTN_RIGHT, \
  stringid+ 2,STDBTN_UP   , \
  stringid+ 3,STDBTN_DOWN , \
  stringid+ 4,STDBTN_SOUTH, \
  stringid+ 5,STDBTN_WEST , \
  stringid+ 6,STDBTN_EAST , \
  stringid+ 7,STDBTN_NORTH, \
  stringid+ 8,STDBTN_L1   , \
  stringid+ 9,STDBTN_R1   , \
  stringid+10,STDBTN_L2   , \
  stringid+11,STDBTN_R2   , \
  stringid+12,STDBTN_AUX1 , \
  stringid+13,STDBTN_AUX2 , \
  stringid+14,STDBTN_AUX3 , \
  0
  
void jlog_del(struct jlog *jlog);

struct jlog *jlog_new(struct bus *bus);

/* Call this once just after startup.
 * Variadic arguments are up to 16 of:
 *   int name_string_id, // What should jcfg call it? For jlog, we don't actually care what the name is.
 *   int standard_btnid, // Zero or a btnid from Standard Mapping. See egg/etc/doc/joystick.md
 * Each name your provide corresponds little-endianly to one bit of the player state.
 * You're not allowed to have buttons without names, but if you're not using jcfg, they can be any integer.
 */
int jlog_define_mapping_(struct jlog *jlog,...);
#define jlog_define_mapping(jlog,...) jlog_define_mapping_(jlog,##__VA_ARGS__,0)

int jlog_btnid_from_standard(const struct jlog *jlog,int stdbtnid);

/* Minimum one, and we might enforce a maximum too.
 * Setting returns the actual new count, no errors.
 * Default 1.
 */
int jlog_get_player_count(const struct jlog *jlog);
int jlog_set_player_count(struct jlog *jlog,int playerc);

int jlog_get_player(const struct jlog *jlog,int playerid);

/* Is any device currently feeding this (playerid)?
 * It's sensible to ask for (playerid==0), that means "is any real device not yet associated with a player?"
 */
int jlog_is_player_mapped(const struct jlog *jlog,int playerid);

void jlog_enable(struct jlog *jlog,int enable);

/* These happen automatically, don't worry about it.
 * Templates get stored in egg storage as "joymap".
 * Each template is formatted about like this:
 *   VID PID VERSION NAME
 *   [(SRCBTNID PART DSTBTNID) ...]
 * NAME is terminated by an LF.
 * Integers are all hexadecimal with no prefix, lowercase letters only.
 * PART is one character.
 * The parens in the buttons block are syntatically redundant, they're intended to help prevent mistakes.
 * We'll add an LF after each buttons block but that's optional.
 */
int jlog_load_templates(struct jlog *jlog);
int jlog_store_templates(struct jlog *jlog);

#endif
