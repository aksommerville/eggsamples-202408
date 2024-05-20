#include "../hardboiled.h"

/* Object definition.
 */
 
struct room_lineup {
  struct room hdr;
  int texid;
};

#define ROOM ((struct room_lineup*)room)

/* Cleanup.
 */
 
static void _lineup_del(struct room *room) {
  egg_texture_del(ROOM->texid);
}

/* Render.
 */
 
static void _lineup_render(struct room *room,int x,int y,int w,int h) {
  egg_draw_decal(1,ROOM->texid,x,y,0,0,96,96,0);
}

/* Nouns in scene.
 */
 
static int _lineup_noun_for_point(struct room *room,int x,int y) {
  return 0;
}

static int _lineup_name_for_noun(struct room *room,int noun) {
  return 0;
}

/* Act.
 */
 
static void _lineup_act(struct room *room,int verb,int noun) {
  egg_log("%s %d %d",__func__,verb,noun);
}

/* New.
 */
 
struct room *room_new_lineup() {
  struct room *room=calloc(1,sizeof(struct room_lineup));
  if (!room) return 0;
  room->del=_lineup_del;
  room->render=_lineup_render;
  room->noun_for_point=_lineup_noun_for_point;
  room->name_for_noun=_lineup_name_for_noun;
  room->act=_lineup_act;
  room->exit=room_new_hq;
  egg_texture_load_image(ROOM->texid=egg_texture_new(),0,RID_image_lineup);
  return room;
}
