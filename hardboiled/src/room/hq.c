#include "../hardboiled.h"

#define NOUN_OMALLEY 1
#define NOUN_LINEUP 2

/* Object definition.
 */
 
struct room_hq {
  struct room hdr;
  int texid;
};

#define ROOM ((struct room_hq*)room)

/* Cleanup.
 */
 
static void _hq_del(struct room *room) {
  egg_texture_del(ROOM->texid);
}

/* Render.
 */
 
static void _hq_render(struct room *room,int x,int y,int w,int h) {
  egg_draw_decal(1,ROOM->texid,x,y,0,0,96,96,0);
}

/* Nouns in scene.
 */
 
static int _hq_noun_for_point(struct room *room,int x,int y) {
  if ((x>=10)&&(y>=5)&&(x<35)&&(y<96)) return NOUN_OMALLEY;
  if ((x>=57)&&(y>=13)&&(x<94)&&(y<76)) return NOUN_LINEUP;
  return 0;
}

static int _hq_name_for_noun(struct room *room,int noun) {
  switch (noun) {
    case NOUN_OMALLEY: return 17;
    case NOUN_LINEUP: return 18;
  }
  return 0;
}

/* Talk to Sgt O'Malley.
 */
 
static void hq_talk_to_omalley() {
  log_add_string(19);
}

/* Act.
 */
 
static void _hq_act(struct room *room,int verb,int noun) {
  egg_log("%s %d %d",__func__,verb,noun);
  switch (noun) {
    case NOUN_OMALLEY: switch (verb) {
        case 0: case VERB_TALK: hq_talk_to_omalley(); break;
        case VERB_EXAMINE: break;//TODO
      } break;
    case NOUN_LINEUP: switch (verb) {
        case 0: case VERB_GO: case VERB_OPEN: change_room(room_new_lineup); break;
        case VERB_EXAMINE: break;//TODO
      } break;
  }
}

/* New.
 */
 
struct room *room_new_hq() {
  struct room *room=calloc(1,sizeof(struct room_hq));
  if (!room) return 0;
  room->del=_hq_del;
  room->render=_hq_render;
  room->noun_for_point=_hq_noun_for_point;
  room->name_for_noun=_hq_name_for_noun;
  room->act=_hq_act;
  room->exit=room_new_streets;
  egg_texture_load_image(ROOM->texid=egg_texture_new(),0,RID_image_hq);
  return room;
}
