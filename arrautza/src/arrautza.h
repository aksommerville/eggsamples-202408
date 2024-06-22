#ifndef ARRAUTZA_H
#define ARRAUTZA_H
  
#include <egg/egg.h>
#include <stdlib/egg-stdlib.h>
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
#define MAPCMD_FOR_EACH \
  _(hero) \
  _(song) \
  _(image)

#include "general/general.h"
  
extern struct globals {
  int TODO;
} g;
  
#endif
