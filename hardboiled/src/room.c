#include "hardboiled.h"
#include "verblist.h"

/* Delete.
 */
 
void room_del(struct room *room) {
  if (!room) return;
  egg_texture_del(room->texid);
  if (room->nounv) free(room->nounv);
  free(room);
}

/* Add noun.
 */
 
static struct noun *room_add_noun(struct room *room,uint8_t x,uint8_t y,uint8_t w,uint8_t h,uint16_t name) {
  if (room->nounc>=room->nouna) {
    int na=room->nouna+8;
    if (na>INT_MAX/sizeof(struct noun)) return 0;
    void *nv=realloc(room->nounv,sizeof(struct noun)*na);
    if (!nv) return 0;
    room->nounv=nv;
    room->nouna=na;
  }
  struct noun *noun=room->nounv+room->nounc++;
  memset(noun,0,sizeof(struct noun));
  noun->x=x;
  noun->y=y;
  noun->w=w;
  noun->h=h;
  noun->name=name;
  return noun;
}

/* Initialize new room after decode.
 */
 
static int room_init(struct room *room) {
  if (room->imageid) {
    if ((room->texid=egg_texture_new())<1) return -1;
    if (egg_texture_load_image(room->texid,0,room->imageid)<0) {
      egg_log("Failed to load image %d for room %d.",room->imageid,room->rid);
      return -1;
    }
  }
  if (room->songid) egg_audio_play_song(0,room->songid,0,1);
  return 0;
}

/* Decode new room. Calls room_init too.
 */
 
static int room_decode(struct room *room,const uint8_t *src,int srcc) {
  int srcp=0;
  while (srcp<srcc) {
    uint8_t opcode=src[srcp++];
    if (!opcode) break; // EOF. We won't use.
    switch (opcode) {
      #define REQUIRE(c) if (srcp>srcc-(c)) return -1;
      #define RD8 src[srcp++]
      #define RD16 ({ uint16_t vv=(src[srcp]<<8)|src[srcp+1]; srcp+=2; vv; })
      
      case 0x01: { // image: 0x01 u16:rid
          REQUIRE(2)
          room->imageid=RD16;
        } break;
      
      case 0x02: { // song: 0x02 u16:rid
          REQUIRE(2)
          room->songid=RD16;
        } break;
      
      case 0x03: { // exit: 0x03 u16:rid
          REQUIRE(2)
          room->exit=RD16;
        } break;
      
      case 0x04: { // door: 0x04 u8:x u8:y u8:w u8:h u16:name u16:go
          REQUIRE(8)
          uint8_t x=RD8;
          uint8_t y=RD8;
          uint8_t w=RD8;
          uint8_t h=RD8;
          uint16_t name=RD16;
          uint16_t roomid=RD16;
          struct noun *noun=room_add_noun(room,x,y,w,h,name);
          if (!noun) return -1;
          noun->roomid=roomid;
        } break;
      
      case 0x05: { // takeable: 0x05 u8:dstx u8:dsty u8:w u8:h u16:name u16:srcx u16:srcy
          REQUIRE(10)
          uint8_t x=RD8;
          uint8_t y=RD8;
          uint8_t w=RD8;
          uint8_t h=RD8;
          uint16_t name=RD16;
          uint16_t srcx=RD16;
          uint16_t srcy=RD16;
          struct noun *noun=room_add_noun(room,x,y,w,h,name);
          if (!noun) return -1;
          noun->inv_srcx=srcx;
          noun->inv_srcy=srcy;
        } break;
      
      case 0x06: { // witness: 0x06 u8:x u8:y u8:w u8:h u16:name u16:prestring u16:itemstring u16:poststring
          REQUIRE(12)
          uint8_t x=RD8;
          uint8_t y=RD8;
          uint8_t w=RD8;
          uint8_t h=RD8;
          uint16_t name=RD16;
          uint16_t pre=RD16;
          uint16_t item=RD16;
          uint16_t post=RD16;
          struct noun *noun=room_add_noun(room,x,y,w,h,name);
          if (!noun) return -1;
          noun->wants=item;
          noun->prestring=pre;
          noun->poststring=post;
        } break;
      
      #undef REQUIRE
      #undef RD8
      #undef RD16
      default: {
          egg_log("room:%d: Unknown opcode 0x%02x at %d/%d",room->rid,opcode,srcp-1,srcc);
          return -1;
        }
    }
  }
  return room_init(room);
}

/* New.
 */
 
struct room *room_new(int rid) {
  uint8_t serial[1024]; // 100 should be enough; if it exceeds 1024 something must be broken.
  int serialc=egg_res_get(serial,sizeof(serial),EGG_RESTYPE_room,0,rid);
  if ((serialc<1)||(serialc>sizeof(serial))) return 0;
  struct room *room=calloc(1,sizeof(struct room));
  if (!room) return 0;
  room->rid=rid;
  if (room_decode(room,serial,serialc)<0) {
    room_del(room);
    return 0;
  }
  return room;
}

