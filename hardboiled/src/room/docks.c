#include "../hardboiled.h"

#define NOUN_STEVE 1

/* Object definition.
 */
 
struct room_docks {
  struct room hdr;
  int texid;
};

#define ROOM ((struct room_docks*)room)

/* Cleanup.
 */
 
static void _docks_del(struct room *room) {
  egg_texture_del(ROOM->texid);
}

/* Render.
 */
 
static void _docks_render(struct room *room,int x,int y,int w,int h) {
  egg_draw_decal(1,ROOM->texid,x,y,0,0,96,96,0);
}

/* Nouns in scene.
 */
 
static int _docks_noun_for_point(struct room *room,int x,int y) {
  if ((x>=61)&&(y>=15)&&(x<88)&&(y<96)) return NOUN_STEVE;
  return 0;
}

static int _docks_name_for_noun(struct room *room,int noun) {
  switch (noun) {
    case NOUN_STEVE: return 90;
  }
  return 0;
}

/* Act.
 */
 
static void _docks_act(struct room *room,int verb,int noun) {
  switch (noun) {
    case NOUN_STEVE: switch (verb) {
        case 0: case VERB_TALK: log_add_string(91); break;
      } break;
  }
}

/* New.
 */
 
struct room *room_new_docks() {
  struct room *room=calloc(1,sizeof(struct room_docks));
  if (!room) return 0;
  room->del=_docks_del;
  room->render=_docks_render;
  room->noun_for_point=_docks_noun_for_point;
  room->name_for_noun=_docks_name_for_noun;
  room->act=_docks_act;
  room->exit=room_new_streets;
  egg_texture_load_image(ROOM->texid=egg_texture_new(),0,RID_image_docks);
  return room;
}
