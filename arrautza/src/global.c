#include "arrautza.h"

/* Load image for new map.
 */
 
static int load_map_image(uint16_t imageid) {
  if (imageid==g.imageid_tilesheet) return 0;
  g.imageid_tilesheet=imageid;
  if (egg_texture_load_image(g.texid_tilesheet,0,imageid)<0) {
    egg_log("Error loading image:%d for map:%d",imageid,g.mapid);
    return -1;
  }
  return 0;
}

/* Read command for newly-loaded map.
 */
 
static int load_map_cb(const uint8_t *cmd,int cmdc,void *userdata) {
  switch (cmd[0]) {
    case MAPCMD_hero: egg_log("TODO: Hero at %d,%d",cmd[1],cmd[2]); break;
    case MAPCMD_song: egg_audio_play_song(0,(cmd[1]<<8)|cmd[2],0,1); break;
    case MAPCMD_image: return load_map_image((cmd[1]<<8)|cmd[2]);
    //TODO neighbor[nswe], door
  }
  return 0;
}

/* Load map.
 */
 
int load_map(uint16_t mapid) {
  g.mapid=mapid;
  int c=egg_res_get(&g.map,sizeof(g.map),EGG_RESTYPE_map,0,mapid);
  if ((c<COLC*ROWC)||(c>sizeof(struct map))) {
    egg_log("Invalid size %d for map:%d. Must be in %d..%d.",c,mapid,COLC*ROWC,(int)sizeof(struct map));
    return -1;
  }
  if (map_for_each_command(&g.map,load_map_cb,0)<0) return -1;
  return 0;
}

/* Render map.
 */
 
void render_map() {
  struct egg_draw_tile vtxv[COLC*ROWC];
  struct egg_draw_tile *vtx=vtxv;
  int y=TILESIZE>>1;
  int yi=ROWC;
  const uint8_t *src=g.map.v;
  for (;yi-->0;y+=TILESIZE) {
    int x=TILESIZE>>1;
    int xi=COLC;
    for (;xi-->0;x+=TILESIZE,vtx++,src++) {
      vtx->x=x;
      vtx->y=y;
      vtx->tileid=*src;
      vtx->xform=0;
    }
  }
  egg_draw_tile(1,g.texid_tilesheet,vtxv,COLC*ROWC);
}
