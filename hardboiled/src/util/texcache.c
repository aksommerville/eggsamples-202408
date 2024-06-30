#include <egg/egg.h>
#include "texcache.h"

/* Advance sequence number for a touched entry.
 * Not bothering to check for overflow but this would be the place if you want to.
 * (Touch 10 entries per frame at 60 Hz, you'll run about 1000 hours before overflowing).
 * So every 41 days of uptime, expect a few poor eviction decisions.
 */
 
static inline void texcache_touch_seq(struct texcache *tc,struct texcache_entry *entry) {
  entry->seq=tc->seq++;
}

/* Get texture for image.
 */
 
int texcache_get(struct texcache *tc,int imageid) {

  // If we already have it, touch it and declare victory.
  struct texcache_entry *entry=tc->v;
  int i=tc->c;
  for (;i-->0;entry++) {
    if (entry->imageid==imageid) {
      texcache_touch_seq(tc,entry);
      return entry->texid;
    }
  }
  
  // If we're not full yet, append a new texture.
  if (tc->c<TEXCACHE_SIZE) {
    entry=tc->v+tc->c++;
    if ((entry->texid=egg_texture_new())<1) {
      tc->c--;
      return 0;
    }
  // Or if we are full, evict the oldest entry.
  } else {
    entry=tc->v;
    struct texcache_entry *q=tc->v;
    for (i=tc->c;i-->0;q++) {
      if (q->seq<entry->seq) {
        entry=q;
      }
    }
    if (TEXCACHE_LOG_EVICTIONS) {
      egg_log("texcache evicting imageid=%d in favor of imageid=%d for texid=%d",entry->imageid,imageid,entry->texid);
    }
  }
  
  // Load the image.
  if (egg_texture_load_image(entry->texid,0,imageid)<0) {
    egg_log("ERROR: Failed to load image:%d",imageid);
    return 0;
  }
  entry->imageid=imageid;
  texcache_touch_seq(tc,entry);
  return entry->texid;
}