/* Trivial accessors.
 */

int room_get_exit(const struct room *room) {
  if (!room) return 0;
  return room->exit;
}

/* Nouns.
 */
 
int room_noun_for_point(const struct room *room,int x,int y) {
  if (!room) return 0;
  const struct noun *noun=room->nounv;
  int id=1;
  for (;id<=room->nounc;id++,noun++) {
    if (x<noun->x) continue;
    if (y<noun->y) continue;
    if (x>=noun->x+noun->w) continue;
    if (y>=noun->y+noun->h) continue;
    if (noun->inv_srcy) {
      if (inv_get(noun->name)) {
        continue;
      }
    }
    return id;
  }
  return 0;
}

int room_name_for_noun(const struct room *room,int id) {
  if (id<1) return 0;
  id--;
  if (id>=room->nounc) return 0;
  return room->nounv[id].name;
}

/* Act, if clicked outside any noun.
 * I think this will always be noop.
 */
 
static int room_act_nounless(struct room *room,int verb) {
  return 0;
}

/* Act on witness.
 * The provided noun must have prestring and wants (and presumably poststring) set.
 */
 
static int room_act_on_witness(struct room *room,int verb,const struct noun *noun) {
  int haveitem=inv_get(noun->wants);
  if (haveitem==0) { // Not yet sated, or static text only.
    switch (verb) {
      case VERB_NONE: case VERB_TALK: log_add_string(noun->prestring); return 1;
    }
    return 0;
  }
  if (haveitem==2) { // Already gave the item and acquired the clue.
    switch (verb) {
      case VERB_NONE: case VERB_TALK: if (noun->poststring) {
          log_add_string(noun->poststring);
          return 1;
        } break;
    }
    return 0;
  }
  // We are in the right state to sate this witness.
  switch (verb) {
    case VERB_NONE: case VERB_TALK: log_add_string(noun->prestring); return 1;
    case VERB_GIVE: {
        if (!selected_item) {
          log_add_string(RID_string_givenull);
          return 0;
        }
        if (selected_item!=noun->wants) {
          log_add_string(RID_string_unwanted);
          return 0;
        }
        verblist_unselect();
        inv_set(noun->wants,2);
        egg_log("%s:%d:TODO: Gave away item, now we need to generate a clue!",__FILE__,__LINE__);
        log_add_text("TODO clue",9);
      } break;
  }
  return 0;
}

/* Act.
 */
 
int room_act(struct room *room,int verb,int nounid) {
  if (nounid<1) return room_act_nounless(room,verb);
  nounid--;
  if (nounid>=room->nounc) return room_act_nounless(room,verb);
  struct noun *noun=room->nounv+nounid;
  
  // If (roomid) set, it's a simple door.
  if (noun->roomid) {
    switch (verb) {
      case VERB_NONE: {
          if (change_room(noun->roomid)<0) return -1;
        } return 1;
    }
    return 0;
  }
  
  // If (inv_srcy) set, it's takeable inventory.
  if (noun->inv_srcy) {
    if (inv_get(noun->name)) return 0; // Shouldn't have landed here, but ok nix it.
    switch (verb) {
      case VERB_NONE: case VERB_TAKE: {
          inv_set(noun->name,1);
          set_focus_noun(0);
        } return 1;
      //TODO These should be Examinable too, innit?
    }
    return 0;
  }
  
  // If (prestring) set it's either static text (wants==0) or a live witness (wants!=0).
  if (noun->prestring) {
    if (noun->wants) {
      return room_act_on_witness(room,verb,noun);
    } else {
      switch (verb) {
        case VERB_NONE: case VERB_TALK: case VERB_EXAMINE: log_add_string(noun->prestring); return 1;
      }
    }
  }
  
  return 0;
}

/* Update.
 */
 
void room_update(struct room *room,double elapsed) {
  // TODO
}

/* Render.
 */
 
void room_render(struct room *room,int x,int y,int w,int h) {
  if (!room->texid) { // Placeholder, should only come up during dev.
    egg_draw_rect(1,x,y,w,h,0xff8000ff);
    return;
  }

  // Background image must be at (0,0) in the room's image.
  egg_draw_decal(1,room->texid,x,y,0,0,w,h,0);

  // Inventoriable nouns.
  const struct noun *noun=room->nounv;
  int i=room->nounc;
  for (;i-->0;noun++) {
    if (!noun->inv_srcy) continue;
    if (inv_get(noun->name)) continue;
    egg_draw_decal(1,room->texid,x+noun->x,y+noun->y,noun->inv_srcx,noun->inv_srcy,noun->w,noun->h,0);
  }

  // TODO Animation?
}
