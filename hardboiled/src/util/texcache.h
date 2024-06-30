/* texcache.h
 * Coordinates loading images as textures.
 * Useful if your render regime is abstract and allows sourcing from multiple images in the same scene.
 * Test your game thoroughly with logging enabled and ensure that you see eviction only at scene changes.
 * If you get spammed with evictions, increase the cache size.
 */
 
#ifndef TEXCACHE_H
#define TEXCACHE_H

/* The platform surely has some texture limits, but we don't know what they are.
 * So keep this as small as possible.
 * And remember, you're probably loading other non-cache textures, account for those too.
 * The default of 4 is deliberately low; I'd expect it to be OK up to 10 or so.
 */
#define TEXCACHE_SIZE 4

// You definitely don't want this in the release build. Occasional evictions are perfectly normal.
#define TEXCACHE_LOG_EVICTIONS 1

struct texcache {
  struct texcache_entry {
    int texid;
    int imageid;
    int seq;
  } v[TEXCACHE_SIZE];
  int c;
  int seq;
};

int texcache_get(struct texcache *tc,int imageid);

#endif
