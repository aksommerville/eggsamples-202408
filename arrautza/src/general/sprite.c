#include "../arrautza.h"
#include "sprite.h"
#include "map.h" /* We borrow map_command_measure(); map and sprite commands are structured the same way. */

/* Sprite object lifecycle.
 */
 
void sprite_del(struct sprite *sprite) {
  if (!sprite) return;
  if (sprite->refc-->1) return;
  if (sprite->sprctl&&sprite->sprctl->del) sprite->sprctl->del(sprite);
  if (sprite->grpv) {
    if (sprite->grpc) egg_log("ERROR: Deleting sprite %p, grpc==%d. Shouldn't be possible!",sprite,sprite->grpc);
    free(sprite->grpv);
  }
  free(sprite);
}

int sprite_ref(struct sprite *sprite) {
  if (!sprite) return -1;
  if (sprite->refc<1) return -1;
  if (sprite->refc>=INT_MAX) return -1;
  sprite->refc++;
  return 0;
}
    
struct sprite *sprite_new(const struct sprctl *sprctl) {
  struct sprite *sprite=0;
  if (sprctl) {
    sprite=calloc(1,sprctl->objlen);
  } else {
    sprite=calloc(1,sizeof(struct sprite));
  }
  if (!sprite) return 0;
  
  sprite->refc=1;
  sprite->sprctl=sprctl;
  sprite->col=-128;
  sprite->row=-128;
  sprite->hbl=sprite->hbr=sprite->hbu=sprite->hbd=0.5; // Default hitbox is exactly one tile.
  sprite->mapsolids=(
    (1<<MAP_PHYSICS_SOLID)|
    (1<<MAP_PHYSICS_WATER)|
    (1<<MAP_PHYSICS_HOLE)|
  0);
  
  if (sprite_add_group(sprite,KEEPALIVE)<0) {
    sprite_del(sprite);
    return 0;
  }
  if (sprctl->grpmask) {
    int i=32; while (i-->0) {
      if (!(sprctl->grpmask&(1<<i))) continue;
      if (sprgrp_add(sprgrpv+i,sprite)<0) {
        sprite_kill(sprite);
        sprite_del(sprite);
        return 0;
      }
    }
  }
  if (sprctl&&sprctl->init) {
    if (sprctl->init(sprite)<0) {
      sprite_kill(sprite);
      sprite_del(sprite);
      return 0;
    }
  }
  return sprite;
}

/* Set hitbox.
 */
 
void sprite_set_hitbox(struct sprite *sprite,double w,double h,double offx,double offy) {
  sprite->hbl=sprite->hbr=w*0.5;
  sprite->hbu=sprite->hbd=h*0.5;
  sprite->hbl-=offx;
  sprite->hbr+=offx;
  sprite->hbu-=offy;
  sprite->hbd+=offy;
}

/* Apply sprdef, during spawn.
 */
 
static int sprdef_apply_cb(const uint8_t *cmd,int cmdc,void *userdata) {
  struct sprite *sprite=userdata;
  switch (cmd[0]) {
    case SPRITECMD_tileid: sprite->tileid=cmd[1]; break;
    case SPRITECMD_xform: sprite->xform=cmd[1]; break;
    case SPRITECMD_invmass: sprite->invmass=cmd[1]; break;
  }
  return 0;
}
 
static int sprdef_apply(struct sprite *sprite,const uint8_t *argv,int argc) {
  sprite->imageid=sprite->sprdef->imageid;
  if (sprite->sprdef->grpmask) {
    int i=31; while (i-->0) {
      if (sprite->sprdef->grpmask&(1<<i)) {
        if (sprgrp_add(sprgrpv+i,sprite)<0) return -1;
      }
    }
  }
  sprdef_for_each_command(sprite->sprdef,sprdef_apply_cb,sprite);
  return 0;
}

/* Spawn sprite from sprdef.
 */

