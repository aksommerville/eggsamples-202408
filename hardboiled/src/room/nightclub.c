#include "../hardboiled.h"

#define NOUN_VIVECA 1
#define NOUN_ARMANDO 2

/* Object definition.
 */
 
struct room_nightclub {
  struct room hdr;
  int texid;
};

#define ROOM ((struct room_nightclub*)room)

/* Cleanup.
 */
 
static void _nightclub_del(struct room *room) {
  egg_texture_del(ROOM->texid);
}

/* Render.
 */
 
static void _nightclub_render(struct room *room,int x,int y,int w,int h) {
  egg_draw_decal(1,ROOM->texid,x,y,0,0,96,96,0);
}

/* Nouns in scene.
 */
 
static int _nightclub_noun_for_point(struct room *room,int x,int y) {
  if ((x>=3)&&(y>=30)&&(x<23)&&(y<96)) return NOUN_VIVECA;
  if ((x>=32)&&(y>=26)&&(x<54)&&(y<96)) return NOUN_ARMANDO;
  return 0;
}

static int _nightclub_name_for_noun(struct room *room,int noun) {
  switch (noun) {
    case NOUN_VIVECA: return 50;
    case NOUN_ARMANDO: return 51;
  }
  return 0;
}

/* Act.
 */
 
static void _nightclub_act(struct room *room,int verb,int noun) {
  switch (noun) {
    case NOUN_VIVECA: switch (verb) {
        case 0: case VERB_TALK: log_add_string(52); break;
      } break;
    case NOUN_ARMANDO: switch (verb) {
        case 0: case VERB_TALK: log_add_string(53); break;
      } break;
  }
}

/* New.
 */
 
struct room *room_new_nightclub() {
  struct room *room=calloc(1,sizeof(struct room_nightclub));
  if (!room) return 0;
  room->del=_nightclub_del;
  room->render=_nightclub_render;
  room->noun_for_point=_nightclub_noun_for_point;
  room->name_for_noun=_nightclub_name_for_noun;
  room->act=_nightclub_act;
  room->exit=room_new_streets;
  egg_texture_load_image(ROOM->texid=egg_texture_new(),0,RID_image_nightclub);
  return room;
}
