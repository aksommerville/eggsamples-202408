#ifndef HERO_INTERNAL_H
#define HERO_INTERNAL_H

#include "arrautza.h"

struct sprite_hero {
  struct sprite hdr;
  int pvinput;
  int facedir; // DIR_N,DIR_W,DIR_E,DIR_S
  int indx,indy; // Input state, digested to (-1,0,1).
};

#define SPRITE ((struct sprite_hero*)sprite)

#endif