struct sprite *sprite_spawn(
  const struct sprdef *sprdef,
  double x,double y,
  const uint8_t *argv,int argc
) {
  if (!sprdef) return 0;
  struct sprite *sprite=sprite_new(sprdef->sprctl);
  if (!sprite) return 0;
  sprite->x=sprite->pvx=x;
  sprite->y=sprite->pvy=y;
  sprite->sprdef=sprdef;
  if (sprdef_apply(sprite,argv,argc)<0) {
    sprite_kill(sprite);
    sprite_del(sprite);
    return 0;
  }
  if (sprite->sprctl&&sprite->sprctl->ready) {
    if (sprite->sprctl->ready(sprite,argv,argc)<0) {
      sprite_kill(sprite);
      sprite_del(sprite);
      return 0;
    }
  }
  if (sprite->grpc<1) {
    sprite_del(sprite);
    return 0;
  }
  sprite_del(sprite);
  return sprite;
}

/* Resource cache.
 */
 
static const struct sprdef **sprdefv=0;
static int sprdefc=0,sprdefa=0;

static int sprdefv_search(int rid) {
  int lo=0,hi=sprdefc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct sprdef *q=sprdefv[ck];
         if (rid<q->rid) hi=ck;
    else if (rid>q->rid) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static int sprdefv_insert(int p,const struct sprdef *sprdef) {
  if ((p<0)||(p>sprdefc)||!sprdef) return -1;
  if (p&&(sprdef->rid<=sprdefv[p-1]->rid)) return -1;
  if ((p<sprdefc)&&(sprdef->rid>=sprdefv[p]->rid)) return -1;
  if (sprdefc>=sprdefa) {
    int na=sprdefa+128;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(sprdefv,sizeof(void*)*na);
    if (!nv) return -1;
    sprdefv=nv;
    sprdefa=na;
  }
  memmove(sprdefv+p+1,sprdefv+p,sizeof(void*)*(sprdefc-p));
  sprdefv[p]=sprdef;
  sprdefc++;
  return 0;
}

static int sprdef_decode_field(const uint8_t *cmd,int cmdc,void *userdata) {
  struct sprdef *sprdef=userdata;
  switch (cmd[0]) {
    case SPRITECMD_image: sprdef->imageid=(cmd[1]<<8)|cmd[2]; break;
    case SPRITECMD_sprctl: {
        int id=(cmd[1]<<8)|cmd[2];
        if (!(sprdef->sprctl=sprctl_by_id(id))) {
          egg_log("ERROR: sprctl %d not found, for sprite:%d",id,sprdef->rid);
          return -1;
        }
      } break;
    case SPRITECMD_groups: sprdef->grpmask=(cmd[1]<<24)|(cmd[2]<<16)|(cmd[3]<<8)|cmd[4]; break;
  }
  return 0;
}

struct sprdef *sprdef_decode(const uint8_t *src,int srcc,int rid) {
  int objlen=sizeof(struct sprdef)+srcc;
  struct sprdef *sprdef=malloc(objlen);
  if (!sprdef) return 0;
  memset(sprdef,0,sizeof(struct sprdef));
  memcpy(sprdef->bin,src,srcc);
  sprdef->rid=rid;
  sprdef->binc=srcc;
  if (sprdef_for_each_command(sprdef,sprdef_decode_field,sprdef)<0) {
    free(sprdef);
    return 0;
  }
  return sprdef;
}

const struct sprdef *sprdef_get(int rid) {
  int p=sprdefv_search(rid);
  if (p>=0) return sprdefv[p];
  p=-p-1;
  uint8_t serial[SPRDEF_RES_SIZE_LIMIT];
  int serialc=egg_res_get(serial,sizeof(serial),EGG_RESTYPE_sprite,0,rid);
  if (serialc<1) return 0;
  if (serialc>sizeof(serial)) {
    egg_log("ERROR: sprite:%d is too large (%d>%d)",rid,serialc,SPRDEF_RES_SIZE_LIMIT);
    return 0;
  }
  struct sprdef *sprdef=sprdef_decode(serial,serialc,rid);
  if (!sprdef) return 0;
  if (sprdefv_insert(p,sprdef)<0) {
    free(sprdef);
    return 0;
  }
  return sprdef;
}

/* Iterate sprdef commands.
 */
 
int sprdef_for_each_command(
  const struct sprdef *sprdef,
  int (*cb)(const uint8_t *cmd,int cmdc,void *userdata),
  void *userdata
) {
  int binp=0;
  while (binp<sprdef->binc) {
    // sprite and map commands have the same length rules, and that's not a coincidence.
    int len=map_command_measure(sprdef->bin+binp,sprdef->binc-binp);
    if (len<1) break;
    int err=cb(sprdef->bin+binp,len,userdata);
    if (err) return err;
    binp+=len;
  }
  return 0;
}

/* Group lifecycle.
 */
 
void sprgrp_del(struct sprgrp *sprgrp) {
  if (!sprgrp) return;
  if (!sprgrp->refc) return; // Static immortal group.
  if (sprgrp->refc-->1) return;
  if (sprgrp->sprv) {
    if (sprgrp->sprc) egg_log("ERROR: Deleting group %p, sprc==%d. Must be zero at this point.",sprgrp,sprgrp->sprc);
    free(sprgrp->sprv);
  }
  free(sprgrp);
}

int sprgrp_ref(struct sprgrp *sprgrp) {
  if (!sprgrp) return -1;
  if (sprgrp->refc<1) return 0; // Static immortal group.
  if (sprgrp->refc>=INT_MAX) return -1;
  sprgrp->refc++;
  return 0;
}

struct sprgrp *sprgrp_new(int mode) {
  struct sprgrp *sprgrp=calloc(1,sizeof(struct sprgrp));
  if (!sprgrp) return 0;
  sprgrp->refc=1;
  sprgrp->mode=mode;
  return sprgrp;
}

/* Search group and sprite lists.
 */
 
static int sprite_grpv_search(const struct sprite *sprite,const struct sprgrp *sprgrp) {
  int lo=0,hi=sprite->grpc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct sprgrp *q=sprite->grpv[ck];
         if (sprgrp<q) hi=ck;
    else if (sprgrp>q) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static int sprgrp_sprv_search(const struct sprgrp *sprgrp,const struct sprite *sprite) {
  switch (sprgrp->mode) {
    
    case SPRGRP_MODE_UNIQUE: {
        int lo=0,hi=sprgrp->sprc;
        while (lo<hi) {
          int ck=(lo+hi)>>1;
          const struct sprite *q=sprgrp->sprv[ck];
               if (sprite<q) hi=ck;
          else if (sprite>q) lo=ck+1;
          else return ck;
        }
        return -lo-1;
      }
      
    case SPRGRP_MODE_SINGLE: {
        if (sprgrp->sprc<1) return -1;
        if (sprgrp->sprv[0]==sprite) return 0;
        return -1;
      }
      
    case SPRGRP_MODE_RENDER:
    case SPRGRP_MODE_EXPLICIT:
    default: {
        int i=sprgrp->sprc;
        while (i-->0) {
          if (sprgrp->sprv[i]==sprite) return i;
        }
        // For RENDER, it's tempting to search for an appropriate insertion point, but pointless.
        // We're going to do a full sort before the next render.
        return -sprgrp->sprc-1;
      }
  }
}

/* Group membership primitives, internal.
 * Use carefully! We only do the stated operation, and update reference counts.
 * You must ensure that all sprite<~>sprgrp links are mutual.
 */
 
static int sprite_grpv_insert(struct sprite *sprite,int p,struct sprgrp *sprgrp) {
  if ((p<0)||(p>sprite->grpc)) return -1;
  if (sprite->grpc>=sprite->grpa) {
    int na=sprite->grpa+8;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(sprite->grpv,sizeof(void*)*na);
    if (!nv) return -1;
    sprite->grpv=nv;
    sprite->grpa=na;
  }
  if (sprgrp_ref(sprgrp)<0) return -1;
  memmove(sprite->grpv+p+1,sprite->grpv+p,sizeof(void*)*(sprite->grpc-p));
  sprite->grpv[p]=sprgrp;
  sprite->grpc++;
  return 0;
}

static int sprgrp_sprv_insert(struct sprgrp *sprgrp,int p,struct sprite *sprite) {
  if ((p<0)||(p>sprgrp->sprc)) return -1;
  if (sprgrp->sprc>=sprgrp->spra) {
    int na=sprgrp->spra+16;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(sprgrp->sprv,sizeof(void*)*na);
    if (!nv) return -1;
    sprgrp->sprv=nv;
    sprgrp->spra=na;
  }
  if (sprite_ref(sprite)<0) return -1;
  memmove(sprgrp->sprv+p+1,sprgrp->sprv+p,sizeof(void*)*(sprgrp->sprc-p));
  sprgrp->sprv[p]=sprite;
  sprgrp->sprc++;
  return 0;
}

static void sprite_grpv_remove(struct sprite *sprite,int p) {
  if ((p<0)||(p>=sprite->grpc)) return;
  struct sprgrp *sprgrp=sprite->grpv[p];
  sprite->grpc--;
  memmove(sprite->grpv+p,sprite->grpv+p+1,sizeof(void*)*(sprite->grpc-p));
  sprgrp_del(sprgrp);
}

static void sprgrp_sprv_remove(struct sprgrp *sprgrp,int p) {
  if ((p<0)||(p>=sprgrp->sprc)) return;
  struct sprite *sprite=sprgrp->sprv[p];
  sprgrp->sprc--;
  memmove(sprgrp->sprv+p,sprgrp->sprv+p+1,sizeof(void*)*(sprgrp->sprc-p));
  sprite_del(sprite);
}

/* Group membership primitives, public interface.
 */

int sprgrp_has(const struct sprgrp *sprgrp,const struct sprite *sprite) {
  if (!sprgrp||!sprite) return 0;
  // A sprite's group list is always sorted by address, and usually smaller than groups' sprite lists.
  return ((sprite_grpv_search(sprite,sprgrp)>=0)?1:0);
}

int sprgrp_add(struct sprgrp *sprgrp,struct sprite *sprite) {
  if (!sprgrp||!sprite) return -1;
  int grpp=sprite_grpv_search(sprite,sprgrp);
  if (grpp>=0) return 0; // Assume it's listed in the group too, if it's listed in the sprite.
  grpp=-grpp-1;
  if (sprite_grpv_insert(sprite,grpp,sprgrp)<0) return -1;
  int sprp=sprgrp_sprv_search(sprgrp,sprite);
  if (sprp<0) {
    sprp=-sprp-1;
    if (sprgrp_sprv_insert(sprgrp,sprp,sprite)<0) {
      sprite_grpv_remove(sprite,grpp); // Remove the group we just inserted in sprite.
      return -1;
    }
  }
  // For RENDER mode, force a full sort after adding anything.
  switch (sprgrp->mode) {
    case SPRGRP_MODE_RENDER: sprgrp->sortdir=0; break;
  }
  return 1;
}

int sprgrp_remove(struct sprgrp *sprgrp,struct sprite *sprite) {
  if (!sprgrp||!sprite) return 0;
  int grpp=sprite_grpv_search(sprite,sprgrp);
  if (grpp<0) return 0; // Assume sprite is not listed in group too.
  sprite_grpv_remove(sprite,grpp);
  int sprp=sprgrp_sprv_search(sprgrp,sprite);
  if (sprp>=0) sprgrp_sprv_remove(sprgrp,sprp);
  return 1;
}

/* Bulk removal.
 */
 
void sprgrp_clear(struct sprgrp *sprgrp) {
  if (!sprgrp->sprc) return;
  if (sprgrp_ref(sprgrp)<0) return;
  while (sprgrp->sprc>0) {
    sprgrp->sprc--;
    struct sprite *sprite=sprgrp->sprv[sprgrp->sprc];
    int grpp=sprite_grpv_search(sprite,sprgrp);
    if (grpp>=0) sprite_grpv_remove(sprite,grpp);
    sprite_del(sprite);
  }
  sprgrp_del(sprgrp);
}

void sprite_kill(struct sprite *sprite) {
  if (!sprite->grpc) return;
  if (sprite_ref(sprite)<0) return;
  while (sprite->grpc>0) {
    sprite->grpc--;
    struct sprgrp *sprgrp=sprite->grpv[sprite->grpc];
    int sprp=sprgrp_sprv_search(sprgrp,sprite);
    if (sprp>=0) sprgrp_sprv_remove(sprgrp,sprp);
    sprgrp_del(sprgrp);
  }
  sprite_del(sprite);
}

void sprgrp_kill(struct sprgrp *sprgrp) {
  if (!sprgrp->sprc) return;
  if (sprgrp_ref(sprgrp)<0) return;
  while (sprgrp->sprc>0) {
    sprgrp->sprc--;
    struct sprite *sprite=sprgrp->sprv[sprgrp->sprc];
    sprite_kill(sprite);
    sprite_del(sprite); // Our reference, that we "sprc--"'d away already.
  }
  sprgrp_del(sprgrp);
}

/* Global groups list.
 */

struct sprgrp sprgrpv[32]={0};

void sprgrpv_init() {
  sprgrpv[SPRGRP_RENDER].mode=SPRGRP_MODE_RENDER;
  sprgrpv[SPRGRP_HERO].mode=SPRGRP_MODE_SINGLE;
}

/* Update all sprites.
 */

void sprgrp_update(struct sprgrp *sprgrp,double elapsed) {
  int i=sprgrp->sprc;
  while (i-->0) {
    struct sprite *sprite=sprgrp->sprv[i];
    if (!sprite->sprctl) continue;
    if (!sprite->sprctl->update) continue;
    sprite->sprctl->update(sprite,elapsed);
  }
}

/* Sort group in preparation of rendering.
 */
 
static int sprite_rendercmp(const struct sprite *a,const struct sprite *b) {
  if (a->layer<b->layer) return -1;
  if (a->layer>b->layer) return 1;
  if (a->y<b->y) return -1;
  if (a->y>b->y) return 1;
  return 0;
}
 
static void sprgrp_render_qsort(struct sprite **v,int c) {
  if (c<2) return;
  int eqp=c>>1,eqc=1,i;
  struct sprite *ref=v[eqp];
  for (i=eqp;i-->0;) {
    struct sprite *q=v[i];
    int cmp=sprite_rendercmp(q,ref);
    if (cmp>0) {
      memmove(v+i,v+i+1,sizeof(void*)*(eqp+eqc-i-1));
      eqp--;
      v[eqp+eqc]=q;
    } else if (!cmp) {
      if (i<eqp-1) {
        memmove(v+i,v+i+1,sizeof(void*)*(eqp-i-1));
        eqp--;
        eqc++;
        v[eqp]=q;
      } else {
        eqp--;
        eqc++;
      }
    }
  }
  for (i=eqp+eqc;i<c;i++) {
    struct sprite *q=v[i];
    int cmp=sprite_rendercmp(q,ref);
    if (cmp<0) {
      memmove(v+eqp+1,v+eqp,sizeof(void*)*(i-eqp));
      v[eqp]=q;
      eqp++;
    } else if (!cmp) {
      if (i>eqp+eqc) {
        memmove(v+eqp+eqc+1,v+eqp+eqc,sizeof(void*)*(i-eqc-eqp));
        v[eqp+eqc]=q;
        eqc++;
      } else {
        eqc++;
      }
    }
  }
  sprgrp_render_qsort(v,eqp);
  sprgrp_render_qsort(v+eqp+eqc,c-eqc-eqp);
}
 
static void sprgrp_render_sort(struct sprgrp *sprgrp) {
  if (sprgrp->sprc<2) return;
  if (!sprgrp->sortdir) {
    sprgrp_render_qsort(sprgrp->sprv,sprgrp->sprc);
    sprgrp->sortdir=1;
  } else {
    int first,last,d,i;
    if (sprgrp->sortdir>0) {
      first=0;
      last=sprgrp->sprc-1;
      d=1;
      sprgrp->sortdir=-1;
    } else {
      first=sprgrp->sprc-1;
      last=0;
      d=-1;
      sprgrp->sortdir=1;
    }
    for (i=first;i!=last;i+=d) {
      struct sprite *a=sprgrp->sprv[i];
      struct sprite *b=sprgrp->sprv[i+d];
      if (sprite_rendercmp(a,b)==d) {
        sprgrp->sprv[i]=b;
        sprgrp->sprv[i+d]=a;
      }
    }
  }
}

/* Render all sprites.
 */

void sprgrp_render(struct sprgrp *sprgrp) {
  int i;

  // Calculate the output position and coverage for each sprite.
  for (i=sprgrp->sprc;i-->0;) {
    struct sprite *sprite=sprgrp->sprv[i];
    if (sprite->sprctl&&sprite->sprctl->calculate_bounds) {
      sprite->sprctl->calculate_bounds(sprite,TILESIZE,0,0);
    } else {
      int dstx=(int)(sprite->x*TILESIZE);
      int dsty=(int)(sprite->y*TILESIZE);
      sprite->bw=TILESIZE;
      sprite->bh=TILESIZE;
      sprite->bx=dstx-(TILESIZE>>1);
      sprite->by=dsty-(TILESIZE>>1);
    }
  }
  
  // Advance the sort.
  sprgrp_render_sort(sprgrp);
  
  // Render all in order.
  int imageid=0;
  #define FLUSH_TILES if (imageid) { \
    tile_renderer_end(&g.tile_renderer); \
    imageid=0; \
  }
  for (i=0;i<sprgrp->sprc;i++) {
    struct sprite *sprite=sprgrp->sprv[i];
    if (sprite->sprctl&&sprite->sprctl->render) {
      FLUSH_TILES
      sprite->sprctl->render(sprite);
    } else if (!sprite->imageid) {
      // Skip it.
    } else {
      if (sprite->imageid!=imageid) {
        FLUSH_TILES
        int texid=texcache_get(&g.texcache,sprite->imageid);
        if (texid<1) continue;
        imageid=sprite->imageid;
        tile_renderer_begin(&g.tile_renderer,texid,0,0xff);
      }
      int dstx=sprite->bx+(TILESIZE>>1);
      int dsty=sprite->by+(TILESIZE>>1);
      tile_renderer_tile(&g.tile_renderer,dstx,dsty,sprite->tileid,sprite->xform);
    }
  }
  FLUSH_TILES
  #undef FLUSH_TILES
  
  // Debug: highlight hitbox of physics sprites.
  if (0) {
    struct sprite **p=sprgrpv[SPRGRP_SOLID].sprv;
    for (i=sprgrpv[SPRGRP_SOLID].sprc;i-->0;p++) {
      struct sprite *sprite=*p;
      int x=(sprite->x-sprite->hbl)*TILESIZE;
      int y=(sprite->y-sprite->hbu)*TILESIZE;
      int w=(sprite->hbl+sprite->hbr)*TILESIZE;
      int h=(sprite->hbu+sprite->hbd)*TILESIZE;
      egg_draw_rect(1,x,y,w,h,0xff000080);
    }
  }
}
