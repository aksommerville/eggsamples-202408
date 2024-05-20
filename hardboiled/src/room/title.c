#include "../hardboiled.h"

/* Object definition.
 */
 
struct room_title {
  struct room hdr;
  int texid;
};

#define ROOM ((struct room_title*)room)

/* Cleanup.
 */
 
static void _title_del(struct room *room) {
  egg_texture_del(ROOM->texid);
}

/* Render.
 */

static void _title_render(struct room *room,int x,int y,int w,int h) {
  egg_draw_decal(1,ROOM->texid,x,y,0,0,w,h,0);
}

/* Act.
 */
 
static void _title_act(struct room *room,int verb,int noun) {
  change_room(room_new_hq);
}

/* New.
 */
 
struct room *room_new_title() {
  struct room *room=calloc(1,sizeof(struct room_title));
  if (!room) return 0;
  room->del=_title_del;
  room->render=_title_render;
  room->act=_title_act;
  if ((ROOM->texid=egg_texture_new())<1) {
    free(room);
    return 0;
  }
  if (egg_texture_load_image(ROOM->texid,0,RID_image_title)<0) {
    _title_del(room);
    free(room);
    return 0;
  }
  return room;
}
