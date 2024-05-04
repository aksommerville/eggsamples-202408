/* fkpt.h
 * Fake pointer.
 */
 
#ifndef FKPT_H
#define FKPT_H

struct fkpt;
struct bus;

void fkpt_del(struct fkpt *fkpt);

struct fkpt *fkpt_new(struct bus *bus);

/* Set an image for the cursor.
 * texid zero for no fake cursor, we'll use the system's if possible.
 * Bounds (0,0,0,0) to use the entire texture.
 * (hotx,hoty) should be in (0,0)..(w-1,h-1).
 * We borrow the texture ID; caller must keep it alive.
 */
void fkpt_set_cursor(
  struct fkpt *fkpt,
  int texid,int x,int y,int w,int h,int xform,
  int hotx,int hoty
);

void fkpt_event(struct fkpt *fkpt,const struct egg_event *event);
void fkpt_update(struct fkpt *fkpt,double elapsed);
void fkpt_render(struct fkpt *fkpt);

void fkpt_enable(struct fkpt *fkpt,int enable);

#endif
