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
#define MAPCMD_FOR_EACH \
  _(hero) \
  _(song) \
  _(image) \
  _(neighborw) \
  _(neighbore) \
  _(neighborn) \
  _(neighbors) \
  _(door)

/* Sprite commands.
 * Same idea as map commands (exact same length rules too).
 */
#define SPRITECMD_image     0x20 /* u16:imageid */
#define SPRITECMD_tileid    0x21 /* u8:tileid u8:unused */
#define SPRITECMD_xform     0x22 /* u8:tileid u8:unused */
#define SPRITECMD_sprctl    0x23 /* u16:sprctl */
#define SPRITECMD_groups    0x40 /* u32:grpmask */
#define SPRITECMD_FOR_EACH \
  _(image) \
  _(tileid) \
  _(xform) \
  _(sprctl) \
  _(groups)

#include "general/general.h"

/* Declare sprctl implementations here.
 * Follow the pattern exactly: Our editor will read this file too.
 */
extern const struct sprctl sprctl_hero;
extern const struct sprctl sprctl_animate;
extern const struct sprctl sprctl_animate_once;
  
extern struct globals {
  int texid_tilesheet;
  uint16_t imageid_tilesheet;
  uint16_t mapid;
  struct map map;
  int instate;
  struct tile_renderer tile_renderer;
  struct texcache texcache;
} g;

int load_map(uint16_t mapid);
int load_neighbor(uint8_t mapcmd);
void render_map();
void check_sprites_footing(struct sprgrp *sprgrp);
  
#endif
