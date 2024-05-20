#include "../hardboiled.h"

#define NOUN_GUS 1

/* Object definition.
 */
 
struct room_newsstand {
  struct room hdr;
  int texid;
};

#define ROOM ((struct room_newsstand*)room)

/* Cleanup.
 */
 
static void _newsstand_del(struct room *room) {
  egg_texture_del(ROOM->texid);
}

/* Render.
 */
 
static void _newsstand_render(struct room *room,int x,int y,int w,int h) {
  egg_draw_decal(1,ROOM->texid,x,y,0,0,96,96,0);
}

/* Nouns in scene.
 */
 
static int _newsstand_noun_for_point(struct room *room,int x,int y) {
  if ((x>=27)&&(y>=26)&&(x<46)&&(y<58)) return NOUN_GUS;
  return 0;
}

static int _newsstand_name_for_noun(struct room *room,int noun) {
  switch (noun) {
    case NOUN_GUS: return 40;
  }
  return 0;
}

/* Act.
 */
 
static void _newsstand_act(struct room *room,int verb,int noun) {
  switch (noun) {
    case NOUN_GUS: switch (verb) {
        case 0: case VERB_TALK: log_add_string(41); break;
      } break;
  }
}

/* New.
 */
 
struct room *room_new_newsstand() {
  struct room *room=calloc(1,sizeof(struct room_newsstand));
  if (!room) return 0;
  room->del=_newsstand_del;
  room->render=_newsstand_render;
  room->noun_for_point=_newsstand_noun_for_point;
  room->name_for_noun=_newsstand_name_for_noun;
  room->act=_newsstand_act;
  room->exit=room_new_streets;
  egg_texture_load_image(ROOM->texid=egg_texture_new(),0,RID_image_newsstand);
  return room;
}
