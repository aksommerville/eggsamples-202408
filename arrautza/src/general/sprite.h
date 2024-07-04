/* sprite.h
 * Generic sprite API.
 */
 
#ifndef SPRITE_H
#define SPRITE_H

struct sprite;
struct sprctl;
struct sprgrp;
struct sprdef;

struct aabb { double l,r,t,b; };

/* sprite: One element on screen, with associate logic hooks.
 ***********************************************************************/
 
struct sprite {
  const struct sprctl *sprctl; // OPTIONAL
  const struct sprdef *sprdef; // OPTIONAL
  struct sprgrp **grpv;
  int grpc,grpa;
  int refc; // Don't touch.
  double x,y; // Real position in tiles.
  double hbl,hbr,hbu,hbd; // Positive distance to each edge of hitbox, from (x,y). Controller must set if solid.
  double pvx,pvy; // Used by physics. Last known position.
  struct aabb aabb; // Used by physics, please ignore.
  int phconstrain; // Used by physics. Controller may read to see which edges are under pressure. (DIR_N,W,E,S)
  int mapsolids; // Bitfields, (1<<physics), which cells are impassible.
  uint8_t invmass; // 0=immobile, 1=heavy ... 255=light. For reference, let's call a man 128.
  int layer; // Lower layers render first. Within a layer, we sort by bottom edge, top to bottom.
  int bx,by,bw,bh; // Render bounds in screen pixels. Generated by renderer when needed.
  int imageid; // (imageid,tileid,xform) for single-tile sprites with no custom render hook.
  uint8_t tileid;
  uint8_t xform;
  int8_t col,row; // Updated automatically for SPRGRP_FOOTING. Can go one space OOB but no further.
};

/* One does not usually create or destroy sprites directly.
 * Use sprite_spawn() to create them, and let groups delete them.
 * sprite_new() does join SPRGRP_KEEPALIVE and any other groups named by sprctl.
 * But it also returns a STRONG reference. So if you decide against it, you must both kill and del.
 */
void sprite_del(struct sprite *sprite);
int sprite_ref(struct sprite *sprite);
struct sprite *sprite_new(const struct sprctl *sprctl);

/* Create a sprite from a definition and add to the appropriate global groups.
 * Returns a WEAK reference.
 */
struct sprite *sprite_spawn(
  const struct sprdef *sprdef,
  double x,double y,
  const uint8_t *argv,int argc
);

/* Convenience to set (hbl,hbr,hbu,hbd).
 * Centered if (offx,offy) zero.
 */
void sprite_set_hitbox(struct sprite *sprite,double w,double h,double offx,double offy);

/* sprctl: Static definition of a sprite controller.
 * Any sprite with custom logic must have a sprctl.
 * Decorative sprites, or those controlled by some other controller, might not.
 **********************************************************************/
 
struct sprctl {
  const char *name; // For troubleshooting, etc.
  int objlen; // >=sizeof(struct sprite)
  uint32_t grpmask; // Join these groups on construction.
  void (*del)(struct sprite *sprite);
  int (*init)(struct sprite *sprite); // Before sprdef applied.
  int (*ready)(struct sprite *sprite,const uint8_t *argv,int argc); // After sprdef applied.
  
  /* Called every frame if you implement.
   * You can unjoin the UPDATE group to turn it off.
   */
  void (*update)(struct sprite *sprite,double elapsed);
  
  /* Your bounds have been calculated -- render at that position and ignore (sprite->x,y).
   * If you implement this, sprite's (texid,tileid,xform) are not used (you can use them).
   * If you do not implement, the sprite renders as a single tile.
   */
  void (*render)(struct sprite *sprite);
  
  /* Populate (bx,by,bw,bh) based on (x,y) and the arguments.
   * If your output is a single tile centered on (x,y), don't implement this.
   */
  void (*calculate_bounds)(struct sprite *sprite,int tilesize,int addx,int addy);
  
  /* Notification that you've moved to a new grid cell.
   * Only applies in SPRGRP_FOOTING.
   * Before the first update, a sprite's footing is at (-128,-128), which is not possible after.
   */
  void (*footing)(struct sprite *sprite,int8_t pvcol,int8_t pvrow);
  
  /* Notification of a resolved sprite collision, for solid sprites only.
   * (dir) is a cardinal direction, which edge of (sprite) the collision occurred on.
   * We do not report collisions against the map. Examine (phconstrain) if you're interested in those.
   */
  void (*collision)(struct sprite *sprite,struct sprite *other,uint8_t dir);
};

