#include "../hardboiled.h"

#define NOUN_JIMMY 1

/* Object definition.
 */
 
struct room_shoeshine {
  struct room hdr;
  int texid;
};

#define ROOM ((struct room_shoeshine*)room)

/* Cleanup.
 */
 
static void _shoeshine_del(struct room *room) {
  egg_texture_del(ROOM->texid);
}

/* Render.
 */
 
static void _shoeshine_render(struct room *room,int x,int y,int w,int h) {
  egg_draw_decal(1,ROOM->texid,x,y,0,0,96,96,0);
}

/* Nouns in scene.
 */
 
static int _shoeshine_noun_for_point(struct room *room,int x,int y) {
  if ((x>=47)&&(y>=12)&&(x<68)&&(y<71)) return NOUN_JIMMY;
  return 0;
}

static int _shoeshine_name_for_noun(struct room *room,int noun) {
  switch (noun) {
    case NOUN_JIMMY: return 30;
  }
  return 0;
}

/* Act.
 */
 
static void _shoeshine_act(struct room *room,int verb,int noun) {
  switch (noun) {
    case NOUN_JIMMY: switch (verb) {
        case 0: case VERB_TALK: log_add_string(31); break;
      } break;
  }
}

/* New.
 */
 
struct room *room_new_shoeshine() {
  struct room *room=calloc(1,sizeof(struct room_shoeshine));
  if (!room) return 0;
  room->del=_shoeshine_del;
  room->render=_shoeshine_render;
  room->noun_for_point=_shoeshine_noun_for_point;
  room->name_for_noun=_shoeshine_name_for_noun;
  room->act=_shoeshine_act;
  room->exit=room_new_streets;
  egg_texture_load_image(ROOM->texid=egg_texture_new(),0,RID_image_shoeshine);
  return room;
}
