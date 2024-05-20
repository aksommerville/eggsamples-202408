#include "../hardboiled.h"

#define NOUN_MAYOR 1
#define NOUN_BOOZE 2
#define NOUN_PEN 3
#define NOUN_LETTER 4
#define NOUN_TEACUP 5

/* Object definition.
 */
 
struct room_cityhall {
  struct room hdr;
  int texid;
};

#define ROOM ((struct room_cityhall*)room)

/* Cleanup.
 */
 
static void _cityhall_del(struct room *room) {
  egg_texture_del(ROOM->texid);
}

/* Render.
 */
 
static void _cityhall_render(struct room *room,int x,int y,int w,int h) {
  egg_draw_decal(1,ROOM->texid,x,y,0,0,96,96,0);
}

/* Nouns in scene.
 */
 
static int _cityhall_noun_for_point(struct room *room,int x,int y) {
  if ((x>=32)&&(y>=4)&&(x<58)&&(y<68)) return NOUN_MAYOR;
  if ((x>=9)&&(y>=15)&&(x<19)&&(y<23)) return NOUN_BOOZE;
  if ((x>=8)&&(y>=67)&&(x<23)&&(y<71)) return NOUN_PEN;
  if ((x>=9)&&(y>=72)&&(x<31)&&(y<85)) return NOUN_LETTER;
  if ((x>=24)&&(y>=90)&&(x<39)&&(y<96)) return NOUN_TEACUP;
  return 0;
}

static int _cityhall_name_for_noun(struct room *room,int noun) {
  switch (noun) {
    case NOUN_MAYOR: return 20;
    case NOUN_BOOZE: return 21;
    case NOUN_PEN: return 22;
    case NOUN_LETTER: return 23;
    case NOUN_TEACUP: return 24;
  }
  return 0;
}

/* Act.
 */
 
static void _cityhall_act(struct room *room,int verb,int noun) {
  switch (noun) {
    case NOUN_MAYOR: switch (verb) {
        case 0: case VERB_TALK: log_add_string(25); break;
      } break;
    case NOUN_BOOZE: switch (verb) {
        case 0: case VERB_EXAMINE: log_add_string(26); break;
        //TODO Take?
      } break;
    case NOUN_PEN: switch (verb) {
        case 0: case VERB_EXAMINE: log_add_string(27); break;
        //TODO Take?
      } break;
    case NOUN_LETTER: switch (verb) {
        case 0: case VERB_EXAMINE: log_add_string(28); break;
        //TODO Take?
      } break;
    case NOUN_TEACUP: switch (verb) {
        case 0: case VERB_EXAMINE: log_add_string(29); break;
        //TODO Take?
      } break;
  }
}

/* New.
 */
 
struct room *room_new_cityhall() {
  struct room *room=calloc(1,sizeof(struct room_cityhall));
  if (!room) return 0;
  room->del=_cityhall_del;
  room->render=_cityhall_render;
  room->noun_for_point=_cityhall_noun_for_point;
  room->name_for_noun=_cityhall_name_for_noun;
  room->act=_cityhall_act;
  room->exit=room_new_streets;
  egg_texture_load_image(ROOM->texid=egg_texture_new(),0,RID_image_cityhall);
  return room;
}