const struct sprctl *sprctl_by_id(int id);

/* sprdef: Resource for a sprite definition.
 * These are the things you'll refer to in a map's spawn points.
 * We implement a cache, so each resource only actually loads once.
 ***************************************************************/
 
struct sprdef {
  int rid;
  const struct sprctl *sprctl;
  uint16_t imageid;
  uint32_t grpmask; // Join these groups, in addition to any declared by sprctl.
  // It's ok to add fields here. Also add them at sprite.c:sprdef_decode_field().
  int binc;
  uint8_t bin[]; // The original verbatim resource, for reading loose fields.
};

const struct sprdef *sprdef_get(int rid);

/* Calls (cb) for each command in (sprdef->bin).
 * (cmdc) is always at least 1, and for most commands is knowable from the first byte.
 * See etc/doc/sprite-format.md.
 */
int sprdef_for_each_command(
  const struct sprdef *sprdef,
  int (*cb)(const uint8_t *cmd,int cmdc,void *userdata),
  void *userdata
);

#define SPRDEF_RES_SIZE_LIMIT 128

/* sprgrp: Mutually-linked group of sprites.
 **************************************************************/
 
#define SPRGRP_MODE_UNIQUE   0 /* Default. Sort by sprite address. */
#define SPRGRP_MODE_RENDER   1 /* Sort incrementally by render order. Can be out of order temporarily. */
#define SPRGRP_MODE_SINGLE   2 /* Adding a sprite evicts any existing one. */
#define SPRGRP_MODE_EXPLICIT 3 /* Preserve order of addition. Not sure this is useful. */
 
struct sprgrp {
  struct sprite **sprv;
  int sprc,spra;
  int refc; // Don't touch.
  int mode;
  int sortdir; // For RENDER mode only. -1,1 to go that direction next time or 0 for a full sort.
};

void sprgrp_del(struct sprgrp *sprgrp);
int sprgrp_ref(struct sprgrp *sprgrp);
struct sprgrp *sprgrp_new(int mode);

int sprgrp_has(const struct sprgrp *sprgrp,const struct sprite *sprite);
int sprgrp_add(struct sprgrp *sprgrp,struct sprite *sprite); // => -1,0,1 = (error,already in,added)
int sprgrp_remove(struct sprgrp *sprgrp,struct sprite *sprite); // ''

#define sprite_in_group(sprite,grpname) sprgrp_has(sprgrpv+SPRGRP_##grpname,sprite)
#define sprite_add_group(sprite,grpname) sprgrp_add(sprgrpv+SPRGRP_##grpname,sprite)
#define sprite_remove_group(sprite,grpname) sprgrp_remove(sprgrpv+SPRGRP_##grpname,sprite)

void sprgrp_clear(struct sprgrp *sprgrp); // Remove all sprites.
void sprite_kill(struct sprite *sprite); // Remove all groups. Usually deletes the sprite too.
void sprgrp_kill(struct sprgrp *sprgrp); // Remove all groups from all sprites in this group.

// A convenience that reads clearer. This is the best way to request deletion of a sprite.
#define sprite_kill_soon(sprite) sprgrp_add(sprgrpv+SPRGRP_DEATHROW,sprite)

/* There are 32 global groups that sprites can join initially.
 */
extern struct sprgrp sprgrpv[32];
#define SPRGRP_KEEPALIVE   0 /* Every sprite is in this group and it doesn't mean anything. */
#define SPRGRP_DEATHROW    1 /* Join to be auto-killed at the end of this update. */
#define SPRGRP_RENDER      2
#define SPRGRP_UPDATE      3
#define SPRGRP_HERO        4 /* Single member. */
#define SPRGRP_FOOTING     5 /* Receive notifications when I move to a new cell. */
#define SPRGRP_SOLID       6 /* Automatically prevent collisions against the grid and other solid sprites. */
#define SPRGRP_FOR_EACH \
  _(KEEPALIVE) \
  _(DEATHROW) \
  _(RENDER) \
  _(UPDATE) \
  _(HERO) \
  _(FOOTING) \
  _(SOLID)

// Special hooks that only main.c should need.
void sprgrpv_init();
void sprgrp_update(struct sprgrp *sprgrp,double elapsed);
void sprgrp_render(struct sprgrp *sprgrp);

/* Our approach to physics is that sprite controllers may move their sprite freely,
 * then it all gets rectified in a batch afterward.
 */
void physics_update(struct sprgrp *sprgrp,double elapsed);
void physics_rebuild_map();

#endif
