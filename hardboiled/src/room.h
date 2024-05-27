/* room.h
 * All the action happens in rooms.
 * Each room has an image with regions marked as nouns.
 * These are backed by resources also called "room".
 *
 * DATA FORMAT, TEXT:
 * Line-oriented text, '#' starts a line comment anywhere.
 *   image NAME # 1
 *   song NAME # 0|1
 *   exit NAME # room; 0|1
 *   noun X Y W H [name=STRINGID] [go=ROOMID] [takeable=SRCX,SRCY] [prestring=STRINGID [wants=STRINGID poststring=STRINGID]]
 *   special lineup
 *
 * DATA FORMAT, BINARY:
 *   eof:      0x00
 *   image:    0x01 u16:rid
 *   song:     0x02 u16:rid
 *   exit:     0x03 u16:rid
 *   door:     0x04 u8:x u8:y u8:w u8:h u16:name u16:go
 *   takeable: 0x05 u8:dstx u8:dsty u8:w u8:h u16:name u16:srcx u16:srcy
 *   witness:  0x06 u8:x u8:y u8:w u8:h u16:name u16:prestring u16:itemstring u16:poststring
 *   special:  0x07 u8:v (ROOM_SPECIAL_*)
 *
 * If noun regions overlap, the first one wins.
 * Rule of thumb: Put smaller things first.
 * A witness with only (prestring) set is static text.
 */
 
#ifndef ROOM_H
#define ROOM_H

/* Some rooms have extra logic tacked on, things I didn't bother figuring out how to generalize.
 */
#define ROOM_SPECIAL_LINEUP 1

/* "noun" is anything that occupies screen space in a room.
 * If it has a name, that name is displayed when the cursor hovers.
 * Nouns have an arbitrary ID, which is their index plus one.
 */
struct noun {
  uint8_t x,y,w,h;
  uint16_t name;
  uint16_t roomid; // If nonzero, clicking here navigates to another room.
  uint16_t inv_srcx,inv_srcy; // If (inv_srcy) nonzero, this is a takeable inventory item, keyed by (name).
  uint16_t wants,prestring,poststring; // (prestring) alone for static text. (wants,poststring) for appeasable witnesses.
};

struct room {
  uint16_t rid;
  uint16_t imageid;
  uint16_t songid;
  uint16_t exit; // room id
  int texid;
  struct noun *nounv;
  int nounc,nouna;
  int special;
};

void room_del(struct room *room);
struct room *room_new(int rid);

/* Every function here tolerates null room.
 */
int room_get_exit(const struct room *room); // => room id or zero
int room_noun_for_point(const struct room *room,int x,int y); // (0,0)..(95,95); => noun or zero
int room_name_for_noun(const struct room *room,int noun); // => string id or zero
int room_act(struct room *room,int verb,int noun); // => Nonzero if accepted.
void room_update(struct room *room,double elapsed);
void room_render(struct room *room,int x,int y,int w,int h);

#endif
