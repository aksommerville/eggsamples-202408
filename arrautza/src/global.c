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
 
struct load_map_context {
  double herox,heroy;
};
 
static int load_map_cb(const uint8_t *cmd,int cmdc,void *userdata) {
  struct load_map_context *ctx=userdata;
  switch (cmd[0]) {
    case MAPCMD_hero: ctx->herox=cmd[1]+0.5; ctx->heroy=cmd[2]+0.5; break;
    case MAPCMD_song: egg_audio_play_song(0,(cmd[1]<<8)|cmd[2],0,1); break;
    case MAPCMD_image: return load_map_image((cmd[1]<<8)|cmd[2]);
    //TODO neighbor[nswe], door
  }
  return 0;
}

/* Load map.
 */
 
int load_map(uint16_t mapid) {

  // Capture the hero sprite if there is one, then kill them all.
  //TODO Are we doing multiplayer? That changes things a lot. I'm assuming singleplayer for now.
  struct sprite *hero=0;
  if (sprgrpv[SPRGRP_HERO].sprc>=1) {
    hero=sprgrpv[SPRGRP_HERO].sprv[0];
    if (sprite_ref(hero)<0) return -1;
    sprgrp_remove(sprgrpv+SPRGRP_KEEPALIVE,hero); // Don't drop his groups!
  }
  sprgrp_kill(sprgrpv+SPRGRP_KEEPALIVE);
  
  // Acquire the map and run its commands.
  g.mapid=mapid;
  int c=egg_res_get(&g.map,sizeof(g.map),EGG_RESTYPE_map,0,mapid);
  if ((c<COLC*ROWC)||(c>sizeof(struct map))) {
    egg_log("Invalid size %d for map:%d. Must be in %d..%d.",c,mapid,COLC*ROWC,(int)sizeof(struct map));
    sprite_del(hero);
    return -1;
  }
  struct load_map_context ctx={
    .herox=COLC*0.5,
    .heroy=ROWC*0.5,
  };
  if (map_for_each_command(&g.map,load_map_cb,&ctx)<0) {
    sprite_del(hero);
    return -1;
  }
  
  // Create the hero if we don't have one yet, and position it as needed.
  // The hero sprite instance stays alive across map loads.
  if (!hero) {
    if (!(hero=sprite_new(&sprctl_hero))) return -1;
    hero->x=ctx.herox;
    hero->y=ctx.heroy;
  } else {
    if (sprgrp_add(sprgrpv+SPRGRP_KEEPALIVE,hero)<0) return -1;
    //TODO If we entered through a door, position at its exit.
    // Normally on a map change, the hero is OOB in one direction. Find that direction and slide him one screenful.
    if (hero->x<0.0) hero->x+=COLC;
    else if (hero->y<0.0) hero->y+=ROWC;
    else if (hero->x>=COLC) hero->x-=COLC;
    else if (hero->y>=ROWC) hero->y-=ROWC;
  }
  
  return 0;
}

/* Load a neighbor map if it exists.
 */
 
int load_neighbor(uint8_t mapcmd) {
  int mapid=map_get_command(&g.map,mapcmd);
  if (mapid<1) return -1;
  return load_map(mapid);//XXX Defer the actual load until the stack drains.
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

/* Update footing for sprites in group.
 */
 
void check_sprites_footing(struct sprgrp *sprgrp) {
  int i=0; for (;i<sprgrp->sprc;i++) {
    struct sprite *sprite=sprgrp->sprv[i];
    int8_t col,row;
    if (sprite->x<0.0) col=-1; else if (sprite->x>=COLC) col=COLC; else col=(int8_t)sprite->x;
    if (sprite->y<0.0) row=-1; else if (sprite->y>=ROWC) row=ROWC; else row=(int8_t)sprite->y;
    if ((col==sprite->col)&&(row==sprite->row)) continue;
    int8_t pvcol=sprite->col; sprite->col=col;
    int8_t pvrow=sprite->row; sprite->row=row;
    if (sprite->sprctl&&sprite->sprctl->footing) {
      sprite->sprctl->footing(sprite,pvcol,pvrow);
    }
  }
}
