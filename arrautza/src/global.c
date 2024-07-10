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

/* Spawn sprite in new map.
 */
 
static int load_map_sprite(const uint8_t *cmd) {
  double x=cmd[1]+0.5;
  double y=cmd[2]+0.5;
  int rid=(cmd[3]<<8)|cmd[4];
  const struct sprdef *sprdef=sprdef_get(rid);
  if (!sprdef) {
    egg_log("ERROR: sprite:%d not found, required by map:%d",rid,g.mapid);
    return -1; // Doesn't have to be fatal.
  }
  struct sprite *sprite=sprite_spawn(sprdef,x,y,cmd+5,4);
  if (!sprite) return 0; // Sprites are allowed to self-delete during spawn. Real errors hopefully are rare.
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
    
    // Call out commands that need some action taken at load.
    // Other things we can ignore. One can reread the commands at any time, it's cheap.
    case MAPCMD_hero: ctx->herox=cmd[1]+0.5; ctx->heroy=cmd[2]+0.5; break;
    case MAPCMD_song: egg_audio_play_song(0,(cmd[1]<<8)|cmd[2],0,1); break;
    case MAPCMD_image: return load_map_image((cmd[1]<<8)|cmd[2]);
    case MAPCMD_sprite: return load_map_sprite(cmd);
    case MAPCMD_ucoord: g.ucoordx=(cmd[1]<<8)|cmd[2]; g.ucoordy=(cmd[3]<<8)|cmd[4]; break;
    
    // All commands that begin with a position and need recorded in poibits.
    // We're just marking the cell as "interesting", so when the hero steps here
    // he can very quickly know whether to read all the commands again.
    case MAPCMD_door:
      {
        uint8_t col=cmd[1];
        uint8_t row=cmd[2];
        if ((col<COLC)&&(row<ROWC)) {
          int p=row*COLC+col;
          g.poibits[p>>3]|=0x80>>(p&7);
        }
      } break;
  }
  return 0;
}

/* Should we remove hero, when rendering the "from" frame for this transition?
 * I think only the PANs will want yes.
 */
 
static int transition_should_hide_hero(int transition) {
  switch (transition) {
    case TRANSITION_PAN_LEFT:
    case TRANSITION_PAN_RIGHT:
    case TRANSITION_PAN_UP:
    case TRANSITION_PAN_DOWN:
      return 1;
  }
  return 0;
}

/* Apply navigation immediately. Private: Must be reached via check_map_change().
 */
 
static int apply_map_change(uint16_t mapid,int dstx,int dsty,int transition) {
  
  // Capture the hero sprite if there is one.
  //TODO Are we doing multiplayer? That changes things a lot. I'm assuming singleplayer for now.
  struct sprite *hero=0;
  if (sprgrpv[SPRGRP_HERO].sprc>=1) {
    hero=sprgrpv[SPRGRP_HERO].sprv[0];
    if (sprite_ref(hero)<0) return -1;
    sprgrp_remove(sprgrpv+SPRGRP_KEEPALIVE,hero); // Don't drop his groups!
  }
  
  // If a transition was requested, redraw the scene into a temporary texture.
  if (transition) {
    g.renderx=g.rendery=0;
    render_map(g.texid_transtex);
    if (transition_should_hide_hero(transition)) {
      sprgrp_remove(sprgrpv+SPRGRP_RENDER,hero);
    }
    sprgrp_render(g.texid_transtex,sprgrpv+SPRGRP_RENDER);
    sprgrp_add(sprgrpv+SPRGRP_RENDER,hero);
    
    /* Transition time and intermediate color (FADE_BLACK and SPOTLIGHT) are hard-coded here.
     * You can change them universally right here, or figure out a more nuanced decision process if you want them variable.
     * (transrgba) of black is an obvious choice, but I dislike it because my maps sometimes have a lot of black at the edges (think interiors).
     * ...actually, with the time short enough, I think black is OK.
     * Keep the time short. Bear in mind that play continues uninterrupted, even during transitions.
     */
    g.transrgba=0x000000ff;
    g.transclock=g.transtotal=0.500;
    
    g.transition=transition;
    if (hero) {
      g.transfx=(int)(hero->x*TILESIZE);
      g.transfy=(int)(hero->y*TILESIZE);
    } else { 
      g.transfx=SCREENW>>1;
      g.transfy=SCREENH>>1;
    }
  } else {
    g.transclock=0.0;
  }

  // Drop all the sprites. We're holding a STRONG reference to (hero), so it's not affected.
  sprgrp_kill(sprgrpv+SPRGRP_KEEPALIVE);
  
  // Acquire the map and run its commands.
  g.mapnext.mapid=0;
  g.mapid=mapid;
  memset(g.poibits,0,sizeof(g.poibits));
  int c=egg_res_get(&g.map,sizeof(g.map),EGG_RESTYPE_map,0,mapid);
  if ((c<COLC*ROWC)||(c>sizeof(struct map))) {
    egg_log("Invalid size %d for map:%d. Must be in %d..%d.",c,mapid,COLC*ROWC,(int)sizeof(struct map));
    sprite_del(hero);
    return -1;
  }
  memset(((char*)&g.map)+c,0,sizeof(struct map)-c);
  struct load_map_context ctx={
    .herox=COLC*0.5,
    .heroy=ROWC*0.5,
  };
  if (map_for_each_command(&g.map,load_map_cb,&ctx)<0) {
    sprite_del(hero);
    return -1;
  }
  
  // Let physics rebuild its view of static walls.
  physics_rebuild_map();
  
  // Create the hero if we don't have one yet, and position it as needed.
  // The hero sprite instance stays alive across map loads.
  if (!hero) {
    if (!(hero=sprite_new(&sprctl_hero))) return -1;
    hero->x=ctx.herox;
    hero->y=ctx.heroy;
  } else {
    if (sprgrp_add(sprgrpv+SPRGRP_KEEPALIVE,hero)<0) return -1;
    // If they gave us a destination point, apply it. (for doors)
    if ((dstx>=0)&&(dsty>=0)&&(dstx<COLC)&&(dsty<ROWC)) {
      hero->x=dstx+0.5;
      hero->y=dsty+0.5;
    } else {
      // Normally on a map change, the hero is OOB in one direction. Find that direction and slide him one screenful.
      if (hero->x<0.0) hero->x+=COLC;
      else if (hero->y<0.0) hero->y+=ROWC;
      else if (hero->x>=COLC) hero->x-=COLC;
      else if (hero->y>=ROWC) hero->y-=ROWC;
    }
  }
  sprite_warped(hero);
  
  //egg_log("loaded map %d, ucoord=%d,%d",g.mapid,g.ucoordx,g.ucoordy);
  
  return 0;
}

