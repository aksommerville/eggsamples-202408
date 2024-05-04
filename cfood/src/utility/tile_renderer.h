/* tile_renderer.h
 * Wraps egg_draw_tile() with a private cache.
 * You must provide a tilesheet containing 256 square images, arranged 16x16.
 */

#ifndef TILE_RENDERER_H
#define TILE_RENDERER_H

struct tile_renderer;

/* Once created, the tile_renderer has a fixed memory footprint.
 * It's normal to create just one for the game's life and reuse it for all tile ops.
 */
void tile_renderer_del(struct tile_renderer *tr);
struct tile_renderer *tile_renderer_new();

/* We output to texid 1 (main) by default.
 */
void tile_renderer_set_output(struct tile_renderer *tr,int texid);

/* Start a batch of tiles.
 * After begin, you must not do any other rendering until you 'end' or 'cancel'.
 */
void tile_renderer_begin(struct tile_renderer *tr,int texid,int tint,int alpha);

/* Finish a batch.
 * Cancel discards any state that hasn't been flushed yet.
 * Beware that some tiles may already have been rendered.
 */
void tile_renderer_end(struct tile_renderer *tr);
void tile_renderer_cancel(struct tile_renderer *tr);

/* Add tiles to the cache and flush as needed.
 * (x,y) are the center of the tile.
 * (tileid) reads LRTB: 0x00 is the top-left corner, 0x0f the top-right, 0x10 left of the second row, etc.
 * (xform) is a combination of EGG_XFORM_*, zero for natural orientation.
 * Strings are one byte per character. If it's encoded UTF-8, any codepoints above U+7f will produce multiple garbage glyphs.
 * We'll provide better text facilities elsewhere; tiles are not the right choice if you need internationalizable text.
 */
void tile_renderer_tile(struct tile_renderer *tr,int x,int y,int tileid,int xform);
void tile_renderer_string(struct tile_renderer *tr,int x,int y,const char *src,int srcc);
void tile_renderer_string_centered(struct tile_renderer *tr,int x,int y,const char *src,int srcc);

#endif
