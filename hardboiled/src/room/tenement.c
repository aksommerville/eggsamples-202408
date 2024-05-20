#include "../hardboiled.h"

#define NOUN_CHARLIE 1
#define NOUN_BILLY 2
#define NOUN_BOTTLE 3

/* Object definition.
 */
 
struct room_tenement {
  struct room hdr;
  int texid;
};

#define ROOM ((struct room_tenement*)room)

/* Cleanup.
 */
 
static void _tenement_del(struct room *room) {
  egg_texture_del(ROOM->texid);
}

/* Render.
 */
 
static void _tenement_render(struct room *room,int x,int y,int w,int h) {
  egg_draw_decal(1,ROOM->texid,x,y,0,0,96,96,0);
}

/* Nouns in scene.
 */
 
static int _tenement_noun_for_point(struct room *room,int x,int y) {
  if ((x>=26)&&(y>=42)&&(x<42)&&(y<80)) return NOUN_CHARLIE;
  if ((x>=68)&&(y>=64)&&(x<80)&&(y<96)) return NOUN_BILLY;
  if ((x>=3)&&(y>=89)&&(x<21)&&(y<93)) return NOUN_BOTTLE;
  return 0;
}

static int _tenement_name_for_noun(struct room *room,int noun) {
  switch (noun) {
    case NOUN_CHARLIE: return 70;
    case NOUN_BILLY: return 71;
    case NOUN_BOTTLE: return 72;
  }
  return 0;
}

/* Act.
 */
 
static void _tenement_act(struct room *room,int verb,int noun) {
  switch (noun) {
    case NOUN_CHARLIE: switch (verb) {
        case 0: case VERB_TALK: log_add_string(73); break;
      } break;
    case NOUN_BILLY: switch (verb) {
        case 0: case VERB_TALK: log_add_string(74); break;
      } break;
    case NOUN_BOTTLE: switch (verb) {
        case 0: case VERB_EXAMINE: log_add_string(75); break;
      } break;
  }
}

/* New.
 */
 
struct room *room_new_tenement() {
  struct room *room=calloc(1,sizeof(struct room_tenement));
  if (!room) return 0;
  room->del=_tenement_del;
  room->render=_tenement_render;
  room->noun_for_point=_tenement_noun_for_point;
  room->name_for_noun=_tenement_name_for_noun;
  room->act=_tenement_act;
  room->exit=room_new_streets;
  egg_texture_load_image(ROOM->texid=egg_texture_new(),0,RID_image_tenement);
  return room;
}