/* Check for navigation and apply if requested.
 */
 
int check_map_change() {
  if (!g.mapnext.mapid) return 0;
  return apply_map_change(g.mapnext.mapid,g.mapnext.dstx,g.mapnext.dsty,g.mapnext.transition);
}

/* Load map.
 */
 
int load_map(uint16_t mapid,int dstx,int dsty,int transition) {
  if (egg_res_get(0,0,EGG_RESTYPE_map,0,mapid)<1) return -1; // Fail immediately if it doesn't exist; don't defer.
  g.mapnext.mapid=mapid;
  g.mapnext.dstx=dstx;
  g.mapnext.dsty=dsty;
  g.mapnext.transition=transition;
  return 0;
}

/* Load a neighbor map if it exists.
 */
 
int load_neighbor(uint8_t mapcmd) {
  int mapid=map_get_command(&g.map,mapcmd);
  if (mapid<1) return -1;
  int transition=TRANSITION_NONE;
  switch (mapcmd) {
    case MAPCMD_neighborw: g.ucoordx--; transition=TRANSITION_PAN_LEFT; break;
    case MAPCMD_neighbore: g.ucoordx++; transition=TRANSITION_PAN_RIGHT; break;
    case MAPCMD_neighborn: g.ucoordy--; transition=TRANSITION_PAN_UP; break;
    case MAPCMD_neighbors: g.ucoordy++; transition=TRANSITION_PAN_DOWN; break;
  }
  return load_map(mapid,-1,-1,transition);
}

/* Render map.
 */
 
