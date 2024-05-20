#include "../hardboiled.h"

static const struct streets_noun {
  int x,y,w,h;
  int stringid;
  struct room *(*ctor)();
} streets_nounv[]={
  {24,0,36,20,8,room_new_cityhall},
  {71,11,6,9,9,room_new_shoeshine},
  {11,32,6,13,10,room_new_newsstand},
  {24,25,36,30,11,room_new_hq},
  {75,23,21,31,12,room_new_warehouse},
  {0,61,24,32,13,room_new_nightclub},
  {25,69,35,23,14,room_new_park},
  {80,68,16,13,15,room_new_docks},
  {0,19,11,55,16,room_new_tenement},
};

/* Object definition.
 */
 
struct room_streets {
  struct room hdr;
  int texid;
};

#define ROOM ((struct room_streets*)room)

/* Cleanup.
 */
 
static void _streets_del(struct room *room) {
  egg_texture_del(ROOM->texid);
}

/* Render.
 */
 
static void _streets_render(struct room *room,int x,int y,int w,int h) {
  egg_draw_decal(1,ROOM->texid,x,y,0,0,96,96,0);
}

/* Nouns in scene.
 */
 
static int _streets_noun_for_point(struct room *room,int x,int y) {
  int noun=1;
  const struct streets_noun *meta=streets_nounv;
  int i=sizeof(streets_nounv)/sizeof(streets_nounv[0]);
  for (;i-->0;meta++,noun++) {
    if (x<meta->x) continue;
    if (y<meta->y) continue;
    if (x>=meta->x+meta->w) continue;
    if (y>=meta->y+meta->h) continue;
    return noun;
  }
  return 0;
}

static int _streets_name_for_noun(struct room *room,int noun) {
  if (noun<1) return 0;
  int c=sizeof(streets_nounv)/sizeof(streets_nounv[0]);
  if (noun>c) return 0;
  return streets_nounv[noun-1].stringid;
}

/* Act.
 */
 
static void _streets_act(struct room *room,int verb,int noun) {
  egg_log("%s %d %d",__func__,verb,noun);
  if (noun<1) return;
  int c=sizeof(streets_nounv)/sizeof(streets_nounv[0]);
  if (noun>c) return;
  change_room(streets_nounv[noun-1].ctor);
}

/* New.
 */
 
struct room *room_new_streets() {
  struct room *room=calloc(1,sizeof(struct room_streets));
  if (!room) return 0;
  room->del=_streets_del;
  room->render=_streets_render;
  room->noun_for_point=_streets_noun_for_point;
  room->name_for_noun=_streets_name_for_noun;
  room->act=_streets_act;
  egg_texture_load_image(ROOM->texid=egg_texture_new(),0,RID_image_streets);
  return room;
}
