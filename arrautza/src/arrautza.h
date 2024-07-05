#ifndef ARRAUTZA_H
#define ARRAUTZA_H
  
#include <egg/egg.h>
#include <stdlib/egg-stdlib.h>
#include <inkeep/inkeep.h>
#include "util/tile_renderer.h"
#include "util/texcache.h"
#include "resid.h"

/* These must remain in sync:
 *  - TILESIZE*COLC==SCREENW
 *  - TILESIZE*ROWC==SCREENH
 *  - SCREENW,SCREENH match metadata:framebuffer.
 *  - TILESIZE matches tilesheet images.
 *  - COLC,ROWC matches maps.
 */
#define SCREENW 320
#define SCREENH 176
#define TILESIZE 16
#define COLC 20
#define ROWC 11

/* Map commands.
 * Beware that the opcode defines its payload length. See etc/doc/map-format.md.
 * Builder, editor, and runtime alike will use these definitions.
 * Multibyte values are big-endian.
 * Block comments after each line are consumed by the editor, see src/editor/js/MagicGlobals.js and .../CommandModal.js.
 * eg "u16:pt", "u32:rgn", "u16:*id"
 */
#define MAPCMD_hero 0x20 /* u16:pt */
#define MAPCMD_song 0x21 /* u16:songid */
#define MAPCMD_image 0x22 /* u16:imageid */
#define MAPCMD_neighborw 0x23 /* u16:mapid */
#define MAPCMD_neighbore 0x24 /* u16:mapid */
#define MAPCMD_neighborn 0x25 /* u16:mapid */
#define MAPCMD_neighbors 0x26 /* u16:mapid */
#define MAPCMD_door 0x80 /* u16:pt u16:mapid u16:dstpt u8:reserved1 u8:reserved2 */
#define MAPCMD_sprite 0x81 /* u16:pt u16:spriteid u8:a u8:b u8:c u8:d */
#define MAPCMD_FOR_EACH \
  _(hero) \
  _(song) \
  _(image) \
  _(neighborw) \
  _(neighbore) \
  _(neighborn) \
  _(neighbors) \
  _(door) \
  _(sprite)

/* Sprite commands.
 * Same idea as map commands (exact same length rules too).
 */
#define SPRITECMD_image     0x20 /* u16:imageid */
#define SPRITECMD_tileid    0x21 /* u8:tileid u8:unused */
#define SPRITECMD_xform     0x22 /* u8:xform u8:unused */
#define SPRITECMD_sprctl    0x23 /* u16:sprctl */
#define SPRITECMD_layer     0x24 /* s8:layer u8:unused */
#define SPRITECMD_invmass   0x25 /* u8:invmass u8:unused */
#define SPRITECMD_groups    0x40 /* u32:grpmask */
#define SPRITECMD_FOR_EACH \
  _(image) \
  _(tileid) \
  _(xform) \
  _(sprctl) \
  _(layer) \
  _(invmass) \
  _(groups)

#include "general/general.h"

/* Declare sprctl implementations here.
 * Follow the pattern exactly: Our editor will read this file too.
 */
extern const struct sprctl sprctl_hero;
extern const struct sprctl sprctl_animate;
extern const struct sprctl sprctl_animate_once;

#define TRANSITION_NONE        0
#define TRANSITION_PAN_LEFT    1 /* Pans are named for the direction of the camera's or hero's movement. */
#define TRANSITION_PAN_RIGHT   2 /* So the content is actually moving opposite the named direction. */
#define TRANSITION_PAN_UP      3 /* (I'm not sure if a convention exists among film people, maybe I've got it backward) */
#define TRANSITION_PAN_DOWN    4 /* ...per Wil, we got it right except they say "truck" and "pedestal", "pan" is rotation. */
#define TRANSITION_DISSOLVE    5 /* One frame directly into the next. */
#define TRANSITION_FADE_BLACK  6 /* A=>Black=>B */
#define TRANSITION_SPOTLIGHT   7 /* Zoop into one focus point, to black, then zoop out from the other focus point. */
  
extern struct globals {
  
  int texid_tilesheet;
  uint16_t imageid_tilesheet;
  uint16_t mapid;
  struct {
    uint16_t mapid;
    int dstx,dsty;
    int transition;
  } mapnext;
  struct map map;
  uint8_t poibits[(COLC*ROWC+7)>>3]; // LRTB cell index big-endianly. Nonzero if something exists at that cell.
  
  int instate;
  
  int texid_transtex; // Contains the outgoing frame during a transition.
  double transclock; // Counts down during transition.
  double transtotal; // Full duration of transition. Constant during a transition.
  int transition;
  int transfx,transfy; // Focus point in old frame, for SPOTLIGHT. (for new frame, we find it dynamically).
  uint32_t transrgba; // For FADE_BLACK and SPOTLIGHT. Alpha must be opaque.
  int renderx,rendery; // Where to put the new frame. Relevant for PAN transitions.
  int texid_spotlight;
  
  struct tile_renderer tile_renderer;
  struct texcache texcache;
} g;

/* These do not load the map immediately.
 * They prepare (g.mapnext), which will be applied when the stack drains.
 * (dstx,dsty) at load_map should be OOB for automatic positioning.
 */
int load_map(uint16_t mapid,int dstx,int dsty,int transition);
int load_neighbor(uint8_t mapcmd);

// main.c calls this at strategic times to commit a map change. Noop if no change pending.
int check_map_change();

void render_map(int dsttexid);
void check_sprites_footing(struct sprgrp *sprgrp);
  
#endif