void render_map(int dsttexid) {
  struct egg_draw_tile vtxv[COLC*ROWC];
  struct egg_draw_tile *vtx=vtxv;
  int y=(TILESIZE>>1)+g.rendery;
  int yi=ROWC;
  const uint8_t *src=g.map.v;
  for (;yi-->0;y+=TILESIZE) {
    int x=(TILESIZE>>1)+g.renderx;
    int xi=COLC;
    for (;xi-->0;x+=TILESIZE,vtx++,src++) {
      vtx->x=x;
      vtx->y=y;
      vtx->tileid=*src;
      vtx->xform=0;
    }
  }
  egg_draw_tile(dsttexid,g.texid_tilesheet,vtxv,COLC*ROWC);
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

/* Send notifications of hero overlap to observers.
 */
 
void check_sprites_heronotify(struct sprgrp *observers,struct sprgrp *heroes) {
  if ((observers->sprc<1)||(heroes->sprc<1)) return;
  int hi=heroes->sprc;
  while (hi-->0) physics_refresh_aabb(heroes->sprv[hi]);
  int oi=observers->sprc;
  while (oi-->0) {
    struct sprite *observer=observers->sprv[oi];
    if (!observer->sprctl||!observer->sprctl->heronotify) continue; // why did you join this group...
    physics_refresh_aabb(observer);
    hi=heroes->sprc;
    while (hi-->0) {
      struct sprite *hero=heroes->sprv[hi];
      if (observer->aabb.l>=hero->aabb.r) continue;
      if (observer->aabb.r<=hero->aabb.l) continue;
      if (observer->aabb.t>=hero->aabb.b) continue;
      if (observer->aabb.b<=hero->aabb.t) continue;
      observer->sprctl->heronotify(observer,hero);
    }
  }
}

/* Test possession of item.
 * We don't have a straightforward record of this.
 * Need to examine inventory and assigned items.
 */
 
int item_possessed(int itemid) {
  if ((itemid<1)||(itemid>ITEM_COUNT)) return 0;
  if (itemid==g.aitem) return 1;
  if (itemid==g.bitem) return 1;
  int i=INVENTORY_SIZE;
  const uint8_t *inv=g.inventory;
  for (;i-->0;inv++) if (*inv==itemid) return 1;
  return 0;
}

/* Acquire item.
 * Store in (aitem) or (bitem) if they're vacant. Otherwise find a vacant slot in (inventory).
 */
 
void acquire_item(int itemid,int count) {
  if ((itemid<1)||(itemid>ITEM_COUNT)) return;
  int invp=-1;
  if (itemid==g.aitem) invp=1000;
  else if (itemid==g.bitem) invp=1001;
  else {
    int i=INVENTORY_SIZE;
    const uint8_t *inv=g.inventory+i-1;
    for (;i-->0;inv--) {
      if (*inv==itemid) { invp=-1; break; }
      if (!*inv) invp=i;
    }
  }
  const struct item_metadata *metadata=item_metadata+itemid;
  if ((metadata->qual_display==QUAL_DISPLAY_COUNT)&&(count>0)) {
    int nc=(int)(g.itemqual[itemid])+count;
    if (nc<0) nc=0; else if (nc>metadata->qual_limit) nc=metadata->qual_limit;
    g.itemqual[itemid]=nc;
  }
  if (invp>=1000) ;
  else if (!g.aitem) g.aitem=itemid;
  else if (!g.bitem) g.bitem=itemid;
  else if (invp>=0) g.inventory[invp]=itemid;
}

/* Update the compass's rotation.
 */

void update_compass(double elapsed) {
  /* Constants, highly tweakable.
   * (rate_near) is how fast we spin when you're right on the target, in radians/sec.
   * (rate_far) is when we're out of range. All other speeds are calculated as a proportion of this.
   * (distance_far) is the distance in meters where we stop providing information at all.
   * Mind that as you approach (distance_far), the output is increasingly difficult for a human to grok.
   */
  const double rate_near=15.0;
  const double rate_far=10.0;
  const double distance_far=COLC*8.0;

  // TODO Determine compass target location.
  int16_t tucx=2,tucy=-2;
  uint8_t tcol=11,trow=7;
  double hx,hy;
  if (sprgrpv[SPRGRP_HERO].sprc>=1) {
    struct sprite *hero=sprgrpv[SPRGRP_HERO].sprv[0];
    hx=hero->x;
    hy=hero->y;
  } else {
    hx=COLC*0.5;
    hy=ROWC*0.5;
  }
  double dx=(tucx-g.ucoordx)*COLC+tcol+0.5-hx;
  double dy=(tucy-g.ucoordy)*ROWC+trow+0.5-hy;
  
  // Within half a tile of the target, we rotate at a fixed fast rate (rate_near).
  if ((dx>-0.10)&&(dx<0.10)&&(dy>-0.10)&&(dy<0.10)) {
    g.compassangle+=rate_near*elapsed;
    if (g.compassangle>M_PI) g.compassangle-=M_PI*2.0;
    return;
  }
  double distance=sqrt(dx*dx+dy*dy);
  if (distance<=0.5) {
    g.compassangle+=rate_near*elapsed;
    if (g.compassangle>M_PI) g.compassangle-=M_PI*2.0;
    return;
  }
  
  // If distance is beyond the "far" threshold, we don't care where the target is.
  if (distance>distance_far) {
    g.compassangle+=rate_far*elapsed;
    if (g.compassangle>M_PI) g.compassangle-=M_PI*2.0;
    return;
  }
  
  // Determine the target angle and our current distance from that.
  double t=atan2(dx,-dy);
  double tdistance=t-g.compassangle;
  while (tdistance<0.0) tdistance+=M_PI*2.0;
  while (tdistance>M_PI*2.0) tdistance-=M_PI*2.0;
  if (tdistance>=M_PI) tdistance=M_PI*2.0-tdistance;
  tdistance/=M_PI;
  
  // Rotation speed will scale between two values, with the lower velocity decreasing with distance.
  // Top speed is constant (rate_far).
  double rate_low=(rate_far*distance)/distance_far;
  double rate=rate_low+(rate_far-rate_low)*tdistance;
  g.compassangle+=rate*elapsed;
  if (g.compassangle>M_PI) g.compassangle-=M_PI*2.0;
}
