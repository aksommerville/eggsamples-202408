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
  
void jlog_del(struct jlog *jlog);

struct jlog *jlog_new(struct bus *bus);

/* Call this once just after startup.
 * (v) contains up to 16 of:
 *   int name_string_id, // What should jcfg call it? For jlog, we don't actually care what the name is.
 *   int standard_btnid, // Zero or a btnid from Standard Mapping. See egg/etc/doc/joystick.md
 * Each name you provide corresponds little-endianly to one bit of the player state.
 * You're not allowed to have buttons without names, but if you're not using jcfg, they can be any integer.
 * I recommend "Left,Right,Up,Down" for the Dpad, then logical names like "Jump", "Fire", and use the standard button as a placement suggestion.
 */
int jlog_define_mapping(struct jlog *jlog,const int *v,int c);

int jlog_btnid_from_standard(const struct jlog *jlog,int stdbtnid);
int jlog_stringid_for_btnid(const struct jlog *jlog,int btnid);
int jlog_get_state_mask(const struct jlog *jlog); // => union of all btnid

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

/* Access to templates, so jcfg can update them.
 */
void *jlog_clear_template(struct jlog *jlog,int vid,int pid,int version,const char *name,int namec);
int jlog_append_template(struct jlog *jlog,void *tm,int srcbtnid,char srcpart,int dstbtnid);

#endif
