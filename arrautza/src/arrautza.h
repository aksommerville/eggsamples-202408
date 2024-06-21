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
#define SCREENW 640
#define SCREENH 352
#define TILESIZE 16
#define COLC 40
#define ROWC 22

/* Map commands.
 * Beware that the opcode defines its payload length. See etc/doc/map-format.md.
 * Builder, editor, and runtime alike will use these definitions.
 * Multibyte values are big-endian.
 */
#define MAPCMD_hero 0x20 /* u8:x u8:y */
#define MAPCMD_song 0x21 /* u16:rid */
#define MAPCMD_FOR_EACH \
  _(hero) \
  _(song)

#include "general/general.h"
  
extern struct globals {
  int TODO;
} g;
  
#endif
