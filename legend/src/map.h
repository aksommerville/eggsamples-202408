/* map.h
 * Resource type describing a screenful of geography.
 */
 
#ifndef MAP_H
#define MAP_H

#define TILESIZE 32
#define COLC 20
#define ROWC 11
#define SCREENW (TILESIZE*COLC)
#define SCREENH (TILESIZE*ROWC)

struct map {
  uint16_t imageid;
  uint16_t songid;
  uint16_t nw,ne,nn,ns; // Neighbor map ids.
  uint8_t v[COLC*ROWC];
};

#endif
