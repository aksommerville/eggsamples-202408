#include "../hardboiled.h"

#define NOUN_MORT 1

/* Object definition.
 */
 
struct room_warehouse {
  struct room hdr;
  int texid;
};

#define ROOM ((struct room_warehouse*)room)

/* Cleanup.
 */
 
static void _warehouse_del(struct room *room) {
  egg_texture_del(ROOM->texid);
}

/* Render.
 */
 
static void _warehouse_render(struct room *room,int x,int y,int w,int h) {
  egg_draw_decal(1,ROOM->texid,x,y,0,0,96,96,0);
}

/* Nouns in scene.
 */
 
static int _warehouse_noun_for_point(struct room *room,int x,int y) {
  if ((x>=55)&&(y>=14)&&(x<75)&&(y<96)) return NOUN_MORT;
  return 0;
}

static int _warehouse_name_for_noun(struct room *room,int noun) {
  switch (noun) {
    case NOUN_MORT: return 80;
  }
  return 0;
}

/* Act.
 */
 
static void _warehouse_act(struct room *room,int verb,int noun) {
  switch (noun) {
    case NOUN_MORT: switch (verb) {
        case 0: case VERB_TALK: log_add_string(81); break;
      } break;
  }
}

/* New.
 */
 
struct room *room_new_warehouse() {
  struct room *room=calloc(1,sizeof(struct room_warehouse));
  if (!room) return 0;
  room->del=_warehouse_del;
  room->render=_warehouse_render;
  room->noun_for_point=_warehouse_noun_for_point;
  room->name_for_noun=_warehouse_name_for_noun;
  room->act=_warehouse_act;
  room->exit=room_new_streets;
  egg_texture_load_image(ROOM->texid=egg_texture_new(),0,RID_image_warehouse);
  return room;
}
