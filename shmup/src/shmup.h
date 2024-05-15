#ifndef SHMUP_H
#define SHMUP_H

#include <egg/egg.h>

extern int texid_sprites;
extern double alarm_clock;

int rand();
void srand_auto();

// Stars overwrites the framebuffer entirely.
void stars_init();
void stars_render();

#endif
