#include "../hardboiled.h"

#define NOUN_LUCY 1
#define NOUN_MILLIE 2
#define NOUN_DOG 3

/* Object definition.
 */
 
struct room_park {
  struct room hdr;
  int texid;
};

#define ROOM ((struct room_park*)room)

/* Cleanup.
 */
 
static void _park_del(struct room *room) {
  egg_texture_del(ROOM->texid);
}

/* Render.
 */
 
static void _park_render(struct room *room,int x,int y,int w,int h) {
  egg_draw_decal(1,ROOM->texid,x,y,0,0,96,96,0);
}

/* Nouns in scene.
 */
 
static int _park_noun_for_point(struct room *room,int x,int y) {
  if ((x>=21)&&(y>=30)&&(x<35)&&(y<73)) return NOUN_LUCY;
  if ((x>=61)&&(y>=33)&&(x<74)&&(y<61)) return NOUN_MILLIE;
  if ((x>=39)&&(y>=74)&&(x<52)&&(y<84)) return NOUN_DOG;
  return 0;
}

static int _park_name_for_noun(struct room *room,int noun) {
  switch (noun) {
    case NOUN_LUCY: return 60;
    case NOUN_MILLIE: return 61;
    case NOUN_DOG: return 62;
  }
  return 0;
}

/* Act.
 */
 
static void _park_act(struct room *room,int verb,int noun) {
  switch (noun) {
    case NOUN_LUCY: switch (verb) {
        case 0: case VERB_TALK: log_add_string(63); break;
      } break;
    case NOUN_MILLIE: switch (verb) {
        case 0: case VERB_TALK: log_add_string(64); break;
      } break;
    case NOUN_DOG: switch (verb) {
        case 0: case VERB_EXAMINE: log_add_string(65); break;
        case VERB_TALK: log_add_string(66); break;
      } break;
  }
}

/* New.
 */
 
struct room *room_new_park() {
  struct room *room=calloc(1,sizeof(struct room_park));
  if (!room) return 0;
  room->del=_park_del;
  room->render=_park_render;
  room->noun_for_point=_park_noun_for_point;
  room->name_for_noun=_park_name_for_noun;
  room->act=_park_act;
  room->exit=room_new_streets;
  egg_texture_load_image(ROOM->texid=egg_texture_new(),0,RID_image_park);
  return room;
}
