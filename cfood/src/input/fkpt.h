/* fkpt.h
 * Fake pointer.
 */
 
#ifndef FKPT_H
#define FKPT_H

struct fkpt;
struct bus;

//TODO

static inline void fkpt_del(struct fkpt *fkpt) {}

static inline struct fkpt *fkpt_new(struct bus *bus) { return (struct fkpt*)""; }

static inline void fkpt_event(struct fkpt *fkpt,const struct egg_event *event) {}
static inline void fkpt_update(struct fkpt *fkpt,double elapsed) {}
static inline void fkpt_render(struct fkpt *fkpt) {}

static inline void fkpt_enable(struct fkpt *fkpt,int enable) {}

#endif
