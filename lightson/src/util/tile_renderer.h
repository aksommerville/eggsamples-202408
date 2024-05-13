/* tile_renderer.h
 * Coordinates calls to egg_draw_tile().
 * This is by far the most efficient way to get lots of sprites on screen.
 * It's also convenient as a crude means of dumping text.
 */
 
#ifndef TILE_RENDERER_H
#define TILE_RENDERER_H

#include <stdint.h>
#include <egg/egg_video.h>

// This can be anything. Long enough for your most complex scene, that's a good rule of thumb.
#define TILE_RENDERER_CACHE_SIZE 256

struct tile_renderer {
  int dsttexid; // Defaults to 1 if unset. You can change directly.
  int texid;
  int tilesize;
  struct egg_draw_tile tilev[TILE_RENDERER_CACHE_SIZE];
  int tilec;
};

/* All other calls must be fenced by begin..end.
 * You must not do any other rendering between these.
 * At end, we restore global tint and alpha to their default (0,0xff).
 * All tiles in a batch share the tint and alpha.
 * You shouldn't care when the tiles get flushed out, except we guarantee they are flushed after end.
 */
void tile_renderer_begin(struct tile_renderer *tr,int texid,uint32_t tint,uint8_t alpha);
void tile_renderer_end(struct tile_renderer *tr);

/* Add one tile to the list.
 * (x,y) is its center.
 */
void tile_renderer_tile(struct tile_renderer *tr,int16_t x,int16_t y,uint8_t tileid,uint8_t xform);

/* Add multiple tiles in a row, with the first centered at (x,y).
 * (src) must be 8859-1, ie the tilesheet is the first 256 codepoints of Unicode.
 * Or whatever, nothing special here, it's just one byte => one tile.
 */
void tile_renderer_string(struct tile_renderer *tr,int16_t x,int16_t y,const char *src,int srcc);

#endif
